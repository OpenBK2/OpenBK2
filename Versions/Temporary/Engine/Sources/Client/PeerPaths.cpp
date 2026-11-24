#include "stdafx.h"

#include "Connection.h"
#include "ConnectionEffortInternal.h"
#include "ConnectionInternal.h"
#include "PeerPathProbePacket.h"
#include "PlayGameProcessor.h"
#include "port/time.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/Net.h"
#include "Server_Client_Common/NetLogger.h"

#include "Client_export.h"

#include <random>

#include <fmt/format.h>

REGISTER_SAVELOAD_CLASS( CLIENT, 95, CPeerPathProbePacket );

std::vector<CPtr<IConnection>> CPlayGameProcessor::GetConnections( int nClientID ) const
{
	std::vector<CPtr<IConnection>> result;
	const auto add = [&]( int peer, IConnection *connection )
	{
		if ( connection && ( nClientID == -1 || nClientID == peer ) &&
		     std::find( result.begin(), result.end(), connection ) == result.end() )
		{
			result.push_back( connection );
		}
	};
	for ( const auto &entry : connections )
	{
		add( entry.first, entry.second );
	}
	for ( const auto &entry : peerSetups )
	{
		add( entry.first, entry.second.pConnection );
	}
	for ( const auto &entry : peerSessions )
	{
		add( entry.first, entry.second.pStandby );
	}
	return result;
}

NNet::CGameLinksManager *CPlayGameProcessor::GetProbeLinks( const SPeerSession &session ) const
{
	return session.bInitiator ? ( session.pClientEffort ? session.pClientEffort->GetLinks() : nullptr )
	                          : pLinksManager.GetPtr();
}

void CPlayGameProcessor::ConfigurePeerProbes( int nClientID )
{
	SPeerSession &session = peerSessions.at( nClientID );
	NNet::CGameLinksManager *links = GetProbeLinks( session );
	if ( !links )
	{
		return;
	}
	if ( !session.probeCookie )
	{
		std::random_device random;
		session.probeCookie = ( static_cast<uint64_t>( random() ) << 32 ) | random();
		session.probeCookie |= 1;
		session.nextProbe = session.probeCookie;
	}
	std::vector<NNet::CNodeAddress> addresses;
	NNet::CNodeAddress address;
	if ( session.route == EPeerConnectionRoute::Direct )
	{
		if ( connections.at( nClientID )->GetPeerAddress( &address ) )
		{
			addresses.push_back( address );
		}
		else
		{
			// The driver can retire this peer between its connection test and route confirmation.
			failedDirectPeers.insert( nClientID );
		}
	}
	if ( session.pClientEffort )
	{
		const auto &targets = session.pClientEffort->GetPeerTargets();
		addresses.insert( addresses.end(), targets.begin(), targets.end() );
	}
	else if ( address.SetInetName( session.szIP.c_str(), session.nPort ) )
	{
		addresses.push_back( address );
	}
	links->ConfigureProbe( nClientID, nMyServerID, nOurGameID, session.nSetupID, session.probeCookie, addresses );
	if ( session.route == EPeerConnectionRoute::Direct && !session.pStandby )
	{
		// Test the alternate game transport now; no gameplay is sent there until agreement.
		pendingServerRemovals.insert( nClientID );
		pNet->SendPacket( new CThroughServerConnectionPacket( 0, nClientID, nOurGameID ) );
		session.pStandby = new CThroughServerConnection( nClientID, pNet );
		session.bStandbyReady = false;
	}
}

void CPlayGameProcessor::RemovePeerProbes( int nClientID )
{
	const auto session = peerSessions.find( nClientID );
	if ( session != peerSessions.end() )
	{
		if ( auto *links = GetProbeLinks( session->second ) )
		{
			links->RemoveProbe( nClientID );
		}
	}
}

void CPlayGameProcessor::ProcessPeerProbe( int nClientID, const CPeerPathProbePacket &packet )
{
	const auto found = peerSessions.find( nClientID );
	if ( found == peerSessions.end() || packet.room != nOurGameID || packet.setup != found->second.nSetupID )
	{
		return;
	}
	SPeerSession &session = found->second;
	const uint32_t now = GetCurrentTimeMilliseconds();
	if ( auto *links = GetProbeLinks( session ) )
	{
		links->SetRemoteCookie( nClientID, packet.cookie );
	}
	if ( packet.reply )
	{
		if ( session.relayQuality.Reply( packet.nonce, now ) )
		{
			session.remoteDirectScore = packet.directScore;
			session.remoteRelayScore = packet.relayScore;
			session.remoteReportTime = now;
		}
	}
	else
	{
		CPtr<CPeerPathProbePacket> reply = new CPeerPathProbePacket();
		reply->room = nOurGameID;
		reply->setup = session.nSetupID;
		reply->nonce = packet.nonce;
		reply->cookie = session.probeCookie;
		reply->reply = true;
		reply->directScore = session.directQuality.Score( now );
		reply->relayScore = session.relayQuality.Score( now );
		pNet->SendPacket( new CDirectPacketToClient( 0, nClientID, reply ) );
	}
}

