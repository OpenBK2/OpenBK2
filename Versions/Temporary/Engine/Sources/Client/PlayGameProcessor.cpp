#include "stdafx.h"

#include "Connection.h"
#include "ConnectionEffort.h"
#include "ConnectionEffortInternal.h"
#include "ConnectionInternal.h"
#include "ConnectServerProcessor.h"
#include "Net/NetLowest.h"
#include "PlayGameProcessor.h"
#include "port/time.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/Net.h"
#include "Server_Client_Common/NetLogger.h"

#include "Client_export.h"

#include <cstdint>

#include <fmt/format.h>

static const uint32_t dwWaitingPacketTimeout = 300000;
const int CONNECTION_TIMEOUT = 20;
// Registration can take 20 seconds; address discovery and three peer attempts
// need at most 16 more. Leave five seconds for relay testing/route confirmation.
static const uint32_t JOIN_TIMEOUT = 45000;
static const uint32_t ROUTE_CONFIRM_TIMEOUT = 5000;

CPlayGameProcessor::CPlayGameProcessor(
		CNet *_pNet, const char *pszServerIP, const int _nNetGameVersion, const int _nServerPort, const int _nTimeOut )
	: pNet( _pNet )
	, nNetGameVersion( _nNetGameVersion )
	, szServerIP( pszServerIP )
	, nServerPort( _nServerPort )
	, nTimeOut( _nTimeOut )
{
	REGISTER_PACKET_PROCESSOR( &CPlayGameProcessor::ProcessCreateGame );
	REGISTER_PACKET_PROCESSOR( &CPlayGameProcessor::ProcessConnectGame );
	REGISTER_PACKET_PROCESSOR( &CPlayGameProcessor::ProcessMyIDPacket );
	REGISTER_PACKET_PROCESSOR( &CPlayGameProcessor::ProcessGameConnectingClientAccepted );
	REGISTER_PACKET_PROCESSOR( &CPlayGameProcessor::ProcessLeaveGame );
	REGISTER_PACKET_PROCESSOR( &CPlayGameProcessor::ProcessClientWantToConnect );
	REGISTER_PACKET_PROCESSOR( &CPlayGameProcessor::ProcessGameKilled );
	REGISTER_PACKET_PROCESSOR( &CPlayGameProcessor::ProcessAnswerConnectGame );
	REGISTER_PACKET_PROCESSOR( &CPlayGameProcessor::ProcessKickClient );
	REGISTER_PACKET_PROCESSOR( &CPlayGameProcessor::ProcessClientWasKicked );
	REGISTER_PACKET_PROCESSOR( &CPlayGameProcessor::ProcessConnectedGameID );
	REGISTER_PACKET_PROCESSOR( &CPlayGameProcessor::ProcessConnectGameFailed );
	REGISTER_PACKET_PROCESSOR( &CPlayGameProcessor::ProcessGameClientDead );

	NEffortsFactory::SetServerInfo( szServerIP, nServerPort, nNetGameVersion );
}

void CPlayGameProcessor::StartServerConnect()
{
	pAcceptGamersNet = new CNet( nNetGameVersion, nServerPort, 60 );
	CNet::SetTimeOut( CONNECTION_TIMEOUT );
	pLinksManager = new NNet::CGameLinksManager();
	pLinksManager->Init( 0 );
	pAcceptGamersNet->InitAsClient( pLinksManager );
	pConnectServerProcessor = new CConnectServerProcessor( pAcceptGamersNet, false );
	if ( serverAddress.SetInetName( szServerIP.c_str(), nServerPort ) )
	{
		CMemoryStream pwd;
		pAcceptGamersNet->ConnectGame( serverAddress, pwd );
	}
}

bool CPlayGameProcessor::ProcessCreateGame( CCreateGamePacket *pPacket )
{
	Clear();
	pConnectGamePacket = pPacket;
	StartServerConnect();

	return true;
}

bool CPlayGameProcessor::ProcessConnectGame( CConnectGamePacket *pPacket )
{
	Clear();
	pConnectGamePacket = pPacket;
	nOurGameID = pPacket->nGameID;
	StartServerConnect();

	return true;
}

