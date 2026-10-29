#include "System/BinSaver.h"
#include "System/GeneralSettings.h"
#include "System/GlobalVars.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <gtest/gtest.h>

namespace
{

namespace fs = std::filesystem;

void Write( const fs::path &path, const std::string &text )
{
	fs::create_directories( path.parent_path() );
	std::ofstream file( path, std::ios::binary );
	file << text;
	ASSERT_TRUE( file.good() );
}

std::string Read( const fs::path &path )
{
	std::ifstream file( path, std::ios::binary );
	return { std::istreambuf_iterator<char>( file ), std::istreambuf_iterator<char>() };
}

void MakeProfiles( const fs::path &path )
{
	Write( path / "startup.cfg", "// defaults\n" );
	Write( path / "default_profile/user.cfg", "// user defaults\n" );
	Write( path / "default_profile/input.cfg", "// input defaults\n" );
}

class CGeneralSettingsTest : public testing::Test
{
protected:
	fs::path root = fs::temp_directory_path() /
	                ( "OpenBK2-settings-" + boost::uuids::to_string( boost::uuids::random_generator()() ) );
	fs::path install = root / "installation";
	fs::path profiles = root / fs::u8path( u8"user space \u0107\u03bb" ) / "Profiles";
	NSettings::CGeneralSettings settings{ install.u8string(), profiles.u8string() };
	std::string error;

