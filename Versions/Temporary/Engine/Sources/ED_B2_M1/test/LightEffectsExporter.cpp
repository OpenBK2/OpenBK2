#include "ED_B2_M1/stdafx.h"

#include "ED_B2_M1/HPObjectRPGStatsExporter.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "Stats_B2_M1/ActionCommand.h"
#include "Stats_B2_M1/ActionsRemap.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <cfenv>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <utility>

#include <gtest/gtest.h>

namespace
{
namespace fs = std::filesystem;

class CTestUserData : public IUserDataContainer
{
	OBJECT_NOCOPY_METHODS( CTestUserData );

public:
	SUserData data;

	SUserData *Get() override
	{
		return &data;
	}

	void Load() override {}

	void Save() override {}
};

// Exercise GLTF extras and real database updates without opening the editor UI.
class LightEffectsExporter : public testing::Test
{
protected:
	fs::path root;
	CObj<NVFS::IVFS> savedVFS;
	CObj<NVFS::IFileCreator> savedCreator;
	CPtr<IManipulator> unit;
	CPtr<IManipulator> geometry;
	int sceneNumber = 0;

	void SetUp() override
	{
		ASSERT_EQ( GetCommandByAction( NDb::USER_ACTION_MOVE ), ACTION_COMMAND_MOVE_TO );
		root = fs::temp_directory_path() /
		       ( "obk2-lights-" + std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() ) );
		fs::create_directories( root );
		fs::copy_file( LIGHT_EFFECTS_SCHEMA, root / "types.xml" );
		std::ofstream( root / "unit.xdb" )
				<< R"xml(<MechUnitRPGStats><visualObject href="visual.xdb#xpointer(/VisObj)"/></MechUnitRPGStats>)xml";
		std::ofstream( root / "visual.xdb" )
				<< R"(<VisObj><Models><Item><Season>SEASON_SUMMER</Season>)"
				<< R"xml(<Model href="model.xdb#xpointer(/Model)"/></Item></Models></VisObj>)xml";
		std::ofstream( root / "model.xdb" )
				<< R"xml(<Model><Geometry href="geometry.xdb#xpointer(/Geometry)"/></Model>)xml";
		std::ofstream( root / "geometry.xdb" ) << "<Geometry/>";
		std::ofstream( root / "texture.xdb" ) << "<Texture/>";
		std::ofstream( root / "effect.xdb" ) << "<ComplexEffect/>";

