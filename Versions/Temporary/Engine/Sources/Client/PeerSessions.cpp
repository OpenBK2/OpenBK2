#include "stdafx.h"

#include "Connection.h"
#include "ConnectionEffortInternal.h"
#include "PeerGamePacket.h"
#include "PlayGameProcessor.h"
#include "port/time.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/Net.h"
#include "Server_Client_Common/NetLogger.h"
#include "Server_Client_Common/NetSaver.h"

#include "Client_export.h"

#include <fmt/format.h>

REGISTER_SAVELOAD_CLASS( CLIENT, 94, CPeerGamePacket );

namespace
{
constexpr uint32_t PEER_HEARTBEAT_PERIOD = 1000;
} // namespace

void CPlayGameProcessor::SendSessionPacket( int nClientID,
                                            uint64_t nSequence,
                                            const std::vector<char> &data,
                                            bool bRequestAck )
{
	const auto connection = connections.find( nClientID );
	if ( connection == connections.end() )
	{
		return;
	}
	SPeerSession &session = peerSessions.at( nClientID );
	CPtr<CPeerGamePacket> packet = new CPeerGamePacket();
	packet->nGameID = nOurGameID;
	packet->nSetupID = session.nSetupID;
	packet->nSequence = nSequence;
	packet->nAcknowledged = session.nNextReceive - 1;
	packet->bRequestAck = bRequestAck;
	packet->data = data;
	connection->second->SendGamePacket( packet );
	session.bAckPending = false;
}

void CPlayGameProcessor::QueueSessionPacket( int nClientID, CNetPacket *pPacket )
{
	SPeerSession &session = peerSessions.at( nClientID );
	// Snapshot the message now: callers and broadcasts may reuse/mutate objects.
	CMemoryStream stream;
	CPtr<CNetPacket> packet = pPacket;
	{
		CPtr<IBinSaver> saver = CreateNetSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &packet );
	}
	if ( session.pendingSend.empty() )
	{
		session.dwLastAckProgress = GetCurrentTimeMilliseconds();
	}
	const uint64_t sequence = session.nNextSend++;
	auto &data = session.pendingSend[sequence];
	const char *bytes = reinterpret_cast<const char *>( stream.GetBuffer() );
	data.assign( bytes, bytes + stream.GetSize() );
	SendSessionPacket( nClientID, sequence, data );
}

void CPlayGameProcessor::ReceiveSessionPacket( CNetPacket *pRawPacket )
{
	const auto found = peerSessions.find( pRawPacket->nClientID );
	if ( found == peerSessions.end() )
	{
		PushPacket( pRawPacket );
		return;
	}
	auto *packet = dynamic_cast<CPeerGamePacket *>( pRawPacket );
	SPeerSession &session = found->second;
	if ( !packet || packet->nGameID != nOurGameID || packet->nSetupID != session.nSetupID )
	{
		return;
	}
	session.dwLastReceive = GetCurrentTimeMilliseconds();
	if ( packet->nAcknowledged > session.nLastAcknowledged && packet->nAcknowledged < session.nNextSend )
	{
		session.nLastAcknowledged = packet->nAcknowledged;
		session.dwLastAckProgress = session.dwLastReceive;
		session.pendingSend.erase( session.pendingSend.begin(), session.pendingSend.upper_bound( packet->nAcknowledged ) );
	}
	if ( packet->nSequence != 0 )
	{
		// A late packet from the old route or a replay of the uncertain suffix
		// must acknowledge again, but must never execute a command twice.
		session.bAckPending = true;
		if ( packet->nSequence >= session.nNextReceive && !packet->data.empty() )
		{
			session.pendingReceive.emplace( packet->nSequence, packet->data );
		}
		while ( !session.pendingReceive.empty() && session.pendingReceive.begin()->first == session.nNextReceive )
		{
			const auto &data = session.pendingReceive.begin()->second;
			CMemoryStream stream;
			stream.Write( data.data(), data.size() );
			stream.Seek( 0 );
			CPtr<CNetPacket> message;
			{
				CPtr<IBinSaver> saver = CreateNetSaver( &stream, SAVER_MODE_READ );
				saver->Add( 1, &message );
			}
			if ( !message )
			{
				break;
			}
			message->nClientID = found->first;
			PushPacket( message );
			++session.nNextReceive;
			session.pendingReceive.erase( session.pendingReceive.begin() );
		}
	}
	if ( packet->bRequestAck )
	{
		session.bAckPending = true;
	}
}

