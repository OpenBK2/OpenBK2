#include "../stdafx.h"

#include "../AIInternalConsts.h"
#include "../B2AI.h"
#include "../DBAIConsts.h"
#include "../Technics.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "System/GlobalVars.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <algorithm>
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

class AIPropertyConsts : public testing::Test
{
	static std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;

protected:
	static CObj<NDb::SAIPropertyConsts> startupValues;
	const NDb::SAIPropertyConsts *example = nullptr;
	CObj<IAILogic> logic;

	static void SetUpTestSuite()
	{
		ASSERT_EQ( CastToObjectBaseImpl<CTank>( nullptr, static_cast<void *>( nullptr ) ), nullptr );
		startupValues = new NDb::SAIPropertyConsts;
		SConsts::GetEffectivePropertyConsts( startupValues.GetPtr() );
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-ai-properties-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( AI_PROPERTIES_TYPES_FILE, directory / "types.xml" );
		std::filesystem::copy_file( AI_PROPERTIES_EXAMPLE_FILE, directory / "properties.xdb" );
		std::ofstream( directory / "legacy.xdb" ) << "<AIGameConsts/>";
		std::ofstream( directory / "partial.xdb" )
				<< "<AIPropertyConsts><CommonAreaDamageCoeff>0.75</CommonAreaDamageCoeff></AIPropertyConsts>";
		const std::string root = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( root ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( root ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		for ( const char *name : { "properties.xdb", "legacy.xdb", "partial.xdb" } )
		{
			ASSERT_TRUE( NDb::RegisterResourceFile( name ) );
		}
	}

	static void TearDownTestSuite()
	{
		startupValues = nullptr;
		NDb::CloseDatabase();
		NVFS::SetMainVFS( nullptr );
		NVFS::SetMainFileCreator( nullptr );
		std::error_code error;
		std::filesystem::remove_all( directory, error );
	}

	void SetUp() override
	{
		SConsts::Load();
		NGlobal::SetVar( "AI.Common.AreaDamageCoeff", 0.125f );
		NGlobal::SetVar( "AI.Common.ArmorForAreaDamage", 7 );
		example = NDb::Get<NDb::SAIPropertyConsts>( CDBID( "properties.xdb" ) );
		ASSERT_NE( example, nullptr );
	}

	void TearDown() override
	{
		logic = nullptr;
		SConsts::Load();
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

	static void LoadFields( CObjectBase *object, const std::function<void( IBinSaver & )> &visit )
	{
		CMemoryStream stream;
		{
			CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
			SFields fields{ visit };
			saver->Add( 1, &fields );
		}
		stream.Seek( 0 );
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->AddPolymorphicBase( 1, object );
	}

	static uint32_t EffectiveChecksum()
	{
		CObj<NDb::SAIPropertyConsts> actual = new NDb::SAIPropertyConsts;
		SConsts::GetEffectivePropertyConsts( actual );
		// The authored example is ordered by name; runtime lookup is ordered by numeric type.
		std::sort( actual->revealInfo.begin(),
		           actual->revealInfo.end(),
		           []( const auto &a, const auto &b ) { return a.szUnitType < b.szUnitType; } );
		return actual->CalcCheckSum();
	}
};

std::filesystem::path AIPropertyConsts::directory;
CObj<NDb::SAIPropertyConsts> AIPropertyConsts::startupValues;

TEST_F( AIPropertyConsts, SchemaDefaultsMatchAllRuntimeInitialValues )
{
	CObj<NDb::SAIPropertyConsts> defaults = new NDb::SAIPropertyConsts;
	EXPECT_EQ( defaults->CalcCheckSum(), startupValues->CalcCheckSum() );
	const auto *legacy = NDb::Get<NDb::SAIGameConsts>( CDBID( "legacy.xdb" ) );
	ASSERT_NE( legacy, nullptr );
	EXPECT_EQ( legacy->pAIPropertyConsts.GetPtr(), nullptr );
	EXPECT_FALSE( SConsts::UsesPropertyConsts() );
}

TEST_F( AIPropertyConsts, ExampleOverridesEveryScalarAndRevealEntry )
{
	SConsts::Load( example );
	EXPECT_TRUE( SConsts::UsesPropertyConsts() );
	EXPECT_EQ( EffectiveChecksum(), example->CalcCheckSum() );
	CObj<NDb::SAIPropertyConsts> actual = new NDb::SAIPropertyConsts;
	SConsts::GetEffectivePropertyConsts( actual );
	EXPECT_FLOAT_EQ( actual->fCommonAreaDamageCoeff, 0.3f );
	EXPECT_EQ( actual->nCommonArmorForAreaDamage, 300 );
	EXPECT_FLOAT_EQ( actual->fPlaneMaxHeight, 2500 );
	EXPECT_FLOAT_EQ( actual->fEngineersEngineerRuCarryWeight, 5040 );
	EXPECT_EQ( actual->revealInfo.size(), 16 );
}

TEST_F( AIPropertyConsts, ConsoleChangesCannotOverrideDataAndAreRetainedForFallback )
{
	SConsts::Load( example );
	NGlobal::SetVar( "AI.Common.AreaDamageCoeff", 0.875f );
	NGlobal::SetVar( "AI.Common.ArmorForAreaDamage", 888 );
	NGlobal::SetVar( "AI.Artillery.DispersionRatio.LineMin", 9.0f );
	NGlobal::SetVar( "plane_min_height", 42.0f );
	NGlobal::SetVar( "AI.Infantry.SpyGlassAngle", 90.0f );
	NGlobal::SetVar( "AI.Aviation.FighterPatrolTime", 60 );
	NGlobal::SetVar( "use_round_units", true );
	NGlobal::SetVar( "AI.RevealInfo.soldier.Query", 0.625f );
	EXPECT_EQ( EffectiveChecksum(), example->CalcCheckSum() );
	SConsts::Load( example );
	EXPECT_EQ( EffectiveChecksum(), example->CalcCheckSum() );

	SConsts::Load();
	EXPECT_FALSE( SConsts::UsesPropertyConsts() );
	CObj<NDb::SAIPropertyConsts> actual = new NDb::SAIPropertyConsts;
	SConsts::GetEffectivePropertyConsts( actual );
	EXPECT_FLOAT_EQ( actual->fCommonAreaDamageCoeff, 0.875f );
	EXPECT_EQ( actual->nCommonArmorForAreaDamage, 888 );
	EXPECT_FLOAT_EQ( actual->fArtilleryDispersionRatioLineMin, 9 );
	EXPECT_FLOAT_EQ( actual->fPlaneMinHeight, 42 );
	EXPECT_FLOAT_EQ( actual->fInfantrySpyGlassAngle, 90 );
	EXPECT_EQ( actual->nAviationFighterPatrolTime, 60 );
	EXPECT_TRUE( actual->bUseRoundUnits );
}

TEST_F( AIPropertyConsts, OmittedResourceFieldsUseSchemaDefaultsNotTheLocalProfile )
{
	const auto *partial = NDb::Get<NDb::SAIPropertyConsts>( CDBID( "partial.xdb" ) );
	ASSERT_NE( partial, nullptr );
	SConsts::Load( example );
	SConsts::Load( partial );
	CObj<NDb::SAIPropertyConsts> actual = new NDb::SAIPropertyConsts;
	SConsts::GetEffectivePropertyConsts( actual );
	EXPECT_FLOAT_EQ( actual->fCommonAreaDamageCoeff, 0.75f );
	EXPECT_EQ( actual->nCommonArmorForAreaDamage, 10 );
	EXPECT_TRUE( actual->revealInfo.empty() );
}

TEST_F( AIPropertyConsts, BinaryRoundTripAndParentChecksumIncludeProperties )
{
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		saver->Add( 1, const_cast<NDb::SAIPropertyConsts *>( example ) );
	}
	stream.Seek( 0 );
	auto *loaded = Record<NDb::SAIPropertyConsts>( "AIPropertyConsts" );
	ASSERT_NE( loaded, nullptr );
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		saver->Add( 1, loaded );
	}
	EXPECT_EQ( loaded->CalcCheckSum(), example->CalcCheckSum() );
	auto *parent = Record<NDb::SAIGameConsts>( "AIGameConsts" );
	ASSERT_NE( parent, nullptr );
	const uint32_t withoutProperties = parent->CalcCheckSum();
	parent->pAIPropertyConsts = loaded;
	const uint32_t withProperties = parent->CalcCheckSum();
	EXPECT_NE( withProperties, withoutProperties );
	loaded->fCommonAreaDamageCoeff += 0.125f;
	EXPECT_NE( parent->CalcCheckSum(), withProperties );
}

TEST_F( AIPropertyConsts, SaveLoadingSelectsItsResourceAndOldSavesReturnToLegacy )
{
	auto *parent = Record<NDb::SAIGameConsts>( "AIGameConsts" );
	ASSERT_NE( parent, nullptr );
	parent->pAIPropertyConsts = example;
	CDBPtr<NDb::SAIGameConsts> constants = parent;
	logic = dynamic_cast<IAILogic *>( NObjectFactory::MakeObject( 0x1108D441 ) );
	ASSERT_NE( logic.GetPtr(), nullptr );
	LoadFields( logic, [&]( IBinSaver &saver ) { saver.Add( 20, &constants ); } );
	EXPECT_TRUE( SConsts::UsesPropertyConsts() );
	EXPECT_EQ( EffectiveChecksum(), example->CalcCheckSum() );

	constants = NDb::Get<NDb::SAIGameConsts>( CDBID( "legacy.xdb" ) );
	LoadFields( logic, [&]( IBinSaver &saver ) { saver.Add( 20, &constants ); } );
	EXPECT_FALSE( SConsts::UsesPropertyConsts() );
	CObj<NDb::SAIPropertyConsts> actual = new NDb::SAIPropertyConsts;
	SConsts::GetEffectivePropertyConsts( actual );
	EXPECT_FLOAT_EQ( actual->fCommonAreaDamageCoeff, 0.125f );
	EXPECT_EQ( actual->nCommonArmorForAreaDamage, 7 );
}
} // namespace
