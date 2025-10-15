#include "stdafx.h"

#include "Connection.h"
#include "ConnectionEffort.h"
#include "ConnectServerProcessor.h"
#include "PlayGameProcessor.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/Net.h"
#include "Server_Client_Common/NetLogger.h"

#include "Client_export.h"

#include <cstdint>

static const uint32_t dwWaitingPacketTimeout = 300000;
const int CONNECTION_TIMEOUT = 20;

CPlayGameProcessor::CPlayGameProcessor( CNet *_pNet, const char* pszServerIP, const int _nNetGameVersion, const int _nServerPort, const int _nTimeOut )
: pNet( _pNet ), nNetGameVersion( _nNetGameVersion), szServerIP( pszServerIP ), nServerPort( _nServerPort ), nTimeOut( _nTimeOut )
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
		CPtr<CConnectionEffort> pEffort = NEffortsFactory::CreateFirstClientEffort( nMyServerID, *iter, pNet, nTimeOut );
		CreateConnectionEffort( pEffort );
	}
//	}

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
	CPtr<CConnectionEffort> pEffort =
		NEffortsFactory::CreateFirstServerEffort( nMyServerID, pPacket->nWantedClient, pPacket->szIP, pPacket->nPort, pNet, pAcceptGamersNet );
	CreateConnectionEffort( pEffort );

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
		while ( pConnectServerProcessor && (pPacket = pConnectServerProcessor->GetPacket()) )
		{
			if ( CConnectServerPacket* pConnectPacket = dynamic_cast_ptr<CConnectServerPacket*>( pPacket ) )
				ProcessConnectServerPacket( pConnectPacket );
		}
	}
}

void CPlayGameProcessor::ProcessAcceptingGamersPackets()
{
	if ( pAcceptGamersNet )
	{
		while ( CPtr<CNetPacket> pPacket = pAcceptGamersNet->ReceivePacket() )
		{
			GetNetLogger()->Log( "client", StrFmt( "receive acceptgamers net %s", GetPacketInfo( pPacket ) ) );
			// wait for pAcceptGamersNet packet processed
			bool bProcessed = ProcessPacket( pPacket );

			if ( pConnectServerProcessor )
				bProcessed = pConnectServerProcessor->ProcessPacket( pPacket );

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
						bProcessed = true;
				}
				++conn_iter;
			}

			// some new client, we didn't start accepting packets from him yet
			if ( !bProcessed && pPacket->nClientID != 0 )
				waitingPackets.push_back( SWaitingPacket( pPacket, GetTickCount() ) );
		}
	}

	for ( TConnections::iterator iter = connections.begin(); iter != connections.end(); ++iter )
	{
		IConnection *pConnection = iter->second;
		while ( CPtr<CNetPacket> pPacket = pConnection->GetPacket() )
			PushPacket( pPacket );
	}
}

void CPlayGameProcessor::ProcessEfforts()
{
	std::list< CPtr<CConnectionEffort> > success, failed;

	for ( TConnectionEfforts::iterator iter = connectionEfforts.begin(); iter != connectionEfforts.end(); ++iter )
	{
		CConnectionEffort *pEffort = iter->second;
		pEffort->Segment();

		const CConnectionEffort::EState eState = pEffort->GetState();
		if ( eState == CConnectionEffort::ES_SUCCESS )
			success.push_back( pEffort );
		if ( eState == CConnectionEffort::ES_FAILED )
			failed.push_back( pEffort );
	}

	for ( std::list< CPtr<CConnectionEffort> >::iterator iter = success.begin(); iter != success.end(); ++iter )
	{
		CPtr<CConnectionEffort> pEffort = *iter;
		const int nClientServerID = pEffort->GetClientServerID();
		connectionEfforts.erase( nClientServerID );

    connections[nClientServerID] = pEffort->CreateConnection();
	}

	for ( std::list< CPtr<CConnectionEffort> >::iterator iter = failed.begin(); iter != failed.end(); ++iter )
	{
		CPtr<CConnectionEffort> pEffort = *iter;
		connectionEfforts.erase( pEffort->GetClientServerID() );

		CPtr<CConnectionEffort> pNextEffort = pEffort->CreateNextEffort( nOurGameID );
		CreateConnectionEffort( pNextEffort );
	}
}