bool CPlayGameProcessor::ProcessAnswerConnectGame( CAnswerConnectGame *pPacket )
{
	/*	if ( pPacket->clients.empty() )
	{
		CPtr<CNetPacket> pPacket = new CConnectGameFailed();
		PushPacket( pPacket );
		Clear();
	}
	else
	{
*/
	for ( std::list<int>::iterator iter = pPacket->clients.begin(); iter != pPacket->clients.end(); ++iter )
	{
		joiningPeers.insert( *iter );
		static uint32_t nextSetupID = GetCurrentTimeMilliseconds();
		SPeerSetup &setup = peerSetups[*iter];
		setup.bInitiator = true;
		setup.nSetupID = ++nextSetupID;
		setup.dwStartTime = GetCurrentTimeMilliseconds();
		setup.pClientEffort = new CSimpleClientEffort(
				nMyServerID, *iter, pNet, szServerIP, nServerPort, nNetGameVersion, nTimeOut, nOurGameID, setup.nSetupID );
		CreateConnectionEffort( setup.pClientEffort, *iter );
	}
	//	}
	dwJoinStartTime = GetCurrentTimeMilliseconds();

	return true;
}

bool CPlayGameProcessor::ProcessConnectGameFailed( CConnectGameFailed *pPacket )
{
	PushPacket( pPacket );
	Clear();

	return true;
}

bool CPlayGameProcessor::ProcessClientWantToConnect( CClientWantToConnect *pPacket )
{
	if ( connections.find( pPacket->nWantedClient ) != connections.end() ||
	     peerSetups.find( pPacket->nWantedClient ) != peerSetups.end() )
	{
		return true;
	}
	SPeerSetup &setup = peerSetups[pPacket->nWantedClient];
	setup.szIP = pPacket->szIP;
	setup.nPort = pPacket->nPort;
	setup.dwStartTime = GetCurrentTimeMilliseconds();

	// Use the existing server-forwarded packet format for setup only. The peer
	// needs the bound local endpoint before login, not after the UDP handshake.
	NNet::CNodeAddressSet localAddresses;
	if ( pLinksManager && pLinksManager->GetSelfAddress( &localAddresses ) )
	{
		NNet::CNodeAddress address;
		for ( int i = 0; localAddresses.GetAddress( i, &address ); ++i )
		{
			pNet->SendPacket( new CDirectPacketToClient(
					0,
					pPacket->nWantedClient,
					new CClientGameConnectInfo( 0, address.GetIP(), address.GetPort(), nMyServerID ) ) );
		}
	}

	return true;
}

bool CPlayGameProcessor::ProcessMyIDPacket( CMyIDPacket *pPacket )
{
	nMyServerID = pPacket->nMyID;
	PushPacket( pPacket );
	return true;
}

void CPlayGameProcessor::ProcessConnectServerProcessor()
{
	if ( pConnectServerProcessor )
	{
		pConnectServerProcessor->Segment();
		CPtr<CNetPacket> pPacket = 0;
		while ( pConnectServerProcessor && ( pPacket = pConnectServerProcessor->GetPacket() ) )
		{
			if ( CConnectServerPacket *pConnectPacket = dynamic_cast_ptr<CConnectServerPacket *>( pPacket ) )
			{
				ProcessConnectServerPacket( pConnectPacket );
			}
		}
	}
}

void CPlayGameProcessor::ProcessAcceptingGamersPackets()
{
	if ( pAcceptGamersNet )
	{
		while ( CPtr<CNetPacket> pPacket = pAcceptGamersNet->ReceivePacket() )
		{
			GetNetLogger()->Log( "client", fmt::format( "receive acceptgamers net {}", GetPacketInfo( pPacket ) ) );
			// wait for pAcceptGamersNet packet processed
			bool bProcessed = ProcessPacket( pPacket );

			if ( pConnectServerProcessor )
			{
				bProcessed = pConnectServerProcessor->ProcessPacket( pPacket );
			}

			TConnectionEfforts::iterator eff_iter = connectionEfforts.begin();
			while ( !bProcessed && eff_iter != connectionEfforts.end() )
			{
				CConnectionEffort *pEffort = eff_iter->second;
				bProcessed = pEffort->ProcessPacket( pPacket );
				++eff_iter;
			}

			TConnections::iterator conn_iter = connections.begin();
			while ( !bProcessed && conn_iter != connections.end() )
			{
				IConnection *pConnection = conn_iter->second;
				if ( pConnection->GetNet() == pAcceptGamersNet )
				{
					if ( pConnection->ProcessPacketFromPlayersNet( pPacket ) )
					{
						bProcessed = true;
					}
				}
				++conn_iter;
			}

			// some new client, we didn't start accepting packets from him yet
			if ( !bProcessed && pPacket->nClientID != 0 )
			{
				waitingPackets.push_back( SWaitingPacket( pPacket, GetCurrentTimeMilliseconds() ) );
			}
		}
	}

	for ( TConnections::iterator iter = connections.begin(); iter != connections.end(); ++iter )
	{
		DrainConnectionPackets( iter->second );
	}
}

