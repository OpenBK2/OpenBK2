#include "stdafx.h"

#include "InternalPackets.h"
#include "TestClientProcessor.h"
#include "Client/ServerClient.h"
#include "Misc/StrProc.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/CustomLobbyPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/LadderLobbyPackets.h"
#include "Server_Client_Common/LoginPackets.h"
#include "Server_Client_Common/ChatPackets.h"

#include "port/time.h"

#include <cstdint>

#include <fmt/format.h>

CTestClientProcessor::CTestClientProcessor( const std::string &szCfgFile )
{
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessConnectServerResult );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessChatPacket );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessMyIDPacket );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessRemoveClient );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessEnteredLobby );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessGetLobbyClientsPacket );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessCustomLobbyClients );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessCreateGame );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessUpdateGameInfo );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessGetLobbyGamesPacket );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessCustomLobbyGamesPacket );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessKillGame );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessLeaveGame );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessConnectGameFailed );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessGameClientRemoved );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessNewGameClient );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessGameKilled );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessConnectGame );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessGameTestBroadcastMsg );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessGameTestDirectMsg );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessConnectedGameID );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessClientWasKicked );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessGameKickClient );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessTestDirectPacket );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessShowGameClients );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessSendSpecGameInfo );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessReceiveSpecificGameInfo );

	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessChatChannelClientsListPacket );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessChatClientListChangeNotifyPacket );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessChatChannelsListPacket );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessChatIgnoreListPacket );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessChatChannelByNickPacket );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessChatFriendNotifyPacket );

	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessLadderInvitePacket );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessLadderStatisticsPacket );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessForgottenPasswordAnswerPacket );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessWelcomePacket );
	REGISTER_PACKET_PROCESSOR( &CTestClientProcessor::ProcessPingPacket );

	CFileStream stream( szCfgFile, CFileStream::WIN_READ_ONLY );
	CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
	pSaver->Add( "GameHeartBeatPeriod", &dwHeartBeatPeriod );
	pSaver = 0;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "Games heartbeat period is {}\n", dwHeartBeatPeriod ).c_str() );
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

	std::string szMessage;
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
			fmt::format( "lobby chat message from {}: {}\n", pPacket->szNick, NStr::ToMBCS( pPacket->wszMessage ) ).c_str() );
	}
	else
	{
		Singleton<IConsoleBuffer>()->WriteASCII(
			CONSOLE_STREAM_CONSOLE,
			fmt::format( "private chat message from {}: {}\n", pPacket->szNick, NStr::ToMBCS( pPacket->wszMessage ) ).c_str() );
	}

	return true;
	
}

bool CTestClientProcessor::ProcessChatChannelClientsListPacket( CChatChannelClientsListPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "Channel clients:\n" );
	for ( std::list<SIDNickPair>::iterator it = pPacket->clientsList.begin(); it != pPacket->clientsList.end(); ++it )
	{
		const SIDNickPair &listPair = *it;
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "{}\n", listPair.szNick ).c_str() );
	}
	return true;
}

bool CTestClientProcessor::ProcessChatClientListChangeNotifyPacket( CChatClientListChangeNotifyPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	if ( pPacket->bJoined )
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "{} joined channel\n", pPacket->szNick ).c_str() );
	}
	else
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "{} leaved channel\n", pPacket->szNick ).c_str() );
	}
	return true;
}

bool CTestClientProcessor::ProcessChatChannelsListPacket( CChatChannelsListPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	// Please, process the differences list correctly in your own code.. I'm so lazy..
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "Channel(s):\n" );
	for ( std::list<std::string>::iterator it = pPacket->added.begin(); it != pPacket->added.end(); ++it )
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "{}\n", *it ).c_str() );
	}
	return true;
}

bool CTestClientProcessor::ProcessChatIgnoreListPacket( CChatIgnoreFriendListPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "Your ignore list:\n" );
	if ( !pPacket->ignoreList.empty() )
	{
		for ( std::list<std::string>::iterator it = pPacket->ignoreList.begin(); it != pPacket->ignoreList.end(); ++it )
		{
			Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "{}\n", *it ).c_str() );
		}
	}
	else
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, ".. is empty\n" );
	}

	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "Your friend list:\n" );
	if ( !pPacket->friendList.empty() )
	{
		for ( std::list<std::string>::iterator it = pPacket->friendList.begin(); it != pPacket->friendList.end(); ++it )
		{
			Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "{}\n", *it ).c_str() );
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
		std::string szOutString = fmt::format( "User {} is in ", pPacket->szNick ) + pPacket->szChannel + ".\n";
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, szOutString.c_str() );
	}
	else
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, 
			fmt::format( "User {} is not in chat.\n", pPacket->szNick ).c_str() );
	return true;
}

