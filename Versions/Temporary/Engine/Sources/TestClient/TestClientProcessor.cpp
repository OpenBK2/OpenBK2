#include "stdafx.h"

#include "InternalPackets.h"
#include "TestClientProcessor.h"
#include "../Client/ServerClient.h"
#include "../Misc/StrProc.h"
#include "../Server_Client_Common/CommonPackets.h"
#include "../Server_Client_Common/CustomLobbyPackets.h"
#include "../Server_Client_Common/GamePackets.h"
#include "../Server_Client_Common/LadderLobbyPackets.h"
#include "../Server_Client_Common/LoginPackets.h"
#include "../Server_Client_Common/ChatPackets.h"

CTestClientProcessor::CTestClientProcessor( const string &szCfgFile )
{
	REGISTER_PACKET_PROCESSOR( ProcessConnectServerResult );
	REGISTER_PACKET_PROCESSOR( ProcessChatPacket );
	REGISTER_PACKET_PROCESSOR( ProcessMyIDPacket );
	REGISTER_PACKET_PROCESSOR( ProcessRemoveClient );
	REGISTER_PACKET_PROCESSOR( ProcessEnteredLobby );
	REGISTER_PACKET_PROCESSOR( ProcessGetLobbyClientsPacket );
	REGISTER_PACKET_PROCESSOR( ProcessCustomLobbyClients );
	REGISTER_PACKET_PROCESSOR( ProcessCreateGame );
	REGISTER_PACKET_PROCESSOR( ProcessUpdateGameInfo );
	REGISTER_PACKET_PROCESSOR( ProcessGetLobbyGamesPacket );
	REGISTER_PACKET_PROCESSOR( ProcessCustomLobbyGamesPacket );
	REGISTER_PACKET_PROCESSOR( ProcessKillGame );
	REGISTER_PACKET_PROCESSOR( ProcessLeaveGame );
	REGISTER_PACKET_PROCESSOR( ProcessConnectGameFailed );
	REGISTER_PACKET_PROCESSOR( ProcessGameClientRemoved );
	REGISTER_PACKET_PROCESSOR( ProcessNewGameClient );
	REGISTER_PACKET_PROCESSOR( ProcessGameKilled );
	REGISTER_PACKET_PROCESSOR( ProcessConnectGame );
	REGISTER_PACKET_PROCESSOR( ProcessGameTestBroadcastMsg );
	REGISTER_PACKET_PROCESSOR( ProcessGameTestDirectMsg );
	REGISTER_PACKET_PROCESSOR( ProcessConnectedGameID );
	REGISTER_PACKET_PROCESSOR( ProcessClientWasKicked );
	REGISTER_PACKET_PROCESSOR( ProcessGameKickClient );
	REGISTER_PACKET_PROCESSOR( ProcessTestDirectPacket );
	REGISTER_PACKET_PROCESSOR( ProcessShowGameClients );
	REGISTER_PACKET_PROCESSOR( ProcessSendSpecGameInfo );
	REGISTER_PACKET_PROCESSOR( ProcessReceiveSpecificGameInfo );

	REGISTER_PACKET_PROCESSOR( ProcessChatChannelClientsListPacket );
	REGISTER_PACKET_PROCESSOR( ProcessChatClientListChangeNotifyPacket );
	REGISTER_PACKET_PROCESSOR( ProcessChatChannelsListPacket );
	REGISTER_PACKET_PROCESSOR( ProcessChatIgnoreListPacket );
	REGISTER_PACKET_PROCESSOR( ProcessChatChannelByNickPacket );
	REGISTER_PACKET_PROCESSOR( ProcessChatFriendNotifyPacket );

	REGISTER_PACKET_PROCESSOR( ProcessLadderInvitePacket );
	REGISTER_PACKET_PROCESSOR( ProcessLadderStatisticsPacket );
	REGISTER_PACKET_PROCESSOR( ProcessForgottenPasswordAnswerPacket );
	REGISTER_PACKET_PROCESSOR( ProcessWelcomePacket );
	REGISTER_PACKET_PROCESSOR( ProcessPingPacket );

	CFileStream stream( szCfgFile, CFileStream::WIN_READ_ONLY );
	CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
	pSaver->Add( "GameHeartBeatPeriod", &dwHeartBeatPeriod );
	pSaver = 0;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "Games heartbeat period is %d\n", dwHeartBeatPeriod ) );
	nMyID = -1;
}

