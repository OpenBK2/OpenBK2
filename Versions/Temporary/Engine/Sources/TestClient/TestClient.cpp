#include "stdafx.h"

#include "InternalPackets.h"
#include "TestClient.h"
#include "TestClientProcessor.h"

#include "Client/ServerClient.h"
#include "Misc/StrProc.h"
#include "Server_Client_Common/Commands.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/LoginPackets.h"
#include "Server_Client_Common/LobbiesIDs.h"
#include "Server_Client_Common/LadderLobbyPackets.h"
#include "System/RandomGen.h"
#include "MultiTester.h"
#include "Pinger.h"

#include <cstdint>

#define LADDER_TEST_LOG
#undef LADDER_TEST_LOG

const int TESTER_CREATION_TIMEOUT = 2000;

#define REGISTER_CMD_FUNC( cmd, FuncName ) \
processCmdsFuncs[cmd] = &CTestClient::##FuncName;

void ForcePacketRegistration(); // For too smart linker

CTestClient::CTestClient( CCommands *_pCommands, const string &szCfgFile )
{
	pCommands = _pCommands;

	REGISTER_CMD_FUNC( ESC_LOGIN, CommandLogin );
	REGISTER_CMD_FUNC( ESC_REGISTER, CommandRegister );
	REGISTER_CMD_FUNC( ESC_ENTER_LOBBY, CommandEnterLobby );
	REGISTER_CMD_FUNC( ESC_LEAVE_LOBBY, CommandLeaveLobby );
	REGISTER_CMD_FUNC( ESC_SET_CLIENT_STATE, CommandClientState );
	REGISTER_CMD_FUNC( ESC_GET_LOBBY_CLIENTS, CommandGetLobbyClients );
	REGISTER_CMD_FUNC( ESC_CREATE_GAME, CommandCreateGame );
	REGISTER_CMD_FUNC( ESC_KILL_GAME, CommandKillGame );
	REGISTER_CMD_FUNC( ESC_SEND_GAME_INFO, CommandSendGameInfo );
	REGISTER_CMD_FUNC( ESC_GET_LOBBY_GAMES, CommandGetLobbyGames );
	REGISTER_CMD_FUNC( ESC_LEAVE_GAME, CommandLeaveGame );
	REGISTER_CMD_FUNC( ESC_CONNECT_GAME, CommandConnectGame );
	REGISTER_CMD_FUNC( ESC_GAME_BROADCAST, CommandGameBroadcastMsg );
	REGISTER_CMD_FUNC( ESC_GAME_DIRECT, CommandGameDirectMsg );
	REGISTER_CMD_FUNC( ESC_GAME_KICK_CLIENT, CommandGameKickClient );
	REGISTER_CMD_FUNC( ESC_DIRECT_MSG, CommandDirectMsg );
	REGISTER_CMD_FUNC( ESC_GAME_CLIENTS, CommandShowGameClients );
	REGISTER_CMD_FUNC( ESC_SPEC_GAME_INFO, CommandSendSpecGameInfo );
	REGISTER_CMD_FUNC( ESC_PAUSE_SERVER_CONN, CommandPauseServerConn );
	REGISTER_CMD_FUNC( ESC_PAUSE_ACCEPT, CommandPauseAccept );
	REGISTER_CMD_FUNC( ESC_PAUSE_CONNECT, CommandPauseConnect );
	REGISTER_CMD_FUNC( ESC_PAUSE_CLIENT, CommandPauseClient );

	REGISTER_CMD_FUNC( ESC_LADDER_TEST, CommandLadderTest );
	REGISTER_CMD_FUNC( ESC_LADDER_WIN, CommandLadderWin );
	REGISTER_CMD_FUNC( ESC_LADDER_INFO, CommandLadderInfo );

	REGISTER_CMD_FUNC( ESC_CHAT_LOBBY, CommandChatLobby );
	REGISTER_CMD_FUNC( ESC_CHAT_PRIVATE, CommandChatPrivate );
	REGISTER_CMD_FUNC( ESC_AFK, CommandAFK );
	REGISTER_CMD_FUNC( ESC_CHAT_JOIN, CommandChatJoin );
	REGISTER_CMD_FUNC( ESC_CHAT_CHANNELS, CommandChatChannels );
	REGISTER_CMD_FUNC( ESC_CHAT_IGNORE, CommandChatIgnore );
	REGISTER_CMD_FUNC( ESC_CHAT_FRIEND, CommandChatFriend );
	REGISTER_CMD_FUNC( ESC_CHAT_WHERE, CommandChatWhere );

	REGISTER_CMD_FUNC( ESC_FORGOT_PASSWORD, CommandForgotPassword );
	REGISTER_CMD_FUNC( ESC_PING, CommandPing );
	REGISTER_CMD_FUNC( ESC_MULTI_TEST, CommandMultiTestServer );

	pTestClientProcessor = new CTestClientProcessor( szCfgFile );

	int nPingerPeriod;
	string szPingerName;
	string szPingerPwd;
	{
		CFileStream stream( szCfgFile, CFileStream::WIN_READ_ONLY );
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );

		pSaver->Add( "ServerNetVersion", &nNetVersion );
		pSaver->Add( "ServerPort", &nPort );
		pSaver->Add( "ServerIP", &szIP );
		pSaver->Add( "PingerPeriod", &nPingerPeriod );
		pSaver->Add( "PingerName", &szPingerName );
		pSaver->Add( "PingerPassword", &szPingerPwd );
	}
	dwLastTesterCreationTime = GetTickCount();
	pPinger = new CPinger( nPingerPeriod, szPingerName, szPingerPwd, szIP, nPort, nNetVersion );

	ForcePacketRegistration();
}

