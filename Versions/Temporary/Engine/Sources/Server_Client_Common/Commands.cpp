#include "stdafx.h"

#include "Commands.h"
#include "CommonClientState.h"
#include "LobbiesIDs.h"
#include "Misc/StrProc.h"

struct SStringToCommand
{
	const char *pszCommand;
	EServerClientCommands eCmd;
	bool bServerCommand;
	const char *pszHelp;
};

static SStringToCommand szCommandsList[] = 
{
	// client
	{ "register", ESC_REGISTER, false, "usage: <name> <password> <CDKey> <email>" },
	{ "login", ESC_LOGIN, false, "usage: <name> <password>" },

	{ "enter_lobby", ESC_ENTER_LOBBY, false, "usage: <lobby name (custom|ladder)>" },
	{ "leave_lobby", ESC_LEAVE_LOBBY, false, "usage: no arguments" },

	{ "ladder_test", ESC_LADDER_TEST, false, "usage: no arguments" },
	{ "ladder_win", ESC_LADDER_WIN, false, "usage: <gameID> <winner1ID>, <winner2ID>, .." },
	{ "ladder_info", ESC_LADDER_INFO, false, "usage: <nick>" },

	{ "set_state", ESC_SET_CLIENT_STATE, false, "set (away|ingame|online) state, usage: <state name>" },
	{ "clients", ESC_GET_LOBBY_CLIENTS, false, "usage: no arguments" },

	{ "create_game", ESC_CREATE_GAME, false, "usage: no arguments" },
	{ "kill_game", ESC_KILL_GAME, false, "kill the game, usage: no arguments(kill currently playing) or nGameID" },
	{ "send_game_info", ESC_SEND_GAME_INFO, false, "usage: <game name> <max number of players> <password> <1|0 (can connect to this game or not)>" },
	{ "games", ESC_GET_LOBBY_GAMES, false, "get list of games in the lobby, usage: no arguments" },
	{ "leave_game", ESC_LEAVE_GAME, false, "leave currently playing game, usage: no arguments" },
	{ "connect_game", ESC_CONNECT_GAME, false, "usage: <game id> [<password>]" },
	{ "game_broadcast", ESC_GAME_BROADCAST, false, "usage: [<number> [<string>]]" },
	{ "game_direct", ESC_GAME_DIRECT, false, "usage: <client id> [<number> [<string>]]" },
	{ "kick", ESC_GAME_KICK_CLIENT, false, "usage: <game client id>" },
	{ "direct_msg", ESC_DIRECT_MSG, false, "send direct msg to client, usage: <client id>" },
	{ "game_clients", ESC_GAME_CLIENTS, false, "shows clients in the currently playing game, usage: <no arguments>" },
	{ "update_specific_game_info", ESC_SPEC_GAME_INFO, false, "send specific internal game info, usage: <map name> " },
	{ "pause_server_connection", ESC_PAUSE_SERVER_CONN, false, "pause connection to server, usage: <1|0 (pause or not)>" },
	{ "pause_accept_gamers_net", ESC_PAUSE_ACCEPT, false, "pause accept players, usage: <1|0 (pause or not)>" },
	{ "pause_connect_gamers_net", ESC_PAUSE_CONNECT, false, "pause connect to player, usage: <client id> <1|0 (pause or not)>" },
	{ "pause_client_module", ESC_PAUSE_CLIENT, false, "pause whole client modle, usage: <1|0 (pause or not)>" },
	
	{ "chat_bc", ESC_CHAT_LOBBY, false, "send chat channel broadcast message , usage: <chat message>" },
	{ "chat_private", ESC_CHAT_PRIVATE, false, "usage: <chat message> <user nick>" },
	{ "afk", ESC_AFK, false, "away from keyboard/in-game, usage: no arguments" },
	{ "chat_join", ESC_CHAT_JOIN, false, "join chat channel, usage: <channel name>"  },
	{ "chat_ignore", ESC_CHAT_IGNORE, false, "add/remove user to/from ignorelist, usage: <nick> <1/0>" },
	{ "chat_friend", ESC_CHAT_FRIEND, false, "add/remove user to/from friendlist, usage: <nick> <1/0>" }, 
	{ "chat_channels", ESC_CHAT_CHANNELS, false, "list chat channels, usage: no arguments" },
	{ "chat_where", ESC_CHAT_WHERE, false, "name of user's channel, usage: <user nick>" },

	{ "multi_test", ESC_MULTI_TEST, false, "stress test for server, usage: <number of testers> <shift>" },
	{ "ping", ESC_PING, false, "pings server, usage: no arguments" },
	{ "forgot_password", ESC_FORGOT_PASSWORD, false, "request for forgotten password retrieval, usage: <nick> <email>" },
	// server
	{ "clients", ESC_CLIENTS, true, "clients list, usage: [<lobby name>]" },
	{ "client_state", ESC_CLIENT_STATE, true, "usage: <client nick>" },
	{ "kick", ESC_KICK, true, "kick client, usage: <client nick>" },
	{ "games", ESC_GAMES, true, "get list of games in the lobby, usage: <lobby name>" },
	{ "reload_config", ESC_RELOAD_CONFIG, true, "reloads configuration files for all lobbies, usage: no arguments" },
	{ "statistics", ESC_SHOW_STATISTICS, true, "displays server statistics, usage: no arguments" },
	{ "broadcast", ESC_BROADCAST, true, "sends broadcast message, usage: filename" },

	{ "lobbies", ESC_SHOW_LOBBIES, true, "show list of lobbies, usage: no arguments" },
	{ "lobbies", ESC_SHOW_LOBBIES, false, "show list of lobbies, usage: no arguments" },
	{ "help", ESC_HELP, true, "" },
	{ "help", ESC_HELP, false, "" },

	{ 0, ESC_NONE },
};

