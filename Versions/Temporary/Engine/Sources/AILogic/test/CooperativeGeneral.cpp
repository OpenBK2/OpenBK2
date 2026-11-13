#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../AIUnitInfoForGeneral.h"
#include "../B2AI.h"
#include "../DBAIConsts.h"
#include "../Diplomacy.h"
#include "../Formation.h"
#include "../GeneralAirForce.h"
#include "../GeneralArtillery.h"
#include "../GeneralConsts.h"
#include "../GeneralInternal.h"
#include "../PlayerReinforcement.h"
#include "../Technics.h"
#include "../Weather.h"
#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/Checksums.h"
#include "GameX/DBConsts.h"
#include "GameX/DBMPConsts.h"
#include "GameX/ScenarioTracker.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "Main/GameTimer.h"
#include "System/RandomGen.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <functional>
#include <type_traits>

#include <gtest/gtest.h>

namespace
{
struct SFields
{
	std::function<void( IBinSaver & )> visit;

	int operator&( IBinSaver &saver )
	{
		visit( saver );
		return 0;
	}
};

using TEnemies = det_map<int, std::pair<CPtr<CAIUnit>, NTimer::STime>>;
using TGenerals = det_map<int, CObj<CGeneral>>;

struct SGeneralState
{
	int party = -1;
	int player = -1;
	int nextEnemy = -1;
	NTimer::STime nextUpdate = 0;
	TEnemies enemies;
	CObj<CGeneralAirForce> air;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 3, &party );
		saver.Add( 4, &enemies );
		saver.Add( 11, &nextUpdate );
		saver.Add( 15, &air );
		saver.Add( 24, &player );
		saver.Add( 25, &nextEnemy );
		return 0;
	}
};

struct SResistanceState
{
	CResistance cells;
	det_map<int, std::pair<int, float>> units;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &cells );
		saver.Add( 6, &units );
		return 0;
	}
};

struct SAirState
{
	int party = 1;
	std::vector<int> players;
	CFreeIds requestIDs;
	NTimer::STime waitUntil = 0;
	EForceType current = FT_NONE;
	bool turn = true;
	CGeneralAirForce::RequestsByForceType requests;
	int owner = -1;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &party );
		saver.Add( 3, &players );
		saver.Add( 4, &requestIDs );
		saver.Add( 7, &waitUntil );
		saver.Add( 8, &current );
		saver.Add( 9, &turn );
		saver.Add( 10, &requests );
		saver.Add( 11, &owner );
		return 0;
	}
};

struct SQueuedAirCall
{
	NDb::EReinforcementType type = NDb::_RT_NONE;
	CVec2 point;
	bool general = false;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &type );
		saver.Add( 3, &point );
		saver.Add( 4, &general );
		return 0;
	}
};

void ExpectSameRandom( const NRandom::SDebugState &a, const NRandom::SDebugState &b )
{
	EXPECT_EQ( a.randcnt, b.randcnt );
	EXPECT_EQ( a.randa, b.randa );
	EXPECT_EQ( a.randb, b.randb );
	EXPECT_EQ( a.randc, b.randc );
	EXPECT_EQ( a.randrslChecksum, b.randrslChecksum );
	EXPECT_EQ( a.randmemChecksum, b.randmemChecksum );
	EXPECT_EQ( a.randomCalls, b.randomCalls );
}

class CooperativeGeneral : public testing::Test
{
protected:
	std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	std::vector<CObj<CObjectBase>> objects;
	std::vector<SBinSaverExternalObject> external;
	CObj<IScenarioTracker> tracker;
	CObj<IAILogic> logic;
	CObj<IGameTimer> timer;
	CObj<CAIMap> aiMap;
	CObj<IRandomSeed> seed;
	NDb::SMapInfo *map = nullptr;
	NDb::SAIGameConsts *aiConsts = nullptr;
	NDb::SMultiplayerConsts *mpConsts = nullptr;

	template <class T>
	T *MakeRecord( const char *type )
	{
		CObj<NDb::IObjMan> record = NDb::CreateNewObject( type );
		EXPECT_NE( record, nullptr );
		if ( !record )
		{
			return nullptr;
		}
		records.push_back( record );
		const std::string name = std::to_string( records.size() ) + ".xdb";
		EXPECT_TRUE( NDb::AddNewObject( name, CDBID( name ), record ) );
		return dynamic_cast<T *>( record->GetObject() );
	}

	template <class T>
	T *MakeObject( int type )
	{
		T *object = dynamic_cast<T *>( NObjectFactory::MakeObject( type ) );
		EXPECT_NE( object, nullptr );
		if ( object )
		{
			objects.push_back( object );
			external.emplace_back( int( external.size() + 1 ), object );
		}
		return object;
	}

	template <class T>
	void Visit( IBinSaver &saver, T *object )
	{
		if constexpr ( std::is_base_of_v<CObjectBase, T> )
		{
			saver.AddPolymorphicBase( 1, object );
		}
		else
		{
			saver.Add( 1, object );
		}
	}

