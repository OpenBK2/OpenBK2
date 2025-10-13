#include "stdafx.h"

#include "MPManagerMode.h"
#include "Server_Client_Common/ChatPackets.h"
#include "Client/ServerClientInterface.h"
#include "MultiplayerNetPackets.h"
#include "MPTransceiver.h"
#include "Main/MainLoop.h"
#include "Main/MainLoopCommands.h"
#include "GameXClassIDs.h"
#include "DBMPConsts.h"
#include "GetConsts.h"
#include "InterfaceState.h"
#include "Misc/StrProc.h"
#include "ScenarioTracker.h"
#include "Misc/StringConversions.h"
#include "DBConsts.h"
#include "MPPacketTraceLog.h"

#include <cstdint>

// CMPManagerMode - UI->MP message handlers

bool CMPManagerMode::OnCreateGameMessage( SMPUICreateGameMessage *pMsg )
{
	// Store info
	gameDesc = pMsg->specificInfo;
	szSessionName = pMsg->info.szSessionName;
	if ( pMsg->info.bPwdReq )
		szPassword = pMsg->szPassword;
	else
		szPassword = "";
	gameDesc.eType = SB2GameSpecificData::EGT_FLAG_CONTROL_2;

	gameDesc.nPlayers = pMsg->info.nPlayersMax;

	// Arrange slots
	slots.clear();
	slots.resize( gameDesc.nPlayers );
	slots[0].szName = szMPName;			// Add self
	slots[0].bPresent = true;
	slots[0].nClientID = GetOwnClientID();
	nHostClientID = slots[0].nClientID;
	nOwnSlot = 0;
	nSlotsUsed = 1;

	bGameRoomInit = false;

	ulGameCheckSum = NGameX::GetGameConsts()->GetMPDataVersionChecksumWithMap( gameDesc.pMPMap );
	ulHostCheckSum = ulGameCheckSum;

	TryToCreateGame();
	return true;
}

bool CMPManagerMode::OnJoinGameMessage( SMPUIJoinGameMessage *pMsg )
{
	bool bFound = false;
	for ( CNetGameList::iterator it = games.begin(); it != games.end(); ++it )
	{
		SNetGameInfo &game = *it;
		if ( game.nGameID == pMsg->nGameID )
		{
			if ( game.nMaxPlayers < 2 || game.nMaxPlayers - game.nPlayers < 1 )		// Do not join games with less than 2 players, or full games
				break;

			bFound = true;
			// Init all necessary data
			slots.clear();
			slots.resize( game.nMaxPlayers );
			nOwnSlot = -1;
			nSlotsUsed = 0;

			szSessionName = game.szSessionName;
			if ( game.bPasswordRequired )
				szPassword = pMsg->szPassword;
			else
				szPassword = "";

			ulGameCheckSum = 0;

			TryToJoinGame( game );

			//games.clear();
			gamesUpdate.bUpdating = false;
			break;
		}
	}

	if ( !bFound )
	{
		PushMessage( new SMPUIGameRoomInitMessage( SMPUIGameRoomInitMessage::ERR_CONNECT_FAILED ) );
		return true;
	}

	pendingClients.clear();
	bGameRoomInit = false;

	return true;
}

bool CMPManagerMode::OnStartGameMessage( SMPUIMessage *pMsg )
{
	// This happens only on the host side, so send StartGame packets to all
	NI_VERIFY( IsGameHost(), "PRG: Should be a host to start the game", return true );
	SendStartGamePacket();
	StartGame();
	return true;
}

bool CMPManagerMode::OnUpdateGameListMessage( SMPUIGameListMessage *pMsg )
{
	if ( gamesUpdate.bUpdating != pMsg->bSendUpdates )
	{
		// When turning on, do NOT do complete refresh, to minimize traffic
		//gamesUpdate.dwVersion = 0;
		//games.clear();
		gamesUpdate.timeNextUpdate = 0;

		if ( pMsg->bSendUpdates && games.size() > 0 )
		{
			SMPUIGameListMessage *pGamesMsg = new SMPUIGameListMessage;
			for ( CNetGameList::iterator it = games.begin(); it != games.end(); ++it )
			{
				SNetGameInfo &ownGame = *it;
				AddGameInfoForUI( &(pGamesMsg->gamesAddChange), ownGame );
			}
			PushMessage( pGamesMsg );
		}
	}
	gamesUpdate.bUpdating = pMsg->bSendUpdates;
	return true;
}

bool CMPManagerMode::OnUpdateSlotMessage( SMPUIUpdateSlotMessage *pMsg )
{
	NI_VERIFY( pMsg->nSlot >= 0 && pMsg->nSlot < slots.size(), "PRG: Player slot index out of bounds", return true );

	SMPSlot &oldSlot = slots[pMsg->nSlot];
	SMPSlot &newSlot = pMsg->info;

	if ( IsInGameRoom() )
	{	
		// Process slot open/close
		if ( IsGameHost() && newSlot.bPresent == false )
		{
			if ( oldSlot.bPresent )				// Kick player
			{
				KickPlayerFromSlot( pMsg->nSlot );
				RemoveClient( oldSlot.nClientID, true );
				--nSlotsUsed;
			}
		}
		uint16_t wOldConnectMask = oldSlot.wConnectedTo;
		oldSlot = newSlot;
		oldSlot.wConnectedTo = wOldConnectMask;
		pClient->SendGamePacket( new CB2SlotInfoPacket( 0, pMsg->nSlot, newSlot ), true );
	}
	else
	{
		NI_ASSERT( 0, "PRG: Wrong state to receive slot info message" );
	}

	return true;
}

