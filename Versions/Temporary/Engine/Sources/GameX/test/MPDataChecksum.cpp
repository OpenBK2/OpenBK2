#include "../stdafx.h"

#include "AILogic/DBAIConsts.h"
#include "GameX/CoopScriptChecksum.h"
#include "GameX/DBConsts.h"
#include "GameX/DBMPConsts.h"
#include "GameX/GetConsts.h"
#include "libdb/Db.h"
#include "libdb/EditorDb.h"
#include "libdb/ObjMan.h"
#include "Main/DBNetConsts.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace
{
// Use the real schema and checksum implementation, but no installed game data
// or graphics/audio initialization. The editor DB constructs loaded records.
class MPDataChecksum : public testing::Test
{
protected:
	static std::filesystem::path directory;
	std::vector<CObj<NDb::IObjMan>> records;
	NDb::SGameConsts *pConsts = nullptr;
	NDb::SNetGameConsts *pNet = nullptr;
	NDb::SAIGameConsts *pAI = nullptr;

	static void SetUpTestSuite()
	{
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		directory = std::filesystem::temp_directory_path() / ( "obk2-mp-checksum-" + std::to_string( stamp ) );
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::filesystem::copy_file( MP_CHECKSUM_TYPES_FILE, directory / "types.xml" );
		const std::string path = directory.generic_string() + "/";
		NVFS::SetMainVFS( NVFS::CreateWinVFS( path ) );
		NVFS::SetMainFileCreator( NVFS::CreateWinFileCreator( path ) );
		ASSERT_TRUE( NDb::OpenDatabase( NVFS::GetMainVFS(), NVFS::GetMainFileCreator(), NDb::DATABASE_MODE_EDITOR ) );
	}

	static void TearDownTestSuite()
	{
		NDb::CloseDatabase();
		std::error_code error;
		std::filesystem::remove_all( directory, error );
	}

	template <class T>
	T *CreateRecord( const char *name )
	{
		CObj<NDb::IObjMan> record = NDb::CreateNewObject( name );
		EXPECT_NE( record, nullptr ) << name;
		if ( !record )
		{
			return nullptr;
		}
		T *object = dynamic_cast<T *>( record->GetObject() );
		EXPECT_NE( object, nullptr ) << name;
		records.push_back( record );
		return object;
	}

	void SetUp() override
	{
		NGlobal::SetVar( "code_version_number", "1.2.3.4" );
		NGlobal::SetVar( "MOD.RelativePath", "" );
		pConsts = CreateRecord<NDb::SGameConsts>( "GameConsts" );
		pNet = CreateRecord<NDb::SNetGameConsts>( "NetGameConsts" );
		pAI = CreateRecord<NDb::SAIGameConsts>( "AIGameConsts" );
		auto *pMP = CreateRecord<NDb::SMultiplayerConsts>( "MultiplayerConsts" );
		ASSERT_NE( pConsts, nullptr );
		ASSERT_NE( pNet, nullptr );
		ASSERT_NE( pAI, nullptr );
		ASSERT_NE( pMP, nullptr );
		pConsts->pAI = pAI;
		pConsts->pNet = pNet;
		pConsts->pMultiplayer = pMP;

		// Give the actual startup/mod refresh function a current game-consts record.
		static unsigned int nextID = 0;
		const std::string path = "consts-" + std::to_string( nextID++ ) + ".xdb";
		ASSERT_TRUE( NDb::AddNewObject( path, CDBID( path ), records.front() ) );
		NGlobal::SetVar( "game_consts", path );
	}

	void WriteScript( const std::string &filename, const std::string &contents )
	{
		std::ofstream stream( directory / filename, std::ios::binary | std::ios::trunc );
		ASSERT_TRUE( stream.is_open() );
		stream.write( contents.data(), std::streamsize( contents.size() ) );
		stream.close();
		ASSERT_TRUE( stream.good() );
	}
};

std::filesystem::path MPDataChecksum::directory;

TEST_F( MPDataChecksum, OlderBinaryMapKeepsClassicDefaultsAndUnrestrictedTechnology )
{
	auto *map = CreateRecord<NDb::SMultiplayerMap>( "MultiplayerMap" );
	ASSERT_NE( map, nullptr );
	CMemoryStream stream;
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		// Only the pre-COOP scalar chunks are present in this legacy record.
		int nSizeX = 64;
		int nSizeY = 96;
		int nPlayers = 4;
		saver->Add( 4, &nSizeX );
		saver->Add( 5, &nSizeY );
		saver->Add( 6, &nPlayers );
	}
	stream.Seek( 0 );
	{
		CPtr<IBinSaver> saver = CreateBinSaver( &stream, SAVER_MODE_READ );
		map->operator&( *saver );
	}
	EXPECT_EQ( map->nSizeX, 64 );
	EXPECT_EQ( map->nSizeY, 96 );
	EXPECT_EQ( map->nPlayers, 4 );
	EXPECT_EQ( map->eGameType, NDb::CLASSIC );
	EXPECT_EQ( map->nForcedTechLevel, -1 );
	EXPECT_TRUE( map->allowedNations.empty() );
}