	template <class TSource, class TTarget>
	void Copy( TSource *source, TTarget *target )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			Visit( *saver, source );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		Visit( *loader, target );
	}

	void SetTime( NTimer::STime time )
	{
		SFields fields{ [&]( IBinSaver &saver ) { saver.Add( 25, &time ); } };
		Copy( &fields, logic.GetPtr() );
		// Partial AI hydration does not carry its map pointer.
		SetAIMap( aiMap );
	}

	void SetMode( IAIScenarioTracker::EGameType type, int local = 0 )
	{
		tracker->SetGameType( type );
		tracker->MissionStart( map, 0 );
		tracker->SetLocalPlayer( local );
		std::vector<int> parties{ 0, 0, 0, 1, 0, 1, 2 };
		std::vector<int> present( 7, 1 );
		std::vector<int> bots{ 0, 0, 0, 1, 0, 1, 1 };
		bool net = type != IAIScenarioTracker::EGT_SINGLE;
		SFields diplomacy{ [&]( IBinSaver &saver )
		                   {
												 saver.Add( 1, &parties );
												 saver.Add( 2, &local );
												 saver.Add( 3, &net );
												 saver.Add( 4, &present );
												 saver.Add( 5, &bots );
											 } };
		Copy( &diplomacy, &theDipl );
	}

	void SetUp() override
	{
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-cooperative-general-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( COOPERATIVE_GENERAL_TYPES_FILE, directory / "types.xml" );
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		auto *constants = MakeRecord<NDb::SGameConsts>( "GameConsts" );
		ASSERT_NE( constants, nullptr );
		NGlobal::SetVar( "game_consts", "1.xdb" );
		aiConsts = MakeRecord<NDb::SAIGameConsts>( "AIGameConsts" );
		constants->pAI = aiConsts;
		mpConsts = MakeRecord<NDb::SMultiplayerConsts>( "MultiplayerConsts" );
		constants->pMultiplayer = mpConsts;
		map = MakeRecord<NDb::SMapInfo>( "MapInfo" );
		ASSERT_NE( map, nullptr );
		map->players.resize( 7 );
		for ( int i = 0; i != 7; ++i )
		{
			map->players[i].ePlayerType = i == 3 || i == 5 || i == 6 ? NDb::AI_BOT : NDb::HUMAN;
			map->players[i].nDiplomacySide = i == 6 ? 2 : ( i == 3 || i == 5 ? 1 : 0 );
		}
		tracker = CreateScenarioTrackerMultiplayer();
		timer = dynamic_cast<IGameTimer *>( NObjectFactory::MakeObject( 0x10075C05 ) );
		ASSERT_NE( timer, nullptr );
		NSingleton::RegisterSingleton( timer, IGameTimer::tidTypeID );
		NSingleton::RegisterSingleton( tracker, IAIScenarioTracker::tidTypeID );
		logic = dynamic_cast<IAILogic *>( NObjectFactory::MakeObject( 0x1108D441 ) );
		ASSERT_NE( logic, nullptr );
		NSingleton::RegisterSingleton( logic, IAILogic::tidTypeID );
		aiMap = new CAIMap( 32, 32, 32, 2, 16 );
		// The real global load initializes derived AI constants (including the
		// intendant cell size) without starting a renderer or running map units.
		SFields globals{ []( IBinSaver & ) {} };
		SFields initialized{ [&]( IBinSaver &saver ) { saver.Add( 1, &globals ); } };
		Copy( &initialized, logic.GetPtr() );
		SetTime( 0 );
		SetMode( IAIScenarioTracker::EGT_MULTI_COOP );
		SGeneralConsts::Init( true );
		theReinfArray.InitPlayerReinforcementArray( map, aiConsts );
		seed = NRandom::CreateRandomSeedCopy();
		seed->InitByZeroSeed();
		NRandom::SetRandomSeed( seed );
	}

	void TearDown() override
	{
		SetWeather( NDb::EWS_CLEAR );
		NSingleton::UnRegisterSingleton( IAILogic::tidTypeID );
		logic = nullptr;
		objects.clear();
		theReinfArray.clear();
		SetAIMap( nullptr );
		aiMap = nullptr;
		NSingleton::UnRegisterSingleton( IAIScenarioTracker::tidTypeID );
		tracker = nullptr;
		NSingleton::UnRegisterSingleton( IGameTimer::tidTypeID );
		timer = nullptr;
		NGlobal::RemoveVar( "game_consts" );
		NGlobal::RemoveVar( "nogeneral" );
		NGlobal::RemoveVar( "temp.nogeneral_sript" );
		NGlobal::RemoveVar( "temp.general_reinforcement" );
		NGlobal::RemoveVar( "AI.General.UpdatePeriod" );
		NGlobal::RemoveVar( "multiplayer_unit_experience" );
		records.clear();
		NDb::CloseDatabase();
		NVFS::SetMainVFS( nullptr );
		NVFS::SetMainFileCreator( nullptr );
		std::error_code error;
		std::filesystem::remove_all( directory, error );
	}

	void SetWeather( NDb::EWeatherState state )
	{
		SFields fields{ [&]( IBinSaver &saver ) { saver.Add( 2, &state ); } };
		Copy( &fields, &theWeather );
	}

	NDb::SReinforcement *AddAirReinforcement( int player, NDb::EReinforcementType type, int pilotLevel = -1 )
	{
		auto *reinforcement = MakeRecord<NDb::SReinforcement>( "Reinforcement" );
		reinforcement->eType = type;
		auto *plane = MakeRecord<NDb::SMechUnitRPGStats>( "MechUnitRPGStats" );
		plane->etype = NDb::RPG_TYPE_AVIA_ATTACK;
		plane->eDBtype = NDb::DB_RPG_TYPE_AVIA_ATTACK;
		auto *actions = MakeRecord<NDb::SUnitActions>( "UnitActions" );
		plane->pActions = actions;
		if ( pilotLevel >= 0 )
		{
			auto *pilot = MakeRecord<NDb::SUnitSpecialAblityDesc>( "UnitSpecialAblityDesc" );
			pilot->eName = NDb::ABILITY_MASTER_PILOT;
			actions->specialAbilities.resize( pilotLevel + 1 );
			for ( int i = 0; i < pilotLevel; ++i )
			{
				auto *ability = MakeRecord<NDb::SUnitSpecialAblityDesc>( "UnitSpecialAblityDesc" );
				ability->eName = NDb::ABILITY_AMBUSH;
				actions->specialAbilities[i] = ability;
			}
			actions->specialAbilities[pilotLevel] = pilot;
		}
		reinforcement->entries.resize( 1 );
		reinforcement->entries[0].pMechUnit = plane;
		map->players[player].reinforcementTypes.push_back( reinforcement );
		map->players[player].nReinforcementCalls = 10;
		map->players[player].fRecycleTimeCoefficient = 1;
		map->players[player].reinforcementPoints.resize( 1 );
		map->players[player].reinforcementPoints[0].bIsDefault = true;
		return reinforcement;
	}

	void InitAirReinforcements( int viewpoint = 0 )
	{
		SetMode( IAIScenarioTracker::EGT_MULTI_COOP, viewpoint );
		theReinfArray.clear();
		theReinfArray.InitPlayerReinforcementArray( map, aiConsts );
	}

	CGeneralAirForce *MakeAirController( int player )
	{
		auto *air = MakeObject<CGeneralAirForce>( 0x1508D4AE );
		SAirState state;
		state.owner = player;
		state.players.push_back( player );
		Copy( &state, air );
		return air;
	}

	SAirState AirState( CGeneralAirForce *air )
	{
		SAirState state;
		Copy( air, &state );
		return state;
	}

	std::vector<SQueuedAirCall> QueuedAirCalls( CPlayerReinforcement *reinforcement )
	{
		std::vector<SQueuedAirCall> commands;
		SFields fields{ [&]( IBinSaver &saver ) { saver.Add( 11, &commands ); } };
		Copy( reinforcement, &fields );
		return commands;
	}

	CGeneral *MakeGeneral( int player, const TEnemies &remembered = {} )
	{
		auto *general = MakeObject<CGeneral>( 0x1508D4B3 );
		CObj<CGeneralAirForce> air = MakeObject<CGeneralAirForce>( 0x1508D4AE );
		CObj<CGeneralArtillery> artillery = MakeObject<CGeneralArtillery>( 0x1508D4AF );
		int party = 1;
		int cursor = -1;
		NTimer::STime deadline = 0;
		TEnemies enemies = remembered;
		SFields fields{ [&]( IBinSaver &saver )
		                {
											saver.Add( 3, &party );
											saver.Add( 4, &enemies );
											saver.Add( 11, &deadline );
											saver.Add( 15, &air );
											saver.Add( 16, &artillery );
											saver.Add( 24, &player );
											saver.Add( 25, &cursor );
										} };
		Copy( &fields, general );
		return general;
	}

	SGeneralState State( CGeneral *general )
	{
		SGeneralState result;
		Copy( general, &result );
		return result;
	}

	CAIUnitInfoForGeneral *MakeEnemyInfo( int id )
	{
		auto *unit = MakeObject<CTank>( AI_TANK );
		unit->nUniqueID = id;
		auto *stats = MakeRecord<NDb::SMechUnitRPGStats>( "MechUnitRPGStats" );
		stats->etype = NDb::RPG_TYPE_ARM_MEDIUM;
		stats->fPrice = 100;
		stats->fMaxHP = 100;
		stats->fSpeed = 10;
		stats->armors.resize( NDb::RPG_BOTTOM + 1 );
		CDBPtr<NDb::SMechUnitRPGStats> statsPointer = stats;
		CObj<NDb::SUnitStatsModifier> modifier = MakeRecord<NDb::SUnitStatsModifier>( "UnitStatsModifier" );
		objects.push_back( modifier.GetPtr() );
		external.emplace_back( int( external.size() + 1 ), modifier );
		CUnitStatsModifiers modifiers;
		modifiers.Set( "test.fixture", modifier );
		uint8_t player = 0;
		float hitPoints = 100;
		SFields ai{ [&]( IBinSaver &saver )
		            {
									saver.Add( 6, &player );
									saver.Add( 15, &hitPoints );
									saver.Add( 58, &modifiers );
								} };
		SFields mech{ [&]( IBinSaver &saver )
		              {
										saver.Add( 1, &ai );
										saver.Add( 2, &statsPointer );
									} };
		SFields tank{ [&]( IBinSaver &saver ) { saver.Add( 1, &mech ); } };
		Copy( &tank, unit );
		auto *info = MakeObject<CAIUnitInfoForGeneral>( 0x1508D4AC );
		CPtr<CAIUnit> owner = unit;
		SFields fields{ [&]( IBinSaver &saver ) { saver.Add( 2, &owner ); } };
		Copy( &fields, info );
		return info;
	}

	uint32_t ResistanceChecksum( CResistancesContainer *resistance )
	{
		uint32_t checksum = 1;
		{
			CPtr<IBinSaver> saver = CreateCheckSumSaver( &checksum, nullptr, 0 );
			saver->Add( 1, resistance );
		}
		return checksum;
	}

	void InstallGenerals( CSupremeBeing *supreme, TGenerals *generals )
	{
		SFields fields{ [&]( IBinSaver &saver ) { saver.Add( 1, generals ); } };
		Copy( &fields, supreme );
	}

	uint32_t SimulationChecksum()
	{
		uint32_t checksum = 1;
		{
			CPtr<IBinSaver> saver = CreateCheckSumSaver( &checksum, nullptr, 0 );
			saver->AddPolymorphicBase( 1, logic );
		}
		SetAIMap( aiMap );
		return checksum;
	}
};
} // namespace

