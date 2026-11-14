#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../AIUnit.h"
#include "../B2AI.h"
#include "../DBAIConsts.h"
#include "../Diplomacy.h"
#include "../HitProtection.h"
#include "../NewUpdater.h"
#include "../Shell.h"
#include "../Soldier.h"
#include "../Technics.h"
#include "../UnitStates.h"
#include "Common_RTS_AI/AIMap.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "System/GlobalVars.h"
#include "System/RandomGen.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
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

class HitProtection : public testing::Test
{
	static std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	CObj<CAIMap> map;
	std::vector<SBinSaverExternalObject> external;
	CMemoryStream updaterState;
	CObj<IAILogic> logic;

protected:
	NDb::SMechUnitRPGStats *stats = nullptr;
	NDb::SWeaponRPGStats *weapon = nullptr;
	NDb::SAIGameConsts *constants = nullptr;
	NDb::SInfantryRPGStats *infantry = nullptr;
	CObj<NDb::SUnitStatsModifier> modifier;
	CObj<CAIUnit> unit;
	CObj<CExplosion> explosion;

	static void SetUpTestSuite()
	{
		// Anchor the factories on Windows and ELF builds with import pruning.
		ASSERT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-hit-protection-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( HIT_PROTECTION_TYPES_FILE, directory / "types.xml" );
		std::ofstream( directory / "legacy.xdb" ) << "<MechUnitRPGStats/>";
		std::ofstream( directory / "legacy-consts.xdb" ) << "<AIGameConsts/>";
		std::ofstream( directory / "area-consts.xdb" )
				<< "<AIGameConsts><AreaDamageSystem>ResistanceBased</AreaDamageSystem>"
					 "<AreaDamageFalloff><a>-0.5</a><b>-0.25</b><c>0.75</c></AreaDamageFalloff></AIGameConsts>";
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		ASSERT_TRUE( NDb::RegisterResourceFile( "legacy.xdb" ) );
		ASSERT_TRUE( NDb::RegisterResourceFile( "legacy-consts.xdb" ) );
		ASSERT_TRUE( NDb::RegisterResourceFile( "area-consts.xdb" ) );
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

	void LoadFields( CObjectBase *object, const std::function<void( IBinSaver & )> &visit )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			SFields fields{ visit };
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		loader->AddPolymorphicBase( 1, object );
	}