TEST_F( MPDataChecksum, MenuReadsReuseSnapshotUntilTheNextAttempt )
{
	NGameX::InitializeMPDataVersionChecksum();
	const uint32_t initial = pConsts->GetMPDataVersionChecksum();
	++pNet->nMaxLatency;
	// A fresh traversal would see this edit: repeated menu reads must not do one.
	for ( int i = 0; i < 100; ++i )
	{
		EXPECT_EQ( initial, pConsts->GetMPDataVersionChecksum() );
	}
	const uint32_t refreshed = pConsts->RecalculateMPDataVersionChecksum();
	EXPECT_NE( initial, refreshed );
	EXPECT_EQ( refreshed, pConsts->GetMPDataVersionChecksum() );
}

TEST_F( MPDataChecksum, StartupAndModRefreshUseTheCurrentConfig )
{
	const uint32_t early = pConsts->GetMPDataVersionChecksum();
	NGlobal::SetVar( "code_version_number", "1.2.3.5" );
	NGameX::InitializeMPDataVersionChecksum();
	const uint32_t vanilla = pConsts->GetMPDataVersionChecksum();
	EXPECT_NE( early, vanilla );

	NGlobal::SetVar( "MOD.RelativePath", "test-mod" );
	NGameX::InitializeMPDataVersionChecksum();
	EXPECT_NE( vanilla, pConsts->GetMPDataVersionChecksum() );
	// Manual unload and campaign/tutorial selection share this refresh path.
	NGlobal::SetVar( "MOD.RelativePath", "" );
	NGameX::InitializeMPDataVersionChecksum();
	EXPECT_EQ( vanilla, pConsts->GetMPDataVersionChecksum() );
}

TEST_F( MPDataChecksum, NewDatabaseRecordStartsWithAnEmptySnapshot )
{
	const uint32_t initial = pConsts->GetMPDataVersionChecksum();
	++pNet->nMaxLatency;
	auto *pReloaded = CreateRecord<NDb::SGameConsts>( "GameConsts" );
	ASSERT_NE( pReloaded, nullptr );
	pReloaded->pAI = pConsts->pAI;
	pReloaded->pNet = pConsts->pNet;
	pReloaded->pMultiplayer = pConsts->pMultiplayer;
	EXPECT_NE( initial, pReloaded->GetMPDataVersionChecksum() );
	EXPECT_EQ( initial, pConsts->GetMPDataVersionChecksum() );
}

TEST_F( MPDataChecksum, MapIsHashedOncePerAttemptAndAgainWhenItChanges )
{
	auto *pFirst = CreateRecord<NDb::SMultiplayerMap>( "MultiplayerMap" );
	auto *pSecond = CreateRecord<NDb::SMultiplayerMap>( "MultiplayerMap" );
	ASSERT_NE( pFirst, nullptr );
	ASSERT_NE( pSecond, nullptr );
	pFirst->nPlayers = 2;
	pSecond->nPlayers = 4;
	const uint32_t initial = pConsts->GetMPDataVersionChecksumWithMap( pFirst );
	pFirst->nPlayers = 3;
	for ( int i = 0; i < 100; ++i )
	{
		EXPECT_EQ( initial, pConsts->GetMPDataVersionChecksumWithMap( pFirst ) );
	}
	pConsts->RecalculateMPDataVersionChecksum();
	const uint32_t refreshed = pConsts->GetMPDataVersionChecksumWithMap( pFirst );
	EXPECT_NE( initial, refreshed );
	EXPECT_NE( refreshed, pConsts->GetMPDataVersionChecksumWithMap( pSecond ) );
	EXPECT_EQ( refreshed, pConsts->GetMPDataVersionChecksumWithMap( pFirst ) );

	// Refreshing base data must also invalidate the combined map checksum.
	++pNet->nMaxLatency;
	pConsts->RecalculateMPDataVersionChecksum();
	EXPECT_NE( refreshed, pConsts->GetMPDataVersionChecksumWithMap( pFirst ) );
}

