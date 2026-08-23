#include "stdafx.h"

#include "MPManagerMode.h"
#include "InterfaceState.h"
#include "MPTransceiver.h"
#include "ScenarioTracker.h"
#include "Misc/StrProc.h"
#include "AILogic/B2AI.h"
#include "Input/Bind.h"
#include "Client/ServerClientInterface.h"
#include "DBMPConsts.h"
#include "GetConsts.h"
#include "MultiplayerNetPackets.h"
#include "MPPacketTraceLog.h"
#include "SceneB2/Scene.h"
#include "Misc/Win32Random.h"
#include "CommandsHistory.h"
#include "System/Text.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "port/time.h"

#include <cstdint>

#include <fmt/format.h>

// CMPManagerMode - game control - scoring, different modes, win/lose conditions, etc

void CMPManagerMode::StartGame()
{
	// Do all initialising
	SB2StartGameParams tranParams;
	tranParams.pMapInfo = gameDesc.pMPMap->pMap;
	tranParams.clients.resize( nSlotsUsed );
	tranParams.nSpeedAdjustment = gameDesc.nGameSpeed;

	// Scenario tracker
	InterfaceState()->VerifyScenarioTracker( IInterfaceState::ESTT_NONE );
	InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_MULTI );
	IScenarioTracker *pScenarioTracker = Singleton<IScenarioTracker>();

	if ( nOwnSlot >= 0 && nOwnSlot < slots.size() )
		slots[nOwnSlot].nClientID = GetOwnClientID();

	Scene()->ResetTimer( GetCurrentTimeMilliseconds() );
	IScenarioTracker::SMultiplayerInfo scenarioInfo;
	pScenarioTracker->SetGameType( IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL );
	pScenarioTracker->MissionStart( gameDesc.pMPMap->pMap, gameDesc.nTechLevel );

	dwLaggers = 0;
	dwLaggersOld = 0;
	dwUserPausedPlayers = 0;
	bWaitWindowShown = false;
	bInitialLoadInProgress = true;
	lags.resize( slots.size() );
	// Track already-scheduled removals to keep host announcements idempotent.
	scheduledDropSegmentBySlot.assign( slots.size(), -1 );
	lagsUpdate.bUpdating = false;
	lagsUpdate.timeUpdatePeriod = 500;
	int nMaxLagTime = ( NGameX::GetMPConsts()->nTimeUserMPLag + NGameX::GetMPConsts()->nTimeUserMPPause ) * 1000;

	dwInitialPlayers = 0;
	int nTranSlot = 0;
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( !IsPlayerPresent( i ) )
			continue;
		SMPSlot &slot = slots[i];

		dwInitialPlayers |= 1UL << i;

		lags[i].nLagLeft = nMaxLagTime;
		lags[i].timeStartLag = 0;

		pScenarioTracker->AddPlayer( i );
		pScenarioTracker->SetPlayerSide( i, slot.nTeam );
		pScenarioTracker->SetPlayerParty( i, slot.nCountry );
		pScenarioTracker->SetPlayerColour( i, slot.nColour );

		IScenarioTracker::SMultiplayerInfo::SPlayer &scenarioPlayer = scenarioInfo.players.emplace_back();
		scenarioPlayer.wszName = NStr::ToUnicode( slot.szName );
		scenarioPlayer.nTeam = slot.nTeam;
		scenarioPlayer.nIndex = i;
		scenarioPlayer.nCountry = slot.nCountry;
		scenarioPlayer.nLevel = 1;
		scenarioPlayer.wszRank = L"";

		if ( nTranSlot < tranParams.clients.size() )
		{
			tranParams.clients[nTranSlot].nClientID = slot.nClientID;
			tranParams.clients[nTranSlot].nPlayer = i;
			tranParams.clients[nTranSlot].nTeam = slot.nTeam;
			++nTranSlot;
		}
		NI_ASSERT( nTranSlot <= nSlotsUsed, "PRG: Incorrectly counted used slots." );
	}
	pScenarioTracker->SetMultiplayerInfo( scenarioInfo );
	NI_ASSERT( nTranSlot == nSlotsUsed, fmt::format( "PRG: Incorrectly counted used slots - {}/{}.", nTranSlot, nSlotsUsed ) );
	pScenarioTracker->SetLocalPlayer( nOwnSlot );

	NGlobal::SetVar( "multiplayer_time_limit", -1 );
	NGlobal::SetVar( "multiplayer_loss_timeout", -1 );
	timeNextInstaLoseCheck = 0;

	// Set AI.MultiplayerCaptureTime
	NGlobal::SetVar( "AI.MultiplayerCaptureTime", gameDesc.nCaptureTime * 1000 );

	NGlobal::SetVar( "multiplayer_unit_experience", gameDesc.bUnitExp ? 1 : 0 );

	// Create transciever
	pTransceiver = new CMPTransceiver;
	pTransceiver->Init( pClient, tranParams, nOwnSlot );
	pTransceiver->StartMission();

	// Start a fresh per-client trace file for this match with stable identifiers and masks.
	std::vector<NGameX::SMatchPacketTraceSlot> traceSlots;
	traceSlots.reserve( slots.size() );
	for ( int i = 0; i < slots.size(); ++i )
	{
		NGameX::SMatchPacketTraceSlot traceSlot;
		traceSlot.nSlot = i;
		traceSlot.nClientID = slots[i].nClientID;
		traceSlot.nTeam = slots[i].nTeam;
		traceSlot.bPresent = slots[i].bPresent;
		traceSlots.push_back( traceSlot );
	}
	const std::string szMapName = gameDesc.pMPMap ? NStr::ToMBCS( GET_TEXT_PRE( gameDesc.pMPMap->, MapName ) ) : std::string( "unknown" );
	NGameX::MatchPacketTrace_Reset();
	NGameX::MatchPacketTrace_SetHeader(
		nGameID,
		szSessionName,
		szMapName,
		ulGameCheckSum,
		GetOwnClientID(),
		nOwnSlot,
		nHostClientID,
		dwInitialPlayers,
		GetPresentMask(),
		IsValid( pTransceiver ) ? pTransceiver->GetPlayerMask() : 0,
		traceSlots );
	NGameX::MatchPacketTrace_Log(
		pTransceiver->GetCurrentCommonSegment(),
		"STATE",
		"StartGame",
		GetOwnClientID(),
		fmt::format( "slots_used={} initial_players={:08X}", nSlotsUsed, dwInitialPlayers ) );

	timeEndMatch = 0;
	bOutcomeKnown = false;
}

