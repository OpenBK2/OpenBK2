#include "../stdafx.h"

#include "AILogic/DBAIConsts.h"
#include "Common_RTS_AI/Checksums.h"
#include "GameX/CommandsHistory.h"
#include "GameX/CoopPlayerInfo.h"
#include "GameX/DBConsts.h"
#include "GameX/DBMPConsts.h"
#include "GameX/MPStatistics.h"
#include "GameX/ScenarioTracker.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "Main/GameTimer.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "System/Text.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>

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
	NDb::SMultiplayerConsts *multiplayerConsts = nullptr;

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
		multiplayerConsts = CreateRecord<NDb::SMultiplayerConsts>( "MultiplayerConsts" );
		constants->pMultiplayer = multiplayerConsts;
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

	void SetBotName( int nPlayer, const std::u16string &name )
	{
		const std::string filename = "bot" + std::to_string( nPlayer ) + ".txt";
		{
			std::ofstream file( directory / filename, std::ios::binary );
			// Text resources are UTF-16LE on every platform, not native wchar_t bytes.
			for ( char16_t character : std::u16string( 1, u'\ufeff' ) + name )
			{
				file.put( char( character & 0xff ) );
				file.put( char( character >> 8 ) );
			}
		}
		map->players[nPlayer].szLocalizedPlayerNameFileRef = filename;
		NText::Reload( filename );
	}

	void SetHumanRoster()
	{
		IScenarioTracker::SMultiplayerInfo info;
		for ( int nPlayer : { 0, 2 } )
		{
			IScenarioTracker::SMultiplayerInfo::SPlayer player;
			player.nIndex = nPlayer;
			player.nTeam = 0;
			player.wszName = L"Human " + std::to_wstring( nPlayer );
			info.players.push_back( player );
			tracker->AddPlayer( nPlayer );
		}
		tracker->SetMultiplayerInfo( info );
		tracker->SetLocalPlayer( 0 );
	}

	void RegisterKills( int nKiller, int nVictim, int nCount )
	{
		IAIScenarioTracker::SKillInfo kill;
		kill.nPlayer = nKiller;
		kill.nKilledUnitPlayer = nVictim;
		kill.fExpPrice = 100;
		kill.eReinfType = NDb::_RT_NONE;
		for ( int i = 0; i < nCount; ++i )
		{
			tracker->RegisterUnitKill( kill );
		}
	}
};

const NGameX::SMPStatisticsPlayer *FindStatisticsPlayer( const std::list<NGameX::SMPStatisticsPlayer> &team,
                                                         const std::wstring &name )
{
	const auto found =
			std::find_if( team.begin(), team.end(), [&]( const auto &player ) { return player.wszName == name; } );
	return found == team.end() ? nullptr : &*found;
}

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