#define REGISTER_PARSE( cmd, FuncName, _bServer )	\
if ( _bServer == bServer )	\
parseCmdFuncs[cmd] = &CCommands::##FuncName;

CCommands::CCommands( const bool _bServer )
: bServer( _bServer )
{
	// client
	REGISTER_PARSE( ESC_REGISTER, ParseRegisterServer, false );
	REGISTER_PARSE( ESC_LOGIN, ParseLoginServer, false );
	REGISTER_PARSE( ESC_ENTER_LOBBY, ParseEnterLobby, false );
	REGISTER_PARSE( ESC_LEAVE_LOBBY, ParseLeaveLobby, false );
	REGISTER_PARSE( ESC_SET_CLIENT_STATE, ParseSetClientState, false );
	REGISTER_PARSE( ESC_GET_LOBBY_CLIENTS, ParseClientGetLobbyClients, false );
	REGISTER_PARSE( ESC_CREATE_GAME, ParseCreateGame, false );
	REGISTER_PARSE( ESC_KILL_GAME, ParseKillGame, false );
	REGISTER_PARSE( ESC_SEND_GAME_INFO, ParseSendGameInfo, false );
	REGISTER_PARSE( ESC_GET_LOBBY_GAMES, ParseGetLobbyGames, false );
	REGISTER_PARSE( ESC_LEAVE_GAME, ParseLeaveGame, false );
	REGISTER_PARSE( ESC_HELP, ParseHelp, false );
	REGISTER_PARSE( ESC_CONNECT_GAME, ParseConnectGame, false );
	REGISTER_PARSE( ESC_GAME_BROADCAST, ParseGameBroadcast, false );
	REGISTER_PARSE( ESC_GAME_DIRECT, ParseGameDirect, false );
	REGISTER_PARSE( ESC_GAME_KICK_CLIENT, ParseGameKickClient, false );
	REGISTER_PARSE( ESC_DIRECT_MSG, ParseDirectMsg, false );
	REGISTER_PARSE( ESC_SHOW_LOBBIES, ParseShowLobbies, false );
	REGISTER_PARSE( ESC_GAME_CLIENTS, ParseShowGameClients, false );
	REGISTER_PARSE( ESC_SPEC_GAME_INFO, ParseSpecGameInfo, false );
	REGISTER_PARSE( ESC_PAUSE_SERVER_CONN, ParsePauseServerConn, false );
	REGISTER_PARSE( ESC_PAUSE_ACCEPT, ParsePauseAccept, false );
	REGISTER_PARSE( ESC_PAUSE_CONNECT, ParsePauseConnect, false );
	REGISTER_PARSE( ESC_PAUSE_CLIENT, ParsePauseClient, false );

	REGISTER_PARSE( ESC_AFK, ParseAFK, false );
	REGISTER_PARSE( ESC_CHAT_LOBBY, ParseChatLobby, false );
	REGISTER_PARSE( ESC_CHAT_PRIVATE, ParseChatPrivate, false );
	REGISTER_PARSE( ESC_CHAT_JOIN, ParseChatJoin, false );
	REGISTER_PARSE( ESC_CHAT_CHANNELS, ParseChatChannels, false );
	REGISTER_PARSE( ESC_CHAT_IGNORE, ParseChatIgnore, false );
	REGISTER_PARSE( ESC_CHAT_FRIEND, ParseChatFriend, false );
	REGISTER_PARSE( ESC_CHAT_WHERE, ParseChatWhere, false );

	REGISTER_PARSE( ESC_LADDER_TEST, ParseLadderTest, false );
	REGISTER_PARSE( ESC_LADDER_WIN, ParseLadderWin, false );
	REGISTER_PARSE( ESC_LADDER_INFO, ParseLadderInfo, false );

	REGISTER_PARSE( ESC_MULTI_TEST, ParseMultiTest, false );
	REGISTER_PARSE( ESC_PING, ParsePing, false );

	REGISTER_PARSE( ESC_FORGOT_PASSWORD, ParseForgotPassword, false ); 
	// server
	REGISTER_PARSE( ESC_CLIENTS, ParseClientsList, true );
	REGISTER_PARSE( ESC_CLIENT_STATE, ParseClientState, true );
	REGISTER_PARSE( ESC_KICK, ParseKick, true );
	REGISTER_PARSE( ESC_GAMES, ParseGames, true );
	REGISTER_PARSE( ESC_HELP, ParseHelp, true );
	REGISTER_PARSE( ESC_SHOW_LOBBIES, ParseShowLobbies, true );
	REGISTER_PARSE( ESC_SHOW_STATISTICS, ParseShowStatistics, true );
	REGISTER_PARSE( ESC_RELOAD_CONFIG, ParseReloadConfig, true );
	REGISTER_PARSE( ESC_BROADCAST, ParseBroadcast, true );
}