	void SetUp() override
	{
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &updaterState, SAVER_MODE_WRITE );
			saver->Add( 1, &updater );
		}
		constants = MakeRecord<NDb::SAIGameConsts>( "AIGameConsts" );
		ASSERT_NE( constants, nullptr );
		logic = dynamic_cast<IAILogic *>( NObjectFactory::MakeObject( 0x1108D441 ) );
		ASSERT_NE( logic.GetPtr(), nullptr );
		NSingleton::RegisterSingleton( logic, IAILogic::tidTypeID );
		CDBPtr<NDb::SAIGameConsts> constantsPointer = constants;
		LoadFields( logic, [&]( IBinSaver &saver ) { saver.Add( 20, &constantsPointer ); } );
		NGlobal::SetVar( "AI.Common.AreaDamageCoeff", 0.3f );
		NGlobal::SetVar( "AI.Common.ArmorForAreaDamage", 300 );
		stats = MakeRecord<NDb::SMechUnitRPGStats>( "MechUnitRPGStats" );
		weapon = MakeRecord<NDb::SWeaponRPGStats>( "WeaponRPGStats" );
		ASSERT_NE( stats, nullptr );
		ASSERT_NE( weapon, nullptr );
		weapon->shells.resize( 1 );
		stats->vAABBHalfSize = CVec2( 8, 8 );
		stats->fSmallAABBCoeff = 0.25f;
		stats->fMaxHP = 200;
		stats->etype = NDb::RPG_TYPE_ARM_MEDIUM;
		for ( auto &armor : stats->armors )
		{
			armor.nMin = armor.nMax = 0;
		}
		weapon->shells[0].nPiercing = 10;
		weapon->shells[0].nPiercingRandom = 0;
		weapon->shells[0].fDamagePower = 10;
		weapon->shells[0].nDamageRandom = 0;
		modifier = MakeRecord<NDb::SUnitStatsModifier>( "UnitStatsModifier" );
		// Avoid the inclusive RNG upper endpoint in tests unrelated to cover rejection.
		modifier->cover.fMultBonus = 2;
		unit = dynamic_cast<CAIUnit *>( NObjectFactory::MakeObject( AI_TANK ) );
		explosion = dynamic_cast<CExplosion *>( NObjectFactory::MakeObject( 0x1108D44B ) );
		ASSERT_TRUE( unit != nullptr );
		ASSERT_TRUE( explosion != nullptr );
		external.emplace_back( 1, unit.GetPtr() );
		external.emplace_back( 2, modifier.GetPtr() );
		map = new CAIMap( 256, 256, 32, 2, 16 );
		SetAIMap( map );
		theDipl.SetNPlayers( 2 );
		theDipl.SetParty( 1, 1 );
		ResetUnit();
		SetImpact( CVec3( 80, 80, 0 ) );
	}

	void TearDown() override
	{
		// Restore the empty updater through its exported save/load interface.
		updaterState.Seek( 0 );
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &updaterState, SAVER_MODE_READ );
			saver->Add( 1, &updater );
		}
		explosion = nullptr;
		unit = nullptr;
		external.clear();
		SetAIMap( nullptr );
		NSingleton::UnRegisterSingleton( IAILogic::tidTypeID );
		logic = nullptr;
	}

	void ResetUnit( bool bInfantry = false, bool bLying = false, int nInside = 0 )
	{
		// Load the real resolver's inputs without starting graphics, orders or a mission.
		CVec3 position( 80, 80, 0 );
		SVector tile( 2, 2 );
		SAIAngle direction( 0 );
		uint8_t player = 1;
		float hp = 100;
		NTimer::STime acknowledged = 1;
		CDBPtr<NDb::SMechUnitRPGStats> statsPointer = stats;
		CUnitStatsModifiers modifiers;
		modifiers.Set( "test.fixture", modifier );
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
									saver.Add( 6, &player );
									saver.Add( 15, &hp );
									saver.Add( 58, &modifiers );
									saver.Add( 68, &acknowledged );
								} };
		SFields mechanical{ [&]( IBinSaver &saver )
		                    {
													saver.Add( 1, &ai );
													saver.Add( 2, &statsPointer );
												} };
		if ( bInfantry )
		{
			CDBPtr<NDb::SInfantryRPGStats> infantryPointer = infantry;
			SFields soldier{ [&]( IBinSaver &saver )
			                 {
												 saver.Add( 1, &ai );
												 saver.Add( 3, &nInside );
												 saver.Add( 4, &infantryPointer );
												 saver.Add( 17, &bLying );
											 } };
			LoadFields( unit, [&]( IBinSaver &saver ) { saver.Add( 1, &soldier ); } );
			// Soldier damage checks its current state's kind even in this headless fixture.
			unit->SetCurState( dynamic_cast<IUnitState *>( NObjectFactory::MakeObject( 0x1108D482 ) ) );
		}
		else
		{
			LoadFields( unit, [&]( IBinSaver &saver ) { saver.Add( 1, &mechanical ); } );
		}
	}

	void MakeInfantry()
	{
		infantry = MakeRecord<NDb::SInfantryRPGStats>( "InfantryRPGStats" );
		ASSERT_NE( infantry, nullptr );
		infantry->vAABBHalfSize = stats->vAABBHalfSize;
		infantry->fSmallAABBCoeff = stats->fSmallAABBCoeff;
		infantry->fMaxHP = 200;
		unit = dynamic_cast<CAIUnit *>( NObjectFactory::MakeObject( AI_INFANTRY ) );
		ASSERT_NE( unit.GetPtr(), nullptr );
		external.clear();
		external.emplace_back( 1, unit.GetPtr() );
		external.emplace_back( 2, modifier.GetPtr() );
		ResetUnit( true );
		SetImpact( CVec3( 80, 80, 0 ) );
	}

	void SetImpact( CVec3 position )
	{
		CDBPtr<NDb::SWeaponRPGStats> weaponPointer = weapon;
		CPtr<CAIUnit> shooter = unit.GetPtr();
		int player = 1;
		NDb::SUnitStatsModifier::SParameterModifier unchanged;
		SFields base{ [&]( IBinSaver &saver )
		              {
										saver.Add( 3, &weaponPointer );
										saver.Add( 4, &shooter );
										saver.Add( 5, &position );
										saver.Add( 7, &player );
										saver.Add( 9, &unchanged );
										saver.Add( 10, &unchanged );
										saver.Add( 11, &unchanged );
										saver.Add( 12, &unchanged );
									} };
		LoadFields( explosion, [&]( IBinSaver &saver ) { saver.Add( 1, &base ); } );
	}
};