bool CPlayGameProcessor::Segment()
{
	ProcessConnectServerProcessor();
	ProcessAcceptingGamersPackets();

	ProcessEfforts();

	uint32_t dwCurTime = GetTickCount();
	std::list<SWaitingPacket>::iterator iter = waitingPackets.begin();
	while ( iter != waitingPackets.end() )
	{
		const SWaitingPacket &packet = *iter;
		if ( dwCurTime - packet.dwStartTime > dwWaitingPacketTimeout )
			iter = waitingPackets.erase( iter );
		else
			++iter;
	}

	std::list<int> deadConnections;
	for ( TConnections::iterator conn_iter = connections.begin(); conn_iter != connections.end(); ++conn_iter )
	{
		IConnection *pConnection = conn_iter->second;
		if ( !pConnection->IsAlive() )
			deadConnections.push_back( conn_iter->first );
	}

	for ( std::list<int>::iterator iter = deadConnections.begin(); iter != deadConnections.end(); ++iter )
		connections.erase( *iter );

	return true;
}

bool CPlayGameProcessor::CreateConnectionEffort( CConnectionEffort *pEffort )
{
	if ( pEffort != 0 )
	{
		connectionEfforts[pEffort->GetClientServerID()] = pEffort;

		std::list<SWaitingPacket>::iterator iter = waitingPackets.begin();
		while ( iter != waitingPackets.end() )
		{
			const SWaitingPacket &packet = *iter;
			const bool bProcessed = pEffort->ProcessPacket( packet.pPacket );
			if ( bProcessed )
				iter = waitingPackets.erase( iter );
			else
				++iter;
		}

		return true;
	}
	else
	{
		const int nClientServerID = pEffort->GetClientServerID();
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
		GetNetLogger()->Log( "client", StrFmt("send acceptgamers net %s", GetPacketInfo( pPacket )) );

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
	pLinksManager->BanAddress( *(serverAddress.GetSockAddr()) );
	pAcceptGamersNet = new CNet( nNetGameVersion, nServerPort, 60 );
	pAcceptGamersNet->InitAsServer( pLinksManager );
	pAcceptGamersNet->StartGame();
	pAcceptGamersNet->StartNewPlayerAccept();

	return true;
}

bool CPlayGameProcessor::ProcessClientWasKicked( CGameClientWasKicked *pPacket )
{
	PushPacket( pPacket );
	connections.erase( pPacket->nKicked );
	connectionEfforts.erase( pPacket->nKicked );

	return true;
}

void CPlayGameProcessor::SendGamePacket( CNetPacket *pPacket, bool bBroadcast )
{
	CPtr<CNetPacket> pRemPacket = pPacket;

	if ( !bBroadcast )
	{
		TConnections::iterator iter = connections.find( pPacket->nClientID );
		if ( iter != connections.end() )
			iter->second->SendGamePacket( pPacket );
	}
	else
	{
		for ( TConnections::iterator iter = connections.begin(); iter != connections.end(); ++iter )
			iter->second->SendGamePacket( pPacket );
	}
}

void CPlayGameProcessor::Clear()
{
	pAcceptGamersNet = 0;
	connections.clear();
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

bool CPlayGameProcessor::ProcessKickClient( CGameKickClient *pPacket )
{
	if ( nOurGameID == pPacket->nGameID )
		pNet->SendPacket( pPacket );

	return true;
}

bool CPlayGameProcessor::ProcessGameKilled( CGameKilled *pPacket )
{
	if ( nOurGameID == pPacket->nGame )
	{
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
	TConnections::iterator iter = connections.find( pPacket->nDeadClient );
	if ( iter != connections.end() )
	{
		PushPacket( new CGameClientRemoved( pPacket->nDeadClient ) );
		connections.erase( iter );
		return true;
	}

	return false;
}

void CPlayGameProcessor::TogglePauseAcceptGamersNet( bool bPause )
{
	if ( pAcceptGamersNet )
		pAcceptGamersNet->DebugTogglePause( bPause );
}

void CPlayGameProcessor::TogglePauseConnectGamer( const int nGamer, bool bPause )
{
	TConnections::iterator iter = connections.find( nGamer );
	if ( iter != connections.end() )
		iter->second->TogglePauseNet( bPause );
}

BASIC_REGISTER_CLASS( CLIENT, CPlayGameProcessor )


