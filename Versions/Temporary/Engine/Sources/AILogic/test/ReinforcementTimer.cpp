#include "../stdafx.h"

#include "../B2AI.h"
#include "../CommonUnit.h"
#include "../DBAIConsts.h"
#include "../Diplomacy.h"
#include "../KeyBuildingBonusSystem.h"
#include "../NewUpdater.h"
#include "../PlayerReinforcement.h"
#include "../Units.h"
#include "Common_RTS_AI/AIMap.h"
#include "GameX/DBConsts.h"
#include "GameX/DBMPConsts.h"
#include "GameX/ScenarioTracker.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "Main/GameTimer.h"
#include "Stats_B2_M1/ReinfUpdates.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <limits>
#include <list>
#include <tuple>
#include <utility>

#include <gtest/gtest.h>

namespace
{
struct SSimulationTime
{
	NTimer::STime time;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 25, &time );
		return 0;
	}
};

struct SQueuedReinforcementUpdates
{
	std::list<CPtr<CEventUpdater::CUpdateData>> pending;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &pending );
		return 0;
	}
};

using TTimerPolicy = std::tuple<IAIScenarioTracker::EGameType, bool>;

class ReinforcementTimer : public testing::TestWithParam<TTimerPolicy>
{
protected:
	std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	CObj<IScenarioTracker> tracker;
	CObj<IGameTimer> timer;
	CObj<IAILogic> logic;
	CObj<CAIMap> aiMap;
	NDb::SMapInfo *map = nullptr;
	NDb::SAIGameConsts *aiConsts = nullptr;
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

	void SetSimulationTime( NTimer::STime time )
	{
		// Restore the real AI clock through its existing save format. No full world
		// segment is needed to exercise the reinforcement controller at this time.
		SSimulationTime fields{ time };
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		loader->AddPolymorphicBase( 1, logic );
	}

	void SetUp() override
	{
		ResetUpdates();
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-reinforcement-timer-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( REINFORCEMENT_TIMER_TYPES_FILE, directory / "types.xml" );
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		auto *constants = CreateRecord<NDb::SGameConsts>( "GameConsts" );
		ASSERT_NE( constants, nullptr );
		NGlobal::SetVar( "game_consts", "1.xdb" );
		aiConsts = CreateRecord<NDb::SAIGameConsts>( "AIGameConsts" );
		ASSERT_NE( aiConsts, nullptr );
		constants->pAI = aiConsts;
		multiplayerConsts = CreateRecord<NDb::SMultiplayerConsts>( "MultiplayerConsts" );
		ASSERT_NE( multiplayerConsts, nullptr );
		multiplayerConsts->vReinfCounterRecycle = CVec2( 2.0f, 2.0f );
		constants->pMultiplayer = multiplayerConsts;
		map = CreateRecord<NDb::SMapInfo>( "MapInfo" );
		ASSERT_NE( map, nullptr );
		map->players.resize( 3 );
		map->players[0].nReinforcementCalls = 5;
		map->players[1].nDiplomacySide = 1;
		map->players[2].nDiplomacySide = 2;
		// A key building makes the real tracker's countdown observable through
		// GetReinforcementXP, which deliberately hides it on maps without flags.
		map->playerBonusObjects.resize( 1 );
		map->playerBonusObjects[0].nLinkID = 7;
		timer = dynamic_cast<IGameTimer *>( NObjectFactory::MakeObject( 0x10075C05 ) );
		ASSERT_NE( timer, nullptr );
		NSingleton::RegisterSingleton( timer, IGameTimer::tidTypeID );
		tracker = CreateScenarioTrackerMultiplayer();
		ASSERT_NE( tracker, nullptr );
		tracker->SetGameType( std::get<0>( GetParam() ) );
		tracker->MissionStart( map, 0 );
		tracker->AddPlayer( 0 );
		tracker->SetPlayerSide( 0, 0 );
		tracker->SetLocalPlayer( 0 );
		tracker->SetAddReinfsWithTimer( TimerEnabled() );
		tracker->SetReinforcementXP( 0, NDb::_RT_NONE, 19.0f );
		NSingleton::RegisterSingleton( tracker, IAIScenarioTracker::tidTypeID );
		logic = dynamic_cast<IAILogic *>( NObjectFactory::MakeObject( 0x1108D441 ) );
		ASSERT_NE( logic, nullptr );
		NSingleton::RegisterSingleton( logic, IAILogic::tidTypeID );
		SetSimulationTime( 0 );
		logic->SetNPlayers( 2 );
		logic->SetNetGame( true );
		logic->SetMyDiplomacyInfo( -1, 0 );
		NGlobal::SetVar( "multiplayer_flag_calls_bonus", 1 );
		ASSERT_FALSE( theDipl.IsAIPlayer( 0 ) );
	}

