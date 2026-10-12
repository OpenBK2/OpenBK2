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
		// A campaign recording also needs the script variables cleared, which is
		// what MissionStart does and CustomMissionStart does not. The campaign
		// and the chapter stay unset either way: a replay does not carry them,
		// and whether anything in the simulation asks for them is a question the
		// verification itself will now answer.
		pSingleTracker->CustomMissionStart( replayInfo.pMap, replayInfo.nDifficulty, replayInfo.bIsTutorial );
		if ( replayInfo.nStartKind == 1 )
			pSingleTracker->ClearMissionScriptVars();

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

	return true;
}

bool CCommandsHistory::VerifyCheckSum( const int nGameTime, const uint32_t uValue )
{
	// Once one sample differs every later one does too: the value is a running
	// adler32 carried across the whole run, not an independent snapshot. So the
	// first difference is the divergence and the rest say nothing.
	if ( bCheckSumMismatch )
		return false;

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

	// The values are only comparable if they were produced the same way, and
	// light_checksum_count selects between a per-unit summary and a full walk
	// of the state. Comparing across the two would report a divergence that is
	// only a difference of method.
	const int nMode = NGlobal::GetVar( "light_checksum_count", 0 );
	if ( nMode != recordedCheckSums.nLightMode )
	{
		if ( !bCheckSumModeReported )
		{
			bCheckSumModeReported = true;
			ReportReplayLine( fmt::format(
				"replay verify: recorded with light_checksum_count={} but running with {}, not comparable",
				recordedCheckSums.nLightMode, nMode ) );
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
		}
	}

	++nCheckSumCursor;

	if ( uRecorded == uValue )
	{
		++nCheckSumsMatched;
		return true;
	}

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
		pAI->WriteDetailedChecksumInfo();
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