std::filesystem::path HitProtection::directory;

TEST_F( HitProtection, DefaultsPreserveLegacyProtectionAndRandomSequence )
{
	const auto *legacy = NDb::Get<NDb::SMechUnitRPGStats>( CDBID( "legacy.xdb" ) );
	ASSERT_NE( legacy, nullptr );
	EXPECT_FLOAT_EQ( legacy->fAABBStrengthMin, 1 );
	EXPECT_FLOAT_EQ( legacy->fAABBStrengthMax, 1 );
	EXPECT_FLOAT_EQ( legacy->fCoverStrengthMin, 1 );
	EXPECT_FLOAT_EQ( legacy->fCoverStrengthMax, 1 );
	const auto before = NRandom::GetRandomCallsCounter();
	EXPECT_FALSE( NHitProtection::CracksAABB( weapon->shells[0], *legacy ) );
	EXPECT_FALSE( NHitProtection::CracksCover( weapon->shells[0], *legacy ) );
	EXPECT_EQ( NRandom::GetRandomCallsCounter(), before );
}

TEST_F( HitProtection, FractionalThresholdsAndEqualityAreIndependentForEachProtection )
{
	auto &shell = weapon->shells[0];
	stats->fAABBStrengthMin = stats->fAABBStrengthMax = 1.25f;
	stats->fCoverStrengthMin = stats->fCoverStrengthMax = 2.5f;
	shell.fAABBCrackBase = 1.125f;
	shell.fCoverCrackBase = 2.5f;
	EXPECT_FALSE( NHitProtection::CracksAABB( shell, *stats ) );
	EXPECT_TRUE( NHitProtection::CracksCover( shell, *stats ) );
	shell.fAABBCrackBase = 1.25f;
	shell.fCoverCrackBase = 2.25f;
	EXPECT_TRUE( NHitProtection::CracksAABB( shell, *stats ) );
	EXPECT_FALSE( NHitProtection::CracksCover( shell, *stats ) );
}

TEST_F( HitProtection, SymmetricCrackAndFractionalStrengthRollsRepeatFromSavedSeed )
{
	auto &shell = weapon->shells[0];
	shell.fAABBCrackBase = 1.5f;
	shell.fAABBCrackRandom = 0.5f;
	stats->fAABBStrengthMin = 1.25f;
	stats->fAABBStrengthMax = 1.75f;
	CObj<IRandomSeed> seed = NRandom::CreateRandomSeedCopy();
	seed->InitByZeroSeed();
	std::vector<bool> expected;
	for ( int pass = 0; pass < 2; ++pass )
	{
		NRandom::SetRandomSeed( seed );
		int successes = 0;
		const auto before = NRandom::GetRandomCallsCounter();
		for ( int i = 0; i < 2048; ++i )
		{
			const bool cracked = NHitProtection::CracksAABB( shell, *stats );
			if ( pass == 0 )
			{
				expected.push_back( cracked );
			}
			else
			{
				EXPECT_EQ( cracked, expected[i] );
			}
			successes += cracked;
		}
		// Both symmetric rolls produce roughly half successes, with two draws per attempt.
		EXPECT_GT( successes, 850 );
		EXPECT_LT( successes, 1200 );
		EXPECT_EQ( NRandom::GetRandomCallsCounter() - before, 4096 );
	}
}