void CTestClientProcessor::SetServerClient( CServerClient *_pServerClient )
{
	pServerClient = _pServerClient;
	nGameID = -1;
}

bool CTestClientProcessor::ProcessConnectServerResult( CConnectServerPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;

	string szMessage;
	if ( pPacket->eConnectionState == CConnectServerPacket::ECS_SUCCESS )
		szMessage = "Connected!\n";
	else
	{
		szMessage = "Connection failed, ";
		switch ( pPacket->eRejectReason )
		{
		case CConnectServerPacket::ERR_CANT_RESOLVE_ADDRESS: 
			szMessage += "can't resolve address\n";
			break;
		case CConnectServerPacket::ERR_CONNECTION_LOST:
			szMessage += "connection lost\n";
			break;

		case CConnectServerPacket::ERR_BAD_NICK:
			szMessage += "bad nick\n";
			break;

		case CConnectServerPacket::ERR_WRONG_CD_KEY:
			szMessage += "wrong cd key\n";
			break;

		case CConnectServerPacket::ERR_ALREADY_REGISTERED:
			szMessage += "already registered\n";
			break;

		case CConnectServerPacket::ERR_ALREADY_ONLINE:
			szMessage += "already online\n";
			break;

		case CConnectServerPacket::ERR_NOT_REGISTERED:
			szMessage += "not registered\n";
			break;

		case CConnectServerPacket::ERR_BANNED_NICK:
			szMessage += "banned nick\n";
			break;

		case CConnectServerPacket::ERR_BANNED_CDKEY:
			szMessage += "banned cdkey\n";
			break;

		case CConnectServerPacket::ERR_WRONG_PASSWORD:
			szMessage += "wrong password\n";
			break;
		case CConnectServerPacket::ERR_KICKED:
			szMessage += "kicked\n";
			break;
		case CConnectServerPacket::ERR_LOGIN_TIMEOUT:
			szMessage += "login timeout\n";
			break;
		case CConnectServerPacket::ERR_WRONG_NET_VERSION:
			szMessage += "wrong net version\n";
			break;
		default:
			szMessage += "reason unknown\n";
		}
	}

	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, szMessage.c_str() );

	return true;
}

bool CTestClientProcessor::ProcessChatPacket( CChatPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;

	if ( pPacket->bIsBroadcast )
	{
		Singleton<IConsoleBuffer>()->WriteASCII(
			CONSOLE_STREAM_CONSOLE,
			StrFmt( "lobby chat message from %s: %s\n", pPacket->szNick.c_str(), NStr::ToMBCS( pPacket->wszMessage.c_str() ) ) );
	}
	else
	{
		Singleton<IConsoleBuffer>()->WriteASCII(
			CONSOLE_STREAM_CONSOLE,
			StrFmt( "private chat message from %s: %s\n", pPacket->szNick.c_str(), NStr::ToMBCS( pPacket->wszMessage ).c_str() ) );
	}

	return true;
	
}

bool CTestClientProcessor::ProcessChatChannelClientsListPacket( CChatChannelClientsListPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "Channel clients:\n" );
	for ( list<SIDNickPair>::iterator it = pPacket->clientsList.begin(); it != pPacket->clientsList.end(); ++it )
	{
		const SIDNickPair &listPair = *it;
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "%s\n", listPair.szNick ) );
	}
	return true;
}

bool CTestClientProcessor::ProcessChatClientListChangeNotifyPacket( CChatClientListChangeNotifyPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	if ( pPacket->bJoined )
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "%s joined channel\n", pPacket->szNick ) );
	}
	else
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "%s leaved channel\n", pPacket->szNick ) );
	}
	return true;
}

bool CTestClientProcessor::ProcessChatChannelsListPacket( CChatChannelsListPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	// Please, process the differences list correctly in your own code.. I'm so lazy..
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "Channel(s):\n" );
	for ( list<string>::iterator it = pPacket->added.begin(); it != pPacket->added.end(); ++it )
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "%s\n", *it ) );
	}
	return true;
}

