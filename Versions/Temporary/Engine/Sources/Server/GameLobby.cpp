#include "stdafx.h"

#include "Clients.h"
#include "GameLobby.h"
#include "InternalPackets.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/LobbiesIDs.h"
#include "Misc/Time64.h"
#include "Statistics.h"
#include "System/XmlSaver.h"

CGameLobby::CGameLobby( CClients *_pClients, const string &_szCfgFile )
: pClients( _pClients ), nGamesCounter( 0 ), szCfgFile( _szCfgFile )
{
	REGISTER_PACKET_PROCESSOR( ProcessCommonClientStatePacket );
	REGISTER_PACKET_PROCESSOR( ProcessGameHeartBeatPacket );
	REGISTER_PACKET_PROCESSOR( ProcessGameStartLoadingPacket );
	REGISTER_PACKET_PROCESSOR( ProcessConnectGamePacket );
	REGISTER_PACKET_PROCESSOR( ProcessLeaveGame );
	REGISTER_PACKET_PROCESSOR( ProcessWant2Connect2Client );
	REGISTER_PACKET_PROCESSOR( ProcessKickClient );

	REGISTER_PACKET_PROCESSOR( ProcessEnterLobby );
	REGISTER_PACKET_PROCESSOR( ProcessLeaveLobby );
	REGISTER_PACKET_PROCESSOR( ProcessRemoveClient );
	REGISTER_PACKET_PROCESSOR( ProcessGetLobbyClients );
	REGISTER_PACKET_PROCESSOR( ProcessCreateGame );
	REGISTER_PACKET_PROCESSOR( ProcessKillGame );
	REGISTER_PACKET_PROCESSOR( ProcessGetLobbyGames );
	REGISTER_PACKET_PROCESSOR( ProcessUpdateGame );
	REGISTER_PACKET_PROCESSOR( ProcessSpecificGameInfo );
	REGISTER_PACKET_PROCESSOR( ProcessThroughServerConnection );

	// for testing
	REGISTER_PACKET_PROCESSOR( ProcessGetLobbyClientsListPacket );
	REGISTER_PACKET_PROCESSOR( ProcessShowLobbyGames );

	ReloadConfig();

	WriteMSG( "GameLobby: Timeout to delete dead games is %d\n", dwGameTimeOut );
	WriteMSG( "GameLobby: Timeout to delete dead games in loading state is %d\n", dwGameLoadingTimeOut ); 
	NStatistics::SetGlobalCounter( "TotalGamesCreated", NStatistics::CreateEventsCounter() );
	NStatistics::SetGlobalCounter( "ThroughServerConnections", NStatistics::CreateEventsCounter() );
}

void CGameLobby::ReloadConfig()
{
	CFileStream stream( szCfgFile, CFileStream::WIN_READ_ONLY );
	NI_ASSERT( stream.IsOk(), StrFmt( "Could not open cfg file: %s", szCfgFile ) );
	CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
	NI_ASSERT( pSaver.GetPtr(), "Could not create XML saver" );
	pSaver->Add( "GameTimeout", &dwGameTimeOut );
	pSaver->Add( "GameLoadingTimeout", &dwGameLoadingTimeOut );
}

void CGameLobby::SetClientLobbyID( const int nClientID, const BYTE cLobbyID )
{
	SCommonClientInfo clientInfo;
	if ( pClients->GetCommonClientInfo( nClientID, &clientInfo ) )
	{
		clientInfo.cLobbyID = cLobbyID;
		pClients->SetCommonClientInfo( nClientID, clientInfo );
		clientsVersions.Change( nClientID );
	}
}

void CGameLobby::SetClientGameID( const int nClientID, const int nGameID )
{
	SCommonClientInfo clientInfo;
	if ( pClients->GetCommonClientInfo( nClientID, &clientInfo ) )
	{
		clientInfo.nGameID = nGameID;
		pClients->SetCommonClientInfo( nClientID, clientInfo );
		clientsVersions.Change( nClientID );
	}
}