void CCommands::GetStringCommands( std::vector<std::string> *pCommands )
{
	pCommands->clear();

	int i = 0;
	while ( szCommandsList[i].pszCommand != 0 )
	{
		if ( szCommandsList[i].bServerCommand == bServer )
			pCommands->push_back( szCommandsList[i].pszCommand );

		++i;
	}

	sort( pCommands->begin(), pCommands->end() );
}

const EServerClientCommands CCommands::FindCmd( const std::string &szCommand )
{
	int i = 0;
	while ( szCommandsList[i].pszCommand != 0 )
	{
		if ( szCommand == szCommandsList[i].pszCommand && szCommandsList[i].bServerCommand == bServer )
			return szCommandsList[i].eCmd;

		++i;
	}

	return ESC_NONE;
}

void CCommands::ParseLoginServer( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 3 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );
	pCmd->params.push_back( szWords[2] );
}

void CCommands::ParseRegisterServer( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 5 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );
	pCmd->params.push_back( szWords[2] );
	pCmd->params.push_back( szWords[3] );
	pCmd->params.push_back( szWords[4] );
}

void CCommands::ParseEnterLobby( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	NStr::ToLower( &szWords[1] );
	static std::string szOut;
	if ( szWords[1] != "custom" && szWords[1] != "ladder" )
	{
		szOut = StrFmt( "Unknown lobby %s\n", szWords[1].c_str() );
		{ SetErrorMsg( szOut ); return; }
	}

	pCmd->params.push_back( szWords[1] );
}

void CCommands::ParseLeaveLobby( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 1 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }
}

void CCommands::ParseChatLobby( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() < 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	for ( int i = 2; i < szWords.size(); ++i )
		szWords[1] += " " + szWords[i];

	pCmd->params.push_back( szWords[1] );
}

void CCommands::ParseChatPrivate( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() < 3 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	for ( int i = 2; i < szWords.size() - 1; ++i )
		szWords[1] += " " + szWords[i];

	pCmd->params.push_back( szWords[1] );
	pCmd->params.push_back( szWords[szWords.size() - 1] );
}

