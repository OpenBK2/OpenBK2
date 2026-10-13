#include "stdafx.h"

#include "CommandsHistory.h"
#include "Transceiver.h"

#include "AILogic/AILogicCommand.h"
// IAILogic, for the checksum dump written when a playback diverges. Transceiver.h
// only forward declares it.
#include "AILogic/B2AI.h"
#include "Misc/StrProc.h"
#include "Main/Profiles.h"
// CreateICExitGame, for ending a verification run once the replay is played out.
#include "Main/MainLoopCommands.h"
#include "GameX/ScenarioTracker.h"
#include "GameX/InterfaceState.h"
#include "System/BuildStamp.h"
#include "System/GlobalVars.h"
#include "MultiplayerCommandManager.h"
#include "Input/Bind.h"
#include "SaveLoadHelper.h"
#include "GameXClassIDs.h"
#include "InterfaceMisc.h"

#include "GameX_export.h"

#include <fmt/format.h>

bool g_nSaveToSameSpace;

START_REGISTER(CommandHistory)
REGISTER_VAR_EX( "checksum_overwrite",	NGlobal::VarBoolHandler, &g_nSaveToSameSpace, false, STORAGE_NONE );
FINISH_REGISTER

// Verification output has to reach a run that shows no window and never opens
// the in-game console, so it goes to stdout as well, the way -show-version
// already prints. The console line is marked persistent because
// CConsoleBuffer::DumpLog only writes those to log.txt, and the default is not
// persistent, which is why these were going nowhere visible.
static void ReportReplayLine( const std::string &szLine )
{
	fmt::print( "{}\n", szLine );
	fflush( stdout );
	if ( IConsoleBuffer *pConsole = Singleton<IConsoleBuffer>() )
		pConsole->WriteASCII( CONSOLE_STREAM_CONSOLE, szLine.c_str(), 0xffffffff, true );
}

// CCommandsHistory

CCommandsHistory::CCommandsHistory() : bFinishedHistory( false ), bGameFinished( true ), bLoadedHistory( false ),
	bCanAddCommand( true ), dwLastCheckSum( 0 )
{
	pStartSeed = NRandom::CreateRandomSeedCopy();
	pStartSeed->Init();
	NRandom::SetRandomSeed( pStartSeed );
}

void CCommandsHistory::StartNewGame( const NDb::SMapInfo *_pMap )
{
	savingHistory.clear();
	if ( !bLoadedHistory )
	{
		loadedHistory.clear();
		replayInfo.pMap = _pMap;
		replayInfo.nLastCommandSegment = 0;
		replayInfo.nLastGameSegment = 0;
		// Captured at mission start rather than when the replay is written,
		// because these can be changed from the console mid-game and what
		// matters is what produced the samples.
		recordedCheckSums = SReplayCheckSums();
		recordedCheckSums.nLightMode = NGlobal::GetVar( "light_checksum_count", 0 );
		recordedCheckSums.nPeriod = NGlobal::GetVar( "checksumperiod", 1000 );
	}
}

const bool CCommandsHistory::SerializeHistory( const std::string &szFileName, const bool bRead )
{
	CFileStream streamMain( fmt::format( "{}{}{}", NSaveLoad::GetReplayPath(), szFileName, NSaveLoad::REPLAY_EXTENSION ), bRead ? CFileStream::WIN_READ_ONLY : CFileStream::WIN_CREATE );
	CPtr<IBinSaver> pSaverMain = CreateBinSaver( &streamMain, bRead ? SAVER_MODE_READ : SAVER_MODE_WRITE );
	if ( !IsValid( pSaverMain ) )
		return false;

	pSaverMain->Add( 2, &replayInfo );
	if ( bRead )
		pSaverMain->Add( 3, &loadedHistory );
	else
		pSaverMain->Add( 3, &savingHistory );
	// Chunk 4 is new. A replay written before it carries no such chunk, and
	// CallObjectSerialize leaves the target alone when StartChunk finds none, so
	// an older replay reads back with no samples and simply cannot be verified.
	pSaverMain->Add( 4, &recordedCheckSums );
	// The ISAAC state the recording started from. Without it a playback runs the
	// fresh entropy this object's constructor installs, which is a different
	// random stream, and the simulation parts company at the first draw that
	// matters. Multiplayer never needed it recorded because InitAfterMapLoad
	// replaces the seed with a zero one for any game that is not single player,
	// which is deterministic by itself; single player keeps the entropy.
	if ( bRead )
	{
		// Distinguish "no such chunk" from a seed that loaded, because the
		// pointer is already populated and Add would leave it alone either way.
		CPtr<IRandomSeed> pConstructed = pStartSeed;
		pStartSeed = 0;
		pSaverMain->Add( 5, &pStartSeed );
		bSeedFromReplay = IsValid( pStartSeed );
		if ( !bSeedFromReplay )
			pStartSeed = pConstructed;
	}
	else
	{
		pSaverMain->Add( 5, &pStartSeed );
	}

  if ( !bRead )
		return NSaveLoad::SerializeReplayInfo( &replayInfo, szFileName, bRead );

	return true;
}

