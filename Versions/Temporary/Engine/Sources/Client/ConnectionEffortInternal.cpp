#include "stdafx.h"

#include "ConnectionEffortInternal.h"
#include "ConnectionInternal.h"
#include "ConnectServerProcessor.h"
#include "LinksManager.h"
#include "Net/NetLowest.h"
#include "port/time.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/Net.h"
#include "Server_Client_Common/NetLogger.h"

#include "Client_export.h"

#include <cstdio>

#include <fmt/format.h>

BASIC_REGISTER_CLASS( CLIENT, CSimpleClientEffort );
BASIC_REGISTER_CLASS( CLIENT, CSimpleServerEffort );
REGISTER_SAVELOAD_CLASS( CLIENT, 93, CPeerSetupPacket );

//*******************************************************************
//*                     CSimpleClientEffort                         *
//*******************************************************************

CSimpleClientEffort::CSimpleClientEffort( const int _nMyServerID,
                                          const int _nClientServerID,
                                          CNet *_pNet,
                                          const std::string &szServerIP,
                                          const int nServerPort,
                                          const int _nGameNetVersion,
                                          const int _nTimeOut,
                                          int _nOurGameID,
                                          uint32_t _nSetupID )
	: nMyServerID( _nMyServerID )
	, nClientServerID( _nClientServerID )
	, nGameNetVersion( _nGameNetVersion )
	, pNet( _pNet )
	, eState( ES_CONNECTING )
	, eSubState( ECONNECT_SERVER )
	, nTimeOut( _nTimeOut )
	, nOurGameID( _nOurGameID )
	, nSetupID( _nSetupID )
{
	REGISTER_PACKET_PROCESSOR( &CSimpleClientEffort::ProcessClientAccepted );
	REGISTER_PACKET_PROCESSOR( &CSimpleClientEffort::ProcessClientConnectInfo );
	REGISTER_PACKET_PROCESSOR( &CSimpleClientEffort::ProcessClientIdentity );

	pConnet2PlayersNet = new CNet( nGameNetVersion, nServerPort, nTimeOut );
	pLinksManager = new NNet::CGameLinksManager();
	pLinksManager->Init( 0 );
	pConnet2PlayersNet->InitAsClient( pLinksManager );

	serverAddress.SetInetName( szServerIP.c_str(), nServerPort );

	CMemoryStream pwd;
	pConnet2PlayersNet->ConnectGame( serverAddress, pwd );
	pConnectServerProcessor = new CConnectServerProcessor( pConnet2PlayersNet, false );
	dwPredBreakThroughSend = dwStartConnectClientTime = GetCurrentTimeMilliseconds();
}

IConnection *CSimpleClientEffort::CreateConnection() const
{
	return eState == ES_SUCCESS ? new CSimpleConnection( pConnet2PlayersNet, true, nClientServerID, 0, nMyServerID ) : 0;
}

bool CSimpleClientEffort::Segment()
{
	if ( eState == ES_CONNECTING && eSubState == EWAIT_ADDRESSES )
	{
		// Local endpoint offers travel on the lobby connection. Give them a short
		// bounded window before trying the public endpoint offered by older clients.
		if ( GetCurrentTimeMilliseconds() - dwStartConnectClientTime >= 1000 )
		{
			StartPeerConnections();
		}
	}
	if ( eState == ES_CONNECTING && pConnectServerProcessor )
	{
		pConnectServerProcessor->Segment();

		CPtr<CNetPacket> pPacket = 0;
		while ( eState == ES_CONNECTING && ( pPacket = pConnet2PlayersNet->ReceivePacket() ) )
		{
			if ( !pConnectServerProcessor->ProcessPacket( pPacket ) )
			{
				ProcessPacket( pPacket );
			}
		}

		// Transport failure can produce a state event without an incoming packet.
		while ( pConnectServerProcessor )
		{
			CPtr<CNetPacket> pConnPacket = pConnectServerProcessor->GetPacket();
			if ( !pConnPacket )
			{
				break;
			}
			if ( CConnectServerPacket *pConnectPacket = dynamic_cast_ptr<CConnectServerPacket *>( pConnPacket ) )
			{
				ProcessConnectServerPacket( pConnectPacket );
			}
		}
	}

	if ( eState == ES_CONNECTING )
	{
		// Bound both the address-registration phase and the peer handshake.
		const uint32_t dwCurTime = GetCurrentTimeMilliseconds();
		if ( nAttempt == 0 && dwCurTime - dwStartConnectClientTime > 1000 * CNet::GetTimeOut() )
		{
			eState = ES_FAILED;
		}
		else if ( IsAttemptExpired() )
		{
			eState = ES_FAILED;
		}
		else if ( eSubState == ECONNECT_CLIENT && dwCurTime - dwPredBreakThroughSend > 3000 )
		{
			dwPredBreakThroughSend = dwCurTime;
			SendBreakThroughPacket( pLinksManager, szClientIP, nClientPort );
		}
	}

	return true;
}