TEST_F( CooperativeGeneral, CreatesOneOwnerPerBotAndKeepsClassicAndSinglePlayerPolicies )
{
	for ( const int viewpoint : { 0, 2, 3, 5 } )
	{
		SCOPED_TRACE( viewpoint );
		SetMode( IAIScenarioTracker::EGT_MULTI_COOP, viewpoint );
		// A local debugging variable cannot disable synchronized AI on one peer.
		NGlobal::SetVar( "nogeneral", viewpoint == 3 );
		CSupremeBeing supreme;
		supreme.Init( map );
		for ( int player = 0; player != 7; ++player )
		{
			auto *general = dynamic_cast<CGeneral *>( supreme.GetEnemyConatiner( player ) );
			if ( player == 3 || player == 5 )
			{
				ASSERT_NE( general, nullptr );
				EXPECT_EQ( general->GetPlayer(), player );
				EXPECT_EQ( general->GetParty(), 1 );
				SGeneralState state = State( general );
				ASSERT_NE( state.air, nullptr );
				std::vector<int> airPlayers;
				int airOwner = -1;
				SFields airState{ [&]( IBinSaver &saver )
				                  {
														saver.Add( 3, &airPlayers );
														saver.Add( 11, &airOwner );
													} };
				Copy( state.air.GetPtr(), &airState );
				EXPECT_EQ( airPlayers, std::vector<int>{ player } );
				EXPECT_EQ( airOwner, player );
				for ( const int unitOwner : { 0, 2, 3, 5, 6 } )
				{
					auto *formation = MakeObject<CFormation>( 0x1108D498 );
					uint8_t owner = uint8_t( unitOwner );
					SFields fields{ [&]( IBinSaver &saver ) { saver.Add( 5, &owner ); } };
					Copy( &fields, formation );
					EXPECT_EQ( general->OwnsUnit( formation ), player == unitOwner );
				}
			}
			else
			{
				EXPECT_EQ( general, nullptr );
			}
		}
		supreme.Clear();
	}
	NGlobal::SetVar( "nogeneral", false );
	CSupremeBeing supreme;
	SetMode( IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL );
	supreme.Init( map );
	EXPECT_EQ( supreme.GetEnemyConatiner( 3 ), nullptr );
	EXPECT_EQ( supreme.GetEnemyConatiner( 5 ), nullptr );
	SetMode( IAIScenarioTracker::EGT_SINGLE );
	supreme.Init( map );
	auto *single = dynamic_cast<CGeneral *>( supreme.GetEnemyConatiner( 3 ) );
	ASSERT_NE( single, nullptr );
	EXPECT_EQ( single->GetParty(), 1 );
	EXPECT_EQ( supreme.GetEnemyConatiner( 5 ), nullptr );
	supreme.Clear();
}

