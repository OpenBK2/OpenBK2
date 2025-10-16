#include "stdafx.h"

#include "MPManagerModeNivalNet.hpp"
#include "Server_Client_Common/CommonPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/LoginPackets.h"
#include "Server_Client_Common/GameInfo.h"
#include "Server_Client_Common/LobbiesIDs.h"
#include "Server_Client_Common/Net.h"
#include "Server_Client_Common/LadderLobbyPackets.h"
#include "Client/ServerClient.h"
#include "Misc/StrProc.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "MultiplayerNetPackets.h"
#include "System/Commands.h"
#include "InterfaceState.h"
#include "GameXClassIDs.h"
#include "Main/MainLoop.h"
#include "ScenarioTracker.h"
#include "DBMPConsts.h"
#include "GetConsts.h"
#include "System/Text.h"
#include "DBGameRoot.h"
#include "Misc/Win32Random.h"
#include "InterfaceMisc.h"
#include "MultiplayerTestNet.h"
#include "DBConsts.h"
#include "MPPacketTraceLog.h"
#include "MPTransceiver.h"

#include "GameX_export.h"

#include <fmt/format.h>

const char* NIVAL_NET_IP = "localhost";
const int NIVAL_NET_PORT = 4200;

#define HEARTBEAT_PERIOD 5000
#define GAMES_REFRESH_PERIOD 5000
static std::wstring s_wszStoredLogin = L"";
static std::wstring s_wszStoredPassword = L"";

REGISTER_SAVELOAD_CLASS( GAMEX, 0x192444C0, CMPManagerModeNivalNet );
START_REGISTER(NivalNetGVars)
// This GVar is here to force it to be saved in user.cfg
// Do not remove, please
REGISTER_VAR_EX( "Multiplayer.NivalNet.StoredLogin", NGlobal::VarWStrHandler, &s_wszStoredLogin, L"", STORAGE_USER );
REGISTER_VAR_EX( "Multiplayer.NivalNet.StoredPassword", NGlobal::VarWStrHandler, &s_wszStoredPassword, L"", STORAGE_USER );
FINISH_REGISTER

CMPManagerModeNivalNet::CMPManagerModeNivalNet() :
nMyClientID( -1 ), eState( EGS_LOGGING_IN ), bLadderGame( false ), timeNextGameHeartbeat( 0 )
{
	// Messages
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_LOGIN_NIVAL_NET, SMPUILoginNivalNetMessage, &CMPManagerModeNivalNet::OnLoginNivalNetMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_REGISTER_NIVAL_NET, SMPUIRegisterMessage, &CMPManagerModeNivalNet::OnRegisterNivalNetMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_CHAT_MESSAGE, SMPUIChatMessage, &CMPManagerModeNivalNet::OnChatMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_NIVAL_NET_CUSTOM_GAME, SMPUIMessage, &CMPManagerModeNivalNet::OnCustomGameMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_NIVAL_NET_LADDER_GAME, SMPUILadderGameMessage, &CMPManagerModeNivalNet::OnSetupLadderGameMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_NIVAL_NET_CANCEL_LADDER, SMPUIMessage, &CMPManagerModeNivalNet::OnCancelLadderMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_BACK_FROM_GAME_LIST, SMPUIMessage, &CMPManagerModeNivalNet::OnBackFromGameListMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_JOIN_CHAT_CHANNEL, SMPUIJoinChannelMessage, &CMPManagerModeNivalNet::OnJoinChatChannelMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_REQUEST_CHAT_CHANNELS, SMPUIMessage, &CMPManagerModeNivalNet::OnRequestChatChannelsMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_REQUEST_INFO, SMPUILadderInfoRequestMessage, &CMPManagerModeNivalNet::OnLadderInfoRequestMessage );
	REGISTER_MPUI_MESSAGE_HANDLER( EMUI_CHANGE_FRIEND_IGNORE, SMPUIChangeFriendIgnoreStatusMessage, &CMPManagerModeNivalNet::OnChangeFriendIgnoreStatusMessage );

	// Packets
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnLobbyGamesPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnMyIDPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnConnectServerPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnConnectGameFailed );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnConnectedGameIDPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnNewGameClientPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnEnterLobbyPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnChatChannelsListPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnChatChannelClientsPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnChatChannelClientNotifyPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnGameClientWasKicked );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnGameClientDeadPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnNetRemoveClientPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnChatIgnoreFriendListPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnChatFriendNotifyPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnChatAFKResponsePacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnLadderStatisticsPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnLadderInvalidStatisticsPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnLadderShortStatisticsPacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnLadderInvitePacket );
	REGISTER_PACKET_PROCESSOR( &CMPManagerModeNivalNet::OnSystemBroadcastPacket );

	updateChannels.bUpdating = false;
	bGotMyLadderStats = false;
	bUIAskedStats = false;
	bAwaitingLadderStatsChange = false;
	gamesUpdate.bUpdating = false;
	gamesUpdate.timeUpdatePeriod = GAMES_REFRESH_PERIOD;
}

void CMPManagerModeNivalNet::UpdateGameList()
{
	if ( eState != EGS_CUSTOM_GAME )
		return;

	CPtr<CGetLobbyGamesPacket> pPkt = new CGetLobbyGamesPacket( 0, gamesUpdate.dwVersion );
	pClient->SendPacket( pPkt );
}

void CMPManagerModeNivalNet::TryToCreateGame()
{
	eState = EGS_JOINING;
	bHost = true;
	nGameID = -1;
	SGameInfo *pGameInfo = new SGameInfo;
	SetServerGameInfo( pGameInfo );

	CPtr<CCreateGamePacket> pCreatePkt = new CCreateGamePacket( 0, pGameInfo );
	pClient->SendPacket( pCreatePkt );
	bLadderGame = false;
}