	void TearDown() override
	{
		// Notifications hold database references, so release them before closing this fixture's database.
		ResetUpdates();
		theReinfArray.clear();
		NSingleton::UnRegisterSingleton( IAILogic::tidTypeID );
		logic = nullptr;
		SetAIMap( nullptr );
		aiMap = nullptr;
		NSingleton::UnRegisterSingleton( IAIScenarioTracker::tidTypeID );
		tracker = nullptr;
		NSingleton::UnRegisterSingleton( IGameTimer::tidTypeID );
		timer = nullptr;
		NGlobal::RemoveVar( "multiplayer_flag_calls_bonus" );
		NGlobal::RemoveVar( "game_consts" );
		records.clear();
		NDb::CloseDatabase();
		std::error_code error;
		std::filesystem::remove_all( directory, error );
	}

	bool TimerEnabled() const
	{
		return std::get<1>( GetParam() );
	}

	float Countdown() const
	{
		return tracker->GetReinforcementXP( 0, NDb::_RT_NONE );
	}

	void CopyController( CPlayerReinforcement *source, CPlayerReinforcement *target )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->Add( 1, source );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		loader->Add( 1, target );
	}

	void ResetUpdates()
	{
		SQueuedReinforcementUpdates empty;
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->Add( 1, &empty );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		loader->Add( 1, &updater );
	}

	SQueuedReinforcementUpdates ReadUpdates()
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->Add( 1, &updater );
		}
		stream.Seek( 0 );
		SQueuedReinforcementUpdates result;
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		loader->Add( 1, &result );
		return result;
	}
};

TEST_P( ReinforcementTimer, InitializationAndExplicitGrantsRespectTimerPolicy )
{
	CPlayerReinforcement controller;
	controller.InitPlayerReinforcement( 0, map, aiConsts );
	EXPECT_FLOAT_EQ( Countdown(), TimerEnabled() ? 2.0f : 19.0f );
	controller.SetRecycleCoeff( 4.0f );
	EXPECT_FLOAT_EQ( Countdown(), TimerEnabled() ? 4.0f : 19.0f );
	controller.GiveReinforcementCalls( 3, true );
	EXPECT_EQ( tracker->GetReinforcementCallsLeft( 0 ), 8 );
	EXPECT_FLOAT_EQ( Countdown(), TimerEnabled() ? 4.0f : 19.0f );
	controller.GiveReinforcementCalls( -2, false );
	EXPECT_EQ( tracker->GetReinforcementCallsLeft( 0 ), 6 );
	controller.Segment();
	EXPECT_EQ( tracker->GetReinforcementCallsLeft( 0 ), 6 );
	EXPECT_FLOAT_EQ( Countdown(), TimerEnabled() ? 4.0f : 19.0f );
}

