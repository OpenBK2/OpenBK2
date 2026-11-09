#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../B2AI.h"
#include "../GlobalWarFog.h"
#include "../NewUpdater.h"
#include "../Technics.h"
#include "../Turret.h"
#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/CommonPathFinder.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "System/GlobalVars.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <string>
#include <vector>

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

class SpecialSights : public testing::Test
{
	static std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	std::vector<SBinSaverExternalObject> external;
	CObj<CAIMap> map;
	CObj<CAIUnit> unit;
	CObj<CTurret> turret;
	CDBPtr<NDb::SMechUnitRPGStats> statsPointer;
	CPtr<NDb::SUnitStatsModifier> modifierPointer;
	int previousDetectionRadius = 8;
	CMemoryStream updaterState;

protected:
	NDb::SMechUnitRPGStats *stats = nullptr;
	NDb::SExtendedStats *extended = nullptr;
	CObj<NDb::SUnitStatsModifier> modifier;
	static constexpr int unitID = 123;
	static constexpr int party = 1;
	const SVector center = SVector( 16, 16 );

	static void SetUpTestSuite()
	{
		ASSERT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-special-sights-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( SPECIAL_SIGHTS_TYPES_FILE, directory / "types.xml" );
		std::ofstream( directory / "legacy-mech.xdb" ) << "<MechUnitRPGStats/>";
		std::ofstream( directory / "legacy-infantry.xdb" ) << "<InfantryRPGStats/>";
		std::ofstream( directory / "detector.xdb" )
				<< "<InfantryRPGStats><SpyDiscoveryRange>24</SpyDiscoveryRange></InfantryRPGStats>";
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		ASSERT_TRUE( NDb::RegisterResourceFile( "legacy-mech.xdb" ) );
		ASSERT_TRUE( NDb::RegisterResourceFile( "legacy-infantry.xdb" ) );
		ASSERT_TRUE( NDb::RegisterResourceFile( "detector.xdb" ) );
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
	T *MakeRecord( const char *type )
	{
		CObj<NDb::IObjMan> record = NDb::CreateNewObject( type );
		if ( !record )
		{
			ADD_FAILURE() << type;
			return nullptr;
		}
		static unsigned nextRecord = 0;
		const std::string name = std::string( type ) + std::to_string( nextRecord++ ) + ".xdb";
		EXPECT_TRUE( NDb::AddNewObject( name, CDBID( name ), record ) );
		records.push_back( record );
		return dynamic_cast<T *>( record->GetObject() );
	}

	void SetUp() override
	{
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &updaterState, SAVER_MODE_WRITE );
			saver->Add( 1, &updater );
		}
		// A headless fog observer has no local presentation grid, including after a party transfer.
		CMemoryStream observerStream;
		int neutralParty = 2;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &observerStream, SAVER_MODE_WRITE );
			saver->Add( 10, &neutralParty );
		}
		observerStream.Seek( 0 );
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &observerStream, SAVER_MODE_READ );
			updater.operator&( *saver );
		}
		previousDetectionRadius = NGlobal::GetVar( "AI.WarFog.CamouflageOpenRadius", 8 );
		NGlobal::SetVar( "AI.WarFog.CamouflageOpenRadius", 8 );
		NSingleton::RegisterSingleton( new CCommonPathFinder(), CCommonPathFinder::tidTypeID );
		map = new CAIMap( 64, 64, 32, 2, 16 );
		SetAIMap( map );
		theWarFog.Init( 64, 64, 32, 2.0f );
		unit = dynamic_cast<CAIUnit *>( NObjectFactory::MakeObject( AI_TANK ) );
		turret = dynamic_cast<CTurret *>( NObjectFactory::MakeObject( 0x1108D4B3 ) );
		ASSERT_NE( unit, nullptr );
		ASSERT_NE( turret, nullptr );
		modifier = new NDb::SUnitStatsModifier;
		modifierPointer = modifier;
		external.emplace_back( 1, modifier );
		external.emplace_back( 2, turret );
		stats = MakeRecord<NDb::SMechUnitRPGStats>( "MechUnitRPGStats" );
		extended = MakeRecord<NDb::SExtendedStats>( "ExtendedStats" );
		ASSERT_NE( stats, nullptr );
		ASSERT_NE( extended, nullptr );
		stats->etype = NDb::RPG_TYPE_ARM_MEDIUM;
		stats->fSight = 4.0f;
		stats->platforms.resize( 2 );
		stats->platforms[0].szRotatePoint = "hull";
		stats->platforms[1].szRotatePoint = "turret";
		stats->pExtendedStats = extended;
		statsPointer = stats;
		PlaceUnit();
	}

	void TearDown() override
	{
		theWarFog.Clear();
		NGlobal::SetVar( "AI.WarFog.CamouflageOpenRadius", previousDetectionRadius );
		CLinkObject::Clear();
		unit = nullptr;
		turret = nullptr;
		updaterState.Seek( 0 );
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &updaterState, SAVER_MODE_READ );
			saver->Add( 1, &updater );
		}
		SetAIMap( nullptr );
		NSingleton::UnRegisterSingleton( CCommonPathFinder::tidTypeID );
	}

	void PlaceUnit( uint16_t facing = 0, NDb::SInfantryRPGStats *infantryStats = nullptr )
	{
		// Populate real units through their save format without starting unrelated AI state machines.
		SAIAngle direction( facing );
		SVector tile( center.x * AI_TILES_IN_VIS_TILE, center.y * AI_TILES_IN_VIS_TILE );
		CVec3 position( tile.x * SConsts::TILE_SIZE, tile.y * SConsts::TILE_SIZE, 0 );
		std::vector<CObj<CTurret>> turrets{ turret };
		SFields path{ [&]( IBinSaver &saver )
		              {
										saver.Add( 3, &direction );
										saver.Add( 4, &direction );
										saver.Add( 9, &position );
										saver.Add( 10, &tile );
									} };
		SFields common{ [&]( IBinSaver &saver ) { saver.Add( 20, &path ); } };
		SFields ai{ [&]( IBinSaver &saver )
		            {
									saver.Add( 1, &common );
									saver.Add( 58, &modifierPointer );
								} };
		SFields mechanical{ [&]( IBinSaver &saver )
		                    {
													saver.Add( 1, &ai );
													saver.Add( 2, &statsPointer );
													saver.Add( 4, &turrets );
												} };
		CDBPtr<NDb::SInfantryRPGStats> infantryPointer = infantryStats;
		float ownSight = -1.0f;
		uint16_t maxAngle = 65535;
		SFields soldier{ [&]( IBinSaver &saver )
		                 {
											 saver.Add( 1, &ai );
											 saver.Add( 4, &infantryPointer );
											 saver.Add( 9, &maxAngle );
											 saver.Add( 10, &ownSight );
										 } };
		SFields fields{ [&]( IBinSaver &saver ) { saver.Add( 1, infantryStats ? &soldier : &mechanical ); } };
		if ( infantryStats )
		{
			unit = dynamic_cast<CAIUnit *>( NObjectFactory::MakeObject( AI_INFANTRY ) );
			ASSERT_NE( unit, nullptr );
		}
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->AddPolymorphicBase( 1, unit );
		ASSERT_EQ( unit->GetStats(), infantryStats ? static_cast<NDb::SUnitBaseRPGStats *>( infantryStats ) : stats );
	}

	void AddSight( float start = -0.3f, float end = 0.3f, float range = 24.0f, const char *platform = "" )
	{
		NDb::SSpecialSight sight;
		sight.fAngleStartRad = start;
		sight.fAngleEndRad = end;
		sight.fSightRange = range;
		sight.szPlatformName = platform;
		extended->specialSightsArray.push_back( sight );
	}

	SWarFogUnitInfo Info() const
	{
		SWarFogUnitInfo info;
		unit->GetFogInfo( &info );
		return info;
	}

	void RegisterSelectedUnit()
	{
		unit->nUniqueID = unitID;
		unit->Mem2UniqueIdObjs();
		// The fixture's partial save omits alive flags; restore them for the public overlay query.
		CMemoryStream stream;
		bool alive = true;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->Add( 2, &alive );
			saver->Add( 3, &alive );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		unit->CUpdatableObj::operator&( *loader );
	}

	void AddUnit()
	{
		theWarFog.AddUnit( unitID, Info(), party );
		theWarFog.Segment();
	}

	void Refresh()
	{
		theWarFog.UpdateUnit( unitID, Info() );
		theWarFog.Segment();
	}

	SVector Point( uint16_t direction, int radius ) const
	{
		const CVec2 offset = GetVectorByDirection( direction ) * float( radius );
		return SVector( ( center.x + int( std::lround( offset.x ) ) ) * AI_TILES_IN_VIS_TILE,
		                ( center.y + int( std::lround( offset.y ) ) ) * AI_TILES_IN_VIS_TILE );
	}

	bool Visible( uint16_t direction, int radius, int owner = party ) const
	{
		return theWarFog.IsTileVisible( Point( direction, radius ), owner );
	}

	void TurnTurret( uint16_t direction )
	{
		// Use the simulation rotation setter without scheduling graphical turret notifications.
		turret->CTurret::TurnHor( direction, true );
	}
};

