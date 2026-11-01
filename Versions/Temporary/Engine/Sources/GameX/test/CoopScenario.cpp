#include "../stdafx.h"

#include "AILogic/DBAIConsts.h"
#include "Common_RTS_AI/Checksums.h"
#include "GameX/CommandsHistory.h"
#include "GameX/DBConsts.h"
#include "GameX/DBMPConsts.h"
#include "GameX/ScenarioTracker.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "Main/GameTimer.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <filesystem>

#include <gtest/gtest.h>

namespace
{
class CoopScenario : public testing::Test
{
protected:
	std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	CObj<IScenarioTracker> tracker;
	CObj<IGameTimer> timer;
	NDb::SMapInfo *map = nullptr;
	NDb::SPartyDependentInfo *botParty = nullptr;
	NDb::SReinforcement *botReinforcement = nullptr;

	template <class T>
	T *CreateRecord( const char *pszClass )
	{
		CObj<NDb::IObjMan> record = NDb::CreateNewObject( pszClass );
		EXPECT_NE( record, nullptr );
		if ( !record )
		{
			return nullptr;
		}
		T *object = dynamic_cast<T *>( record->GetObject() );
		records.push_back( record );
		const std::string name = std::to_string( records.size() ) + ".xdb";
		EXPECT_TRUE( NDb::AddNewObject( name, CDBID( name ), record ) );
		return object;
	}

	void SetUp() override
	{
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-coop-scenario-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( COOP_SCENARIO_TYPES_FILE, directory / "types.xml" );
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		auto *constants = CreateRecord<NDb::SGameConsts>( "GameConsts" );
		ASSERT_NE( constants, nullptr );
		NGlobal::SetVar( "game_consts", "1.xdb" );
		constants->pAI = CreateRecord<NDb::SAIGameConsts>( "AIGameConsts" );
		constants->pMultiplayer = CreateRecord<NDb::SMultiplayerConsts>( "MultiplayerConsts" );
		map = CreateRecord<NDb::SMapInfo>( "MapInfo" );
		botParty = CreateRecord<NDb::SPartyDependentInfo>( "PartyDependentInfo" );
		botReinforcement = CreateRecord<NDb::SReinforcement>( "Reinforcement" );
		ASSERT_NE( map, nullptr );
		ASSERT_NE( botParty, nullptr );
		ASSERT_NE( botReinforcement, nullptr );
		map->players.resize( 5 );
		// Interleaved bot ownership must not be mistaken for a connected human.
		for ( int nBot : { 1, 3 } )
		{
			map->players[nBot].ePlayerType = NDb::AI_BOT;
			map->players[nBot].nDiplomacySide = 1;
			map->players[nBot].pPartyInfo = botParty;
			map->players[nBot].reinforcementTypes.push_back( botReinforcement );
			map->players[nBot].nReinforcementCalls = 7;
		}
		map->players.back().nDiplomacySide = 2;
		tracker = CreateScenarioTrackerMultiplayer();
		ASSERT_NE( tracker, nullptr );
		timer = dynamic_cast<IGameTimer *>( NObjectFactory::MakeObject( 0x10075C05 ) );
		ASSERT_NE( timer, nullptr );
		NSingleton::RegisterSingleton( timer, IGameTimer::tidTypeID );
		tracker->SetGameType( IAIScenarioTracker::EGT_MULTI_COOP );
		tracker->MissionStart( map, 0 );
	}

