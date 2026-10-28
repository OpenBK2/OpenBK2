#include "Misc/Tools.h"
#include "System/Basic.h"
#include "System/Streams.h"
#include "System/VFS.h"
#include "System/WinVFS.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <gtest/gtest.h>
#include <zlib.h>

// A file inside a pak can be overridden by a loose copy under the VFS's base directory
// with the same path and a newer time. These tests hold the VFS to which copy it serves,
// for files whose directories exist on disk, do not, exist only under another spelling,
// or come into existence after the VFS has looked.

namespace
{

// 2024-01-01, older than any loose file these tests write, so a loose copy always wins
const uint32_t PAK_TIME = 0x58210000;

void Put16( std::vector<uint8_t> *pOut, uint16_t n )
{
	pOut->push_back( n & 0xff );
	pOut->push_back( n >> 8 );
}

void Put32( std::vector<uint8_t> *pOut, uint32_t n )
{
	Put16( pOut, n & 0xffff );
	Put16( pOut, n >> 16 );
}

// A zip of stored entries, name and contents each.
std::vector<uint8_t> MakePak( const std::vector<std::pair<std::string, std::string>> &entries )
{
	std::vector<uint8_t> zip;
	std::vector<uint8_t> central;
	for ( const std::pair<std::string, std::string> &entry : entries )
	{
		const std::string &szName = entry.first;
		const std::string &szData = entry.second;
		const uint32_t nSize = static_cast<uint32_t>( szData.size() );
		const uint32_t nCrc = crc32( 0, reinterpret_cast<const Bytef *>( szData.data() ), nSize );
		const uint32_t nLocal = static_cast<uint32_t>( zip.size() );
		Put32( &zip, 0x04034b50 );
		Put16( &zip, 20 );
		Put16( &zip, 0 );
		Put16( &zip, 0 );
		Put32( &zip, PAK_TIME );
		Put32( &zip, nCrc );
		Put32( &zip, nSize );
		Put32( &zip, nSize );
		Put16( &zip, static_cast<uint16_t>( szName.size() ) );
		Put16( &zip, 0 );
		zip.insert( zip.end(), szName.begin(), szName.end() );
		zip.insert( zip.end(), szData.begin(), szData.end() );
		Put32( &central, 0x02014b50 );
		Put16( &central, 20 );
		Put16( &central, 20 );
		Put16( &central, 0 );
		Put16( &central, 0 );
		Put32( &central, PAK_TIME );
		Put32( &central, nCrc );
		Put32( &central, nSize );
		Put32( &central, nSize );
		Put16( &central, static_cast<uint16_t>( szName.size() ) );
		Put16( &central, 0 );
		Put16( &central, 0 );
		Put16( &central, 0 );
		Put16( &central, 0 );
		Put32( &central, 0 );
		Put32( &central, nLocal );
		central.insert( central.end(), szName.begin(), szName.end() );
	}
	const uint32_t nCentral = static_cast<uint32_t>( zip.size() );
	zip.insert( zip.end(), central.begin(), central.end() );
	Put32( &zip, 0x06054b50 );
	Put16( &zip, 0 );
	Put16( &zip, 0 );
	Put16( &zip, static_cast<uint16_t>( entries.size() ) );
	Put16( &zip, static_cast<uint16_t>( entries.size() ) );
	Put32( &zip, static_cast<uint32_t>( central.size() ) );
	Put32( &zip, nCentral );
	Put16( &zip, 0 );
	return zip;
}

class VFSLooseFiles : public testing::Test
{
protected:
	std::filesystem::path directory;

	void SetUp() override
	{
		// Only this fixture owns this directory, including when test processes run in parallel.
		directory = std::filesystem::temp_directory_path() /
		            boost::filesystem::unique_path( "obk2-vfs-loose-%%%%-%%%%-%%%%" ).string();
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		const std::vector<uint8_t> zip = MakePak( { { "Units/Tank/stats.xdb", "packed tank" },
		                                            { "Units/Tank/model.xdb", "packed model" },
		                                            { "Weapons/Gun/stats.xdb", "packed gun" },
		                                            { "UnitsX/Plane/stats.xdb", "packed plane" },
		                                            { "Root.xdb", "packed root" } } );
		std::ofstream file( directory / "test.pak", std::ios::binary );
		file.write( reinterpret_cast<const char *>( zip.data() ), static_cast<std::streamsize>( zip.size() ) );
		file.close();
		ASSERT_TRUE( file.good() );
	}

	void TearDown() override
	{
		std::error_code ec;
		std::filesystem::remove_all( directory, ec );
		EXPECT_FALSE( ec ) << ec.message();
	}

	void WriteLoose( const std::string &szPath, const std::string &szData )
	{
		const std::filesystem::path path = directory / szPath;
		std::filesystem::create_directories( path.parent_path() );
		std::ofstream file( path, std::ios::binary );
		file << szData;
		file.close();
		ASSERT_TRUE( file.good() );
	}

	CObj<NVFS::IVFS> MakeVFS()
	{
		return NVFS::CreateWinVFS( directory.string() + "/" );
	}