TEST_P( ReinforcementTimer, BaseOwnershipUsesSignedMapBonusesForEveryTeammate )
{
	// Two teammates on each side and a neutral slot exercise the real ownership handler.
	map->players.resize( 5 );
	for ( int i = 0; i < 5; ++i )
	{
		map->players[i].nReinforcementCalls = 10;
		map->players[i].nDiplomacySide = i / 2;
	}
	logic->SetNPlayers( 4 );
	for ( int i = 0; i < 5; ++i )
	{
		theDipl.SetParty( i, i / 2 );
	}
	CKeyBuildingBonusSystem bonuses;
	bonuses.InitBonusSystem( map );
	for ( const auto values : { std::pair<int, int>( 1, -1 ), { 3, -2 }, { 0, 0 }, { -2, 4 }, { 3, -50 } } )
	{
		SCOPED_TRACE( testing::Message() << values.first << ", " << values.second );
		tracker->MissionStart( map, 0 );
		EXPECT_EQ( tracker->GetOnBaseCaptureReinfPointsGain(), 1 );
		EXPECT_EQ( tracker->GetOnBaseLostReinfPointsLoss(), -1 );
		tracker->SetAddReinfsWithTimer( TimerEnabled() );
		tracker->SetOnBaseCaptureReinfPointsGain( values.first );
		tracker->SetOnBaseLostReinfPointsLoss( values.second );
		for ( int i = 0; i < 4; ++i )
		{
			tracker->AddPlayer( i );
			tracker->SetPlayerSide( i, i / 2 );
		}
		theReinfArray.InitPlayerReinforcementArray( map, aiConsts );
		bonuses.ChangeOwnership( 4, 0, 7, true );
		// Initial placement, same-side transfers and non-base buildings grant nothing.
		bonuses.ChangeOwnership( 0, 1, 7, false );
		bonuses.ChangeOwnership( 0, 2, 999, false );
		for ( int i = 0; i < 5; ++i )
		{
			EXPECT_EQ( tracker->GetReinforcementCallsLeft( i ), 10 );
		}
		bonuses.ChangeOwnership( 0, 2, 7, false );
		for ( int i = 0; i < 4; ++i )
		{
			EXPECT_EQ( tracker->GetReinforcementCallsLeft( i ),
			           (std::max)( 0, 10 + ( i < 2 ? values.second : values.first ) ) );
			EXPECT_EQ( tracker->GetStatistics( i, IScenarioTracker::ESK_REINFORCEMENTS_CALLED ), 0 );
		}
		EXPECT_EQ( tracker->GetReinforcementCallsLeft( 4 ), 10 );
		// Capturing from neutral applies only the gain, without charging the neutral player.
		bonuses.ChangeOwnership( 2, 4, 7, true );
		const int before = tracker->GetReinforcementCallsLeft( 0 );
		bonuses.ChangeOwnership( 4, 0, 7, false );
		EXPECT_EQ( tracker->GetReinforcementCallsLeft( 0 ), (std::max)( 0, before + values.first ) );
		EXPECT_EQ( tracker->GetReinforcementCallsLeft( 4 ), 10 );
	}
}

TEST_P( ReinforcementTimer, AuthoredBonusesSaturateAtIntegerLimits )
{
	CPlayerReinforcement controller;
	controller.InitPlayerReinforcement( 0, map, aiConsts );
	controller.GiveReinforcementCalls( ( std::numeric_limits<int>::max )(), false );
	EXPECT_EQ( tracker->GetReinforcementCallsLeft( 0 ), ( std::numeric_limits<int>::max )() );
	controller.GiveReinforcementCalls( 1, false );
	EXPECT_EQ( tracker->GetReinforcementCallsLeft( 0 ), ( std::numeric_limits<int>::max )() );
	SetSimulationTime( 2001 );
	controller.Segment();
	EXPECT_EQ( tracker->GetReinforcementCallsLeft( 0 ), ( std::numeric_limits<int>::max )() );
	controller.GiveReinforcementCalls( ( std::numeric_limits<int>::min )(), false );
	EXPECT_EQ( tracker->GetReinforcementCallsLeft( 0 ), 0 );
}

TEST_P( ReinforcementTimer, ElapsedTimerAndSaveLoadGrantOnlyWhenEnabled )
{
	CPlayerReinforcement controller;
	controller.InitPlayerReinforcement( 0, map, aiConsts );
	CPlayerReinforcement restored;
	CopyController( &controller, &restored );
	SetSimulationTime( 2000 );
	restored.Segment();
	// The existing cadence grants after, rather than at, the saved deadline.
	EXPECT_EQ( tracker->GetReinforcementCallsLeft( 0 ), 5 );
	SetSimulationTime( 2001 );
	restored.Segment();
	EXPECT_EQ( tracker->GetReinforcementCallsLeft( 0 ), TimerEnabled() ? 6 : 5 );
	EXPECT_FLOAT_EQ( Countdown(), TimerEnabled() ? 1.0f : 19.0f );
	CopyController( &restored, &controller );
	controller.Segment();
	EXPECT_EQ( tracker->GetReinforcementCallsLeft( 0 ), TimerEnabled() ? 6 : 5 );
	SetSimulationTime( 4001 );
	controller.Segment();
	EXPECT_EQ( tracker->GetReinforcementCallsLeft( 0 ), TimerEnabled() ? 7 : 5 );
	EXPECT_FLOAT_EQ( Countdown(), TimerEnabled() ? 1.0f : 19.0f );
}