TEST_F( CooperativeGeneral, MultiBotUpdatesAndRandomStateIgnoreInsertionOrderAndViewpoint )
{
	std::vector<NTimer::STime> expected;
	NRandom::SDebugState expectedRandom{};
	for ( const int viewpoint : { 0, 2, 3, 5 } )
	{
		for ( const bool reverse : { false, true } )
		{
			SCOPED_TRACE( testing::Message() << viewpoint << ", reverse=" << reverse );
			SetMode( IAIScenarioTracker::EGT_MULTI_COOP, viewpoint );
			NGlobal::SetVar( "AI.General.UpdatePeriod", viewpoint == 0 ? 50 : 90000 );
			NGlobal::SetVar( "nogeneral", viewpoint == 3 );
			NGlobal::SetVar( "temp.nogeneral_sript", viewpoint == 3 );
			NGlobal::SetVar( "temp.general_reinforcement", viewpoint == 0 );
			CGeneral *first = MakeGeneral( 3 );
			CGeneral *second = MakeGeneral( 5 );
			TGenerals generals;
			generals[reverse ? 5 : 3] = reverse ? second : first;
			generals[reverse ? 3 : 5] = reverse ? first : second;
			CSupremeBeing supreme;
			InstallGenerals( &supreme, &generals );
			NRandom::SetRandomSeed( seed );
			std::vector<NTimer::STime> decisions;
			for ( const NTimer::STime time : { 100u, 5000u, 10000u } )
			{
				SetTime( time );
				supreme.Segment();
				decisions.push_back( State( first ).nextUpdate );
				decisions.push_back( State( second ).nextUpdate );
				bool airTurn = false;
				SFields air{ [&]( IBinSaver &saver ) { saver.Add( 9, &airTurn ); } };
				Copy( State( first ).air.GetPtr(), &air );
				// With no ground reinforcements the real rear manager passes its
				// turn to aviation, even when a peer disabled the local debug flag.
				EXPECT_TRUE( airTurn );
			}
			NRandom::SDebugState random;
			NRandom::GetDebugState( &random );
			EXPECT_GE( random.randomCalls, 6 );
			if ( expected.empty() )
			{
				expected = decisions;
				expectedRandom = random;
			}
			else
			{
				EXPECT_EQ( decisions, expected );
				ExpectSameRandom( random, expectedRandom );
			}
			supreme.Clear();
		}
	}
}