void CMPManagerModeNivalNet::TryToJoinGame( const SNetGameInfo &game )
{
	nGameID = -1;
	gameDesc.pMPMap = 0;
	CPtr<CConnectGamePacket> pConnectPkt = new CConnectGamePacket( 0, game.nGameID, szPassword );
	pClient->SendPacket( pConnectPkt );

	eState = EGS_JOINING;
	bHost = false;
	bLadderGame = false;
}

void CMPManagerModeNivalNet::OnSetMySlotNumber()
{
	pClient->SendGamePacket( new CB2SlotInfoPacket( nHostClientID, nOwnSlot, slots[nOwnSlot] ), false );
	CheckJoinGameConditions();
}

void CMPManagerModeNivalNet::CheckJoinGameConditions()
{
	if ( eState != EGS_JOINING )
		return;
	std::string szDebugOut = "+++ CheckJoinConditions: ";
	if ( nGameID == -1 )
		szDebugOut += "no_GameID ";
	if ( nOwnSlot == -1 )
		szDebugOut += "no_OwnSlot ";
	if ( !gameDesc.pMPMap )
		szDebugOut += "no_Map ";

	 if ( gameDesc.nPlayers < 0 )
	 {
		DebugTrace( "Client has not MP map of the game room" );
		PushMessage( new SMPUIGameRoomInitMessage( SMPUIGameRoomInitMessage::ERR_CHECKSUM ) );
		OnLeaveGame();
		return;
	 }

	if ( nGameID != -1 && nOwnSlot != -1 && gameDesc.pMPMap )
	{
		ulGameCheckSum = NGameX::GetGameConsts()->GetMPDataVersionChecksumWithMap( gameDesc.pMPMap );
		if ( ulHostCheckSum != ulGameCheckSum )
		{
			DebugTrace( "+++ Checksums differ: host %d, me %d", ulHostCheckSum, ulGameCheckSum );
			PushMessage( new SMPUIGameRoomInitMessage( SMPUIGameRoomInitMessage::ERR_CHECKSUM ) );
			OnLeaveGame();
			return;
		}
		if ( slots[0].wConnectedTo == slots[nOwnSlot].wConnectedTo )
		{
			eState = EGS_GAME_ROOM;
			SendGameRoomInitMessages();
		}
		else
		{
			szDebugOut += fmt::format( "Connect( {} != {} )", slots[0].wConnectedTo, slots[nOwnSlot].wConnectedTo );
		}
	}
	DebugTrace( szDebugOut.c_str() );
}

void CMPManagerModeNivalNet::OnLeaveGame()
{
	// Send LeaveGame
	if ( IsInGameRoom() && nGameID != -1 )
	{
		pClient->SendPacket( new CLeaveGamePacket( 0, nGameID ) );
		if ( IsGameHost() )
			pClient->SendPacket( new CKillGamePacket( 0, nGameID ) );
	}
	eState = EGS_CUSTOM_GAME;
}

bool CMPManagerModeNivalNet::ShouldSendHeartbeatNow()
{
	return ( ( IsGameRunning() || IsInGameRoom() || eState == EGS_LADDER_WAIT_CLIENTS ) 
		&& nGameID >= 0 && ( Singleton<IGameTimer>()->GetAbsTime() > timeNextGameHeartbeat ) );
}

void CMPManagerModeNivalNet::RefreshServerGameKeepalive( bool bUpdateGameInfo, const char *szReason, int nRemovedClientID )
{
	if ( nGameID < 0 )
		return;

	timeNextGameHeartbeat = Singleton<IGameTimer>()->GetAbsTime() + HEARTBEAT_PERIOD;
	pClient->SendPacket( new CGameHeartBeatPacket( 0, nGameID ) );
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"TX",
		"CGameHeartBeatPacket",
		GetOwnClientID(),
		fmt::format( "game_id={} reason={} update_info={}", nGameID, szReason ? szReason : "", bUpdateGameInfo ? 1 : 0 ) );

	if ( bUpdateGameInfo && gameDesc.pMPMap )
	{
		CPtr<SGameInfo> pGameInfo = new SGameInfo;
		SetServerGameInfo( pGameInfo );
		pGameInfo->bCanConnect = false;
		pGameInfo->nPlayers = 0;
		for ( int i = 0; i < slots.size(); ++i )
		{
			if ( IsPlayerPresent( i ) && GetSlotClientID( i ) != nRemovedClientID )
				++pGameInfo->nPlayers;
		}
		pClient->SendPacket( new CUpdateGameInfo( 0, *pGameInfo ) );
		NGameX::MatchPacketTrace_Log(
			IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
			"TX",
			"CUpdateGameInfo",
			GetOwnClientID(),
			fmt::format( "game_id={} players={} reason={}", nGameID, pGameInfo->nPlayers, szReason ? szReason : "" ) );
	}
}

void CMPManagerModeNivalNet::OnGameControlHostChanged( int nOldHostClientID, int nNewHostClientID, int nRemovedClientID )
{
	if ( !IsGameRunning() )
		return;

	const bool bOwnsGameControl = ( GetOwnClientID() == nNewHostClientID );
	RefreshServerGameKeepalive( bOwnsGameControl, "control_host_changed", nRemovedClientID );
}

bool CMPManagerModeNivalNet::Segment()
{
	if ( ShouldSendHeartbeatNow() )
	{
		RefreshServerGameKeepalive( false, "periodic", -1 );
		//DebugTrace( "+++ Send Heartbeat, game %d, time %d", nGameID, Singleton<IGameTimer>()->GetAbsTime() );
	}

	if ( updateChannels.CheckNeedUpdate() )
		RequestChatChannels( updateChannels.dwVersion );

	if ( eState == EGS_LADDER_WAIT_CLIENTS )
		ProcessLadderWaitForClients();

	return CMPManagerMode::Segment();
}

void CMPManagerModeNivalNet::OnGameSpecificInfo()
{
	CheckJoinGameConditions();
}