void CMPManagerMode::WinGame()
{
//	WriteToPipe( PIPE_CHAT, StrFmt( "You won the game" ) );

	Singleton<IScenarioTracker>()->MissionWin();
	nWinningSide = slots[nOwnSlot].nTeam;
	NInput::PostEvent( "multiplayer_win", 0, 0 );
	pTransceiver->EndGame();
	EndGame();
}

void CMPManagerMode::LoseGame()
{
//	WriteToPipe( PIPE_CHAT, StrFmt( "You lost the game" ) );

	Singleton<IScenarioTracker>()->MissionCancel();
	nWinningSide = ( slots[nOwnSlot].nTeam == 0 ) ? 1 : 0;
	NInput::PostEvent( "multiplayer_loose", 0, 0 );
	pTransceiver->EndGame();
	EndGame();
}

void CMPManagerMode::ScheduleWinGame()
{
	if ( bOutcomeKnown || !IsValid( pTransceiver ) )
		return;

	NGameX::MatchPacketTrace_Log(
		pTransceiver->GetCurrentCommonSegment(),
		"DECISION",
		"ScheduleWinGame",
		GetOwnClientID(),
		fmt::format( "game_id={}", nGameID ) );
	pTransceiver->ScheduleGameEnd( 0 );
	bWinOnGameEnd = true;
	bOutcomeKnown = true;
}

void CMPManagerMode::ScheduleLoseGame()
{
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"TX",
		"CB2GameLostPacket",
		GetOwnClientID(),
		fmt::format( "game_id={}", nGameID ) );
	CB2GameLostPacket *pPkt = new CB2GameLostPacket( 0, nGameID, pTransceiver->ScheduleGameEnd( 0 ) );
	pClient->SendGamePacket( pPkt, true );
	bWinOnGameEnd = false;
	bOutcomeKnown = true;
}