TEST_F( CoopScenario, ResultTeamsIncludeNamedBotsWithFinalKillsAndLosses )
{
	SetBotName( 1, u"General \u017dukov" );
	SetBotName( 3, u"General Montgomery" );
	// Even an AI-tagged neutral must never become a competitor row.
	map->players.back().ePlayerType = NDb::AI_BOT;
	multiplayerConsts->sides.resize( 2 );
	multiplayerConsts->sides[1].pPartyInfo = botParty;
	for ( int nWinner : { 0, 1 } )
	{
		tracker->MissionStart( map, 0 );
		SetHumanRoster();
		RegisterKills( 0, 1, 1 );
		RegisterKills( 1, 0, 2 );
		RegisterKills( 2, 3, 3 );
		RegisterKills( 3, 2, 1 );
		tracker->SetScriptOutcome( nWinner );
		if ( nWinner == 0 )
		{
			tracker->MissionWin();
		}
		else
		{
			tracker->MissionCancel();
		}
		ASSERT_EQ( tracker->GetCurrentMission(), nullptr );
		const auto teams = NGameX::BuildMultiplayerStatistics( tracker, multiplayerConsts );
		EXPECT_EQ( teams.wszLocalPlayerName, L"Human 0" );
		const auto &humans = nWinner == 0 ? teams.wonTeam : teams.lostTeam;
		const auto &bots = nWinner == 1 ? teams.wonTeam : teams.lostTeam;
		ASSERT_EQ( humans.size(), 2 );
		ASSERT_EQ( bots.size(), 2 );
		const auto *first = FindStatisticsPlayer( bots, L"General \u017dukov" );
		const auto *second = FindStatisticsPlayer( bots, L"General Montgomery" );
		ASSERT_NE( first, nullptr );
		ASSERT_NE( second, nullptr );
		EXPECT_EQ( first->nCountry, 1 );
		EXPECT_EQ( first->nTeam, 1 );
		EXPECT_EQ( first->nUnitsKilled, 2 );
		EXPECT_EQ( first->nUnitsLost, 1 );
		EXPECT_EQ( second->nUnitsKilled, 1 );
		EXPECT_EQ( second->nUnitsLost, 3 );
		EXPECT_GT( first->nScore, second->nScore );
		EXPECT_EQ( bots.front().wszName, first->wszName );
		EXPECT_EQ( tracker->GetMultiplayerInfo()->players.size(), 2 );

		// Save/load retains the ended mission and score snapshot. Replay viewpoints
		// likewise use different local humans without changing the winning side.
		CMemoryStream stream;
		{
			CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->AddPolymorphicBase( 1, tracker.GetPtr() );
		}
		CObj<IScenarioTracker> restored = CreateScenarioTrackerMultiplayer();
		stream.Seek( 0 );
		{
			CObj<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
			loader->AddPolymorphicBase( 1, restored.GetPtr() );
		}
		restored->SetLocalPlayer( 2 );
		const auto replayTeams = NGameX::BuildMultiplayerStatistics( restored, multiplayerConsts );
		EXPECT_EQ( replayTeams.wszLocalPlayerName, L"Human 2" );
		const auto &replayBots = nWinner == 1 ? replayTeams.wonTeam : replayTeams.lostTeam;
		ASSERT_EQ( replayBots.size(), 2 );
		const auto *replayFirst = FindStatisticsPlayer( replayBots, first->wszName );
		ASSERT_NE( replayFirst, nullptr );
		EXPECT_EQ( replayFirst->nUnitsKilled, 2 );
		EXPECT_EQ( replayFirst->nUnitsLost, 1 );
		EXPECT_EQ( replayFirst->nScore, first->nScore );
		EXPECT_EQ( restored->GetMultiplayerInfo()->players.size(), 2 );
	}
}

TEST_F( CoopScenario, UnnamedBotsHaveDistinctFallbacksAndClassicUsesOnlyNetworkRoster )
{
	SetHumanRoster();
	tracker->SetScriptOutcome( 0 );
	tracker->MissionWin();
	const auto teams = NGameX::BuildMultiplayerStatistics( tracker, multiplayerConsts );
	ASSERT_EQ( teams.lostTeam.size(), 2 );
	EXPECT_NE( FindStatisticsPlayer( teams.lostTeam, L"AI 2" ), nullptr );
	EXPECT_NE( FindStatisticsPlayer( teams.lostTeam, L"AI 4" ), nullptr );
	tracker->SetGameType( IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL );
	const auto classic = NGameX::BuildMultiplayerStatistics( tracker, multiplayerConsts );
	EXPECT_EQ( classic.wonTeam.size(), 2 );
	EXPECT_TRUE( classic.lostTeam.empty() );
}

TEST_F( CoopScenario, BotPresentationKeepsSparseMapIdsNamesAndPositions )
{
	SetBotName( 1, u"General \u017dukov \u6771" );
	SetBotName( 3, u"" );
	map->players[1].vMPStartPos = CVec2( 12.5f, 34.25f );
	map->players[3].vMPStartPos = CVec2( 78.75f, 90.5f );
	// Neutral owners are excluded even when tagged as bots, anywhere in the array.
	map->players[2].ePlayerType = NDb::AI_BOT;
	map->players[2].nDiplomacySide = 2;
	map->players.back().ePlayerType = NDb::AI_BOT;
	const auto players = NGameX::CollectCoopMapPlayers( map );
	ASSERT_EQ( players.size(), 2 );
	EXPECT_EQ( players[0].nPlayerIndex, 1 );
	EXPECT_EQ( players[0].wszName, L"General \u017dukov \u6771" );
	EXPECT_EQ( players[0].vMPStartPos, CVec2( 12.5f, 34.25f ) );
	EXPECT_EQ( players[1].nPlayerIndex, 3 );
	EXPECT_EQ( players[1].wszName, L"AI 4" );
	EXPECT_EQ( players[1].vMPStartPos, CVec2( 78.75f, 90.5f ) );
	EXPECT_TRUE( NGameX::CollectCoopMapPlayers( nullptr ).empty() );
}