bool CTestClientProcessor::ProcessChatIgnoreListPacket( CChatIgnoreFriendListPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "Your ignore list:\n" );
	if ( !pPacket->ignoreList.empty() )
	{
		for ( list<string>::iterator it = pPacket->ignoreList.begin(); it != pPacket->ignoreList.end(); ++it )
		{
			Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "%s\n", *it ) );
		}
	}
	else
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, ".. is empty\n" );
	}

	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "Your friend list:\n" );
	if ( !pPacket->friendList.empty() )
	{
		for ( list<string>::iterator it = pPacket->friendList.begin(); it != pPacket->friendList.end(); ++it )
		{
			Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "%s\n", *it ) );
		}
	}
	else
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, ".. is empty\n" );
	}

	return true;

}

bool CTestClientProcessor::ProcessChatChannelByNickPacket( CChatChannelByNickPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	if ( pPacket->szChannel != "" )
	{
		string szOutString = StrFmt( "User %s is in ", pPacket->szNick ) + pPacket->szChannel + ".\n";
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, szOutString.c_str() );
	}
	else
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, 
			StrFmt( "User %s is not in chat.\n", pPacket->szNick ) );
	return true;
}

bool CTestClientProcessor::ProcessChatFriendNotifyPacket( CChatFriendNotifyPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	string szStatus;
	switch( EChatStatus( pPacket->cChatStatus ) )
	{
	case ONLINE:
		szStatus = "online";
		break;
	case AFK:
		szStatus = "afk";
		break;
	default:
		szStatus = "offline";
	}
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "Notification from friend. %s is now %s.\n", 
		pPacket->szNick.c_str(), szStatus.c_str() ) );
	return true;
}

bool CTestClientProcessor::ProcessMyIDPacket( CMyIDPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;

	nMyID = pPacket->nMyID;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "my id is %d\n", nMyID ) );
	return true;
}

bool CTestClientProcessor::ProcessWelcomePacket( CSystemBroadcastPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "%s\n", NStr::ToMBCS( pPacket->wszText ) ) );
	return true;
}

bool CTestClientProcessor::ProcessRemoveClient( CNetRemoveClient *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;

	NI_ASSERT( pPacket->nClientID == 0, StrFmt( "wrong client %d removed, 0 expected\n", pPacket->nClientID ) );
	if ( pPacket->nClientID == 0 )
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "connection with server was lost\n" ) );

	return true;
}

bool CTestClientProcessor::ProcessEnteredLobby( CEnteredLobby *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	dwClientsVersion = 0;
	dwGamesVersion = 0;

	return true;
}

bool CTestClientProcessor::ProcessGetLobbyClientsPacket( CGetLobbyClientsPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;	

	pPacket->dwVersion = dwClientsVersion;
	if ( pServerClient )
		pServerClient->SendPacket( pPacket );
	else
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "not connected yet\n" );

	return true;
}

bool CTestClientProcessor::ProcessCustomLobbyClients( CCustomLobbyClientsPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;	

	dwClientsVersion = pPacket->dwVersion;
	if ( pPacket->bFullUpdate )
		lobbyClients.clear();

	for ( list<int>::iterator iter = pPacket->removed.begin(); iter != pPacket->removed.end(); ++iter )
		lobbyClients.erase( *iter );

	if ( pPacket->added.empty() && pPacket->changed.empty() && lobbyClients.empty() )
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "no clients except me\n" );
	else
	{
		for ( list<SCustomLobbyClientInfo>::iterator iter = pPacket->added.begin(); iter != pPacket->added.end(); ++iter )
		{
			SCustomLobbyClientInfo &info = *iter;
			lobbyClients[info.nID] = info;
		}

		for ( list<SCustomLobbyClientInfo>::iterator iter = pPacket->changed.begin(); iter != pPacket->changed.end(); ++iter )
		{
			SCustomLobbyClientInfo &info = *iter;
			lobbyClients[info.nID] = info;
		}

		string szStr = StrFmt( "lobby clients received, version %d", dwClientsVersion );
		if ( pPacket->bFullUpdate )
			szStr += ", fullupdate";
		szStr += ":\n";

		for ( hash_map<int, SCustomLobbyClientInfo>::iterator iter = lobbyClients.begin(); iter != lobbyClients.end(); ++iter )
		{
			SCustomLobbyClientInfo &info = iter->second;
			szStr += StrFmt( "    client %s, ", info.szNick.c_str() );

			if ( info.bWant2ReceiveChat )
				szStr += "receiving chat, ";
			else
				szStr += "is not receiving chat, ";

			switch ( info.eState )
			{
			case ES_ONLINE:
				szStr += "online";
				break;
			case ES_AWAY:
				szStr += "away";
				break;
			case ES_INGAME:
				szStr += "ingame";
				break;
			default:
				szStr += StrFmt( "unknown state %d", info.eState );
			}

			if ( info.nGameID == -1 )
				szStr += ", not in a game";
			else
				szStr += StrFmt( ", in game %d", info.nGameID );

			szStr += "\n";
		}

		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, szStr.c_str() );
	}

	return true;
}