TEST_F( CooperativeGeneral, RememberedEnemyScanContinuesAtSavedCursorAndSeed )
{
	TEnemies enemies;
	for ( int id = 100; id != 125; ++id )
	{
		// Expired references are legal after their units disappear. The first ten
		// remain visible, so restarting the scan on load would miss the next ten.
		enemies[id] = { nullptr, id < 110 ? NTimer::STime( -1 ) : 0 };
	}
	CGeneral *original = MakeGeneral( 3, enemies );
	SetTime( 20000 );
	original->Segment();
	EXPECT_EQ( State( original ).nextEnemy, 110 );
	CMemoryStream saved;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &saved, SAVER_MODE_WRITE, external );
		saver->AddPolymorphicBase( 1, original );
	}
	CObj<IRandomSeed> continuationSeed = NRandom::CreateRandomSeedCopy();
	NRandom::SetRandomSeed( continuationSeed );
	SetTime( 25000 );
	original->Segment();
	const SGeneralState expected = State( original );
	ASSERT_EQ( expected.enemies.size(), 15 );
	EXPECT_EQ( expected.nextEnemy, 120 );
	NRandom::SDebugState expectedRandom;
	NRandom::GetDebugState( &expectedRandom );
	// Reload into the already advanced object as well as a fresh one. Both paths
	// must replace stale container iterators and restore the pending scan.
	for ( CGeneral *restored : { original, MakeGeneral( 3 ) } )
	{
		saved.Seek( 0 );
		{
			CPtr<IBinSaver> loader = CreateBinSaver( &saved, SAVER_MODE_READ, external );
			loader->AddPolymorphicBase( 1, restored );
		}
		NRandom::SetRandomSeed( continuationSeed );
		restored->Segment();
		const SGeneralState actual = State( restored );
		EXPECT_EQ( actual.nextEnemy, expected.nextEnemy );
		EXPECT_EQ( actual.nextUpdate, expected.nextUpdate );
		EXPECT_EQ( actual.enemies.size(), expected.enemies.size() );
		for ( const auto &entry : expected.enemies )
		{
			EXPECT_NE( actual.enemies.find( entry.first ), actual.enemies.end() );
		}
		NRandom::SDebugState random;
		NRandom::GetDebugState( &random );
		ExpectSameRandom( random, expectedRandom );
	}
}

TEST_F( CooperativeGeneral, AirTurnDoesNotSkipTheBotSelectedAsReplayViewpoint )
{
	for ( const int viewpoint : { 0, 2, 3, 5 } )
	{
		for ( const int player : { 3, 5 } )
		{
			SCOPED_TRACE( testing::Message() << viewpoint << ", bot=" << player );
			SetMode( IAIScenarioTracker::EGT_MULTI_COOP, viewpoint );
			auto *air = MakeObject<CGeneralAirForce>( 0x1508D4AE );
			int party = 1;
			int owner = player;
			std::vector<int> players{ player };
			bool turn = true;
			SFields fields{ [&]( IBinSaver &saver )
			                {
												saver.Add( 2, &party );
												saver.Add( 3, &players );
												saver.Add( 9, &turn );
												saver.Add( 11, &owner );
											} };
			Copy( &fields, air );
			const auto calls = NRandom::GetRandomCallsCounter();
			air->Segment();
			Copy( air, &fields );
			// No aircraft are available, so every viewpoint returns the air turn.
			EXPECT_FALSE( turn );
			EXPECT_EQ( NRandom::GetRandomCallsCounter(), calls );
		}
	}
}