	void TearDown() override
	{
		// root is a unique temporary directory owned by this fixture.
		std::error_code ignored;
		fs::remove_all( root, ignored );
	}
};

TEST_F( CGeneralSettingsTest, CopiesDefaultsOnceAndKeepsTheInstallationUntouched )
{
	MakeProfiles( install / "Profiles" );
	Write( install / "Profiles/player/Saves/campaign.sav", "save bytes" );
	Write( install / "Profiles/player/Replays/match.replay", "replay bytes" );
	ASSERT_TRUE( settings.PrepareProfiles( &error ) ) << error;
	EXPECT_EQ( "save bytes", Read( profiles / "player/Saves/campaign.sav" ) );
	EXPECT_EQ( "replay bytes", Read( profiles / "player/Replays/match.replay" ) );
	Write( profiles / "startup.cfg", "user changed this\n" );
	Write( install / "Profiles/added-later.cfg", "do not merge" );
	ASSERT_TRUE( settings.PrepareProfiles( &error ) ) << error;
	EXPECT_EQ( "user changed this\n", Read( profiles / "startup.cfg" ) );
	EXPECT_EQ( "// defaults\n", Read( install / "Profiles/startup.cfg" ) );
	EXPECT_FALSE( fs::exists( profiles / "added-later.cfg" ) );
}

TEST_F( CGeneralSettingsTest, ExistingUserProfilesDoNotRequireTheInstallationTemplate )
{
	MakeProfiles( profiles );
	ASSERT_TRUE( settings.PrepareProfiles( &error ) ) << error;
	EXPECT_FALSE( fs::exists( install ) );
}

TEST_F( CGeneralSettingsTest, EditorCopiesTheWholeFolderOnceBesideProfiles )
{
	const auto editor = profiles.parent_path() / "Editor";
	Write( install / "Editor/UserData.xml", "editor defaults" );
	Write( install / "Editor/ResizeDialogStyles/Map.xml", "placement" );
	Write( install / "Editor/IconCache/Units/example.xdb", "cached icon" );
	ASSERT_TRUE( settings.PrepareEditor( &error ) ) << error;
	EXPECT_EQ( editor.u8string(), settings.GetEditorRoot() );
	EXPECT_EQ( "placement", Read( editor / "ResizeDialogStyles/Map.xml" ) );
	EXPECT_EQ( "cached icon", Read( editor / "IconCache/Units/example.xdb" ) );
	Write( editor / "UserData.xml", "user settings" );
	Write( install / "Editor/new-default.xml", "do not merge" );
	ASSERT_TRUE( settings.PrepareEditor( &error ) ) << error;
	EXPECT_EQ( "user settings", Read( editor / "UserData.xml" ) );
	EXPECT_EQ( "editor defaults", Read( install / "Editor/UserData.xml" ) );
	EXPECT_FALSE( fs::exists( editor / "new-default.xml" ) );
	EXPECT_FALSE( fs::exists( profiles ) );
}

TEST_F( CGeneralSettingsTest, ExistingEditorWorksWithoutAnInstallationTemplate )
{
	const auto editor = profiles.parent_path() / "Editor";
	Write( editor / "UserData.xml", "keep this" );
	ASSERT_TRUE( settings.PrepareEditor( &error ) ) << error;
	EXPECT_EQ( "keep this", Read( editor / "UserData.xml" ) );
	EXPECT_FALSE( fs::exists( install ) );
}

TEST_F( CGeneralSettingsTest, MissingEditorReportsBothLocations )
{
	EXPECT_FALSE( settings.PrepareEditor( &error ) );
	EXPECT_NE( std::string::npos, error.find( "No Editor folder found" ) );
	EXPECT_NE( std::string::npos, error.find( settings.GetEditorRoot() ) );
	EXPECT_NE( std::string::npos, error.find( ( install / "Editor" ).u8string() ) );
	EXPECT_FALSE( fs::exists( profiles.parent_path() / "Editor" ) );
}

TEST_F( CGeneralSettingsTest, EditorRejectsBlockedOrInstallationDestinations )
{
	Write( install / "Editor/UserData.xml", "defaults" );
	Write( profiles.parent_path() / "Editor", "a file" );
	EXPECT_FALSE( settings.PrepareEditor( &error ) );
	EXPECT_EQ( "a file", Read( profiles.parent_path() / "Editor" ) );
	const NSettings::CGeneralSettings inInstall( install.u8string(), ( install / "Profiles" ).u8string() );
	EXPECT_FALSE( inInstall.PrepareEditor( &error ) );
	EXPECT_FALSE( inInstall.ClearEditorIconCache( &error ) );
	EXPECT_EQ( "defaults", Read( install / "Editor/UserData.xml" ) );
}

TEST_F( CGeneralSettingsTest, EditorResolvesLegacyPathsWithoutFallingBackToDefaults )
{
	const auto editor = profiles.parent_path() / "Editor";
	Write( editor / "BitFields/AIActions.xml", "user actions" );
	Write( install / "Editor/obsolete.xml", "installation only" );
	for ( const auto &name : { std::string( "../editor/bitfields/aiactions.xml" ),
	                           std::string( "Editor\\BitFields\\AIActions.xml" ),
	                           ( install / "Editor/BitFields/AIActions.xml" ).u8string(),
	                           ( editor / "bitfields/aiactions.xml" ).u8string() } )
	{
		SCOPED_TRACE( name );
		const auto resolved = fs::u8path( settings.ResolveEditorPath( name ) );
		EXPECT_TRUE( resolved.is_absolute() );
		EXPECT_EQ( "user actions", Read( resolved ) );
	}
	EXPECT_FALSE( fs::exists( fs::u8path( settings.ResolveEditorPath( "Editor/obsolete.xml" ) ) ) );
	const auto data = ( install / "Data/Editor/EditorOptions.xdb" ).u8string();
	EXPECT_EQ( data, settings.ResolveEditorPath( data ) );
	EXPECT_EQ( "Textures/icon.dds", settings.ResolveEditorPath( "Textures/icon.dds" ) );
}

TEST_F( CGeneralSettingsTest, ClearingIconsDeletesOnlyTheUserCacheAndIsRepeatable )
{
	const auto editor = profiles.parent_path() / "Editor";
	Write( install / "Editor/IconCache/Units/example.xdb", "installed icon" );
	Write( editor / "IconCache/Units/example.xdb", "user icon" );
	Write( editor / "IconCache/Buildings/example.xdb", "another icon" );
	Write( editor / "UserData.xml", "keep settings" );
	Write( profiles / "player/Saves/map.sav", "keep save" );
	ASSERT_TRUE( settings.ClearEditorIconCache( &error ) ) << error;
	EXPECT_FALSE( fs::exists( editor / "IconCache" ) );
	EXPECT_EQ( "installed icon", Read( install / "Editor/IconCache/Units/example.xdb" ) );
	EXPECT_EQ( "keep settings", Read( editor / "UserData.xml" ) );
	EXPECT_EQ( "keep save", Read( profiles / "player/Saves/map.sav" ) );
	ASSERT_TRUE( settings.ClearEditorIconCache( &error ) ) << error;
	ASSERT_TRUE( settings.PrepareEditor( &error ) ) << error;
	EXPECT_FALSE( fs::exists( editor / "IconCache" ) );
}

TEST_F( CGeneralSettingsTest, ClearingIconsDoesNotFollowALinkOutsideTheEditorFolder )
{
	const auto editor = profiles.parent_path() / "Editor";
	Write( root / "outside/keep.txt", "keep this" );
	fs::create_directories( editor );
	std::error_code linkError;
	fs::create_directory_symlink( root / "outside", editor / "IconCache", linkError );
	if ( linkError )
	{
		GTEST_SKIP() << "Directory symlinks unavailable: " << linkError.message();
	}
	EXPECT_FALSE( settings.ClearEditorIconCache( &error ) );
	EXPECT_EQ( "keep this", Read( root / "outside/keep.txt" ) );
}

TEST_F( CGeneralSettingsTest, MissingBothReportsBothLocationsWithoutCreatingProfiles )
{
	EXPECT_FALSE( settings.PrepareProfiles( &error ) );
	EXPECT_NE( std::string::npos, error.find( "No Profiles folder found" ) );
	EXPECT_NE( std::string::npos, error.find( profiles.u8string() ) );
	EXPECT_FALSE( fs::exists( profiles ) );
}

TEST_F( CGeneralSettingsTest, InvalidExistingUserDirectoryIsNotOverwritten )
{
	MakeProfiles( install / "Profiles" );
	Write( profiles / "keep-me.sav", "precious save" );
	EXPECT_FALSE( settings.PrepareProfiles( &error ) );
	EXPECT_EQ( "precious save", Read( profiles / "keep-me.sav" ) );
	EXPECT_FALSE( fs::exists( profiles / "startup.cfg" ) );
}

TEST_F( CGeneralSettingsTest, DestinationCreationFailureDoesNotChangeTheTemplate )
{
	MakeProfiles( install / "Profiles" );
	Write( profiles.parent_path(), "a file blocks the destination" );
	EXPECT_FALSE( settings.PrepareProfiles( &error ) );
	EXPECT_EQ( "// defaults\n", Read( install / "Profiles/startup.cfg" ) );
}

TEST_F( CGeneralSettingsTest, InterruptedStagingDirectoryIsNeverUsedAsProfiles )
{
	MakeProfiles( install / "Profiles" );
	Write( profiles.parent_path() / ".Profiles-import-interrupted/startup.cfg", "partial" );
	ASSERT_TRUE( settings.PrepareProfiles( &error ) ) << error;
	EXPECT_EQ( "// defaults\n", Read( profiles / "startup.cfg" ) );
}

TEST_F( CGeneralSettingsTest, InstallationCannotBeSelectedAsTheWritableProfiles )
{
	MakeProfiles( install / "Profiles" );
	NSettings::CGeneralSettings invalid( install.u8string(), ( install / "Profiles" ).u8string() );
	EXPECT_FALSE( invalid.PrepareProfiles( &error ) );
}

TEST_F( CGeneralSettingsTest, ConcurrentInitializersSeeACompleteCopy )
{
	MakeProfiles( install / "Profiles" );
	bool first = false;
	bool second = false;
	std::string secondError;
	std::thread a( [&]() { first = settings.PrepareProfiles( &error ); } );
	std::thread b( [&]() { second = settings.PrepareProfiles( &secondError ); } );
	a.join();
	b.join();
	EXPECT_TRUE( first ) << error;
	EXPECT_TRUE( second ) << secondError;
	EXPECT_TRUE( fs::is_regular_file( profiles / "default_profile/user.cfg" ) );
}

TEST_F( CGeneralSettingsTest, LegacyPathsAlwaysReadUserProfilesAndPreserveResourceIDs )
{
	MakeProfiles( install / "Profiles" );
	ASSERT_TRUE( settings.PrepareProfiles( &error ) ) << error;
	Write( install / "Profiles/only-in-install.cfg", "stale settings" );
	for ( const char *path : { "Profiles/startup.cfg", "../Profiles/startup.cfg", ".\\profiles\\startup.cfg" } )
	{
		EXPECT_EQ( profiles / "startup.cfg", fs::u8path( settings.ResolveConfigPath( path ) ) );
	}
	EXPECT_EQ( profiles / "startup.cfg",
	           fs::u8path( settings.ResolveConfigPath( ( install / "Profiles/startup.cfg" ).u8string() ) ) );
	EXPECT_EQ( profiles / "only-in-install.cfg",
	           fs::u8path( settings.ResolveConfigPath( "Profiles/only-in-install.cfg" ) ) );
	EXPECT_EQ( "GameRoot.xdb", settings.ResolveResourceID( "../Data/GameRoot.xdb" ) );
	EXPECT_EQ( "GameRoot.xdb", settings.ResolveResourceID( "..\\data\\GameRoot.xdb" ) );
	EXPECT_EQ( "Consts/Game.xdb", settings.ResolveResourceID( "Consts/Game.xdb" ) );
}

TEST_F( CGeneralSettingsTest, ConfigPathsResolveMixedCaseFromTheirFilesystemRoot )
{
	const auto config = profiles / "Player/Nested/Settings.cfg";
	Write( config, "user settings" );
	Write( install / "Profiles/Player/Nested/Settings.cfg", "installation defaults" );
	// Legacy and absolute paths must resolve case within the user tree on Linux.
	for ( const auto &name : { std::string( "../profiles/player/nested/settings.cfg" ),
	                           ( install / "Profiles/player/nested/settings.cfg" ).u8string(),
	                           ( profiles / "player/nested/settings.cfg" ).u8string() } )
	{
		SCOPED_TRACE( name );
		const auto resolved = fs::u8path( settings.ResolveConfigPath( name ) );
		EXPECT_TRUE( resolved.is_absolute() );
		EXPECT_EQ( "user settings", Read( resolved ) );
	}
	// A nested exec uses the same case resolution relative to its including file.
	EXPECT_EQ( "user settings",
	           Read( fs::u8path( settings.ResolveConfigPath( "nested/settings.cfg",
	                                                         ( profiles / "Player/startup.cfg" ).u8string() ) ) ) );
}

TEST_F( CGeneralSettingsTest, NestedExecRestoresStorageAndNeverSerializesCredentials )
{
	MakeProfiles( install / "Profiles" );
	Write( install / "Profiles/startup.cfg", "exec .\\profiles\\nested\\child.cfg\nsetvar settings_test_after = 42\n" );
	Write( install / "Profiles/nested/child.cfg", "exec sibling.cfg\nexec missing.cfg\nexec ../startup.cfg\n" );
	Write( install / "Profiles/nested/sibling.cfg", "setvar settings_test_nested = 7\n" );
	ASSERT_TRUE( NSettings::InitializeGeneralSettings( settings, &error ) ) << error;
	NGlobal::LoadConfig( "../Profiles/startup.cfg", STORAGE_USER );
	EXPECT_EQ( 42.0f, NGlobal::GetVar( "settings_test_after" ).GetFloat() );
	EXPECT_EQ( 7.0f, NGlobal::GetVar( "settings_test_nested" ).GetFloat() );
	NGlobal::SetVar( "Multiplayer.NivalNet.StoredLogin", L"private account", STORAGE_USER );
	NGlobal::SetVar( "Multiplayer.NivalNet.StoredPassword", L"private password", STORAGE_USER );
	NGlobal::SetVar( "Multiplayer.PlayerName", L"private account", STORAGE_USER );
	NGlobal::SaveAllVars( ( profiles / "global.cfg" ).u8string(), ( profiles / "user.cfg" ).u8string() );
	const auto saved = Read( profiles / "user.cfg" );
	EXPECT_NE( std::string::npos, saved.find( "settings_test_after = 42" ) );
	EXPECT_NE( std::string::npos, saved.find( "settings_test_nested = 7" ) );
	EXPECT_EQ( std::string::npos, saved.find( "private" ) );
	EXPECT_EQ( std::string::npos, saved.find( "StoredPassword" ) );
	EXPECT_EQ( std::string::npos, saved.find( "StoredLogin" ) );
	NGlobal::RemoveVar( "settings_test_after" );
	NGlobal::RemoveVar( "settings_test_nested" );
}

} // namespace