void CCommands::ParseAFK( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 1 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }
}

void CCommands::ParseChatJoin( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );
}

void CCommands::ParseChatChannels( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 1 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }
}

void CCommands::ParseChatIgnore( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 3 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	static std::string szOut;
	if ( szWords[2] != "1" && szWords[2] != "0" )
	{
		szOut = StrFmt( "Use 0 or 1 instead of %s\n", szWords[2].c_str() );
		{ SetErrorMsg( szOut ); return; }
	}

	pCmd->params.push_back( szWords[1] );
	pCmd->params.push_back( szWords[2] );
}

void CCommands::ParseChatFriend( std::vector<std::string> &szWords, SCommand *pCmd )
{
	ParseChatIgnore( szWords, pCmd );
}

void CCommands::ParseChatWhere( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );
}

void CCommands::ParseSetClientState( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	NStr::ToLower( &szWords[1] );

	static std::string szOut;
	if ( szWords[1] != "online" && szWords[1] != "away" && szWords[1] != "ingame" )
	{
		szOut = StrFmt( "Unknown state %s\n", szWords[1].c_str() );
		{ SetErrorMsg( szOut ); return; }
	}

	ECommonClientState eState = ES_ONLINE;
	if ( szWords[1] == "online" )
		eState = ES_ONLINE;
	if ( szWords[1] == "away" )
		eState = ES_AWAY;
	if ( szWords[1] == "ingame" )
		eState = ES_INGAME;

	pCmd->params.push_back( StrFmt( "%d", (int)eState ) );
}

void CCommands::ParseClientsList( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 1 && szWords.size() != 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	if ( szWords.size() == 2 )
	{
		NStr::ToLower( &szWords[1] );
		static std::string szOut;
		if ( szWords[1] != "custom" )
		{
			szOut = StrFmt( "unknown lobby %s\n", szWords[1].c_str() );
			{ SetErrorMsg( szOut ); return; }
		}

		pCmd->params.push_back( StrFmt( "%d", (int)ERID_CUSTOM ) );
	}
}

void CCommands::ParseClientState( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );
}

void CCommands::ParseKick( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );
}

void CCommands::ParseReloadConfig( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 1 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }
}

void CCommands::ParseShowStatistics( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 1 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }
}

void CCommands::ParseClientGetLobbyClients( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 1 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }
}

void CCommands::ParseGames( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	static std::string szOut;
	if ( szWords[1] != "custom" )
	{
		szOut = StrFmt( "Unknown lobby %s", szWords[1].c_str() );
		{ SetErrorMsg( szOut ); return; }
	}
}

void CCommands::ParseCreateGame( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 1 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }
}

void CCommands::ParseKillGame( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() > 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }
	else
	{
		if ( szWords.size() == 2 )
			pCmd->params.push_back( szWords[1] );
	}
}

void CCommands::ParseSendGameInfo( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 5 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );
	pCmd->params.push_back( szWords[2] );
	pCmd->params.push_back( szWords[3] );
	pCmd->params.push_back( szWords[4] );
}

void CCommands::ParseGetLobbyGames( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 1 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }
}

void CCommands::ParseLeaveGame( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 1 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }
}

void CCommands::ParseHelp( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() == 1 )
	{
		static std::string szStr = "available commands:\n";
		int i = 0;
		while ( szCommandsList[i].pszCommand != 0 )
		{
			if ( szCommandsList[i].bServerCommand == bServer )
				szStr += StrFmt( "    %s\n", szCommandsList[i].pszCommand );

			++i;
		}

		SetErrorMsg( szStr.c_str() );
	}
	else
	{
		int i = 0;
		while ( szCommandsList[i].pszCommand != 0 )
		{
			if ( szWords[1] == szCommandsList[i].pszCommand && szCommandsList[i].bServerCommand == bServer )
			{
				static std::string szOut;
				szOut = StrFmt( "%s\n", szCommandsList[i].pszHelp );
				{ SetErrorMsg( szOut ); return; }
			}

			++i;
		}

		static std::string szOut;
		szOut = StrFmt( "command %s not found\n", szWords[1].c_str() );
		{ SetErrorMsg( szOut ); return; }
	}
}

