#include "stdafx.h"

#include <fmt/format.h>

#include "MPManagerModeLAN.hpp"

#include "Misc/StrProc.h"
#include "MPTransceiver.h"
#include "Server_Client_Common/ChatPackets.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "DBMPConsts.h"
#include "GetConsts.h"
#include "InterfaceState.h"
#include "System/Text.h"
#include "System/Commands.h"
#include "DBConsts.h"
#include "MPPacketTraceLog.h"

#include "Misc/Time64.h"
#include "MPLANTest.h"

#include "GameX_export.h"

#include <cstdint>

#define SERVER_DEAD_TIMEOUT 5000
#define LAN_PORT 4201
#define LAN_TIMEOUT_SEC 20

UINT64 nAsyncTime;
const uint32_t ASYNC_EXIT_TIMEOUT = 20000;

REGISTER_SAVELOAD_CLASS( GAMEX, 0x19243482, CMPManagerModeLAN );

class CLANPasswordPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CLANPasswordPacket );
public:
	ZDATA
	std::string szPassword;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&szPassword); return 0; }

	CLANPasswordPacket() { }
	CLANPasswordPacket( const int nClientID, const std::string &_szPassword ) : CNetPacket( nClientID ), szPassword( _szPassword ) { }
};
REGISTER_SAVELOAD_CLASS( GAMEX, 0x19256440, CLANPasswordPacket );

class CRequestLANPasswordPacket : public CNetPacket
{
	OBJECT_NOCOPY_METHODS( CRequestLANPasswordPacket );
public:
	ZDATA
	ZEND int operator&( IBinSaver &f ) { return 0; }

	CRequestLANPasswordPacket() { }
	CRequestLANPasswordPacket( const int nClientID ) : CNetPacket( nClientID ) { }
};
REGISTER_SAVELOAD_CLASS( GAMEX, 0x19256441, CRequestLANPasswordPacket );


CMPManagerModeLAN::CMPManagerModeLAN() :
wGameUniqueID(0), eCurrentState( EGS_NOT_IN_GAME )
{
	pLANClient = CLANClient::Create( NGlobal::GetVar( "NetGameVersion", 1 ), LAN_PORT, LAN_TIMEOUT_SEC );
	if ( !pLANClient )
	{
		OnCriticalNetworkError( InterfaceState()->GetTextEntry( "T_NET_DRIVER_CRITICAL_ERROR" ) );
		return;
	}
	pClient = pLANClient;

	// Messages
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_CHAT_MESSAGE, SMPUIChatMessage, &CMPManagerModeLAN::OnChatMessage );

	// Packets
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeLAN::OnNetNewClient );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeLAN::OnConnectGameFailed );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeLAN::OnGameClientWasKicked );

	REGISTER_PACKET_PROCESSOR( &CMPManagerModeLAN::OnLANPassword );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeLAN::OnRequestLANPassword );
}

bool CMPManagerModeLAN::Segment()
{
	if ( pLanTester ) 
	{
		if ( !pLanTester->IsStarted() )
		{
			pLanTester->Start();
		}
		else
		{
			if ( NGlobal::GetVar( "MP_async", 0 ) == 2 )
			{
				nAsyncTime = GetLongTickCount();
				NGlobal::SetVar( "MP_async", 1 );
			}
			else if( NGlobal::GetVar( "MP_async", 0 ) == 1 && GetLongTickCount() > nAsyncTime + ASYNC_EXIT_TIMEOUT )
			{
				pLanTester->AsyncDetected();
			}
		}
	}

	return CMPManagerMode::Segment();
}