void CTestClient::CommandLogin( const SCommand &cmd )
{
	pServerClient = new CServerClient( szIP.c_str(), nNetVersion, nPort, 30 );

	CNetPacket *pPacket = new CLoginPacket( 0, cmd.GetStr( 0 ), cmd.GetStr( 1 ) );
	pServerClient->SendPacket( pPacket );

	Singleton<IConsoleBuffer>()->WriteASCII(
		CONSOLE_STREAM_CONSOLE,
		StrFmt( "logging in server %s, port %d, gameversion %d, nickname %s, password %s...\n",
						szIP.c_str(), nPort, nNetVersion, cmd.GetStr( 0 ).c_str(), cmd.GetStr( 1 ).c_str() )
	);
	pTestClientProcessor->SetServerClient( pServerClient );

}

void CTestClient::CommandRegister( const SCommand &cmd )
{
	pServerClient = new CServerClient( szIP.c_str(), nNetVersion, nPort, 30 );

	CNetPacket *pPacket = new CRegisterPacket( 0, cmd.GetStr( 0 ), cmd.GetStr( 1 ), cmd.GetStr( 2 ), cmd.GetStr( 3 ) );
	pServerClient->SendPacket( pPacket );

	Singleton<IConsoleBuffer>()->WriteASCII(
		CONSOLE_STREAM_CONSOLE,
		StrFmt( "registering to server %s, port %d, gameversion %d, nickname %s, password %s, cdkey %s, email %s...\n",
		szIP.c_str(), nPort, nNetVersion, cmd.GetStr( 0 ).c_str(), cmd.GetStr( 1 ).c_str(), cmd.GetStr( 2 ).c_str(), cmd.GetStr( 3 ) )
	);

	pTestClientProcessor->SetServerClient( pServerClient );
}

void CTestClient::CommandChatLobby( const SCommand &cmd )
{
	if ( pServerClient )
	{
		CNetPacket *pPacket = new CChatPacket( 0, NStr::ToUnicode( cmd.GetStr( 0 ).c_str() ), "", 0, true );
		pServerClient->SendPacket( pPacket );
	}
	else
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "not connected yet\n" );
}

