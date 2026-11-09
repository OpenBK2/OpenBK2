#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../Artillery.h"
#include "../Executor.h"
#include "../Technics.h"
#include "../UnitStatsModifiers.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "Stats_B2_M1/AbilityActions.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <filesystem>
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

class UnitStatsModifiers : public testing::Test
{
	static std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;

protected:
	static void SetUpTestSuite()
	{
		ASSERT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-unit-modifiers-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( UNIT_STATS_MODIFIERS_TYPES_FILE, directory / "types.xml" );
		const std::string root = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( root ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( root ) );
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

	NDb::SUnitStatsModifier *Modifier()
	{
		CObj<NDb::IObjMan> record = NDb::CreateNewObject( "UnitStatsModifier" );
		if ( !record )
		{
			ADD_FAILURE() << "Could not create modifier resource";
			return nullptr;
		}
		static unsigned nextRecord = 0;
		const std::string name = "modifier" + std::to_string( nextRecord++ ) + ".xdb";
		EXPECT_TRUE( NDb::AddNewObject( name, CDBID( name ), record ) );
		records.push_back( record );
		return dynamic_cast<NDb::SUnitStatsModifier *>( record->GetObject() );
	}

	template <class T>
	void RoundTrip( T *object )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->Add( 1, object );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		loader->Add( 1, object );
	}

	void RoundTrip( CObjectBase *object )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			saver->AddPolymorphicBase( 1, object );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ );
		loader->AddPolymorphicBase( 1, object );
	}
};

std::filesystem::path UnitStatsModifiers::directory;

TEST_F( UnitStatsModifiers, CombinesAndRemovesEveryParameter )
{
	auto *first = Modifier();
	auto *second = Modifier();
	// Exercise every field, including fields with currently limited gameplay consumers.
	const auto members = { &NDb::SUnitStatsModifier::durability,
	                       &NDb::SUnitStatsModifier::smallAABBCoeff,
	                       &NDb::SUnitStatsModifier::camouflage,
	                       &NDb::SUnitStatsModifier::sightPower,
	                       &NDb::SUnitStatsModifier::sightRange,
	                       &NDb::SUnitStatsModifier::speed,
	                       &NDb::SUnitStatsModifier::rotateSpeed,
	                       &NDb::SUnitStatsModifier::weaponDispersion,
	                       &NDb::SUnitStatsModifier::weaponDamage,
	                       &NDb::SUnitStatsModifier::weaponPiercing,
	                       &NDb::SUnitStatsModifier::weaponTrackDamageProb,
	                       &NDb::SUnitStatsModifier::weaponRelaxTime,
	                       &NDb::SUnitStatsModifier::weaponAimTime,
	                       &NDb::SUnitStatsModifier::weaponShellSpeed,
	                       &NDb::SUnitStatsModifier::weaponArea,
	                       &NDb::SUnitStatsModifier::weaponArea2,
	                       &NDb::SUnitStatsModifier::cover };
	for ( auto member : members )
	{
		( first->*member ).fAddBonus = 3;
		( first->*member ).fMultBonus = 2;
		( second->*member ).fAddBonus = -1;
		( second->*member ).fMultBonus = 0.25f;
	}
	CUnitStatsModifiers modifiers;
	modifiers.Set( "first", first );
	modifiers.Set( "second", second );
	for ( auto member : members )
	{
		EXPECT_FLOAT_EQ( ( modifiers.GetCombined()->*member ).Get( 10 ), 6 );
	}
	modifiers.Remove( "first" );
	for ( auto member : members )
	{
		EXPECT_FLOAT_EQ( ( modifiers.GetCombined()->*member ).Get( 10 ), 2.25f );
	}
	modifiers.Clear();
	for ( auto member : members )
	{
		EXPECT_FLOAT_EQ( ( modifiers.GetCombined()->*member ).Get( 10 ), 10 );
	}
}

