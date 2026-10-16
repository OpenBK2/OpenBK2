// File selection must depend on the installed files, never on the player's timezone.
#include "Misc/Tools.h"
#include "System/Basic.h"
#include "System/FileTime.h"
#include "System/Streams.h"
#include "System/VFS.h"
#include "System/WinVFS.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/predef.h>
#include <gtest/gtest.h>
#include <zlib.h>

namespace
{

struct SFileTimeCase
{
	std::time_t utc;
	uint32_t packed;
};

// Independent epoch/DOS pairs, including a year boundary, leap day, and wall
// times that would be missing or ambiguous under US daylight-saving rules.
const SFileTimeCase S_TIMES[] = { { 315532800, 0x00210000 },
                                  { 1704069000, 0x582103c0 },
                                  { 1709251198, 0x585dbf7d },
                                  { 1719837296, 0x58e1645c },
                                  { 1710037800, 0x586a13c0 },
                                  { 1730597400, 0x59630bc0 } };

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

// A single stored ZIP entry with a literal DOS timestamp. Using PackFileTime
// to create this fixture would hide a timezone error shared by both inputs.
std::vector<uint8_t> MakePak( uint32_t nTimestamp )
{
	const std::string szName = "test.xdb";
	const std::string szData = "archive";
	const uint32_t nSize = static_cast<uint32_t>( szData.size() );
	const uint32_t nCrc = crc32( 0, reinterpret_cast<const Bytef *>( szData.data() ), nSize );
	std::vector<uint8_t> zip;
	Put32( &zip, 0x04034b50 );
	Put16( &zip, 20 );
	Put16( &zip, 0 );
	Put16( &zip, 0 );
	Put32( &zip, nTimestamp );
	Put32( &zip, nCrc );
	Put32( &zip, nSize );
	Put32( &zip, nSize );
	Put16( &zip, static_cast<uint16_t>( szName.size() ) );
	Put16( &zip, 0 );
	zip.insert( zip.end(), szName.begin(), szName.end() );
	zip.insert( zip.end(), szData.begin(), szData.end() );
	const uint32_t nCentral = static_cast<uint32_t>( zip.size() );
	Put32( &zip, 0x02014b50 );
	Put16( &zip, 20 );
	Put16( &zip, 20 );
	Put16( &zip, 0 );
	Put16( &zip, 0 );
	Put32( &zip, nTimestamp );
	Put32( &zip, nCrc );
	Put32( &zip, nSize );
	Put32( &zip, nSize );
	Put16( &zip, static_cast<uint16_t>( szName.size() ) );
	Put16( &zip, 0 );
	Put16( &zip, 0 );
	Put16( &zip, 0 );
	Put16( &zip, 0 );
	Put32( &zip, 0 );
	Put32( &zip, 0 );
	zip.insert( zip.end(), szName.begin(), szName.end() );
	const uint32_t nCentralSize = static_cast<uint32_t>( zip.size() ) - nCentral;
	Put32( &zip, 0x06054b50 );
	Put16( &zip, 0 );
	Put16( &zip, 0 );
	Put16( &zip, 1 );
	Put16( &zip, 1 );
	Put32( &zip, nCentralSize );
	Put32( &zip, nCentral );
	Put16( &zip, 0 );
	return zip;
}

int SetTimezone( const char *pszTimezone )
{
#if BOOST_OS_WINDOWS
	const int nResult = _putenv_s( "TZ", pszTimezone ? pszTimezone : "" );
	_tzset();
#else
	const int nResult = pszTimezone ? setenv( "TZ", pszTimezone, 1 ) : unsetenv( "TZ" );
	tzset();
#endif
	return nResult;
}

class VFSTimestamps : public testing::TestWithParam<const char *>
{
	bool bHadTimezone = false;
	std::string szOldTimezone;

protected:
	std::filesystem::path directory;

	void SetUp() override
	{
		const char *pszTimezone = std::getenv( "TZ" );
		bHadTimezone = pszTimezone != nullptr;
		szOldTimezone = pszTimezone ? pszTimezone : "";
		ASSERT_EQ( SetTimezone( GetParam() ), 0 );
		// Only this fixture owns this directory, including when test processes run in parallel.
		directory = std::filesystem::temp_directory_path() /
		            boost::filesystem::unique_path( "obk2-vfs-time-%%%%-%%%%-%%%%" ).string();
		ASSERT_TRUE( std::filesystem::create_directory( directory ) );
		std::ofstream file( directory / "test.xdb", std::ios::binary );
		file << "loose";
		file.close();
		ASSERT_TRUE( file.good() );
	}