void CTestClient::CommandChatPrivate( const SCommand &cmd )
{
	if ( pServerClient )
	{
		CNetPacket *pPacket = new CChatPacket( 0, NStr::ToUnicode( cmd.GetStr( 1 ).c_str() ), cmd.GetStr( 0 ).c_str(), 0, false );
		pServerClient->SendPacket( pPacket );
	}
	else
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "not connected yet\n" );
}

void CTestClient::CommandAFK( const SCommand &cmd )
{
	if ( pServerClient )
	{
		CNetPacket *pPacket = new CChatAFKPacket( 0 );
		pServerClient->SendPacket( pPacket );
	}
	else
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "not connected yet\n" );
}

void CTestClient::CommandChatJoin( const SCommand &cmd )
{
	if ( pServerClient )
	{
		CNetPacket *pPacket = new CChatChannelPacket( 0, cmd.GetStr( 0 ) );
		pServerClient->SendPacket( pPacket );
	}
	else
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "not connected yet\n" );
}

void CTestClient::CommandChatChannels( const SCommand &cmd )
{
	if ( pServerClient )
	{
		// Don't send ( 0,0 ), send ( 0, dwYourCurrentListVersion ) ! Please..
		CNetPacket *pPacket = new CChatChannelsListRequestPacket( 0, 0 );
		pServerClient->SendPacket( pPacket );
	}
	else
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "not connected yet\n" );
}

void CTestClient::CommandChatIgnore( const SCommand &cmd )
{
	if ( pServerClient )
	{
		CNetPacket *pPacket = new CChatModifyIgnoreFriendListPacket( 0, cmd.GetStr( 0 ),
			( cmd.GetInt( 1 ) == 1 ) ? CChatModifyIgnoreFriendListPacket::ADD_IGNORE : CChatModifyIgnoreFriendListPacket::REMOVE_IGNORE );
		pServerClient->SendPacket( pPacket );
		pPacket = new CChatGetIgnoreFriendListPacket( 0 );
		pServerClient->SendPacket( pPacket );
	}
	else
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "not connected yet\n" );
}

void CTestClient::CommandChatFriend( const SCommand &cmd )
{
	if ( pServerClient )
	{
		CNetPacket *pPacket = new CChatModifyIgnoreFriendListPacket( 0, cmd.GetStr( 0 ),
			( cmd.GetInt( 1 ) == 1 ) ? CChatModifyIgnoreFriendListPacket::ADD_FRIEND : CChatModifyIgnoreFriendListPacket::REMOVE_FRIEND );
		pServerClient->SendPacket( pPacket );
		pPacket = new CChatGetIgnoreFriendListPacket( 0 );
		pServerClient->SendPacket( pPacket );
	}
	else
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "not connected yet\n" );
}

void CTestClient::CommandChatWhere( const SCommand &cmd )
{
	if ( pServerClient )
	{
		CNetPacket *pPacket = new CChatChannelByNickPacket( 0, cmd.GetStr( 0 ), "" );
		pServerClient->SendPacket( pPacket );
	}
	else
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "not connected yet\n" );
}

void CTestClient::CommandEnterLobby( const SCommand &cmd )
{
	if ( pServerClient )
	{
		CNetPacket *pPacket;
		if ( cmd.GetStr( 0 ) == string( "custom" ) )
			pPacket = new CEnterLobbyPacket( 0, ERID_CUSTOM );
		else
			pPacket = new CEnterLobbyPacket( 0, ERID_LADDER );
    
		pServerClient->SendPacket( pPacket );

		consoleCommandPackets.push_back( new CEnteredLobby() );
	}
	else
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "not connected yet\n" );
}

void CTestClient::CommandLeaveLobby( const SCommand &cmd )
{
	if ( pServerClient )
	{
		CNetPacket *pPacket = new CLeaveLobbyPacket( 0 );
		pServerClient->SendPacket( pPacket );
	}
	else
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "not connected yet\n" );
}