bool CCommandsHistory::LoadHistory( const std::string &szFileName )
{
	if ( !SerializeHistory( szFileName, true ) )
		return false;

	bLoadedHistory = true;
	bFinishedHistory = false;
	bGameFinished = false;
	savingHistory.clear();

	// Sample the playback exactly where the recording sampled, by taking the
	// parameters from the replay rather than from whatever this run was started
	// with. A sample is not an independent snapshot of the state: the value is a
	// running adler32 carried across the whole run, so one taken after ten walks
	// of the state cannot be compared with one taken after a single walk. Asking
	// whoever verifies a replay to remember which variables were set when it was
	// recorded only produces divergences that are a difference of sampling.
	//
	// Changing the period is therefore a property of the recording, not of the
	// verification: to compare at a finer resolution the replay has to be
	// recorded at that resolution.
	if ( recordedCheckSums.nPeriod > 0 )
	{
		NGlobal::SetVar( "checksumperiod", recordedCheckSums.nPeriod );
		NGlobal::SetVar( "light_checksum_count", recordedCheckSums.nLightMode );
		if ( NGlobal::GetVar( "checksum_verify", 0 ) != 0 )
		{
			ReportReplayLine( fmt::format(
				"replay verify: {} samples recorded every {} ms, {} state",
				recordedCheckSums.Size(), recordedCheckSums.nPeriod,
				recordedCheckSums.nLightMode != 0 ? "per unit" : "full" ) );
		}
	}

	// Said before anything is played, because it decides what a divergence later
	// in this run means. A replay recorded by another build, or against map data
	// since edited, diverges identically on every attempt, which is exactly how
	// a reproducible defect presents and cannot be told apart from one by
	// looking at the divergence.
	//
	// Facts rather than a verdict, deliberately. No build identifier available
	// here separates "the simulation changed" from "the same sources were built
	// differently": the image id differs between a release build and a sanitizer
	// build of identical code, and comparing exactly those two is the main thing
	// this harness does, so a line claiming a change would fire on nearly every
	// verification run. What the configurations do is name the likely reason, and
	// the reader knows which comparison they intended.
	//
	// Reported whatever the run is for, not only under checksum_verify, since a
	// mismatch makes the playback untrustworthy and not merely unverifiable.
	// Silent when everything agrees, and silent when the replay carries no
	// stamp, which every replay recorded before these chunks does.
	{
		const std::string &szImageId = NBuildStamp::GetSimulationImageId();
		const std::string &szConfiguration = NBuildStamp::GetBuildConfiguration();
		const uint32_t uMapCheckSum = replayInfo.pMap != 0 ? replayInfo.pMap->CalcCheckSum() : 0;
		if ( !replayInfo.szImageId.empty() && !szImageId.empty() && replayInfo.szImageId != szImageId )
		{
			// Named so the two cases read differently at a glance. Same
			// configuration and a different id means something in the
			// simulation modules was rebuilt, which is the case worth
			// suspecting. A different configuration explains the difference by
			// itself and says nothing about the sources either way.
			const std::string szRecordedCfg = !replayInfo.szConfiguration.empty() ? replayInfo.szConfiguration : "unrecorded";
			if ( replayInfo.szConfiguration == szConfiguration )
			{
				ReportReplayLine( fmt::format(
					"replay: recorded by {} build {}, this is {} build {}. Same configuration, so the simulation modules were rebuilt since: a divergence may be that change rather than a defect",
					szRecordedCfg, replayInfo.szImageId, szConfiguration, szImageId ) );
			}
			else
			{
				ReportReplayLine( fmt::format(
					"replay: recorded by {} build {}, this is {} build {}. Build ids always differ between configurations, so this says nothing about whether the sources did",
					szRecordedCfg, replayInfo.szImageId, szConfiguration, szImageId ) );
			}
		}
		if ( replayInfo.uMapCheckSum != 0 && uMapCheckSum != 0 && replayInfo.uMapCheckSum != uMapCheckSum )
		{
			ReportReplayLine( fmt::format(
				"replay: recorded against map checksum {:#x}, this map is {:#x}. The map has been edited since",
				replayInfo.uMapCheckSum, uMapCheckSum ) );
		}
	}

	// Before the mission is brought up, which is where the first draws happen.
	// For a game that is not single player InitAfterMapLoad replaces this with a
	// zero seed anyway, which is equally deterministic, so doing it here is
	// right for both.
	if ( bSeedFromReplay )
		NRandom::SetRandomSeed( pStartSeed );

	// A single-player replay has to start the way StartNewMap starts one, not
	// the way a multiplayer game does: the tracker is already EGT_SINGLE, so
	// SetGameType would assert, and there are no lobby slots to register
	// players from. The block this mirrors carries a comment asking for the two
	// to be changed together.
	if ( replayInfo.bSinglePlayer )
	{
		InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_SINGLE );
		Singleton<IAIScenarioTracker>()->SetLocalPlayer( 0 );
		IScenarioTracker *pSingleTracker = Singleton<IScenarioTracker>();
		pSingleTracker->Clear();
		// The difficulty and the tutorial flag come from the recording now
		// rather than being hard-coded, because both reach the simulation: the
		// difficulty through unit statistics and the AI, which is what nearly
		// every random draw in a run is spent on.
		//
		// The campaign and the chapter stay unset: a replay does not carry them,
		// and whether anything in the simulation asks for them is a question the
		// verification itself will now answer.
		pSingleTracker->CustomMissionStart( replayInfo.pMap, replayInfo.nDifficulty, replayInfo.bIsTutorial );
		// Unconditionally, not only for a campaign. CustomMissionStart leaves
		// the temp. globals alone, so without this a playback inherits whatever
		// the process did beforehand and stops being a function of the replay.
		// Nothing is restored in their place: they are ordinary globals sharing
		// a namespace with settings like the resolution and the sound volume,
		// so picking out the ones a mission script owns is not something a
		// replay should be trying to do, and a simulation that depended on the
		// rest would be broken in a much worse way. Clearing is also what the
		// campaign path does, so it is the behaviour being reproduced.
		pSingleTracker->ClearMissionScriptVars();

		// Said out loud during a verification run, because it is the one known
		// difference left between a campaign recording and its playback, and so
		// the first thing to suspect if such a replay diverges.
		if ( replayInfo.nStartKind == 1 && NGlobal::GetVar( "Replay.ExitWhenFinished", 0 ) != 0 )
		{
			ReportReplayLine( "replay: recorded in a campaign mission, whose campaign and chapter are not restored" );
		}

		NGlobal::SetVar( "History.Playing", 1 );
		return true;
	}

	InterfaceState()->MakeScenarioTracker( IInterfaceState::ESTT_MULTI );
	IScenarioTracker *pScenarioTracker = Singleton<IScenarioTracker>();
	pScenarioTracker->SetGameType( IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL );
	pScenarioTracker->MissionStart( replayInfo.pMap, replayInfo.nTechLevel );

	IScenarioTracker::SMultiplayerInfo scenarioInfo;
	for ( int i = 0; i < replayInfo.slots.size(); ++i )
	{
		const SMPSlot &slot = replayInfo.slots[i];
		if ( !slot.bPresent )
			continue;

		pScenarioTracker->AddPlayer( i );
		pScenarioTracker->SetPlayerSide( i, slot.nTeam );
		pScenarioTracker->SetPlayerParty( i, slot.nCountry );
		pScenarioTracker->SetPlayerColour( i, slot.nColour );

		scenarioInfo.players.push_back( IScenarioTracker::SMultiplayerInfo::SPlayer() );
		IScenarioTracker::SMultiplayerInfo::SPlayer &scenarioPlayer = scenarioInfo.players.back();
		scenarioPlayer.wszName = NStr::ToUnicode( slot.szName );
		scenarioPlayer.nTeam = slot.nTeam;
		scenarioPlayer.nIndex = i;
		scenarioPlayer.nCountry = slot.nCountry;
		scenarioPlayer.nLevel = 1;
		scenarioPlayer.wszRank = L"";
	}
	pScenarioTracker->SetMultiplayerInfo( scenarioInfo );

	NGlobal::SetVar( "AI.MultiplayerCaptureTime", replayInfo.nCaptureTime * 1000 );
	NGlobal::SetVar( "multiplayer_unit_experience", replayInfo.bUnitExperience ? 1 : 0 );
	NGlobal::SetVar( "History.Playing", 1 );

	return true;
}

