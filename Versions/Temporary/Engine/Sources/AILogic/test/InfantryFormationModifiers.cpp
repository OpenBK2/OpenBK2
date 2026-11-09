#include "../stdafx.h"

#include "../AIClassesID.h"
#include "../B2AI.h"
#include "../DBAIConsts.h"
#include "../Diplomacy.h"
#include "../Entrenchment.h"
#include "../Formation.h"
#include "../Soldier.h"
#include "../Technics.h"
#include "Common_RTS_AI/GroupSmoothPath.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
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

class InfantryFormationModifiers : public testing::Test
{
	static std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	std::vector<CObj<CObjectBase>> objects;
	std::vector<SBinSaverExternalObject> external;
	CObj<IAILogic> logic;

protected:
	NDb::SAIGameConsts *constants = nullptr;
	NDb::SSquadRPGStats *squad = nullptr;
	NDb::SInfantryRPGStats *infantry = nullptr;
	CObj<CFormation> formation;
	CObj<CGroupSmoothPath> path;
	std::vector<CPtr<CSoldier>> soldiers;

	static void SetUpTestSuite()
	{
		ASSERT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-formation-modifiers-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( FORMATION_MODIFIERS_TYPES_FILE, directory / "types.xml" );
		std::ofstream( directory / "legacy.xdb" ) << "<AIGameConsts/>";
		std::ofstream( directory / "disabled.xdb" ) << "<AIGameConsts><InfantryFormationModifiersApplyInTrenches>false"
																									 "</InfantryFormationModifiersApplyInTrenches></AIGameConsts>";
		const std::string root = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( root ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( root ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		ASSERT_TRUE( NDb::RegisterResourceFile( "legacy.xdb" ) );
		ASSERT_TRUE( NDb::RegisterResourceFile( "disabled.xdb" ) );
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
	T *Record( const char *type )
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

	void Keep( CObjectBase *object )
	{
		objects.push_back( object );
		external.emplace_back( int( external.size() + 1 ), object );
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
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ, external );
		saver->AddPolymorphicBase( 1, object );
	}

	void SetUp() override
	{
		constants = Record<NDb::SAIGameConsts>( "AIGameConsts" );
		ASSERT_NE( constants, nullptr );
		logic = dynamic_cast<IAILogic *>( NObjectFactory::MakeObject( 0x1108D441 ) );
		ASSERT_NE( logic.GetPtr(), nullptr );
		NSingleton::RegisterSingleton( logic, IAILogic::tidTypeID );
		CDBPtr<NDb::SAIGameConsts> constantsPointer = constants;
		LoadFields( logic, [&]( IBinSaver &saver ) { saver.Add( 20, &constantsPointer ); } );
		theDipl.SetNPlayers( 3 );
		theDipl.SetParty( 2, 2 );
		formation = dynamic_cast<CFormation *>( NObjectFactory::MakeObject( 0x1108D498 ) );
		ASSERT_NE( formation.GetPtr(), nullptr );
		path = new CGroupSmoothPath;
		Keep( formation );
		Keep( path );
		squad = Record<NDb::SSquadRPGStats>( "SquadRPGStats" );
		infantry = Record<NDb::SInfantryRPGStats>( "InfantryRPGStats" );
		ASSERT_NE( squad, nullptr );
		ASSERT_NE( infantry, nullptr );
		squad->formations.resize( 2 );
		for ( int i = 0; i < 2; ++i )
		{
			auto *modifier = Record<NDb::SUnitStatsModifier>( "UnitStatsModifier" );
			ASSERT_NE( modifier, nullptr );
			modifier->cover.fMultBonus = i == 0 ? 0.5f : 0.25f;
			modifier->speed.fMultBonus = i == 0 ? 3.0f : 4.0f;
			squad->formations[i].pStatsModifiers = modifier;
			squad->formations[i].nLieFlag = 0;
		}
	}

	void TearDown() override
	{
		NSingleton::UnRegisterSingleton( IAILogic::tidTypeID );
		soldiers.clear();
		external.clear();
		objects.clear();
		formation = nullptr;
		path = nullptr;
		logic = nullptr;
	}

	CSoldier *AddSoldier( int insideType, CObjectBase *shelter = nullptr )
	{
		auto *soldier = dynamic_cast<CSoldier *>( NObjectFactory::MakeObject( AI_INFANTRY ) );
		EXPECT_NE( soldier, nullptr );
		Keep( soldier );
		soldiers.push_back( soldier );
		CObj<NDb::SUnitStatsModifier> modifier = Record<NDb::SUnitStatsModifier>( "UnitStatsModifier" );
		// These represent independent terrain, shelter or ability modifiers and must survive.
		modifier->cover.fMultBonus = 0.75f;
		modifier->speed.fMultBonus = 2;
		Keep( modifier );
		CUnitStatsModifiers modifiers;
		modifiers.Set( "test.fixture", modifier );
		uint8_t player = 2;
		CDBPtr<NDb::SInfantryRPGStats> statsPointer = infantry;
		CPtr<CFormation> formationPointer = formation.GetPtr();
		CPtr<CObjectBase> shelterPointer = shelter;
		bool alwaysVisible = true;
		std::vector<int> visibility( 3, true );
		std::vector<int> disappearing( 3, false );
		std::vector<NTimer::STime> lastSeen( 3, 1 );
		SFields ai{ [&]( IBinSaver &saver )
		            {
									saver.Add( 6, &player );
									saver.Add( 43, &alwaysVisible );
									saver.Add( 44, &disappearing );
									saver.Add( 46, &lastSeen );
									saver.Add( 51, &visibility );
									saver.Add( 58, &modifiers );
								} };
		SFields fields{ [&]( IBinSaver &saver )
		                {
											saver.Add( 1, &ai );
											saver.Add( 2, &shelterPointer );
											saver.Add( 3, &insideType );
											saver.Add( 4, &statsPointer );
											saver.Add( 18, &formationPointer );
										} };
		LoadFields( soldier, [&]( IBinSaver &saver ) { saver.Add( 1, &fields ); } );
		CDBPtr<NDb::SSquadRPGStats> squadPointer = squad;
		LoadFields( formation,
		            [&]( IBinSaver &saver )
		            {
									saver.Add( 8, &squadPointer );
									saver.Add( 23, &path );
									saver.Add( 24, &soldiers );
								} );
		soldier->ApplyFormationStatsModifier( squad->formations[0].pStatsModifiers, true );
		return soldier;
	}

	void CheckModifier( CSoldier *soldier, float cover, float speed )
	{
		EXPECT_FLOAT_EQ( soldier->GetCover(), cover );
		EXPECT_FLOAT_EQ( soldier->GetStatsModifier()->speed.Get( 1 ), speed );
	}
};

std::filesystem::path InfantryFormationModifiers::directory;

TEST_F( InfantryFormationModifiers, DefaultsAndAuthoredXml )
{
	EXPECT_TRUE( constants->bInfantryFormationModifiersApplyInTrenches );
	const auto *legacy = NDb::Get<NDb::SAIGameConsts>( CDBID( "legacy.xdb" ) );
	const auto *disabled = NDb::Get<NDb::SAIGameConsts>( CDBID( "disabled.xdb" ) );
	ASSERT_NE( legacy, nullptr );
	ASSERT_NE( disabled, nullptr );
	EXPECT_TRUE( legacy->bInfantryFormationModifiersApplyInTrenches );
	EXPECT_FALSE( disabled->bInfantryFormationModifiersApplyInTrenches );
}

TEST_F( InfantryFormationModifiers, DefaultKeepsModifiersInEveryLocation )
{
	// Serialized occupancy: free, building, transport, entrenchment.
	for ( int inside = 0; inside < 4; ++inside )
	{
		CheckModifier( AddSoldier( inside ), 0.375f, 6 );
	}
	formation->ChangeGeometry( 1 );
	for ( CSoldier *soldier : soldiers )
	{
		CheckModifier( soldier, 0.1875f, 8 );
	}
}

TEST_F( InfantryFormationModifiers, DisabledSkipsShelteredMembersDuringFormationChanges )
{
	constants->bInfantryFormationModifiersApplyInTrenches = false;
	for ( int inside = 0; inside < 4; ++inside )
	{
		CSoldier *soldier = AddSoldier( inside );
		const bool sheltered = inside == 1 || inside == 3;
		CheckModifier( soldier, sheltered ? 0.75f : 0.375f, sheltered ? 2 : 6 );
	}
	formation->ChangeGeometry( 1 );
	for ( int inside = 0; inside < 4; ++inside )
	{
		const bool sheltered = inside == 1 || inside == 3;
		CheckModifier( soldiers[inside], sheltered ? 0.75f : 0.1875f, sheltered ? 2 : 8 );
	}
	formation->ChangeGeometry( 0 );
	formation->ChangeGeometry( 1 );
	CheckModifier( soldiers[1], 0.75f, 2 );
	CheckModifier( soldiers[3], 0.75f, 2 );
}

TEST_F( InfantryFormationModifiers, LegacyBinaryDefaultsAndConfiguredValuesSurviveLoading )
{
	LoadFields( constants, []( IBinSaver & ) {} );
	EXPECT_TRUE( constants->bInfantryFormationModifiersApplyInTrenches );
	const uint32_t legacyChecksum = constants->CalcCheckSum();
	bool enabled = false;
	LoadFields( constants, [&]( IBinSaver &saver ) { saver.Add( 18, &enabled ); } );
	EXPECT_FALSE( constants->bInfantryFormationModifiersApplyInTrenches );
	EXPECT_NE( constants->CalcCheckSum(), legacyChecksum );
	enabled = true;
	LoadFields( constants, [&]( IBinSaver &saver ) { saver.Add( 18, &enabled ); } );
	EXPECT_TRUE( constants->bInfantryFormationModifiersApplyInTrenches );
	EXPECT_EQ( constants->CalcCheckSum(), legacyChecksum );
}

TEST_F( InfantryFormationModifiers, LeavingShelterRemovesMasterOfStreetsAndTrenchOnly )
{
	auto *trench = dynamic_cast<CEntrenchment *>( NObjectFactory::MakeObject( 0x1108D497 ) );
	ASSERT_NE( trench, nullptr );
	Keep( trench );
	CSoldier *soldier = AddSoldier( 3, trench );
	auto *bonus = Record<NDb::SUnitStatsModifier>( "UnitStatsModifier" );
	bonus->speed.fMultBonus = 0;
	const std::string master = NStatsModifierSource::Ability( NDb::ABILITY_MASTER_OF_STREETS );
	soldier->SetStatsModifier( master, bonus );
	soldier->SetStatsModifier( NStatsModifierSource::Trench, bonus );
	EXPECT_EQ( soldier->GetStatsModifier()->speed.Get( 1 ), 0 );
	// Exercise the real exit, which temporarily changes occupancy for visibility updates.
	soldier->SetFree();
	EXPECT_TRUE( soldier->IsFree() );
	EXPECT_EQ( soldier->FindStatsModifier( master ), nullptr );
	EXPECT_EQ( soldier->FindStatsModifier( NStatsModifierSource::Trench ), nullptr );
	CheckModifier( soldier, 0.375f, 6 );
	soldier->SetFree();
	CheckModifier( soldier, 0.375f, 6 );
}
} // namespace