bool CSimpleClientEffort::ProcessClientAccepted( CGameConnectingClientAccepted *pPacket )
{
	if ( pPacket->nConnection == nClientServerID )
	{
		CPtr<CNetPacket> pPacket = new CWant2Connect2Client( 0, nClientServerID, nClientServerID );
		pNet->SendPacket( pPacket );

		return true;
	}
	else
	{
		return false;
	}
}

bool CSimpleClientEffort::ProcessClientConnectInfo( CClientGameConnectInfo *pPacket )
{
	if ( pPacket->nClient2Connect == nClientServerID && eSubState == ECONNECT_SERVER )
	{
		pConnectServerProcessor = 0;
		pConnet2PlayersNet = 0;
		pLinksManager->BanAddress( *( serverAddress.GetSockAddr() ) );

		szClientIP = pPacket->szIP;
		nClientPort = pPacket->nPort;

		// Public-address login can fail even on one PC when the router cannot
		// loop traffic back inside. Local addresses are needed BEFORE login.
		eSubState = EWAIT_ADDRESSES;
		dwPredBreakThroughSend = dwStartConnectClientTime = GetCurrentTimeMilliseconds();

		return true;
	}
	else
	{
		return false;
	}
}

void CSimpleClientEffort::AddPeerLocalAddress( const std::string &szIP, int nPort )
{
	// These are numeric IPv4 endpoints, not names to resolve on a peer's behalf.
	if ( eState != ES_CONNECTING || nAttempt != 0 || peerLocalAddresses.size() >= NNet::N_MAX_HOST_HOMES || nPort <= 0 ||
	     nPort > 65535 || szIP.empty() || szIP.size() > 15 ||
	     szIP.find_first_not_of( "0123456789." ) != std::string::npos )
	{
		return;
	}
	unsigned int octets[4] = {};
	char trailing;
	if ( std::sscanf( szIP.c_str(), "%u.%u.%u.%u%c", &octets[0], &octets[1], &octets[2], &octets[3], &trailing ) != 4 ||
	     octets[0] == 0 || octets[0] == 127 || octets[0] >= 224 || octets[1] > 255 || octets[2] > 255 ||
	     octets[3] > 255 || fmt::format( "{}.{}.{}.{}", octets[0], octets[1], octets[2], octets[3] ) != szIP )
	{
		return;
	}
	NNet::CNodeAddress candidate;
	if ( candidate.SetInetName( szIP.c_str(), nPort ) && pLinksManager->IsLocalAddr( candidate ) )
	{
		peerLocalAddresses.push_back( candidate );
	}
}

void CSimpleClientEffort::StartPeerConnections()
{
	NNet::CNodeAddress publicAddress;
	if ( !publicAddress.SetInetName( szClientIP.c_str(), nClientPort ) )
	{
		eState = ES_FAILED;
		return;
	}
	peerTargets.clear();
	nNextPeerTarget = 0;
	if ( !peerLocalAddresses.empty() )
	{
		// Prefer another instance on this machine, then a same-subnet address.
		// Try at most one local endpoint so a coincident private subnet elsewhere
		// cannot consume the entire handshake timeout before the public attempt.
		NNet::CNodeAddress preferred = peerLocalAddresses.front();
		NNet::CNodeAddressSet ownAddresses;
		if ( pLinksManager->GetSelfAddress( &ownAddresses ) )
		{
			for ( const auto &candidate : peerLocalAddresses )
			{
				NNet::CNodeAddress own;
				for ( int i = 0; ownAddresses.GetAddress( i, &own ); ++i )
				{
					if ( candidate.SameIP( own ) )
					{
						preferred = candidate;
					}
				}
			}
		}
		if ( preferred != publicAddress )
		{
			peerTargets.push_back( preferred );
		}
	}
	peerTargets.push_back( publicAddress );
	eSubState = ECONNECT_CLIENT;
	StartNextPeerConnection();
}