void CMPManagerModeLAN::UpdateGameList()
{
	if ( eCurrentState != EGS_NOT_IN_GAME )
		return;

	NTimer::STime curTime = Singleton<IGameTimer>()->GetAbsTime();
	NNet::IDriver::SGameInfo gameInfo;
	NNet::CNodeAddress gameNode;
	bool bWrongVersion;
	float fPing;
	int nGame = 0;				// counts games for GetGameInfo()
	SMPUIGameListMessage *pGamesMsg = new SMPUIGameListMessage;

	while ( pLANClient->GetGameInfo( nGame++, &gameNode, &bWrongVersion, &fPing, &gameInfo ) )
	{
		CNetGameList::iterator iter = games.begin();
		while ( iter != games.end() && !gameNode.SameIP( iter->lanNode ) )
			++iter;

		if ( iter == games.end() )			// new server
		{
			SNetGameInfo &game = games.emplace_back();

			game.nGameID = ++wGameUniqueID;
			game.fPing = fPing * 1000;
			game.lanNode = gameNode;
			game.lastSeen = curTime;
			ExtractGameInfo( &game, &gameInfo );
			AddGameInfoForUI( &(pGamesMsg->gamesAddChange), game );
			if ( pLanTester )
				pLanTester->NewGameFound( game.nGameID, game.szSessionName );
		}
		else														// old server information refresh
		{
			// information was refreshed
			SNetGameInfo &game = *iter;

			game.fPing = fPing * 1000;
			game.lastSeen = curTime;
			ExtractGameInfo( &game, &gameInfo );
			AddGameInfoForUI( &(pGamesMsg->gamesAddChange), game );
		}
	}

	// Remove dead servers
	for ( CNetGameList::iterator it = games.begin(); it != games.end(); )
	{
		SNetGameInfo &game = *it;
		if ( curTime - game.lastSeen > NGlobal::GetVar( "lan_server_timeout", SERVER_DEAD_TIMEOUT ) )
		{
			it = games.erase( it );
			pGamesMsg->gamesRemoved.push_back( game.nGameID );
			continue;
		}
		++it;
	}
	PushMessage( pGamesMsg );
}


//
//   Messages
//


bool CMPManagerModeLAN::OnChatMessage( SMPUIChatMessage *pMsg )
{
	std::wstring wszFilteredText = InterfaceState()->FilterMPChatText( pMsg->wszText );
	
	CPtr<CChatPacket> pPacket = new CChatPacket( 0, wszFilteredText, "", 0, true );

	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( i == nOwnSlot || !IsPlayerPresent( i ) || slots[i].nClientID < 0 )
			continue;

		pPacket->nClientID = slots[i].nClientID;
		pPacket->szNick = szMPName;
		pLANClient->SendPacket( pPacket );
	}

	// Report it back
	PushMessage( new SMPUIChatMessage( szMPName, wszFilteredText ) );
	return true;
}


//
//   Packets
//


bool CMPManagerModeLAN::OnNetNewClient( class CNetNewClient *pPacket )
{
	CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW, "NEW LAN CLIENT" );

	if ( eCurrentState == EGS_HOST )
	{
		if ( !szPassword.empty() )
		{
			pendingClients.push_back( pPacket->nClientID );
			pLANClient->SendPacket( new CRequestLANPasswordPacket( pPacket->nClientID ) );
			return true;
		}

		if ( !HostAssignNewClient( pPacket->nClientID ) )
		{
			pLANClient->Kick( pPacket->nClientID );
			return true;
		}
		CompileAndSendLANServerInfo();
	}
	else if ( eCurrentState == EGS_CLIENT )
	{
		ClientAssignNewClient( pPacket->nClientID );
	}
	else if ( eCurrentState == EGS_JOINING )
	{
		JoiningClientAssignNewClient( pPacket->nClientID );
	}

	return true;
}

bool CMPManagerModeLAN::OnConnectGameFailed( class CConnectGameFailed *pPacket )
{
	if ( eCurrentState != EGS_JOINING )
		return true;

	CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW, "++ CONNECT FAILED ++" );

	PushMessage( new SMPUIGameRoomInitMessage( SMPUIGameRoomInitMessage::ERR_CONNECT_FAILED ) );
	OnLeaveGame();

	return true;
}

bool CMPManagerModeLAN::OnGameClientWasKicked( class CGameClientWasKicked *pPacket )
{
	//DebugTrace( "+++ ClientWasKicked packet from client %d", pPacket->nClientID );
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"RX",
		"CGameClientWasKicked",
		pPacket->nClientID,
		fmt::format( "kicked={} in_room={}", pPacket->nKicked, IsInGameRoom() ? 1 : 0 ) );
	if ( IsInGameRoom() )
	{
		NGameX::MatchPacketTrace_SetFinalState( GetPresentMask(), dwLaggers, IsValid( pTransceiver ) ? pTransceiver->GetPlayerMask() : 0 );
		NGameX::MatchPacketTrace_Flush( "kicked" );
		PushMessage( new SMPUIGameRoomInitMessage( SMPUIGameRoomInitMessage::ERR_KICKED ) );
		OnLeaveGame();
	}

	return true;
}