TEST_P( ReinforcementTimer, DisabledStartingArmyStillEnablesOrdinaryReinforcements )
{
	auto *party = CreateRecord<NDb::SPartyDependentInfo>( "PartyDependentInfo" );
	auto *recipe = CreateRecord<NDb::SReinforcement>( "Reinforcement" );
	auto *vehicle = CreateRecord<NDb::SMechUnitRPGStats>( "MechUnitRPGStats" );
	auto *deployment = CreateRecord<NDb::SDeployTemplate>( "DeployTemplate" );
	ASSERT_NE( party, nullptr );
	ASSERT_NE( recipe, nullptr );
	ASSERT_NE( vehicle, nullptr );
	ASSERT_NE( deployment, nullptr );
	vehicle->eDBtype = NDb::DB_RPG_TYPE_ARM_MEDIUM;
	recipe->entries.resize( 1 );
	recipe->entries[0].pMechUnit = vehicle;
	deployment->entries.resize( 1 );
	multiplayerConsts->sides.resize( 1 );
	auto &side = multiplayerConsts->sides[0];
	side.pPartyInfo = party;
	side.techLevels.resize( 1 );
	side.techLevels[0].pStartingUnits = recipe;
	side.techLevels[0].reinforcements.push_back( recipe );
	auto &player = map->players[0];
	player.pPartyInfo = party;
	player.reinforcementTypes.push_back( recipe );
	player.reinforcementPoints.resize( 1 );
	player.reinforcementPoints[0].bIsDefault = false;
	player.reinforcementPoints[0].pTemplate = deployment;
	tracker->MissionStart( map, 0 );
	tracker->AddPlayer( 0 );
	tracker->SetPlayerSide( 0, 0 );
	tracker->SetPlayerParty( 0, 0 );
	tracker->SetSpawnStartingUnits( false );
	tracker->SetAddReinfsWithTimer( TimerEnabled() );
	ASSERT_EQ( tracker->GetStartUnits( 0 ), recipe );
	ASSERT_EQ( tracker->GetReinforcement( 0, recipe->eType ), recipe );
	// Initialize only the real unit registry, allowing a direct no-spawn check
	// without loading a scene. The configured starting recipe is nonempty.
	aiMap = new CAIMap( 256, 256, 32, 2, 16 );
	SetAIMap( aiMap );
	units.Init();
	ASSERT_EQ( logic->GetUnitCount( 0 ), 0 );
	CPlayerReinforcement controller;
	controller.InitPlayerReinforcement( 0, map, aiConsts );
	ASSERT_FALSE( controller.CanCallNow() );
	controller.PlaceInitialUnits();
	EXPECT_TRUE( controller.CanCallNow() );
	EXPECT_EQ( logic->GetUnitCount( 0 ), 0 );
	EXPECT_EQ( tracker->GetReinforcementCallsLeft( 0 ), 5 );
	CPlayerReinforcement restored;
	CopyController( &controller, &restored );
	EXPECT_TRUE( restored.CanCallNow() );
}