std::string GenerateDateTimeReplayFilename() {
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    // Convert to local time
    std::tm local_time = *std::localtime(&time_t);
    
    // Create string stream for formatting
    std::ostringstream oss;
    
    // Format: "MatchFrom_YYYY-MM-DD_HOURS-MINS-SECONDS"
    oss << "MatchFrom_"
        << std::put_time(&local_time, "%Y-%m-%d_%H-%M-%S");
    
    return oss.str();
}

void CMPManagerMode::EndGame()
{
	NGameX::MatchPacketTrace_Log(
		IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
		"STATE",
		"EndGame",
		GetOwnClientID(),
		fmt::format( "winning_side={}", nWinningSide ) );
	NGameX::MatchPacketTrace_SetFinalState( GetPresentMask(), dwLaggers, IsValid( pTransceiver ) ? pTransceiver->GetPlayerMask() : 0 );
	NGameX::MatchPacketTrace_Flush( "match_end" );

	if ( IsValid( pTransceiver ) )
		pCommandsHistory = dynamic_cast<CCommandsHistory*>(pTransceiver->GetCommandsHistory());
	else
		pCommandsHistory = 0;
	// Improve the replay file name
	std::string Filename = GenerateDateTimeReplayFilename();
	SaveReplay( Filename );
	pTransceiver = 0;
}

void CMPManagerMode::CheckEndGameConditions()
{
	if ( !IsGameRunning() )
		return;

	if ( bOutcomeKnown )
	{
		if ( pTransceiver->IsGameEnded() )
		{
			if ( bWinOnGameEnd )
				WinGame();
			else
				LoseGame();
		}
		return;
	}

	if ( CheckAllLeftWin() )
		return;

	if ( CheckKeyBuildingsWinLose() )
		return;

	if ( CheckScoreWinLose() )
		return;

	if ( CheckInstantLose() )
	return;

	return;
}

bool CMPManagerMode::CheckAllLeftWin()
{
	if ( !pTransceiver || pTransceiver->IsGameEnded() )
		return true;
	if ( IsGameRunning() )
	{
		int nOppositePlayers = 0;
		for ( int i = 0; i < slots.size(); ++i )
		{
			if ( IsPlayerPresent( i ) && slots[nOwnSlot].nTeam + slots[i].nTeam == 1 )	
				++nOppositePlayers;		// one of them is 1 and another is 0, i.e. enemies
		}

		if ( nOppositePlayers == 0 )
		{
			ScheduleWinGame();
			return true;
		}
	}
	return false;
}

bool CMPManagerMode::CheckScoreWinLose()
{
	IGameTimer *pTimer = Singleton<IGameTimer>();
	NTimer::STime curGameTime = pTimer->GetGameTime();
	float fTimeSpeedMultiplier = 1.0f;

	if ( gameDesc.nGameSpeed > 0 )
		fTimeSpeedMultiplier += gameDesc.nGameSpeed;
	else if ( gameDesc.nGameSpeed < 0 )
		fTimeSpeedMultiplier /= 1 - gameDesc.nGameSpeed;

	if ( timeEndMatch == 0 )
		timeEndMatch = curGameTime + gameDesc.nTimeLimit * 60000 * fTimeSpeedMultiplier;

	if ( curGameTime > timeEndMatch )
		NGlobal::SetVar( "multiplayer_time_limit", -1 );
	else
		NGlobal::SetVar( "multiplayer_time_limit", int(timeEndMatch) / 1000 );

	if ( timeEndMatch > 0 && curGameTime > timeEndMatch )
	{
		int nLostTeam = GetTeamWithLowestScore();
		int nOwnTeam = slots[nOwnSlot].nTeam;

		if ( nOwnTeam == nLostTeam )
		{
//			WriteToPipe( PIPE_CHAT, StrFmt( "Game time over, lost by score" ) );
			ScheduleLoseGame();
			return true;
		}
	}
	return false;
}

