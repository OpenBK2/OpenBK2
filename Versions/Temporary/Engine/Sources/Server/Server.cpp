#include "stdafx.h"
#include "clients.h"
#include "ControlLobby.h"
#include "InternalPackets.h"
#include "Server.h"
#include "Chat.h"
#include "Server_Client_Common/CommonPackets.h"

#include "libdb/Logger.h"
#include "Misc/StrProc.h"
#include "Server_Client_Common/Commands.h"
#include "Server_Client_Common/LobbiesIDs.h"
#include "Server_Client_Common/Net.h"
#include "Server_Client_Common/NetLogger.h"
#include "Misc/Time64.h"
#include "System/XmlSaver.h"
#include "Terminal.h"

#include "Statistics.h"

#include "Database.h"

#include <fmt/format.h>


// The ## was a no-op that only MSVC accepted: FuncName is already a complete
// token here, and a conforming preprocessor rejects a ## with nothing on its
// left to paste onto.
#define REGISTER_CMD_FUNC( cmd, FuncName ) \
processCmdsFuncs[cmd] = &CGameServer::FuncName;

void ForcePacketRegistration(); // For too smart linker

CGameServer::CGameServer( CCommands *_pCommands, const std::string &szCfgFile )
: pCommands( _pCommands )
{
	REGISTER_CMD_FUNC( ESC_CLIENTS, CommandClientsList );
	REGISTER_CMD_FUNC( ESC_CLIENT_STATE, CommandClientState );
	REGISTER_CMD_FUNC( ESC_KICK, CommandKick );
	REGISTER_CMD_FUNC( ESC_GAMES, CommandGames );
	REGISTER_CMD_FUNC( ESC_RELOAD_CONFIG, CommandReloadConfig );
	REGISTER_CMD_FUNC( ESC_SHOW_STATISTICS, CommandShowStatistics );
	REGISTER_CMD_FUNC( ESC_BROADCAST, CommandBroadcast );

	pDatabase = CreateMariaDbDatabase();

	// Initialised, because a configuration file that opens but does not parse
	// leaves every one of these untouched, and they were being read anyway.
	int nNetVersion = 0, nPort = 0;
	std::string szServerName, szDBName;
	int nTerminalPort = 0;
	{
		CFileStream stream( szCfgFile, CFileStream::WIN_READ_ONLY );
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
		if ( !pSaver )
		{
			// This used to dereference the null and take the process with it,
			// with nothing printed: an access violation before any of the
			// output below could run. main checks the file before getting
			// here, so reaching this means it went away in between.
			WriteMSG( "Cannot read the configuration file: %s\n", szCfgFile.c_str() );
			return;
		}

		pSaver->Add( "NetVersion", &nNetVersion );
		pSaver->Add( "Port", &nPort );
		pSaver->Add( "MySQLServer", &szServerName );
		pSaver->Add( "MySQLDBName", &szDBName );
		pSaver->Add( "ServerLogPeriod", &nServerStatisticsLogPeriod );
		pSaver->Add( "TerminalPort", &nTerminalPort );
	}

	const bool bRegisterClosedBetaUsers = false;
	std::vector<std::string> names;
	std::vector<std::string> passwords;
	std::vector<std::string> emails;
	if ( bRegisterClosedBetaUsers )
	{
		SDbConnection betaConnection;
		betaConnection.szHost = "127.0.0.1";
		betaConnection.szUser = "NivalNET";
		betaConnection.szDatabase = "test";
		pDatabase->Connect( betaConnection );

		CDbResult result;
		pDatabase->Query( "SELECT name, pwd, email FROM users", &result );
		for ( int i = 0; i < result.GetRowCount(); ++i )
		{
			names.push_back( result.Get( i, 0 ) );
			passwords.push_back( result.Get( i, 1 ) );
			emails.push_back( result.Get( i, 2 ) );
		}
	}

	SDbConnection connection;
	connection.szHost = szServerName;
	connection.szUser = "NivalNET";
	connection.szDatabase = szDBName;
	// Reported, not asserted. NI_ASSERT expands to nothing while _DO_ASSERT_SLOW
	// is undefined, which is every build that exists, so a failure here was
	// swallowed and the success message printed regardless. The server then ran
	// with no database at all, announcing that it had one, and the only sign of
	// trouble was every query retrying once and failing twice.
	//
	// It carries on rather than giving up, because that is what it did before
	// and because the failure is usually a database that has not been started
	// yet. Nothing will work until it is: every login, chat and ladder path is
	// a query.
	const bool bConnected = pDatabase->Connect( connection );
	if ( bConnected )
	{
		WriteMSG( "Database connection established.\n" );
	}
	else
	{
		WriteMSG( "Database connection FAILED: %s\n", pDatabase->GetLastError().c_str() );
	}
	WriteMSG( "Server = %s, DBName = %s\n", szServerName.c_str(), szDBName.c_str() );

	pClients = new CClients( pDatabase );
	
	if ( bRegisterClosedBetaUsers )
	{
		for ( int i = 0; i < names.size(); ++i )
		{
			std::string szName = names[i];
			std::string szPwd = passwords[i];
			std::string szEmail = emails[i];
			if ( !pClients->IsNickRegistered( szName ) )
				pClients->Register( szName, szPwd, szName + szPwd + std::to_string(  names.size() ), szEmail );
		}
	}

//		for ( int i = 0; i < 10000; ++i )
//		{
//			string szText = std::to_string(  i );
//			pClients->Register( szText, szText, szText );
//		}

	pNet = new CNet( nNetVersion, nPort, 30 );
	CNet::SetTimeOut( 20.0f );
	const bool bSuccess = pNet->InitAsServer();
	if ( !bSuccess )
		WriteMSG( "Cannot bind socket to the port %d\n", nPort );

	pNet->StartGame();
	pNet->StartNewPlayerAccept();

	AddLobby( new CControlLobby( pClients, pNet, szCfgFile ) );

	AddLobby( new CChatLobby( pClients, szCfgFile ) );

	WriteMSG( "Server started, port %d, gameversion %d\n", nPort, nNetVersion );
	
	nDatabasePingTime = GetLongTickCount();
	nLastStatisticsLogTime = GetLongTickCount() - nServerStatisticsLogPeriod;
	pTerminal = new CTerminal( pCommands, nTerminalPort );
}