void CPlayGameProcessor::DrainConnectionPackets( IConnection *pConnection, bool bServerConfirmed )
{
	if ( bServerConfirmed )
	{
		// Departure can overtake route confirmation. Deliver the already received
		// session prefix before the ordered server removal, including during recovery.
		for ( auto &entry : peerSetups )
		{
			const auto connection = connections.find( entry.first );
			if ( connection != connections.end() && connection->second.GetPtr() == pConnection )
			{
				for ( const auto &packet : entry.second.packets )
				{
					ReceiveSessionPacket( packet );
				}
				entry.second.packets.clear();
			}
		}
	}
	while ( CPtr<CNetPacket> pPacket = pConnection->GetPacket() )
	{
		if ( CGameClientRemoved *pRemoved = dynamic_cast<CGameClientRemoved *>( pPacket.GetPtr() ) )
		{
			if ( peerSessions.find( pRemoved->nClientID ) != peerSessions.end() && !bServerConfirmed )
			{
				failedDirectPeers.insert( pRemoved->nClientID );
				continue;
			}
			const auto setup = peerSetups.find( pRemoved->nClientID );
			if ( setup != peerSetups.end() )
			{
				setup->second.bTested = false;
				continue;
			}
			// LOGOUT on a direct channel can overtake the final command prefix
			// and self-departure relayed through the lobby. Subscribe to the same
			// server stream instead of choosing a fallback cutoff prematurely.
			if ( !bServerConfirmed && pendingServerRemovals.insert( pRemoved->nClientID ).second )
			{
				pNet->SendPacket( new CThroughServerConnectionPacket( 0, pRemoved->nClientID, nOurGameID ) );
			}
			continue;
		}
		if ( dynamic_cast<CNewGameClient *>( pPacket.GetPtr() ) )
		{
			const auto setup = peerSetups.find( pPacket->nClientID );
			if ( setup != peerSetups.end() )
			{
				setup->second.bTested = true;
				continue;
			}
			joiningPeers.erase( pPacket->nClientID );
		}
		const auto setup = peerSetups.find( pPacket->nClientID );
		if ( setup != peerSetups.end() && !bServerConfirmed )
		{
			setup->second.packets.push_back( pPacket );
			continue;
		}
		ReceiveSessionPacket( pPacket );
	}
}

void CPlayGameProcessor::ProcessEfforts()
{
	std::list<CPtr<CConnectionEffort>> success, failed;

	for ( TConnectionEfforts::iterator iter = connectionEfforts.begin(); iter != connectionEfforts.end(); ++iter )
	{
		CConnectionEffort *pEffort = iter->second;
		pEffort->Segment();

		const CConnectionEffort::EState eState = pEffort->GetState();
		if ( eState == CConnectionEffort::ES_SUCCESS )
		{
			success.push_back( pEffort );
		}
		if ( eState == CConnectionEffort::ES_FAILED )
		{
			failed.push_back( pEffort );
		}
	}

	for ( std::list<CPtr<CConnectionEffort>>::iterator iter = success.begin(); iter != success.end(); ++iter )
	{
		CPtr<CConnectionEffort> pEffort = *iter;
		const int nClientServerID = pEffort->GetClientServerID();
		connectionEfforts.erase( nClientServerID );

		connections[nClientServerID] = pEffort->CreateConnection();
		IConnection *pConnection = connections[nClientServerID];

		// The identity and the first game packets can arrive in one receive batch.
		// Replay buffered packets in order as soon as the accepting connection exists.
		if ( pConnection->GetNet() == pAcceptGamersNet )
		{
			for ( auto packet = waitingPackets.begin(); packet != waitingPackets.end(); )
			{
				if ( pConnection->ProcessPacketFromPlayersNet( packet->pPacket ) )
				{
					packet = waitingPackets.erase( packet );
				}
				else
				{
					++packet;
				}
			}
			DrainConnectionPackets( pConnection );
		}
	}

	for ( std::list<CPtr<CConnectionEffort>>::iterator iter = failed.begin(); iter != failed.end(); ++iter )
	{
		CPtr<CConnectionEffort> pEffort = *iter;
		// Taken before the next effort is made, because it is the id this
		// failed effort was for and CreateConnectionEffort needs it to report
		// the client as removed when there is no next effort to try.
		const int nClientServerID = pEffort->GetClientServerID();
		connectionEfforts.erase( nClientServerID );
		if ( peerSetups.find( nClientServerID ) != peerSetups.end() )
		{
			// Retry/fallback belongs to the initiating setup state machine.
			continue;
		}

		CPtr<CConnectionEffort> pNextEffort = pEffort->CreateNextEffort( nOurGameID );
		if ( !pNextEffort && joiningPeers.find( nClientServerID ) != joiningPeers.end() )
		{
			// A failed join is not a departure of an already assigned player.
			// Release the server room and send the UI's actual failure event.
			pNet->SendPacket( new CLeaveGamePacket( 0, nOurGameID ) );
			Clear();
			PushPacket( new CConnectGameFailed() );
			return;
		}
		CreateConnectionEffort( pNextEffort, nClientServerID );
	}
}