bool CCommandsHistory::SaveReplay( const std::string &szFileName, const SB2GameSpecificData &gameDesc, const std::vector<SMPSlot> &slots, const int nWinningSide )
{
	replayInfo.bUnitExperience = gameDesc.bUnitExp;
	replayInfo.nCaptureTime = gameDesc.nCaptureTime;;
	replayInfo.nTechLevel = gameDesc.nTechLevel;
	replayInfo.bUnitExperience = gameDesc.bUnitExp;
	replayInfo.nWinningSide = nWinningSide;
	replayInfo.slots = slots;
	// Taken from the running game rather than from the caller, because it is a
	// property of what was recorded. A single-player replay has no lobby slots,
	// so without this LoadHistory would take the multiplayer path and register
	// no players at all.
	IScenarioTracker *pTracker = Singleton<IScenarioTracker>();
	replayInfo.bSinglePlayer = pTracker->GetGameType() == IAIScenarioTracker::EGT_SINGLE;
	// A campaign mission is entered through MissionStart, which leaves the
	// campaign, the chapter and the difficulty as the campaign set them, where a
	// custom mission goes through CustomMissionStart and sets them itself. The
	// playback has to take the matching path, and a campaign is recognised by
	// the tracker having one.
	replayInfo.nStartKind = pTracker->GetCurrentCampaign() != 0 ? 1 : 0;
	replayInfo.nDifficulty = pTracker->GetDifficultyLevel();
	replayInfo.bIsTutorial = pTracker->IsTutorialMission();
	// What made this recording, so a playback against a later build reports that
	// instead of a divergence. Written at save time rather than at record time:
	// neither can change while a mission runs, and a replay saved out of a
	// loaded savegame would otherwise carry the identity of whatever recorded
	// the save.
	replayInfo.szImageId = NBuildStamp::GetSimulationImageId();
	replayInfo.szConfiguration = NBuildStamp::GetBuildConfiguration();
	replayInfo.uMapCheckSum = replayInfo.pMap != 0 ? replayInfo.pMap->CalcCheckSum() : 0;

	if ( !SerializeHistory( szFileName, false ) )
		return false;

//	WriteToPipe( PIPE_CHAT, StrFmt( "Replay \"%s\" succesfully saved", szFileName ), 0xFF00FF00, true );
	return true;
}