TEST_F( UnitStatsModifiers, SourcesReplaceWithoutStackingAndMayShareResources )
{
	auto *first = Modifier();
	auto *second = Modifier();
	first->speed.fMultBonus = 2;
	second->speed.fMultBonus = 3;
	CUnitStatsModifiers modifiers;
	modifiers.Set( "ability", first );
	modifiers.Set( "ability", first );
	EXPECT_FLOAT_EQ( modifiers.GetCombined()->speed.Get( 1 ), 2 );
	modifiers.Set( "terrain", first );
	EXPECT_FLOAT_EQ( modifiers.GetCombined()->speed.Get( 1 ), 4 );
	modifiers.Set( "ability", second );
	EXPECT_FLOAT_EQ( modifiers.GetCombined()->speed.Get( 1 ), 6 );
	modifiers.Remove( "absent" );
	modifiers.Set( "ability", nullptr );
	modifiers.Remove( "ability" );
	EXPECT_FLOAT_EQ( modifiers.GetCombined()->speed.Get( 1 ), 2 );
	EXPECT_EQ( modifiers.Find( "terrain" ), first );
	EXPECT_EQ( modifiers.Find( "ability" ), nullptr );
}

TEST_F( UnitStatsModifiers, ZeroMultipliersAndZeroCountsCanBeRemovedInAnyOrder )
{
	auto *zero = Modifier();
	auto *count = Modifier();
	auto *other = Modifier();
	zero->speed.fMultBonus = 0;
	count->speed.nZeroCount = 1;
	other->speed.fAddBonus = 2;
	other->speed.fMultBonus = 1.3f;
	CUnitStatsModifiers modifiers;
	modifiers.Set( "other", other );
	const float expected = modifiers.GetCombined()->speed.Get( 10 );
	for ( int i = 0; i < 100; ++i )
	{
		modifiers.Set( "zero1", zero );
		modifiers.Set( "zero2", zero );
		modifiers.Set( "count", count );
		modifiers.Remove( "zero1" );
		EXPECT_EQ( modifiers.GetCombined()->speed.Get( 10 ), 0 );
		modifiers.Remove( "count" );
		EXPECT_EQ( modifiers.GetCombined()->speed.Get( 10 ), 0 );
		modifiers.Remove( "zero2" );
		// Exact comparison detects accumulated inverse-arithmetic drift.
		EXPECT_EQ( modifiers.GetCombined()->speed.Get( 10 ), expected );
	}
}

TEST_F( UnitStatsModifiers, SaveLoadPreservesSourcesAndDetMapInsertionOrder )
{
	auto *large = Modifier();
	auto *negative = Modifier();
	auto *smallBonus = Modifier();
	large->speed.fAddBonus = 1e20f;
	negative->speed.fAddBonus = -1e20f;
	smallBonus->speed.fAddBonus = 1;
	CUnitStatsModifiers modifiers;
	// Key sorting would lose the small term. Replacement must keep the existing slot too.
	modifiers.Set( "z", large );
	modifiers.Set( "a", negative );
	modifiers.Set( "m", smallBonus );
	modifiers.Set( "z", large );
	EXPECT_EQ( modifiers.GetCombined()->speed.Get( 0 ), 1 );
	RoundTrip( &modifiers );
	EXPECT_EQ( modifiers.GetCombined()->speed.Get( 0 ), 1 );
	EXPECT_EQ( modifiers.Find( "z" ), large );
	modifiers.Remove( "z" );
	modifiers.Remove( "a" );
	EXPECT_EQ( modifiers.GetCombined()->speed.Get( 0 ), 1 );
	modifiers.Remove( "m" );
	EXPECT_EQ( modifiers.GetCombined()->speed.Get( 10 ), 10 );
}

TEST_F( UnitStatsModifiers, UnitSaveLoadRetainsRemovableZeroSource )
{
	CObj<CAIUnit> unit = dynamic_cast<CAIUnit *>( NObjectFactory::MakeObject( AI_TANK ) );
	ASSERT_TRUE( unit );
	auto *zero = Modifier();
	auto *other = Modifier();
	zero->weaponDamage.fMultBonus = 0;
	other->weaponDamage.fMultBonus = 3;
	unit->SetStatsModifier( "zero", zero );
	unit->SetStatsModifier( "other", other );
	RoundTrip( static_cast<CObjectBase *>( unit.GetPtr() ) );
	EXPECT_EQ( unit->GetStatsModifier()->weaponDamage.Get( 10 ), 0 );
	unit->RemoveStatsModifier( "zero" );
	EXPECT_EQ( unit->GetStatsModifier()->weaponDamage.Get( 10 ), 30 );
}