bool CPlayGameProcessor::Segment()
{
	ProcessConnectServerProcessor();
	ProcessAcceptingGamersPackets();

	ProcessEfforts();
	ProcessPeerSetups();
	ProcessPeerSessions();
	if ( !joiningPeers.empty() && GetCurrentTimeMilliseconds() - dwJoinStartTime > JOIN_TIMEOUT )
	{
		// Also bound the final connection-test exchange, after identity succeeds.
		pNet->SendPacket( new CLeaveGamePacket( 0, nOurGameID ) );
		Clear();
		PushPacket( new CConnectGameFailed() );
	}

	uint32_t dwCurTime = GetCurrentTimeMilliseconds();
	std::list<SWaitingPacket>::iterator iter = waitingPackets.begin();
	while ( iter != waitingPackets.end() )
	{
		const SWaitingPacket &packet = *iter;
		if ( dwCurTime - packet.dwStartTime > dwWaitingPacketTimeout )
		{
			iter = waitingPackets.erase( iter );
		}
		else
		{
			++iter;
		}
	}

	std::list<int> deadConnections;
	for ( TConnections::iterator conn_iter = connections.begin(); conn_iter != connections.end(); ++conn_iter )
	{
		IConnection *pConnection = conn_iter->second;
		if ( !pConnection->IsAlive() )
		{
			deadConnections.push_back( conn_iter->first );
		}
	}

	for ( std::list<int>::iterator iter = deadConnections.begin(); iter != deadConnections.end(); ++iter )
	{
		connections.erase( *iter );
	}

	return true;
}

void CPlayGameProcessor::SendPeerSetup( int nClientID, CPeerSetupPacket::EAction action )
{
	const SPeerSetup &setup = peerSetups.at( nClientID );
	CPtr<CPeerSetupPacket> packet = new CPeerSetupPacket( nOurGameID, setup.nSetupID, setup.nAttempt, action );
	packet->bRecovery = setup.bRecovery;
	pNet->SendPacket( new CDirectPacketToClient( 0, nClientID, packet ) );
}

void CPlayGameProcessor::SelectRelay( int nClientID )
{
	SPeerSetup &setup = peerSetups.at( nClientID );
	setup.route = EPeerConnectionRoute::Relay;
	setup.bTested = false;
	setup.bConfirmed = false;
	setup.packets.clear();
	setup.dwStartTime = GetCurrentTimeMilliseconds();
	connectionEfforts.erase( nClientID );
	connections.erase( nClientID );
	setup.pClientEffort = nullptr;
	if ( setup.bRecovery )
	{
		peerSessions.at( nClientID ).pClientEffort = nullptr;
	}
	if ( setup.bInitiator )
	{
		// Selection precedes the relay test on the same ordered lobby stream.
		SendPeerSetup( nClientID, CPeerSetupPacket::SelectRelay );
	}
	pNet->SendPacket( new CThroughServerConnectionPacket( 0, nClientID, nOurGameID ) );
	connections[nClientID] = new CThroughServerConnection( nClientID, pNet );
	GetNetLogger()->Log(
			"client", fmt::format( "peer {} selecting server relay after {} direct attempts", nClientID, setup.nAttempt ) );
}