	// What the VFS serves for the path, or "<none>" when it has nothing by that name.
	static std::string Read( NVFS::IVFS *pVFS, const std::string &szPath )
	{
		if ( !pVFS->DoesFileExist( szPath ) )
		{
			return "<none>";
		}
		std::unique_ptr<CDataStream> stream( pVFS->OpenFile( szPath ) );
		if ( stream == nullptr )
		{
			return "<unopenable>";
		}
		std::string data( stream->GetSize(), '\0' );
		stream->Read( data.data(), static_cast<int>( data.size() ) );
		return data;
	}
};

TEST_F( VFSLooseFiles, PackedFilesWithNoLooseDirectoryComeFromThePak )
{
	CObj<NVFS::IVFS> pVFS = MakeVFS();
	EXPECT_EQ( Read( pVFS, "Units/Tank/stats.xdb" ), "packed tank" );
	EXPECT_EQ( Read( pVFS, "Weapons/Gun/stats.xdb" ), "packed gun" );
	EXPECT_EQ( Read( pVFS, "Root.xdb" ), "packed root" );
	EXPECT_EQ( Read( pVFS, "Units/Tank/missing.xdb" ), "<none>" );
	EXPECT_EQ( Read( pVFS, "Nowhere/At/All.xdb" ), "<none>" );
}

TEST_F( VFSLooseFiles, ANewerLooseFileOverridesThePackedOne )
{
	WriteLoose( "Units/Tank/stats.xdb", "loose tank" );
	WriteLoose( "Root.xdb", "loose root" );
	CObj<NVFS::IVFS> pVFS = MakeVFS();
	EXPECT_EQ( Read( pVFS, "Units/Tank/stats.xdb" ), "loose tank" );
	// a neighbour in the same directory that has no loose copy still comes from the pak
	EXPECT_EQ( Read( pVFS, "Units/Tank/model.xdb" ), "packed model" );
	EXPECT_EQ( Read( pVFS, "Root.xdb" ), "loose root" );
	EXPECT_EQ( Read( pVFS, "Weapons/Gun/stats.xdb" ), "packed gun" );
}

TEST_F( VFSLooseFiles, AFileOnlyOnDiskIsFound )
{
	WriteLoose( "Custom/Missions/New Map/map.xdb", "loose map" );
	CObj<NVFS::IVFS> pVFS = MakeVFS();
	EXPECT_EQ( Read( pVFS, "Custom/Missions/New Map/map.xdb" ), "loose map" );
	EXPECT_EQ( Read( pVFS, "Custom/Missions/Other Map/map.xdb" ), "<none>" );
}

TEST_F( VFSLooseFiles, DirectoriesMatchByWholeComponent )
{
	// Units is missing on disk while UnitsX exists: asking about the first must not
	// decide anything about the second, though one name begins with the other.
	WriteLoose( "UnitsX/Plane/stats.xdb", "loose plane" );
	CObj<NVFS::IVFS> pVFS = MakeVFS();
	EXPECT_EQ( Read( pVFS, "Units/Tank/stats.xdb" ), "packed tank" );
	EXPECT_EQ( Read( pVFS, "UnitsX/Plane/stats.xdb" ), "loose plane" );
}

TEST_F( VFSLooseFiles, ADirectorySpelledInAnotherCaseIsFound )
{
	WriteLoose( "UNITS/tank/stats.xdb", "loose tank" );
	CObj<NVFS::IVFS> pVFS = MakeVFS();
	EXPECT_EQ( Read( pVFS, "Units/Tank/stats.xdb" ), "loose tank" );
}

TEST_F( VFSLooseFiles, EitherSeparatorWorks )
{
	WriteLoose( "Units/Tank/stats.xdb", "loose tank" );
	CObj<NVFS::IVFS> pVFS = MakeVFS();
	EXPECT_EQ( Read( pVFS, "Units\\Tank\\stats.xdb" ), "loose tank" );
	EXPECT_EQ( Read( pVFS, "Weapons\\Gun/stats.xdb" ), "packed gun" );
}

// What the VFS learns about the disk it keeps for its lifetime: a loose file that
// appears after it has looked is not picked up (there is no hot reload), and a new VFS
// sees it.
TEST_F( VFSLooseFiles, ALooseFileAddedLaterIsSeenByTheNextVFS )
{
	CObj<NVFS::IVFS> pVFS = MakeVFS();
	EXPECT_EQ( Read( pVFS, "Custom/Missions/New Map/map.xdb" ), "<none>" );
	EXPECT_EQ( Read( pVFS, "Units/Tank/stats.xdb" ), "packed tank" );
	WriteLoose( "Custom/Missions/New Map/map.xdb", "loose map" );
	WriteLoose( "Units/Tank/stats.xdb", "loose tank" );
	EXPECT_EQ( Read( pVFS, "Units/Tank/stats.xdb" ), "packed tank" );
	// Custom is remembered as missing, so nothing under it is looked for again
	EXPECT_EQ( Read( pVFS, "Custom/Missions/New Map/map.xdb" ), "<none>" );
	CObj<NVFS::IVFS> pNextVFS = MakeVFS();
	EXPECT_EQ( Read( pNextVFS, "Custom/Missions/New Map/map.xdb" ), "loose map" );
	EXPECT_EQ( Read( pNextVFS, "Units/Tank/stats.xdb" ), "loose tank" );
}

} // namespace