TEST_F( CooperativeGeneral, ActiveWeatherRetainsAirSupportWithoutQueuingCalls )
{
	AddAirReinforcement( 3, NDb::RT_GROUND_ATTACK_PLANES );
	AddAirReinforcement( 3, NDb::RT_BOMBERS );
	for ( const int viewpoint : { 0, 2, 3, 5 } )
	{
		for ( const auto weather : { NDb::EWS_BAD, NDb::EWS_FADE_IN, NDb::EWS_FADE_OUT } )
		{
			for ( const auto force : { FT_AIR_GUNPLANE, FT_AIR_BOMBER } )
			{
				SCOPED_TRACE( testing::Message() << viewpoint << ", weather=" << weather << ", force=" << force );
				InitAirReinforcements( viewpoint );
				SetWeather( weather );
				ASSERT_TRUE( theReinfArray[3].CanCallNow() );
				auto *air = MakeAirController( 3 );
				const int request = air->RequestForSupport( CVec2( 120, 240 ), force );
				const auto randomCalls = NRandom::GetRandomCallsCounter();
				air->Segment();
				const auto state = AirState( air );
				EXPECT_FALSE( state.turn );
				EXPECT_EQ( state.current, FT_NONE );
				EXPECT_EQ( state.waitUntil, 0 );
				const auto requests = state.requests.find( force );
				ASSERT_NE( requests, state.requests.end() );
				ASSERT_EQ( requests->second.size(), 1 );
				EXPECT_EQ( requests->second.begin()->first, request );
				EXPECT_EQ( requests->second.begin()->second.vPoint, CVec2( 120, 240 ) );
				EXPECT_TRUE( QueuedAirCalls( &theReinfArray[3] ).empty() );
				EXPECT_EQ( tracker->GetReinforcementCallsLeft( 3 ), 10 );
				EXPECT_EQ( NRandom::GetRandomCallsCounter(), randomCalls );
			}
		}
	}
}

TEST_F( CooperativeGeneral, WeatherChangeBeforeQueuedCreationPreservesCallsAndRetriesAfterSave )
{
	AddAirReinforcement( 3, NDb::RT_GROUND_ATTACK_PLANES );
	InitAirReinforcements();
	SetWeather( NDb::EWS_CLEAR );
	auto *air = MakeAirController( 3 );
	const int request = air->RequestForSupport( CVec2( 120, 240 ), FT_AIR_GUNPLANE );
	air->Segment();
	auto commands = QueuedAirCalls( &theReinfArray[3] );
	ASSERT_EQ( commands.size(), 1 );
	EXPECT_EQ( commands[0].type, NDb::RT_GROUND_ATTACK_PLANES );
	EXPECT_EQ( commands[0].point, CVec2( 120, 240 ) );
	EXPECT_TRUE( commands[0].general );

	// Save/load preserves both the General's support request and the queue's origin.
	auto *restored = MakeObject<CGeneralAirForce>( 0x1508D4AE );
	Copy( air, restored );
	CPlayerReinforcement savedReinforcement;
	Copy( &theReinfArray[3], &savedReinforcement );
	Copy( &savedReinforcement, &theReinfArray[3] );
	ASSERT_TRUE( QueuedAirCalls( &theReinfArray[3] )[0].general );
	SetWeather( NDb::EWS_FADE_IN );
	const auto randomCalls = NRandom::GetRandomCallsCounter();
	theReinfArray[3].Segment();
	EXPECT_TRUE( QueuedAirCalls( &theReinfArray[3] ).empty() );
	EXPECT_EQ( tracker->GetReinforcementCallsLeft( 3 ), 10 );
	EXPECT_TRUE( theReinfArray[3].CanCallNow() );
	EXPECT_EQ( NRandom::GetRandomCallsCounter(), randomCalls );
	EXPECT_EQ( AirState( restored ).requests[FT_AIR_GUNPLANE].begin()->first, request );

	SetTime( 1001 );
	SetWeather( NDb::EWS_CLEAR );
	restored->Segment();
	commands = QueuedAirCalls( &theReinfArray[3] );
	ASSERT_EQ( commands.size(), 1 );
	EXPECT_EQ( commands[0].point, CVec2( 120, 240 ) );
	EXPECT_TRUE( commands[0].general );
	EXPECT_EQ( tracker->GetReinforcementCallsLeft( 3 ), 10 );

	// A reused queue loaded from the old format must not mark explicit calls as General calls.
	struct SLegacyCall
	{
		NDb::EReinforcementType type = NDb::RT_GROUND_ATTACK_PLANES;
		CVec2 point = CVec2( 10, 20 );

		int operator&( IBinSaver &saver )
		{
			saver.Add( 2, &type );
			saver.Add( 3, &point );
			return 0;
		}
	};

	std::vector<SLegacyCall> legacyCommands( 1 );
	SFields legacy{ [&]( IBinSaver &saver ) { saver.Add( 11, &legacyCommands ); } };
	Copy( &legacy, &savedReinforcement );
	commands = QueuedAirCalls( &savedReinforcement );
	ASSERT_EQ( commands.size(), 1 );
	EXPECT_FALSE( commands[0].general );
	EXPECT_EQ( commands[0].point, CVec2( 10, 20 ) );
}