void CPlayGameProcessor::ProcessPeerSetup( int nClientID, const CPeerSetupPacket &packet )
{
	if ( packet.nGameID != nOurGameID || packet.nAttempt < 1 )
	{
		return;
	}
	if ( packet.bRecovery )
	{
		const auto session = peerSessions.find( nClientID );
		if ( session == peerSessions.end() || session->second.nSetupID != packet.nSetupID ||
		     session->second.route != EPeerConnectionRoute::Direct )
		{
			return;
		}
		if ( packet.eAction == CPeerSetupPacket::RequestRecovery )
		{
			if ( session->second.bInitiator && packet.nAttempt == session->second.nAttempt )
			{
				BeginPeerRecovery( nClientID );
			}
			return;
		}
		if ( !session->second.bInitiator && peerSetups.find( nClientID ) == peerSetups.end() )
		{
			if ( packet.eAction == CPeerSetupPacket::BeginAttempt && packet.nAttempt == session->second.nAttempt + 1 )
			{
				PreparePeerRecovery( nClientID );
			}
			else if ( packet.eAction == CPeerSetupPacket::SelectRelay && packet.nAttempt == session->second.nAttempt )
			{
				// The direct retry succeeded here, but its confirmation missed the
				// initiator's deadline. Session sequences make this late switch safe.
				PreparePeerRecovery( nClientID );
				SelectRelay( nClientID );
				return;
			}
		}
	}
	const auto found = peerSetups.find( nClientID );
	// Only accept setup from a server-introduced room member, for this room.
	if ( found == peerSetups.end() || found->second.bRecovery != packet.bRecovery ||
	     ( !packet.bRecovery && packet.nAttempt > CSimpleClientEffort::DIRECT_ATTEMPTS ) )
	{
		return;
	}
	SPeerSetup &setup = found->second;
	if ( !setup.bInitiator && packet.eAction == CPeerSetupPacket::BeginAttempt &&
	     setup.route == EPeerConnectionRoute::Unknown && packet.nAttempt > setup.nAttempt &&
	     ( setup.nAttempt == 0 || setup.nSetupID == packet.nSetupID ) )
	{
		setup.nSetupID = packet.nSetupID;
		setup.nAttempt = packet.nAttempt;
		setup.dwStartTime = GetCurrentTimeMilliseconds();
		setup.bTested = false;
		setup.packets.clear();
		connections.erase( nClientID );
		connectionEfforts.erase( nClientID );
		CreateConnectionEffort( new CSimpleServerEffort( nMyServerID,
		                                                 nClientID,
		                                                 setup.szIP,
		                                                 setup.nPort,
		                                                 pNet,
		                                                 pAcceptGamersNet,
		                                                 pLinksManager,
		                                                 nOurGameID,
		                                                 setup.nSetupID,
		                                                 setup.nAttempt ),
		                        nClientID );
		SendPeerSetup( nClientID, CPeerSetupPacket::AcceptAttempt );
		return;
	}
	if ( setup.nSetupID != packet.nSetupID || setup.nAttempt != packet.nAttempt )
	{
		return;
	}
	if ( setup.bInitiator )
	{
		if ( packet.eAction == CPeerSetupPacket::AcceptAttempt && setup.pClientEffort )
		{
			setup.pClientEffort->AcceptAttempt( packet );
		}
		else if ( ( packet.eAction == CPeerSetupPacket::ConfirmDirect && setup.route == EPeerConnectionRoute::Direct ) ||
		          ( packet.eAction == CPeerSetupPacket::ConfirmRelay && setup.route == EPeerConnectionRoute::Relay ) )
		{
			setup.bConfirmed = true;
		}
	}
	else if ( setup.route == EPeerConnectionRoute::Unknown ||
	          ( setup.bRecovery && setup.route == EPeerConnectionRoute::Direct ) )
	{
		if ( packet.eAction == CPeerSetupPacket::SelectRelay &&
		     ( setup.bRecovery || setup.nAttempt == CSimpleClientEffort::DIRECT_ATTEMPTS ) )
		{
			SelectRelay( nClientID );
		}
		else if ( packet.eAction == CPeerSetupPacket::SelectDirect )
		{
			setup.route = EPeerConnectionRoute::Direct;
			setup.dwStartTime = GetCurrentTimeMilliseconds();
		}
	}
}

void CPlayGameProcessor::FinishPeerSetup( int nClientID )
{
	SPeerSetup &setup = peerSetups.at( nClientID );
	GetNetLogger()->Log( "client",
	                     fmt::format( "peer {} connected via {}",
	                                  nClientID,
	                                  setup.route == EPeerConnectionRoute::Direct ? "direct UDP" : "server relay" ) );
	joiningPeers.erase( nClientID );
	SPeerSession &session = peerSessions[nClientID];
	session.bInitiator = setup.bInitiator;
	session.nSetupID = setup.nSetupID;
	session.nAttempt = setup.nAttempt;
	session.szIP = setup.szIP;
	session.nPort = setup.nPort;
	session.pClientEffort = setup.pClientEffort;
	session.route = setup.route;
	session.bRecoveryRequested = false;
	session.dwLastReceive = session.dwLastAckProgress = session.dwLastHeartbeat = GetCurrentTimeMilliseconds();
	if ( !setup.bRecovery )
	{
		PushPacket( new CNewGameClient( nClientID ) );
	}
	for ( const auto &packet : setup.packets )
	{
		ReceiveSessionPacket( packet );
	}
	peerSetups.erase( nClientID );
	FlushSessionPackets( nClientID );
}