bool CCommandsHistory::AddChecksumLog( const int nGameTime, const unsigned long ulChecksum, const int nEntry )
{
	if ( !bLoadedHistory && nEntry == 0 )
	{
		dwLastCheckSum = ulChecksum;
		// Entry zero is the whole-state value. nGameTime is game time, not a
		// segment number, whatever the CSegmentChecksum typedef above calls it.
		recordedCheckSums.Add( nGameTime, static_cast<uint32_t>( ulChecksum ),
			static_cast<uint32_t>( NRandom::GetRandomCallsCounter() ) );
	}

	if ( bLoadedHistory && nEntry == 0 )
		VerifyCheckSum( nGameTime, static_cast<uint32_t>( ulChecksum ) );

	// Only entry 0, which is the whole-state value; the rest are the running
	// total partway through the walk and there are thousands of them per sample.
	if ( nEntry == 0 && NGlobal::GetVar( "checksum_trace", 0 ) != 0 )
	{
		ReportReplayLine( fmt::format( "checksum {} t={} {:#010x}",
			bLoadedHistory ? "playback" : "record", nGameTime, ulChecksum ) );
	}

	// Every entry of one chosen sample, which is the one thing nothing else can
	// say. Entry zero is the whole state and tells us only that two runs differ;
	// the entries after it are the running total taken field by field through the
	// walk, so the first entry at which two of these files disagree names the
	// field. Set checksum_entry_dump_time to a sample time on both the run that
	// records and the run that plays back, and compare the two files.
	//
	// The entry number also feeds break_count and break_segment in the checksum
	// saver, which stop in the debugger at exactly that field.
	const int nEntryDumpTime = NGlobal::GetVar( "checksum_entry_dump_time", -1 );
	if ( nEntryDumpTime >= 0 && nGameTime == nEntryDumpTime )
	{
		// Opened once and left to the process exit to close. Thousands of entries
		// arrive per sample and reopening per line would dominate the run.
		if ( pEntryDumpFile == 0 )
		{
			const char *szName = bLoadedHistory ? "checksum_entries_playback.txt" : "checksum_entries_record.txt";
			pEntryDumpFile = fopen( szName, "w" );
			if ( pEntryDumpFile )
			{
				ReportReplayLine( fmt::format( "checksum entries for t={} going to {}", nGameTime, szName ) );
			}
		}
		if ( pEntryDumpFile )
			fmt::print( pEntryDumpFile, "{} {:#010x}\n", nEntry, ulChecksum );
	}
	else if ( pEntryDumpFile != 0 )
	{
		// Closed as soon as a later sample arrives, rather than left to the
		// process exit. The run that records one of these is a person playing and
		// then quitting, and a quarter of a million buffered lines are worth
		// nothing if that quit is not a clean one.
		fclose( pEntryDumpFile );
		pEntryDumpFile = 0;
	}

	return true;
}

