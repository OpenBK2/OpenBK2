#include "stdafx.h"

#include "ConnectionEffortInternal.h"
#include "ConnectionInternal.h"
#include "ConnectServerProcessor.h"
#include "LinksManager.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/Net.h"

#include "Client_export.h"

BASIC_REGISTER_CLASS( CLIENT, CSimpleClientEffort );
BASIC_REGISTER_CLASS( CLIENT, CSimpleServerEffort );

//*******************************************************************
//*                     CSimpleClientEffort                         *
//*******************************************************************

CSimpleClientEffort::CSimpleClientEffort( const int _nMyServerID, const int _nClientServerID,
	CNet *_pNet, const std::string &szServerIP, const int nServerPort, const int _nGameNetVersion, const int _nTimeOut )
: nMyServerID( _nMyServerID ), nClientServerID( _nClientServerID ), nGameNetVersion( _nGameNetVersion ),
	pNet( _pNet ), eState( ES_CONNECTING ), eSubState( ECONNECT_SERVER ), nTimeOut( _nTimeOut )
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
	fPredBreakThroughSend = fStartConnectClientTime = GetTickCount();
}

IConnection* CSimpleClientEffort::CreateConnection() const
{
	return 
		eState == ES_SUCCESS ? new CSimpleConnection( pConnet2PlayersNet, true, nClientServerID, 0, nMyServerID ) : 0;
}