int CMPManagerMode::GetTeamWithLowestScore()
{
	IScenarioTracker *pScenarioTracker = Singleton<IScenarioTracker>();
	pScenarioTracker->UpdateStatistics();

	int nScores[2];
	nScores[0] = 0;
	nScores[1] = 0;
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( !IsPlayerPresent( i ) )
			continue;
		nScores[slots[i].nTeam] += pScenarioTracker->GetStatistics( i, IScenarioTracker::ESK_SCORE );
	}

	if ( nScores[0] == nScores[1] )
		nScores[slots[0].nTeam] += 1;

	if ( nScores[0] > nScores[1] )
		return 1;
	else
		return 0;
}

bool CMPManagerMode::CheckKeyBuildingsWinLose()
{
	IGameTimer *pTimer = Singleton<IGameTimer>();
	NTimer::STime curGameTime = pTimer->GetGameTime();
	IScenarioTracker *pScenarioTracker = Singleton<IScenarioTracker>();

	std::pair<int, int> flags = pScenarioTracker->GetKeyBuildingSummary();

	int nNewWinningSide = 2;
	if ( flags.first > 0 && flags.second == 0 )
		nNewWinningSide = 0;
	else if ( flags.first == 0 && flags.second > 0 )
		nNewWinningSide = 1;

	int nSecondsLeft = -1;
	if ( nNewWinningSide == nWinningSide )
	{
		if ( nNewWinningSide != 2 )
		{
			nSecondsLeft = (std::max) ( 0, int( ( int(timeResolution) - int(curGameTime) ) / 1000 ) );
			if ( curGameTime > timeResolution )
			{
				if ( nWinningSide != slots[nOwnSlot].nTeam )
				{
//					WriteToPipe( PIPE_CHAT, StrFmt( "Lost all keypoints, timeout exceeded" ) );
					ScheduleLoseGame();
					return true;
				}
			}
		}
	}
	else
	{
		if ( nNewWinningSide != 2 )
		{
			nSecondsLeft = 60;
			timeResolution = curGameTime + nSecondsLeft * 1000;
		}
		else
			timeResolution = 0;
	}
	nWinningSide = nNewWinningSide;

	NGlobal::SetVar( "multiplayer_loss_timeout", nSecondsLeft );
	return false;
}

bool CMPManagerMode::CheckInstantLose()
{
	IGameTimer *pTimer = Singleton<IGameTimer>();
	NTimer::STime curGameTime = pTimer->GetGameTime();
	IScenarioTracker *pScenarioTracker = Singleton<IScenarioTracker>();
	int nOwnTeam = slots[nOwnSlot].nTeam;

	if ( curGameTime > timeNextInstaLoseCheck )
	{
		timeNextInstaLoseCheck = curGameTime + 5000;

		if ( nWinningSide + nOwnTeam == 1 && Singleton<IAILogic>()->HasPlayerNoUnits( nOwnSlot ) )
		{
//			WriteToPipe( PIPE_CHAT, StrFmt( "All keypoints lost, instant loss" ) );
			ScheduleLoseGame();
			return true;
		}
	}
	return false;
}