std::filesystem::path SpecialSights::directory;

TEST_F( SpecialSights, EmptyDefaultsPreserveOriginalCircle )
{
	const NDb::SSpecialSight sight;
	EXPECT_FLOAT_EQ( sight.fAngleStartRad, 0.0f );
	EXPECT_FLOAT_EQ( sight.fAngleEndRad, 0.0f );
	EXPECT_FLOAT_EQ( sight.fSightRange, 0.0f );
	EXPECT_TRUE( sight.szPlatformName.empty() );
	EXPECT_TRUE( extended->specialSightsArray.empty() );
	EXPECT_TRUE( Info().specialSights.empty() );
	AddUnit();
	for ( const uint16_t direction : { 0, 16384, 32768, 49152 } )
	{
		EXPECT_TRUE( Visible( direction, 2 ) );
		EXPECT_FALSE( Visible( direction, 4 ) );
	}
	stats->pExtendedStats = nullptr;
	EXPECT_TRUE( Info().specialSights.empty() );
}

TEST_F( SpecialSights, HullSectorWrapsAndAddsToCircle )
{
	AddSight();
	AddUnit();
	EXPECT_TRUE( Visible( 0, 10 ) );
	EXPECT_FALSE( Visible( 16384, 10 ) );
	EXPECT_TRUE( Visible( 32768, 2 ) );
	PlaceUnit( 16384 );
	Refresh();
	EXPECT_FALSE( Visible( 0, 10 ) );
	EXPECT_TRUE( Visible( 16384, 10 ) );
	// The equivalent positive-angle interval also crosses zero.
	extended->specialSightsArray[0].fAngleStartRad = float( 2.0 * PI ) - 0.3f;
	Refresh();
	EXPECT_TRUE( Visible( 16384, 10 ) );
}