TEST_P( ReinforcementTimer, DefaultEntryWithoutOwnedBasesPublishesAvailableTypesAndEnabledButton )
{
	// Match The Encirclement: two human slots have eight types, twenty credits and
	// an independent default entry. Three enemy-held bases unlock other entries.
	auto *party = CreateRecord<NDb::SPartyDependentInfo>( "PartyDependentInfo" );
	ASSERT_NE( party, nullptr );
	const std::vector<NDb::EReinforcementType> types{ NDb::RT_MAIN_INFANTRY,
	                                                  NDb::RT_ASSAULT_INFANTRY,
	                                                  NDb::RT_ARTILLERY_ANTITANK,
	                                                  NDb::RT_LIGHT_TANKS,
	                                                  NDb::RT_TANKS,
	                                                  NDb::RT_HEAVY_TANKS,
	                                                  NDb::RT_FIGHTERS,
	                                                  NDb::RT_ENGINEERING };
	multiplayerConsts->sides.resize( 1 );
	auto &side = multiplayerConsts->sides[0];
	side.pPartyInfo = party;
	side.techLevels.resize( 3 );
	for ( auto type : types )
	{
		auto *recipe = CreateRecord<NDb::SReinforcement>( "Reinforcement" );
		ASSERT_NE( recipe, nullptr );
		recipe->eType = type;
		side.techLevels[2].reinforcements.push_back( recipe );
	}
	side.techLevels[2].pStartingUnits = side.techLevels[2].reinforcements.front();
	map->players.resize( 4 );
	for ( int player = 0; player < 2; ++player )
	{
		auto &info = map->players[player];
		info.nDiplomacySide = 0;
		info.pPartyInfo = party;
		info.nReinforcementCalls = 20;
		info.reinforcementTypes = side.techLevels[2].reinforcements;
		info.reinforcementPoints.resize( 4 );
		info.reinforcementPoints[0].bIsDefault = true;
	}
	map->players[2].nDiplomacySide = 1;
	map->players[2].ePlayerType = NDb::AI_BOT;
	map->players[3].nDiplomacySide = 2;
	map->playerBonusObjects.resize( 3 );
	for ( int base = 0; base < 3; ++base )
	{
		auto &bonus = map->playerBonusObjects[base];
		bonus.nLinkID = 100 + base;
		bonus.playerBonuses.resize( 3 );
		for ( int player = 0; player < 2; ++player )
		{
			bonus.playerBonuses[player].nPointID = base + 1;
		}
	}
	logic->SetNPlayers( 3 );
	theDipl.SetParty( 2, 1 );
	for ( int local = 0; local < 2; ++local )
	{
		SCOPED_TRACE( local );
		tracker->MissionStart( map, 2 );
		for ( int player = 0; player < 3; ++player )
		{
			tracker->AddPlayer( player );
		}
		for ( int player = 0; player < 2; ++player )
		{
			tracker->SetPlayerParty( player, 0 );
		}
		tracker->SetLocalPlayer( local );
		tracker->SetSpawnStartingUnits( false );
		tracker->SetAddReinfsWithTimer( TimerEnabled() );
		logic->SetMyDiplomacyInfo( -1, local );
		for ( const auto &base : map->playerBonusObjects )
		{
			tracker->KeyBuildingOwnerChange( base.nLinkID, 2 );
		}
		ResetUpdates();
		CPlayerReinforcement controller;
		controller.InitPlayerReinforcement( local, map, aiConsts );
		ASSERT_TRUE( controller.CanCallNow() );
		controller.PlaceInitialUnits();
		controller.Segment();
		EXPECT_EQ( tracker->GetReinforcementCallsLeft( local ), 20 );
		const auto updates = ReadUpdates();
		std::vector<NDb::EReinforcementType> announced;
		bool entryEnabled = false;
		bool buttonEnabled = false;
		for ( const auto &update : updates.pending )
		{
			ASSERT_TRUE( update->bValid );
			if ( const auto *available = dynamic_cast<const SAIAvailableReinfUpdate *>( update->pData.GetPtr() ) )
			{
				ASSERT_NE( available->pReinf, nullptr );
				EXPECT_TRUE( available->bEnabled );
				EXPECT_EQ( available->nReinforcementCallsLeft, 20 );
				announced.push_back( available->pReinf->eType );
			}
			if ( const auto *point = dynamic_cast<const SAIReinfPointUpdate *>( update->pData.GetPtr() ) )
			{
				EXPECT_EQ( point->nPointID, 0 );
				EXPECT_TRUE( point->bEnable );
				entryEnabled = entryEnabled || point->bEnable;
			}
			if ( const auto *recycle = dynamic_cast<const SAIReinfRecycleUpdate *>( update->pData.GetPtr() ) )
			{
				EXPECT_TRUE( recycle->bEnabled );
				EXPECT_EQ( recycle->timeRecycleEnd, 0u );
				buttonEnabled = recycle->bEnabled;
			}
		}
		std::sort( announced.begin(), announced.end() );
		EXPECT_EQ( announced, types );
		EXPECT_TRUE( entryEnabled );
		EXPECT_TRUE( buttonEnabled );
		CPlayerReinforcement restored;
		CopyController( &controller, &restored );
		EXPECT_TRUE( restored.CanCallNow() );
	}
}

INSTANTIATE_TEST_SUITE_P( MultiplayerModes,
                          ReinforcementTimer,
                          testing::Combine( testing::Values( IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL,
                                                             IAIScenarioTracker::EGT_MULTI_COOP ),
                                            testing::Bool() ) );
} // namespace