TEST_F( CooperativeGeneral, MasterPilotUsesAircraftOwnersExperienceInsteadOfViewpointOrParty )
{
	NGlobal::SetVar( "multiplayer_unit_experience", 1 );
	AddAirReinforcement( 3, NDb::RT_GROUND_ATTACK_PLANES, 1 );
	AddAirReinforcement( 5, NDb::RT_GROUND_ATTACK_PLANES, 1 );
	auto *levels = MakeRecord<NDb::SAIExpLevel>( "AIExpLevel" );
	levels->eDBType = NDb::RT_GROUND_ATTACK_PLANES;
	levels->levels.resize( 5 );
	for ( int i = 0; i < levels->levels.size(); ++i )
	{
		levels->levels[i].fExperience = i * 100.0f;
	}
	mpConsts->expLevels.push_back( levels );
	for ( const int viewpoint : { 0, 2, 3, 5 } )
	{
		SCOPED_TRACE( viewpoint );
		InitAirReinforcements( viewpoint );
		SetWeather( NDb::EWS_BAD );
		for ( const int experiencedPlayer : { 0, 5 } )
		{
			IAIScenarioTracker::SKillInfo kill;
			kill.nPlayer = experiencedPlayer;
			kill.nKilledUnitPlayer = 6;
			kill.eReinfType = NDb::RT_GROUND_ATTACK_PLANES;
			kill.fExpPrice = 100;
			EXPECT_TRUE( tracker->RegisterUnitKill( kill ) );
		}
		ASSERT_EQ( tracker->GetReinforcementXPLevel( 3, NDb::RT_GROUND_ATTACK_PLANES ), 0 );
		ASSERT_EQ( tracker->GetReinforcementXPLevel( 5, NDb::RT_GROUND_ATTACK_PLANES ), 1 );
		for ( const int owner : { 3, 5 } )
		{
			auto *air = MakeAirController( owner );
			air->RequestForSupport( CVec2( 120, 240 ), FT_AIR_GUNPLANE );
			const auto randomCalls = NRandom::GetRandomCallsCounter();
			air->Segment();
			EXPECT_EQ( QueuedAirCalls( &theReinfArray[owner] ).size(), owner == 5 ? 1 : 0 );
			EXPECT_EQ( tracker->GetReinforcementCallsLeft( owner ), 10 );
			EXPECT_EQ( NRandom::GetRandomCallsCounter(), randomCalls );
		}
	}
}

TEST_F( CooperativeGeneral, MixedAircraftBatchRequiresEveryFixedWingPilotToBeWeatherCapable )
{
	auto *reinforcement = AddAirReinforcement( 3, NDb::RT_GROUND_ATTACK_PLANES, 0 );
	auto *ordinary = AddAirReinforcement( 5, NDb::RT_GROUND_ATTACK_PLANES );
	reinforcement->entries.push_back( ordinary->entries[0] );
	InitAirReinforcements();
	SetWeather( NDb::EWS_BAD );
	auto *air = MakeAirController( 3 );
	air->RequestForSupport( CVec2( 120, 240 ), FT_AIR_GUNPLANE );
	air->Segment();
	EXPECT_TRUE( QueuedAirCalls( &theReinfArray[3] ).empty() );

	// The second entry is now a helicopter, whose own movement states do not force weather departure.
	const NDb::SMechUnitRPGStats *ordinaryStats = ordinary->entries[0].pMechUnit;
	auto *helicopter = const_cast<NDb::SMechUnitRPGStats *>( ordinaryStats );
	helicopter->etype = NDb::RPG_TYPE_AVIA_HELICOPTER;
	auto state = AirState( air );
	state.turn = true;
	Copy( &state, air );
	air->Segment();
	EXPECT_EQ( QueuedAirCalls( &theReinfArray[3] ).size(), 1 );
}

TEST_F( CooperativeGeneral, PendingAirRequestsSurviveSaveAndOlderSavesClearThem )
{
	auto *air = MakeObject<CGeneralAirForce>( 0x1508D4AE );
	const int bomber = air->RequestForSupport( CVec2( 120, 240 ), FT_AIR_BOMBER );
	const int fighter = air->RequestForSupport( CVec2( 450, 680 ), FT_AIR_FIGHTER );
	auto *restored = MakeObject<CGeneralAirForce>( 0x1508D4AE );
	Copy( air, restored );
	CGeneralAirForce::RequestsByForceType requests;
	SFields saved{ [&]( IBinSaver &saver ) { saver.Add( 10, &requests ); } };
	Copy( restored, &saved );
	ASSERT_EQ( requests[FT_AIR_BOMBER].size(), 1 );
	ASSERT_EQ( requests[FT_AIR_FIGHTER].size(), 1 );
	EXPECT_EQ( requests[FT_AIR_BOMBER].begin()->first, bomber );
	EXPECT_EQ( requests[FT_AIR_BOMBER].begin()->second.vPoint, CVec2( 120, 240 ) );
	EXPECT_EQ( requests[FT_AIR_FIGHTER].begin()->first, fighter );
	EXPECT_EQ( requests[FT_AIR_FIGHTER].begin()->second.vPoint, CVec2( 450, 680 ) );
	// An old save has no requests chunk. Reusing an object must not retain orders
	// from the previously loaded game, and its owner falls back to the old side.
	int party = 1;
	SFields legacy{ [&]( IBinSaver &saver ) { saver.Add( 2, &party ); } };
	Copy( &legacy, restored );
	Copy( restored, &saved );
	EXPECT_TRUE( requests.empty() );
	CGeneral *general = MakeGeneral( 5 );
	SFields oldGeneral{ [&]( IBinSaver &saver ) { saver.Add( 3, &party ); } };
	Copy( &oldGeneral, general );
	EXPECT_EQ( general->GetPlayer(), party );
}