void CPlayGameProcessor::ProcessPeerSetups()
{
	std::vector<int> ready, failed;
	for ( auto &entry : peerSetups )
	{
		const int nClientID = entry.first;
		SPeerSetup &setup = entry.second;
		if ( setup.bInitiator && setup.route == EPeerConnectionRoute::Unknown )
		{
			CSimpleClientEffort *effort = setup.pClientEffort;
			setup.nAttempt = effort->GetAttempt();
			if ( setup.bTested )
			{
				setup.route = EPeerConnectionRoute::Direct;
				setup.dwStartTime = GetCurrentTimeMilliseconds();
				SendPeerSetup( nClientID, CPeerSetupPacket::SelectDirect );
			}
			else if ( effort->GetState() == CConnectionEffort::ES_FAILED || effort->IsAttemptExpired() )
			{
				if ( setup.nAttempt == 0 )
				{
					// Registration/introduction failed, so no peer handshake is possible.
					failed.push_back( nClientID );
				}
				else if ( setup.nAttempt == effort->GetAttemptLimit() )
				{
					SelectRelay( nClientID );
				}
				else
				{
					connections.erase( nClientID );
					setup.packets.clear();
					effort->StartNextPeerConnection();
					setup.nAttempt = effort->GetAttempt();
					CreateConnectionEffort( effort, nClientID );
				}
			}
		}
		if ( setup.route != EPeerConnectionRoute::Unknown && setup.bTested )
		{
			if ( !setup.bInitiator )
			{
				SendPeerSetup( nClientID,
				               setup.route == EPeerConnectionRoute::Direct ? CPeerSetupPacket::ConfirmDirect
				                                                           : CPeerSetupPacket::ConfirmRelay );
				setup.bConfirmed = true;
			}
			if ( setup.bConfirmed )
			{
				ready.push_back( nClientID );
				continue;
			}
		}
		if ( setup.bRecovery && setup.bInitiator && setup.route == EPeerConnectionRoute::Direct &&
		     setup.pClientEffort->IsAttemptExpired() )
		{
			SelectRelay( nClientID );
		}
		const uint32_t timeout = setup.route == EPeerConnectionRoute::Unknown ? JOIN_TIMEOUT : ROUTE_CONFIRM_TIMEOUT;
		// During a match the existing lag/departure policy remains authoritative
		// if the lobby itself is unresponsive. Keep queued commands until it recovers.
		if ( !setup.bRecovery && GetCurrentTimeMilliseconds() - setup.dwStartTime >= timeout )
		{
			failed.push_back( nClientID );
		}
	}
	for ( int nClientID : ready )
	{
		FinishPeerSetup( nClientID );
	}
	for ( int nClientID : failed )
	{
		if ( joiningPeers.find( nClientID ) != joiningPeers.end() )
		{
			pNet->SendPacket( new CLeaveGamePacket( 0, nOurGameID ) );
			Clear();
			PushPacket( new CConnectGameFailed() );
			return;
		}
		peerSetups.erase( nClientID );
		connectionEfforts.erase( nClientID );
		connections.erase( nClientID );
	}
}

EPeerConnectionRoute CPlayGameProcessor::GetPeerRoute( int nClientID ) const
{
	const auto connection = connections.find( nClientID );
	return connection != connections.end() && peerSetups.find( nClientID ) == peerSetups.end()
	           ? connection->second->GetRoute()
	           : EPeerConnectionRoute::Unknown;
}

bool CPlayGameProcessor::CreateConnectionEffort( CConnectionEffort *pEffort, const int nClientServerID )
{
	if ( pEffort != 0 )
	{
		// Still read off the effort rather than from the argument, so this
		// branch keeps registering exactly the id it always did.
		connectionEfforts[pEffort->GetClientServerID()] = pEffort;

		std::list<SWaitingPacket>::iterator iter = waitingPackets.begin();
		while ( iter != waitingPackets.end() )
		{
			const SWaitingPacket &packet = *iter;
			const bool bProcessed = pEffort->ProcessPacket( packet.pPacket );
			if ( bProcessed )
			{
				iter = waitingPackets.erase( iter );
			}
			else
			{
				++iter;
			}
		}

		return true;
	}
	else
	{
		// pEffort is null in this branch, which the test above has just
		// established, so the id cannot come from it. It used to be read
		// through the null pointer: GCC proved that and -Wnonnull reported it.
		// Whatever the dereference happened to return is what was announced as
		// removed and erased from both maps, so the wrong client could be
		// dropped. The caller's id is the right one.
		PushPacket( new CGameClientRemoved( nClientServerID ) );
		connectionEfforts.erase( nClientServerID );
		connections.erase( nClientServerID );

		return false;
	}
}