bool CPlayGameProcessor::SwitchSessionRoute( int nClientID, EPeerConnectionRoute route )
{
	SPeerSession &session = peerSessions.at( nClientID );
	if ( session.route == route )
	{
		return true;
	}
	if ( !session.pStandby || !session.bStandbyReady || !session.pStandby->IsAlive() ||
	     session.pStandby->GetRoute() != route )
	{
		return false;
	}
	CPtr<IConnection> previous = connections[nClientID];
	connections[nClientID] = session.pStandby;
	session.pStandby = previous;
	session.bStandbyReady = previous && previous->IsAlive();
	session.route = route;
	session.lastSwitch = GetCurrentTimeMilliseconds();
	session.bBetter = false;
	// Both transports are still drained. Session ACKs/deduplication protect the overlap.
	FlushSessionPackets( nClientID );
	GetNetLogger()->Log( "client",
	                     fmt::format( "peer {} route changed to {} after path validation",
	                                  nClientID,
	                                  route == EPeerConnectionRoute::Direct ? "direct UDP" : "server relay" ) );
	return true;
}

void CPlayGameProcessor::RequestRouteSwitch( int nClientID, EPeerConnectionRoute route )
{
	SPeerSession &session = peerSessions.at( nClientID );
	if ( !session.bInitiator || session.pendingRoute != EPeerConnectionRoute::Unknown )
	{
		return;
	}
	session.pendingRoute = route;
	CPtr<CPeerSetupPacket> request =
			new CPeerSetupPacket( nOurGameID, session.nSetupID, session.nAttempt, CPeerSetupPacket::SwitchRoute );
	request->nRouteSerial = ++session.routeSerial;
	request->route = route;
	pNet->SendPacket( new CDirectPacketToClient( 0, nClientID, request ) );
	const uint32_t now = GetCurrentTimeMilliseconds();
	GetNetLogger()->Log( "client",
	                     fmt::format( "peer {} proposing {}: direct score {}, relay score {}, command ACK age {} ms",
	                                  nClientID,
	                                  route == EPeerConnectionRoute::Direct ? "direct UDP" : "server relay",
	                                  session.directQuality.Score( now ),
	                                  session.relayQuality.Score( now ),
	                                  session.pendingSend.empty() ? 0 : now - session.dwLastAckProgress ) );
}

void CPlayGameProcessor::ProcessRouteSwitch( int nClientID, const CPeerSetupPacket &packet )
{
	const auto found = peerSessions.find( nClientID );
	if ( found == peerSessions.end() || peerSetups.count( nClientID ) || packet.nSetupID != found->second.nSetupID ||
	     packet.nAttempt != found->second.nAttempt ||
	     ( packet.route != EPeerConnectionRoute::Direct && packet.route != EPeerConnectionRoute::Relay ) )
	{
		return;
	}
	SPeerSession &session = found->second;
	if ( !session.bInitiator && packet.eAction == CPeerSetupPacket::SwitchRoute &&
	     packet.nRouteSerial > session.routeSerial )
	{
		session.routeSerial = packet.nRouteSerial;
		const bool switched = SwitchSessionRoute( nClientID, packet.route );
		CPtr<CPeerSetupPacket> reply =
				new CPeerSetupPacket( nOurGameID,
		                          session.nSetupID,
		                          session.nAttempt,
		                          switched ? CPeerSetupPacket::ConfirmRoute : CPeerSetupPacket::RejectRoute );
		reply->nRouteSerial = packet.nRouteSerial;
		reply->route = packet.route;
		pNet->SendPacket( new CDirectPacketToClient( 0, nClientID, reply ) );
	}
	else if ( session.bInitiator && packet.eAction == CPeerSetupPacket::RejectRoute &&
	          packet.nRouteSerial == session.routeSerial && packet.route == session.pendingRoute )
	{
		// A candidate may close between measurement and selection. Keep the current route.
		session.pendingRoute = EPeerConnectionRoute::Unknown;
		session.bBetter = false;
		session.lastSwitch = GetCurrentTimeMilliseconds();
	}
	else if ( session.bInitiator && packet.eAction == CPeerSetupPacket::ConfirmRoute &&
	          packet.nRouteSerial == session.routeSerial && packet.route == session.pendingRoute )
	{
		if ( SwitchSessionRoute( nClientID, packet.route ) )
		{
			session.pendingRoute = EPeerConnectionRoute::Unknown;
		}
		else if ( packet.route == EPeerConnectionRoute::Direct )
		{
			// The other end committed, but our standby closed before confirmation.
			// Keep sending on relay and coordinate a fresh direct handshake.
			session.pendingRoute = EPeerConnectionRoute::Unknown;
			BeginPeerRecovery( nClientID );
		}
	}
}