bool CTestClientProcessor::ProcessChatFriendNotifyPacket( CChatFriendNotifyPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	std::string szStatus;
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
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "Notification from friend. {} is now {}.\n", 
		pPacket->szNick, szStatus ).c_str() );
	return true;
}

bool CTestClientProcessor::ProcessMyIDPacket( CMyIDPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;

	nMyID = pPacket->nMyID;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "my id is {}\n", nMyID ).c_str() );
	return true;
}

bool CTestClientProcessor::ProcessWelcomePacket( CSystemBroadcastPacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "{}\n", NStr::ToMBCS( pPacket->wszText ) ).c_str() );
	return true;
}

bool CTestClientProcessor::ProcessRemoveClient( CNetRemoveClient *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;

	NI_ASSERT( pPacket->nClientID == 0, fmt::format( "wrong client {} removed, 0 expected\n", pPacket->nClientID ) );
	if ( pPacket->nClientID == 0 )
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "connection with server was lost\n" ).c_str() );

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

	for ( std::list<int>::iterator iter = pPacket->removed.begin(); iter != pPacket->removed.end(); ++iter )
		lobbyClients.erase( *iter );

	if ( pPacket->added.empty() && pPacket->changed.empty() && lobbyClients.empty() )
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "no clients except me\n" );
	else
	{
		for ( std::list<SCustomLobbyClientInfo>::iterator iter = pPacket->added.begin(); iter != pPacket->added.end(); ++iter )
		{
			SCustomLobbyClientInfo &info = *iter;
			lobbyClients[info.nID] = info;
		}

		for ( std::list<SCustomLobbyClientInfo>::iterator iter = pPacket->changed.begin(); iter != pPacket->changed.end(); ++iter )
		{
			SCustomLobbyClientInfo &info = *iter;
			lobbyClients[info.nID] = info;
		}

		std::string szStr = fmt::format( "lobby clients received, version {}", dwClientsVersion );
		if ( pPacket->bFullUpdate )
			szStr += ", fullupdate";
		szStr += ":\n";

		for ( std::unordered_map<int, SCustomLobbyClientInfo>::iterator iter = lobbyClients.begin(); iter != lobbyClients.end(); ++iter )
		{
			SCustomLobbyClientInfo &info = iter->second;
			szStr += fmt::format( "    client {}, ", info.szNick );

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
				szStr += fmt::format( "unknown state {}", static_cast<int>( info.eState ) );
			}

			if ( info.nGameID == -1 )
				szStr += ", not in a game";
			else
				szStr += fmt::format( ", in game {}", info.nGameID );

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
		dwLastGameUpdate = GetCurrentTimeMilliseconds();
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

		dwLastGameUpdate = GetCurrentTimeMilliseconds();
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

	for ( std::list<int>::iterator iter = pPacket->removed.begin(); iter != pPacket->removed.end(); ++iter )
		lobbyGames.erase( *iter );

	if ( pPacket->added.empty() && pPacket->changed.empty() && lobbyGames.empty() )
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "no games in the lobby\n" );
	else
	{
		for ( std::list<SGameInfo>::iterator iter = pPacket->added.begin(); iter != pPacket->added.end(); ++iter )
		{
			SGameInfo &info = *iter;
			lobbyGames[info.nID] = info;
		}

		for ( std::list<SGameInfo>::iterator iter = pPacket->changed.begin(); iter != pPacket->changed.end(); ++iter )
		{
			SGameInfo &info = *iter;
			lobbyGames[info.nID] = info;
		}

		std::string szStr = fmt::format( "lobby games received, version {}", dwGamesVersion );
		if ( pPacket->bFullUpdate )
			szStr += ", fullupdate";
		szStr += ":\n";

		for ( std::unordered_map<int, SGameInfo>::iterator iter = lobbyGames.begin(); iter != lobbyGames.end(); ++iter )
		{
			SGameInfo &info = iter->second;
			szStr += fmt::format( "    \"{}\", id {}, max players {}", info.szName, info.nID, info.nMaxPlayers );
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

	std::string szStr = "connection to game failed";

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
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "connection with game client {} lost\n", pPacket->nClientID ).c_str() );

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
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "game {} killed\n", pPacket->nGame ).c_str() );

	gameClients.clear();
	nGameID = -1;

	return true;
}