void CPlayGameProcessor::ProcessConnectServerPacket( CConnectServerPacket *pPacket )
{
	if ( pPacket->eConnectionState == CConnectServerPacket::ECS_SUCCESS )
	{
		CPtr<CNetPacket> pNewGameConnectPacket = new CNewGameConnectingClient( 0, nMyServerID, 0 );
		pAcceptGamersNet->SendPacket( pNewGameConnectPacket );
		GetNetLogger()->Log( "client", fmt::format( "send acceptgamers net {}", GetPacketInfo( pPacket ) ) );

		pConnectServerProcessor = 0;
	}
	else
	{
		PushPacket( new CConnectGameFailed() );
		pConnectServerProcessor = 0;
	}
}

bool CPlayGameProcessor::ProcessGameConnectingClientAccepted( CGameConnectingClientAccepted *pPacket )
{
	pNet->SendPacket( pConnectGamePacket );

	pAcceptGamersNet = 0;
	pLinksManager->BanAddress( *( serverAddress.GetSockAddr() ) );
	pAcceptGamersNet = new CNet( nNetGameVersion, nServerPort, 60 );
	pAcceptGamersNet->InitAsServer( pLinksManager );
	pAcceptGamersNet->StartGame();
	pAcceptGamersNet->StartNewPlayerAccept();

	return true;
}

bool CPlayGameProcessor::ProcessClientWasKicked( CGameClientWasKicked *pPacket )
{
	TConnections::iterator iter = connections.find( pPacket->nKicked );
	if ( iter != connections.end() )
	{
		// Server notifications can follow forwarded game packets in the same
		// receive batch. Deliver those packets before destroying their queue.
		DrainConnectionPackets( iter->second, true );
		connections.erase( iter );
	}
	PushPacket( pPacket );
	connectionEfforts.erase( pPacket->nKicked );
	pendingServerRemovals.erase( pPacket->nKicked );
	peerSetups.erase( pPacket->nKicked );
	peerSessions.erase( pPacket->nKicked );
	failedDirectPeers.erase( pPacket->nKicked );

	return true;
}

void CPlayGameProcessor::SendGamePacket( CNetPacket *pPacket, bool bBroadcast )
{
	CPtr<CNetPacket> pRemPacket = pPacket;

	if ( !bBroadcast )
	{
		if ( peerSessions.find( pPacket->nClientID ) != peerSessions.end() )
		{
			QueueSessionPacket( pPacket->nClientID, pPacket );
			return;
		}
		TConnections::iterator iter = connections.find( pPacket->nClientID );
		if ( iter != connections.end() && peerSetups.find( iter->first ) == peerSetups.end() )
		{
			iter->second->SendGamePacket( pPacket );
		}
	}
	else
	{
		for ( const auto &entry : peerSessions )
		{
			QueueSessionPacket( entry.first, pPacket );
		}
		for ( TConnections::iterator iter = connections.begin(); iter != connections.end(); ++iter )
		{
			if ( peerSetups.find( iter->first ) == peerSetups.end() &&
			     peerSessions.find( iter->first ) == peerSessions.end() )
			{
				iter->second->SendGamePacket( pPacket );
			}
		}
	}
}

void CPlayGameProcessor::Clear()
{
	pAcceptGamersNet = 0;
	connections.clear();
	peerSetups.clear();
	peerSessions.clear();
	failedDirectPeers.clear();
	joiningPeers.clear();
	pendingServerRemovals.clear();
	connectionEfforts.clear();
	pConnectGamePacket = 0;
	pConnectServerProcessor = 0;
	waitingPackets.clear();
}

bool CPlayGameProcessor::ProcessLeaveGame( CLeaveGamePacket *pPacket )
{
	Clear();
	return false;
}

bool CPlayGameProcessor::ProcessPacket( CNetPacket *pPacket )
{
	bool bProcessed = false;

	TConnectionEfforts::iterator iter = connectionEfforts.begin();
	while ( !bProcessed && iter != connectionEfforts.end() )
	{
		bProcessed = iter->second->ProcessPacket( pPacket );
		++iter;
	}

	TConnections::iterator conn_iter = connections.begin();
	while ( !bProcessed && conn_iter != connections.end() )
	{
		bProcessed = conn_iter->second->ProcessPacketFromServer( pPacket );
		++conn_iter;
	}

	return bProcessed ? true : CPacketProcessor::ProcessPacket( pPacket );
}