void CPlayGameProcessor::ProcessPeerPaths( int nClientID, uint32_t now )
{
	SPeerSession &session = peerSessions.at( nClientID );
	auto &direct = session.directQuality;
	auto &relay = session.relayQuality;
	NNet::CGameLinksManager *links = GetProbeLinks( session );
	if ( links )
	{
		for ( const auto &reply : links->TakeProbeReplies( nClientID ) )
		{
			direct.Reply( reply.first, reply.second );
		}
	}
	direct.Expire( now );
	relay.Expire( now );
	if ( links && now - session.lastProbe >= SPeerPathQuality::PROBE_PERIOD )
	{
		session.lastProbe = now;
		const uint64_t nonce = ++session.nextProbe;
		direct.pending[nonce] = relay.pending[nonce] = now;
		links->SendProbe( nClientID, nonce );
		CPtr<CPeerPathProbePacket> probe = new CPeerPathProbePacket();
		probe->room = nOurGameID;
		probe->setup = session.nSetupID;
		probe->nonce = nonce;
		probe->cookie = session.probeCookie;
		probe->directScore = direct.Score( now );
		probe->relayScore = relay.Score( now );
		pNet->SendPacket( new CDirectPacketToClient( 0, nClientID, probe ) );
	}
	if ( peerSetups.count( nClientID ) )
	{
		return;
	}
	const bool directActive = session.route == EPeerConnectionRoute::Direct;
	const bool noDirectTraffic = now - session.dwLastReceive >= direct.SilenceTimeout() &&
	                             ( !direct.replies || now - direct.lastReply >= direct.SilenceTimeout() );
	if ( directActive && ( failedDirectPeers.count( nClientID ) || noDirectTraffic ) )
	{
		// One retry only for actual silence/transport closure, not a blocked ordered queue.
		BeginPeerRecovery( nClientID );
		return;
	}
	if ( !session.bInitiator || session.pendingRoute != EPeerConnectionRoute::Unknown )
	{
		return;
	}
	if ( now - session.lastSwitch < SPeerPathQuality::SWITCH_COOLDOWN )
	{
		session.bBetter = false;
		return;
	}
	int directScore = direct.Score( now );
	int relayScore = relay.Score( now );
	const bool freshReport = now - session.remoteReportTime < SPeerPathQuality::TIMEOUT;
	if ( freshReport && directScore >= 0 && session.remoteDirectScore >= 0 )
	{
		directScore = (std::max)( directScore, session.remoteDirectScore );
	}
	else
	{
		directScore = -1;
	}
	if ( freshReport && relayScore >= 0 && session.remoteRelayScore >= 0 )
	{
		relayScore = (std::max)( relayScore, session.remoteRelayScore );
	}
	else
	{
		relayScore = -1;
	}
	const bool stalledCommands =
			!session.pendingSend.empty() && now - session.dwLastAckProgress >= 2 * SPeerPathQuality::TIMEOUT;
	// Tiny probes alone do not prove that the reliable command stream is usable.
	// Allow a validated alternative after a sustained command stall as well.
	const bool alternativeHealthy = directActive ? relayScore >= 0 : directScore >= 0;
	const bool better =
			alternativeHealthy &&
			( stalledCommands || SPeerPathQuality::PreferAlternative( directActive, directScore, relayScore ) ||
	      ( !directActive && !relay.Healthy( now ) ) );
	if ( !better )
	{
		session.bBetter = false;
		return;
	}
	if ( !session.bBetter )
	{
		session.bBetter = true;
		session.betterSince = now;
		return;
	}
	if ( now - session.betterSince < SPeerPathQuality::SWITCH_HOLD )
	{
		return;
	}
	const EPeerConnectionRoute target = directActive ? EPeerConnectionRoute::Relay : EPeerConnectionRoute::Direct;
	if ( session.pStandby && session.bStandbyReady && session.pStandby->GetRoute() == target &&
	     session.pStandby->IsAlive() )
	{
		RequestRouteSwitch( nClientID, target );
	}
	else if ( !directActive )
	{
		// Keep relay carrying commands throughout a background direct handshake.
		BeginPeerRecovery( nClientID );
	}
}