TEST_F( UnitStatsModifiers, ScopedAntiAviationRestoresOverlapsAndNestedScopes )
{
	CObj<CAIUnit> unit = dynamic_cast<CAIUnit *>( NObjectFactory::MakeObject( AI_TANK ) );
	ASSERT_TRUE( unit );
	auto *other = Modifier();
	auto *antiAir = Modifier();
	auto *zero = Modifier();
	other->weaponDamage.fMultBonus = 3;
	antiAir->weaponDamage.fMultBonus = 2;
	zero->weaponDamage.fMultBonus = 0;
	unit->SetStatsModifier( "other", other );
	{
		CScopedUnitStatsModifier shot( unit, NStatsModifierSource::AntiAviation, antiAir );
		EXPECT_EQ( unit->GetStatsModifier()->weaponDamage.Get( 10 ), 60 );
		{
			CScopedUnitStatsModifier nested( unit, NStatsModifierSource::AntiAviation, zero );
			EXPECT_EQ( unit->GetStatsModifier()->weaponDamage.Get( 10 ), 0 );
		}
		EXPECT_EQ( unit->GetStatsModifier()->weaponDamage.Get( 10 ), 60 );
	}
	EXPECT_EQ( unit->GetStatsModifier()->weaponDamage.Get( 10 ), 30 );
	EXPECT_EQ( unit->FindStatsModifier( NStatsModifierSource::AntiAviation ), nullptr );
}

TEST_F( UnitStatsModifiers, CounterFireExitEventsRemoveOnlyTheirOwnBonus )
{
	CObj<CArtillery> unit = dynamic_cast<CArtillery *>( NObjectFactory::MakeObject( AI_ARTILLERY ) );
	CObj<IExecutor> executor = dynamic_cast<IExecutor *>( NObjectFactory::MakeObject( 0x1912D300 ) );
	ASSERT_TRUE( unit );
	ASSERT_TRUE( executor );
	unit->nUniqueID = 123;
	auto *bonus = Modifier();
	bonus->weaponDamage.fMultBonus = 2;
	unit->SetStatsModifier( "other", bonus );
	const std::string source = NStatsModifierSource::Ability( NDb::ABILITY_ANTIATRILLERY_FIRE );
	for ( auto event :
	      { EID_ABILITY_DISABLE, EID_ABILITY_DEACTIVATE_AUTOCAST, EID_ABILITY_DEACTIVATE, EID_ABILITY_ENABLE } )
	{
		SCOPED_TRACE( int( event ) );
		unit->SetStatsModifier( source, bonus );
		CPtr<CArtillery> pointer = unit.GetPtr();
		SAbilitySwitchState state;
		state.eState = EASS_ACTIVE;
		state.bAutocast = true;
		bool applied = true;
		SFields fields{ [&]( IBinSaver &saver )
		                {
											saver.Add( 2, &pointer );
											saver.Add( 3, &state );
											saver.Add( 12, &applied );
										} };
		std::vector<SBinSaverExternalObject> external{ { 1, unit } };
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE, external );
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		{
			CPtr<IBinSaver> loader = CreateBinSaver( &stream, SAVER_MODE_READ, external );
			loader->AddPolymorphicBase( 1, executor );
		}
		CExecutorEventSpecialAbility notification( SExecutorEventParam( event, 0, unit->GetUniqueId() ),
		                                           NDb::ABILITY_ANTIATRILLERY_FIRE );
		EXPECT_TRUE( executor->NotifyEvent( notification ) );
		EXPECT_EQ( unit->FindStatsModifier( source ), nullptr );
		EXPECT_EQ( unit->GetStatsModifier()->weaponDamage.Get( 10 ), 20 );
	}
}
} // namespace