void CMPManagerMode::AnalyzeLaggers()
{
	NTimer::STime curTime = GameTimer()->GetAbsTime();

	// TODO: inspect!

	if ( bInitialLoadInProgress )
	{
		if ( lagsUpdate.CheckNeedUpdate() )
			SendLagInfo();
		return;
	}

	// Analyze my lag, if any
	// This should only happen if I set the pause myself
	if ( IsPlayerLagging( nOwnSlot ) )
	{
		SLagInfo &lagInfo = lags[nOwnSlot];
		int nTimeLeft = lagInfo.nLagLeft - ( curTime - lagInfo.timeStartLag );

		if ( nTimeLeft < NGameX::GetMPConsts()->nTimeUserMPLag )			// Too little time left, remove pause
		{
			lagInfo.nLagLeft = (std::max)( nTimeLeft, 0 );
			pTransceiver->CommandTimeOut( false );
			dwLaggers &= ~( 1UL << nOwnSlot );
			ShowWaitWindow( false );
		}
	}

	uint32_t dwPlayers = 0;
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( IsPlayerPresent( i ) )
			dwPlayers |= ( 1UL << i );
	}

	// Analyze other laggers
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( i == nOwnSlot || !IsPlayerPresent( i ) )
			continue;

		SLagInfo &lagInfo = lags[i];
		const bool bLaggingGameControlHost = ( GetSlotClientID( i ) == nHostClientID );
		const bool bUserPaused = ( dwUserPausedPlayers & ( 1UL << i ) ) != 0;

		if ( HasPlayerStartedLagging( i ) )
		{
			lagInfo.timeStartLag = curTime;
			//DebugTrace( "*** LAG START for player %d at time %d", i, curTime );
			if ( bLaggingGameControlHost && !bUserPaused && IsValid( pTransceiver ) &&
				i >= 0 && i < scheduledDropSegmentBySlot.size() &&
				scheduledDropSegmentBySlot[i] < 0 )
			{
				const int nReplacementHostClientID = GetReplacementHostClientID( nHostClientID );
				if ( nReplacementHostClientID >= 0 && GetOwnClientID() == nReplacementHostClientID )
				{
					const int nOldHostClientID = nHostClientID;
					PromoteGameControlHostAfterRemoval( nOldHostClientID );
					const int nDropSegment = pTransceiver->GetCurrentCommonSegment();
					NGameX::MatchPacketTrace_Log(
						nDropSegment,
						"DECISION",
						"HostLagDropAuthorityAssumed",
						GetOwnClientID(),
						fmt::format( "slot={} old_host={} new_host={}", i, nOldHostClientID, nHostClientID ) );
					ScheduleSynchronizedPlayerDrop( i, nDropSegment );
					BroadcastSynchronizedPlayerDrop( i, nDropSegment, "host_lag_timeout" );
				}
				else
				{
					NGameX::MatchPacketTrace_Log(
						pTransceiver->GetCurrentCommonSegment(),
						"DECISION",
						"HostLagAwaitingReplacementAuthority",
						GetOwnClientID(),
						fmt::format( "slot={} replacement={}", i, nReplacementHostClientID ) );
				}
			}
			else if ( bLaggingGameControlHost && bUserPaused )
			{
				NGameX::MatchPacketTrace_Log(
					IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
					"DECISION",
					"HostLagUserPauseObserved",
					GetOwnClientID(),
					fmt::format( "slot={} user_pause_mask={:08X}", i, dwUserPausedPlayers ) );
			}
		}
		else if ( HasPlayerStoppedLagging( i ) )
		{
			int nTimeLeft = lagInfo.nLagLeft - ( curTime - lagInfo.timeStartLag );
			lagInfo.nLagLeft = (std::max)( nTimeLeft, 0 );
			lagInfo.dwHatedBy = 0;
			lagInfo.timeStartLag = 0;
			dwUserPausedPlayers &= ~( 1UL << i );
			CPtr<CB2LagTimeUpdatePacket> pPkt = new CB2LagTimeUpdatePacket( 0, i, lagInfo.nLagLeft );
			NGameX::MatchPacketTrace_Log(
				IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
				"TX",
				"CB2LagTimeUpdatePacket",
				GetOwnClientID(),
				fmt::format( "player={} time_left={}", i, lagInfo.nLagLeft ) );
			pClient->SendGamePacket( pPkt, true );
			//DebugTrace( "*** LAG STOP for player %d at time %d, time left %d", i, curTime, lagInfo.nLagLeft ); 
		}
		else if ( IsPlayerLagging( i ) )
		{
			int nTimeLeft = lagInfo.nLagLeft - ( curTime - lagInfo.timeStartLag );
			if ( nTimeLeft <= 0 )
			{
				if ( !( lagInfo.dwHatedBy & ( 1UL << nOwnSlot ) ) )
				{
					CPtr<CB2SuggestKickPacket> pPkt = new CB2SuggestKickPacket( 0, i );
					NGameX::MatchPacketTrace_Log(
						IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
						"TX",
						"CB2SuggestKickPacket",
						GetOwnClientID(),
						fmt::format( "slot_to_kick={}", i ) );
					pClient->SendGamePacket( pPkt, true );
					const uint32_t dwPreVoteMask = lagInfo.dwHatedBy;
					lagInfo.dwHatedBy |= ( 1UL << nOwnSlot );
					NGameX::MatchPacketTrace_Log(
						IsValid( pTransceiver ) ? pTransceiver->GetCurrentCommonSegment() : -1,
						"DECISION",
						"SetOwnKickVote",
						GetOwnClientID(),
						fmt::format( "slot={} pre_votes={:08X} post_votes={:08X}", i, dwPreVoteMask, lagInfo.dwHatedBy ) );
				}
				// Kick voting is game-control-host authoritative; authority migrates if the original host leaves.
				const uint32_t dwEligibleVoters = ( dwPlayers & ~dwLaggers ) & ~( 1UL << i );
				if ( dwEligibleVoters != 0 && lagInfo.dwHatedBy == dwEligibleVoters )
				{
					const int nReplacementHostClientID = bLaggingGameControlHost ? GetReplacementHostClientID( nHostClientID ) : -1;
					const bool bOwnsDropAuthority =
						IsGameControlHost() || ( bLaggingGameControlHost && GetOwnClientID() == nReplacementHostClientID );
					if ( bOwnsDropAuthority && IsValid( pTransceiver ) &&
						i >= 0 && i < scheduledDropSegmentBySlot.size() &&
						scheduledDropSegmentBySlot[i] < 0 )
					{
						if ( bLaggingGameControlHost )
							PromoteGameControlHostAfterRemoval( nHostClientID );
						else
							KickPlayerFromSlot( i );
						const int nDropSegment = pTransceiver->GetCurrentCommonSegment();
						NGameX::MatchPacketTrace_Log(
							nDropSegment,
							"DECISION",
							"LagKickConsensusReached",
							GetOwnClientID(),
							fmt::format( "slot={} eligible={:08X} votes={:08X} control_host={} replacement={}",
								i, dwEligibleVoters, lagInfo.dwHatedBy, bLaggingGameControlHost ? 1 : 0, nReplacementHostClientID ) );
						ScheduleSynchronizedPlayerDrop( i, nDropSegment );
						BroadcastSynchronizedPlayerDrop( i, nDropSegment, bLaggingGameControlHost ? "host_lag_timeout" : "lag_kick" );
					}
				}
			}
		}
	}
	dwLaggersOld = dwLaggers;

	if ( lagsUpdate.CheckNeedUpdate() )
		SendLagInfo();
}

