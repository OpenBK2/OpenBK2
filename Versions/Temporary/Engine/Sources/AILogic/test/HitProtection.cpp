#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../AIUnit.h"
#include "../Diplomacy.h"
#include "../HitProtection.h"
#include "../NewUpdater.h"
#include "../Shell.h"
#include "../Technics.h"
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

protected:
	NDb::SMechUnitRPGStats *stats = nullptr;
	NDb::SWeaponRPGStats *weapon = nullptr;
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
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		ASSERT_TRUE( NDb::RegisterResourceFile( "legacy.xdb" ) );
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
		modifier = new NDb::SUnitStatsModifier;
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
	}

	void ResetUnit()
	{
		// Load the real resolver's inputs without starting graphics, orders or a mission.
		CVec3 position( 80, 80, 0 );
		SVector tile( 2, 2 );
		SAIAngle direction( 0 );
		uint8_t player = 1;
		float hp = 100;
		NTimer::STime acknowledged = 1;
		CDBPtr<NDb::SMechUnitRPGStats> statsPointer = stats;
		CPtr<NDb::SUnitStatsModifier> modifierPointer = modifier.GetPtr();
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
									saver.Add( 58, &modifierPointer );
									saver.Add( 68, &acknowledged );
								} };
		SFields mechanical{ [&]( IBinSaver &saver )
		                    {
													saver.Add( 1, &ai );
													saver.Add( 2, &statsPointer );
												} };
		LoadFields( unit, [&]( IBinSaver &saver ) { saver.Add( 1, &mechanical ); } );
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
}

TEST_F( HitProtection, AllNewFieldsSurviveBinarySerializationAndAffectChecksums )
{
	stats->fAABBStrengthMin = 0.25f;
	stats->fAABBStrengthMax = 0.75f;
	stats->fCoverStrengthMin = 1.25f;
	stats->fCoverStrengthMax = 1.75f;
	auto &shell = weapon->shells[0];
	shell.fAABBCrackBase = 2.25f;
	shell.fAABBCrackRandom = 2.75f;
	shell.fCoverCrackBase = 3.25f;
	shell.fCoverCrackRandom = 3.75f;
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
	checkFields(
			stats,
			{ &stats->fAABBStrengthMin, &stats->fAABBStrengthMax, &stats->fCoverStrengthMin, &stats->fCoverStrengthMax } );
	checkFields( weapon,
	             { &shell.fAABBCrackBase, &shell.fAABBCrackRandom, &shell.fCoverCrackBase, &shell.fCoverCrackRandom } );
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
	ASSERT_NE( loadedUnit, nullptr );
	ASSERT_NE( loadedWeapon, nullptr );
	copyFields( stats, loadedUnit );
	copyFields( weapon, loadedWeapon );
	EXPECT_FLOAT_EQ( loadedUnit->fAABBStrengthMin, 0.25f );
	EXPECT_FLOAT_EQ( loadedUnit->fAABBStrengthMax, 0.75f );
	EXPECT_FLOAT_EQ( loadedUnit->fCoverStrengthMin, 1.25f );
	EXPECT_FLOAT_EQ( loadedUnit->fCoverStrengthMax, 1.75f );
	ASSERT_EQ( loadedWeapon->shells.size(), 1 );
	EXPECT_FLOAT_EQ( loadedWeapon->shells[0].fAABBCrackBase, 2.25f );
	EXPECT_FLOAT_EQ( loadedWeapon->shells[0].fAABBCrackRandom, 2.75f );
	EXPECT_FLOAT_EQ( loadedWeapon->shells[0].fCoverCrackBase, 3.25f );
	EXPECT_FLOAT_EQ( loadedWeapon->shells[0].fCoverCrackRandom, 3.75f );
}
} // namespace