bool CTestClientProcessor::ProcessConnectGame( CConnectGamePacket *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
/*
	if ( lobbyGames.find( pPacket->nGameID ) == lobbyGames.end() )
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "cant find game {}\n", pPacket->nGameID ).c_str() );
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
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "connected to game {}\n", pPacket->nGameID ).c_str() );
	nGameID = pPacket->nGameID;

	return true;
}

bool CTestClientProcessor::ProcessClientWasKicked( CGameClientWasKicked *pPacket )
{
	CPtr<CNetPacket> pDelete = pPacket;
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "client {} was kicked from the game\n", pPacket->nKicked ).c_str() );

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
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, fmt::format( "direct msg from client {} received\n", pPacket->nClientID ).c_str() );
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
		std::string szStr = fmt::format( "game {} clients: {}", nGameID, nMyID );
		for ( std::unordered_set<int>::iterator iter = gameClients.begin(); iter != gameClients.end(); ++iter )
			szStr += fmt::format( ", {}", *iter );
		szStr += "\n";

		WriteMSG( szStr.c_str() );
	}

	return true;
}

std::list<int> ladderGameTeam1;
std::list<int> ladderGameTeam2;

bool CTestClientProcessor::ProcessLadderInvitePacket( CLadderInvitePacket *pPacket )
{
	ladderGameTeam1 = pPacket->team1;
	ladderGameTeam2 = pPacket->team2;
	std::string szText = "";
	WriteMSG( "Starting ladder game..\n" );
	szText = "Team 1 : ";
	for ( std::list<int>::iterator it = ladderGameTeam1.begin(); it != ladderGameTeam1.end(); ++it )
	{
		szText += fmt::format( "{} ", *it );
	}
	szText += "\n";
	WriteMSG( szText.c_str() );
	szText = "Team 2 : ";
	for ( std::list<int>::iterator it = ladderGameTeam2.begin(); it != ladderGameTeam2.end(); ++it )
	{
		szText += fmt::format( "{} ", *it );
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

//	string szOut = fmt::format( "Player {} ladder statistics:\n", pPacket->szNick );
//	szOut += fmt::format( "  Level = {},\n  XP = {} ( CurrLevel = {}, NextLevel = {} ),\n  Solo wins/loses = {}/{},\n  Team wins/loses = {}/{},\n",
//		pPacket->info.nLevel, pPacket->info.nXP, pPacket->info.nLevelXP, pPacket->info.nNextLevelXP,
//		pPacket->info.nWinsSolo, pPacket->info.nLosesSolo,
//		pPacket->info.nWinsTeam, pPacket->info.nLosesTeam );
//	szOut += " Race wins:\n";
//	for ( hash_map<int,int>::iterator it = pPacket->info.raceWins.begin(); it != pPacket->info.raceWins.end(); ++it )
//	{
//		szOut += fmt::format( "  Race {}: {}\n", it->first, it->second );
//	}
//	szOut += " Race loses:\n";
//	for ( hash_map<int,int>::iterator it = pPacket->info.raceLoses.begin(); it != pPacket->info.raceLoses.end(); ++it )
//	{
//		szOut += fmt::format( "  Race {}: {}\n", it->first, it->second );
//	}
//	szOut += fmt::format( "  MaxXPEarned: {}\n", pPacket->info.nMaxXPEarned );
//	szOut += fmt::format( "  MaxXPLost: {}\n", pPacket->info.nMaxXPLost );
//	szOut += fmt::format( "  FavoriteUnit: {}\n", pPacket->info.nFavoriteUnit );
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
		const uint32_t dwCurTime = GetCurrentTimeMilliseconds();
		if ( dwCurTime - dwLastGameUpdate > dwHeartBeatPeriod )
		{
			CNetPacket *pPacket = new CGameHeartBeatPacket( 0, nGameID );
			pServerClient->SendPacket( pPacket );

			dwLastGameUpdate = dwCurTime;
		}
	}

	return true;
}