bool CTestClientProcessor::ProcessCreateGame( CCreateGamePacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;

	if ( nGameID != -1 )
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "cannot create game, in game now\n" );
	else
	{
		pServerClient->SendPacket( pPacket );
		dwLastGameUpdate = GetTickCount();
		nGameID = nMyID;
	}

	return true;
}

bool CTestClientProcessor::ProcessKillGame( CKillGamePacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;

	if ( pPacket->nGameID == -1 && nGameID == -1 )
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "cannot kill game, not in a game now\n" );
	else
	{
		pPacket->nClientID = 0;
		if ( pPacket->nGameID == -1 )
		{
			pPacket->nGameID = nGameID;
			nGameID = -1;
		}
		pServerClient->SendPacket( pPacket );

		gameClients.clear();
	}

	return true;
}

bool CTestClientProcessor::ProcessUpdateGameInfo( CUpdateGameInfo *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;

	if ( nGameID == -1 )
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "cannot update game info, not in a game now\n" );
	else
	{
		pPacket->nClientID = 0;
		pPacket->gameInfo.nID = nGameID;
		pServerClient->SendPacket( pPacket );

		dwLastGameUpdate = GetTickCount();
	}

	return true;
}

bool CTestClientProcessor::ProcessGetLobbyGamesPacket( CGetLobbyGamesPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	pPacket->dwVersion = dwGamesVersion;
	pServerClient->SendPacket( pPacket );

	return true;
}

bool CTestClientProcessor::ProcessCustomLobbyGamesPacket( CLobbyGamesPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;	

	dwGamesVersion = pPacket->dwVersion;
	if ( pPacket->bFullUpdate )
		lobbyGames.clear();

	for ( list<int>::iterator iter = pPacket->removed.begin(); iter != pPacket->removed.end(); ++iter )
		lobbyGames.erase( *iter );

	if ( pPacket->added.empty() && pPacket->changed.empty() && lobbyGames.empty() )
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "no games in the lobby\n" );
	else
	{
		for ( list<SGameInfo>::iterator iter = pPacket->added.begin(); iter != pPacket->added.end(); ++iter )
		{
			SGameInfo &info = *iter;
			lobbyGames[info.nID] = info;
		}

		for ( list<SGameInfo>::iterator iter = pPacket->changed.begin(); iter != pPacket->changed.end(); ++iter )
		{
			SGameInfo &info = *iter;
			lobbyGames[info.nID] = info;
		}

		string szStr = StrFmt( "lobby games received, version %d", dwGamesVersion );
		if ( pPacket->bFullUpdate )
			szStr += ", fullupdate";
		szStr += ":\n";

		for ( hash_map<int, SGameInfo>::iterator iter = lobbyGames.begin(); iter != lobbyGames.end(); ++iter )
		{
			SGameInfo &info = iter->second;
			szStr += StrFmt( "    \"%s\", id %d, max players %d", info.szName.c_str(), info.nID, info.nMaxPlayers );
			if ( info.bCanConnect )
				szStr += ", can connect";
			else
				szStr += ", cannot connect";

			szStr += "\n";
		}

		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, szStr.c_str() );
	}

	return true;
}

bool CTestClientProcessor::ProcessLeaveGame( CLeaveGamePacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;

	if ( nGameID == -1 )
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "cannot leave game, not in a game now\n" );
	else
	{
		pPacket->nGameID = nGameID;
		pServerClient->SendPacket( pPacket );
	}

	nGameID = -1;
	gameClients.clear();

	return true;
}

bool CTestClientProcessor::ProcessConnectGameFailed( CConnectGameFailed *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;

	string szStr = "connection to game failed";

	switch ( pPacket->eReason )
	{
	case CConnectGameFailed::ER_WRONG_PASSWORD:
		szStr += ", wrong password";
		break;
	case CConnectGameFailed::ER_MAX_PLAYERS_REACHED:
		szStr += ", max players reached";
		break;
	case CConnectGameFailed::ER_GAME_CLOSE_TO_CONNECT:
		szStr += ", game is closed to connect";
		break;
	default:
		szStr += "unknown reason, something wrong";
		break;
	}
	szStr += "\n";
	WriteMSG( szStr.c_str() );

	gameClients.clear();

	return true;
}