bool CMPManagerModeLAN::OnRequestLANPassword( class CRequestLANPasswordPacket *pPacket )
{
	if ( eCurrentState != EGS_JOINING )
		return true;

	pLANClient->SendPacket( new CLANPasswordPacket( pPacket->nClientID, szPassword ) );
	return true;
}

bool CMPManagerModeLAN::OnLANPassword( class CLANPasswordPacket *pPacket )
{
	if ( eCurrentState != EGS_HOST )
		return true;

	for ( std::list<int>::iterator it = pendingClients.begin(); it != pendingClients.end(); ++it )
	{
		if ( *it = pPacket->nClientID )
		{
			if ( pPacket->szPassword == szPassword )
			{
				if ( HostAssignNewClient( pPacket->nClientID ) )
					CompileAndSendLANServerInfo();
				else
					pLANClient->Kick( pPacket->nClientID );
			}
			else
			{
				CConnectGameFailed *pFailPkt = new CConnectGameFailed( pPacket->nClientID, CConnectGameFailed::ER_WRONG_PASSWORD );
				pClient->SendPacket( pFailPkt );
			}
			pendingClients.erase( it );
			break;
		}
	}
	return true;
}

void CMPManagerModeLAN::OnSetMySlotNumber()
{
	if ( pLanTester )
		pLanTester->SetMySlot( nOwnSlot );

	eCurrentState = EGS_CLIENT;
}



void CMPManagerModeLAN::TryToCreateGame()
{
	eCurrentState = EGS_HOST;
	pLANClient->StartGame();
	CompileAndSendLANServerInfo();

	SendGameRoomInitMessages();
}

void CMPManagerModeLAN::TryToJoinGame( const SNetGameInfo &game )
{
	pLANClient->ConnectGame( game.lanNode );

	eCurrentState = EGS_JOINING;
}

void CMPManagerModeLAN::StartGame()
{
	if ( IsGameHost() )
	{
		pLANClient->StopGameInfoSend();
		pLANClient->StopNewPlayerAccept();
	}
	eCurrentState = EGS_GAME_STARTED;
	CMPManagerMode::StartGame();
}

void CMPManagerModeLAN::EndGame()
{
	if ( pLanTester )
		pLanTester->EndGame();
	StartNewLANDriver();
	if ( !pLANClient )
	{
		OnCriticalNetworkError( InterfaceState()->GetTextEntry( "T_NET_DRIVER_CRITICAL_ERROR" ) );
		return;
	}
	if ( eCurrentState == EGS_GAME_STARTED )
		PushMessage( new SMPUIGameAftemathMessage( false ) );
	eCurrentState = EGS_NOT_IN_GAME;
	CMPManagerMode::EndGame();
}

void CMPManagerModeLAN::StartNewLANDriver()
{
	pLANClient = 0;
	pClient = 0;
	pLANClient = CLANClient::Create( NGlobal::GetVar( "NetGameVersion", 1 ), LAN_PORT, LAN_TIMEOUT_SEC );
	pClient = pLANClient;
}

void CMPManagerModeLAN::CompileAndSendLANServerInfo()
{
	if ( !gameDesc.pMPMap || eCurrentState != EGS_HOST )
		return;

	pLANClient->StopGameInfoSend();

	NNet::IDriver::SGameInfo netInfo;
	netInfo.wszServerName = NStr::ToUnicode( szSessionName );
	netInfo.nCurPlayers = nSlotsUsed;
	netInfo.nMaxPlayers = gameDesc.nPlayers;
	netInfo.bPasswordRequired = ( szPassword.length() != 0 );
	netInfo.nHostPort = LAN_PORT;
	netInfo.wszMapName = GET_TEXT_PRE( gameDesc.pMPMap->, MapName );
	SGameSettings miscInfo;
	miscInfo.pMPMap = gameDesc.pMPMap;
	miscInfo.nGameType = gameDesc.eType;
	miscInfo.nTechLevel = gameDesc.nTechLevel;
	{
		CPtr<IBinSaver> pSaver = CreateBinSaver( &netInfo.gameSettings, SAVER_MODE_WRITE );
		pSaver->Add( 1, &miscInfo );
	}
	pLANClient->StartGameInfoSend( netInfo );
}