TEST_F( SpecialSights, NamedPlatformTracksTurretAndHullAndRejectsUnknownName )
{
	AddSight( -0.3f, 0.3f, 24.0f, "turret" );
	TurnTurret( 16384 );
	AddUnit();
	EXPECT_FALSE( Visible( 0, 10 ) );
	EXPECT_TRUE( Visible( 16384, 10 ) );
	TurnTurret( 32768 );
	Refresh();
	EXPECT_FALSE( Visible( 16384, 10 ) );
	EXPECT_TRUE( Visible( 32768, 10 ) );
	PlaceUnit( 16384 );
	Refresh();
	EXPECT_TRUE( Visible( 49152, 10 ) );
	extended->specialSightsArray[0].szPlatformName = "missing";
	Refresh();
	EXPECT_TRUE( Info().specialSights.empty() );
	EXPECT_FALSE( Visible( 49152, 10 ) );
	extended->specialSightsArray[0].szPlatformName = "hull";
	Refresh();
	EXPECT_TRUE( Visible( 16384, 10 ) );
}

TEST_F( SpecialSights, ModifiersApplyBeforeMaximumAndInvalidEntriesAreIgnored )
{
	AddSight( -0.3f, 0.3f, 10.0f );
	modifier->sightRange.fAddBonus = 2.0f;
	modifier->sightRange.fMultBonus = 2.0f;
	ASSERT_EQ( Info().specialSights.size(), 1u );
	EXPECT_EQ( Info().specialSights[0].nRadius, 12 );
	extended->specialSightsArray[0].fSightRange = ( std::numeric_limits<float>::max )();
	modifier->sightRange.fMultBonus = 1.0f;
	EXPECT_EQ( Info().specialSights[0].nRadius, theWarFog.GetMaxRadius() );
	modifier->sightRange.nZeroCount = 1;
	EXPECT_TRUE( Info().specialSights.empty() );
	modifier->sightRange.nZeroCount = 0;
	extended->specialSightsArray.clear();
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float infinity = std::numeric_limits<float>::infinity();
	AddSight( 0, 0, 24 );
	AddSight( nan, 1, 24 );
	AddSight( 0, infinity, 24 );
	AddSight( 0, 1, nan );
	AddSight( 0, 1, infinity );
	AddSight( 0, 1, -1 );
	AddSight( 0, 1, 0 );
	EXPECT_TRUE( Info().specialSights.empty() );
}