	void TearDown() override
	{
		EXPECT_EQ( SetTimezone( bHadTimezone ? szOldTimezone.c_str() : nullptr ), 0 );
		std::error_code ec;
		std::filesystem::remove_all( directory, ec );
		EXPECT_FALSE( ec ) << ec.message();
	}

	void CheckSelection( bool bEnumerate )
	{
		// Winter and summer catch both timezone offsets and daylight-saving offsets.
		for ( const int nCase : { 1, 3 } )
		{
			const SFileTimeCase &time = S_TIMES[nCase];
			const std::vector<uint8_t> zip = MakePak( time.packed );
			{
				std::ofstream file( directory / "test.pak", std::ios::binary );
				file.write( reinterpret_cast<const char *>( zip.data() ), static_cast<std::streamsize>( zip.size() ) );
				file.close();
				ASSERT_TRUE( file.good() );
			}
			// Older, equal, rounded to equal, newer. A tie keeps the archive, as before.
			for ( const int nOffset : { -2, 0, 1, 2 } )
			{
				SCOPED_TRACE( testing::Message() << "epoch=" << time.utc << ", offset=" << nOffset );
				boost::filesystem::last_write_time( ( directory / "test.xdb" ).string(), time.utc + nOffset );
				CObj<NVFS::IVFS> pVFS = NVFS::CreateWinVFS( directory.string() + "/" );
				if ( bEnumerate )
				{
					std::vector<std::string> names;
					pVFS->GetAllFileNames( &names, "" );
					EXPECT_NE( std::find( names.begin(), names.end(), "test.xdb" ), names.end() );
				}
				ASSERT_TRUE( pVFS->DoesFileExist( "test.xdb" ) );
				std::unique_ptr<CDataStream> stream( pVFS->OpenFile( "test.xdb" ) );
				ASSERT_NE( stream, nullptr );
				std::string data( stream->GetSize(), '\0' );
				stream->Read( data.data(), static_cast<int>( data.size() ) );
				const bool bLooseWins = nOffset == 2;
				EXPECT_EQ( data, bLooseWins ? "loose" : "archive" );
				NVFS::SFileStats stats;
				ASSERT_TRUE( pVFS->GetFileStats( &stats, "test.xdb" ) );
				EXPECT_EQ( static_cast<uint32_t>( stats.mtime ), time.packed + ( bLooseWins ? 1 : 0 ) );
				EXPECT_EQ( stats.pszName == nullptr, bLooseWins );
				EXPECT_EQ( stats.nSize, bLooseWins ? 5 : 7 );
			}
		}
	}
};

TEST_P( VFSTimestamps, PacksUTCWithTwoSecondResolution )
{
	for ( const SFileTimeCase &time : S_TIMES )
	{
		SCOPED_TRACE( time.utc );
		EXPECT_EQ( PackFileTime( time.utc ), time.packed );
		EXPECT_EQ( PackFileTime( time.utc + 1 ), time.packed );
	}
	EXPECT_EQ( PackFileTime( 0 ), 0u );
}

TEST_P( VFSTimestamps, UnpacksUTCWithoutDaylightSavingNormalization )
{
	for ( const SFileTimeCase &time : S_TIMES )
	{
		SCOPED_TRACE( time.utc );
		EXPECT_EQ( UnpackFileTime( time.packed ), time.utc );
	}
}

TEST_P( VFSTimestamps, LooseFileStatsUseUTC )
{
	for ( const SFileTimeCase &time : S_TIMES )
	{
		SCOPED_TRACE( time.utc );
		const std::string path = ( directory / "test.xdb" ).string();
		boost::filesystem::last_write_time( path, time.utc );
		NVFS::SFileStats stats;
		ASSERT_TRUE( NVFS::GetWinFileStats( &stats, path ) );
		EXPECT_EQ( static_cast<uint32_t>( stats.mtime ), time.packed );
	}
}

TEST_P( VFSTimestamps, DirectLookupSelectsTheSameContents )
{
	CheckSelection( false );
}

TEST_P( VFSTimestamps, EnumerationSelectsTheSameContents )
{
	CheckSelection( true );
}

// POSIX-style TZ offsets have reversed signs. EST5EDT also exercises DST.
// Each test restores TZ; no worker threads run while the process timezone changes.
INSTANTIATE_TEST_SUITE_P( Timezones, VFSTimestamps, testing::Values( "UTC0", "EAS-5", "WES5", "EST5EDT" ) );

} // namespace