bool CSimpleClientEffort::Segment()
{
	if ( eState == ES_CONNECTING )
	{
		pConnectServerProcessor->Segment();

		CPtr<CNetPacket> pPacket = 0;
		while ( eState == ES_CONNECTING && ( pPacket = pConnet2PlayersNet->ReceivePacket() ) )
		{
			if ( !pConnectServerProcessor->ProcessPacket( pPacket ) )
				ProcessPacket( pPacket );

			while ( CPtr<CNetPacket> pConnPacket = pConnectServerProcessor->GetPacket() )
			{
				if ( CConnectServerPacket* pConnectPacket = dynamic_cast_ptr<CConnectServerPacket*>( pConnPacket ) )
					ProcessConnectServerPacket( pConnectPacket );
			}
		}
	}

	if ( eState != ES_SUCCESS && eState != ES_FAILED && eSubState == ECONNECT_CLIENT )
	{
		const float fCurTime = GetTickCount();
		if ( fStartConnectClientTime + 1000 * pConnet2PlayersNet->GetTimeOut() < fCurTime )
			eState = ES_FAILED;
		else if ( fPredBreakThroughSend + 3000 < fCurTime )
		{
			fPredBreakThroughSend = fCurTime;
			SendBreakThroughPacket( szClientIP, nClientPort );
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
		return false;
}

bool CSimpleClientEffort::ProcessClientConnectInfo( CClientGameConnectInfo *pPacket )
{
	if ( pPacket->nClient2Connect == nClientServerID )
	{
		pConnectServerProcessor = 0;
		pConnet2PlayersNet = 0;
		pLinksManager->BanAddress( *(serverAddress.GetSockAddr()) );

		szClientIP = pPacket->szIP;
		nClientPort = pPacket->nPort;

		// send breakthrough packet
		SendBreakThroughPacket( szClientIP, nClientPort );

		pConnet2PlayersNet = new CNet( nGameNetVersion, 0, nTimeOut );
		pConnet2PlayersNet->InitAsClient( pLinksManager );

		NNet::CNodeAddress address;
		address.SetInetName( szClientIP.c_str(), nClientPort );

		CMemoryStream pwd;
		pConnet2PlayersNet->ConnectGame( address, pwd );
		pConnectServerProcessor = new CConnectServerProcessor( pConnet2PlayersNet, false );

		eSubState = ECONNECT_CLIENT;
		fPredBreakThroughSend = fStartConnectClientTime = GetTickCount();

		return true;
	}
	else
		return false;
}

void CSimpleClientEffort::ProcessConnectServerPacket( CConnectServerPacket *pPacket )
{
	if ( pPacket->eConnectionState == CConnectServerPacket::ECS_FAILED )
		eState = ES_FAILED;
	else
	{
		if ( eSubState == ECONNECT_SERVER )
		{
			CPtr<CNetPacket> pPacket = new CNewGameConnectingClient( 0, nMyServerID, nClientServerID );
			pNet->SendPacket( pPacket );
		}
		else
		{
			CPtr<CNetPacket> pPacket = new CIndentityPacket( 0, nMyServerID );
			pConnet2PlayersNet->SendPacket( pPacket );
		}
	}
}

bool CSimpleClientEffort::ProcessClientIdentity( CIndentityPacket *pPacket )
{
	if ( pPacket->nServerID == nClientServerID )
	{
		eState = ES_SUCCESS;
		return true;
	}

	return false;
}

CConnectionEffort* CSimpleClientEffort::CreateNextEffort( const int nOurGameID )
{
	return new CThroughServerEffort( nClientServerID, nMyServerID, nOurGameID, pNet );
}

//*******************************************************************
//*                     CSimpleServerEffort                         *
//*******************************************************************

CSimpleServerEffort::CSimpleServerEffort( const int _nMyServerID, const int _nClientServerID, const std::string &_szClientIP, const int _nClientPort, CNet *_pNet, CNet *_pAcceptGamersNet )
: nMyServerID( _nMyServerID ), nClientServerID( _nClientServerID ), pNet( _pNet ), eState( ES_CONNECTING ),
	szClientIP( _szClientIP ), nClientPort( _nClientPort ), pAcceptGamersNet( _pAcceptGamersNet )
{
	REGISTER_PACKET_PROCESSOR( &CSimpleServerEffort::ProcessClientIdentity );

	SendBreakThroughPacket( szClientIP, nClientPort );
	fPredBreakThroughSend = fStartConnectClientTime = GetTickCount();
}

bool CSimpleServerEffort::Segment()
{
	if ( eState != ES_SUCCESS && eState != ES_FAILED )
	{
		const float fCurTime = GetTickCount();
		if ( fStartConnectClientTime + 1000 * pAcceptGamersNet->GetTimeOut() < fCurTime )
			eState = ES_FAILED;
		else if ( fPredBreakThroughSend + 3000 < fCurTime )
		{
			fPredBreakThroughSend = fCurTime;
			SendBreakThroughPacket( szClientIP, nClientPort );
		}
	}

	return true;
}

bool CSimpleServerEffort::ProcessClientIdentity( CIndentityPacket *pPacket )
{
	if ( pPacket->nServerID == nClientServerID )
	{
		nClientLocalID = pPacket->nClientID;
		eState = ES_SUCCESS;

		CPtr<CNetPacket> pPacket = new CIndentityPacket( nClientLocalID, nMyServerID );
		pAcceptGamersNet->SendPacket( pPacket );

		return true;
	}
	else
		return false;
}

IConnection* CSimpleServerEffort::CreateConnection() const
{
	return eState == ES_SUCCESS ? new CSimpleConnection( pAcceptGamersNet, false, nClientServerID, nClientLocalID, nMyServerID ) : 0;
}

CConnectionEffort* CSimpleServerEffort::CreateNextEffort( const int nOurGameID )
{
	return new CThroughServerEffort( nClientServerID, nMyServerID, nOurGameID, pNet );
}

//*******************************************************************
//*                     CThroughServerEffort                         *
//*******************************************************************

CThroughServerEffort::CThroughServerEffort( const int _nClientServerID, const int _nMyServerID, const int _nOurGameID, CNet *_pNet )
: nClientServerID( _nClientServerID ), nMyServerID( _nMyServerID ), nOurGameID( _nOurGameID ), pNet( _pNet )
{
	pNet->SendPacket( new CThroughServerConnectionPacket( 0, nClientServerID, nOurGameID ) );
}

IConnection* CThroughServerEffort::CreateConnection() const
{
	return new CThroughServerConnection( nClientServerID, pNet );
}