TEST_F( SpecialSights, FullTurnAndOverlappingSectorsLeaveNoVisibilityAfterRemoval )
{
	AddSight( 0, float( 2.0 * PI ), 24 );
	AddSight();
	AddSight();
	AddUnit();
	for ( const uint16_t direction : { 0, 16384, 32768, 49152 } )
	{
		EXPECT_TRUE( Visible( direction, 10 ) );
	}
	theWarFog.DeleteUnit( unitID );
	theWarFog.Segment();
	for ( const uint16_t direction : { 0, 16384, 32768, 49152 } )
	{
		EXPECT_FALSE( Visible( direction, 10 ) );
		EXPECT_FALSE( Visible( direction, 1 ) );
	}
}

TEST_F( SpecialSights, CappedRangeAndCamouflageUseExistingFogLimits )
{
	theWarFog.Clear();
	theWarFog.Init( 64, 64, 20, 2.0f );
	AddSight( -0.3f, 0.3f, 10000.0f );
	AddUnit();
	EXPECT_TRUE( Visible( 0, 9 ) );
	EXPECT_FALSE( Visible( 0, 12 ) );
	EXPECT_TRUE( theWarFog.IsUnitVisible( Point( 0, 6 ), party, true ) );
	EXPECT_FALSE( theWarFog.IsUnitVisible( Point( 0, 9 ), party, true ) );
}