void CGameServer::AddLobby( CPacketProcessor *pLobby )
{
	lobbies.push_back( pLobby );
}

void CGameServer::ProcessCommands()
{
	SCommand cmd;
	while ( pCommands->GetCommand( &cmd ) )
	{
		std::unordered_map<int, PROCESS_CMD_FUNC>::iterator iter = processCmdsFuncs.find( cmd.nCmd );
//		NI_ASSERT( iter != processCmdsFuncs.end(), fmt::format( "Can't process cmd {}", cmd.nCmd ) );

		if ( iter == processCmdsFuncs.end() )
			continue;

		PROCESS_CMD_FUNC pfnFunc = iter->second;
		(this->*pfnFunc)( cmd );
	}

	while ( !consoleCommandPackets.empty() )
	{
		CPtr<CNetPacket> pPacket = consoleCommandPackets.front();
		consoleCommandPackets.pop_front();

		int i = 0;
		while ( i < lobbies.size() && !lobbies[i]->ProcessPacket( pPacket ) )
			++i;
	}
}

void CGameServer::RecievePackets()
{
	while ( CPtr<CNetPacket> pPacket = pNet->ReceivePacket() )
	{
		const bool bOnline = pClients->IsOnLine( pPacket->nClientID );
		const bool bControlProcessed = lobbies[0]->ProcessPacket( pPacket );

		int i = 0;
		if ( bOnline && !bControlProcessed )
		{
			while ( i < lobbies.size() && !lobbies[i]->ProcessPacket( pPacket ) )
				++i;
		}
#ifndef _FINALRELEASE
		pClients->Log( pPacket->nClientID, fmt::format( "receive {}, {}", GetPacketInfo( pPacket ), i ) );
#endif
	}
}

void CGameServer::SendPackets()
{
	for ( int i = 0; i < lobbies.size(); ++i )
	{
		int nPacketsPerLobbyLeft = 200; 
		while ( CPtr<CNetPacket> pPacket = lobbies[i]->GetPacket() )
		{
#ifndef _FINALRELEASE
			pClients->Log( pPacket->nClientID, fmt::format( "{} send {}", i, GetPacketInfo( pPacket ) ) );
#endif
			pNet->SendPacket( pPacket );
			--nPacketsPerLobbyLeft;
			if ( nPacketsPerLobbyLeft == 0 && lobbies[i]->CanBePaused() )
				break;
		}
	}
}

void CGameServer::Segment()
{
	RecievePackets();
	for ( int i = 0; i < lobbies.size(); ++i )
		lobbies[i]->Segment();
	SendPackets();

	ProcessCommands();
	pClients->RecalcDBOverload();

	const uint64_t nTime = GetLongTickCount();
	if ( nTime > nDatabasePingTime + 60000 ) // once per minute
	{
		nDatabasePingTime = GetLongTickCount();
		// Spins until the database answers, which is what the mysql_ping loop
		// this replaces did. The whole server is this one thread, so a database
		// that stays down stops everything, including the terminal. Left as it
		// was rather than changed here; it wants its own commit.
		while ( !pDatabase->IsAlive() )
		{
		}
	}

	if ( nTime > nLastStatisticsLogTime + nServerStatisticsLogPeriod )
	{
		nLastStatisticsLogTime = nTime;
		std::vector<std::string> names;
		std::vector<float> values;
		NStatistics::DumpToNameValueVectors( &names, &values );
		NStatistics::Reset();
		pClients->DBLogServerStatistics( names, values );
	}

	pTerminal->Segment();
}

