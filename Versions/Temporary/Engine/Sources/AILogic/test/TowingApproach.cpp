#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../AIMap.h"
#include "../Artillery.h"
#include "../Formation.h"
#include "../PresizePath.h"
#include "../Technics.h"
#include "../UnitStates.h"
#include "Common_RTS_AI/AIMap.h"
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

struct SUninstallBoundary
{
};

class CArtilleryUninstallBoundary : public IUnitState
{
	OBJECT_BASIC_METHODS( CArtilleryUninstallBoundary );

public:
	bool interrupted = false;

	void Segment() override {}

	ETryStateInterruptResult TryInterruptState( CAICommand * ) override
	{
		// Stop at the real uninstall transition before unrelated world command scheduling.
		interrupted = true;
		throw SUninstallBoundary();
	}

	bool IsAttackingState() const override
	{
		return false;
	}

	const CVec2 GetPurposePoint() const override
	{
		return VNULL2;
	}
};

class TowingApproach : public testing::TestWithParam<float>
{
	static std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	std::vector<CObj<CObjectBase>> objects;
	std::vector<SBinSaverExternalObject> external;
	CAIMap *previousMap = nullptr;
	CObj<CAIMap> map;
	CPtr<CFormation> crew;

protected:
	CPtr<CArtillery> artillery;
	static constexpr int approachByMoveBack = 4;
	static constexpr int startUninstall = 2;
	static constexpr int startPrecisionApproach = 5;
	static constexpr int precisionApproach = 6;
	static constexpr float hookDistance = 111.0f;
	const CVec2 artilleryPosition = CVec2( 1000, 1000 );
	CObj<IUnitState> state;
	CPtr<CAITransportUnit> truck;