TEST_F( SpecialSights, DetectionOverlayUsesTheRealObserverAndCyanCircle )
{
	RegisterSelectedUnit();
	CObj<IAILogic> logic = dynamic_cast<IAILogic *>( NObjectFactory::MakeObject( 0x1108D441 ) );
	ASSERT_NE( logic, nullptr );
	SShootAreas areas;
	logic->GetDetectionAreas( unitID, &areas );
	EXPECT_TRUE( areas.areas.empty() );

	AddUnit();
	logic->GetDetectionAreas( unitID, &areas );
	ASSERT_EQ( areas.areas.size(), 1u );
	const SShootArea &area = areas.areas.front();
	EXPECT_FLOAT_EQ( area.fMaxR, 2.0f * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE );
	EXPECT_FLOAT_EQ( area.vCenter3D.x, center.x * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE );
	EXPECT_FLOAT_EQ( area.vCenter3D.y, center.y * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE );
	EXPECT_EQ( area.wStartAngle, area.wFinishAngle );
	EXPECT_EQ( area.GetColor(), 0x0000ffffu );
	EXPECT_EQ( area.GetMiniMapCircleColor(), 0xf0ff );

	AddSight();
	Refresh();
	areas.areas.clear();
	logic->GetDetectionAreas( unitID, &areas );
	ASSERT_EQ( areas.areas.size(), 1u );
	EXPECT_FLOAT_EQ( areas.areas.front().fMaxR, 8.0f * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE );
	EXPECT_TRUE( theWarFog.IsUnitVisible( Point( 0, 6 ), party, true ) );
	EXPECT_FALSE( theWarFog.IsUnitVisible( Point( 0, 9 ), party, true ) );

	stats->fSpyDiscoveryRange = 100.0f;
	Refresh();
	areas.areas.clear();
	logic->GetDetectionAreas( unitID, &areas );
	ASSERT_EQ( areas.areas.size(), 1u );
	EXPECT_FLOAT_EQ( areas.areas.front().fMaxR, 12.0f * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE );
	EXPECT_TRUE( theWarFog.IsUnitVisible( Point( 0, 10 ), party, true ) );
	EXPECT_FALSE( theWarFog.IsUnitVisible( Point( 16384, 10 ), party, true ) );

	theWarFog.DeleteUnit( unitID );
	areas.areas.clear();
	logic->GetDetectionAreas( unitID, &areas );
	EXPECT_TRUE( areas.areas.empty() );
	theWarFog.Segment();
	EXPECT_FALSE( theWarFog.IsUnitVisible( Point( 0, 6 ), party, true ) );
}

TEST_F( SpecialSights, DetectionRadiusFollowsConfigurationSightChangesAndFogCap )
{
	NGlobal::SetVar( "AI.WarFog.CamouflageOpenRadius", 3 );
	AddSight();
	AddUnit();
	EXPECT_FLOAT_EQ( theWarFog.GetDetectionRadius( unitID ), 3.0f * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE );
	EXPECT_TRUE( theWarFog.IsUnitVisible( Point( 0, 2 ), party, true ) );
	EXPECT_FALSE( theWarFog.IsUnitVisible( Point( 0, 4 ), party, true ) );

	modifier->sightRange.nZeroCount = 1;
	Refresh();
	EXPECT_FLOAT_EQ( theWarFog.GetDetectionRadius( unitID ), 0.0f );
	EXPECT_FALSE( theWarFog.IsUnitVisible( Point( 0, 2 ), party, true ) );

	// Reconfigure only between worlds, as a mod's simulation settings are applied before a mission.
	theWarFog.Clear();
	theWarFog.Init( 64, 64, 10, 2.0f );
	NGlobal::SetVar( "AI.WarFog.CamouflageOpenRadius", 20 );
	modifier->sightRange.nZeroCount = 0;
	AddUnit();
	EXPECT_FLOAT_EQ( theWarFog.GetDetectionRadius( unitID ), 5.0f * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE );
}

TEST_F( SpecialSights, SpyDetectionOverrideUsesMetersFallbackAndSightCap )
{
	stats->fSight = 28.0f;
	AddUnit();

	// Changing only the override must refresh a stationary observer and remove its old close counters.
	struct SCase
	{
		float range;
		int expectedTiles;
	};

	for ( const SCase &test : { SCase{ 24.0f, 12 },
	                            SCase{ 6.0f, 3 },
	                            SCase{ 0.0f, 8 },
	                            SCase{ -5.0f, 8 },
	                            SCase{ 7.9f, 3 },
	                            SCase{ ( std::numeric_limits<float>::max )(), 14 } } )
	{
		SCOPED_TRACE( test.range );
		stats->fSpyDiscoveryRange = test.range;
		Refresh();
		EXPECT_FLOAT_EQ( theWarFog.GetDetectionRadius( unitID ),
		                 float( test.expectedTiles * AI_TILES_IN_VIS_TILE * AI_TILE_SIZE ) );
		EXPECT_TRUE( theWarFog.IsUnitVisible( Point( 0, test.expectedTiles - 1 ), party, true ) );
		EXPECT_FALSE( theWarFog.IsUnitVisible( Point( 0, test.expectedTiles + 1 ), party, true ) );
	}
	modifier->sightRange.fMultBonus = 0.5f;
	Refresh();
	EXPECT_FLOAT_EQ( theWarFog.GetDetectionRadius( unitID ), 14.0f * AI_TILE_SIZE );
	EXPECT_FALSE( theWarFog.IsUnitVisible( Point( 0, 10 ), party, true ) );
	modifier->sightRange.nZeroCount = 1;
	Refresh();
	EXPECT_FLOAT_EQ( theWarFog.GetDetectionRadius( unitID ), 0.0f );
	EXPECT_FALSE( theWarFog.IsUnitVisible( Point( 0, 2 ), party, true ) );
}