bool CCommandsHistory::VerifyCheckSum( const int nGameTime, const uint32_t uValue )
{
	const size_t nRecorded = recordedCheckSums.Size();
	if ( nRecorded == 0 )
		return true;

	// A replay with no recorded seed replays the commands against a different
	// random stream, so it would diverge whatever the code did. Say that rather
	// than report it as a divergence.
	if ( !bSeedFromReplay )
	{
		if ( !bCheckSumModeReported )
		{
			bCheckSumModeReported = true;
			ReportReplayLine( "replay verify: this replay records no start seed, nothing to compare" );
		}
		return true;
	}

	// The values are only comparable if they were produced the same way.
	// light_checksum_count selects between a per-unit summary and a full walk of
	// the state, and the period decides how many of those walks a value has
	// accumulated, since it is a running total and not a snapshot. LoadHistory
	// sets both from the replay, so reaching this is either a replay written
	// before the period was recorded or a console change mid-run, and in both
	// cases the difference would be reported as a divergence it is not.
	const int nMode = NGlobal::GetVar( "light_checksum_count", 0 );
	const int nPeriod = NGlobal::GetVar( "checksumperiod", 1000 );
	if ( nMode != recordedCheckSums.nLightMode || nPeriod != recordedCheckSums.nPeriod )
	{
		if ( !bCheckSumModeReported )
		{
			bCheckSumModeReported = true;
			ReportReplayLine( fmt::format(
				"replay verify: recorded with light_checksum_count={} every {} ms but running with {} every {} ms, not comparable",
				recordedCheckSums.nLightMode, recordedCheckSums.nPeriod, nMode, nPeriod ) );
		}
		return true;
	}

	// Both sides advance in increasing game time, so step the cursor rather
	// than search. A recorded time this run never reached is skipped instead of
	// being treated as a difference: a shorter run is caught by the count of
	// matches, not by pretending the missing samples differed.
	while ( nCheckSumCursor < nRecorded && recordedCheckSums.times[nCheckSumCursor] < nGameTime )
		++nCheckSumCursor;

	if ( nCheckSumCursor >= nRecorded || recordedCheckSums.times[nCheckSumCursor] != nGameTime )
		return true;

	const uint32_t uRecorded = recordedCheckSums.values[nCheckSumCursor];

	// Checked before the state, and reported once, because the count parting
	// company earlier than the state says the divergence is in what consumed
	// the generator rather than in the arithmetic.
	if ( !bRngCallsMismatch && recordedCheckSums.HasRngCalls() )
	{
		const uint32_t uRecordedCalls = recordedCheckSums.rngCalls[nCheckSumCursor];
		const uint32_t uCalls = static_cast<uint32_t>( NRandom::GetRandomCallsCounter() );
		if ( uRecordedCalls != uCalls )
		{
			bRngCallsMismatch = true;
			ReportReplayLine( fmt::format(
				"replay verify: random draw count parts company at t={} ms, recorded {} but made {}",
				nGameTime, uRecordedCalls, uCalls ) );
			// Said every time, because it is the explanation that costs the least
			// to rule out and the one that looks most like a real divergence when
			// it is not: a replay carries no record of what built it, so one
			// recorded before a change to the simulation diverges against a build
			// that has the change, and does so identically on every run, which is
			// exactly how a reproducible bug presents.
			ReportReplayLine( "replay verify: if this replay was recorded by a different build, re-record it before reading anything into that" );
			// Dumped here rather than only where the state difference shows up,
			// because this is the earlier moment and the call sites are kept in a
			// 512 entry ring: by the time the state differs the ring has rolled
			// past the draw that caused it, and the dump names call sites that
			// are only consequences. A count that differs by a few says a branch
			// consuming the generator was taken on one side and not the other,
			// and the window is the one period before this sample, which is what
			// the ring still holds at this point.
			if ( IAILogic *pAI = Singleton<IAILogic>() )
				pAI->WriteDetailedChecksumInfo( "last_async_rng_debug.txt" );
		}
	}

	++nCheckSumCursor;

	if ( uRecorded == uValue )
	{
		++nCheckSumsMatched;
		return true;
	}

	// Once one sample differs every later one does too, the value being a running
	// adler32 carried across the whole run rather than an independent snapshot,
	// so there is nothing to say about the state after this point. The draw count
	// is not like that: it is an absolute count and keeps meaning something, and
	// it is the count rather than the checksum that says whether the simulation
	// itself parted company or whether only the bytes being hashed did. So the
	// walk above still runs and only the reporting stops here.
	if ( bCheckSumMismatch )
		return false;

	bCheckSumMismatch = true;
	ReportReplayLine( fmt::format(
		"replay verify: DIVERGED at t={} ms, recorded {:#010x} but got {:#010x} ({} samples matched before it, random draw count {})",
		nGameTime, uRecorded, uValue, nCheckSumsMatched,
		!recordedCheckSums.HasRngCalls() ? "not recorded"
			: bRngCallsMismatch ? "already differed" : "still agrees" ) );

	// The same bundle the multiplayer ASYNC path writes, which is the state
	// worth having at the moment the two runs parted: the RNG counters and
	// ISAAC state, and the commands that were replayed.
	if ( IAILogic *pAI = Singleton<IAILogic>() )
		pAI->WriteDetailedChecksumInfo( "last_async_detailed_debug.txt" );
	WriteCommandsHistoryToFile( "last_async_cmds.txt" );

	return false;
}

