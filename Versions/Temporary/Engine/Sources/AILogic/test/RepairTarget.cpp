#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../Artillery.h"
#include "../Formation.h"
#include "../FormationStates.h"
#include "../Technics.h"
#include "../Trains.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "System/BinSaver.h"
#include "System/Streams.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace
{
struct SFixtureFields
{
	std::function<void( IBinSaver & )> visit;

	int operator&( IBinSaver &saver )
	{
		visit( saver );
		return 0;
	}
};

class RepairTarget : public testing::Test
{
	static std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	std::vector<CObj<CObjectBase>> objects;
	std::vector<SBinSaverExternalObject> external;

protected:
	static constexpr int serving = 6;
	static constexpr int approaching = 2;
	CObj<IUnitState> state;
	CPtr<CFormation> formation;
	CPtr<CAITransportUnit> home;
	CPtr<CAIUnit> nextTarget;

	static void SetUpTestSuite()
	{
		// Force AILogic's real factory registrations to load on Windows and ELF.
		ASSERT_EQ( CastToUserObject( static_cast<CObjectBase *>( nullptr ), static_cast<CTank *>( nullptr ) ), nullptr );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-repair-target-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( REPAIR_TARGET_TYPES_FILE, directory / "types.xml" );
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
	}

	static void TearDownTestSuite()
	{
		NDb::CloseDatabase();
		NVFS::SetMainVFS( nullptr );
		NVFS::SetMainFileCreator( nullptr );
		std::error_code error;
		std::filesystem::remove_all( directory, error );
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

	void LoadFields( CObjectBase *object, const std::function<void( IBinSaver & )> &visit )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			SFixtureFields fields{ visit };
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->AddPolymorphicBase( 1, object );
	}

	CAIUnit *MakeUnit( int type, NDb::EUnitRPGType rpgType, float hp = 100, bool trackDamaged = false )
	{
		CAIUnit *unit = MakeObject<CAIUnit>( type );
		if ( !unit )
		{
			return nullptr;
		}
		CObj<NDb::IObjMan> record = NDb::CreateNewObject( "MechUnitRPGStats" );
		EXPECT_NE( record, nullptr );
		if ( !record )
		{
			return nullptr;
		}
		auto *stats = dynamic_cast<NDb::SMechUnitRPGStats *>( record->GetObject() );
		EXPECT_NE( stats, nullptr );
		if ( !stats )
		{
			return nullptr;
		}
		stats->etype = rpgType;
		stats->fMaxHP = 100;
		stats->fRepairCost = 1;
		static unsigned nextRecord = 0;
		const std::string name = "unit-" + std::to_string( nextRecord++ ) + ".xdb";
		EXPECT_TRUE( NDb::AddNewObject( name, CDBID( name ), record ) );
		records.push_back( record );
		CDBPtr<NDb::SMechUnitRPGStats> statsPointer = stats;
		// Keep preferred-target weighting finite without initializing a map or calling SetCenter.
		CVec3 position = type == AI_TRANSPORT_UNIT ? CVec3( 1, 0, 0 ) : VNULL3;
		bool idle = true;
		SFixtureFields pathUnit{ [&]( IBinSaver &saver )
		                         {
															 saver.Add( 9, &position );
															 saver.Add( 16, &idle );
														 } };
		SFixtureFields commonUnit{ [&]( IBinSaver &saver ) { saver.Add( 20, &pathUnit ); } };
		SFixtureFields aiUnit{ [&]( IBinSaver &saver )
		                       {
														 saver.Add( 1, &commonUnit );
														 saver.Add( 15, &hp );
													 } };
		SFixtureFields mechanical{ [&]( IBinSaver &saver )
		                           {
																 saver.Add( 1, &aiUnit );
																 saver.Add( 2, &statsPointer );
															 } };
		// Artillery owns its stats directly; tanks, transports and trains inherit CMilitaryCar.
		LoadFields( unit,
		            [&]( IBinSaver &saver )
		            {
									if ( dynamic_cast<CArtillery *>( unit ) )
									{
										mechanical.visit( saver );
									}
									else
									{
										saver.Add( 1, &mechanical );
										if ( dynamic_cast<CTank *>( unit ) )
										{
											saver.Add( 2, &trackDamaged );
										}
									}
								} );
		EXPECT_EQ( unit->GetStats(), stats );
		EXPECT_FLOAT_EQ( unit->GetHitPoints(), hp );
		// Mechanical IsIdle checks its current state's name before the path's idle flag.
		const int restType = type == AI_ARTILLERY                            ? 0x1108D4D7
		                     : type == AI_LOCOMOTIVE || type == AI_TRAIN_CAR ? 0x191BF340
		                                                                     : 0x1508D494;
		unit->SetCurState( MakeObject<IUnitState>( restType ) );
		return unit;
	}

	void SetUp() override
	{
		state = MakeObject<IUnitState>( 0x1108D474 );
		formation = MakeObject<CFormation>( 0x1108D498 );
		home = dynamic_cast<CAITransportUnit *>( MakeUnit( 0x1108D445, NDb::RPG_TYPE_TRN_SUPPORT ) );
		nextTarget = MakeUnit( 0x1108D4AB, NDb::RPG_TYPE_ART_ROCKET, 50 );
		ASSERT_NE( state, nullptr );
		ASSERT_NE( formation, nullptr );
		ASSERT_NE( home, nullptr );
		ASSERT_NE( nextTarget, nullptr );
		ASSERT_TRUE( nextTarget->IsAlive() );
		ASSERT_TRUE( nextTarget->IsIdle() );
	}

	void PrepareRepair( CAIUnit *target, CTank *cachedTank = nullptr, int phase = serving )
	{
		float resources = 10000;
		CPtr<CAIUnit> current = target;
		CPtr<CTank> tank = cachedTank;
		CVec2 position = target->GetCenterPlain();
		bool nearTruck = true;
		SFixtureFields service{ [&]( IBinSaver &saver )
		                        {
															saver.Add( 2, &phase );
															saver.Add( 3, &home );
															saver.Add( 5, &resources );
															saver.Add( 6, &nextTarget );
														} };
		// Restore the same fields as a saved live repair state, including an optional stale track cache.
		LoadFields( state,
		            [&]( IBinSaver &saver )
		            {
									saver.Add( 1, &service );
									saver.Add( 2, &formation );
									saver.Add( 3, &current );
									saver.Add( 4, &tank );
									saver.Add( 5, &position );
									saver.Add( 8, &nearTruck );
								} );
	}

	void ExpectRepair( int expectedPhase, CAIUnit *expectedTarget, CTank *expectedTank )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			saver->AddPolymorphicBase( 1, state );
		}
		int phase = -1;
		CPtr<CAIUnit> current;
		// Preserve an incorrectly typed saved cache so the test can detect it, rather than casting it away.
		CPtr<CAIUnit> tank;
		SFixtureFields service{ [&]( IBinSaver &saver ) { saver.Add( 2, &phase ); } };
		SFixtureFields fields{ [&]( IBinSaver &saver )
		                       {
														 saver.Add( 1, &service );
														 saver.Add( 3, &current );
														 saver.Add( 4, &tank );
													 } };
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->Add( 1, &fields );
		EXPECT_EQ( phase, expectedPhase );
		EXPECT_EQ( current.GetPtr(), expectedTarget );
		EXPECT_EQ( tank.GetPtr(), expectedTank );
	}
};