TEST_F( SpecialSights, InfantryUsesItsOwnSpyDetectionStat )
{
	NDb::SInfantryRPGStats *infantry = MakeRecord<NDb::SInfantryRPGStats>( "InfantryRPGStats" );
	ASSERT_NE( infantry, nullptr );
	infantry->fSight = 28.0f;
	infantry->fSpyDiscoveryRange = 24.0f;
	PlaceUnit( 0, infantry );
	EXPECT_FLOAT_EQ( Info().fSpyDiscoveryRange, 24.0f );
	AddUnit();
	EXPECT_FLOAT_EQ( theWarFog.GetDetectionRadius( unitID ), 24.0f * AI_TILE_SIZE );
	EXPECT_TRUE( theWarFog.IsUnitVisible( Point( 0, 10 ), party, true ) );
	infantry->fSpyDiscoveryRange = 6.0f;
	Refresh();
	EXPECT_TRUE( theWarFog.IsUnitVisible( Point( 0, 2 ), party, true ) );
	EXPECT_FALSE( theWarFog.IsUnitVisible( Point( 0, 10 ), party, true ) );
}

TEST_F( SpecialSights, PendingDetectionChangeSurvivesSaveTransferAndRemoval )
{
	stats->fSight = 28.0f;
	stats->fSpyDiscoveryRange = 24.0f;
	AddUnit();
	stats->fSpyDiscoveryRange = 6.0f;
	theWarFog.UpdateUnit( unitID, Info() );
	// Save before Segment: old coverage and the newly requested radius differ.
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &theWarFog );
	}
	theWarFog.Clear();
	stream.Seek( 0 );
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->Add( 1, &theWarFog );
	}
	theWarFog.Segment();
	EXPECT_TRUE( theWarFog.IsUnitVisible( Point( 0, 2 ), party, true ) );
	EXPECT_FALSE( theWarFog.IsUnitVisible( Point( 0, 10 ), party, true ) );
	theWarFog.ChangeParty( unitID, Info(), 0 );
	theWarFog.Segment();
	EXPECT_FALSE( theWarFog.IsUnitVisible( Point( 0, 2 ), party, true ) );
	EXPECT_TRUE( theWarFog.IsUnitVisible( Point( 0, 2 ), 0, true ) );
	EXPECT_FALSE( theWarFog.IsUnitVisible( Point( 0, 10 ), 0, true ) );
	theWarFog.DeleteUnit( unitID );
	theWarFog.Segment();
	EXPECT_FALSE( theWarFog.IsUnitVisible( Point( 0, 2 ), 0, true ) );
}