bool CPlayGameProcessor::ProcessPeerSetupEnvelope( CNetPacket *pPacket )
{
	auto *envelope = dynamic_cast<CDirectPacketToClient *>( pPacket );
	if ( !envelope )
	{
		return false;
	}
	auto *address = dynamic_cast<CClientGameConnectInfo *>( envelope->pPacket.GetPtr() );
	if ( auto *setup = dynamic_cast<CPeerSetupPacket *>( envelope->pPacket.GetPtr() ) )
	{
		ProcessPeerSetup( envelope->nClient, *setup );
		return true;
	}
	if ( !address )
	{
		return false;
	}
	// Preserve the server-authenticated sender, and consume only setup hints.
	const auto effort = connectionEfforts.find( envelope->nClient );
	if ( envelope->nClient == address->nClient2Connect && effort != connectionEfforts.end() )
	{
		effort->second->AddPeerLocalAddress( address->szIP, address->nPort );
	}
	return true;
}

bool CPlayGameProcessor::ProcessKickClient( CGameKickClient *pPacket )
{
	if ( nOurGameID == pPacket->nGameID )
	{
		pNet->SendPacket( pPacket );
	}

	return true;
}

bool CPlayGameProcessor::ProcessGameKilled( CGameKilled *pPacket )
{
	if ( nOurGameID == pPacket->nGame )
	{
		// As with a single-client departure, the server can kill the room in
		// the same batch that delivered its final relayed game packets.
		for ( TConnections::iterator iter = connections.begin(); iter != connections.end(); ++iter )
		{
			DrainConnectionPackets( iter->second, true );
		}
		Clear();
		PushPacket( pPacket );
	}

	return true;
}

bool CPlayGameProcessor::ProcessConnectedGameID( CConnectedGameID *pPacket )
{
	nOurGameID = pPacket->nGameID;
	PushPacket( pPacket );

	return true;
}

bool CPlayGameProcessor::ProcessGameClientDead( CGameClientDead *pPacket )
{
	if ( peerSetups.find( pPacket->nDeadClient ) != peerSetups.end() &&
	     peerSessions.find( pPacket->nDeadClient ) == peerSessions.end() )
	{
		if ( joiningPeers.find( pPacket->nDeadClient ) != joiningPeers.end() )
		{
			pNet->SendPacket( new CLeaveGamePacket( 0, nOurGameID ) );
			Clear();
			PushPacket( new CConnectGameFailed() );
		}
		else
		{
			peerSetups.erase( pPacket->nDeadClient );
			connectionEfforts.erase( pPacket->nDeadClient );
			connections.erase( pPacket->nDeadClient );
		}
		return true;
	}
	TConnections::iterator iter = connections.find( pPacket->nDeadClient );
	if ( iter == connections.end() && pendingServerRemovals.find( pPacket->nDeadClient ) == pendingServerRemovals.end() )
	{
		return false;
	}
	if ( iter != connections.end() )
	{
		// A relay may already hold the departing host's final drop or outcome
		// packet. Preserve its order before publishing the removal notification.
		DrainConnectionPackets( iter->second, true );
		connections.erase( iter );
	}
	// The direct connection may already have been retired by Segment. Its
	// pending subscription still authorizes this ordered removal notification.
	pendingServerRemovals.erase( pPacket->nDeadClient );
	connectionEfforts.erase( pPacket->nDeadClient );
	peerSetups.erase( pPacket->nDeadClient );
	peerSessions.erase( pPacket->nDeadClient );
	failedDirectPeers.erase( pPacket->nDeadClient );
	PushPacket( new CGameClientRemoved( pPacket->nDeadClient ) );
	return true;
}

void CPlayGameProcessor::TogglePauseAcceptGamersNet( bool bPause )
{
	if ( pAcceptGamersNet )
	{
		pAcceptGamersNet->DebugTogglePause( bPause );
	}
}

void CPlayGameProcessor::TogglePauseConnectGamer( const int nGamer, bool bPause )
{
	TConnections::iterator iter = connections.find( nGamer );
	if ( iter != connections.end() )
	{
		iter->second->TogglePauseNet( bPause );
	}
}

BASIC_REGISTER_CLASS( CLIENT, CPlayGameProcessor )
