#pragma once

#include "Server_Client_Common/CommandsInterface.h"

class CPacketProcessor;
struct IDatabase;

class CGameServer : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CGameServer );

	CObj<class CNet> pNet;
	CPtr<class CCommands> pCommands;
	CObj<class CClients> pClients;
	CObj<class CTerminal> pTerminal;

	std::vector< CPtr<CPacketProcessor> > lobbies;
	uint64_t nLastStatisticsLogTime;
	int nServerStatisticsLogPeriod;

	typedef void (CGameServer::*PROCESS_CMD_FUNC)( const SCommand &cmd );
	std::unordered_map<int, PROCESS_CMD_FUNC> processCmdsFuncs;
	std::list< CPtr<class CNetPacket> > consoleCommandPackets;

	// Owned here and handed to CClients, which is the only thing that queries
	// it. Kept alive past the lobbies, which hold pointers into CClients.
	CObj<IDatabase> pDatabase;
	uint64_t nDatabasePingTime;
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
	// szBackend selects the database: "mysql" for a MariaDB or MySQL daemon,
	// "sqlite" for a file. Empty takes what server.xml says, and what that says
	// by default is mysql, which is what the shipped configuration expects.
	// szDatabaseFile overrides the file name the sqlite backend opens.
	CGameServer( class CCommands *pCommands, const std::string &szCfgFile,
		const std::string &szBackend = std::string(),
		const std::string &szDatabaseFile = std::string() );
	virtual ~CGameServer();
	class CClients* GetClients() { return pClients; }

	void AddLobby( CPacketProcessor *pLobby );

	void Segment();
};


