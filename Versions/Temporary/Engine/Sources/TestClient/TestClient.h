#pragma once

#include "../Server_Client_Common/CommandsInterface.h"

class CCommands;
interface IMultiTester;
class CPinger;

class CTestClient : virtual public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CTestClient );

	CPtr<CCommands> pCommands;

	typedef void (CTestClient::*PROCESS_CMD_FUNC)( const SCommand &cmd );
	hash_map<int, PROCESS_CMD_FUNC> processCmdsFuncs;

	CPtr<class CServerClient> pServerClient;
	CPtr<class CTestClientProcessor> pTestClientProcessor;
	CObj<CPinger> pPinger;

	list< CPtr<class CNetPacket> > consoleCommandPackets;

	int nNetVersion, nPort;
	string szIP;

	vector< CObj<IMultiTester> > testers;
	int nTestersNameShift;
	DWORD dwLastTesterCreationTime;
	//
	void ProcessCommands();

	//
	void CommandLogin( const SCommand &cmd );
	void CommandRegister( const SCommand &cmd );
	void CommandEnterLobby( const SCommand &cmd );
	void CommandLeaveLobby( const SCommand &cmd );
	void CommandClientState( const SCommand &cmd );
	void CommandGetLobbyClients( const SCommand &cmd );
	void CommandCreateGame( const SCommand &cmd );
	void CommandKillGame( const SCommand &cmd );
	void CommandSendGameInfo( const SCommand &cmd );
	void CommandGetLobbyGames( const SCommand &cmd );
	void CommandLeaveGame( const SCommand &cmd );
	void CommandConnectGame( const SCommand &cmd );
	void CommandGameBroadcastMsg( const SCommand &cmd );
	void CommandGameDirectMsg( const SCommand &cmd );
	void CommandGameKickClient( const SCommand &cmd );
	void CommandDirectMsg( const SCommand &cmd );
	void CommandShowGameClients( const SCommand &cmd );
	void CommandSendSpecGameInfo( const SCommand &cmd );
	void CommandPauseServerConn( const SCommand &cmd );
	void CommandPauseAccept( const SCommand &cmd );
	void CommandPauseConnect( const SCommand &cmd );
	void CommandPauseClient( const SCommand &cmd );

	void CommandChatLobby( const SCommand &cmd );
	void CommandChatPrivate( const SCommand &cmd );
	void CommandAFK( const SCommand &cmd );
	void CommandChatJoin( const SCommand &cmd );
	void CommandChatChannels( const SCommand &cmd );
	void CommandChatIgnore( const SCommand &cmd );
	void CommandChatFriend( const SCommand &cmd );
	void CommandChatWhere( const SCommand &cmd );

	void CommandLadderTest( const SCommand &cmd );
	void CommandLadderWin( const SCommand &cmd );
	void CommandLadderInfo( const SCommand &cmd );

	void CommandForgotPassword( const SCommand &cmd );
	void CommandPing( const SCommand &cmd );
	void CommandMultiTestServer( const SCommand &cmd );
public:
	CTestClient() { }
	CTestClient( CCommands *pCommands, const string &szCfgFile );

	void Segment();
};