void CGameLobby::ClientEntered( const int nID )
{
	lobbyClients.insert( nID );
	clientsVersions.Add( nID );
	SetClientLobbyID( nID, GetLobbyID() );
}

void CGameLobby::ClientLeaved( const int nID )
{
	if ( lobbyClients.find( nID ) != lobbyClients.end() )
	{
		lobbyClients.erase( nID );
		clientsVersions.Remove( nID );

		for ( hash_map<int, SLobbyGameInfo>::iterator iter = lobbyGames.begin(); iter != lobbyGames.end(); ++iter )
		{
			SLobbyGameInfo &gameInfo = iter->second;
			hash_set<int>::iterator clients_iter = gameInfo.clients.find( nID );
			if ( clients_iter != gameInfo.clients.end() )
				gameInfo.clients.erase( nID );

			SetClientGameID( nID, -1 );
			GameInfoChanged( iter->first );

		}

		InformThroughServerClients( nID );

		SetClientLobbyID( nID, ERID_NO_LOBBY );
	}
}

void CGameLobby::GameInfoChanged( const int nID )
{
	if ( lobbyGames.find( nID ) != lobbyGames.end() )
	{
		gamesVersions.Change( nID );
		const UINT64 nTime = GetLongTickCount();
		if ( lobbyGames[nID].nLastGameHeartBeat < nTime )
			lobbyGames[nID].nLastGameHeartBeat = nTime;
		if ( lobbyGames[nID].gameInfo.nPlayers > 0 && lobbyGames[nID].clients.size() == 0 )
			KillGame( nID );
		else
			lobbyGames[nID].gameInfo.nPlayers = lobbyGames[nID].clients.size();
	}
}

bool CGameLobby::ProcessCommonClientStatePacket( CCommonClientStatePacket *pPacket )
{
	if ( lobbyClients.find( pPacket->nClientID ) != lobbyClients.end() )
	{
		clientsVersions.Change( pPacket->nClientID );
		return true;
	}
	else
		return false;
}

bool CGameLobby::ProcessGetLobbyClientsListPacket( CGetLobbyClientsListPacket *pPacket )
{
	if ( GetLobbyID() != pPacket->nLobbyID )
		return false;

	string szList;
	if ( lobbyClients.empty() )
		szList = "no clients online\n";
	else
	{
		szList = "Lobby clients:\n";
		for ( hash_set<int>::iterator iter = lobbyClients.begin(); iter != lobbyClients.end(); ++iter )
		{
			string szNick;
			if ( !pClients->GetNick( *iter, &szNick ) )
				szList += StrFmt( "  something wrong with client %d\n", *iter );
			else
				szList += "  " + szNick + "\n";
		}
	}

	WriteMSG( "%s", szList.c_str() );
	return true;
}

bool CGameLobby::ProcessGameHeartBeatPacket( CGameHeartBeatPacket *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;

	if ( lobbyGames.find( pPacket->nGameID ) != lobbyGames.end() )
	{
		const UINT64 nTime = GetLongTickCount();
		if ( lobbyGames[pPacket->nGameID].nLastGameHeartBeat < nTime )
			lobbyGames[pPacket->nGameID].nLastGameHeartBeat = nTime;
		return true;
	}
	else
		return false;
}

bool CGameLobby::ProcessGameStartLoadingPacket( CGameStartLoadingPacket *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;

	if ( lobbyGames.find( pPacket->nGameID ) != lobbyGames.end() )
	{
		lobbyGames[pPacket->nGameID].nLastGameHeartBeat = GetLongTickCount() + dwGameLoadingTimeOut;
		return true;
	}
	else
		return false;
}

