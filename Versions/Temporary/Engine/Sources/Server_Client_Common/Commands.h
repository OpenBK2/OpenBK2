#pragma once

#include "CommandsInterface.h"

enum EServerClientCommands
{
	ESC_NONE,

	// client
	ESC_REGISTER,
	ESC_LOGIN,

	ESC_ENTER_LOBBY,
	ESC_LEAVE_LOBBY,

	ESC_LADDER_TEST,
	ESC_LADDER_WIN,
	ESC_LADDER_INFO,

	ESC_CHAT_LOBBY,
	ESC_CHAT_PRIVATE,
	ESC_AFK,
	ESC_CHAT_JOIN,
	ESC_CHAT_CHANNELS,
	ESC_CHAT_IGNORE,
	ESC_CHAT_WHERE,
	ESC_CHAT_FRIEND,

	ESC_SET_CLIENT_STATE,
	ESC_GET_LOBBY_CLIENTS,

	ESC_CREATE_GAME,
	ESC_KILL_GAME,
	ESC_SEND_GAME_INFO,
	ESC_GET_LOBBY_GAMES,
	ESC_LEAVE_GAME,
	ESC_CONNECT_GAME,
	ESC_GAME_BROADCAST,
	ESC_GAME_DIRECT,
	ESC_GAME_KICK_CLIENT,
	ESC_DIRECT_MSG,
	ESC_GAME_CLIENTS,
	ESC_SPEC_GAME_INFO,
	ESC_PAUSE_SERVER_CONN,
	ESC_PAUSE_ACCEPT,
	ESC_PAUSE_CONNECT,
	ESC_PAUSE_CLIENT,

	ESC_FORGOT_PASSWORD,
	ESC_MULTI_TEST,
	ESC_PING,
	// server
	ESC_CLIENTS,
	ESC_CLIENT_STATE,
	ESC_KICK,
	ESC_GAMES,
	
	ESC_SHOW_STATISTICS,

	ESC_RELOAD_CONFIG,
	ESC_BROADCAST,

	ESC_SHOW_LOBBIES,
	ESC_HELP,
};

class CCommands : public CCommandsBase
{
	OBJECT_NOCOPY_METHODS( CCommands );

	typedef void (CCommands::*PARSE_CMD_FUNC)( vector<string> &szWords, SCommand *pCmd );
	hash_map<int, PARSE_CMD_FUNC> parseCmdFuncs;
	bool bServer;

	//
	const EServerClientCommands FindCmd( const string &szCommand );

	// client
	void ParseLoginServer( vector<string> &szWords, SCommand *pCmd );
	void ParseRegisterServer( vector<string> &szWords, SCommand *pCmd );
	void ParseEnterLobby( vector<string> &szWords, SCommand *pCmd );
	void ParseLeaveLobby( vector<string> &szWords, SCommand *pCmd );
	void ParseSetClientState( vector<string> &szWords, SCommand *pCmd );
	void ParseClientGetLobbyClients( vector<string> &szWords, SCommand *pCmd );
	void ParseCreateGame( vector<string> &szWords, SCommand *pCmd );
	void ParseKillGame( vector<string> &szWords, SCommand *pCmd );
	void ParseSendGameInfo( vector<string> &szWords, SCommand *pCmd );
	void ParseGetLobbyGames( vector<string> &szWords, SCommand *pCmd );
	void ParseLeaveGame( vector<string> &szWords, SCommand *pCmd );
	void ParseConnectGame( vector<string> &szWords, SCommand *pCmd );
	void ParseGameBroadcast( vector<string> &szWords, SCommand *pCmd );
	void ParseGameDirect( vector<string> &szWords, SCommand *pCmd );
	void ParseGameKickClient( vector<string> &szWords, SCommand *pCmd );
	void ParseDirectMsg( vector<string> &szWords, SCommand *pCmd );
	void ParseShowGameClients( vector<string> &szWords, SCommand *pCmd );
	void ParseSpecGameInfo( vector<string> &szWords, SCommand *pCmd );
	void ParsePauseServerConn( vector<string> &szWords, SCommand *pCmd );
	void ParsePauseAccept( vector<string> &szWords, SCommand *pCmd );
	void ParsePauseConnect( vector<string> &szWords, SCommand *pCmd );
	void ParsePauseClient( vector<string> &szWords, SCommand *pCmd );

	void ParseChatLobby( vector<string> &szWords, SCommand *pCmd );
	void ParseChatPrivate( vector<string> &szWords, SCommand *pCmd );
	void ParseAFK( vector<string> &szWords, SCommand *pCmd );
	void ParseChatJoin( vector<string> &szWords, SCommand *pCmd );
	void ParseChatChannels( vector<string> &szWords, SCommand *pCmd );
	void ParseChatIgnore( vector<string> &szWords, SCommand *pCmd );
	void ParseChatFriend( vector<string> &szWords, SCommand *pCmd );
	void ParseChatWhere( vector<string> &szWords, SCommand *pCmd ); 

	void ParseLadderTest( vector<string> &szWords, SCommand *pCmd );
	void ParseLadderWin( vector<string> &szWords, SCommand *pCmd );
	void ParseLadderInfo( vector<string> &szWords, SCommand *pCmd );

	void ParseForgotPassword( vector<string> &szWords, SCommand *pCmd );
	void ParsePing( vector<string> &szWords, SCommand *pCmd );
	void ParseMultiTest( vector<string> &szWords, SCommand *pCmd );
	// server
	void ParseClientsList( vector<string> &szWords, SCommand *pCmd );
	void ParseClientState( vector<string> &szWords, SCommand *pCmd );
	void ParseKick( vector<string> &szWords, SCommand *pCmd );
	void ParseGames( vector<string> &szWords, SCommand *pCmd );
	void ParseReloadConfig( vector<string> &szWords, SCommand *pCmd );
	void ParseShowStatistics( vector<string> &szWords, SCommand *pCmd );
	void ParseBroadcast( vector<string> &szWords, SCommand *pCmd );

	void ParseShowLobbies( vector<string> &szWords, SCommand *pCmd );
	void ParseHelp( vector<string> &szWords, SCommand *pCmd );

	string szErrorMsg;
	void SetErrorMsg( const string &_szMsg ) { szErrorMsg = _szMsg; }
	const string& GetErrorMsg() const { return szErrorMsg; }

public:
	CCommands() : bServer( false ) { }
	CCommands( const bool bServer );

	virtual void GetStringCommands( vector<string> *pCommands );
	virtual bool LineEntered( const string &szLine, string *pszErr );
};

