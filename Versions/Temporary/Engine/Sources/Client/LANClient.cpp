#include "stdafx.h"

#include "LANClient.h"
#include "Net/NetAddress.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/NetPacket.h"
#include "Server_Client_Common/NetSaver.h"

#include <fmt/format.h>

CLANClient *
CLANClient::Create( const int nNetGameVersion, const int nGamePort, const int _nTimeOut, std::string *pszError )
{
	NNet::SNetDriverConsts netConsts( _nTimeOut );
	CPtr<NNet::IDriver> pDriver = NNet::CreateNetDriver( netConsts, true );
	if ( !pDriver )
	{
		if ( pszError )
		{
			*pszError = "CreateNetDriver failed";
		}
		return 0;
	}
	CPtr<NNet::ILinksManager> pLinks = NNet::CreateServerLinksManager( nGamePort );
	// Why the game port could not be had; empty when it was
	std::string szGamePortError;
	if ( !pLinks )
	{
		// Every instance asks for the same port, so a second one on the same
		// machine is refused it. Rather than fail, take any free port: games are
		// looked for by broadcasting to the game port and answered at whatever
		// port the request came from, and a login works the same way, so this
		// instance can still browse and join. It cannot host, because nobody
		// would look for its game on this port; see IsOnGamePort.
		szGamePortError = NNet::GetLastLinksManagerError();
		pLinks = NNet::CreateClientLinksManager();
	}
	if ( !pLinks )
	{
		// The links manager knows which socket call failed and why
		if ( pszError )
		{
			*pszError = NNet::GetLastLinksManagerError();
		}
		return 0;
	}
	pDriver->Init( nNetGameVersion, nGamePort, false, pLinks );

	CLANClient *pClient = new CLANClient();
	pClient->pNetDriver = pDriver;
	pClient->szGamePortError = szGamePortError;
	return pClient;
}

CLANClient::~CLANClient() {}

CNetPacket *CLANClient::GetPacket()
{
	if ( !packets.empty() )
	{
		CPtr<CNetPacket> pPacket = packets.front();
		packets.pop_front();
		return pPacket.Extract();
	}
	else
	{
		return 0;
	}
}

void CLANClient::SendPacket( class CNetPacket *pPacket )
{
	CPtr<CNetPacket> pNetPacket = pPacket;

	CMemoryStream memStream;
	{
		CPtr<IBinSaver> pSaver = CreateBinSaver( &memStream, SAVER_MODE_WRITE );
		pSaver->Add( 1, &pNetPacket );
	}
	pNetDriver->SendDirect( pNetPacket->nClientID, memStream );
}

void CLANClient::SendGamePacket( class CNetPacket *pPacket, bool bBroadcast )
{
	CPtr<CNetPacket> pNetPacket = pPacket;

	CMemoryStream memStream;
	{
		CPtr<IBinSaver> pSaver = CreateBinSaver( &memStream, SAVER_MODE_WRITE );
		pSaver->Add( 1, &pNetPacket );
	}

	for ( std::list<int>::iterator it = clients.begin(); it != clients.end(); ++it )
	{
		if ( bBroadcast )
		{
			pNetDriver->SendDirect( *it, memStream );
			continue;
		}
		else if ( *it == pPacket->nClientID )
		{
			pNetDriver->SendDirect( *it, memStream );
			break;
		}
	}
}

void CLANClient::Segment()
{
	pNetDriver->Step();

	NNet::IDriver::EMessage eMsgID;
	int nClientID;

	if ( bTryingConnect )
	{
		if ( pNetDriver->GetState() == NNet::IDriver::INACTIVE )
		{
			bTryingConnect = false;
			int nReason = pNetDriver->GetRejectReason();
			CPtr<CNetPacket> pNetPacket = new CConnectGameFailed( 0, CConnectGameFailed::ER_UNKNOWN );
			packets.push_back( pNetPacket );
		}
		else if ( pNetDriver->GetState() == NNet::IDriver::ACTIVE )
		{
			bTryingConnect = false;
			// Report success?
		}
	}

	CMemoryStream memStream;
	while ( pNetDriver->GetMessage( &eMsgID, &nClientID, 0, &memStream ) )
	{
		CPtr<IBinSaver> pSaver = CreateBinSaver( &memStream, SAVER_MODE_READ );
		CPtr<CNetPacket> pNetPacket;

		switch ( eMsgID )
		{
			case NNet::IDriver::DIRECT:
			case NNet::IDriver::BROADCAST:
			{
				pSaver->Add( 1, &pNetPacket );
				break;
			}
			case NNet::IDriver::NEW_CLIENT:
			{
				pNetPacket = new CNetNewClient();

				std::list<int>::iterator it = clients.begin();
				for ( ; it != clients.end() && *it != nClientID; ++it )
					;
				if ( it == clients.end() )
				{
					clients.push_back( nClientID );
				}
				else
				{
					NI_ASSERT( 0, fmt::format( "PRG:Client {} not present", nClientID ) );
				}
			}
			break;
			case NNet::IDriver::REMOVE_CLIENT:
			{
				std::list<int>::iterator it = clients.begin();
				for ( ; it != clients.end() && *it != nClientID; ++it )
					;
				if ( it != clients.end() )
				{
					pNetPacket = new CGameClientRemoved( nClientID );
					clients.erase( it );
				}
				else
				{
					NI_ASSERT( 0, fmt::format( "PRG:Client {} not present", nClientID ) );
				}
			}
			break;
			case NNet::IDriver::KICKED:
			{
				pNetPacket = new CGameClientWasKicked( -1, -1 );
			}
			break;
			default:
				break;
		}

		if ( pNetPacket )
		{
			pNetPacket->nClientID = nClientID;
			packets.push_back( pNetPacket );
		}
	}
}

bool CLANClient::GetGameInfo(
		int nIdx, NNet::CNodeAddress *pAddr, bool *pWrongVersion, float *pPing, NNet::IDriver::SGameInfo *pGameInfo )
{
	return pNetDriver->GetGameInfo( nIdx, pAddr, pWrongVersion, pPing, pGameInfo );
}

void CLANClient::ConnectGame( const NNet::CNodeAddress &addr )
{
	bTryingConnect = true;
	pNetDriver->ConnectGame( addr, CMemoryStream() );
}

void CLANClient::StartGame()
{
	pNetDriver->StartGame();
}

void CLANClient::StartGameInfoSend( const NNet::IDriver::SGameInfo &gameInfo )
{
	pNetDriver->StartGameInfoSend( gameInfo );
}

void CLANClient::StopGameInfoSend()
{
	pNetDriver->StopGameInfoSend();
}

void CLANClient::StartNewPlayerAccept()
{
	pNetDriver->StartNewPlayerAccept();
}

void CLANClient::StopNewPlayerAccept()
{
	pNetDriver->StopGameInfoSend();
}

void CLANClient::Kick( int nClient )
{
	pNetDriver->Kick( nClient );
}