void CMPManagerModeNivalNet::StartGame()
{
	if ( IsGameHost() || bLadderGame && bHost )
	{
		// Tell the server that the game is started
		CPtr<SGameInfo> pGameInfo = new SGameInfo;
		SetServerGameInfo( pGameInfo );
		pGameInfo->bCanConnect = false;
		CPtr<CUpdateGameInfo> pUpdatePkt = new CUpdateGameInfo( 0, *pGameInfo );
		pClient->SendPacket( pUpdatePkt );

		// Delay heartbeats
		CPtr<CGameStartLoadingPacket> pDelayHeartbeatsPkt = new CGameStartLoadingPacket( 0, nGameID );
		pClient->SendPacket( pDelayHeartbeatsPkt );

		timeNextGameHeartbeat = 0;
		//DebugTrace( "+++ Game Starting packets sent" );
	}

	CPtr<CChatAFKPacket> pAFKPkt = new CChatAFKPacket( 0 );
	pClient->SendPacket( pAFKPkt );

	if ( bLadderGame )		// Here we do not know the number of players, count them
	{
		int nClientsConnected = 0;
		for ( int i = 0; i < slots.size(); ++i )
		{
			if ( slots[i].bPresent && slots[i].bAccept )
				++nClientsConnected;
		}
		nSlotsUsed = nClientsConnected;
	}

	eState = EGS_GAME_STARTED;
	CMPManagerMode::StartGame();
}

void CMPManagerModeNivalNet::EndGame()
{
	if ( eState == EGS_GAME_STARTED )
	{
		if ( bLadderGame )
			bAwaitingLadderStatsChange = true;
		else
		{
			PushMessage( new SMPUIGameAftemathMessage( false ) );
			bAwaitingLadderStatsChange = false;
		}
	}
	eState = EGS_CUSTOM_GAME;
	CPtr<CLeaveGamePacket> pLeavePkt = new CLeaveGamePacket( 0, nGameID );
	pClient->SendPacket( pLeavePkt );

	CMPManagerMode::EndGame();
	SendGameStatistics();
	if ( bLadderGame )
	{
		eState = EGS_LOBBY;
		pClient->SendPacket( new CLeaveLobbyPacket( 0 ) );
		bLadderGame = false;
	}
}

void CMPManagerModeNivalNet::SetServerGameInfo( SGameInfo *pInfo )
{
	pInfo->nID = nGameID;
	pInfo->nMaxPlayers = slots.size();
	pInfo->nPlayers = nSlotsUsed;
	pInfo->szMapName = NStr::ToMBCS( GET_TEXT_PRE( gameDesc.pMPMap->, MapName ) );
	pInfo->szName = szSessionName;
	pInfo->nMapSizeX = gameDesc.pMPMap->nSizeX;
	pInfo->nMapSizeY = gameDesc.pMPMap->nSizeY;
	pInfo->nGameType = gameDesc.eType;
	pInfo->nTechLevel = gameDesc.nTechLevel;
	pInfo->bHasPassword = !( szPassword.length() == 0 );
	pInfo->szPassword = szPassword;
}

void CMPManagerModeNivalNet::InitLobby()
{
	updateChannels.dwVersion = 0;
	updateChannels.bUpdating = false;
	updateChannels.timeNextUpdate = 0;
	updateChannels.timeUpdatePeriod = 10000;

	chatChannels.clear();
}

void CMPManagerModeNivalNet::KickPlayerFromSlot( const int nSlot )
{
	if ( IsInGameRoom() && IsGameHost() )
	{
		CPtr<CGameKickClient> pKickPkt = new CGameKickClient( 0, nGameID, slots[nSlot].nClientID );
		NGameX::MatchPacketTrace_Log(
			IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
			"TX",
			"CGameKickClient",
			GetOwnClientID(),
			fmt::format( "slot={} kicked_client={} game_id={}", nSlot, slots[nSlot].nClientID, nGameID ) );
		pClient->SendPacket( pKickPkt );
	}
}

void CMPManagerModeNivalNet::CreateServerClient()
{
	pClient = 0;
	std::string szNivalNetIP = NStr::ToMBCS( NGlobal::GetVar( "NivalNetIP", NIVAL_NET_IP ) );
	int nNivalNetPort = NGlobal::GetVar( "NivalNetPort", NIVAL_NET_PORT );
	pClient = new CServerClient( szNivalNetIP.c_str(), NGlobal::GetVar( "NetGameVersion", 1 ), nNivalNetPort, 30 );
	CNet::SetTimeOut( 20.0f );
}

void CMPManagerModeNivalNet::SendGameStatistics()
{
	IScenarioTracker *pScenarioTracker = Singleton<IScenarioTracker>();

	if ( !bLadderGame || !pScenarioTracker->IsMissionWon() )
		return;

	CPtr<CLadderGameResultPacket> pPkt = new CLadderGameResultPacket;
	pPkt->nClientID = 0;
	pPkt->nGameID = nGameID;
	int nWinTeam = slots[nOwnSlot].nTeam;
	for ( int i = 0; i < slots.size(); ++i )
	{
		SMPSlot &slot = slots[i];
		if ( i < nOwnSlot && slot.nTeam == nWinTeam )
			return;			// I am not the first winner

		if ( !( dwInitialPlayers & ( 1UL << i ) ) )
			continue;

		if ( slot.nTeam == nWinTeam )
			pPkt->winners.insert( slot.nClientID );

		pPkt->races[slot.nClientID] = slot.nCountry;
		pPkt->unitsKilled[slot.nClientID] = pScenarioTracker->GetStatistics( i, IScenarioTracker::ESK_UNITS_KILLED );
		pPkt->unitsLost[slot.nClientID] = pScenarioTracker->GetStatistics( i, IScenarioTracker::ESK_UNITS_LOST );
		pPkt->playerUnitEff[slot.nClientID] = pScenarioTracker->GetStatistics( i, IScenarioTracker::ESK_TACTICAL_EFFICIENCY );
		pPkt->playerKeyPointEff[slot.nClientID] = pScenarioTracker->GetStatistics( i, IScenarioTracker::ESK_STRATEGIC_EFFICIENCY );
		std::vector<int> reinfCallsByType;
		pScenarioTracker->GetReinforcementCallsInfo( i, &reinfCallsByType );
		pPkt->reinfUsed[slot.nClientID] = reinfCallsByType;
	}
	pClient->SendPacket( pPkt );
}

