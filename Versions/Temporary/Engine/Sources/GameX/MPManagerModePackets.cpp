#include "stdafx.h"

#include "MPManagerMode.h"
#include "MultiplayerNetPackets.h"
#include "Server_Client_Common/GamePackets.h"
#include "Server_Client_Common/ChatPackets.h"
#include "Server_Client_Common/CommonPackets.h"
#include "Client/ServerClientInterface.h"
#include "Misc/StrProc.h"
#include "Misc/StringConversions.h"
#include "MPTransceiver.h"
#include "ScenarioTracker.h"
#include "DBMPConsts.h"
#include "InterfaceState.h"
#include "System/WinFrame.h"
#include "MPPacketTraceLog.h"

#include "MPLANTest.h"

// CMPManagerMode - packet handling methods

bool CMPManagerMode::OnB2SlotInfoPacket( class CB2SlotInfoPacket *pPacket )
{
	DebugTrace( "+++ Slot Info packet from client %d for slot %d, name %s, conn %d", 
		pPacket->nClientID, pPacket->nSlot, pPacket->info.szName.c_str(), pPacket->info.wConnectedTo );
	// Store for myself
	NI_VERIFY( pPacket->nSlot >= 0 && pPacket->nSlot < slots.size(), "PRG: Got slot info that is outside bounds", return true );
	//NI_VERIFY( IsInGameRoom(), "PRG: Received slot info while not in game room", return true );
	if ( pPacket->info.bPresent && !IsPlayerPresent( pPacket->nSlot ) )
	{
		if ( nOwnSlot >= 0 )
			pClient->SendGamePacket( new CPingPacket( pPacket->nClientID, nOwnSlot, GameTimer()->GetAbsTime() ), false );
		// Only clients increase slot counter here, server increases it in NEW_CLIENT handler
		if ( !IsGameHost() )
			++nSlotsUsed;
	}
	else if ( !pPacket->info.bPresent && IsPlayerPresent( pPacket->nSlot ) )
	{
		--nSlotsUsed;
	}

	SMPSlot &slot = slots[pPacket->nSlot];
	int nStoredPing = slot.nPing;
	slot = pPacket->info;
	slot.nClientID = pPacket->nClientID;
	slot.nPing = nStoredPing;

	if ( nOwnSlot >= 0 )
		UpdateMyConnectivityMask();

	// Send message
	if ( bGameRoomInit && IsInGameRoom() )
	{
		SMPUIUpdateSlotMessage *pMsg = new SMPUIUpdateSlotMessage;
		pMsg->info = slot;
		pMsg->nSlot = pPacket->nSlot;
		PushMessage( pMsg );
		DebugTrace( "+++ Show Slot Info " );
	}
	else
		DebugTrace( "+++ DO NOT Show Slot Info " );
	if ( pLanTester )
		pLanTester->ClientInfoChanged( pPacket->nSlot, pPacket->info.bAccept );

	return true;
}

bool CMPManagerMode::OnGameClientRemoved( class CGameClientRemoved *pPacket )
{
	DebugTrace( "+++ Client Removed packet from client %d", pPacket->nClientID );
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"RX",
		"CGameClientRemoved",
		pPacket->nClientID,
		StrFmt( "running=%d in_room=%d", IsGameRunning() ? 1 : 0, IsInGameRoom() ? 1 : 0 ) );
	RemoveClient( pPacket->nClientID, false );
	return true;
}