TEST( CoopReplay, BotMetadataRoundTripAndCopyPreserveUnicodePositionsAndHumanSlots )
{
	SMultiplayerReplayInfo original;
	original.nMultiplayerMode = 1;
	original.bCoopPlayersRecorded = true;
	original.coopPlayers.push_back( { 1, L"General \u017dukov \u6771", CVec2( 12.5f, 34.25f ) } );
	original.coopPlayers.push_back( { 7, L"AI 8", CVec2( 78.75f, 90.5f ) } );
	original.slots.resize( 3 );
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
	SMultiplayerReplayInfo copied;
	copied = restored;
	EXPECT_TRUE( copied.bCoopPlayersRecorded );
	const auto players = NGameX::GetCoopReplayPlayers( copied );
	ASSERT_EQ( players.size(), 2 );
	EXPECT_EQ( players[0].nPlayerIndex, 1 );
	EXPECT_EQ( players[0].wszName, L"General \u017dukov \u6771" );
	EXPECT_EQ( players[0].vMPStartPos, CVec2( 12.5f, 34.25f ) );
	EXPECT_EQ( players[1].nPlayerIndex, 7 );
	EXPECT_EQ( players[1].wszName, L"AI 8" );
	EXPECT_EQ( players[1].vMPStartPos, CVec2( 78.75f, 90.5f ) );
	ASSERT_EQ( copied.slots.size(), 3 );
	EXPECT_TRUE( copied.slots[0].bPresent );
	EXPECT_FALSE( copied.slots[1].bPresent );
	EXPECT_TRUE( copied.slots[2].bPresent );
}

TEST_F( CoopScenario, OldReplayClearsStaleBotSnapshotAndFallsBackToCurrentMap )
{
	SetBotName( 1, u"Authored bot" );
	SMultiplayerReplayInfo info;
	info.nMultiplayerMode = 1;
	info.pMap = map;
	CMemoryStream stream;
	{
		CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		// The older replay has a COOP mode and map, but neither presentation chunk.
		saver->Add( 2, &info.pMap );
		saver->Add( 18, &info.nMultiplayerMode );
	}
	info.bCoopPlayersRecorded = true;
	info.coopPlayers.push_back( { 99, L"Stale bot", CVec2( 1, 2 ) } );
	stream.Seek( 0 );
	{
		CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		info.operator&( *saver );
	}
	EXPECT_FALSE( info.bCoopPlayersRecorded );
	EXPECT_TRUE( info.coopPlayers.empty() );
	const auto players = NGameX::GetCoopReplayPlayers( info );
	ASSERT_EQ( players.size(), 2 );
	EXPECT_EQ( players[0].nPlayerIndex, 1 );
	EXPECT_EQ( players[0].wszName, L"Authored bot" );
	EXPECT_EQ( players[1].nPlayerIndex, 3 );
	info.pMap = nullptr;
	EXPECT_TRUE( NGameX::GetCoopReplayPlayers( info ).empty() );
}

TEST_F( CoopScenario, RecordedEmptyAndClassicReplaysDoNotInventBotsFromCurrentMap )
{
	SMultiplayerReplayInfo info;
	info.nMultiplayerMode = 1;
	info.pMap = map;
	info.bCoopPlayersRecorded = true;
	EXPECT_TRUE( NGameX::GetCoopReplayPlayers( info ).empty() );
	info.bCoopPlayersRecorded = false;
	EXPECT_EQ( NGameX::GetCoopReplayPlayers( info ).size(), 2 );
	info.nMultiplayerMode = 0;
	EXPECT_TRUE( NGameX::GetCoopReplayPlayers( info ).empty() );
	info.bCoopPlayersRecorded = true;
	info.coopPlayers = NGameX::CollectCoopMapPlayers( map );
	EXPECT_TRUE( NGameX::GetCoopReplayPlayers( info ).empty() );
}

