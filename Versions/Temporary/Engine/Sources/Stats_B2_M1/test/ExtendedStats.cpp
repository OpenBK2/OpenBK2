#include "../stdafx.h"

#include "../RPGStats.h"
#include "AILogic/AIClassesID.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

class CHelicopter;

namespace
{
TEST( ExtendedStatsRegistration, DatabaseAndSimulationTypesHaveDistinctFactories )
{
	// The exported cast forces AILogic to load, even with Windows import pruning
	// or ELF --as-needed. No simulation/world initialization is required.
	EXPECT_EQ( CastToUserObject( static_cast<CObjectBase *>( nullptr ), static_cast<CHelicopter *>( nullptr ) ),
	           nullptr );
	EXPECT_NE( int( NDb::SExtendedStats::typeID ), int( AI_HELICOPTER ) );
	ASSERT_TRUE( NObjectFactory::IsRegistered( AI_HELICOPTER ) );
	CObj<CObjectBase> object = NObjectFactory::MakeObject( NDb::SExtendedStats::typeID );
	ASSERT_NE( dynamic_cast<NDb::SExtendedStats *>( object.GetPtr() ), nullptr );
}

class ExtendedStats : public testing::Test
{
protected:
	static std::filesystem::path directory;

	static void SetUpTestSuite()
	{
		// Fail before binding fields if another loaded module owns this resource's ID.
		CObj<CObjectBase> object = NObjectFactory::MakeObject( NDb::SExtendedStats::typeID );
		ASSERT_NE( dynamic_cast<NDb::SExtendedStats *>( object.GetPtr() ), nullptr );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-extended-stats-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( EXTENDED_STATS_TYPES_FILE, directory / "types.xml" );
		std::ofstream( directory / "legacy.xdb" ) << "<MechUnitRPGStats/>";
		std::ofstream( directory / "defaults.xdb" ) << "<ExtendedStats/>";
		std::ofstream( directory / "extended.xdb" )
				<< "<ExtendedStats><ForwardTowed>true</ForwardTowed>"
					 "<VerticalOffsetAngleRad>-0.5</VerticalOffsetAngleRad></ExtendedStats>";
		std::ofstream( directory / "unit.xdb" )
				<< "<MechUnitRPGStats><ExtendedStats href=\"extended.xdb\"/></MechUnitRPGStats>";
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		for ( const char *file : { "legacy.xdb", "defaults.xdb", "extended.xdb", "unit.xdb" } )
		{
			ASSERT_TRUE( NDb::RegisterResourceFile( file ) );
		}
	}