		const std::string folder = root.generic_u8string() + "/";
		savedVFS = NVFS::GetMainVFS();
		savedCreator = NVFS::GetMainFileCreator();
		NVFS::SetMainVFS( NVFS::CreateWinVFS( folder ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( folder ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		for ( const char *name : { "geometry.xdb", "model.xdb", "visual.xdb", "unit.xdb", "texture.xdb", "effect.xdb" } )
		{
			ASSERT_TRUE( NDb::RegisterResourceFile( name ) );
		}
		IResourceManager::InitSingleton();
		auto *user = new CTestUserData;
		user->data.constUserData.szExportSourceFolder = folder;
		NSingleton::RegisterSingleton( user, IUserDataContainer::tidTypeID );
		unit = Singleton<IResourceManager>()->CreateObjectManipulator( "MechUnitRPGStats", std::string( "unit.xdb" ) );
		geometry = Singleton<IResourceManager>()->CreateObjectManipulator( "Geometry", std::string( "geometry.xdb" ) );
		ASSERT_NE( unit, nullptr );
		ASSERT_NE( geometry, nullptr );
	}

	void TearDown() override
	{
		unit = nullptr;
		geometry = nullptr;
		NSingleton::UnRegisterSingleton( IUserDataContainer::tidTypeID );
		IResourceManager::UninitSingleton();
		NDb::CloseDatabase();
		NVFS::SetMainVFS( savedVFS );
		NVFS::SetMainFileCreator( savedCreator );
		std::error_code error;
		fs::remove_all( root, error );
	}

	void Scene( std::initializer_list<std::pair<const char *, const char *>> lights )
	{
		// Use a fresh path when changing the scene so cache timestamps cannot hide edits.
		const std::string name = "lights" + std::to_string( sceneNumber++ ) + ".gltf";
		std::ofstream file( root / name );
		file << R"({"asset":{"version":"2.0"},"scene":0,"scenes":[{"nodes":[0]}],)"
				 << R"("nodes":[{"name":"Basis","children":[)";
		for ( size_t i = 0; i < lights.size(); ++i )
		{
			file << ( i ? "," : "" ) << i + 1;
		}
		file << "]}";
		for ( const auto &light : lights )
		{
			file << R"(,{"name":")" << light.first << R"(","extras":{)" << light.second << "}}";
		}
		file << "]}";
		file.close();
		ASSERT_TRUE( geometry->SetValue( "ModelFileRef", name ) );
	}

	void Import()
	{
		CHPObjectRPGStatsExporter::ExportSingleLightFX( unit );
	}

	template <class T>
	T Value( const char *field )
	{
		T result{};
		EXPECT_TRUE( CManipulatorManager::GetValue( &result, unit, field ) );
		return result;
	}

	void ExpectColour( int index, float r, float g, float b )
	{
		const std::string prefix = "LightEffects.[" + std::to_string( index ) + "].Colour.";
		EXPECT_FLOAT_EQ( Value<float>( ( prefix + "x" ).c_str() ), r );
		EXPECT_FLOAT_EQ( Value<float>( ( prefix + "y" ).c_str() ), g );
		EXPECT_FLOAT_EQ( Value<float>( ( prefix + "z" ).c_str() ), b );
	}
};

TEST_F( LightEffectsExporter, ImportsDirectSizesWithoutEnableFlagsAndDefaultsToNight )
{
	Scene( { { "LEffect01", R"("FlareSize":2,"PointLightSize":3,"ConeSize":4,"conelengthscale":5)" } } );
	Import();
	ASSERT_EQ( Value<int>( "LightEffects" ), 1 );
	EXPECT_FLOAT_EQ( Value<float>( "LightEffects.[0].FlareSize" ), 2 );
	EXPECT_FLOAT_EQ( Value<float>( "LightEffects.[0].PointLightSize" ), 3 );
	EXPECT_FLOAT_EQ( Value<float>( "LightEffects.[0].ConeSize" ), 4 );
	EXPECT_FLOAT_EQ( Value<float>( "LightEffects.[0].ConeLength" ), 5 );
	EXPECT_TRUE( Value<bool>( "LightEffects.[0].OnAtNight" ) );
	EXPECT_FALSE( Value<bool>( "LightEffects.[0].OnAtDay" ) );
	ExpectColour( 0, 1, 1, 1 );
}

TEST_F( LightEffectsExporter, DirectZeroAndFalseOverrideLegacyValues )
{
	Scene( { { "LEffect01",
	           R"("pointlightintensity":9,"POINTLIGHTSIZE":0,"conebasisscale":8,"ConeSize":0,)"
	           R"("atnight":true,"OnAtNight":false,"needcone":true,"needflare":true)" } } );
	Import();
	ASSERT_EQ( Value<int>( "LightEffects" ), 1 );
	EXPECT_FLOAT_EQ( Value<float>( "LightEffects.[0].PointLightSize" ), 0 );
	EXPECT_FLOAT_EQ( Value<float>( "LightEffects.[0].ConeSize" ), 0 );
	EXPECT_FLOAT_EQ( Value<float>( "LightEffects.[0].FlareSize" ), 0 );
	EXPECT_FALSE( Value<bool>( "LightEffects.[0].OnAtNight" ) );
}

TEST_F( LightEffectsExporter, RetainsLegacyAliasesButIgnoresObsoleteEnableFlags )
{
	Scene( { { "LEffect01",
	           R"("flaresize":2,"pointlightintensity":3,"conebasisscale":4,"conelengthscale":5,)"
	           R"("needflare":false,"needcone":false,"atday":true,"atnight":false)" },
	         { "LEffect02", "" } } );
	Import();
	ASSERT_EQ( Value<int>( "LightEffects" ), 2 );
	EXPECT_FLOAT_EQ( Value<float>( "LightEffects.[0].FlareSize" ), 2 );
	EXPECT_FLOAT_EQ( Value<float>( "LightEffects.[0].PointLightSize" ), 3 );
	EXPECT_FLOAT_EQ( Value<float>( "LightEffects.[0].ConeSize" ), 4 );
	EXPECT_FLOAT_EQ( Value<float>( "LightEffects.[0].ConeLength" ), 5 );
	EXPECT_TRUE( Value<bool>( "LightEffects.[0].OnAtDay" ) );
	EXPECT_FALSE( Value<bool>( "LightEffects.[0].OnAtNight" ) );
	EXPECT_FLOAT_EQ( Value<float>( "LightEffects.[1].PointLightSize" ), 0 );
	EXPECT_FLOAT_EQ( Value<float>( "LightEffects.[1].ConeSize" ), 0 );
	EXPECT_FLOAT_EQ( Value<float>( "LightEffects.[1].FlareSize" ), 0 );
}

TEST_F( LightEffectsExporter, MergesNewAndRemovedLocatorsWithoutLosingEditorFields )
{
	Scene( { { "LEffect01", "" }, { "LEffect02", "" }, { "LEffect04", "" } } );
	Import();
	ASSERT_EQ( Value<int>( "LightEffects" ), 3 );
	ASSERT_TRUE( CManipulatorManager::SetVec3( CVec3( 0.1f, 0.2f, 0.3f ), unit, "LightEffects.[1].Colour" ) );
	ASSERT_TRUE( unit->SetValue( "LightEffects.[1].FlareTexture", "texture.xdb" ) );
	ASSERT_TRUE( unit->SetValue( "LightEffects.[1].ConeTexture", "texture.xdb" ) );
	ASSERT_TRUE( unit->SetValue( "LightEffects.[1].AdditionalEffect", "effect.xdb" ) );

	Scene( { { "LEffect03", R"("PointLightSize":7)" }, { "LEffect02", R"("PointLightSize":6)" } } );
	for ( int pass = 0; pass < 2; ++pass )
	{
		Import();
		ASSERT_EQ( Value<int>( "LightEffects" ), 2 );
		EXPECT_EQ( Value<std::string>( "LightEffects.[0].LocatorName" ), "LEffect02" );
		EXPECT_EQ( Value<std::string>( "LightEffects.[1].LocatorName" ), "LEffect03" );
		EXPECT_FLOAT_EQ( Value<float>( "LightEffects.[0].PointLightSize" ), 6 );
		EXPECT_FLOAT_EQ( Value<float>( "LightEffects.[1].PointLightSize" ), 7 );
		ExpectColour( 0, 0.1f, 0.2f, 0.3f );
		EXPECT_EQ( CDBID( Value<std::string>( "LightEffects.[0].FlareTexture" ) ), CDBID( "texture.xdb" ) );
		EXPECT_EQ( CDBID( Value<std::string>( "LightEffects.[0].ConeTexture" ) ), CDBID( "texture.xdb" ) );
		EXPECT_EQ( CDBID( Value<std::string>( "LightEffects.[0].AdditionalEffect" ) ), CDBID( "effect.xdb" ) );
	}
	Scene( {} );
	Import();
	EXPECT_EQ( Value<int>( "LightEffects" ), 0 );
}

TEST_F( LightEffectsExporter, PreservesBlackAndNormalizesNonzeroColourOnCreateAndUpdate )
{
	Scene( { { "LEffect01", R"("pointlightr":0,"pointlightg":0,"pointlightb":0)" } } );
	std::feclearexcept( FE_ALL_EXCEPT );
	Import();
	EXPECT_EQ( std::fetestexcept( FE_INVALID | FE_DIVBYZERO ), 0 );
	ASSERT_EQ( Value<int>( "LightEffects" ), 1 );
	ExpectColour( 0, 0, 0, 0 );
	Scene( { { "LEffect01", R"("pointlightr":1,"pointlightg":0.5,"pointlightb":0)" } } );
	Import();
	ExpectColour( 0, 1, 0.5f, 0 );
	Scene( { { "LEffect01", R"("pointlightr":-100,"pointlightg":0,"pointlightb":-2)" } } );
	std::feclearexcept( FE_ALL_EXCEPT );
	Import();
	EXPECT_EQ( std::fetestexcept( FE_INVALID | FE_DIVBYZERO ), 0 );
	ExpectColour( 0, 0, 0, 0 );
}
} // namespace
