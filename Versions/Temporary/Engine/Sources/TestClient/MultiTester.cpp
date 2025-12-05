#include "stdafx.h"
#include "MultiTester.hpp"
#include "Client/ServerClient.h"
#include "Server_Client_Common/NetPacket.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/LoginPackets.h"
#include "Server_Client_Common/LobbiesIDs.h"
#include "Server_Client_Common/LadderLobbyPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Misc/StrProc.h"
#include "Misc/Win32Random.h"

enum EMultiTesterStage
{
	LOGIN = 0,
	ENTER_LOBBY = 1,
	ENTER_CHAT = 2,
	MAIN_STAGE = 3,
	CANCELLED = 255
};

enum EMultiTesterStatus
{
	ACTIVE = 0,
	WAITING_FOR_RESPONSE = 1
};

const int CHAT_CHANNELS_NUMBER = 10;
const int CHAT_CHANNEL_CHANGE_PERIOD = 20000;
const int CHAT_MESSAGE_SEND_PERIOD = 3000;
const int SEGMENT_DURATION = 1000;
const int GAME_LENGTH = 200000;

CMultiTester::CMultiTester()
{
	REGISTER_PACKET_PROCESSOR( &CMultiTester::ProcessConnectServerResult )
	REGISTER_PACKET_PROCESSOR( &CMultiTester::ProcessChatChannelClientsListPacket )
	REGISTER_PACKET_PROCESSOR( &CMultiTester::ProcessMyIDPacket )
	REGISTER_PACKET_PROCESSOR( &CMultiTester::ProcessChatPacket )
	REGISTER_PACKET_PROCESSOR( &CMultiTester::ProcessEnteredLobby )
	REGISTER_PACKET_PROCESSOR( &CMultiTester::ProcessClientRemoved )
	REGISTER_PACKET_PROCESSOR( &CMultiTester::ProcessLadderInvitePacket )
	REGISTER_PACKET_PROCESSOR( &CMultiTester::ProcessGameKilled )
	REGISTER_PACKET_PROCESSOR( &CMultiTester::ProcessNewGameClient )
}

void CMultiTester::Init( const std::string& _szServerAddress, const int _nNetVersion, const int _nServerPort, const int _nTimeOut,
												 const std::string &_szName, const std::string &_szPassword, const std::string &_szCDKey, const int _nTestMode )
{
	nTestMode = _nTestMode;
	szServerAddress = _szServerAddress;
	nNetVersion = _nNetVersion;
	nServerPort = _nServerPort;
	nTimeOut = _nTimeOut;
	szName = _szName;
	szPassword = _szPassword;
	szCDKey = _szCDKey;
	nStage = LOGIN;
	nStatus = ACTIVE;
	bCancelled = false;
	dwLastChatChannelChangeTime = GetTickCount();
	dwLastChatMessageTime = GetTickCount();
	dwLastSegmentTime = GetTickCount();
	dwGameStartTime = GetTickCount();
	dwLastGameHeartBeatTime = GetTickCount();
	bLadderInfoSend = false;
	bIsInGame = false;
	pServerClient = new CServerClient( szServerAddress.c_str(), nNetVersion, nServerPort, nTimeOut );
}

bool CMultiTester::IsActive() const
{
	return nStatus == ACTIVE;
}

void CMultiTester::WaitForResponse()
{
	nStatus = WAITING_FOR_RESPONSE;
}

void CMultiTester::Activate()
{
	nStatus = ACTIVE;
}

bool CMultiTester::Segment()
{
  if ( bCancelled || !pServerClient )
		return false;
	
	pServerClient->Segment();

	CPtr<CNetPacket> pPacket;
	while ( !bCancelled && ( pPacket = pServerClient->GetPacket() ) )
	{
		ProcessPacket( pPacket );
	}

	if ( bCancelled )
		return false;

	// В режиме ожидания ответа сервера только обрабатываем пакеты
	DWORD dwTime = GetTickCount();
	if ( !IsActive() )
		return true;

	switch( nStage )
	{
	case LOGIN:
		LoginStage();
		break;
	case ENTER_LOBBY:
		EnterLobbyStage();
		break;
	case ENTER_CHAT:
		EnterChatStage();
		break;
	case MAIN_STAGE:
		MainStage();
		break;
	}
	return true;
}