TEST_F( SpecialSights, SpyDetectionDataDefaultsRoundTripAndChecksum )
{
	const CDBPtr<NDb::SMechUnitRPGStats> legacyMech = NDb::Get<NDb::SMechUnitRPGStats>( CDBID( "legacy-mech.xdb" ) );
	const CDBPtr<NDb::SInfantryRPGStats> legacyInfantry =
			NDb::Get<NDb::SInfantryRPGStats>( CDBID( "legacy-infantry.xdb" ) );
	const CDBPtr<NDb::SInfantryRPGStats> detector = NDb::Get<NDb::SInfantryRPGStats>( CDBID( "detector.xdb" ) );
	ASSERT_NE( legacyMech, nullptr );
	ASSERT_NE( legacyInfantry, nullptr );
	ASSERT_NE( detector, nullptr );
	EXPECT_FLOAT_EQ( legacyMech->fSpyDiscoveryRange, -1.0f );
	EXPECT_FLOAT_EQ( legacyInfantry->fSpyDiscoveryRange, -1.0f );
	EXPECT_FLOAT_EQ( detector->fSpyDiscoveryRange, 24.0f );
	EXPECT_FLOAT_EQ( stats->fSpyDiscoveryRange, -1.0f );
	NDb::CResource *resource = stats;
	const auto checksum = resource->CalcCheckSum();
	stats->fSpyDiscoveryRange = 24.0f;
	EXPECT_NE( resource->CalcCheckSum(), checksum );
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->AddPolymorphicBase( 1, stats );
	}
	NDb::SMechUnitRPGStats *loaded = MakeRecord<NDb::SMechUnitRPGStats>( "MechUnitRPGStats" );
	ASSERT_NE( loaded, nullptr );
	stream.Seek( 0 );
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->AddPolymorphicBase( 1, loaded );
	}
	EXPECT_FLOAT_EQ( loaded->fSpyDiscoveryRange, 24.0f );

	// Legacy binary records must retain the negative default when chunk 40 is absent.
	CMemoryStream legacyStream;
	SFields empty{ []( IBinSaver & ) {} };
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &legacyStream, SAVER_MODE_WRITE );
		saver->Add( 1, &empty );
	}
	NDb::SMechUnitRPGStats *legacyLoaded = MakeRecord<NDb::SMechUnitRPGStats>( "MechUnitRPGStats" );
	ASSERT_NE( legacyLoaded, nullptr );
	SFields base{ [&]( IBinSaver &saver ) { legacyLoaded->NDb::SUnitBaseRPGStats::operator&( saver ); } };
	legacyStream.Seek( 0 );
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &legacyStream, SAVER_MODE_READ );
		saver->Add( 1, &base );
	}
	EXPECT_FLOAT_EQ( legacyLoaded->fSpyDiscoveryRange, -1.0f );
}

TEST_F( SpecialSights, AircraftSectorDoesNotRevealAnEntireCircle )
{
	stats->etype = NDb::RPG_TYPE_AVIA_SCOUT;
	stats->fSight = 0.0f;
	AddSight();
	ASSERT_TRUE( Info().bPlane );
	AddUnit();
	EXPECT_TRUE( Visible( 0, 10 ) );
	EXPECT_FALSE( Visible( 16384, 10 ) );
	EXPECT_FALSE( Visible( 32768, 2 ) );
	// Removing the observer also removes sectors when it becomes neutral.
	theWarFog.ChangeParty( unitID, Info(), 2 );
	theWarFog.Segment();
	EXPECT_FALSE( Visible( 0, 10 ) );
}

TEST_F( SpecialSights, MovementRangeReductionAndClearingRemoveOldCoverage )
{
	AddSight();
	AddSight( float( PI ) - 0.3f, float( PI ) + 0.3f, 12 );
	AddUnit();
	EXPECT_TRUE( Visible( 32768, 5 ) );
	EXPECT_FALSE( Visible( 32768, 10 ) );
	SWarFogUnitInfo moved = Info();
	moved.vPos = SVector( 2, 2 );
	theWarFog.UpdateUnit( unitID, moved );
	theWarFog.Segment();
	EXPECT_FALSE( Visible( 0, 10 ) );
	Refresh();
	EXPECT_TRUE( Visible( 0, 10 ) );
	extended->specialSightsArray[0].fSightRange = 8;
	Refresh();
	EXPECT_FALSE( Visible( 0, 10 ) );
	EXPECT_TRUE( Visible( 0, 3 ) );
	extended->specialSightsArray.clear();
	Refresh();
	EXPECT_FALSE( Visible( 0, 3 ) );
	EXPECT_TRUE( Visible( 0, 2 ) );
}