TEST_F( MPDataChecksum, CoopScriptFingerprintIncludesMissionBytesAndFilename )
{
	auto *pMap = CreateRecord<NDb::SMapInfo>( "MapInfo" );
	ASSERT_NE( pMap, nullptr );
	pMap->szScriptFileRef = "coop-mission.lua";
	WriteScript( "coop-mission.lua", "function Mission() return 1 end\n" );
	const uint32_t initial = NGameX::CalculateCoopScriptChecksum( pMap, pAI );
	WriteScript( "coop-mission.lua", "function Mission() return 2 end\n" );
	const uint32_t edited = NGameX::CalculateCoopScriptChecksum( pMap, pAI );
	EXPECT_NE( initial, edited );
	WriteScript( "coop-renamed.lua", "function Mission() return 2 end\n" );
	pMap->szScriptFileRef = "coop-renamed.lua";
	EXPECT_NE( edited, NGameX::CalculateCoopScriptChecksum( pMap, pAI ) );
}

TEST_F( MPDataChecksum, CoopScriptFingerprintIncludesCommonBytesNamesAndLoadOrder )
{
	auto *pMap = CreateRecord<NDb::SMapInfo>( "MapInfo" );
	ASSERT_NE( pMap, nullptr );
	pAI->commonScriptFileRefs.resize( 2 );
	pAI->commonScriptFileRefs[0].szScriptFileRef = "coop-common-first.lua";
	pAI->commonScriptFileRefs[1].szScriptFileRef = "coop-common-second.lua";
	WriteScript( "coop-common-first.lua", "value = 1\n" );
	WriteScript( "coop-common-second.lua", "value = value + 1\n" );
	const uint32_t initial = NGameX::CalculateCoopScriptChecksum( pMap, pAI );
	WriteScript( "coop-common-first.lua", "value = 2\n" );
	const uint32_t edited = NGameX::CalculateCoopScriptChecksum( pMap, pAI );
	EXPECT_NE( initial, edited );
	WriteScript( "coop-common-copy.lua", "value = 2\n" );
	pAI->commonScriptFileRefs[0].szScriptFileRef = "coop-common-copy.lua";
	const uint32_t renamed = NGameX::CalculateCoopScriptChecksum( pMap, pAI );
	EXPECT_NE( edited, renamed );
	std::swap( pAI->commonScriptFileRefs[0], pAI->commonScriptFileRefs[1] );
	EXPECT_NE( renamed, NGameX::CalculateCoopScriptChecksum( pMap, pAI ) );
}

TEST_F( MPDataChecksum, CoopScriptFingerprintDistinguishesUnsetMissingAndEmptyFiles )
{
	auto *pMap = CreateRecord<NDb::SMapInfo>( "MapInfo" );
	ASSERT_NE( pMap, nullptr );
	const uint32_t unset = NGameX::CalculateCoopScriptChecksum( pMap, pAI );
	pMap->szScriptFileRef = "coop-empty.lua";
	const uint32_t missing = NGameX::CalculateCoopScriptChecksum( pMap, pAI );
	EXPECT_NE( unset, missing );
	WriteScript( "coop-empty.lua", "" );
	EXPECT_NE( missing, NGameX::CalculateCoopScriptChecksum( pMap, pAI ) );
}

TEST_F( MPDataChecksum, OnlyCoopCompatibilitySnapshotsIncludeScriptContents )
{
	auto *pMission = CreateRecord<NDb::SMapInfo>( "MapInfo" );
	auto *pMap = CreateRecord<NDb::SMultiplayerMap>( "MultiplayerMap" );
	ASSERT_NE( pMission, nullptr );
	ASSERT_NE( pMap, nullptr );
	pMap->pMap = pMission;
	pMission->szScriptFileRef = "coop-snapshot.lua";
	WriteScript( "coop-snapshot.lua", "value = 1\n" );
	pMap->eGameType = NDb::COOP;
	const uint32_t initial = pConsts->GetMPDataVersionChecksumWithMap( pMap );
	WriteScript( "coop-snapshot.lua", "value = 2\n" );
	EXPECT_EQ( initial, pConsts->GetMPDataVersionChecksumWithMap( pMap ) );
	pConsts->RecalculateMPDataVersionChecksum();
	EXPECT_NE( initial, pConsts->GetMPDataVersionChecksumWithMap( pMap ) );

	// Existing CLASSIC compatibility checks keep their historical inputs.
	pMap->eGameType = NDb::CLASSIC;
	pConsts->RecalculateMPDataVersionChecksum();
	const uint32_t classic = pConsts->GetMPDataVersionChecksumWithMap( pMap );
	WriteScript( "coop-snapshot.lua", "value = 3\n" );
	pConsts->RecalculateMPDataVersionChecksum();
	EXPECT_EQ( classic, pConsts->GetMPDataVersionChecksumWithMap( pMap ) );
}
} // namespace