void CMultiTester::LoginStage()
{
	pServerClient->SendPacket( new CLoginPacket( 0, szName, szPassword ) );
	WaitForResponse();
}

bool CMultiTester::ProcessConnectServerResult( CConnectServerPacket *pPacket )
{
	if ( pPacket->eConnectionState == CConnectServerPacket::ECS_SUCCESS )
	{
		nStage = ENTER_LOBBY;
		Activate();
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "%s logged in\n", szName.c_str() ) );
		return true;
	}
	Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "%s login failed from client id=%d, reason=%d\n", szName.c_str(), pPacket->nClientID, pPacket->eRejectReason ) );
	bCancelled = true;
	return true;
}

void CMultiTester::EnterLobbyStage()
{
	pServerClient->SendPacket( new CEnterLobbyPacket( 0, ERID_LADDER ) );
	WaitForResponse();
}

bool CMultiTester::ProcessEnteredLobby( CEnterLobbyPacket *pPacket )
{
	Activate();
	nStage = ENTER_CHAT;
	return true;
}

void CMultiTester::EnterChatStage()
{
	ChangeChatChannel( "TEST_CHAT_CHANNEL_0" );
	nStage = MAIN_STAGE;
}

void CMultiTester::ChangeChatChannel( const std::string &szChannelName )
{
	pServerClient->SendPacket( new CChatChannelPacket( 0, szChannelName ) );
	dwLastChatChannelChangeTime = GetTickCount();
}

void CMultiTester::SendChatMessage( const std::string &szMessage )
{
	pServerClient->SendPacket( new CChatPacket( 0, NStr::ToUnicode( szMessage ), "", 0, true ) );
	dwLastChatMessageTime = GetTickCount();
}

bool CMultiTester::ProcessChatPacket( CChatPacket *pPacket )
{
	return true;
}

bool CMultiTester::ProcessChatChannelClientsListPacket( CChatChannelClientsListPacket *pPacket )
{
	return true;
}

bool CMultiTester::ProcessClientRemoved( CNetRemoveClient *pPacket )
{
	if ( pPacket->nClientID == 0 )
	{	
		Singleton<IConsoleBuffer>()->WriteASCII( CONSOLE_STREAM_CONSOLE, StrFmt( "%s connection lost\n", szName.c_str() ) );
		bCancelled = true;
		pServerClient = 0;
	}
	return true;
}

void CMultiTester::MainStage()
{
	DWORD dwTime = GetTickCount();
	if ( dwTime < dwLastSegmentTime + SEGMENT_DURATION )
		return;
	dwLastSegmentTime = dwTime;
	if ( nTestMode & MTM_CHAT )
	{
		TestChat();
	}
	if ( nTestMode & MTM_LADDER )
	{
		TestLadder();
	}
}

void CMultiTester::TestChat()
{
	DWORD dwTime = GetTickCount();
	if ( dwTime > CHAT_CHANNEL_CHANGE_PERIOD + dwLastChatChannelChangeTime && ( NWin32Random::Random( 0, 10 ) == 0 ) )
	{
		std::string szChannelName = StrFmt( "TEST_CHAT_CHANNEL_%d", NWin32Random::Random( 0, CHAT_CHANNELS_NUMBER ) );
		ChangeChatChannel( szChannelName );
		return;
	}

	if ( dwTime > CHAT_MESSAGE_SEND_PERIOD + dwLastChatMessageTime && ( NWin32Random::Random( 0, 10 ) == 0 ) )
	{
		std::string szMessage = "В чащах юга жил был цитрус. Но фальшивый экземпляр.";
		SendChatMessage( szMessage );
		return;
	}
}

void RandomizeList( std::list<int> *pList );