void CMPManagerModeLAN::OnLeaveGame()
{
	if ( eCurrentState == EGS_NOT_IN_GAME )
		return;

	eCurrentState = EGS_NOT_IN_GAME;
	StartNewLANDriver();
	if ( !pLANClient )
	{
		OnCriticalNetworkError( InterfaceState()->GetTextEntry( "T_NET_DRIVER_CRITICAL_ERROR" ) );
		return;
	}
}

void CMPManagerModeLAN::OnGameRoomClientAdded()
{
	if ( IsGameHost() )
		CompileAndSendLANServerInfo();
}

void CMPManagerModeLAN::OnGameRoomClientRemoved()
{
	if ( IsGameHost() )
		CompileAndSendLANServerInfo();
}

void CMPManagerModeLAN::OnGameSpecificInfo()
{
	// Check if this client has the map or not? - only when the player got his slot first
	if ( gameDesc.nPlayers < 0 )
	{
		PushMessage(new SMPUIGameRoomInitMessage(SMPUIGameRoomInitMessage::ERR_CHECKSUM));
		OnLeaveGame();
		return;
	}

	ulGameCheckSum = NGameX::GetGameConsts()->GetMPDataVersionChecksumWithMap( gameDesc.pMPMap );
	if ( ulHostCheckSum != ulGameCheckSum )
	{
		DebugTrace( "+++ Checksums differ: host %d, me %d", ulHostCheckSum, ulGameCheckSum );
		PushMessage( new SMPUIGameRoomInitMessage( SMPUIGameRoomInitMessage::ERR_CHECKSUM ) );
		OnLeaveGame();
		return;
	}

	SendGameRoomInitMessages();
}

void CMPManagerModeLAN::KickPlayerFromSlot( const int nSlot )
{
	if ( IsInGameRoom() && IsGameHost() )
	{
		CPtr<CGameClientWasKicked> pKickPkt = new CGameClientWasKicked( slots[nSlot].nClientID, slots[nSlot].nClientID );
		NGameX::MatchPacketTrace_Log(
			IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
			"TX",
			"CGameClientWasKicked",
			GetOwnClientID(),
			fmt::format( "slot={} kicked_client={}", nSlot, slots[nSlot].nClientID ) );
		pLANClient->SendPacket( pKickPkt );

		//pLANClient->Kick( slots[nSlot].nClientID );
	}
}

void CMPManagerModeLAN::ExtractGameInfo( SNetGameInfo *pDst, NNet::IDriver::SGameInfo *pSrc )
{
	pDst->szSessionName = NStr::ToMBCS( pSrc->wszServerName );
	pDst->nPlayers = pSrc->nCurPlayers;
	pDst->nMaxPlayers = pSrc->nMaxPlayers;
	pDst->bPasswordRequired = pSrc->bPasswordRequired;
	SGameSettings miscInfo;
	{
		pSrc->gameSettings.Seek( 0 );
		CPtr<IBinSaver> pSaver = CreateBinSaver( &pSrc->gameSettings, SAVER_MODE_READ );
		pSaver->Add( 1, &miscInfo );
	}
	pDst->nGameType = miscInfo.nGameType;
	pDst->nTechLevel = miscInfo.nTechLevel;
	if ( !miscInfo.pMPMap )
	{
		pDst->szMapName = "??????????";
		pDst->nSizeX = 0;
		pDst->nSizeY = 0;
	}
	else
	{
		pDst->szMapName = NStr::ToMBCS( GET_TEXT_PRE( miscInfo.pMPMap->, MapName ) );
		pDst->nSizeX = miscInfo.pMPMap->nSizeX;
		pDst->nSizeY = miscInfo.pMPMap->nSizeY;
	}
}