bool CMPManagerMode::OnGameKilled( class CGameKilled *pPacket )
{
	DebugTrace( "+++ GameKilled packet from client %d, game %d", pPacket->nClientID, pPacket->nGame );
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"RX",
		"CGameKilled",
		pPacket->nClientID,
		StrFmt( "game=%d running=%d in_room=%d", pPacket->nGame, IsGameRunning() ? 1 : 0, IsInGameRoom() ? 1 : 0 ) );
	if ( pPacket->nGame != nGameID )
	{
		NGameX::MatchPacketTrace_Log(
			IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
			"RX",
			"CGameKilledIgnoredWrongGame",
			pPacket->nClientID,
			StrFmt( "packet_game=%d local_game=%d", pPacket->nGame, nGameID ) );
		return true;
	}

	if ( IsInGameRoom() )
	{
		NGameX::MatchPacketTrace_SetFinalState( GetPresentMask(), dwLaggers, IsValid( pTransceiver ) ? pTransceiver->GetPlayerMask() : 0 );
		NGameX::MatchPacketTrace_Flush( "game_killed" );
		PushMessage( new SMPUIGameRoomInitMessage( SMPUIGameRoomInitMessage::ERR_GAME_KILLED ) );
		OnLeaveGame();
	}
	else if ( IsGameRunning() )
	{
		NGameX::MatchPacketTrace_Log(
			IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
			"RX",
			"CGameKilledIgnoredRunningGame",
			pPacket->nClientID,
			StrFmt( "game=%d present=%08X host_id=%d", pPacket->nGame, GetPresentMask(), nHostClientID ) );
		return true;
	}

	return true;
}

bool CMPManagerMode::OnB2GameRoomStartGamePacket( class CB2GameRoomStartGamePacket *pPacket )
{
	NI_VERIFY( IsInGameRoom() && !IsGameHost(), "PRG: StartGame received by not a client", return true );

	//DebugTrace( "+++ StartGame packet from client %d", pPacket->nClientID );
	if ( pPacket->slots.size() > slots.size() )
		slots.resize( nSlotsUsed );
	RehashSlots( pPacket->slotRehash );
	for ( int i = 0; i < slots.size(); ++i )
	{
		slots[i].nCountry = pPacket->slots[i].nCountry;
		slots[i].nTeam = pPacket->slots[i].nTeam;
	}
	StartGame();

	return true;
}

bool CMPManagerMode::OnB2GameSpecificInfoPacket( class CB2GameSpecificInfoPacket *pPacket )
{
	//DebugTrace( "+++ SpecificInfo packet from client %d, players %d", pPacket->nClientID, pPacket->info.nPlayers );

	// Absolute shit (or in nival devs terms: CRAP) of solution, but it should work really well when detecting if the client has the MP map or not
	if (pPacket->info.pMPMap.GetBarePtrNoLoad() == nullptr)
		pPacket->info.nPlayers = -1;

	if ( !IsInGameRoom() || IsGameHost() )
		return true;

	gameDesc = pPacket->info;
	OnGameSpecificInfo();

	return true;
}

bool CMPManagerMode::OnChatPacket( class CChatPacket *pPacket )
{
	if ( IsGameRunning() )
	{
		auto scenario = Singleton<IScenarioTracker>();

		int playerID = -1;
		const auto& players = scenario->GetMultiplayerInfo()->players;
		auto nick = NStr::ToUnicode(pPacket->szNick);
		for(int i = 0; i < players.size(); i++)
		{
			if (players[i].wszName == nick)
			{
				playerID = players[i].nIndex;
				break;
			}
		}

		std::wstring colorStr = L"", colorEndStr = L"";
		if (playerID >= 0)
		{
			DWORD color = scenario->GetPlayerColor(playerID).dwColor;
			colorStr = L"<color = " + string_conversion::utf8_to_wstring(string_conversion::RGBA_to_hex(color)) + L">";
			colorEndStr = L"<color = FFFFFFFF>";
		}

		std::wstring teamOnly = L"";
		if (!pPacket->bIsBroadcast)
			teamOnly = L" (Team)";

		std::wstring txt = L"<font size = 16pt outlinesize = 1 outlinecolor = black forcefontsize = 1>" + colorStr + nick + colorEndStr + teamOnly + L": " + pPacket->wszMessage;

		InterfaceState()->AddMPChatMessage( txt );

		// notify the user if the app is minimized or unfocused
		NWinFrame::FlashTaskbarIfInactive();

		return true;
	}
	CPtr<SMPUIChatMessage> pMsg = new SMPUIChatMessage( pPacket->szNick, pPacket->wszMessage );
	pMsg->bPrivate = !pPacket->bIsBroadcast;
	PushMessage( pMsg );

	return true;
}