void CSimpleClientEffort::StartNextPeerConnection()
{
	pConnectServerProcessor = nullptr;
	pConnet2PlayersNet = nullptr;
	if ( nAttempt >= nAttemptLimit || peerTargets.empty() )
	{
		eState = ES_FAILED;
		return;
	}
	NNet::CNodeAddress address = peerTargets[nNextPeerTarget++ % peerTargets.size()];
	szClientIP = address.GetIP();
	nClientPort = address.GetPort();
	++nAttempt;
	eState = ES_CONNECTING;
	eSubState = EWAIT_ATTEMPT;
	dwPeerAttemptStart = dwPredBreakThroughSend = GetCurrentTimeMilliseconds();
	GetNetLogger()->Log( "client",
	                     fmt::format( "direct peer {} attempt {}/{} trying {} (15000 ms)",
	                                  nClientServerID,
	                                  nAttempt,
	                                  nAttemptLimit,
	                                  address.GetFastName() ) );
	// Wait for the other end to retire its previous tentative connection before
	// sending a new UDP identity. Lobby delivery shares the 15-second budget.
	CPtr<CPeerSetupPacket> begin = new CPeerSetupPacket( nOurGameID, nSetupID, nAttempt, CPeerSetupPacket::BeginAttempt );
	begin->bRecovery = bRecovery;
	pNet->SendPacket( new CDirectPacketToClient( 0, nClientServerID, begin ) );
}

void CSimpleClientEffort::RetryEstablishedConnection()
{
	// Reuse the registered socket and the last working endpoint. Recovery gets
	// one fresh 15-second attempt, without repeating server registration.
	bRecovery = true;
	nAttemptLimit = nAttempt + 1;
	if ( nNextPeerTarget != 0 )
	{
		--nNextPeerTarget;
	}
	NNet::CNodeAddress validated;
	if ( pLinksManager->GetProbeAddress( nClientServerID, &validated ) )
	{
		// A restored NAT/local path can differ from the last failed login target.
		const auto target = std::find( peerTargets.begin(), peerTargets.end(), validated );
		nNextPeerTarget = static_cast<size_t>( target - peerTargets.begin() );
		if ( target == peerTargets.end() )
		{
			peerTargets.push_back( validated );
		}
	}
	StartNextPeerConnection();
}

bool CSimpleClientEffort::IsAttemptExpired() const
{
	return nAttempt != 0 && GetCurrentTimeMilliseconds() - dwPeerAttemptStart >= DIRECT_ATTEMPT_TIMEOUT;
}

NNet::CGameLinksManager *CSimpleClientEffort::GetLinks() const
{
	return pLinksManager;
}

void CSimpleClientEffort::AcceptAttempt( const CPeerSetupPacket &packet )
{
	if ( eSubState != EWAIT_ATTEMPT || packet.nSetupID != nSetupID || packet.nAttempt != nAttempt )
	{
		return;
	}
	eSubState = ECONNECT_CLIENT;
	NNet::CNodeAddress address;
	address.SetInetName( szClientIP.c_str(), nClientPort );
	SendBreakThroughPacket( pLinksManager, szClientIP, nClientPort );
	pConnet2PlayersNet = new CNet( nGameNetVersion, 0, nTimeOut );
	pConnet2PlayersNet->InitAsClient( pLinksManager );
	pConnet2PlayersNet->ConnectGame( address, CMemoryStream() );
	pConnectServerProcessor = new CConnectServerProcessor( pConnet2PlayersNet, false );
	dwPredBreakThroughSend = GetCurrentTimeMilliseconds();
}

void CSimpleClientEffort::ProcessConnectServerPacket( CConnectServerPacket *pPacket )
{
	if ( pPacket->eConnectionState == CConnectServerPacket::ECS_FAILED )
	{
		if ( eSubState == ECONNECT_CLIENT && pPacket->eRejectReason == CConnectServerPacket::ERR_LOGIN_TIMEOUT &&
		     !IsAttemptExpired() )
		{
			// The legacy login backoff stops after about six seconds. Continue it
			// on this socket/endpoint without consuming another coordinated attempt.
			NNet::CNodeAddress address;
			address.SetInetName( szClientIP.c_str(), nClientPort );
			pConnet2PlayersNet->ConnectGame( address, CMemoryStream() );
			pConnectServerProcessor = new CConnectServerProcessor( pConnet2PlayersNet, false );
			return;
		}
		eState = ES_FAILED;
	}
	else
	{
		if ( eSubState == ECONNECT_SERVER )
		{
			CPtr<CNetPacket> pPacket = new CNewGameConnectingClient( 0, nMyServerID, nClientServerID );
			// The server must advertise the socket retained for the peer connection.
			pConnet2PlayersNet->SendPacket( pPacket );
		}
		else
		{
			CPtr<CNetPacket> pPacket =
					new CPeerSetupPacket( nOurGameID, nSetupID, nAttempt, CPeerSetupPacket::Identity, nMyServerID );
			pConnet2PlayersNet->SendPacket( pPacket );
		}
	}
}