void CTestClient::CommandClientState( const SCommand &cmd )
{
	if ( pServerClient )
	{
		CNetPacket *pPacket = new CCommonClientStatePacket( 0, ECommonClientState( cmd.GetInt( 0 ) ) );
		pServerClient->SendPacket( pPacket );
	}
	else
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "not connected yet\n" );
}

void CTestClient::CommandGetLobbyClients( const SCommand &cmd )
{
	if ( pServerClient )
		consoleCommandPackets.push_back( new CGetLobbyClientsPacket( 0, 0 ) );
	else
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "not connected yet\n" );
}

void CTestClient::CommandCreateGame( const SCommand &cmd )
{
	if ( pServerClient )
	{
		CPtr<CNetPacket> pPacket = new CCreateGamePacket( 0 );
		consoleCommandPackets.push_back( pPacket );
	}
}

void CTestClient::CommandKillGame( const SCommand &cmd )
{
	if ( pServerClient )
	{
		if ( cmd.params.size() == 0 )   
			consoleCommandPackets.push_back( new CKillGamePacket( 0, -1 ) );
		else
			consoleCommandPackets.push_back( new CKillGamePacket( 0, cmd.GetInt( 0 ) ) );
	}
}

void CTestClient::CommandSendGameInfo( const SCommand &cmd )
{
	if ( pServerClient )
	{
		CUpdateGameInfo *pPacket = new CUpdateGameInfo();
		pPacket->gameInfo.szName = cmd.GetStr(0);
		pPacket->gameInfo.nMaxPlayers = cmd.GetInt(1);
		pPacket->gameInfo.szPassword = cmd.GetStr(2);
		pPacket->gameInfo.bHasPassword = cmd.GetStr(2) != "";
		pPacket->gameInfo.bCanConnect = cmd.GetInt(3);
		consoleCommandPackets.push_back( pPacket );
	}
}

void CTestClient::CommandGetLobbyGames( const SCommand &cmd )
{
	if ( pServerClient )
		consoleCommandPackets.push_back( new CGetLobbyGamesPacket( 0, 0 ) );
}

void CTestClient::CommandLeaveGame( const SCommand &cmd )
{
	if ( pServerClient )
		consoleCommandPackets.push_back( new CLeaveGamePacket( 0, 0 ) );
}

void CTestClient::CommandConnectGame( const SCommand &cmd )
{
	if ( pServerClient )
		consoleCommandPackets.push_back( new CConnectGamePacket( 0, cmd.GetInt(0), cmd.GetStr(1) ) );
}

void CTestClient::CommandGameBroadcastMsg( const SCommand &cmd )
{
	if ( pServerClient )
	{
		CPtr<CGameTestBroadcastMsg> pPacket = new CGameTestBroadcastMsg( cmd.GetInt(0), cmd.GetStr(1) );
		pServerClient->SendGamePacket( pPacket, true );
	}
}

void CTestClient::CommandGameDirectMsg( const SCommand &cmd )
{
	if ( pServerClient )
	{
		CPtr<CGameTestDirectMsg> pPacket =
			new CGameTestDirectMsg( cmd.GetInt(0), cmd.GetInt(1), cmd.GetStr(2) );
		pServerClient->SendGamePacket( pPacket, false );
	}
}

void CTestClient::CommandGameKickClient( const SCommand &cmd )
{
	if ( pServerClient )
		consoleCommandPackets.push_back( new CGameKickClient( 0, 0, cmd.GetInt( 0 ) ) );
}

void CTestClient::CommandDirectMsg( const SCommand &cmd )
{
	if ( pServerClient )
		pServerClient->SendPacket( new CTestDirectPacket( cmd.GetInt( 0 ) ) );
}

void CTestClient::CommandShowGameClients( const SCommand &cmd )
{
	if ( pServerClient )
		consoleCommandPackets.push_back( new CTestShowGameClients() );
}

void CTestClient::CommandSendSpecGameInfo( const SCommand &cmd )
{
	if ( pServerClient )
		consoleCommandPackets.push_back( new CTestSpecGameInfo( cmd.GetStr( 0 ) ) );
}