void CMPManagerModeNivalNet::ProcessLadderWaitForClients()
{
	if ( GameTimer()->GetAbsTime() > timeOutLadderGame )
	{
		eState = EGS_LOBBY;
		pClient->SendPacket( new CLeaveLobbyPacket( 0 ) );
		PushMessage( new SMPUILadderStatusChangeMessage( SMPUILadderStatusChangeMessage::ELS_CANCELLED ) );
		bLadderGame = false;
		return;
	}

	if ( !bHost )
		return;
	// The following is host only
	int nClientsConnected = 0;
	for ( int i = 0; i < slots.size(); ++i )
	{
		SMPSlot &slot = slots[i];
		if ( slot.bPresent )
		{
			if ( slot.bAccept && slot.wConnectedTo == slots[nOwnSlot].wConnectedTo )
				++nClientsConnected;
			else
				return;
		}
	}
	nSlotsUsed = nClientsConnected;

	eState = EGS_GAME_ROOM;
	bLadderGame = true;
	SendStartGamePacket();
	StartGame();
}

int CMPManagerModeNivalNet::DetermineLadderCountry( const int nInitial, const NDb::EHistoricalSide eSide )
{
	if ( nInitial >= 0 )
		return nInitial;

	int nAttempts = pMPConsts->sides.size() + 3;
	int nIndex = NWin32Random::Random( pMPConsts->sides.size() );
	while ( --nAttempts > 0 )
	{
		if ( pMPConsts->sides[nIndex].eHistoricalSide == eSide )
			return nIndex;
		nIndex = ( nIndex + 1 ) % pMPConsts->sides.size();
	}
	return 0;
}

void CMPManagerModeNivalNet::SendLadderGameAftermath( const SLadderStatistics &oldStats, const SLadderStatistics &newStats )
{
	SMPUIGameAftemathMessage *pMsg = new SMPUIGameAftemathMessage( true );
	pMsg->nLevel = newStats.nLevel;
	pMsg->nOldLevel = oldStats.nLevel;
	pMsg->nRank = GetRankFromLevel( newStats.nRace, newStats.nLevel, 0 );
	pMsg->nOldRank = GetRankFromLevel( oldStats.nRace, oldStats.nLevel, 0 );
	pMsg->nCountry = newStats.nRace;
	pMsg->nExpEarned = newStats.nXP - oldStats.nXP;
	pMsg->nExpTotal1 = newStats.nXP - newStats.nLevelXP;
	pMsg->nExpTotal2 = newStats.nNextLevelXP - newStats.nLevelXP;

	// Medals
	for ( int i = 0; i < pMPConsts->sides.size(); ++i )
	{
		const NDb::SMultiplayerSide &side = pMPConsts->sides[i];
		int nMedalChange = oldStats.medals[i] ^ newStats.medals[i];
		for ( int j = 0; j < side.medals.size(); ++j )
		{
			if ( nMedalChange & ( 1UL << j ) )
				pMsg->medals.push_back( side.medals[j] );
		}
	}

	PushMessage( pMsg );
}

const int CMPManagerModeNivalNet::GetRankFromLevel( const int nRace, const int nLevel, std::wstring *pwszOut )
{
	if ( pwszOut )
		*pwszOut = L"-";
	int nResult = 0;
	const std::vector<NDb::SLadderRank> &ranks = pMPConsts->sides[nRace].ladderRanks;
	for ( int i = 0; i < ranks.size(); ++i )
	{
		if ( ranks[i].nLevel > nLevel )
			break;
		else
		{
			if ( pwszOut )
				*pwszOut = GET_TEXT_PRE( ranks[i]., Name );
			nResult = i;
		}
	}
	return nResult;
}

void CMPManagerModeNivalNet::ExtractGameInfo( SNetGameInfo *pDst, const struct SGameInfo &src )
{
	pDst->nGameID = src.nID;
	pDst->szSessionName = src.szName;
	pDst->nPlayers = src.nPlayers;
	pDst->nMaxPlayers = src.nMaxPlayers;
	pDst->szMapName = src.szMapName;
	pDst->nSizeX = src.nMapSizeX;
	pDst->nSizeY = src.nMapSizeY;
	pDst->nGameType = src.nGameType;
	pDst->nTechLevel = src.nTechLevel;
	pDst->bPasswordRequired = src.bHasPassword;
}

void CMPManagerModeNivalNet::OnSurrender()
{
	if ( !IsGameRunning() || !bLadderGame )
		return;

	pClient->SendPacket( new CLadderSurrenderPacket( 0, nGameID ) );
}


//
//   Messages
//


bool CMPManagerModeNivalNet::OnLoginNivalNetMessage( SMPUILoginNivalNetMessage *pMsg )
{
	CreateServerClient();	
	if ( !pClient )
	{
		OnCriticalNetworkError( InterfaceState()->GetTextEntry( "T_NET_DRIVER_CRITICAL_ERROR" ) );
		return true;
	}
	szMPName = NStr::ToMBCS( pMsg->wszLogin );
	NGlobal::SetVar( "Multiplayer.PlayerName", szMPName );
	CLoginPacket *pLoginPacket = new CLoginPacket( 0, szMPName, NStr::ToMBCS( pMsg->wszPassword ) );
	pClient->SendPacket( pLoginPacket );
	nMyClientID = -1;

	if ( pMsg->bRememberPassword )
	{
		NGlobal::SetVar( "Multiplayer.NivalNet.StoredLogin", pMsg->wszLogin );
		NGlobal::SetVar( "Multiplayer.NivalNet.StoredPassword", pMsg->wszPassword );
	}
	else
	{
		NGlobal::SetVar( "Multiplayer.NivalNet.StoredLogin", L"" );
		NGlobal::SetVar( "Multiplayer.NivalNet.StoredPassword", L"" );
	}

	eState = EGS_LOGGING_IN;

	return true;
}