bool CMPManagerMode::OnSlotNumberPacket( class CSlotNumberPacket *pPacket )
{
	DebugTrace( "+++ SlotNumber packet from client %d, assigned slot %d", pPacket->nClientID, pPacket->nSlot );
	nHostClientID = pPacket->nClientID;
	nOwnSlot = pPacket->nSlot;
	++nSlotsUsed;
	slots[nOwnSlot].szName = szMPName;
	slots[nOwnSlot].nClientID = GetOwnClientID();
	slots[nOwnSlot].nTeam = nOwnSlot % 2;
	slots[nOwnSlot].nColour = nOwnSlot % ( (pMPConsts) ? pMPConsts->playerColorInfos.size() : 4 );
	slots[nOwnSlot].bPresent = true;
	ulHostCheckSum = pPacket->ulCheckSum;

	for ( std::list<int>::iterator it = pendingClients.begin(); it != pendingClients.end(); ++it )
		ClientAssignNewClient( *it );
	pendingClients.clear();
	OnSetMySlotNumber();
	return true;
}

bool CMPManagerMode::OnConnectGameFailedPacket( class CConnectGameFailed *pPacket )
{
	PushMessage( new SMPUIGameRoomInitMessage( SMPUIGameRoomInitMessage::ERR_GAME_FULL ) );
	OnLeaveGame();

	return true;
}

bool CMPManagerMode::OnB2SuggestKickPacket( class CB2SuggestKickPacket *pPacket )
{
	// Vote accounting must use packet sender identity, not local slot order, to stay deterministic across peers.
	const int nVoterSlot = GetSlotByClientID( pPacket->nClientID );
	if ( nVoterSlot < 0 || nVoterSlot >= slots.size() || !IsPlayerPresent( nVoterSlot ) )
		return true;

	const int nSlotToKick = pPacket->nSlotToKick;
	if ( nSlotToKick < 0 || nSlotToKick >= slots.size() || !IsPlayerPresent( nSlotToKick ) )
		return true;

	const DWORD dwPreVotes = lags[nSlotToKick].dwHatedBy;
	lags[nSlotToKick].dwHatedBy |= ( 1UL << nVoterSlot );
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"RX",
		"CB2SuggestKickPacket",
		pPacket->nClientID,
		StrFmt( "slot_to_kick=%d voter_slot=%d pre_votes=%08X post_votes=%08X", nSlotToKick, nVoterSlot, dwPreVotes, lags[nSlotToKick].dwHatedBy ) );

	return true;
}

bool CMPManagerMode::OnB2LagTimeUpdatePacket( class CB2LagTimeUpdatePacket *pPacket )
{
	if ( !IsGameRunning() || !IsPlayerPresent( pPacket->nPlayer ) )
		return true;

	SLagInfo &lag = lags[pPacket->nPlayer];
	//DebugTrace( "*** LAG UPDATE player %d, time %d --> %d", pPacket->nPlayer, lag.nLagLeft, pPacket->nTimeLeft );
	const int nPreLagLeft = lag.nLagLeft;
	lag.nLagLeft = (std::min)( pPacket->nTimeLeft, int( lag.nLagLeft ) );
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"RX",
		"CB2LagTimeUpdatePacket",
		pPacket->nClientID,
		StrFmt( "player=%d pre_time_left=%d post_time_left=%d", pPacket->nPlayer, nPreLagLeft, lag.nLagLeft ) );

	return true;
}

bool CMPManagerMode::OnB2UserPausePacket( class CB2UserPausePacket *pPacket )
{
	if ( !IsGameRunning() )
		return true;

	const int nSlot = GetSlotByClientID( pPacket->nClientID );
	if ( nSlot < 0 || nSlot >= slots.size() || !IsPlayerPresent( nSlot ) )
		return true;

	const DWORD dwPreUserPausedPlayers = dwUserPausedPlayers;
	if ( pPacket->bPaused )
		dwUserPausedPlayers |= ( 1UL << nSlot );
	else
		dwUserPausedPlayers &= ~( 1UL << nSlot );
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"RX",
		"CB2UserPausePacket",
		pPacket->nClientID,
		StrFmt( "slot=%d paused=%d pre_user_pause_mask=%08X post_user_pause_mask=%08X",
			nSlot, pPacket->bPaused ? 1 : 0, dwPreUserPausedPlayers, dwUserPausedPlayers ) );

	return true;
}