TEST_F( HitProtection, DirectAABBCrackKeepsFullFootprintButStillRequiresArmorPenetration )
{
	SetImpact( CVec3( 86, 80, 0 ) );
	EXPECT_TRUE( unit->ProcessCumulativeExpl( explosion, 0, false ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 100 );
	weapon->shells[0].fAABBCrackBase = 1;
	EXPECT_TRUE( unit->ProcessCumulativeExpl( explosion, 0, false ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 90 );
	for ( auto &armor : stats->armors )
	{
		armor.nMin = armor.nMax = 100;
	}
	EXPECT_TRUE( unit->ProcessCumulativeExpl( explosion, 0, false ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 90 );
	SetImpact( CVec3( 90, 80, 0 ) );
	EXPECT_FALSE( unit->ProcessCumulativeExpl( explosion, 0, false ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 90 );
}

TEST_F( HitProtection, DirectCoverCrackBypassesEvenCertainCover )
{
	modifier->cover.fMultBonus = 0;
	unit->SetStatsModifier( "test.fixture", modifier );
	EXPECT_TRUE( unit->ProcessCumulativeExpl( explosion, 0, false ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 100 );
	weapon->shells[0].fCoverCrackBase = 1;
	EXPECT_TRUE( unit->ProcessCumulativeExpl( explosion, 0, false ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 90 );
	EXPECT_TRUE( unit->IsSavedByCover() );
}

TEST_F( HitProtection, SplashUsesBothCracksAndRetainsItsOwnArmorAndRadiusGates )
{
	SetImpact( CVec3( 89, 80, 0 ) );
	modifier->cover.fMultBonus = 0;
	unit->SetStatsModifier( "test.fixture", modifier );
	EXPECT_FALSE( unit->ProcessAreaDamage( explosion, 0, 10, 2 ) );
	weapon->shells[0].fCoverCrackBase = 1;
	EXPECT_FALSE( unit->ProcessAreaDamage( explosion, 0, 10, 2 ) );
	weapon->shells[0].fAABBCrackBase = 1;
	EXPECT_TRUE( unit->ProcessAreaDamage( explosion, 0, 10, 2 ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 97 );
	const float hp = unit->GetHitPoints();
	for ( auto &armor : stats->armors )
	{
		armor.nMin = armor.nMax = 301;
	}
	EXPECT_FALSE( unit->ProcessAreaDamage( explosion, 0, 10, 2 ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), hp );
	SetImpact( CVec3( 100, 80, 0 ) );
	EXPECT_FALSE( unit->ProcessAreaDamage( explosion, 0, 10, 2 ) );
}

TEST_F( HitProtection, LegacySplashIgnoresResistanceAndPreservesRandomDraws )
{
	stats->fAreaDmgResistanceMin = stats->fAreaDmgResistanceMax = 100;
	constants->areaDamageFalloff.fc = 0;
	SetImpact( CVec3( 87, 80, 0 ) );
	const auto before = NRandom::GetRandomCallsCounter();
	EXPECT_TRUE( unit->ProcessAreaDamage( explosion, 0, 100, 10 ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 97 );
	// Legacy cover, armor, damage and the tank's track-break roll; no area resistance roll.
	EXPECT_EQ( NRandom::GetRandomCallsCounter() - before, 4 );
}

TEST_F( HitProtection, ResistanceReplacesArmorAndUsesNearestCombatBoxDistance )
{
	constants->eAreaDamageSystem = NDb::SAIGameConsts::ResistanceBased;
	stats->fAreaDmgResistanceMin = stats->fAreaDmgResistanceMax = 0.75f;
	weapon->shells[0].fAreaCrackingBase = 0.75f;
	weapon->shells[0].fAreaCrackingRandom = 0;
	for ( auto &armor : stats->armors )
	{
		armor.nMin = armor.nMax = 1000;
	}
	// The compressed box extends to x=82. A blast at x=87 is halfway through Area=10.
	SetImpact( CVec3( 87, 80, 0 ) );
	EXPECT_TRUE( unit->ProcessAreaDamage( explosion, 0, 100, 10 ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 92.5f );
	ResetUnit();
	stats->fAreaDmgResistanceMin = stats->fAreaDmgResistanceMax = 0.875f;
	EXPECT_FALSE( unit->ProcessAreaDamage( explosion, 0, 100, 10 ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 100 );
}

TEST_F( HitProtection, ResistanceFalloffHandlesEdgesZeroRadiusAndCustomClampedCoefficients )
{
	constants->eAreaDamageSystem = NDb::SAIGameConsts::ResistanceBased;
	weapon->shells[0].fAreaCrackingBase = 1;
	weapon->shells[0].fAreaCrackingRandom = 0;
	const auto check = [&]( float impactX, float radius, float expectedHP )
	{
		ResetUnit();
		SetImpact( CVec3( impactX, 80, 0 ) );
		EXPECT_TRUE( unit->ProcessAreaDamage( explosion, 0, 100, radius ) );
		EXPECT_FLOAT_EQ( unit->GetHitPoints(), expectedHP );
	};
	check( 80, 10, 90 );
	check( 82, 10, 90 );
	check( 87, 10, 92.5f );
	check( 92, 10, 100 );
	check( 80, 0, 100 );
	SetImpact( CVec3( 93, 80, 0 ) );
	EXPECT_FALSE( unit->ProcessAreaDamage( explosion, 0, 100, 10 ) );

	auto &curve = constants->areaDamageFalloff;
	curve.fa = 0.25f;
	curve.fb = -0.5f;
	curve.fc = 0.75f;
	check( 87, 10, 94.375f );
	check( 80, 0, 95 ); // A point blast uses x=1, even when the point lies inside the box.
	curve.fc = 2;
	check( 87, 10, 90 );
	curve.fc = -2;
	check( 87, 10, 100 );
}

TEST_F( HitProtection, ResistanceRetainsCoverGeometryAndHeightAndLeavesDirectHitsAlone )
{
	constants->eAreaDamageSystem = NDb::SAIGameConsts::ResistanceBased;
	modifier->cover.fMultBonus = 0;
	unit->SetStatsModifier( "test.fixture", modifier );
	weapon->shells[0].fAreaCrackingBase = 1;
	weapon->shells[0].fAreaCrackingRandom = 0;
	SetImpact( CVec3( 89, 80, 0 ) );
	EXPECT_FALSE( unit->ProcessAreaDamage( explosion, 0, 100, 2 ) );
	weapon->shells[0].fCoverCrackBase = 1;
	const auto before = NRandom::GetRandomCallsCounter();
	EXPECT_FALSE( unit->ProcessAreaDamage( explosion, 0, 100, 2 ) );
	EXPECT_EQ( NRandom::GetRandomCallsCounter(), before );
	weapon->shells[0].fAABBCrackBase = 1;
	EXPECT_TRUE( unit->ProcessAreaDamage( explosion, 0, 100, 2 ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 92.5f );
	ResetUnit();
	SetImpact( CVec3( 80, 80, 64 ) );
	EXPECT_FALSE( unit->ProcessAreaDamage( explosion, 0, 100, 10 ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 100 );
	stats->fAreaDmgResistanceMin = stats->fAreaDmgResistanceMax = 100;
	SetImpact( CVec3( 80, 80, 0 ) );
	EXPECT_TRUE( unit->ProcessBurstExpl( explosion, 0, 100, 10 ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 90 );
}

TEST_F( HitProtection, InfantryUsesOnlyArea2AndCannotRetryFailedResistanceOrCover )
{
	MakeInfantry();
	constants->eAreaDamageSystem = NDb::SAIGameConsts::ResistanceBased;
	weapon->shells[0].fAreaCrackingBase = 1;
	weapon->shells[0].fAreaCrackingRandom = 0;
	SetImpact( CVec3( 87, 80, 0 ) );
	// Both postures use Area2=10, with a gap of 5 to the compressed box.
	for ( bool lying : { false, true } )
	{
		ResetUnit( true, lying );
		EXPECT_TRUE( unit->ProcessAreaDamage( explosion, 0, 10, 1 ) );
		EXPECT_FLOAT_EQ( unit->GetHitPoints(), 92.5f );
	}
	ResetUnit( true );
	EXPECT_FALSE( unit->ProcessAreaDamage( explosion, 0, 1, 100 ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 100 );
	infantry->fAreaDmgResistanceMin = infantry->fAreaDmgResistanceMax = 2;
	EXPECT_FALSE( unit->ProcessAreaDamage( explosion, 0, 10, 100 ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 100 );
	infantry->fAreaDmgResistanceMin = infantry->fAreaDmgResistanceMax = 0;
	modifier->cover.fMultBonus = 0;
	unit->SetStatsModifier( "test.fixture", modifier );
	EXPECT_FALSE( unit->ProcessAreaDamage( explosion, 0, 10, 100 ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 100 );
	weapon->shells[0].fCoverCrackBase = 1;
	ResetUnit( true, false, 1 ); // Buildings retain their infantry splash exclusion.
	EXPECT_FALSE( unit->ProcessAreaDamage( explosion, 0, 10, 100 ) );
	ResetUnit( true );
	SetImpact( CVec3( 80, 80, 9 ) );
	EXPECT_FALSE( unit->ProcessAreaDamage( explosion, 0, 10, 100 ) );
}

TEST_F( HitProtection, LegacyInfantryRetainsStandingFallbackAndIgnoresResistance )
{
	MakeInfantry();
	infantry->fAreaDmgResistanceMin = infantry->fAreaDmgResistanceMax = 100;
	modifier->cover.fMultBonus = 0;
	unit->SetStatsModifier( "test.fixture", modifier );
	SetImpact( CVec3( 87, 80, 0 ) );
	EXPECT_TRUE( unit->ProcessAreaDamage( explosion, 0, 10, 1 ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 97 );
	ResetUnit( true, true );
	EXPECT_FALSE( unit->ProcessAreaDamage( explosion, 0, 10, 1 ) );
	EXPECT_FLOAT_EQ( unit->GetHitPoints(), 100 );
}

TEST_F( HitProtection, AreaResistanceRandomRollsRepeatAndKeepFractionalThresholds )
{
	auto &shell = weapon->shells[0];
	CObj<IRandomSeed> seed = NRandom::CreateRandomSeedCopy();
	seed->InitByZeroSeed();
	std::vector<bool> expected;
	for ( int pass = 0; pass < 2; ++pass )
	{
		NRandom::SetRandomSeed( seed );
		const auto before = NRandom::GetRandomCallsCounter();
		int successes = 0;
		for ( int i = 0; i < 2048; ++i )
		{
			const bool cracked = NHitProtection::CracksAreaResistance( shell, *stats );
			if ( pass == 0 )
			{
				expected.push_back( cracked );
			}
			else
			{
				EXPECT_EQ( cracked, expected[i] );
			}
			successes += cracked;
		}
		EXPECT_GT( successes, 850 );
		EXPECT_LT( successes, 1200 );
		EXPECT_EQ( NRandom::GetRandomCallsCounter() - before, 4096 );
	}
	shell.fAreaCrackingRandom = 0;
	stats->fAreaDmgResistanceMin = stats->fAreaDmgResistanceMax = 0.625f;
	EXPECT_FALSE( NHitProtection::CracksAreaResistance( shell, *stats ) );
	shell.fAreaCrackingBase = 0.625f;
	EXPECT_TRUE( NHitProtection::CracksAreaResistance( shell, *stats ) );
}

TEST_F( HitProtection, NearestRectangleDistanceHandlesRotationCornersAndPointBoxes )
{
	SRect rect;
	rect.InitRect( CVec2( 20, 30 ), CVec2( 0, 1 ), 4, 2, 3 );
	EXPECT_FLOAT_EQ( rect.GetDistanceToPoint2( CVec2( 20, 30 ) ), 0 );
	EXPECT_FLOAT_EQ( rect.GetDistanceToPoint2( CVec2( 23, 34 ) ), 0 );
	EXPECT_FLOAT_EQ( rect.GetDistanceToPoint2( CVec2( 20, 25 ) ), 9 );
	EXPECT_FLOAT_EQ( rect.GetDistanceToPoint2( CVec2( 26, 38 ) ), 25 );
	EXPECT_TRUE( rect.IsIntersectCircle( CVec2( 26, 38 ), 5 ) );
	EXPECT_FALSE( rect.IsIntersectCircle( CVec2( 26, 38 ), 4.99f ) );
	rect.InitRect( CVec2( 20, 30 ), CVec2( 1, 0 ), 0, 0 );
	EXPECT_FLOAT_EQ( rect.GetDistanceToPoint2( CVec2( 23, 34 ) ), 25 );
}

TEST_F( HitProtection, AreaDefaultsAndAuthoredXml )
{
	const auto *legacy = NDb::Get<NDb::SAIGameConsts>( CDBID( "legacy-consts.xdb" ) );
	const auto *authored = NDb::Get<NDb::SAIGameConsts>( CDBID( "area-consts.xdb" ) );
	ASSERT_NE( legacy, nullptr );
	ASSERT_NE( authored, nullptr );
	EXPECT_EQ( legacy->eAreaDamageSystem, NDb::SAIGameConsts::Legacy );
	EXPECT_FLOAT_EQ( legacy->areaDamageFalloff.fa, -1 );
	EXPECT_FLOAT_EQ( legacy->areaDamageFalloff.fb, 0 );
	EXPECT_FLOAT_EQ( legacy->areaDamageFalloff.fc, 1 );
	EXPECT_EQ( authored->eAreaDamageSystem, NDb::SAIGameConsts::ResistanceBased );
	EXPECT_FLOAT_EQ( authored->areaDamageFalloff.fa, -0.5f );
	EXPECT_FLOAT_EQ( authored->areaDamageFalloff.fb, -0.25f );
	EXPECT_FLOAT_EQ( authored->areaDamageFalloff.fc, 0.75f );
	EXPECT_FLOAT_EQ( stats->fAreaDmgResistanceMin, 0 );
	EXPECT_FLOAT_EQ( stats->fAreaDmgResistanceMax, 1 );
	EXPECT_FLOAT_EQ( weapon->shells[0].fAreaCrackingBase, 0.5f );
	EXPECT_FLOAT_EQ( weapon->shells[0].fAreaCrackingRandom, 0.5f );
	// Loading old records must retain nonzero defaults in both the shell and nested coefficients.
	LoadFields( constants, []( IBinSaver & ) {} );
	EXPECT_EQ( constants->eAreaDamageSystem, NDb::SAIGameConsts::Legacy );
	EXPECT_FLOAT_EQ( constants->areaDamageFalloff.fa, -1 );
	EXPECT_FLOAT_EQ( constants->areaDamageFalloff.fc, 1 );
	int count = 1;
	SFields oldShell{ []( IBinSaver & ) {} };
	SFields oldShells{ [&]( IBinSaver &saver )
	                   {
											 saver.Add( 2, &count );
											 saver.Add( 1, &oldShell, 1 );
										 } };
	LoadFields( weapon, [&]( IBinSaver &saver ) { saver.Add( 12, &oldShells ); } );
	ASSERT_EQ( weapon->shells.size(), 1 );
	EXPECT_FLOAT_EQ( weapon->shells[0].fAreaCrackingBase, 0.5f );
	EXPECT_FLOAT_EQ( weapon->shells[0].fAreaCrackingRandom, 0.5f );
}

TEST_F( HitProtection, OlderBinaryUnitFieldsKeepStrengthDefaults )
{
	// An older base record has no new chunks. Generated preserveDefault guards
	// must prevent the binary reader from replacing strength 1 with zero.
	CMemoryStream stream;
	SFields oldFields{ []( IBinSaver & ) {} };
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, &oldFields );
	}
	stream.Seek( 0 );
	SFields baseFields{ [&]( IBinSaver &saver ) { stats->NDb::SUnitBaseRPGStats::operator&( saver ); } };
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->Add( 1, &baseFields );
	}
	EXPECT_FLOAT_EQ( stats->fAABBStrengthMin, 1 );
	EXPECT_FLOAT_EQ( stats->fAABBStrengthMax, 1 );
	EXPECT_FLOAT_EQ( stats->fCoverStrengthMin, 1 );
	EXPECT_FLOAT_EQ( stats->fCoverStrengthMax, 1 );
	EXPECT_FLOAT_EQ( stats->fAreaDmgResistanceMin, 0 );
	EXPECT_FLOAT_EQ( stats->fAreaDmgResistanceMax, 1 );
}

TEST_F( HitProtection, AllNewFieldsSurviveBinarySerializationAndAffectChecksums )
{
	stats->fAABBStrengthMin = 0.25f;
	stats->fAABBStrengthMax = 0.75f;
	stats->fCoverStrengthMin = 1.25f;
	stats->fCoverStrengthMax = 1.75f;
	stats->fAreaDmgResistanceMin = 0.375f;
	stats->fAreaDmgResistanceMax = 0.875f;
	auto &shell = weapon->shells[0];
	shell.fAABBCrackBase = 2.25f;
	shell.fAABBCrackRandom = 2.75f;
	shell.fCoverCrackBase = 3.25f;
	shell.fCoverCrackRandom = 3.75f;
	shell.fAreaCrackingBase = 0.625f;
	shell.fAreaCrackingRandom = 0.125f;
	constants->eAreaDamageSystem = NDb::SAIGameConsts::ResistanceBased;
	constants->areaDamageFalloff.fa = -0.75f;
	constants->areaDamageFalloff.fb = 0.25f;
	constants->areaDamageFalloff.fc = 0.5f;
	const auto checkFields = []( NDb::CResource *resource, const std::vector<float *> &fields )
	{
		for ( float *field : fields )
		{
			const uint32_t before = resource->CalcCheckSum();
			const float saved = *field;
			*field += 0.125f;
			EXPECT_NE( resource->CalcCheckSum(), before );
			*field = saved;
			EXPECT_EQ( resource->CalcCheckSum(), before );
		}
	};
	checkFields( stats,
	             { &stats->fAABBStrengthMin,
	               &stats->fAABBStrengthMax,
	               &stats->fCoverStrengthMin,
	               &stats->fCoverStrengthMax,
	               &stats->fAreaDmgResistanceMin,
	               &stats->fAreaDmgResistanceMax } );
	checkFields( weapon,
	             { &shell.fAABBCrackBase,
	               &shell.fAABBCrackRandom,
	               &shell.fCoverCrackBase,
	               &shell.fCoverCrackRandom,
	               &shell.fAreaCrackingBase,
	               &shell.fAreaCrackingRandom } );
	checkFields(
			constants,
			{ &constants->areaDamageFalloff.fa, &constants->areaDamageFalloff.fb, &constants->areaDamageFalloff.fc } );
	const uint32_t resistanceChecksum = constants->CalcCheckSum();
	constants->eAreaDamageSystem = NDb::SAIGameConsts::Legacy;
	EXPECT_NE( constants->CalcCheckSum(), resistanceChecksum );
	constants->eAreaDamageSystem = NDb::SAIGameConsts::ResistanceBased;
	const auto copyFields = []( CObjectBase *source, CObjectBase *destination )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->AddPolymorphicBase( 1, source );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->AddPolymorphicBase( 1, destination );
	};
	auto *loadedUnit = MakeRecord<NDb::SMechUnitRPGStats>( "MechUnitRPGStats" );
	auto *loadedWeapon = MakeRecord<NDb::SWeaponRPGStats>( "WeaponRPGStats" );
	auto *loadedConsts = MakeRecord<NDb::SAIGameConsts>( "AIGameConsts" );
	ASSERT_NE( loadedUnit, nullptr );
	ASSERT_NE( loadedWeapon, nullptr );
	ASSERT_NE( loadedConsts, nullptr );
	copyFields( stats, loadedUnit );
	copyFields( weapon, loadedWeapon );
	copyFields( constants, loadedConsts );
	EXPECT_FLOAT_EQ( loadedUnit->fAABBStrengthMin, 0.25f );
	EXPECT_FLOAT_EQ( loadedUnit->fAABBStrengthMax, 0.75f );
	EXPECT_FLOAT_EQ( loadedUnit->fCoverStrengthMin, 1.25f );
	EXPECT_FLOAT_EQ( loadedUnit->fCoverStrengthMax, 1.75f );
	EXPECT_FLOAT_EQ( loadedUnit->fAreaDmgResistanceMin, 0.375f );
	EXPECT_FLOAT_EQ( loadedUnit->fAreaDmgResistanceMax, 0.875f );
	ASSERT_EQ( loadedWeapon->shells.size(), 1 );
	EXPECT_FLOAT_EQ( loadedWeapon->shells[0].fAABBCrackBase, 2.25f );
	EXPECT_FLOAT_EQ( loadedWeapon->shells[0].fAABBCrackRandom, 2.75f );
	EXPECT_FLOAT_EQ( loadedWeapon->shells[0].fCoverCrackBase, 3.25f );
	EXPECT_FLOAT_EQ( loadedWeapon->shells[0].fCoverCrackRandom, 3.75f );
	EXPECT_FLOAT_EQ( loadedWeapon->shells[0].fAreaCrackingBase, 0.625f );
	EXPECT_FLOAT_EQ( loadedWeapon->shells[0].fAreaCrackingRandom, 0.125f );
	EXPECT_EQ( loadedConsts->eAreaDamageSystem, NDb::SAIGameConsts::ResistanceBased );
	EXPECT_FLOAT_EQ( loadedConsts->areaDamageFalloff.fa, -0.75f );
	EXPECT_FLOAT_EQ( loadedConsts->areaDamageFalloff.fb, 0.25f );
	EXPECT_FLOAT_EQ( loadedConsts->areaDamageFalloff.fc, 0.5f );
}
} // namespace