bool CTestClientProcessor::ProcessGameClientRemoved( CGameClientRemoved *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "connection with game client %d lost\n", pPacket->nClientID ) );

	gameClients.erase( pPacket->nClientID );
	return true;
}

bool CTestClientProcessor::ProcessNewGameClient( CNewGameClient *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	WriteMSG( "client %d added to game\n", pPacket->nClientID );

	gameClients.insert( pPacket->nClientID );

	return true;
}

bool CTestClientProcessor::ProcessGameKilled( CGameKilled *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "game %d killed\n", pPacket->nGame ) );

	gameClients.clear();
	nGameID = -1;

	return true;
}

bool CTestClientProcessor::ProcessConnectGame( CConnectGamePacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
/*
	if ( lobbyGames.find( pPacket->nGameID ) == lobbyGames.end() )
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "cant find game %d\n", pPacket->nGameID ) );
	else
*/
	pServerClient->SendPacket( pPacket );

	gameClients.clear();
	nGameID = pPacket->nGameID;

	return true;
}

bool CTestClientProcessor::ProcessGameTestBroadcastMsg( CGameTestBroadcastMsg *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	WriteMSG( "broadcast game msg from client %d received: number %d, str %s\n",
						 pPacket->nClientID, pPacket->nNumber, pPacket->szStr.c_str() );

	return true;
}

bool CTestClientProcessor::ProcessGameTestDirectMsg( CGameTestDirectMsg *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	WriteMSG( "direct game msg from client %d received: number %d, str %s\n",
						pPacket->nClientID, pPacket->nNumber, pPacket->szStr.c_str() );

	return true;
}

bool CTestClientProcessor::ProcessConnectedGameID( CConnectedGameID *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "connected to game %d\n", pPacket->nGameID ) );
	nGameID = pPacket->nGameID;

	return true;
}

bool CTestClientProcessor::ProcessClientWasKicked( CGameClientWasKicked *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "client %d was kicked from the game\n", pPacket->nKicked ) );

	gameClients.erase( pPacket->nKicked );
	return true;
}

bool CTestClientProcessor::ProcessGameKickClient( CGameKickClient *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	if ( pServerClient )
	{
		if ( nGameID == -1 )
			Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "cannot kick client, not in a game now\n" );
		else
		{
			pPacket->nGameID = nGameID;
			pServerClient->SendPacket( pPacket );

			gameClients.erase( pPacket->nKicked );
		}
	}

	return true;
}

bool CTestClientProcessor::ProcessTestDirectPacket( CTestDirectPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "direct msg from client %d received\n", pPacket->nClientID ) );
	return true;
}

bool CTestClientProcessor::ProcessSendSpecGameInfo( CTestSpecGameInfo *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	if ( nGameID == -1 )
		WriteMSG( "not in a game now\n" );
	else
		pServerClient->SendPacket( new CSpecificGameInfo( 0, nGameID, pPacket ) );

	return true;
}

bool CTestClientProcessor::ProcessReceiveSpecificGameInfo( CSpecificGameInfo *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	if ( pPacket->pInfo == 0 )
		WriteMSG( "null specific game info received\n" );
	else
	{
		CTestSpecGameInfo *pTestInfo = dynamic_cast_ptr<CTestSpecGameInfo*>( pPacket->pInfo );
		if ( pTestInfo == 0 )
			WriteMSG( "unknown specific game info received\n" );
		else
			WriteMSG( "test specific game info recieved: map name \"%s\"\n", pTestInfo->szMapName.c_str() );
	}

	return true;
}

bool CTestClientProcessor::ProcessShowGameClients( CTestShowGameClients *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	if ( nGameID == -1 )
		WriteMSG( "not in a game now" );
	else
	{
		string szStr = StrFmt( "game %d clients: %d", nGameID, nMyID );
		for ( hash_set<int>::iterator iter = gameClients.begin(); iter != gameClients.end(); ++iter )
			szStr += StrFmt( ", %d", *iter );
		szStr += "\n";

		WriteMSG( szStr.c_str() );
	}

	return true;
}