TEST_F( CooperativeGeneral, BotsTrackSharedEnemyReportsIndependentlyAcrossMovementSaveAndDeath )
{
	CAIUnitInfoForGeneral *info = MakeEnemyInfo( 120 );
	CResistancesContainer first, second;
	const CVec2 start( 100, 100 );
	const CVec2 moved( 2000, 2000 );
	const auto calls = NRandom::GetRandomCallsCounter();
	const auto report = [&]( CResistancesContainer *container, const CVec2 &position )
	{ container->UpdateEnemyUnitInfo( info, 0, position, 0, VNULL2, -1.0f ); };
	report( &first, start );
	report( &second, start );
	SResistanceState initial;
	Copy( &first, &initial );
	ASSERT_EQ( initial.cells.size(), 1 );
	ASSERT_EQ( initial.units.size(), 1 );
	EXPECT_GT( initial.cells[0].GetWeight(), 0 );
	EXPECT_EQ( ResistanceChecksum( &first ), ResistanceChecksum( &second ) );
	const auto initialChecksum = ResistanceChecksum( &first );
	// The same unit-info object reports to both generals. Moving the first bot's
	// contribution must not overwrite the old cell needed by the second bot.
	report( &first, moved );
	report( &second, moved );
	EXPECT_EQ( ResistanceChecksum( &first ), ResistanceChecksum( &second ) );
	EXPECT_NE( ResistanceChecksum( &first ), initialChecksum );
	SResistanceState movedState;
	Copy( &second, &movedState );
	ASSERT_EQ( movedState.cells.size(), 2 );
	for ( const auto &cell : movedState.cells )
	{
		EXPECT_FLOAT_EQ( cell.GetWeight(),
		                 cell.GetCellNumber() == initial.cells[0].GetCellNumber() ? 0.0f : initial.cells[0].GetWeight() );
	}
	EXPECT_FLOAT_EQ( info->GetWeight(), 0 );
	EXPECT_EQ( info->GetRegisteredPos(), VNULL2 );
	CResistancesContainer restored;
	Copy( &second, &restored );
	EXPECT_EQ( ResistanceChecksum( &second ), ResistanceChecksum( &restored ) );
	for ( auto *container : { &first, &second, &restored } )
	{
		container->UnitDied( info );
		SResistanceState dead;
		Copy( container, &dead );
		EXPECT_TRUE( dead.units.empty() );
		for ( const auto &cell : dead.cells )
		{
			EXPECT_FLOAT_EQ( cell.GetWeight(), 0 );
		}
		const auto once = ResistanceChecksum( container );
		container->UnitDied( info );
		EXPECT_EQ( ResistanceChecksum( container ), once );
	}
	EXPECT_EQ( ResistanceChecksum( &first ), ResistanceChecksum( &restored ) );
	EXPECT_EQ( NRandom::GetRandomCallsCounter(), calls );
	// Equal visible cell weights can still represent different future removals.
	// The contribution's unit identity must therefore participate in checksums.
	CResistancesContainer sameCellFirst, sameCellSecond;
	report( &sameCellFirst, start );
	CAIUnitInfoForGeneral *other = MakeEnemyInfo( 121 );
	sameCellSecond.UpdateEnemyUnitInfo( other, 0, start, 0, VNULL2, -1.0f );
	SResistanceState otherState;
	Copy( &sameCellSecond, &otherState );
	ASSERT_EQ( otherState.cells.size(), 1 );
	EXPECT_FLOAT_EQ( otherState.cells[0].GetWeight(), initial.cells[0].GetWeight() );
	EXPECT_NE( ResistanceChecksum( &sameCellFirst ), ResistanceChecksum( &sameCellSecond ) );
}

TEST_F( CooperativeGeneral, GlobalChecksumIncludesCooperativeGeneralDecisionsOnly )
{
	CGeneral *general = MakeGeneral( 3 );
	TGenerals generals;
	generals[3] = general;
	CSupremeBeing supreme;
	InstallGenerals( &supreme, &generals );
	SFields globals{ [&]( IBinSaver &saver ) { saver.Add( 18, &supreme ); } };
	SFields ai{ [&]( IBinSaver &saver ) { saver.Add( 1, &globals ); } };
	Copy( &ai, logic.GetPtr() );
	for ( const auto mode : { IAIScenarioTracker::EGT_MULTI_COOP,
	                          IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL,
	                          IAIScenarioTracker::EGT_SINGLE } )
	{
		SetMode( mode );
		const uint32_t before = SimulationChecksum();
		NTimer::STime deadline = State( general ).nextUpdate + 12345;
		SFields changed{ [&]( IBinSaver &saver ) { saver.Add( 11, &deadline ); } };
		Copy( &changed, general );
		const uint32_t after = SimulationChecksum();
		if ( mode == IAIScenarioTracker::EGT_MULTI_COOP )
		{
			EXPECT_NE( before, after );
		}
		else
		{
			EXPECT_EQ( before, after );
		}
	}
	// The AI owns the global copy until teardown; release the local owner first.
	supreme.Clear();
}