void CCommands::ParseConnectGame( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() < 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );

	std::string szPassword = "";
	if ( szWords.size() > 2 )
	{
		szPassword = szWords[2];
		for ( int i = 3; i < szWords.size(); ++i )
			szPassword += " " + szWords[i];
	}

	pCmd->params.push_back( szPassword );
}

void CCommands::ParseGameBroadcast( std::vector<std::string> &szWords, SCommand *pCmd )
{
	const std::string szNum = szWords.size() > 1 ? szWords[1] : "0";
	pCmd->params.push_back( szNum );

	std::string szStr = szWords.size() > 2 ? szWords[2] : "undefined";
	for ( int i = 3; i < szWords.size(); ++i )
		szStr += " " + szWords[i];

	pCmd->params.push_back( szStr );
}

void CCommands::ParseGameDirect( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() < 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );

	const std::string szNum = szWords.size() > 2 ? szWords[2] : "0";
	pCmd->params.push_back( szNum );

	std::string szStr = szWords.size() > 3 ? szWords[3] : "undefined";
	for ( int i = 4; i < szWords.size(); ++i )
		szStr += " " + szWords[i];

	pCmd->params.push_back( szStr );
}

void CCommands::ParseGameKickClient( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );
}

void CCommands::ParseDirectMsg( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );
}

void CCommands::ParseShowLobbies( std::vector<std::string> &szWords, SCommand *pCmd )
{
	SetErrorMsg( "lobbies: ladder custom\n" );
}

void CCommands::ParseShowGameClients( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 1 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }
}

void CCommands::ParsePauseServerConn( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );
}

void CCommands::ParsePauseAccept( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );
}

void CCommands::ParsePauseConnect( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 3 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );
	pCmd->params.push_back( szWords[2] );
}

void CCommands::ParseSpecGameInfo( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() < 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	std::string szMapName = szWords[1];
	for ( int i = 2; i < szWords.size(); ++i )
		szMapName += szWords[i];

	pCmd->params.push_back( szMapName );
}

void CCommands::ParsePauseClient( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );
}

void CCommands::ParseLadderTest( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 1 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

}

void CCommands::ParseLadderWin( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() < 3 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }
	for ( int i = 1; i < szWords.size(); ++i )
	{
		pCmd->params.push_back( szWords[i] );
	}
}

void CCommands::ParseLadderInfo( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );
}

void CCommands::ParseForgotPassword( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 3 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );
	pCmd->params.push_back( szWords[2] );
}

void CCommands::ParseMultiTest( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 3 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }

	pCmd->params.push_back( szWords[1] );
	pCmd->params.push_back( szWords[2] );
}

void CCommands::ParsePing( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 1 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }
}

void CCommands::ParseBroadcast( std::vector<std::string> &szWords, SCommand *pCmd )
{
	if ( szWords.size() != 2 )
		{ SetErrorMsg( "Wrong number of arguments\n" ); return; }
	pCmd->params.push_back( szWords[1] );
}

bool CCommands::LineEntered( const std::string &szLineEntered, std::string *pszErr )
{
	std::vector<std::string> szWords;
	std::string szLine = szLineEntered;
	PreprocessLine( &szLine, &szWords );

	if ( !szWords.empty() )
	{
		EServerClientCommands eCmd = FindCmd( szWords[0] );
		if ( eCmd == ESC_NONE )
		{
			*pszErr = StrFmt( "Unknown command %s\n", szWords[0].c_str() );
			return false;
		}
		else
		{
			std::unordered_map<int, PARSE_CMD_FUNC>::iterator iter = parseCmdFuncs.find( eCmd );
			NI_ASSERT( iter != parseCmdFuncs.end(), StrFmt( "Can't parse command %s", szWords[0].c_str() ) );

			if ( iter == parseCmdFuncs.end() )
				return false;

			PARSE_CMD_FUNC pfnFunc = iter->second;
			SCommand cmd;
			cmd.nCmd = eCmd;
			(this->*pfnFunc)( szWords, &cmd );
			const std::string &szErrorMsg = GetErrorMsg();
			if ( szErrorMsg.empty() )
			{ 
				PushCommand( cmd );
				return true;
			}
			else
			{
				*pszErr = szErrorMsg;
				SetErrorMsg( "" );
				return false;
			}
		}

	}
	else
		return true;
}