void CGameServer::CommandClientsList( const SCommand &cmd )
{
	if ( !cmd.params.empty() )
	{
		CGetLobbyClientsListPacket *pPacket = new CGetLobbyClientsListPacket( cmd.GetInt( 0 ) );
		consoleCommandPackets.push_back( pPacket );
	}
	else
	{
		const std::unordered_map<int, SCommonClientInfo> &onLine = pClients->GetOnLine();
		std::string szList;
		if ( onLine.empty() )
			szList = "no clients online\n";
		else
		{
			if ( onLine.size() < 20 )
			{
				szList = "clients list: \n";
				for ( std::unordered_map<int, SCommonClientInfo>::const_iterator iter = onLine.begin(); iter != onLine.end(); ++iter )
				{
					std::string szNick;
					if ( !pClients->GetNick( iter->first, &szNick ) )
						szList += fmt::format( "  something wrong with client {}\n", iter->first );
					else
						szList += "  " + szNick + "\n";
				}
			}
			szList += fmt::format( "Total clients: {}\n", onLine.size() );
		}

		WriteMSG( "%s", szList.c_str() );
	}
}

void CGameServer::CommandKick( const SCommand &cmd )
{
	std::string szStr;
	const std::string szNick = cmd.GetStr( 0 );
	if ( pClients->IsOnLine( szNick ) )
	{
		int nID;
		if ( pClients->GetClientID( szNick, &nID ) )
		{
			pNet->Kick( nID );
			szStr = "kicked " + cmd.GetStr( 0 ) + "\n";
		}
		else
			szStr = "something wrong with " + cmd.GetStr( 0 ) + "\n";
	}
	else
		szStr = "unknown client " + cmd.GetStr( 0 ) + "\n";

	WriteMSG( "%s", szStr.c_str() );	
}

void CGameServer::CommandGames( const SCommand &cmd )
{
	CShowLobbyGamesPacket *pPacket = new CShowLobbyGamesPacket( ERID_CUSTOM );
	consoleCommandPackets.push_back( pPacket );
}

void CGameServer::CommandClientState( const SCommand &cmd )
{
	std::string szStr;
	
	int nID;
	if ( !pClients->GetClientID( cmd.GetStr( 0 ), &nID ) )
		szStr = fmt::format( "Nick {} isn't online", cmd.GetStr( 0 ) );
	else
	{
		SCommonClientInfo clientInfo;
		if ( !pClients->GetCommonClientInfo( nID, &clientInfo ) )
			szStr = fmt::format( "Something wrong with nick {}", cmd.GetStr( 0 ) );
		else
		{
			szStr = fmt::format( "client {}: ", cmd.GetStr( 0 ) );
			if ( clientInfo.bWant2ReceiveChat )
				szStr += "chat open, ";
			else
				szStr += "chat closed, ";

			switch ( clientInfo.eState )
			{
				case ES_ONLINE: szStr += "online"; break;
				case ES_AWAY: szStr += "away"; break;
				case ES_INGAME: szStr += "ingame"; break;
			}

			switch ( clientInfo.cLobbyID )
			{
				case ERID_CUSTOM:
					szStr += ", in custom lobby";
					break;
				case ERID_NO_LOBBY:
					szStr += ", not in a lobby";
					break;
				default:
					szStr += ", unknown lobby: something wrong with the lobby info";
			}

			if ( clientInfo.nGameID == -1 )
				szStr += ", not in a game";
			else
				szStr += fmt::format( ", in game {}", clientInfo.nGameID );

			WriteMSG( "%s", (szStr + "\n").c_str() );
		}
	}
}

void CGameServer::CommandReloadConfig( const SCommand &cmd )
{
	for ( std::vector< CPtr<CPacketProcessor> >::iterator it = lobbies.begin(); it != lobbies.end(); ++it )
	{
		CPacketProcessor * pLobby = *it;
		pLobby->ReloadConfig();
	}
	WriteMSG( "%s", "Configuration reloaded.\n" );
}

void CGameServer::CommandShowStatistics( const SCommand &cmd )
{
	WriteMSG( "%s", NStatistics::DumpToString().c_str() );
	WriteMSG( "Current database load is %f QPS.\n", pClients->GetQPS() );
}

void CGameServer::CommandBroadcast( const SCommand &cmd )
{
	const std::string szFileName = "../Messages/" + cmd.params[0];
	std::string szText;
	{
		CFileStream stream( szFileName, CFileStream::WIN_READ_ONLY );
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
		if ( pSaver )
			pSaver->Add( "Text", &szText );
	}
	std::wstring wszText = NStr::ToUnicode( szText );
	CChatLobby::SetWelcomeText( wszText );
	const std::unordered_map<int, SCommonClientInfo>& onlineClients = pClients->GetOnLine();
	for ( std::unordered_map<int, SCommonClientInfo>::const_iterator it = onlineClients.begin(); it != onlineClients.end(); ++it )
	{
		const int nID = it->first;
		pNet->SendPacket( new CSystemBroadcastPacket( nID, wszText ) );
	}
}

CGameServer::~CGameServer()
{
	// CObj closes and releases the database; the lobbies and CClients that
	// hold it are released before this runs.
}

#undef REGISTER_CMD_FUNC