	void TearDown() override
	{
		tracker = nullptr;
		NSingleton::UnRegisterSingleton( IGameTimer::tidTypeID );
		timer = nullptr;
		NGlobal::RemoveVar( "game_consts" );
		records.clear();
		NDb::CloseDatabase();
		std::error_code error;
		std::filesystem::remove_all( directory, error );
	}
};

TEST_F( CoopScenario, BotsExistWithoutHumanSlotsAndKeepAuthoredLoadouts )
{
	EXPECT_FALSE( tracker->IsPlayerPresent( 0 ) );
	EXPECT_FALSE( tracker->IsPlayerPresent( 2 ) );
	EXPECT_TRUE( tracker->IsPlayerPresent( 4 ) );
	for ( int nBot : { 1, 3 } )
	{
		EXPECT_TRUE( tracker->IsPlayerPresent( nBot ) );
		EXPECT_EQ( tracker->GetPlayerParty( nBot ), botParty );
		EXPECT_EQ( tracker->GetReinforcement( nBot, botReinforcement->eType ), botReinforcement );
		EXPECT_EQ( tracker->GetReinforcementCallsLeft( nBot ), 7 );
		EXPECT_EQ( tracker->GetEnemyDifficultyRCallsModifier(), 1.0f );
		EXPECT_EQ( tracker->GetEnemyDifficultyRTimeModifier(), 1.0f );
		EXPECT_EQ( tracker->GetStartUnits( nBot ), nullptr );
		tracker->SetPlayerParty( nBot, 999 );
		EXPECT_EQ( tracker->GetPlayerParty( nBot ), botParty );
	}
	tracker->AddPlayer( 2 );
	EXPECT_TRUE( tracker->IsPlayerPresent( 2 ) );
	EXPECT_FALSE( tracker->IsPlayerPresent( 0 ) );
}

TEST_F( CoopScenario, ScriptResultsAreExplicitFirstWinsAndResetForEachMission )
{
	EXPECT_EQ( tracker->GetScriptOutcome(), -1 );
	tracker->SetScriptOutcome( 2 );
	EXPECT_EQ( tracker->GetScriptOutcome(), -1 );
	tracker->SetScriptOutcome( 0 );
	tracker->SetScriptOutcome( 1 );
	EXPECT_EQ( tracker->GetScriptOutcome(), 0 );
	tracker->MissionStart( map, 0 );
	EXPECT_EQ( tracker->GetScriptOutcome(), -1 );
	tracker->SetScriptOutcome( 1 );
	EXPECT_EQ( tracker->GetScriptOutcome(), 1 );
	tracker->SetGameType( IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL );
	tracker->MissionStart( map, 0 );
	tracker->SetScriptOutcome( 0 );
	EXPECT_EQ( tracker->GetScriptOutcome(), -1 );
	EXPECT_FALSE( tracker->IsPlayerPresent( 1 ) );
}

TEST_F( CoopScenario, SaveLoadPreservesModeDifficultyAndPendingResult )
{
	tracker->SetMultiplayerDifficulty( 4 );
	tracker->SetScriptOutcome( 1 );
	CMemoryStream stream;
	{
		CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->AddPolymorphicBase( 1, tracker.GetPtr() );
	}
	CObj<IScenarioTracker> restored = CreateScenarioTrackerMultiplayer();
	stream.Seek( 0 );
	{
		CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->AddPolymorphicBase( 1, restored.GetPtr() );
	}
	EXPECT_TRUE( restored->IsCooperative() );
	EXPECT_EQ( restored->GetDifficultyLevel(), 4 );
	EXPECT_EQ( restored->GetScriptOutcome(), 1 );
	EXPECT_TRUE( restored->IsPlayerPresent( 3 ) );
}

TEST( CoopReplay, MetadataRoundTripPreservesModeAndResolvedDifficulty )
{
	SMultiplayerReplayInfo original;
	original.nMultiplayerMode = 1;
	original.nDifficulty = 3;
	original.nTechLevel = 2;
	original.nScriptResult = 2;
	original.slots.resize( 5 );
	original.slots[0].bPresent = true;
	original.slots[2].bPresent = true;
	CMemoryStream stream;
	{
		CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &original );
	}
	SMultiplayerReplayInfo restored;
	stream.Seek( 0 );
	{
		CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->Add( 1, &restored );
	}
	EXPECT_EQ( restored.nMultiplayerMode, 1 );
	EXPECT_EQ( restored.nDifficulty, 3 );
	EXPECT_EQ( restored.nTechLevel, 2 );
	EXPECT_EQ( restored.nScriptResult, 2 );
	ASSERT_EQ( restored.slots.size(), 5 );
	EXPECT_TRUE( restored.slots[2].bPresent );
	EXPECT_FALSE( restored.slots[1].bPresent );
}

TEST( CoopReplay, MissingOutcomeChunkMeansUnfinished )
{
	CMemoryStream stream;
	{
		CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		int nMode = 1;
		saver->Add( 18, &nMode );
	}
	SMultiplayerReplayInfo restored;
	restored.nScriptResult = 2;
	stream.Seek( 0 );
	{
		CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		restored.operator&( *saver );
	}
	EXPECT_EQ( restored.nMultiplayerMode, 1 );
	EXPECT_EQ( restored.nScriptResult, 0 );
}

struct SReplayCompletionState
{
	bool bFinishedHistory = true;
	bool bGameFinished = false;
	bool bLoadedHistory = true;
	SMultiplayerReplayInfo info;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 3, &bFinishedHistory );
		saver.Add( 4, &bGameFinished );
		saver.Add( 5, &bLoadedHistory );
		saver.Add( 8, &info );
		return 0;
	}
};

TEST_F( CoopScenario, ReplayCompletionChecksResultBetweenChecksumSamples )
{
	for ( const int nExpectedOutcome : { -1, 0, 1 } )
	{
		for ( const int nActualOutcome : { -1, 0, 1 } )
		{
			CObj<ICommandsHistory> history = dynamic_cast<ICommandsHistory *>( NObjectFactory::MakeObject( 0x1007AB80 ) );
			ASSERT_NE( history, nullptr );
			const auto *concreteHistory = dynamic_cast<const CCommandsHistory *>( history.GetPtr() );
			ASSERT_NE( concreteHistory, nullptr );
			SReplayCompletionState state;
			state.info.nMultiplayerMode = 1;
			state.info.nScriptResult = nExpectedOutcome + 1;
			state.info.nLastGameSegment = 10;
			CMemoryStream stream;
			{
				CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
				saver->Add( 1, &state );
			}
			stream.Seek( 0 );
			{
				CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
				saver->AddPolymorphicBase( 1, history.GetPtr() );
			}
			tracker->MissionStart( map, 0 );
			tracker->SetScriptOutcome( nActualOutcome );
			NSingleton::RegisterSingleton( tracker, IScenarioTracker::tidTypeID );
			// This queues an exit command without running the graphical main loop.
			NGlobal::SetVar( "Replay.ExitWhenFinished", 1 );
			NGlobal::SetVar( "Replay.Result", 1 );
			history->ExecuteSegmentCommands( 10, nullptr );
			NGlobal::RemoveVar( "Replay.ExitWhenFinished" );
			NGlobal::RemoveVar( "Replay.Result" );
			NSingleton::UnRegisterSingleton( IScenarioTracker::tidTypeID );
			EXPECT_TRUE( history->IsPlaybackFinished() );
			EXPECT_EQ( concreteHistory->HasCheckSumMismatch(), nExpectedOutcome != nActualOutcome );
		}
	}
}
} // namespace