	static void SetUpTestSuite()
	{
		// Keep the DLL's real factory registrations linked on Windows as well as ELF.
		ASSERT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-towing-approach-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( TOWING_APPROACH_TYPES_FILE, directory / "types.xml" );
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

	void SetUnitFields( CAIUnit *unit, NDb::EUnitRPGType type, float towPoint )
	{
		CObj<NDb::IObjMan> record = NDb::CreateNewObject( "MechUnitRPGStats" );
		ASSERT_NE( record, nullptr );
		auto *stats = dynamic_cast<NDb::SMechUnitRPGStats *>( record->GetObject() );
		ASSERT_NE( stats, nullptr );
		stats->etype = type;
		stats->vTowPoint = CVec2( 0, towPoint );
		static unsigned nextRecord = 0;
		const std::string name = "unit-" + std::to_string( nextRecord++ ) + ".xdb";
		ASSERT_TRUE( NDb::AddNewObject( name, CDBID( name ), record ) );
		records.push_back( record );
		CDBPtr<NDb::SMechUnitRPGStats> statsPointer = stats;
		CVec3 position( artilleryPosition, 0 );
		SFixtureFields pathUnit{ [&]( IBinSaver &saver ) { saver.Add( 9, &position ); } };
		SFixtureFields commonUnit{ [&]( IBinSaver &saver ) { saver.Add( 20, &pathUnit ); } };
		SFixtureFields aiUnit{ [&]( IBinSaver &saver ) { saver.Add( 1, &commonUnit ); } };
		SFixtureFields mechanical{ [&]( IBinSaver &saver )
		                           {
																 saver.Add( 1, &aiUnit );
																 saver.Add( 2, &statsPointer );
															 } };
		LoadFields( unit,
		            [&]( IBinSaver &saver )
		            {
									if ( unit == artillery )
									{
										mechanical.visit( saver );
										saver.Add( 16, &crew );
									}
									else
									{
										saver.Add( 1, &mechanical );
									}
								} );
		ASSERT_EQ( unit->GetStats(), stats );
	}

	void SetUp() override
	{
		Initialize( GetParam() );
	}

	void Initialize( float artilleryTowPoint )
	{
		previousMap = GetAIMap();
		map = new CAIMap( 32, 32, 64, 2, 32 );
		SetAIMap( map );
		state = MakeObject<IUnitState>( 0x1108D472 );
		truck = MakeObject<CAITransportUnit>( AI_TRANSPORT_UNIT );
		artillery = MakeObject<CArtillery>( AI_ARTILLERY );
		crew = MakeObject<CFormation>( 0x1108D498 );
		ASSERT_NE( state, nullptr );
		ASSERT_NE( truck, nullptr );
		ASSERT_NE( artillery, nullptr );
		ASSERT_NE( crew, nullptr );
		SetUnitFields( truck, NDb::RPG_TYPE_TRN_SUPPORT, -80.0f );
		SetUnitFields( artillery, NDb::RPG_TYPE_ART_GUN, artilleryTowPoint );
		ASSERT_TRUE( artillery->HasServeCrew() );
		truck->SetCurState( state );
		// Initialize the real default path and free collision so Stop exercises its normal implementation.
		truck->CBasePathUnit::Init( CVec3( artilleryPosition, 0 ), 0, map, nullptr, nullptr );
		int phase = approachByMoveBack;
		CVec2 destination = artilleryPosition;
		LoadFields( state,
		            [&]( IBinSaver &saver )
		            {
									saver.Add( 2, &truck );
									saver.Add( 3, &phase );
									saver.Add( 4, &artillery );
									saver.Add( 5, &destination );
								} );
	}

	void TearDown() override
	{
		SetAIMap( previousMap );
	}

	void SetMovementSnapshot( float distance, bool idle = false, uint16_t frontDirection = 0 )
	{
		// Restore consecutive movement samples without changing the live attachment state.
		// Segment computes the actual hook geometry and decides whether to stop the real path.
		CVec3 position( artilleryPosition + artillery->GetFrontDirectionVector() * distance, 0 );
		SAIAngle facing( frontDirection );
		SAIAngle travel( frontDirection + 32768 );
		float speed = idle ? 0.0f : 0.4f;
		bool forward = false;
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->Add( 3, &travel );
			saver->Add( 4, &facing );
			saver->Add( 6, &forward );
			saver->Add( 9, &position );
			saver->Add( 12, &speed );
			saver->Add( 16, &idle );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		truck->CBasePathUnit::operator&( *loader );
		ASSERT_EQ( truck->IsIdle(), idle );
		ASSERT_FALSE( truck->IsGoForward() );
		ASSERT_EQ( truck->GetFrontDirection(), frontDirection );
		ASSERT_EQ( truck->GetDirection(), uint16_t( frontDirection + 32768 ) );
	}

	void SetArtilleryInTankPit()
	{
		bool inTankPit = true;
		SFixtureFields aiUnit{ [&]( IBinSaver &saver ) { saver.Add( 74, &inTankPit ); } };
		LoadFields( artillery, [&]( IBinSaver &saver ) { saver.Add( 1, &aiUnit ); } );
		ASSERT_TRUE( artillery->IsInTankPit() );
	}

	int GetPhase()
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			saver->AddPolymorphicBase( 1, state );
		}
		int phase = -1;
		SFixtureFields fields{ [&]( IBinSaver &saver ) { saver.Add( 3, &phase ); } };
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->Add( 1, &fields );
		return phase;
	}

	void ExpectStoppedForUninstall()
	{
		EXPECT_EQ( GetPhase(), startUninstall );
		EXPECT_TRUE( truck->IsIdle() );
		EXPECT_FLOAT_EQ( truck->GetSpeed(), 0.0f );
	}
};

std::filesystem::path TowingApproach::directory;

class TowingPrecisionApproach : public TowingApproach
{
protected:
	CObj<CArtilleryUninstallBoundary> uninstallBoundary;

	void SetUp() override
	{
		// The trtt pair has similar rear towing offsets, so overlapping centers put their hooks only 26 apart.
		Initialize( -93.0f );
		SetUnitFields( truck, NDb::RPG_TYPE_TRN_SUPPORT, -67.0f );
		uninstallBoundary = new CArtilleryUninstallBoundary;
		artillery->SetCurState( uninstallBoundary );
		int phase = precisionApproach;
		CVec2 destination = artilleryPosition;
		LoadFields( state,
		            [&]( IBinSaver &saver )
		            {
									saver.Add( 2, &truck );
									saver.Add( 3, &phase );
									saver.Add( 4, &artillery );
									saver.Add( 5, &destination );
								} );
	}

	CObj<CPresizePath> SetPrecisionPath( bool finished )
	{
		const CVec2 finalDirection = -artillery->GetFrontDirectionVector();
		CObj<CPresizePath> path =
				new CPresizePath( truck, artilleryPosition + finalDirection * 160.0f, finalDirection, truck->GetSmoothPath() );
		if ( finished )
		{
			// Restore the saved finished phase; full precision movement is covered by TowingPrecision.
			int phase = 5;
			LoadFields( path, [&]( IBinSaver &saver ) { saver.Add( 3, &phase ); } );
		}
		truck->SetSmoothPath( path );
		return path;
	}