TEST_F( CoopScenario, CoopReplayViewpointUsesSparseHumansEvenWhenBotsWin )
{
	map->players[0].ePlayerType = NDb::AI_BOT;
	map->players[0].nDiplomacySide = 1;
	map->players[1].ePlayerType = NDb::HUMAN;
	map->players[1].nDiplomacySide = 0;
	SMultiplayerReplayInfo info;
	info.nMultiplayerMode = 1;
	info.nWinningSide = 1;
	info.nScriptResult = 2;
	info.pMap = map;
	info.slots.resize( 6 );
	for ( const int nPlayer : { 0, 2, 3, 4, 5 } )
	{
		info.slots[nPlayer].bPresent = true;
		info.slots[nPlayer].nTeam = nPlayer == 3 ? 1 : 0;
	}
	// Reject a bot mislabeled team 0, the neutral owner and out-of-map indexes.
	EXPECT_EQ( NGameX::FindReplayViewpoint( info, 0 ), 2 );
	EXPECT_EQ( NGameX::FindReplayViewpoint( info, 1 ), -1 );
	info.slots[2].bPresent = false;
	EXPECT_EQ( NGameX::FindReplayViewpoint( info, 0 ), -1 );
	info.slots[1].bPresent = true;
	info.slots[1].nTeam = 0;
	EXPECT_EQ( NGameX::FindReplayViewpoint( info, 0 ), 1 );
	info.nWinningSide = 0;
	info.nScriptResult = 1;
	EXPECT_EQ( NGameX::FindReplayViewpoint( info, 0 ), 1 );
}

TEST_F( CoopScenario, ClassicReplayViewpointsRemainAvailableForBothTeams )
{
	SMultiplayerReplayInfo info;
	info.pMap = map;
	info.slots.resize( 3 );
	info.slots[1].bPresent = true;
	info.slots[1].nTeam = 0;
	info.slots[2].bPresent = true;
	info.slots[2].nTeam = 1;
	// CLASSIC ignores COOP map roles and uses its recorded network roster.
	EXPECT_EQ( NGameX::FindReplayViewpoint( info, 0 ), 1 );
	EXPECT_EQ( NGameX::FindReplayViewpoint( info, 1 ), 2 );
	info.pMap = nullptr;
	EXPECT_EQ( NGameX::FindReplayViewpoint( info, 0 ), 1 );
	EXPECT_EQ( NGameX::FindReplayViewpoint( info, 1 ), 2 );
}

TEST( CoopReplay, SinglePlayerReplayUsesPlayerZeroWithoutNetworkSlots )
{
	SMultiplayerReplayInfo info;
	info.bSinglePlayer = true;
	EXPECT_EQ( NGameX::FindReplayViewpoint( info, 0 ), 0 );
	EXPECT_EQ( NGameX::FindReplayViewpoint( info, 1 ), -1 );
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

TEST_F( CoopScenario, HistorySnapshotsBotsAtStartAndResetsThemForClassic )
{
	SetHumanRoster();
	SetBotName( 1, u"Recorded \u017dukov" );
	map->players[1].vMPStartPos = CVec2( 12.5f, 34.25f );
	CObj<ICommandsHistory> history = dynamic_cast<ICommandsHistory *>( NObjectFactory::MakeObject( 0x1007AB80 ) );
	ASSERT_NE( history, nullptr );
	NSingleton::RegisterSingleton( tracker, IScenarioTracker::tidTypeID );
	history->StartNewGame( map );
	NSingleton::UnRegisterSingleton( IScenarioTracker::tidTypeID );
	SetBotName( 1, u"Renamed after start" );
	map->players[1].vMPStartPos = CVec2( 100, 200 );
	map->players[3].ePlayerType = NDb::HUMAN;

	const auto readHistoryInfo = [&]()
	{
		CMemoryStream stream;
		{
			CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->AddPolymorphicBase( 1, history.GetPtr() );
		}
		SReplayCompletionState state;
		stream.Seek( 0 );
		{
			CObj<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
			saver->Add( 1, &state );
		}
		return state.info;
	};
	const auto recorded = readHistoryInfo();
	EXPECT_TRUE( recorded.bCoopPlayersRecorded );
	const auto players = NGameX::GetCoopReplayPlayers( recorded );
	ASSERT_EQ( players.size(), 2 );
	EXPECT_EQ( players[0].wszName, L"Recorded \u017dukov" );
	EXPECT_EQ( players[0].vMPStartPos, CVec2( 12.5f, 34.25f ) );
	EXPECT_EQ( players[1].nPlayerIndex, 3 );
	EXPECT_EQ( tracker->GetMultiplayerInfo()->players.size(), 2 );
	EXPECT_TRUE( recorded.slots.empty() );

	tracker->SetGameType( IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL );
	NSingleton::RegisterSingleton( tracker, IScenarioTracker::tidTypeID );
	history->StartNewGame( map );
	NSingleton::UnRegisterSingleton( IScenarioTracker::tidTypeID );
	const auto classic = readHistoryInfo();
	EXPECT_FALSE( classic.bCoopPlayersRecorded );
	EXPECT_TRUE( classic.coopPlayers.empty() );
}

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