bool CGameLobby::ProcessConnectGamePacket( CConnectGamePacket *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;

	hash_map<int, SLobbyGameInfo>::iterator iter = lobbyGames.find( pPacket->nGameID );
	if ( iter != lobbyGames.end() )
	{
		SLobbyGameInfo &gameInfo = iter->second;
		if ( !gameInfo.gameInfo.bCanConnect )
			PushPacket( new CConnectGameFailed( pPacket->nClientID, CConnectGameFailed::ER_GAME_CLOSE_TO_CONNECT ) );
		else if ( gameInfo.gameInfo.bHasPassword && gameInfo.gameInfo.szPassword != pPacket->szPassword )
			PushPacket( new CConnectGameFailed( pPacket->nClientID, CConnectGameFailed::ER_WRONG_PASSWORD ) );
		else if ( gameInfo.clients.size() >= gameInfo.gameInfo.nMaxPlayers )
			PushPacket( new CConnectGameFailed( pPacket->nClientID, CConnectGameFailed::ER_MAX_PLAYERS_REACHED ) );
		else
		{
			PushPacket( new CConnectedGameID( pPacket->nClientID, pPacket->nGameID ) );

			if ( PlayerNeedSpecificGameInfo() )
			{
				CPtr<CSpecificGameInfo> pSpecificGameInfo = new CSpecificGameInfo();
				pSpecificGameInfo->nClientID = pPacket->nClientID;
				pSpecificGameInfo->nGameID = iter->first;
				if ( iter->second.pSpecificGameInfo )
					pSpecificGameInfo->pInfo = iter->second.pSpecificGameInfo->pInfo;

				PushPacket( pSpecificGameInfo );
			}

			CPtr<CAnswerConnectGame> pAnswerPacket = new CAnswerConnectGame( pPacket->nClientID );
			for ( hash_set<int>::iterator clients_iter = gameInfo.clients.begin(); clients_iter != gameInfo.clients.end(); ++clients_iter )
				pAnswerPacket->clients.push_back( *clients_iter );
			PushPacket( pAnswerPacket );

			gameInfo.clients.insert( pPacket->nClientID );
			GameInfoChanged( pPacket->nGameID );

			SetClientGameID( pPacket->nClientID, pPacket->nGameID );
		}
	}

	return true;
}

void CGameLobby::InformThroughServerClients( const int nLeftGameClient )
{
	if ( throughServerClients.find( nLeftGameClient ) != throughServerClients.end() )
	{
		for ( hash_set<int>::iterator iter = throughServerClients[nLeftGameClient].begin(); iter != throughServerClients[nLeftGameClient].end(); ++iter )
			PushPacket( new CGameClientDead( *iter, nLeftGameClient ) );

		throughServerClients.erase( nLeftGameClient );
	}
}

bool CGameLobby::ProcessLeaveGame( CLeaveGamePacket *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;

	hash_map<int, SLobbyGameInfo>::iterator iter = lobbyGames.find( pPacket->nGameID );
	if ( iter != lobbyGames.end() )
	{
		SLobbyGameInfo &info = iter->second;
		hash_set<int>::iterator clients_iter = info.clients.find( pPacket->nClientID );
		if ( clients_iter != info.clients.end() )
		{
			info.clients.erase( clients_iter );
			GameInfoChanged( iter->first );
		}

		SetClientGameID( pPacket->nClientID, -1 );
		InformThroughServerClients( pPacket->nClientID );
	}

	return true;
}

bool CGameLobby::ProcessWant2Connect2Client( CWant2Connect2Client *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) || !IsLobbyClient( pPacket->nClient2Connect ) )
		return false;

	SGameConnection::SAddressInfo addrClient, addrClientTo;
	if ( pClients->GetGameConnectInfo( pPacket->nClientID, pPacket->nMyConnect, &addrClient ) &&
		pClients->GetGameConnectInfo( pPacket->nClient2Connect, 0, &addrClientTo ) )
	{
		CPtr<CNetPacket> pInfoToClient = 
			new CClientGameConnectInfo( pPacket->nClientID, addrClientTo.szIP, addrClientTo.nPort, pPacket->nClient2Connect );
		CPtr<CNetPacket> pInfoToClientToConnect =
			new CClientWantToConnect( pPacket->nClient2Connect, pPacket->nClientID, addrClient.szIP, addrClient.nPort );

		PushPacket( pInfoToClient );
		PushPacket( pInfoToClientToConnect );
	}

	return true;
}