CHistory CCommandsHistory::GetCommandsHistory() const
{
	if (savingHistory.size() > 0)
		return savingHistory;
	return loadedHistory;
}

void CCommandsHistory::AddCommand( const int nSegment, IAILogicCommandB2 *pCmd )
{
	if ( bCanAddCommand && pCmd->NeedToBeStored() )
		savingHistory[nSegment].push_back( pCmd );

	if ( nSegment > replayInfo.nLastCommandSegment )
		replayInfo.nLastCommandSegment = nSegment;
}

void CCommandsHistory::ExecuteSegmentCommands( const int nSegment, ITransceiver *pTranceiver )
{
	if ( !bLoadedHistory )
	{
		if ( nSegment > replayInfo.nLastGameSegment )
			replayInfo.nLastGameSegment = nSegment;
	}
	else
	{
		if ( !bFinishedHistory )
		{
			bCanAddCommand = false;
			if ( loadedHistory.find( nSegment ) != loadedHistory.end() )
			{
				// replay_trace puts the segment, the game time and each command
				// on stdout as it is fed in, which is what narrows a divergence
				// to the commands that ran inside the window it appeared in.
				// The same type name and description the dump file uses.
				const bool bTrace = NGlobal::GetVar( "replay_trace", 0 ) != 0;
				for ( std::list< CPtr<IAILogicCommandB2> >::iterator iter = loadedHistory[nSegment].begin(); iter != loadedHistory[nSegment].end(); ++iter )
				{
					if ( bTrace )
					{
						IAILogicCommandB2 *pCmd = *iter;
						ReportReplayLine( fmt::format( "replay segment {}: {} {{{}}}",
							nSegment, pCmd->GetFullTypeName(), pCmd->GetDebugInfo() ) );
					}
					pTranceiver->SendCommand( *iter );
				}
			}
			bCanAddCommand = true;
			if ( nSegment >= replayInfo.nLastCommandSegment )
			{
//				WriteToPipe( PIPE_CHAT, "Replay finished: No more commands", 0xFF00FF00, true );
				bFinishedHistory = true;
			}
		}
		if ( !bGameFinished )
		{
			if ( nSegment >= replayInfo.nLastGameSegment )
			{
//				WriteToPipe( PIPE_CHAT, "Replay finished: Last game segment executed", 0xFF00FF00, true );
				// Reaching the end is necessary but not sufficient: a run that
				// played out while diverging, or one that compared nothing at
				// all because the replay carries no samples, has not verified
				// anything. Replay.Result starts nonzero and is cleared only
				// when there is something to claim, so every other outcome
				// reports failure.
				const bool bVerified = !bCheckSumMismatch && nCheckSumsMatched > 0;
				const auto szResult = fmt::format(
					"replay played out at segment {}: {}", nSegment,
					bCheckSumMismatch ? "DIVERGED"
						: nCheckSumsMatched > 0
							? fmt::format( "{} checksums matched", nCheckSumsMatched )
							: std::string( "nothing to compare, no recorded checksums" ) );
				ReportReplayLine( szResult );

				if ( NGlobal::GetVar( "Replay.ExitWhenFinished", 0 ) != 0 )
				{
					if ( bVerified )
						NGlobal::SetVar( "Replay.Result", 0 );
					NMainLoop::Command( CreateICExitGame() );
				}
				else
				{
					Singleton<IGameTimer>()->Pause( true, PAUSE_TYPE_USER_PAUSE );
					NMainLoop::Command( ML_COMMAND_MESSAGE_BOX,
						CICMessageBox::MakeConfigString( "MessageBoxWindowOk",
						InterfaceState()->GetTextEntry( "T_REPLAY_FINISHED" ) ).c_str() );
				}

				bGameFinished = true;
			}
		}
	}
}

ICommandsHistory *CreateCommandsHistory( const SReplayInfo &replay )
{
	CCommandsHistory *pRes = new CCommandsHistory();
	if ( replay.bDoReplay )
	{
		if ( !pRes->LoadHistory( replay.szReplayName ) )
		{
			NI_ASSERT( false, fmt::format( "Cannot load replay from file \"{}\"", replay.szReplayName ) );
			NGlobal::SetVar( "History.Playing", 0 );
		}
	}
	else
		NGlobal::SetVar( "History.Playing", 0 );

	return pRes;
}

REGISTER_SAVELOAD_CLASS( GAMEX, 0x1007AB80, CCommandsHistory );