bool CMPManagerModeNivalNet::OnCustomGameMessage( SMPUIMessage *pMsg )
{
	CPtr<CEnterLobbyPacket> pPkt = new CEnterLobbyPacket( 0, ERID_CUSTOM );
	pClient->SendPacket( pPkt );

	CPtr<CChatAFKPacket> pAFKPkt = new CChatAFKPacket( 0 );
	pClient->SendPacket( pAFKPkt );

	updateChannels.bUpdating = false;

	return true;
}

bool CMPManagerModeNivalNet::OnBackFromGameListMessage( SMPUIMessage *pMsg )
{
	NI_ASSERT( eState == EGS_CUSTOM_GAME || eState == EGS_LOBBY, "PRG: NivalNet: Wrong state to receive BackFromGameList" );
	eState = EGS_LOBBY;
	pClient->SendPacket( new CLeaveLobbyPacket( 0 ) );
	InitLobby();
	return false;		// so that MPManager handles it as well
}

bool CMPManagerModeNivalNet::OnRegisterNivalNetMessage( SMPUIRegisterMessage *pMsg )
{
	CreateServerClient();
	if ( !pClient )
	{
		OnCriticalNetworkError( InterfaceState()->GetTextEntry( "T_NET_DRIVER_CRITICAL_ERROR" ) );
		return true;
	}
	szMPName = NStr::ToMBCS( pMsg->wszName );
	NGlobal::SetVar( "Multiplayer.PlayerName", szMPName );
	CRegisterPacket *pRegisterPkt = new CRegisterPacket( 0, szMPName, NStr::ToMBCS( pMsg->wszPassword ), 
		NStr::ToMBCS( pMsg->wszCDKey ), NStr::ToMBCS( pMsg->wszEmail ) );
	pClient->SendPacket( pRegisterPkt );
	nMyClientID = -1;
	eState = EGS_LOGGING_IN;
	return true;
}

bool CMPManagerModeNivalNet::OnLadderInfoRequestMessage( SMPUILadderInfoRequestMessage *pMsg )
{
	if ( pMsg->szNick == szMPName && bGotMyLadderStats && !pMsg->bShort )
	{
		SMPUILadderStatsMessage *pMsg = new SMPUILadderStatsMessage( myLadderStats );
		pMsg->nRank = GetRankFromLevel( myLadderStats.nRace, myLadderStats.nLevel, 0 );
		PushMessage( pMsg );
		return true;
	}
	CLadderStatisticsRequestPacket *pPkt = new CLadderStatisticsRequestPacket( 0, pMsg->szNick, !pMsg->bShort );
	pClient->SendPacket( pPkt );
	if ( !pMsg->bShort )
		bUIAskedStats = true;
	return true;
}

bool CMPManagerModeNivalNet::OnCancelLadderMessage( SMPUIMessage *pMsg )
{
	CPtr<CEnterLobbyPacket> pPkt = new CEnterLobbyPacket( 0, ERID_NO_LOBBY );
	pClient->SendPacket( pPkt );
	eState = EGS_LOBBY;
	return true;
}

bool CMPManagerModeNivalNet::OnSetupLadderGameMessage( SMPUILadderGameMessage *pMsg )
{
	CPtr<CEnterLobbyPacket> pLobbyPkt = new CEnterLobbyPacket( 0, ERID_LADDER );
	pClient->SendPacket( pLobbyPkt );

	eState = EGS_LOBBY_WAIT_LADDER;
	pendingClients.clear();
	nOwnSlot = -1;
	nLadderCountry = pMsg->nCountry - 1;
	bHistoricity = pMsg->bHistoricity;
	int nLadderSide = 0;
	if ( nLadderCountry >= 0 )
	{
		if ( pMPConsts->sides[nLadderCountry].eHistoricalSide == NDb::HS_AXIS )
			nLadderSide = 2;
		else
			nLadderSide = 1;
	}
	unsigned int uCheckSum = 0;
	for ( int i = 0; i < NGameX::GetGameRoot()->multiplayerMaps.size(); ++i )
	{
		uCheckSum += NGameX::GetGameRoot()->multiplayerMaps[i]->CalcCheckSum();
	}
	CPtr<CLadderInfoPacket> pLadderPkt = new CLadderInfoPacket( 0, nLadderSide, bHistoricity, pMsg->nTeamSize, pMsg->techLevels, pMsg->maps, uCheckSum );
	pClient->SendPacket( pLadderPkt );
	SMPUILadderStatusChangeMessage *pStatusMsg = new SMPUILadderStatusChangeMessage( SMPUILadderStatusChangeMessage::ELS_SEARCH_STARTED );
	PushMessage( pStatusMsg );

	if ( !bGotMyLadderStats )
	{
		CLadderStatisticsRequestPacket *pPkt = new CLadderStatisticsRequestPacket( 0, szMPName, true );
		pClient->SendPacket( pPkt );
	}
	
	return true;
}


//
//   Packets
//