void CMultiTester::TestLadder()
{
	if ( !bLadderInfoSend && !bIsInGame )
	{
		std::list<int> maps;
		for ( int i = 0; i < 10; ++i )
		{
			maps.push_back( i );
		}
		RandomizeList( &maps );
		maps.pop_front();
		maps.pop_front();
		maps.pop_front();
		maps.pop_front();

		std::list<int> techLevels;
		for ( int i = 0; i < 4; ++i )
		{
			techLevels.push_back( i );
		}
		RandomizeList( &techLevels );
		techLevels.pop_front();
		techLevels.pop_front();
		pServerClient->SendPacket( new CLadderInfoPacket( 0, NWin32Random::Random( 0, 2 ), NWin32Random::Random( 0, 2 ) == 1,
			NWin32Random::Random( 1, 4 ), techLevels, maps, 12345 ) );
		bLadderInfoSend = true;
		return;
	}
	if ( bIsInGame )
	{
		const DWORD dwTime = GetTickCount();
		if ( dwTime > 5000 + dwLastGameHeartBeatTime )
		{
			pServerClient->SendPacket( new CGameHeartBeatPacket( 0, nGameID ) );
			dwLastGameHeartBeatTime = dwTime;
		}
		if ( dwTime > GAME_LENGTH + dwGameStartTime )
		{
			pServerClient->SendPacket( new CKillGamePacket( 0, nGameID ) );
			bIsInGame = false;
		}
	}
}

bool CMultiTester::ProcessLadderInvitePacket( CLadderInvitePacket *pPacket )
{
	nGameID = pPacket->nGameID;
	const int nMapID = pPacket->nMapID;
	const int nTechLevel = pPacket->nTechLevel;
	playersToWait.clear();
	int nWinTeam = ( nGameID + nMapID + nTechLevel ) % 2;
	winnersSet.clear();
	playersRaces.clear();

	for ( std::list<int>::const_iterator it = pPacket->team1.begin(); it != pPacket->team1.end(); ++it )
	{
		const int nPlayerID = *it;
		if ( nPlayerID != nMyID )
			playersToWait.insert( nPlayerID );
		if ( nWinTeam == 0 )
			winnersSet.insert( nPlayerID );
		playersRaces[nPlayerID] = 0;
	}
	for ( std::list<int>::const_iterator it = pPacket->team2.begin(); it != pPacket->team2.end(); ++it )
	{
		const int nPlayerID = *it;
		if ( nPlayerID != nMyID )
			playersToWait.insert( nPlayerID );
		if ( nWinTeam == 1 )
			winnersSet.insert( nPlayerID );
		playersRaces[nPlayerID] = 1;
	}

	pServerClient->SendPacket( new CConnectGamePacket( 0, nGameID, "" ) );
	bIsInGame = true;
	bLadderInfoSend = false;
	dwGameStartTime = GetTickCount() - NWin32Random::Random( 0, 60000 );
	return true;
}

bool CMultiTester::ProcessNewGameClient( CNewGameClient *pPacket )
{
	playersToWait.erase( pPacket->nClientID );

  if ( playersToWait.empty() )
	{
		std::unordered_map<int, std::vector<int> >	units;
		for ( std::unordered_map<int,int>::iterator it = playersRaces.begin(); it != playersRaces.end(); ++it )
		{
			const int nPlayerID = it->first;
			units[nPlayerID] = std::vector<int>( 20, 1 );
		}
		pServerClient->SendPacket( new CLadderGameResultPacket( 0, nGameID, winnersSet, playersRaces, units ) );
	}
	return true;
}

bool CMultiTester::ProcessGameKilled( CGameKilled *pPacket )
{
	if ( nGameID == pPacket->nGame )
	{
		bIsInGame = false;
	}
	return true;
}

bool CMultiTester::ProcessMyIDPacket( CMyIDPacket *pPacket )
{
	nMyID = pPacket->nMyID;
	return true;
}

IMultiTester *CreateMultiTester()
{
	return new CMultiTester();
}