void CTestClient::CommandPauseServerConn( const SCommand &cmd )
{
	if ( pServerClient )
		pServerClient->TogglePauseServerConnection( cmd.GetInt( 0 ) );
}

void CTestClient::CommandPauseAccept( const SCommand &cmd )
{
	if ( pServerClient )
		pServerClient->TogglePauseAcceptGamers( cmd.GetInt( 0 ) );
}

void CTestClient::CommandPauseConnect( const SCommand &cmd )
{
	if ( pServerClient )
		pServerClient->TogglePauseConnectGamer( cmd.GetInt( 0 ), cmd.GetInt( 1 ) );
}

void CTestClient::CommandPauseClient( const SCommand &cmd )
{
	if ( pServerClient )
		pServerClient->TogglePause( cmd.GetInt( 0 ) );
}

void RandomizeList( list<int> *pList )
{
	hash_map< int, int > tempHash;
	int nSize = pList->size();
	int nMaxRand = nSize - 1;
	uint32_t dwTime = GetTickCount();
	for ( int i = 0; i < dwTime % 100; ++i )
		NRandom::Random( 0, nMaxRand );
	for( list<int>::iterator it = pList->begin(); it != pList->end(); ++it )
	{
		int nRandNum = NRandom::Random( 0, nMaxRand );
		while( tempHash.find( nRandNum ) != tempHash.end() )
		{
			nRandNum = NRandom::Random( 0, nMaxRand );
		}
		tempHash[ nRandNum ] = *it;
	}
	pList->clear();
	for( int i = nMaxRand; i >= 0; --i )
	{
		pList->push_back( tempHash[ i ] );
	}
}

void CTestClient::CommandLadderTest( const SCommand &cmd )
{
	if ( !pServerClient )
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "not connected yet\n" );
		return;
	}

	CPtr<CLadderInfoPacket> pPacket = new CLadderInfoPacket(); 
	{
		CFileStream stream( "../laddertest.xml", CFileStream::WIN_READ_ONLY );
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
		pSaver->Add( "LadderInfo", &(*pPacket) );
	}

	RandomizeList( &pPacket->maps );
	RandomizeList( &pPacket->techLevels );
	int nMapsSize = pPacket->maps.size();
	int nTechLevels = pPacket->techLevels.size();
	for ( int i = 0; i < nMapsSize / 2; ++i )
	{
		pPacket->maps.pop_front();
	}
	for ( int i = 0; i < nTechLevels / 2; ++i )
	{
		pPacket->techLevels.pop_front();
	}

	CPtr<CLadderInfoPacket> pPacketToSend = new CLadderInfoPacket( 0, pPacket->nSide, pPacket->bHistoricity,
		pPacket->nTeamSize, pPacket->techLevels, pPacket->maps, 0 );

#ifdef LADDER_TEST_LOG
	int nTime = GetTickCount();
	string szFileName = StrFmt( "../ladder_test_request_%d.xml", nTime ); 
	{
		CFileStream stream( CreateStream( szFileName.c_str(), STREAM_PATH_ABSOLUTE ) );
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_WRITE );

		pSaver->Add( "LadderInfo", &(*pPacketToSend) );
	}
#endif

	pServerClient->SendPacket( pPacketToSend );
}

extern list<int> ladderGameTeam1;
extern list<int> ladderGameTeam2;