std::filesystem::path RepairTarget::directory;

TEST_F( RepairTarget, FullHealthArtilleryAndTrainsAdvanceToTheNextRepairTarget )
{
	const std::pair<int, NDb::EUnitRPGType> types[] = { { 0x1108D4AB, NDb::RPG_TYPE_ART_ROCKET },
	                                                    { 0x19171400, NDb::RPG_TYPE_TRAIN_LOCOMOTIVE },
	                                                    { 0x191992C1, NDb::RPG_TYPE_TRAIN_CARGO } };
	for ( const auto &type : types )
	{
		SCOPED_TRACE( type.first );
		CAIUnit *target = MakeUnit( type.first, type.second );
		ASSERT_NE( target, nullptr );
		ASSERT_EQ( dynamic_cast<CTank *>( target ), nullptr );
		PrepareRepair( target );
		state->Segment();
		ExpectRepair( approaching, nextTarget, nullptr );
	}
}

TEST_F( RepairTarget, FullHealthTankWaitsForDamagedTracksButAdvancesWithIntactTracks )
{
	for ( bool damaged : { true, false } )
	{
		SCOPED_TRACE( damaged );
		auto *tank = dynamic_cast<CTank *>( MakeUnit( 0x1108D444, NDb::RPG_TYPE_ARM_MEDIUM, 100, damaged ) );
		ASSERT_NE( tank, nullptr );
		ASSERT_EQ( tank->IsTrackDamaged(), damaged );
		PrepareRepair( tank, tank );
		state->Segment();
		ExpectRepair( damaged ? serving : approaching, damaged ? tank : nextTarget.GetPtr(), damaged ? tank : nullptr );
	}
}

TEST_F( RepairTarget, LoadingANonTankTargetDiscardsAPreviousTanksTrackCache )
{
	auto *previous = dynamic_cast<CTank *>( MakeUnit( 0x1108D444, NDb::RPG_TYPE_ARM_MEDIUM, 100, true ) );
	ASSERT_NE( previous, nullptr );
	PrepareRepair( nextTarget, previous );
	state->Segment();
	ExpectRepair( serving, nextTarget, nullptr );
	EXPECT_TRUE( previous->IsTrackDamaged() );
}

TEST_F( RepairTarget, SelectingArtilleryOrTrainDoesNotRetainATankCache )
{
	auto *previous = dynamic_cast<CTank *>( MakeUnit( 0x1108D444, NDb::RPG_TYPE_ARM_MEDIUM, 100, true ) );
	ASSERT_NE( previous, nullptr );
	const std::pair<int, NDb::EUnitRPGType> types[] = { { 0x1108D4AB, NDb::RPG_TYPE_ART_ROCKET },
	                                                    { 0x19171400, NDb::RPG_TYPE_TRAIN_LOCOMOTIVE },
	                                                    { 0x191992C1, NDb::RPG_TYPE_TRAIN_CARGO } };
	for ( const auto &type : types )
	{
		SCOPED_TRACE( type.first );
		nextTarget = MakeUnit( type.first, type.second, 50 );
		ASSERT_NE( nextTarget, nullptr );
		PrepareRepair( previous, previous, 1 ); // Find the preferred target using the real selection logic.
		state->Segment();
		ExpectRepair( approaching, nextTarget, nullptr );
		EXPECT_TRUE( previous->IsTrackDamaged() );
	}
}
} // namespace
