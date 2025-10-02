#include "stdafx.h"

#include "ConnectServerProcessor.h"
#include "PlayGameProcessor.h"
#include "ServerClient.h"


#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/Net.h"
#include "Server_Client_Common/NetLogger.h"

//*******************************************************************
//*                     CPacketsConvertor                            *
//*******************************************************************

class CPacketsConvertor : public CPacketProcessor
{
	OBJECT_NOCOPY_METHODS( CPacketsConvertor );
public:
	CPacketsConvertor();

	virtual bool Segment() { return true; }
	bool ProcessDirectClientPacket( CDirectPacketToClient *pPacket );
};

CPacketsConvertor::CPacketsConvertor()
{
	REGISTER_PACKET_PROCESSOR( &CPacketsConvertor::ProcessDirectClientPacket );
}

bool CPacketsConvertor::ProcessDirectClientPacket( CDirectPacketToClient *pFromServerPacket )
{
	CPtr<CDirectPacketToClient> pDelete = pFromServerPacket;

	CPtr<CNetPacket> pDirectPacket = pFromServerPacket->pPacket;
	pDirectPacket->nClientID = pFromServerPacket->nClient;
	PushPacket( pDirectPacket );

	return true;
}

//*******************************************************************
//*												CServerClient                              *
//*******************************************************************

void CServerClient::PushPacket( CNetPacket *pPacket )
{
	packets.push_back( pPacket );
}

CServerClient::CServerClient( const char* pServerIPAddress, const int nNetGameVersion, const int nServerPort, const int _nTimeOut )
: bDebugPaused( false ), nTimeOut( _nTimeOut )
{
	pNet = new CNet( nNetGameVersion, nServerPort, nTimeOut );
	pNet->InitAsClient();

	NNet::CNodeAddress address;
	if ( address.SetInetName( pServerIPAddress, nServerPort ) )
	{
		CMemoryStream pwd;
		pNet->ConnectGame( address, pwd );

		processors.push_back( new CConnectServerProcessor( pNet, true ) );
		processors.push_back( new CPacketsConvertor() );

		pPlayGameProcessor = new CPlayGameProcessor( pNet, pServerIPAddress, nNetGameVersion, nServerPort, nTimeOut );
		CPacketProcessor *pProcessor = pPlayGameProcessor;
		processors.push_back( pProcessor );
	}
	else
		PushPacket( new CConnectServerPacket( CConnectServerPacket::ECS_FAILED, CConnectServerPacket::ERR_CANT_RESOLVE_ADDRESS ) );

	GetNetLogger()->OpenLogFile( "client" );
}

CServerClient::~CServerClient()
{
	GetNetLogger()->CloseLogFile( "client" );
}

CNetPacket* CServerClient::GetPacket()
{
	if ( bDebugPaused )
		return 0;

	if ( !packets.empty() )
	{
		CPtr<CNetPacket> pPacket = packets.front();
		packets.pop_front();
		return pPacket.Extract();
	}
	else
	{
		CNetPacket *pPacket = pPlayGameProcessor->GetPacket();
		if ( pPacket && pPacket->nClientID == 0 )
			GetNetLogger()->Log( "client", StrFmt("from net %s", GetPacketInfo( pPacket )) );

		return pPacket;
	}
}

void CServerClient::GetPacketsFromNet()
{
	if ( bDebugPaused )
		return;

	while ( CPtr<CNetPacket> pPacket = pNet->ReceivePacket() )
	{
		GetNetLogger()->Log( "client", StrFmt("receive net %s", GetPacketInfo( pPacket )) );

		bool bProcessed = false;

		list< CPtr<CPacketProcessor> >::iterator iter = processors.begin();
		while ( iter != processors.end() && !bProcessed )
		{
			bProcessed = (*iter)->ProcessPacket( pPacket );
			++iter;
		}

		if ( !bProcessed )
			PushPacket( pPacket );
	}
}

void CServerClient::SendPacket( CNetPacket *pPacket2Process )
{
	if ( bDebugPaused )
	{
		pausedPackets.push_back( pPacket2Process );
		return;
	}

	bool bProcessed = false;
	CPtr<CNetPacket> pPacket = pPacket2Process;

	GetNetLogger()->Log( "client", StrFmt("send %s", GetPacketInfo( pPacket )) );

	// direct packet to client
	if ( pPacket->nClientID != 0 )
	{
		CPtr<CDirectPacketToClient> pPacket2Server = 
			new CDirectPacketToClient( 0, pPacket->nClientID, pPacket );
		pNet->SendPacket( pPacket2Server );
	}
	// packet to server
	else
	{
		list< CPtr<CPacketProcessor> >::iterator iter = processors.begin();
		while ( iter != processors.end() && !bProcessed )
		{
			bProcessed = (*iter)->ProcessPacket( pPacket );
			++iter;
		}

		if ( !bProcessed )
			pNet->SendPacket( pPacket );
	}
}

void CServerClient::Segment()
{
	if ( bDebugPaused )
		return;
	
	GetPacketsFromNet();

	list< CPtr<CPacketProcessor> >::iterator iter = processors.begin();
	while ( iter != processors.end() )
	{
		CPacketProcessor *pProcessor = *iter;
		const bool bSegment = pProcessor->Segment();

		CPtr<CNetPacket> pPacket;
		while ( pPacket = pProcessor->GetPacket() )
			PushPacket( pPacket );

		if ( !bSegment )
			iter = processors.erase( iter );
		else
			++iter;
	}
}

void CServerClient::SendGamePacket( CNetPacket *_pPacket, bool bBroadcast )
{
	if ( bDebugPaused )
	{
		pausedGamePackets.push_back( SPausedGamePacket( _pPacket, bBroadcast ) );
		return;
	}

	
	CPtr<CNetPacket> pPacket = _pPacket;
	GetNetLogger()->Log( "client", StrFmt("send game %s, broadcast %d", GetPacketInfo( pPacket ), int(bBroadcast)) );
	pPlayGameProcessor->SendGamePacket( pPacket, bBroadcast );
}

void CServerClient::TogglePauseServerConnection( bool bPause )
{
	if ( pNet )
		pNet->DebugTogglePause( bPause );
}

void CServerClient::TogglePauseAcceptGamers( bool bPause )
{
	if ( pPlayGameProcessor )
		pPlayGameProcessor->TogglePauseAcceptGamersNet( bPause );
}

void CServerClient::TogglePauseConnectGamer( const int nGamer, bool bPause )
{
	if ( pPlayGameProcessor )
		pPlayGameProcessor->TogglePauseConnectGamer( nGamer, bPause );
}

void CServerClient::TogglePause( bool bPause )
{
	bDebugPaused = bPause;
	if ( !bDebugPaused )
	{
		for ( list< CPtr<class CNetPacket> >::iterator iter = pausedPackets.begin(); iter != pausedPackets.end(); ++iter )
			SendPacket( *iter );
		pausedPackets.clear();

		for ( list<SPausedGamePacket>::iterator iter = pausedGamePackets.begin(); iter != pausedGamePackets.end(); ++iter )
			SendGamePacket( iter->pPacket, iter->bBroadcast );
		pausedGamePackets.clear();
	}
}

IServerClient* IServerClient::Create( const char* pServerIPAddress, const int nNetGameVersion, const int nServerPort, const int nTimeOut )
{
	return new CServerClient( pServerIPAddress, nNetGameVersion, nServerPort, nTimeOut );
}

BASIC_REGISTER_CLASS( IServerClient );