void CTestClient::CommandLadderWin( const SCommand &cmd )
{
	if ( !pServerClient )
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "not connected yet\n" );
		return;
	}
	hash_set<int> winners;
	winners.clear();
	for ( int i = 1; i < cmd.params.size(); ++i )
	{
		int nID = cmd.GetInt( i );
		winners.insert( nID );
	}

	hash_map<int,int> races;
	races.clear();

	for ( list<int>::iterator it = ladderGameTeam1.begin(); it != ladderGameTeam1.end(); ++it )
	{
		races[*it] = NRandom::Random( 0, 3 );
	}
	for ( list<int>::iterator it = ladderGameTeam2.begin(); it != ladderGameTeam2.end(); ++it )
	{
		races[*it] = NRandom::Random( 0, 3 );
	}

	CPtr<CKillGamePacket> pPacket = new CKillGamePacket( 0, cmd.GetInt( 0 ) );
	CPtr<CLadderGameResultPacket> pResultPacket = new CLadderGameResultPacket( 0, cmd.GetInt( 0 ), winners, races );
	pServerClient->SendPacket( pPacket );
	pServerClient->SendPacket( pResultPacket );

#ifdef LADDER_TEST_LOG
	int nTime = GetTickCount();
	string szFileName = StrFmt( "../ladder_test_result_%d.xml", nTime ); 
	{
		CFileStream stream( CreateStream( szFileName.c_str(), STREAM_PATH_ABSOLUTE ) );
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_WRITE );

		pSaver->Add( "LadderInfo", &(*pResultPacket) );
	}
#endif
}

void CTestClient::CommandLadderInfo( const SCommand &cmd )
{
	CNetPacket *pPacket = new CLadderStatisticsRequestPacket( 0, cmd.GetStr( 0 ), true );
	pServerClient->SendPacket( pPacket );
}

void CTestClient::CommandForgotPassword( const SCommand &cmd )
{
	pServerClient = new CServerClient( szIP.c_str(), nNetVersion, nPort, 30 );

	CNetPacket *pPacket = new CForgottenPasswordPacket( cmd.GetStr( 0 ), cmd.GetStr( 1 ) );
	pServerClient->SendPacket( pPacket );

	Singleton<IConsoleBuffer>()->WriteASCII(
		CONSOLE_STREAM_CONSOLE,
		"Forgotten password request sent.\n"
		);

	pTestClientProcessor->SetServerClient( pServerClient );
}

void CTestClient::CommandMultiTestServer( const SCommand &cmd )
{
	testers.clear();
	nTestersNameShift = NStr::ToInt( cmd.params[1] );
	const int nNumberOfTesters = NStr::ToInt( cmd.params[0] );
	testers.resize( nNumberOfTesters, 0 );
}

void CTestClient::CommandPing( const SCommand &cmd )
{
	if ( !pServerClient )
	{
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "not connected yet\n" );
		return;
	}
	CNetPacket *pPacket = new CPingPacket( 0, pTestClientProcessor->GetMyID(), GetTickCount() );
	pServerClient->SendPacket( pPacket );
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, "Ping sent.\n"	);

}

void CTestClient::ProcessCommands()
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

		pTestClientProcessor->ProcessPacket( pPacket );
	}
}

void CTestClient::Segment()
{
	ProcessCommands();

	if ( pServerClient )
	{
		pTestClientProcessor->Segment();
		pServerClient->Segment();

		while ( CPtr<CNetPacket> pPacket = pServerClient->GetPacket() )
			pTestClientProcessor->ProcessPacket( pPacket );
	}

	if ( pPinger )
		pPinger->Segment();

	bool bCanCreateTester = ( GetTickCount() > ( TESTER_CREATION_TIMEOUT + dwLastTesterCreationTime ) );
	if ( !testers.empty() ) 
	{
		for ( int i = 0; i < testers.size(); ++i )
		{
			if ( testers[i] && !testers[i]->IsCancelled() )	
			{
				testers[i]->Segment();
			}
			else if ( bCanCreateTester )
			{
				string szName = StrFmt( "%d", i + nTestersNameShift );
				testers[i] = CreateMultiTester();
				testers[i]->Init( szIP, nNetVersion, nPort, 30, szName, szName, szName, MTM_LADDER | MTM_CHAT );
				testers[i]->Segment();
				bCanCreateTester = false;
				dwLastTesterCreationTime = GetTickCount();
			}
		}
	}

}

#undef REGISTER_CMD_FUNC

