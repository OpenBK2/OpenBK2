#pragma once

#include "CommandsInterface.h"

#include "Server_Client_Common_export.h"

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

class SERVER_CLIENT_COMMON_EXPORT CCommands : public CCommandsBase
{
	OBJECT_NOCOPY_METHODS( CCommands );

	typedef void (CCommands::*PARSE_CMD_FUNC)( std::vector<std::string> &szWords, SCommand *pCmd );
	std::unordered_map<int, PARSE_CMD_FUNC> parseCmdFuncs;
	bool bServer;

	//
	const EServerClientCommands FindCmd( const std::string &szCommand );

	// client
	void ParseLoginServer( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseRegisterServer( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseEnterLobby( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseLeaveLobby( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseSetClientState( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseClientGetLobbyClients( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseCreateGame( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseKillGame( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseSendGameInfo( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseGetLobbyGames( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseLeaveGame( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseConnectGame( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseGameBroadcast( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseGameDirect( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseGameKickClient( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseDirectMsg( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseShowGameClients( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseSpecGameInfo( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParsePauseServerConn( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParsePauseAccept( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParsePauseConnect( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParsePauseClient( std::vector<std::string> &szWords, SCommand *pCmd );

	void ParseChatLobby( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseChatPrivate( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseAFK( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseChatJoin( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseChatChannels( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseChatIgnore( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseChatFriend( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseChatWhere( std::vector<std::string> &szWords, SCommand *pCmd );

	void ParseLadderTest( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseLadderWin( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseLadderInfo( std::vector<std::string> &szWords, SCommand *pCmd );

	void ParseForgotPassword( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParsePing( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseMultiTest( std::vector<std::string> &szWords, SCommand *pCmd );
	// server
	void ParseClientsList( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseClientState( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseKick( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseGames( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseReloadConfig( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseShowStatistics( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseBroadcast( std::vector<std::string> &szWords, SCommand *pCmd );

	void ParseShowLobbies( std::vector<std::string> &szWords, SCommand *pCmd );
	void ParseHelp( std::vector<std::string> &szWords, SCommand *pCmd );

	std::string szErrorMsg;
	void SetErrorMsg( const std::string &_szMsg ) { szErrorMsg = _szMsg; }
	const std::string& GetErrorMsg() const { return szErrorMsg; }

public:
	CCommands() : bServer( false ) { }
	CCommands( const bool bServer );

	virtual void GetStringCommands( std::vector<std::string> *pCommands );
	virtual bool LineEntered( const std::string &szLine, std::string *pszErr );
};