bool CMPManagerMode::OnB2DropPlayerAtSegmentPacket( class CB2DropPlayerAtSegmentPacket *pPacket )
{
	if ( !IsGameRunning() )
		return true;

	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"RX",
		"CB2DropPlayerAtSegmentPacket",
		pPacket->nClientID,
		StrFmt( "slot=%d target_seg=%d host_id=%d", int( pPacket->nSlotToDrop ), pPacket->nSegment, nHostClientID ) );

	const int nSlotToDrop = pPacket->nSlotToDrop;
	const bool bDropsCurrentHost =
		nSlotToDrop >= 0 && nSlotToDrop < slots.size() &&
		GetSlotClientID( nSlotToDrop ) == nHostClientID;

	if ( !IsAuthoritativeDropPacket( pPacket ) )
	{
		NGameX::MatchPacketTrace_Log(
			IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
			"RX",
			"CB2DropPlayerAtSegmentPacketRejected",
			pPacket->nClientID,
			StrFmt( "slot=%d target_seg=%d host_id=%d", int( pPacket->nSlotToDrop ), pPacket->nSegment, nHostClientID ) );
		return true;
	}

	int nDropSegment = pPacket->nSegment;
	if ( bDropsCurrentHost && IsValid( pTransceiver ) && nDropSegment > pTransceiver->GetCurrentCommonSegment() )
	{
		nDropSegment = pTransceiver->GetCurrentCommonSegment();
		NGameX::MatchPacketTrace_Log(
			nDropSegment,
			"DECISION",
			"ClampHostDropToLocalSegment",
			GetOwnClientID(),
			StrFmt( "slot=%d packet_seg=%d local_seg=%d", nSlotToDrop, pPacket->nSegment, nDropSegment ) );
	}

	ScheduleSynchronizedPlayerDrop( pPacket->nSlotToDrop, nDropSegment );
	return true;
}

bool CMPManagerMode::OnB2GameLostPacket( class CB2GameLostPacket *pPacket )
{
	if ( !IsGameRunning() || bOutcomeKnown || pPacket->nGameID != nGameID )
		return true;

	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"RX",
		"CB2GameLostPacket",
		pPacket->nClientID,
		StrFmt( "game_id=%d segment=%d", pPacket->nGameID, pPacket->nSegment ) );

	//DebugTrace( "+++ GameLost packet" );
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( i == nOwnSlot )
			continue;

		if ( slots[i].nClientID == pPacket->nClientID )
		{
			pTransceiver->ScheduleGameEnd( pPacket->nSegment );
			//DebugTrace( "+++ Game Lost for team %d (from slot %d) scheduled", slots[i].nTeam, i );
			bOutcomeKnown = true;
			bWinOnGameEnd = ( slots[nOwnSlot].nTeam != slots[i].nTeam );
			break;
		}
	}

	return true;
}

bool CMPManagerMode::OnPingPacket( class CPingPacket *pPacket )
{
	if ( pPacket->nFromID != nOwnSlot )		// Pong
	{
		pClient->SendGamePacket( new CPingPacket( pPacket->nClientID, pPacket->nFromID, pPacket->nSendTime ), false );
		return true;
	}

	if ( !IsInGameRoom() )
		return true;

	NTimer::STime curTime = GameTimer()->GetAbsTime();
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( i == nOwnSlot )
			continue;
		SMPSlot &slot = slots[i];
		if ( slot.nClientID == pPacket->nClientID )
		{
			const int nTimeDiff = curTime - pPacket->nSendTime;
			slot.nPing = (std::max)( 0, nTimeDiff );
			if ( bGameRoomInit )
			{
				SMPUIUpdateSlotMessage *pMsg = new SMPUIUpdateSlotMessage;
				pMsg->info = slot;
				pMsg->nSlot = i;
				PushMessage( pMsg );
			}
			break;
		}
	}

	return true;
}


