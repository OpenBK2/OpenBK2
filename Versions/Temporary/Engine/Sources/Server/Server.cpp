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

#include <typeinfo.h>
#include "vendor/MySQL/include/mysql.h"


#define REGISTER_CMD_FUNC( cmd, FuncName ) \
processCmdsFuncs[cmd] = &CGameServer::##FuncName;

void ForcePacketRegistration(); // For too smart linker

CGameServer::CGameServer( CCommands *_pCommands, const string &szCfgFile )
: pCommands( _pCommands )
{
	REGISTER_CMD_FUNC( ESC_CLIENTS, CommandClientsList );
	REGISTER_CMD_FUNC( ESC_CLIENT_STATE, CommandClientState );
	REGISTER_CMD_FUNC( ESC_KICK, CommandKick );
	REGISTER_CMD_FUNC( ESC_GAMES, CommandGames );
	REGISTER_CMD_FUNC( ESC_RELOAD_CONFIG, CommandReloadConfig );
	REGISTER_CMD_FUNC( ESC_SHOW_STATISTICS, CommandShowStatistics );
	REGISTER_CMD_FUNC( ESC_BROADCAST, CommandBroadcast );

	pMySQL = new MYSQL();
	mysql_init( pMySQL );

	int nNetVersion, nPort;
	string szServerName, szDBName;
	int nTerminalPort;
	{
		CFileStream stream( szCfgFile, CFileStream::WIN_READ_ONLY );
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );

		pSaver->Add( "NetVersion", &nNetVersion );
		pSaver->Add( "Port", &nPort );
		pSaver->Add( "MySQLServer", &szServerName );
		pSaver->Add( "MySQLDBName", &szDBName );
		pSaver->Add( "ServerLogPeriod", &nServerStatisticsLogPeriod );
		pSaver->Add( "TerminalPort", &nTerminalPort );
	}

	const bool bRegisterClosedBetaUsers = false;
	vector<string> names;
	vector<string> passwords;
	vector<string> emails;
	if ( bRegisterClosedBetaUsers )
	{

		mysql_real_connect( pMySQL, "127.0.0.1", "NivalNET", "", "test", 0, 0, 0 );
		string szQuery = "SELECT name, pwd, email FROM users";
		mysql_real_query( pMySQL, szQuery.c_str(), szQuery.size() );
		MYSQL_RES *pResult = 0;
		pResult = mysql_store_result( pMySQL );
		for ( int i = 0; i < mysql_num_rows( pResult ); ++i )
		{
			MYSQL_ROW row = mysql_fetch_row( pResult );
			names.push_back( row[0] );
			passwords.push_back( row[1] );
			emails.push_back( row[2] );
		}
		mysql_free_result( pResult );
	}

	if ( !mysql_real_connect( pMySQL, szServerName.c_str(), "NivalNET", "", szDBName.c_str(), 0, 0, 0 ) )
	{
		NI_ASSERT( false, "MySQL connection error!" );
	}
	WriteMSG( "MySQL connection established. \n" );
	WriteMSG( "MySQLServer = %s, MySQLDBName = %s\n", szServerName.c_str(), szDBName.c_str() );

	pClients = new CClients( pMySQL );
	
	if ( bRegisterClosedBetaUsers )
	{
		for ( int i = 0; i < names.size(); ++i )
		{
			string szName = names[i];
			string szPwd = passwords[i];
			string szEmail = emails[i];
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
	
	nMySQLLastPingTime = GetLongTickCount();
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
		hash_map<int, PROCESS_CMD_FUNC>::iterator iter = processCmdsFuncs.find( cmd.nCmd );
//		NI_ASSERT( iter != processCmdsFuncs.end(), StrFmt( "Can't process cmd %d", cmd.nCmd ) );

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
		pClients->Log( pPacket->nClientID, StrFmt( "receive %s, %d", GetPacketInfo( pPacket ), i ) );
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
			pClients->Log( pPacket->nClientID, StrFmt( "%d send %s", i, GetPacketInfo( pPacket ) ) );
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

	const UINT64 nTime = GetLongTickCount();
	if ( nTime > nMySQLLastPingTime + 60000 ) // once per minute
	{
		nMySQLLastPingTime = GetLongTickCount();
		while( mysql_ping( pMySQL ) );
	}

	if ( nTime > nLastStatisticsLogTime + nServerStatisticsLogPeriod )
	{
		nLastStatisticsLogTime = nTime;
		vector<string> names;
		vector<float> values;
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
		const hash_map<int, SCommonClientInfo> &onLine = pClients->GetOnLine();
		string szList;
		if ( onLine.empty() )
			szList = "no clients online\n";
		else
		{
			if ( onLine.size() < 20 )
			{
				szList = "clients list: \n";
				for ( hash_map<int, SCommonClientInfo>::const_iterator iter = onLine.begin(); iter != onLine.end(); ++iter )
				{
					string szNick;
					if ( !pClients->GetNick( iter->first, &szNick ) )
						szList += StrFmt( "  something wrong with client %d\n", iter->first );
					else
						szList += "  " + szNick + "\n";
				}
			}
			szList += StrFmt( "Total clients: %d\n", onLine.size() );
		}

		WriteMSG( "%s", szList.c_str() );
	}
}

void CGameServer::CommandKick( const SCommand &cmd )
{
	string szStr;
	const string szNick = cmd.GetStr( 0 );
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
	string szStr;
	
	int nID;
	if ( !pClients->GetClientID( cmd.GetStr( 0 ), &nID ) )
		szStr = StrFmt( "Nick %s isn't online", cmd.GetStr( 0 ).c_str() );
	else
	{
		SCommonClientInfo clientInfo;
		if ( !pClients->GetCommonClientInfo( nID, &clientInfo ) )
			szStr = StrFmt( "Something wrong with nick %s", cmd.GetStr( 0 ).c_str() );
		else
		{
			szStr = StrFmt( "client %s: ", cmd.GetStr( 0 ).c_str() );
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
				szStr += StrFmt( ", in game %d", clientInfo.nGameID );

			WriteMSG( "%s", (szStr + "\n").c_str() );
		}
	}
}

void CGameServer::CommandReloadConfig( const SCommand &cmd )
{
	for ( vector< CPtr<CPacketProcessor> >::iterator it = lobbies.begin(); it != lobbies.end(); ++it )
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
	const string szFileName = "../Messages/" + cmd.params[0];
	string szText;
	{
		CFileStream stream( szFileName, CFileStream::WIN_READ_ONLY );
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
		if ( pSaver )
			pSaver->Add( "Text", &szText );
	}
	wstring wszText = NStr::ToUnicode( szText );
	CChatLobby::SetWelcomeText( wszText );
	const hash_map<int, SCommonClientInfo>& onlineClients = pClients->GetOnLine();
	for ( hash_map<int, SCommonClientInfo>::const_iterator it = onlineClients.begin(); it != onlineClients.end(); ++it )
	{
		const int nID = it->first;
		pNet->SendPacket( new CSystemBroadcastPacket( nID, wszText ) );
	}
}

CGameServer::~CGameServer()
{
	mysql_close( pMySQL );
	delete pMySQL;
}

#undef REGISTER_CMD_FUNC