bool CMPManagerModeNivalNet::OnConnectServerPacket( class CConnectServerPacket *pPacket )
{
	CPtr<SMPUIConnectResultMessage> pMsg = new SMPUIConnectResultMessage;
	if ( eState != EGS_LOGGING_IN )
	{
		// Server is down, most probably, send emergency message elsewhere
		return true;
	}

	if ( pPacket->eConnectionState == CConnectServerPacket::ECS_SUCCESS )
	{
		pMsg->bSuccess = true;
		eState = EGS_LOBBY;
	}
	else
	{
		pMsg->bSuccess = false;
		pClient = 0;
		pClient = new CEmptyServerClient;
		switch ( pPacket->eRejectReason )
		{
		case CConnectServerPacket::ERR_CANT_RESOLVE_ADDRESS:
			pMsg->szTextTag = "CONNECT_ERR_CANT_RESOLVE_ADDRESS";
			break;
		case CConnectServerPacket::ERR_CONNECTION_LOST:
			pMsg->szTextTag = "CONNECT_ERR_CONNECTION_LOST";
			break;
		case CConnectServerPacket::ERR_BAD_NICK:
			pMsg->szTextTag = "CONNECT_ERR_BAD_NICK";
			break;
		case CConnectServerPacket::ERR_WRONG_CD_KEY:
			pMsg->szTextTag = "CONNECT_ERR_WRONG_CD_KEY";
			break;
		case CConnectServerPacket::ERR_ALREADY_REGISTERED:
			pMsg->szTextTag = "CONNECT_ERR_ALREADY_REGISTERED";
			break;
		case CConnectServerPacket::ERR_ALREADY_ONLINE:
			pMsg->szTextTag = "CONNECT_ERR_ALREADY_ONLINE";
			break;
		case CConnectServerPacket::ERR_NOT_REGISTERED:
			pMsg->szTextTag = "CONNECT_ERR_NOT_REGISTERED";
			break;
		case CConnectServerPacket::ERR_BANNED_NICK:
			pMsg->szTextTag = "CONNECT_ERR_BANNED_NICK";
			break;
		case CConnectServerPacket::ERR_BANNED_CDKEY:
			pMsg->szTextTag = "CONNECT_ERR_BANNED_CDKEY";
			break;
		case CConnectServerPacket::ERR_WRONG_PASSWORD:
			pMsg->szTextTag = "CONNECT_ERR_WRONG_PASSWORD";
			break;
		case CConnectServerPacket::ERR_KICKED:
			pMsg->szTextTag = "CONNECT_ERR_KICKED";
			break;
		case CConnectServerPacket::ERR_LOGIN_TIMEOUT:
			pMsg->szTextTag = "CONNECT_ERR_LOGIN_TIMEOUT";
			break;
		case CConnectServerPacket::ERR_WRONG_NET_VERSION:
			pMsg->szTextTag = "CONNECT_ERR_WRONG_NET_VERSION";
			break;
		case CConnectServerPacket::ERR_CRITICAL_BUSY:
			pMsg->szTextTag = "CONNECT_ERR_CRITICAL_BUSY";
			break;
		default:
			pMsg->szTextTag = "CONNECT_ERR_UNKNOWN";
			break;
		}
	}
	PushMessage( pMsg );

	if ( pPacket->eConnectionState == CConnectServerPacket::ECS_SUCCESS )
	{
		gamesUpdate.dwVersion = 0;
		games.clear();
		InitLobby();
	}

	return true;
}

bool CMPManagerModeNivalNet::OnLobbyGamesPacket( CLobbyGamesPacket *pPacket )
{
	gamesUpdate.dwVersion = pPacket->dwVersion;
	SMPUIGameListMessage *pGamesMsg = new SMPUIGameListMessage;

	if ( pPacket->bFullUpdate )
	{
		// Add all games as deleted
		for ( CNetGameList::iterator it = games.begin(); it != games.end(); ++it )
			pGamesMsg->gamesRemoved.push_back( (*it).nGameID );

		games.clear();
	}

	// Process new games
	for ( std::list<SGameInfo>::iterator it = pPacket->added.begin(); it != pPacket->added.end(); ++it )
	{
		SGameInfo &game = *it;

		if ( !game.bCanConnect )
			continue;

		SNetGameInfo newGame;
		ExtractGameInfo( &newGame, game );
		AddGameInfoForUI( &(pGamesMsg->gamesAddChange), newGame );
		games.push_back( newGame );
	}

	if ( !pPacket->bFullUpdate )
	{
		// Process changes
		for ( std::list<SGameInfo>::iterator it = pPacket->changed.begin(); it != pPacket->changed.end(); ++it )
		{
			SGameInfo &game = *it;

			CNetGameList::iterator itGame = games.begin();
			for ( ; itGame != games.end() && itGame->nGameID != game.nID; ++itGame )
				;
			if ( itGame == games.end() )
				continue;

			if ( !game.bCanConnect )
			{
				games.erase( itGame );
				pGamesMsg->gamesRemoved.push_back( game.nID );
				continue;
			}
			SNetGameInfo &ownGame = *itGame;
			ExtractGameInfo( &ownGame, game );
			AddGameInfoForUI( &(pGamesMsg->gamesAddChange), ownGame );
		}
		// Process deletes
		for ( std::list<int>::iterator it = pPacket->removed.begin(); it != pPacket->removed.end(); ++it )
		{
			CNetGameList::iterator itGame = games.begin();
			for ( ; itGame != games.end() && itGame->nGameID != *it; ++itGame )
				;
			if ( itGame == games.end() )
				continue;

			pGamesMsg->gamesRemoved.push_back( *it );
			games.erase( itGame );
		}
	}
	PushMessage( pGamesMsg );

	return true;
}

bool CMPManagerModeNivalNet::OnMyIDPacket( class CMyIDPacket *pPacket )
{
	//DebugTrace( "+++ MyID packet from client %d, id %d", pPacket->nClientID, pPacket->nMyID );
	nMyClientID = pPacket->nMyID;

	return true;
}