bool CMPManagerMode::OnLagMessage( SMPUILagMessage *pMsg )
{
	const uint32_t dwLaggersBefore = dwLaggers;
	if ( bInitialLoadInProgress && !pMsg->bInitialWait )
	{
		dwLaggersOld = 0;
		bInitialLoadInProgress = false;
	}
	else if ( dwLaggers == pMsg->dwLaggingPlayers )
	{
		if ( pMsg->dwLaggingPlayers == 0 )
			ShowWaitWindow( false );
		return true;
	}

	dwLaggers = pMsg->dwLaggingPlayers;
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"STATE",
		"OnLagMessage",
		GetOwnClientID(),
		StrFmt( "initial=%d pre_laggers=%08X post_laggers=%08X", pMsg->bInitialWait ? 1 : 0, dwLaggersBefore, dwLaggers ) );
	if ( pMsg->dwLaggingPlayers == 0 )								// Turning it off
		ShowWaitWindow( false );
	else if ( dwLaggersOld == 0 )											// Turning it on
		ShowWaitWindow( true );

	//DebugTrace( "*** LAG STATUS: %x ", pMsg->dwLaggingPlayers );

	return true;
}

bool CMPManagerMode::OnLeaveGameMessage( SMPUIMessage *pMsg )
{
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"DECISION",
		"OnLeaveGameMessage",
		GetOwnClientID(),
		StrFmt( "running=%d", IsGameRunning() ? 1 : 0 ) );
	if ( IsGameRunning() )
	{
		if ( IsGameControlHost() && IsValid( pTransceiver ) )
			BroadcastSynchronizedPlayerDrop( nOwnSlot, pTransceiver->GetCurrentCommonSegment(), "control_host_leave" );
		OnSurrender();
		LoseGame();
	}
	return true;
}

bool CMPManagerMode::OnInterruptMessage( SMPUIMessage *pMsg )
{
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"DECISION",
		"OnInterruptMessage",
		GetOwnClientID(),
		"interrupt=1" );
	OnLeaveGame();
	return true;
}

bool CMPManagerMode::OnPauseMessage( SMPUIMessage *pMsg )
{
	if ( !IsGameRunning() )
		return true;

	bool bPausedNow = IsPlayerLagging( nOwnSlot );
	NTimer::STime curTime = GameTimer()->GetAbsTime();
	if ( !bPausedNow && lags[nOwnSlot].nLagLeft < NGameX::GetMPConsts()->nTimeUserMPLag * 1000 )
		return true;

	const bool bPauseOn = !bPausedNow;
	if ( nOwnSlot >= 0 && nOwnSlot < slots.size() )
	{
		if ( bPauseOn )
			dwUserPausedPlayers |= ( 1UL << nOwnSlot );
		else
			dwUserPausedPlayers &= ~( 1UL << nOwnSlot );
		pClient->SendGamePacket( new CB2UserPausePacket( 0, bPauseOn ), true );
		NGameX::MatchPacketTrace_Log(
			IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
			"TX",
			"CB2UserPausePacket",
			GetOwnClientID(),
			StrFmt( "slot=%d paused=%d user_pause_mask=%08X", nOwnSlot, bPauseOn ? 1 : 0, dwUserPausedPlayers ) );
	}

	pTransceiver->CommandTimeOut( bPauseOn );
	if ( bPausedNow )							// Pause off
	{
		dwLaggers &= ~( 1UL << nOwnSlot );
		int nTimeLeft = lags[nOwnSlot].nLagLeft - ( curTime - lags[nOwnSlot].timeStartLag );
		lags[nOwnSlot].nLagLeft = (std::max)( nTimeLeft, 0 );
		lags[nOwnSlot].dwHatedBy = 0;
		lags[nOwnSlot].timeStartLag = 0;
	}
	else													// Pause on
	{
		dwLaggers |= ( 1UL << nOwnSlot );
		lags[nOwnSlot].timeStartLag = curTime;
	}

	ShowWaitWindow( !bPausedNow );
	return true;
}

bool CMPManagerMode::OnInGameChatMessage( SMPUIInGameChatMessage *pMsg )
{
	std::wstring wszFilteredText = InterfaceState()->FilterMPChatText( pMsg->wszText );
	for ( int i = 0; i < slots.size(); ++i )
	{
		SMPSlot &slot = slots[i];
		if ( !slot.bPresent )
			continue;
		if ( pMsg->bTeamOnly && slot.nTeam != slots[nOwnSlot].nTeam )
			continue;

		pClient->SendGamePacket( new CChatPacket( slot.nClientID, wszFilteredText, szMPName, slot.nClientID, !pMsg->bTeamOnly ), false );
	}

	auto scenario = Singleton<IScenarioTracker>();

	int playerID = -1;
	const auto& players = scenario->GetMultiplayerInfo()->players;
	for(int i = 0; i < players.size(); i++)
	{
		if (players[i].wszName == NStr::ToUnicode(szMPName))
		{
			playerID = players[i].nIndex;
			break;
		}
	}

	std::wstring colorStr = L"", colorEndStr = L"";
	if (playerID >= 0)
	{
		uint32_t color = scenario->GetPlayerColor(playerID).dwColor;
		colorStr = L"<color = " + string_conversion::utf8_to_wstring(string_conversion::RGBA_to_hex(color)) + L">";
		colorEndStr = L"<color = FFFFFFFF>";
	}

	std::wstring teamOnly = L"";
	if (pMsg->bTeamOnly)
		teamOnly = L" (Team)";

	std::wstring txt = L"<font size = 16pt outlinesize = 1 outlinecolor = black forcefontsize = 1>" + colorStr + NStr::ToUnicode( szMPName ) + colorEndStr + teamOnly + L": " + wszFilteredText;

	InterfaceState()->AddMPChatMessage( txt );
	return true;
}