bool CGameLobby::ProcessKickClient( CGameKickClient *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;

	if ( !PlayerCanKickPlayer() )
		return true;

	hash_map<int, SLobbyGameInfo>::iterator iter = lobbyGames.find( pPacket->nGameID );
	if ( iter != lobbyGames.end() )
	{
		SLobbyGameInfo &info = iter->second;
		for ( hash_set<int>::iterator clients_iter = info.clients.begin(); clients_iter != info.clients.end(); ++clients_iter )
		{
			const int nClientID = *clients_iter;
			PushPacket( new CGameClientWasKicked( nClientID, pPacket->nKicked ) );
		}

		hash_set<int>::iterator clients_iter = info.clients.find( pPacket->nKicked );
		if ( clients_iter != info.clients.end() )
		{
			info.clients.erase( clients_iter );
			GameInfoChanged( iter->first );
			SetClientGameID( pPacket->nKicked, -1 );
		}
	}

	return true;
}

bool CGameLobby::ProcessEnterLobby( CEnterLobbyPacket *pPacket )
{
	if ( IsLobbyClient( pPacket->nClientID ) )
		RemoveLobbyClient( pPacket->nClientID );

	if ( pPacket->nLobbyID != GetLobbyID() )
		return false;

	ClientEntered( pPacket->nClientID );
	ClientEnterToLobby( pPacket->nClientID );

	// Сообщение о том, что вход в лобби прошел успешно 
	PushPacket( new CEnterLobbyPacket( pPacket->nClientID, pPacket->nLobbyID ) );
	//
	return false; // Оставляем пакет для других лобби
}

bool CGameLobby::ProcessLeaveLobby( CLeaveLobbyPacket *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;

	RemoveLobbyClient( pPacket->nClientID );

	return true;
}

void CGameLobby::RemoveLobbyClient( const int nClientID )
{
	ClientLeaved( nClientID );
	ClientLeaveLobby( nClientID );
}

bool CGameLobby::ProcessRemoveClient( CNetRemoveClient *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;

	RemoveLobbyClient( pPacket->nClientID );

	return false;
}

bool CGameLobby::ProcessCreateGame( CCreateGamePacket *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;

	const int nGameID = CreateGame( 2 );
	lobbyGames[nGameID].clients.insert( pPacket->nClientID );
	SetClientGameID( pPacket->nClientID, nGameID );
	PushPacket( new CConnectedGameID( pPacket->nClientID, nGameID ) );
	GameInfoChanged( nGameID );

	if ( pPacket->pInitialGameInfo )
	{
		SLobbyGameInfo &lobbyGameInfo = lobbyGames[nGameID];
		lobbyGameInfo.gameInfo = *( pPacket->pInitialGameInfo );
		lobbyGameInfo.gameInfo.nID = nGameID;
		lobbyGameInfo.gameInfo.nPlayers = lobbyGameInfo.clients.size();
	}

	return true;
}

int CGameLobby::CreateGame( const int nMaxPlayers )
{
	const int nGameID = ++nGamesCounter;
	lobbyGames.erase( nGameID );
	lobbyGames[nGameID].nLastGameHeartBeat = GetLongTickCount();
	lobbyGames[nGameID].clients.clear();
	lobbyGames[nGameID].gameInfo.nID = nGameID;
	lobbyGames[nGameID].gameInfo.nMaxPlayers = nMaxPlayers;

	lobbyGames[nGameID].pSpecificGameInfo = new CSpecificGameInfo();
	lobbyGames[nGameID].pSpecificGameInfo->nGameID = nGameID;

	gamesVersions.Add( nGameID );

	NStatistics::GetGlobal( "TotalGamesCreated" )->Add( 1.0f );

	return nGameID;
}