void CPlayGameProcessor::FlushSessionPackets( int nClientID )
{
	SPeerSession &session = peerSessions.at( nClientID );
	for ( const auto &entry : session.pendingSend )
	{
		SendSessionPacket( nClientID, entry.first, entry.second );
	}
	session.dwLastReceive = session.dwLastAckProgress = GetCurrentTimeMilliseconds();
}

void CPlayGameProcessor::PreparePeerRecovery( int nClientID )
{
	// Both ends need ordered departure notifications, including an accepting
	// peer that was asked to retry before it detected a transport failure itself.
	if ( pendingServerRemovals.insert( nClientID ).second )
	{
		pNet->SendPacket( new CThroughServerConnectionPacket( 0, nClientID, nOurGameID ) );
	}
	SPeerSession &session = peerSessions.at( nClientID );
	SPeerSetup &setup = peerSetups[nClientID];
	setup.bRecovery = true;
	setup.bInitiator = session.bInitiator;
	setup.nSetupID = session.nSetupID;
	setup.nAttempt = session.nAttempt;
	setup.szIP = session.szIP;
	setup.nPort = session.nPort;
	setup.pClientEffort = session.pClientEffort;
	setup.dwStartTime = GetCurrentTimeMilliseconds();
	// A usable relay continues carrying commands while the direct candidate is tested.
	if ( connections.count( nClientID ) && connections.at( nClientID )->GetRoute() == EPeerConnectionRoute::Direct )
	{
		connections.erase( nClientID );
	}
	if ( session.pStandby && session.pStandby->GetRoute() == EPeerConnectionRoute::Direct )
	{
		session.pStandby = nullptr;
		session.bStandbyReady = false;
	}
	session.pendingRoute = EPeerConnectionRoute::Unknown;
	session.bRecoveryRequested = true;
}

void CPlayGameProcessor::BeginPeerRecovery( int nClientID )
{
	SPeerSession &session = peerSessions.at( nClientID );
	if ( peerSetups.find( nClientID ) != peerSetups.end() )
	{
		return;
	}
	// Keep the original ordered departure notification path while reconnecting.
	if ( pendingServerRemovals.insert( nClientID ).second )
	{
		pNet->SendPacket( new CThroughServerConnectionPacket( 0, nClientID, nOurGameID ) );
	}
	if ( session.bInitiator )
	{
		if ( !session.pClientEffort )
		{
			return;
		}
		PreparePeerRecovery( nClientID );
		SPeerSetup &setup = peerSetups.at( nClientID );
		setup.pClientEffort->RetryEstablishedConnection();
		setup.nAttempt = setup.pClientEffort->GetAttempt();
		CreateConnectionEffort( setup.pClientEffort, nClientID );
		GetNetLogger()->Log( "client",
		                     fmt::format( "peer {} {}",
		                                  nClientID,
		                                  session.route == EPeerConnectionRoute::Direct
		                                      ? "unresponsive; retrying direct once before relay"
		                                      : "direct path recovered; testing it while relay remains active" ) );
	}
	else if ( !session.bRecoveryRequested )
	{
		// Preserve a single authority even if both ends notice the failure together.
		session.bRecoveryRequested = true;
		CPtr<CPeerSetupPacket> request =
				new CPeerSetupPacket( nOurGameID, session.nSetupID, session.nAttempt, CPeerSetupPacket::RequestRecovery );
		request->bRecovery = true;
		pNet->SendPacket( new CDirectPacketToClient( 0, nClientID, request ) );
	}
}

void CPlayGameProcessor::ProcessPeerSessions()
{
	const uint32_t now = GetCurrentTimeMilliseconds();
	for ( auto &entry : peerSessions )
	{
		const int nClientID = entry.first;
		SPeerSession &session = entry.second;
		ProcessPeerPaths( nClientID, now );
		if ( now - session.dwLastHeartbeat >= PEER_HEARTBEAT_PERIOD )
		{
			SendSessionPacket( nClientID, 0, {}, true );
			session.dwLastHeartbeat = now;
		}
		else if ( session.bAckPending )
		{
			SendSessionPacket( nClientID, 0, {} );
		}
	}
	failedDirectPeers.clear();
}