	float GetFlatHookDistance() const
	{
		// On this flat map the authored offsets project directly along the units' front directions.
		const auto *truckStats = checked_cast<const NDb::SMechUnitRPGStats *>( truck->GetStats() );
		const auto *artilleryStats = checked_cast<const NDb::SMechUnitRPGStats *>( artillery->GetStats() );
		const CVec2 truckHook = truck->GetCenterPlain() + truck->GetFrontDirectionVector() * truckStats->vTowPoint.y;
		const CVec2 artilleryHook =
				artillery->GetCenterPlain() + artillery->GetFrontDirectionVector() * artilleryStats->vTowPoint.y;
		return fabs( truckHook - artilleryHook );
	}
};

TEST_F( TowingPrecisionApproach, OverlappingHooksDoNotFinishAnActivePrecisionPath )
{
	SetMovementSnapshot( 0.0f );
	CObj<CPresizePath> path = SetPrecisionPath( false );
	ASSERT_FALSE( path->IsFinished() );
	ASSERT_NEAR( GetFlatHookDistance(), 26.0f, 0.1f );
	ASSERT_LT( GetFlatHookDistance(), 32.0f );
	EXPECT_NO_THROW( state->Segment() );
	EXPECT_FALSE( uninstallBoundary->interrupted );
	EXPECT_EQ( GetPhase(), precisionApproach );
	EXPECT_EQ( truck->GetSmoothPath(), path.GetPtr() );
	EXPECT_FALSE( truck->IsIdle() );
	EXPECT_GT( truck->GetSpeed(), 0.0f );
}

TEST_F( TowingPrecisionApproach, FinishedAlignedPrecisionPathStartsUninstalling )
{
	SetMovementSnapshot( -160.0f, false, 32768 );
	CObj<CPresizePath> path = SetPrecisionPath( true );
	ASSERT_TRUE( path->IsFinished() );
	ASSERT_NEAR( GetFlatHookDistance(), 0.0f, 0.1f );
	ASSERT_LT( GetFlatHookDistance(), 32.0f );
	EXPECT_THROW( state->Segment(), SUninstallBoundary );
	EXPECT_TRUE( uninstallBoundary->interrupted );
	EXPECT_NE( truck->GetSmoothPath(), path.GetPtr() );
	ExpectStoppedForUninstall();
}

TEST_P( TowingApproach, ContinuesReversingBeforeReachingTheHookDistance )
{
	SetMovementSnapshot( hookDistance + 16.0f );
	state->Segment();
	EXPECT_EQ( GetPhase(), approachByMoveBack );
	EXPECT_FALSE( truck->IsIdle() );
	EXPECT_GT( truck->GetSpeed(), 0.0f );
}

TEST_P( TowingApproach, StopsAtTheNormalHookDistance )
{
	SetMovementSnapshot( hookDistance );
	state->Segment();
	ExpectStoppedForUninstall();
}

TEST_P( TowingApproach, StopsWhenOneMovementStepSkipsTheNarrowHookDistanceBand )
{
	SetMovementSnapshot( hookDistance + 16.0f );
	state->Segment();
	ASSERT_EQ( GetPhase(), approachByMoveBack );
	// A 32-unit reverse step crosses both sides of the old +/-5 stopping band.
	SetMovementSnapshot( hookDistance - 16.0f );
	state->Segment();
	ExpectStoppedForUninstall();
}

TEST_P( TowingApproach, OverlappingCentersStillStopAndStartUninstalling )
{
	SetMovementSnapshot( 0.0f );
	state->Segment();
	ExpectStoppedForUninstall();
}

TEST_P( TowingApproach, IdleNearbyTruckWaitsForTheGunToPackAndTurn )
{
	// Collision avoidance may stop the coarse approach before its narrow distance band.
	SetMovementSnapshot( hookDistance + 32.0f, true );
	state->Segment();
	ExpectStoppedForUninstall();
}

TEST_P( TowingApproach, EntrenchedGunStillLeavesItsPitBeforePackingAndTurning )
{
	SetArtilleryInTankPit();
	SetMovementSnapshot( hookDistance, true );
	state->Segment();
	EXPECT_EQ( GetPhase(), startPrecisionApproach );
}

// The initial center-distance approach must work for either authored towing end.
INSTANTIATE_TEST_SUITE_P( FrontAndRearTowed, TowingApproach, testing::Values( 31.0f, -31.0f ) );
} // namespace