void CGameLobby::EraseGameClients( const int nGameID )
{
	for ( hash_set<int>::iterator iter = lobbyGames[nGameID].clients.begin(); iter != lobbyGames[nGameID].clients.end(); ++iter )
	{
		CPtr<CNetPacket> pPacket = new CGameKilled( *iter, nGameID );
		PushPacket( pPacket );
		SetClientGameID( *iter, -1 );
	}
}

bool CGameLobby::KillGame( const int nGameID )
{
	if ( lobbyGames.find( nGameID ) != lobbyGames.end() )
	{
		EraseGameClients( nGameID );

		lobbyGames.erase( nGameID );
		gamesVersions.Remove( nGameID );
		GameDead( nGameID );
		return true;
	}
	return false;
}

bool CGameLobby::ProcessKillGame( CKillGamePacket *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;

	const int nGameID = pPacket->nGameID;
	return KillGame( nGameID );	
}

bool CGameLobby::ProcessGetLobbyGames( CGetLobbyGamesPacket *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;

	if ( !PlayerCanSeeGamesList() )
		return true;

	SGetGameInfoFunc getGameInfo( this );
	GetUpdate( pPacket, (CLobbyGamesPacket*)0, getGameInfo, false );

	return true;
}

bool CGameLobby::ProcessGetLobbyClients( CGetLobbyClientsPacket *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;

	GetLobbyClients( pPacket );
	return true;
}

bool CGameLobby::ProcessUpdateGame( CUpdateGameInfo *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;

	if ( lobbyGames.find( pPacket->gameInfo.nID ) == lobbyGames.end() )
		return false;

	if ( !PlayerCanUpdateGameInfo() )
		return true;

	GameInfoChanged( pPacket->gameInfo.nID );
	lobbyGames[pPacket->gameInfo.nID].gameInfo = pPacket->gameInfo;

	return true;
}

const bool CGameLobby::GetGameInfo( const int nGameID, SGameInfo *pInfo ) const
{
	hash_map<int, SLobbyGameInfo>::const_iterator iter = lobbyGames.find( nGameID );
	if ( iter != lobbyGames.end() )
	{
		*pInfo = iter->second.gameInfo;
		pInfo->szPassword = "";
		return true;
	}
	else
		return false;
}

bool CGameLobby::ProcessSpecificGameInfo( CSpecificGameInfo *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;

	if ( !PlayerNeedSpecificGameInfo() )
		return true;

	hash_map<int, SLobbyGameInfo>::iterator iter = lobbyGames.find( pPacket->nGameID );
	if ( iter != lobbyGames.end() )
	{
		iter->second.pSpecificGameInfo = pPacket;

		SLobbyGameInfo &gameInfo = iter->second;
		for ( hash_set<int>::iterator clients_iter = gameInfo.clients.begin(); clients_iter != gameInfo.clients.end(); ++clients_iter )
		{
			if ( *clients_iter != pPacket->nClientID )
			{
				CPtr<CSpecificGameInfo> pSpecificGameInfo = new CSpecificGameInfo();
				pSpecificGameInfo->nClientID = *clients_iter;
				pSpecificGameInfo->nGameID = pPacket->nGameID;
				if ( pPacket )
					pSpecificGameInfo->pInfo = pPacket->pInfo;

				PushPacket( pSpecificGameInfo );
			}
		}

		return true;
	}
	else
		return false;
}

