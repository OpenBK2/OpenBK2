#include "stdafx.h"

#include "clients.h"
#include "CustomLobby.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/CustomLobbyPackets.h"
#include "Server_Client_Common/LobbiesIDs.h"
#include "Statistics.h"

CCustomLobby::CCustomLobby( class CClients *pClients, const string &szCfgFile )
: CGameLobby( pClients, szCfgFile )
{
	nLobbyID = ERID_CUSTOM;
	pStatisticsCollector = NStatistics::CreateCollector( "CUSTOM" );
	pStatisticsCollector->SetSpecific( "PlayersAverage", NStatistics::CreateAverageValueCounter() );
	pStatisticsCollector->SetSpecific( "GamesTotal", NStatistics::CreateEventsCounter() );
	pStatisticsCollector->SetSpecific( "GamesPerSecond", NStatistics::CreateAverageValuePerTimeCounter() );
	pStatisticsCollector->SetSpecific( "TotalPlayersEntered", NStatistics::CreateEventsCounter() );
	pStatisticsCollector->SetSpecific( "GamesAverage", NStatistics::CreateAverageValueCounter() );
	nPlayersTotal = 0;
	nGamesTotal = 0;
}

const bool CCustomLobby::GetClientInfo( const int nClientID, SCustomLobbyClientInfo *pInfo ) const
{
	SCommonClientInfo commonClientInfo;
	string szNick;
	if ( GetClients()->GetCommonClientInfo( nClientID, &commonClientInfo ) && GetClients()->GetNick( nClientID, &szNick ) )
	{
		pInfo->eState = commonClientInfo.eState;
		pInfo->bWant2ReceiveChat = commonClientInfo.bWant2ReceiveChat;
		pInfo->szNick = szNick;
		pInfo->nID = nClientID;
		pInfo->nGameID = commonClientInfo.nGameID;

		return true;
	}
	else
		return false;
}

void CCustomLobby::GetLobbyClients( CGetLobbyClientsPacket *pPacket )
{
	SGetClientInfoFunc getClientInfo( this );
	GetUpdate( pPacket, (CCustomLobbyClientsPacket*)0, getClientInfo, true );
}

bool CCustomLobby::Segment()
{
	CGameLobby::Segment();
	(*pStatisticsCollector)[ "PlayersAverage" ]->Add( float( nPlayersTotal ) );
	(*pStatisticsCollector)[ "GamesAverage" ]->Add( float( nGamesTotal ) );
	return true;
}

void CCustomLobby::ClientEnterToLobby( const int nClientID )
{
	(*pStatisticsCollector)[ "TotalPlayersEntered" ]->Add( 1.0f );
	++nPlayersTotal;
}

void CCustomLobby::ClientLeaveLobby( const int nClientID )
{
	--nPlayersTotal;
}

int CCustomLobby::CreateGame( const int nMaxPlayers )
{
	(*pStatisticsCollector)[ "GamesTotal" ]->Add( 1.0f );
	(*pStatisticsCollector)[ "GamesPerSecond" ]->Add( 1.0f );
	++nGamesTotal;
	return CGameLobby::CreateGame( nMaxPlayers );
}

void CCustomLobby::GameDead( const int nGameID )
{
	--nGamesTotal;
}


