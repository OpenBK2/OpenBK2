#include "MapEditorLib/stdafx.h"

#include "MapEditorLib/Tools_Resources.h"
#include "System/GeneralSettings.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <gtest/gtest.h>

namespace
{
namespace fs = std::filesystem;

struct SValue
{
	int nValue = 0;

	int operator&( IXmlSaver &saver )
	{
		saver.Add( "Value", &nValue );
		return 0;
	}
};

void Write( const fs::path &path, const std::string &text )
{
	fs::create_directories( path.parent_path() );
	std::ofstream stream( path, std::ios::binary );
	stream << text;
	ASSERT_TRUE( stream.good() );
}

std::string Read( const fs::path &path )
{
	std::ifstream stream( path, std::ios::binary );
	return { std::istreambuf_iterator<char>( stream ), std::istreambuf_iterator<char>() };
}

class CEditorLocalDataTest : public testing::Test
{
protected:
	fs::path root =
			fs::temp_directory_path() / ( "OpenBK2-editor-" + boost::uuids::to_string( boost::uuids::random_generator()() ) );
	fs::path install = root / "installation";
	fs::path user = root / fs::u8path( u8"user space \u0107\u03bb" );
	std::string error;

	void SetUp() override
	{
		Write( install / "Profiles/startup.cfg", "// defaults\n" );
		Write( install / "Profiles/default_profile/user.cfg", "// defaults\n" );
		Write( install / "Profiles/default_profile/input.cfg", "// defaults\n" );
		Write( install / "Editor/UserData.xml", "<Base><Settings><Value>12</Value></Settings></Base>" );
		const NSettings::CGeneralSettings settings( install.u8string(), ( user / "Profiles" ).u8string() );
		ASSERT_TRUE( NSettings::InitializeGeneralSettings( settings, &error ) ) << error;
		ASSERT_TRUE( settings.PrepareEditor( &error ) ) << error;
	}

	void TearDown() override
	{
		// The fixture owns this uniquely named temporary tree.
		std::error_code ignored;
		fs::remove_all( root, ignored );
	}
};
} // namespace

TEST_F( CEditorLocalDataTest, LegacyXmlPathsReadAndWriteOnlyTheUserCopy )
{
	const auto original = Read( install / "Editor/UserData.xml" );
	const auto legacy = ( install / "Editor/UserData.xml" ).u8string();
	SValue value;
	ASSERT_TRUE( LoadXMLResource( legacy, "Settings", value ) );
	EXPECT_EQ( 12, value.nValue );
	value.nValue = 37;
	ASSERT_TRUE( SaveXMLResource( legacy, "Settings", value ) );
	value.nValue = 0;
	ASSERT_TRUE( LoadXMLResource( legacy, "Settings", value ) );
	EXPECT_EQ( 37, value.nValue );
	EXPECT_EQ( original, Read( install / "Editor/UserData.xml" ) );
	EXPECT_NE( original, Read( user / "Editor/UserData.xml" ) );

	// Clearing/deleting local settings must not resurrect installation defaults.
	fs::remove( user / "Editor/UserData.xml" );
	EXPECT_FALSE( LoadXMLResource( legacy, "Settings", value ) );
}

TEST_F( CEditorLocalDataTest, CacheCanBeWrittenAgainAfterClearing )
{
	const auto legacy = ( install / "Editor/IconCache/Units/tank.xdb" ).u8string();
	SValue value;
	value.nValue = 41;
	ASSERT_TRUE( SaveXMLResource( legacy, "Icon", value ) );
	ASSERT_TRUE( NSettings::GetGeneralSettings()->ClearEditorIconCache( &error ) ) << error;
	EXPECT_FALSE( fs::exists( user / "Editor/IconCache" ) );
	value.nValue = 42;
	ASSERT_TRUE( SaveXMLResource( legacy, "Icon", value ) );
	value.nValue = 0;
	ASSERT_TRUE( LoadXMLResource( legacy, "Icon", value ) );
	EXPECT_EQ( 42, value.nValue );
	EXPECT_FALSE( fs::exists( install / "Editor/IconCache" ) );
}