bool CGameLobby::Segment()
{
	const UINT64 nTime = GetLongTickCount();
	list<int> deadGames;
	for ( hash_map<int, SLobbyGameInfo>::iterator iter = lobbyGames.begin(); iter != lobbyGames.end(); ++iter )
	{
		const int nGameID = iter->first;
		if ( nTime > iter->second.nLastGameHeartBeat + dwGameTimeOut )
		{
#ifndef CONSOLE_LOG_SILENCE
			WriteMSG( "game \"%s\", number %d is dead, deleted\n",
				lobbyGames[nGameID].gameInfo.szName.c_str(), nGameID );
#endif
			deadGames.push_back( nGameID );
		}
	}

	while ( !deadGames.empty() )
	{
		const int nGameID = deadGames.front();
		deadGames.pop_front();

		EraseGameClients( nGameID );

		lobbyGames.erase( nGameID );
		gamesVersions.Remove( nGameID );
		GameDead( nGameID );
	}

	return true;
}

bool CGameLobby::GetGameClients( const int nGame, hash_set<int> *pClients )
{
	hash_map<int, SLobbyGameInfo>::iterator iter = lobbyGames.find( nGame );
	if ( iter == lobbyGames.end() )
		return false;
	else
	{
		*pClients = iter->second.clients;
		return true;
	}
}

bool CGameLobby::ProcessThroughServerConnection( CThroughServerConnectionPacket *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;

	hash_map<int, SLobbyGameInfo>::iterator iter = lobbyGames.find( pPacket->nGameID );
	if ( iter == lobbyGames.end() )
		return false;
	else
	{
		hash_set<int> &clients = iter->second.clients;
		if ( clients.find( pPacket->nClientWith ) == clients.end() )
			PushPacket( new CGameClientDead( pPacket->nClientID, pPacket->nClientWith ) );
		else
		{
			throughServerClients[pPacket->nClientWith].insert( pPacket->nClientID );
			NStatistics::GetGlobal( "ThroughServerConnections" )->Add( 1.0f );
#ifndef CONSOLE_LOG_SILENCE
			string szClientNick, szClientWithNick;
			if ( pClients->GetNick( pPacket->nClientID, &szClientNick ) && 
				pClients->GetNick( pPacket->nClientWith, &szClientWithNick ) )
			{
				WriteMSG( "clients %s and %s are connected through the server\n", 
					szClientNick.c_str(), szClientWithNick.c_str() );
			}
#endif
		}

		return true;
	}
}

bool CGameLobby::ProcessShowLobbyGames( CShowLobbyGamesPacket *pPacket )
{
	if ( pPacket->nLobbyID != GetLobbyID() )
		return false;

	string szStr;
	if ( lobbyGames.empty() )
		szStr = "no games is online\n";
	else
	{
		szStr = "games:\n";
		for ( hash_map<int, SLobbyGameInfo>::iterator iter = lobbyGames.begin(); iter != lobbyGames.end(); ++iter )
		{
			SGameInfo &info = iter->second.gameInfo;
			NI_ASSERT( iter->first == info.nID, StrFmt( "wrong ids for game, %d->%d", iter->first, info.nID ) );
			szStr += StrFmt( "    \"%s\", id %d, max players %d, has_password-%d, password \"%s\"", 
				info.szName.c_str(), info.nID, 
				info.nMaxPlayers, (int)info.bHasPassword, info.szPassword.c_str() );
			if ( info.bCanConnect )
				szStr += ", can connect";
			else
				szStr += ", cannot connect";

			hash_set<int> clients;
			if ( !GetGameClients( info.nID, &clients ) )
				szStr += ", something wrong with the game";
			else
			{
				szStr += "\n    ";
				if ( clients.empty() )
					szStr += "no clients";
				else
				{
					hash_set<int>::iterator iter = clients.begin();
					szStr += StrFmt( "clients: %d", *iter );
					++iter;
					while ( iter != clients.end() )
					{
						szStr += StrFmt( ", %d", *iter );
						++iter;
					}
				}
			}

			szStr += "\n";
		}
	}

	WriteMSG( "%s", szStr.c_str() );

	return true;
}



BASIC_REGISTER_CLASS( CGameLobby );