bool CMPManagerModeNivalNet::OnConnectGameFailed( class CConnectGameFailed *pPacket )
{
	//DebugTrace( "+++ ConnectGameFailed packet from client %d, reason %d", pPacket->nClientID, pPacket->eReason );
	if ( eState != EGS_JOINING && eState != EGS_LOBBY_WAIT_LADDER )
		return true;

	if ( eState == EGS_JOINING )
	{
		PushMessage( new SMPUIGameRoomInitMessage( SMPUIGameRoomInitMessage::ERR_CONNECT_FAILED ) );
		eState = EGS_CUSTOM_GAME;
	}
	else
	{
		PushMessage( new SMPUILadderStatusChangeMessage( SMPUILadderStatusChangeMessage::ELS_CANCELLED ) );
		eState = EGS_LOBBY;
	}

	return true;
}

bool CMPManagerModeNivalNet::OnConnectedGameIDPacket( class CConnectedGameID *pPacket )
{
	//DebugTrace( "+++ ConnectedGameID packet, game %d", pPacket->nGameID );
	nGameID = pPacket->nGameID;
	if ( eState == EGS_LOBBY_WAIT_LADDER )
	{
		timeNextGameHeartbeat = 0;
		timeOutLadderGame = GameTimer()->GetAbsTime() + 60000;
		eState = EGS_LADDER_WAIT_CLIENTS;
		return true;
	}

	if ( eState == EGS_JOINING && !bHost )
	{
		timeNextGameHeartbeat = 0;
		CheckJoinGameConditions();
		return true;
	}

	if ( eState == EGS_JOINING && bHost )
	{
		eState = EGS_GAME_ROOM;
		SendGameRoomInitMessages();
		timeNextGameHeartbeat = 0;
	}

	return true;
}

bool CMPManagerModeNivalNet::OnNewGameClientPacket( class CNewGameClient *pPacket )
{
	if ( IsGameHost() )
	{
		if ( !HostAssignNewClient( pPacket->nClientID ) )
			return true;
	}
	else if ( eState == EGS_GAME_ROOM && !bHost || eState == EGS_LADDER_WAIT_CLIENTS )
	{
		DebugTrace( "+++ NewGameClient packet from client %d, ClientAssign", pPacket->nClientID );
		ClientAssignNewClient( pPacket->nClientID );
	}
	else if ( eState == EGS_JOINING || eState == EGS_LOBBY_WAIT_LADDER )
	{
		DebugTrace( "+++ NewGameClient packet from client %d, pending", pPacket->nClientID );
		JoiningClientAssignNewClient( pPacket->nClientID );
	}

	return true;
}

bool CMPManagerModeNivalNet::OnEnterLobbyPacket( class CEnterLobbyPacket *pPacket )
{
	//DebugTrace( "+++ EnterLobby packet from client %d, lobby %d", pPacket->nClientID, pPacket->nLobbyID );
	if ( pPacket->nLobbyID == ERID_CUSTOM && eState == EGS_LOBBY )
		eState = EGS_CUSTOM_GAME;

	return true;
}

bool CMPManagerModeNivalNet::OnGameClientWasKicked( class CGameClientWasKicked *pPacket )
{
	//DebugTrace( "+++ ClientWasKicked packet for client %d", pPacket->nKicked );
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"RX",
		"CGameClientWasKicked",
		pPacket->nClientID,
		fmt::format( "kicked={} in_room={}", pPacket->nKicked, IsInGameRoom() ? 1 : 0 ) );
	if ( IsInGameRoom() )
	{
		if ( nMyClientID == pPacket->nKicked )
		{
			NGameX::MatchPacketTrace_SetFinalState( GetPresentMask(), dwLaggers, IsValid( pTransceiver ) ? pTransceiver->GetPlayerMask() : 0 );
			NGameX::MatchPacketTrace_Flush( "kicked" );
			PushMessage( new SMPUIGameRoomInitMessage( SMPUIGameRoomInitMessage::ERR_KICKED ) );
			OnLeaveGame();
		}
		else
		{
			for ( int i = 0; i < slots.size(); ++i )
			{
				if ( IsPlayerPresent( i ) && slots[i].nClientID == pPacket->nKicked )
				{
					slots[i].bPresent = false;
					slots[i].bAccept = false;

					SMPUIUpdateSlotMessage *pMsg = new SMPUIUpdateSlotMessage;
					pMsg->info = slots[i];
					pMsg->nSlot = i;
					PushMessage( pMsg );
					break;
				}
			}
		}
	}

	return true;
}

bool CMPManagerModeNivalNet::OnNetRemoveClientPacket( class CNetRemoveClient *pPacket )
{
	if ( pPacket->nClientID == 0 )			// Disconnected from Server 
	{
		OnCriticalNetworkError( InterfaceState()->GetTextEntry( "T_NIVAL_NET_SERVER_DISCONNECT" ) );
	}
	RemoveClient( pPacket->nClientID, false );
	
	return true;
}

bool CMPManagerModeNivalNet::OnGameClientDeadPacket( class CGameClientDead *pPacket )
{
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"RX",
		"CGameClientDead",
		pPacket->nClientID,
		fmt::format( "dead_client={}", pPacket->nDeadClient ) );
	RemoveClient( pPacket->nDeadClient, false );
	return true;
}

bool CMPManagerModeNivalNet::OnLadderShortStatisticsPacket( class CLadderShortStatisticsPacket *pPacket )
{
	if ( pPacket->nRace < 0 || pPacket->nRace >= pMPConsts->sides.size() )
	{
		NI_ASSERT( 0, fmt::format( "MP: Passed race ({}) is out of bounds", pPacket->nRace ) );
		return true;
	}

	SMPUIShortInfoMessage *pMsg = new SMPUIShortInfoMessage();
	pMsg->nLevel = pPacket->nLevel;
	GetRankFromLevel( pPacket->nRace, pPacket->nLevel, &pMsg->wszRank );
	PushMessage( pMsg );
	return true;
}

