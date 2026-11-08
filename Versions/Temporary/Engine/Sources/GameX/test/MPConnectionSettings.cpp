#include "../stdafx.h"

#include "../DBMPConsts.h"
#include "../MPConnectionSettings.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "System/GlobalVars.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
class MPConnectionSettings : public testing::Test
{
	static std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;

protected:
	NDb::SMultiplayerConsts *properties = nullptr;

	static void SetUpTestSuite()
	{
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-mp-connection-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( MP_CONNECTION_TYPES_FILE, directory / "types.xml" );
		std::ofstream( directory / "legacy.xdb" ) << "<MultiplayerConsts/>";
		std::ofstream( directory / "endpoint.xdb" )
				<< "<MultiplayerConsts><NivalNetIP>127.0.0.1</NivalNetIP><NivalNetPort>4300</NivalNetPort>"
					 "<NetGameVersion>7</NetGameVersion></MultiplayerConsts>";
		const std::string root = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( root ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( root ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
		ASSERT_TRUE( NDb::RegisterResourceFile( "legacy.xdb" ) );
		ASSERT_TRUE( NDb::RegisterResourceFile( "endpoint.xdb" ) );
	}

	static void TearDownTestSuite()
	{
		NDb::CloseDatabase();
		NVFS::SetMainVFS( nullptr );
		NVFS::SetMainFileCreator( nullptr );
		std::error_code error;
		std::filesystem::remove_all( directory, error );
	}

	NDb::SMultiplayerConsts *CreateRecord()
	{
		CObj<NDb::IObjMan> record = NDb::CreateNewObject( "MultiplayerConsts" );
		if ( !record )
		{
			ADD_FAILURE() << "MultiplayerConsts factory not registered";
			return nullptr;
		}
		records.push_back( record );
		return dynamic_cast<NDb::SMultiplayerConsts *>( record->GetObject() );
	}

	void SetUp() override
	{
		NGlobal::SetVar( "NivalNetIP", "profile.example.invalid" );
		NGlobal::SetVar( "NivalNetPort", 4400 );
		NGlobal::SetVar( "NetGameVersion", 9 );
		properties = CreateRecord();
		ASSERT_NE( properties, nullptr );
	}

	void TearDown() override
	{
		NGlobal::RemoveVar( "NivalNetIP" );
		NGlobal::RemoveVar( "NivalNetPort" );
		NGlobal::RemoveVar( "NetGameVersion" );
	}

	static void ExpectSettings( const NDb::SMultiplayerConsts *properties, const char *address, int port, int version )
	{
		const auto settings = NGameX::GetMPConnectionSettings( properties );
		EXPECT_EQ( settings.szServerAddress, address );
		EXPECT_EQ( settings.nServerPort, port );
		EXPECT_EQ( settings.nGameVersion, version );
		// LAN resolves the same protocol identifier without the online endpoint.
		EXPECT_EQ( NGameX::GetMPGameVersion( properties ), version );
	}
};

std::filesystem::path MPConnectionSettings::directory;

TEST_F( MPConnectionSettings, NoResourceOrFieldsKeepLegacySettingsAndDefaults )
{
	ExpectSettings( nullptr, "profile.example.invalid", 4400, 9 );
	ExpectSettings( properties, "profile.example.invalid", 4400, 9 );
	NGlobal::RemoveVar( "NivalNetIP" );
	NGlobal::RemoveVar( "NivalNetPort" );
	NGlobal::RemoveVar( "NetGameVersion" );
	ExpectSettings( nullptr, "localhost", 4200, 1 );
	ExpectSettings( properties, "localhost", 4200, 1 );
}

TEST_F( MPConnectionSettings, LegacyAndAuthoredXmlResolveTheCorrectSource )
{
	const auto *legacy = NDb::Get<NDb::SMultiplayerConsts>( CDBID( "legacy.xdb" ) );
	const auto *endpoint = NDb::Get<NDb::SMultiplayerConsts>( CDBID( "endpoint.xdb" ) );
	ASSERT_NE( legacy, nullptr );
	ASSERT_NE( endpoint, nullptr );
	EXPECT_TRUE( legacy->szNivalNetIP.empty() );
	EXPECT_EQ( legacy->nNivalNetPort, 0 );
	EXPECT_EQ( legacy->nNetGameVersion, -1 );
	ExpectSettings( legacy, "profile.example.invalid", 4400, 9 );
	ExpectSettings( endpoint, "127.0.0.1", 4300, 7 );

	NGlobal::SetVar( "NivalNetIP", "changed.example.invalid" );
	NGlobal::SetVar( "NivalNetPort", 4500 );
	NGlobal::SetVar( "NetGameVersion", 10 );
	ExpectSettings( endpoint, "127.0.0.1", 4300, 7 );
	// Resolving a different mod must not retain the previous endpoint or change its profile.
	ExpectSettings( legacy, "changed.example.invalid", 4500, 10 );
}

TEST_F( MPConnectionSettings, EachUnsetFieldFallsBackIndependentlyAndVersionZeroIsExplicit )
{
	properties->szNivalNetIP = "data.example.invalid";
	ExpectSettings( properties, "data.example.invalid", 4400, 9 );
	properties->nNivalNetPort = 4600;
	ExpectSettings( properties, "data.example.invalid", 4600, 9 );
	properties->nNetGameVersion = 0;
	ExpectSettings( properties, "data.example.invalid", 4600, 0 );
	properties->szNivalNetIP.clear();
	properties->nNivalNetPort = 0;
	ExpectSettings( properties, "profile.example.invalid", 4400, 0 );
}

TEST_F( MPConnectionSettings, OlderBinaryRecordsKeepTheVersionFallbackSentinel )
{
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		// A pre-extension record has only the old pause-time chunk here.
		int pause = 120;
		saver->Add( 9, &pause );
	}
	stream.Seek( 0 );
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		properties->operator&( *saver );
	}
	EXPECT_EQ( properties->nNetGameVersion, -1 );
	ExpectSettings( properties, "profile.example.invalid", 4400, 9 );
}

TEST_F( MPConnectionSettings, BinaryRoundTripPreservesConnectionFieldsWithoutChangingGameplayChecksum )
{
	const uint32_t checksum = properties->CalcCheckSum();
	properties->szNivalNetIP = "127.0.0.1";
	properties->nNivalNetPort = 4700;
	properties->nNetGameVersion = 11;
	EXPECT_EQ( properties->CalcCheckSum(), checksum );
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		properties->operator&( *saver );
	}
	stream.Seek( 0 );
	auto *loaded = CreateRecord();
	ASSERT_NE( loaded, nullptr );
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		loaded->operator&( *saver );
	}
	ExpectSettings( loaded, "127.0.0.1", 4700, 11 );
}
} // namespace