	static void TearDownTestSuite()
	{
		NDb::CloseDatabase();
		std::error_code error;
		std::filesystem::remove_all( directory, error );
	}
};

std::filesystem::path ExtendedStats::directory;

TEST_F( ExtendedStats, MissingRecordAndFieldsKeepLegacyDefaults )
{
	const NDb::SMechUnitRPGStats *unit = NDb::Get<NDb::SMechUnitRPGStats>( CDBID( "legacy.xdb" ) );
	ASSERT_NE( unit, nullptr );
	EXPECT_TRUE( unit->pExtendedStats == nullptr );
	const NDb::SExtendedStats *defaults = NDb::Get<NDb::SExtendedStats>( CDBID( "defaults.xdb" ) );
	ASSERT_NE( defaults, nullptr );
	EXPECT_FALSE( defaults->bForwardTowed );
	EXPECT_FLOAT_EQ( defaults->fVerticalOffsetAngleRad, 0.0f );
	CObj<NDb::SExtendedStats> constructed = new NDb::SExtendedStats;
	EXPECT_FALSE( constructed->bForwardTowed );
	EXPECT_FLOAT_EQ( constructed->fVerticalOffsetAngleRad, 0.0f );
}

TEST_F( ExtendedStats, UnitLoadsReferencedSettingsFromXml )
{
	const NDb::SMechUnitRPGStats *unit = NDb::Get<NDb::SMechUnitRPGStats>( CDBID( "unit.xdb" ) );
	ASSERT_NE( unit, nullptr );
	ASSERT_TRUE( unit->pExtendedStats != nullptr );
	EXPECT_TRUE( unit->pExtendedStats->bForwardTowed );
	EXPECT_FLOAT_EQ( unit->pExtendedStats->fVerticalOffsetAngleRad, -0.5f );
}

TEST_F( ExtendedStats, BothFieldsAffectTheOwningUnitsChecksum )
{
	CObj<NDb::IObjMan> unitRecord = NDb::CreateNewObject( "MechUnitRPGStats" );
	CObj<NDb::IObjMan> extendedRecord = NDb::CreateNewObject( "ExtendedStats" );
	ASSERT_TRUE( unitRecord != nullptr );
	ASSERT_TRUE( extendedRecord != nullptr );
	auto *unit = dynamic_cast<NDb::SMechUnitRPGStats *>( unitRecord->GetObject() );
	auto *extended = dynamic_cast<NDb::SExtendedStats *>( extendedRecord->GetObject() );
	ASSERT_NE( unit, nullptr );
	ASSERT_NE( extended, nullptr );
	unit->pExtendedStats = extended;
	// Use the virtual resource interface, as the MP data traversal does.
	const NDb::CResource *resource = unit;
	const uint32_t original = resource->CalcCheckSum();
	extended->bForwardTowed = true;
	EXPECT_NE( resource->CalcCheckSum(), original );
	extended->bForwardTowed = false;
	EXPECT_EQ( resource->CalcCheckSum(), original );
	for ( const float angle : { -0.5f, 0.5f } )
	{
		extended->fVerticalOffsetAngleRad = angle;
		EXPECT_NE( resource->CalcCheckSum(), original );
	}
	extended->fVerticalOffsetAngleRad = 0.0f;
	EXPECT_EQ( resource->CalcCheckSum(), original );
}

TEST_F( ExtendedStats, BinarySerializationPreservesBothSettings )
{
	// Cover both directions and signed corrections through the generated serializer.
	for ( const bool forward : { false, true } )
	{
		for ( const float angle : { -0.5f, 0.0f, 0.5f } )
		{
			CObj<NDb::SExtendedStats> original = new NDb::SExtendedStats;
			original->bForwardTowed = forward;
			original->fVerticalOffsetAngleRad = angle;
			CMemoryStream stream;
			{
				CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
				saver->Add( 1, original.GetPtr() );
			}
			stream.Seek( 0 );
			CObj<NDb::SExtendedStats> loaded = new NDb::SExtendedStats;
			{
				CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
				saver->Add( 1, loaded.GetPtr() );
			}
			EXPECT_EQ( loaded->bForwardTowed, forward );
			EXPECT_FLOAT_EQ( loaded->fVerticalOffsetAngleRad, angle );
		}
	}
}

TEST_F( ExtendedStats, EditorCanCreateAssignSaveAndReloadSettings )
{
	{
		CObj<NDb::IObjMan> extended = NDb::CreateNewObject( "ExtendedStats" );
		CObj<NDb::IObjMan> unit = NDb::CreateNewObject( "MechUnitRPGStats" );
		ASSERT_TRUE( extended != nullptr );
		ASSERT_TRUE( unit != nullptr );
		ASSERT_TRUE( NDb::AddNewObject( "created-extended.xdb", CDBID( "created-extended.xdb" ), extended ) );
		ASSERT_TRUE( NDb::AddNewObject( "created-unit.xdb", CDBID( "created-unit.xdb" ), unit ) );
		// Use the editor's property-binding API, including its resource reference setter.
		ASSERT_TRUE( extended->SetValue( "ForwardTowed", true ) );
		ASSERT_TRUE( extended->SetValue( "VerticalOffsetAngleRad", -0.5f ) );
		ASSERT_TRUE( unit->SetValue( "ExtendedStats", CDBID( "created-extended.xdb" ) ) );
		NDb::SaveChanges();
		ASSERT_TRUE( NDb::GetLastSaveError().empty() );
	}
	// Discard cached objects so this exercises the XML load path from the crash report.
	NDb::CloseDatabase();
	ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
	CObj<NDb::IObjMan> unit = NDb::GetManipulator( CDBID( "created-unit.xdb" ) );
	ASSERT_TRUE( unit != nullptr );
	const auto *stats = dynamic_cast<NDb::SMechUnitRPGStats *>( unit->GetObject() );
	ASSERT_NE( stats, nullptr );
	ASSERT_TRUE( stats->pExtendedStats != nullptr );
	EXPECT_TRUE( stats->pExtendedStats->bForwardTowed );
	EXPECT_FLOAT_EQ( stats->pExtendedStats->fVerticalOffsetAngleRad, -0.5f );
}
} // namespace