bool CSimpleClientEffort::ProcessClientIdentity( CPeerSetupPacket *pPacket )
{
	if ( eState == ES_CONNECTING && eSubState == ECONNECT_CLIENT && pPacket->eAction == CPeerSetupPacket::Identity &&
	     pPacket->nServerID == nClientServerID && pPacket->nGameID == nOurGameID && pPacket->nSetupID == nSetupID &&
	     pPacket->nAttempt == nAttempt )
	{
		eState = ES_SUCCESS;
		return true;
	}

	return false;
}

CConnectionEffort *CSimpleClientEffort::CreateNextEffort( const int nOurGameID )
{
	// The room processor coordinates retries and relay selection at both ends.
	return nullptr;
}

//*******************************************************************
//*                     CSimpleServerEffort                         *
//*******************************************************************

CSimpleServerEffort::CSimpleServerEffort( const int _nMyServerID,
                                          const int _nClientServerID,
                                          const std::string &_szClientIP,
                                          const int _nClientPort,
                                          CNet *_pNet,
                                          CNet *_pAcceptGamersNet,
                                          NNet::ILinksManager *_pLinksManager,
                                          int _nOurGameID,
                                          uint32_t _nSetupID,
                                          int _nAttempt )
	: nMyServerID( _nMyServerID )
	, nClientServerID( _nClientServerID )
	, pNet( _pNet )
	, eState( ES_CONNECTING )
	, szClientIP( _szClientIP )
	, nClientPort( _nClientPort )
	, pAcceptGamersNet( _pAcceptGamersNet )
	, pLinksManager( _pLinksManager )
	, nOurGameID( _nOurGameID )
	, nSetupID( _nSetupID )
	, nAttempt( _nAttempt )
{
	REGISTER_PACKET_PROCESSOR( &CSimpleServerEffort::ProcessClientIdentity );

	SendBreakThroughPacket( pLinksManager, szClientIP, nClientPort );
	dwPredBreakThroughSend = dwStartConnectClientTime = GetCurrentTimeMilliseconds();
}

bool CSimpleServerEffort::Segment()
{
	if ( eState != ES_SUCCESS && eState != ES_FAILED )
	{
		const uint32_t dwCurTime = GetCurrentTimeMilliseconds();
		if ( dwCurTime - dwStartConnectClientTime > 1000 * pAcceptGamersNet->GetTimeOut() )
		{
			eState = ES_FAILED;
		}
		else if ( dwCurTime - dwPredBreakThroughSend > 3000 )
		{
			dwPredBreakThroughSend = dwCurTime;
			SendBreakThroughPacket( pLinksManager, szClientIP, nClientPort );
		}
	}

	return true;
}

bool CSimpleServerEffort::ProcessClientIdentity( CPeerSetupPacket *pPacket )
{
	if ( pPacket->eAction == CPeerSetupPacket::Identity && pPacket->nServerID == nClientServerID &&
	     pPacket->nGameID == nOurGameID && pPacket->nSetupID == nSetupID && pPacket->nAttempt == nAttempt )
	{
		nClientLocalID = pPacket->nClientID;
		eState = ES_SUCCESS;

		CPtr<CNetPacket> reply =
				new CPeerSetupPacket( nOurGameID, nSetupID, nAttempt, CPeerSetupPacket::Identity, nMyServerID );
		reply->nClientID = nClientLocalID;
		pAcceptGamersNet->SendPacket( reply );

		return true;
	}
	else
	{
		return false;
	}
}

IConnection *CSimpleServerEffort::CreateConnection() const
{
	return eState == ES_SUCCESS
	           ? new CSimpleConnection( pAcceptGamersNet, false, nClientServerID, nClientLocalID, nMyServerID )
	           : 0;
}

CConnectionEffort *CSimpleServerEffort::CreateNextEffort( const int nOurGameID )
{
	// Only the joining client can select relay, through the setup handshake.
	return nullptr;
}

//*******************************************************************
//*                     CThroughServerEffort                         *
//*******************************************************************

CThroughServerEffort::CThroughServerEffort( const int _nClientServerID,
                                            const int _nMyServerID,
                                            const int _nOurGameID,
                                            CNet *_pNet )
	: nClientServerID( _nClientServerID )
	, nMyServerID( _nMyServerID )
	, nOurGameID( _nOurGameID )
	, pNet( _pNet )
{
	pNet->SendPacket( new CThroughServerConnectionPacket( 0, nClientServerID, nOurGameID ) );
}

IConnection *CThroughServerEffort::CreateConnection() const
{
	return new CThroughServerConnection( nClientServerID, pNet );
}