void CMPManagerMode::CreateRehash( std::vector<uint8_t> *pOrder )
{
	std::vector<uint8_t> &order = *pOrder;
	int nSize = slots.size();
	order.resize( nSize, -1 );
	for ( int i = 0; i < nSize; ++i )
		order[i] = i;

	if ( !gameDesc.bRandomPlacement )
		return;

	for ( int i = 0; i < 2 * nSize; ++i )
	{
		int nIndex1 = NWin32Random::Random() % nSize;
		int nIndex2 = NWin32Random::Random() % nSize;
		std::swap( order[nIndex1], order[nIndex2] );
	}
}

void CMPManagerMode::RehashSlots( const std::vector<uint8_t> &order )
{
	std::string szDebugOut = "+++ Rehash slots:";
	for ( int i = 0; i < order.size(); ++i )
	{
		szDebugOut += fmt::format( " {},", order[i] );
	}
	DebugTrace( "%s", szDebugOut.c_str() );

	std::vector<SMPSlot> newSlots;
	newSlots.resize( slots.size() );
	for ( int i = 0; i < slots.size(); ++i )
	{
		newSlots[order[i]] = slots[i];
		newSlots[order[i]].nClientID = slots[i].nClientID;
	}
	nOwnSlot = order[nOwnSlot];
	for ( int i = 0; i < slots.size(); ++i )
	{
		SMPSlot &slot = slots[i];
		slot = newSlots[i];
		slot.nClientID = newSlots[i].nClientID;
		if ( !slot.bPresent )
			DebugTrace( "+++ New Slot %d: empty", i );
		else
			DebugTrace( "+++ New Slot %d: %s, CID %d, side %d, team %d", i, slot.szName.c_str(), slot.nClientID, slot.nCountry, slot.nTeam );
	}
}