TEST_F( SpecialSights, TerrainStillBlocksGroundSectors )
{
	AddSight();
	AddUnit();
	ASSERT_TRUE( Visible( 0, 10 ) );
	// Build a tall ring around the observer using the real terrain-visibility cache.
	for ( int y = -3; y <= 3; ++y )
	{
		for ( int x = -3; x <= 3; ++x )
		{
			if ( std::abs( x ) == 3 || std::abs( y ) == 3 )
			{
				theWarFog.AddStaticObjectTile(
						SVector( ( center.x + x ) * AI_TILES_IN_VIS_TILE, ( center.y + y ) * AI_TILES_IN_VIS_TILE ), 456, 1000.0f );
			}
		}
	}
	theWarFog.FinishInitialization();
	theWarFog.Segment();
	EXPECT_FALSE( Visible( 0, 10 ) );
}

TEST_F( SpecialSights, FogSaveLoadPreservesSectorsAndRemovesLoadedCoverage )
{
	AddSight();
	AddSight( -0.4f, 0.4f, 18, "turret" );
	TurnTurret( 16384 );
	AddUnit();
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &theWarFog );
	}
	theWarFog.Clear();
	stream.Seek( 0 );
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->Add( 1, &theWarFog );
	}
	theWarFog.Segment();
	EXPECT_TRUE( Visible( 0, 10 ) );
	EXPECT_TRUE( Visible( 16384, 7 ) );
	EXPECT_FALSE( Visible( 32768, 10 ) );
	theWarFog.DeleteUnit( unitID );
	theWarFog.Segment();
	EXPECT_FALSE( Visible( 0, 10 ) );
	EXPECT_FALSE( Visible( 16384, 7 ) );
}

TEST_F( SpecialSights, SightDataRoundTripAndChecksumIncludePlatformName )
{
	AddSight( -0.4f, 0.6f, 28, "turret" );
	const auto checksum = extended->CalcCheckSum();
	extended->specialSightsArray[0].szPlatformName = "hull";
	EXPECT_NE( extended->CalcCheckSum(), checksum );
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, extended );
	}
	CObj<NDb::SExtendedStats> loaded = new NDb::SExtendedStats;
	stream.Seek( 0 );
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->Add( 1, loaded.GetPtr() );
	}
	ASSERT_EQ( loaded->specialSightsArray.size(), 1u );
	EXPECT_EQ( loaded->specialSightsArray[0].szPlatformName, "hull" );
	EXPECT_FLOAT_EQ( loaded->specialSightsArray[0].fAngleStartRad, -0.4f );
	EXPECT_FLOAT_EQ( loaded->specialSightsArray[0].fAngleEndRad, 0.6f );
	EXPECT_FLOAT_EQ( loaded->specialSightsArray[0].fSightRange, 28.0f );
}

TEST_F( SpecialSights, LegacyFogRecordWithoutSectorsLoadsAsOriginalCircle )
{
	SWarFogUnitInfo original = Info();
	SFields oldFormat{ [&]( IBinSaver &saver )
	                   {
											 saver.Add( 2, &original.vPos );
											 saver.Add( 3, &original.sector );
											 saver.Add( 4, &original.nRadius );
											 saver.Add( 5, &original.bPlane );
										 } };
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &oldFormat );
	}
	SWarFogUnitInfo loaded;
	loaded.fSpyDiscoveryRange = 24.0f;
	stream.Seek( 0 );
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->Add( 1, &loaded );
	}
	EXPECT_EQ( loaded.nRadius, original.nRadius );
	EXPECT_TRUE( loaded.specialSights.empty() );
	EXPECT_FLOAT_EQ( loaded.fSpyDiscoveryRange, -1.0f );
	theWarFog.AddUnit( unitID, loaded, party );
	theWarFog.Segment();
	EXPECT_TRUE( Visible( 32768, 2 ) );
	EXPECT_FALSE( Visible( 0, 10 ) );
}
} // namespace
