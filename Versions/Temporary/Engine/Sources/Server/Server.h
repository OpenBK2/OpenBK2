#pragma once

#include "../Server_Client_Common/CommandsInterface.h"

class CPacketProcessor;
struct st_mysql;
typedef st_mysql MYSQL;

class CGameServer : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CGameServer );

	CObj<class CNet> pNet;
	CPtr<class CCommands> pCommands;
	CObj<class CClients> pClients;
	CObj<class CTerminal> pTerminal;

	vector< CPtr<CPacketProcessor> > lobbies;
	UINT64 nLastStatisticsLogTime;
	int nServerStatisticsLogPeriod;

	typedef void (CGameServer::*PROCESS_CMD_FUNC)( const SCommand &cmd );
	hash_map<int, PROCESS_CMD_FUNC> processCmdsFuncs;
	list< CPtr<class CNetPacket> > consoleCommandPackets;

	MYSQL *pMySQL;
	UINT64 nMySQLLastPingTime;
	//
	void CommandClientsList( const SCommand &cmd );
	void CommandClientState( const SCommand &cmd );
	void CommandKick( const SCommand &cmd );
	void CommandGames( const SCommand &cmd );
	void CommandReloadConfig( const SCommand &cmd );
	void CommandShowStatistics( const SCommand &cmd );
	void CommandBroadcast( const SCommand &cmd );

	void ProcessCommands();
	void RecievePackets();
	void SendPackets();
public:
	CGameServer() { }
	CGameServer( class CCommands *pCommands, const string &szCfgFile );
	virtual ~CGameServer();
	class CClients* GetClients() { return pClients; }

	void AddLobby( CPacketProcessor *pLobby );

	void Segment();
};


