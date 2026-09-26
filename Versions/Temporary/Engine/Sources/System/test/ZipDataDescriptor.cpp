// A zip written as a stream opens with its contents.
//
// Zip writers that stream, libarchive among them and so cmake -E tar, set bit 3
// of an entry's flags, leave the sizes in its local header zero, and put the
// real ones in a descriptor after the data and in the central directory.
// CZipFile used to read the sizes from the local header, so every file in such
// an archive opened empty. obk2_fonts.pak, which the build makes with cmake -E
// tar, left the game with no fonts at all that way.
//
// The archives here are built byte by byte in exactly that layout, one stored
// and one deflated, so the test does not depend on any zip tool.

// The standard headers the engine's stdafx.h prelude would supply, which the
// object model's headers rely on without including
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <system_error>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <zlib.h>

#include "Misc/Asserts.h"
#include "Misc/Tools.h"
#include "System/System.h"
#include "System/Basic.h"
#include "System/Streams.h"
#include "System/VFS.h"
#include "System/WinVFS.h"

#include <gtest/gtest.h>

namespace {

void Put16( std::vector<uint8_t> *pOut, uint16_t n ) { pOut->push_back( n & 0xff ); pOut->push_back( n >> 8 ); }
void Put32( std::vector<uint8_t> *pOut, uint32_t n ) { Put16( pOut, n & 0xffff ); Put16( pOut, n >> 16 ); }

std::vector<uint8_t> Deflate( const std::string &szData )
{
	std::vector<uint8_t> out( compressBound( static_cast<uLong>( szData.size() ) ) + 64 );
	z_stream stream = {};
	deflateInit2( &stream, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY );
	stream.next_in = (Bytef*)szData.data();
	stream.avail_in = static_cast<uInt>( szData.size() );
	stream.next_out = out.data();
	stream.avail_out = static_cast<uInt>( out.size() );
	deflate( &stream, Z_FINISH );
	out.resize( stream.total_out );
	deflateEnd( &stream );
	return out;
}

// One entry, streamed: flags bit 3, zero sizes and CRC in the local header, a
// data descriptor after the data, and the real values in the central directory
std::vector<uint8_t> StreamedZip( const std::string &szName, const std::string &szData, bool bDeflate )
{
	const std::vector<uint8_t> packed = bDeflate ? Deflate( szData ) : std::vector<uint8_t>( szData.begin(), szData.end() );
	const uint32_t nCrc = crc32( 0, (const Bytef*)szData.data(), static_cast<uInt>( szData.size() ) );
	const uint16_t nMethod = bDeflate ? 8 : 0;
	std::vector<uint8_t> zip;
	Put32( &zip, 0x04034b50 );
	Put16( &zip, 20 );
	Put16( &zip, 0x0008 );
	Put16( &zip, nMethod );
	Put16( &zip, 0 );
	Put16( &zip, 0x5B3A );			// a 2025 date, so the entry looks ordinary
	Put32( &zip, 0 );
	Put32( &zip, 0 );
	Put32( &zip, 0 );
	Put16( &zip, static_cast<uint16_t>( szName.size() ) );
	Put16( &zip, 0 );
	zip.insert( zip.end(), szName.begin(), szName.end() );
	zip.insert( zip.end(), packed.begin(), packed.end() );
	Put32( &zip, 0x08074b50 );
	Put32( &zip, nCrc );
	Put32( &zip, static_cast<uint32_t>( packed.size() ) );
	Put32( &zip, static_cast<uint32_t>( szData.size() ) );
	const uint32_t nCentral = static_cast<uint32_t>( zip.size() );
	Put32( &zip, 0x02014b50 );
	Put16( &zip, 20 );
	Put16( &zip, 20 );
	Put16( &zip, 0x0008 );
	Put16( &zip, nMethod );
	Put16( &zip, 0 );
	Put16( &zip, 0x5B3A );
	Put32( &zip, nCrc );
	Put32( &zip, static_cast<uint32_t>( packed.size() ) );
	Put32( &zip, static_cast<uint32_t>( szData.size() ) );
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

// Puts the archive alone in a fresh directory as a .pak and reads szName back
// through a VFS over that directory, the way the game reads its Data
std::string ReadThroughVFS( const std::vector<uint8_t> &zip, const char *pszDirectory, const std::string &szName )
{
	const std::filesystem::path directory = std::filesystem::temp_directory_path() / pszDirectory;
	std::error_code error;
	std::filesystem::remove_all( directory, error );
	std::filesystem::create_directories( directory );
	{
		std::ofstream file( directory / "streamed.pak", std::ios::binary );
		file.write( reinterpret_cast<const char*>( zip.data() ), static_cast<std::streamsize>( zip.size() ) );
	}
	std::string szRead;
	{
		CObj<NVFS::IVFS> pVFS = NVFS::CreateWinVFS( directory.string() + static_cast<char>( std::filesystem::path::preferred_separator ) );
		EXPECT_TRUE( pVFS->DoesFileExist( szName ) );
		CDataStream *pStream = pVFS->OpenFile( szName );
		EXPECT_TRUE( pStream != 0 );
		if ( pStream != 0 )
		{
			szRead.resize( pStream->GetSize() );
			if ( !szRead.empty() )
				pStream->Read( &szRead[0], static_cast<int>( szRead.size() ) );
			delete pStream;
		}
	}
	std::filesystem::remove_all( directory, error );
	return szRead;
}

const std::string S_CONTENT = "<Font ObjectRecordID=\"1000000\"><FontFile href=\"/Fonts/Files/PTSans-Regular.ttf\" /></Font>\n"
	"repeated so that deflate has something to do: <Font><Font><Font><Font><Font><Font><Font><Font>\n";

TEST( ZipDataDescriptor, StoredEntryOpensWhole )
{
	EXPECT_EQ( ReadThroughVFS( StreamedZip( "Fonts/Body/Font.xdb", S_CONTENT, false ), "obk2_zip_stored", "Fonts/Body/Font.xdb" ), S_CONTENT );
}

TEST( ZipDataDescriptor, DeflatedEntryOpensWhole )
{
	EXPECT_EQ( ReadThroughVFS( StreamedZip( "Fonts/Body/Font.xdb", S_CONTENT, true ), "obk2_zip_deflated", "Fonts/Body/Font.xdb" ), S_CONTENT );
}

}