list<int> ladderGameTeam1;
list<int> ladderGameTeam2;

bool CTestClientProcessor::ProcessLadderInvitePacket( CLadderInvitePacket *pPacket )
{
	ladderGameTeam1 = pPacket->team1;
	ladderGameTeam2 = pPacket->team2;
	string szText = "";
	WriteMSG( "Starting ladder game..\n" );
	szText = "Team 1 : ";
	for ( list<int>::iterator it = ladderGameTeam1.begin(); it != ladderGameTeam1.end(); ++it )
	{
		szText += StrFmt( "%d ", *it );
	}
	szText += "\n";
	WriteMSG( szText.c_str() );
	szText = "Team 2 : ";
	for ( list<int>::iterator it = ladderGameTeam2.begin(); it != ladderGameTeam2.end(); ++it )
	{
		szText += StrFmt( "%d ", *it );
	}
	szText += "\n";
	WriteMSG( szText.c_str() );

	CPtr<CNetPacket> pDelete = pPacket;
	if ( pServerClient )
		pServerClient->SendPacket( new CConnectGamePacket( 0, pPacket->nGameID, "" ) );

	return true;
}

bool CTestClientProcessor::ProcessLadderStatisticsPacket( CLadderStatisticsPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;

//	string szOut = StrFmt( "Player %s ladder statistics:\n", pPacket->szNick );
//	szOut += StrFmt( "  Level = %d,\n  XP = %d ( CurrLevel = %d, NextLevel = %d ),\n  Solo wins/loses = %d/%d,\n  Team wins/loses = %d/%d,\n",
//		pPacket->info.nLevel, pPacket->info.nXP, pPacket->info.nLevelXP, pPacket->info.nNextLevelXP,
//		pPacket->info.nWinsSolo, pPacket->info.nLosesSolo,
//		pPacket->info.nWinsTeam, pPacket->info.nLosesTeam );
//	szOut += " Race wins:\n";
//	for ( hash_map<int,int>::iterator it = pPacket->info.raceWins.begin(); it != pPacket->info.raceWins.end(); ++it )
//	{
//		szOut += StrFmt( "  Race %d: %d\n", it->first, it->second );
//	}
//	szOut += " Race loses:\n";
//	for ( hash_map<int,int>::iterator it = pPacket->info.raceLoses.begin(); it != pPacket->info.raceLoses.end(); ++it )
//	{
//		szOut += StrFmt( "  Race %d: %d\n", it->first, it->second );
//	}
//	szOut += StrFmt( "  MaxXPEarned: %d\n", pPacket->info.nMaxXPEarned );
//	szOut += StrFmt( "  MaxXPLost: %d\n", pPacket->info.nMaxXPLost );
//	szOut += StrFmt( "  FavoriteUnit: %d\n", pPacket->info.nFavoriteUnit );
//	WriteMSG( szOut.c_str() );

	return true;
}

bool CTestClientProcessor::ProcessForgottenPasswordAnswerPacket( CForgottenPasswordAnswerPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	switch( pPacket->eAnswer )
	{
	case CForgottenPasswordAnswerPacket::NICK_NOT_REGISTERED:
		WriteMSG( "Nick is not registered \n" );
		break;
	case CForgottenPasswordAnswerPacket::INVALID_EMAIL:
		WriteMSG( "Invalid email address\n" );
		break;
	case CForgottenPasswordAnswerPacket::OK:
		WriteMSG( "Password was sent to you via email.\n" );
		break;
	case CForgottenPasswordAnswerPacket::SORRY_SERVICE_IS_NOT_IMPLEMENTED_YET:
		WriteMSG( "The service is not available for this milestone.\n" );
		break;
	}
	return true;
}

bool CTestClientProcessor::ProcessPingPacket( CPingPacket *pPacket )
{
	WriteMSG( "Ping received.\n" );
	return true;
}

bool CTestClientProcessor::Segment()
{
	if ( nGameID != -1 )
	{
		const DWORD dwCurTime = GetTickCount();
		if ( dwCurTime - dwLastGameUpdate > dwHeartBeatPeriod )
		{
			CNetPacket *pPacket = new CGameHeartBeatPacket( 0, nGameID );
			pServerClient->SendPacket( pPacket );

			dwLastGameUpdate = dwCurTime;
		}
	}

	return true;
}