bool CMPManagerModeNivalNet::OnLadderInvitePacket( class CLadderInvitePacket *pPacket )
{
	//DebugTrace( "+++ LadderInvite packet" );
	const NDb::SGameRoot *pGameRoot = NGameX::GetGameRoot();
	NI_VERIFY( pGameRoot, "Could not get GameRoot", return true );
	NI_VERIFY( pPacket->nMapID < pGameRoot->multiplayerMaps.size(), "NNet Ladder Invite: Map Index exceeds bounds", return true );
	nGameID = pPacket->nGameID;
	int nTeam1Size = pPacket->team1.size();
	int nTeam2Size = pPacket->team2.size();
	gameDesc.pMPMap = pGameRoot->multiplayerMaps[pPacket->nMapID];
	gameDesc.nPlayers = nTeam1Size + nTeam2Size;
	gameDesc.nTimeLimit = 60;
	gameDesc.nGameSpeed = 0;
	gameDesc.nTechLevel = pPacket->nTechLevel;
	gameDesc.bUnitExp = true;
	gameDesc.nCaptureTime = 30;

	DebugTrace( "+++ LadderInvite: game %d, teams %d-%d, map %d, TL %d", nGameID, nTeam1Size, nTeam2Size, pPacket->nMapID, pPacket->nTechLevel );
	std::string szDebugLadder = "+++ Ladder Teams: ";

	bGameRoomInit = false;
	slots.clear();
	slots.resize( nTeam1Size + nTeam2Size );
	int nSlot = 0;
	nOwnSlot = -1;
	NDb::EHistoricalSide eMySide = NDb::HS_ALLIES;
	szDebugLadder += "Team1 (Allies) - ";
	for ( std::list<int>::iterator it = pPacket->team1.begin(); it != pPacket->team1.end(); ++it, ++nSlot )
	{
		SMPSlot &slot = slots[nSlot];
		slot.nClientID = *it;
		slot.nTeam = 0;
		slot.bPresent = true;
		slot.bAccept = false;
		if ( *it == nMyClientID && nOwnSlot < 0 )
		{
			nOwnSlot = nSlot;
			szDebugLadder += "*";
		}
		szDebugLadder += fmt::format( "{}, ", slot.nClientID );
	}
	szDebugLadder += "Team2 (Axis) - ";
	for ( std::list<int>::iterator it = pPacket->team2.begin(); it != pPacket->team2.end(); ++it, ++nSlot )
	{
		SMPSlot &slot = slots[nSlot];
		slot.nClientID = *it;
		slot.nTeam = 1;
		slot.bPresent = true;
		slot.bAccept = false;
		if ( *it == nMyClientID && nOwnSlot < 0 )
		{
			nOwnSlot = nSlot;
			eMySide = NDb::HS_AXIS;
			szDebugLadder += "*";
		}
		szDebugLadder += fmt::format( "{}, ", slot.nClientID );
	}
	NI_VERIFY( nOwnSlot >= 0, "NNet Ladder Invite: Could not find own client ID", return true );
	bHost = ( nOwnSlot == 0 );
	nHostClientID = slots[0].nClientID;
	SMPSlot &mySlot = slots[nOwnSlot];
	mySlot.szName = szMPName;
	mySlot.bPresent = true;
	mySlot.bAccept = true;
	mySlot.nColour = nOwnSlot;
	mySlot.nCountry = DetermineLadderCountry( nLadderCountry, eMySide );
	mySlot.bRandomCountry = true;
	bLadderGame = true;

	DebugTrace( szDebugLadder.c_str() );
	DebugTrace( "+++ LadderInvite: my slot %d, country %d", nOwnSlot, mySlot.nCountry );

	if ( pendingClients.size() > 0 )
	{
		for ( std::list<int>::iterator it = pendingClients.begin(); it != pendingClients.end(); ++it )
			ClientAssignNewClient( *it );

		pendingClients.clear();
	}
	CPtr<CConnectGamePacket> pConnectPkt = new CConnectGamePacket( 0, nGameID, "" );
	pClient->SendPacket( pConnectPkt );

	SMPUILadderStatusChangeMessage *pStatusMsg = new SMPUILadderStatusChangeMessage( SMPUILadderStatusChangeMessage::ELS_GAME_FOUND );
	PushMessage( pStatusMsg );

	return true;
}

bool CMPManagerModeNivalNet::OnLadderStatisticsPacket( class CLadderStatisticsPacket *pPacket )
{
	if ( pPacket->szNick == szMPName )
	{
		if ( bGotMyLadderStats && bAwaitingLadderStatsChange )
		{
			SendLadderGameAftermath( myLadderStats, pPacket->info );
			bAwaitingLadderStatsChange = false;
		}
		myLadderStats = pPacket->info;
		bGotMyLadderStats = true;
	}

	if ( bUIAskedStats )
	{
		bUIAskedStats = false;
		SMPUILadderStatsMessage *pMsg = new SMPUILadderStatsMessage( pPacket->info );
		pMsg->nRank = GetRankFromLevel( pPacket->info.nRace, pPacket->info.nLevel, 0 );
		PushMessage( pMsg );
	}

	return true;
}

bool CMPManagerModeNivalNet::OnSystemBroadcastPacket( class CSystemBroadcastPacket *pPacket )
{
	if ( pPacket->wszText.length() == 0 )
		return true;

	std::wstring wszMsg = InterfaceState()->GetTextEntry( "NIVAL_NET_BROADCAST_HEADER" );
	wszMsg += L"<br>" + pPacket->wszText;
	NMainLoop::Command( ML_COMMAND_MESSAGE_BOX, 
		CICMessageBox::MakeConfigString( "MessageBoxWindowOk", wszMsg ).c_str() );
	return true;
}

bool CMPManagerModeNivalNet::OnLadderInvalidStatisticsPacket( class CLadderInvalidStatisticsPacket *pPacket )
{
	if ( bAwaitingLadderStatsChange )
	{
		PushMessage( 	new SMPUIGameAftemathMessage( false ) );
		bAwaitingLadderStatsChange = false;
	}
	return true;
}


