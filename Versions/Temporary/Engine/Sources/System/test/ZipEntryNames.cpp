// A zip entry named in code page 437 opens by its UTF-8 name.
//
// Without bit 11 of its flags a zip entry's name is in CP437, the format's
// default and what the zip tools of the game's day wrote. The VFS takes names as
// UTF-8 and hands them to std::filesystem::u8path, which threw on such a name and
// ended the process: MOD-18's Daten.pak names a folder "38_cm_Sturmm\x94rser",
// with a CP437 o-umlaut. CZipFile now converts those names as it reads the
// directory, keeps names that are already UTF-8 (flagged or not), and prefers an
// Info-ZIP Unicode Path extra field when one belongs to the name.
//
// The archives are built byte by byte, so the test depends on no zip tool.

// The standard headers the engine's stdafx.h prelude would supply, which the
// object model's headers rely on without including
#include "Misc/Asserts.h"
#include "Misc/Tools.h"
#include "System/Basic.h"
#include "System/Streams.h"
#include "System/System.h"
#include "System/VFS.h"
#include "System/WinVFS.h"

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

#include <gtest/gtest.h>
#include <zlib.h>

namespace
{

// "Sturmmoerser" with an o-umlaut, as CP437 stores it and as UTF-8 does
const std::string S_CP437_NAME = "Units/Sturmm\x94rser/1.xdb";
const std::string S_UTF8_NAME = "Units/Sturmm\xC3\xB6rser/1.xdb";
const std::string S_CONTENT = "<MechUnitRPGStats ObjectRecordID=\"1\" />\n";

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

void PutBytes( std::vector<uint8_t> *pOut, const std::string &szBytes )
{
	pOut->insert( pOut->end(), szBytes.begin(), szBytes.end() );
}

uint32_t Crc( const std::string &szBytes )
{
	return crc32( 0, reinterpret_cast<const Bytef *>( szBytes.data() ), static_cast<uInt>( szBytes.size() ) );
}

// An Info-ZIP Unicode Path extra field for szLegacyName, naming szUTF8Name
std::string UnicodePathField( const std::string &szLegacyName, const std::string &szUTF8Name )
{
	std::vector<uint8_t> field;
	Put16( &field, 0x7075 );
	Put16( &field, static_cast<uint16_t>( 5 + szUTF8Name.size() ) );
	field.push_back( 1 );
	Put32( &field, Crc( szLegacyName ) );
	PutBytes( &field, szUTF8Name );
	return std::string( field.begin(), field.end() );
}

// One stored entry with the name bytes, flags and extra field given
std::vector<uint8_t> StoredZip( const std::string &szRawName, const uint16_t wFlags, const std::string &szExtra )
{
	const uint32_t nCrc = Crc( S_CONTENT );
	const uint32_t nSize = static_cast<uint32_t>( S_CONTENT.size() );
	std::vector<uint8_t> zip;
	Put32( &zip, 0x04034b50 );
	Put16( &zip, 20 );
	Put16( &zip, wFlags );
	Put16( &zip, 0 ); // stored
	Put16( &zip, 0 );
	Put16( &zip, 0x5B3A );
	Put32( &zip, nCrc );
	Put32( &zip, nSize );
	Put32( &zip, nSize );
	Put16( &zip, static_cast<uint16_t>( szRawName.size() ) );
	Put16( &zip, static_cast<uint16_t>( szExtra.size() ) );
	PutBytes( &zip, szRawName );
	PutBytes( &zip, szExtra );
	PutBytes( &zip, S_CONTENT );
	const uint32_t nCentral = static_cast<uint32_t>( zip.size() );
	Put32( &zip, 0x02014b50 );
	Put16( &zip, 20 );
	Put16( &zip, 20 );
	Put16( &zip, wFlags );
	Put16( &zip, 0 );
	Put16( &zip, 0 );
	Put16( &zip, 0x5B3A );
	Put32( &zip, nCrc );
	Put32( &zip, nSize );
	Put32( &zip, nSize );
	Put16( &zip, static_cast<uint16_t>( szRawName.size() ) );
	Put16( &zip, static_cast<uint16_t>( szExtra.size() ) );
	Put16( &zip, 0 );
	Put16( &zip, 0 );
	Put16( &zip, 0 );
	Put32( &zip, 0 );
	Put32( &zip, 0 );
	PutBytes( &zip, szRawName );
	PutBytes( &zip, szExtra );
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

struct SReadResult
{
	std::vector<std::string> names;
	std::string szContent;
};

// Puts the archive alone in a fresh directory as a .pak, lists it and reads
// szName back through a VFS over that directory, the way the game reads its Data
SReadResult ReadThroughVFS( const std::vector<uint8_t> &zip, const char *pszDirectory, const std::string &szName )
{
	const std::filesystem::path directory = std::filesystem::temp_directory_path() / pszDirectory;
	std::error_code error;
	std::filesystem::remove_all( directory, error );
	std::filesystem::create_directories( directory );
	{
		std::ofstream file( directory / "names.pak", std::ios::binary );
		file.write( reinterpret_cast<const char *>( zip.data() ), static_cast<std::streamsize>( zip.size() ) );
	}
	SReadResult result;
	{
		CObj<NVFS::IVFS> pVFS =
				NVFS::CreateWinVFS( directory.string() + static_cast<char>( std::filesystem::path::preferred_separator ) );
		pVFS->GetAllFileNames( &result.names, "Units/" );
		CDataStream *pStream = pVFS->DoesFileExist( szName ) ? pVFS->OpenFile( szName ) : 0;
		if ( pStream != 0 )
		{
			result.szContent.resize( pStream->GetSize() );
			if ( !result.szContent.empty() )
			{
				pStream->Read( &result.szContent[0], static_cast<int>( result.szContent.size() ) );
			}
			delete pStream;
		}
	}
	std::filesystem::remove_all( directory, error );
	return result;
}

// The VFS reports names with the platform's separator; compare with forward ones
std::string Portable( std::string szName )
{
	for ( char &c : szName )
	{
		if ( c == '\\' )
		{
			c = '/';
		}
	}
	return szName;
}

TEST( ZipEntryNames, CP437NameOpensByUTF8Name )
{
	const SReadResult result = ReadThroughVFS( StoredZip( S_CP437_NAME, 0, "" ), "obk2_zip_cp437", S_UTF8_NAME );
	ASSERT_EQ( result.names.size(), 1u );
	EXPECT_EQ( Portable( result.names[0] ), S_UTF8_NAME );
	EXPECT_EQ( result.szContent, S_CONTENT );
}

TEST( ZipEntryNames, FlaggedUTF8NameIsKept )
{
	const SReadResult result = ReadThroughVFS( StoredZip( S_UTF8_NAME, 0x0800, "" ), "obk2_zip_utf8_flag", S_UTF8_NAME );
	ASSERT_EQ( result.names.size(), 1u );
	EXPECT_EQ( Portable( result.names[0] ), S_UTF8_NAME );
	EXPECT_EQ( result.szContent, S_CONTENT );
}

// Many writers store UTF-8 without setting the flag; such a name must not be
// converted a second time
TEST( ZipEntryNames, UnflaggedUTF8NameIsKept )
{
	const SReadResult result = ReadThroughVFS( StoredZip( S_UTF8_NAME, 0, "" ), "obk2_zip_utf8_noflag", S_UTF8_NAME );
	ASSERT_EQ( result.names.size(), 1u );
	EXPECT_EQ( Portable( result.names[0] ), S_UTF8_NAME );
	EXPECT_EQ( result.szContent, S_CONTENT );
}

// The field names the entry in UTF-8; here it says something CP437 decoding
// would not give, so the test can tell which one was used
TEST( ZipEntryNames, UnicodePathFieldIsPreferred )
{
	const std::string szFieldName = "Units/Sturmm\xC3\xB8rser/1.xdb"; // o-slash
	const SReadResult result = ReadThroughVFS(
			StoredZip( S_CP437_NAME, 0, UnicodePathField( S_CP437_NAME, szFieldName ) ), "obk2_zip_upath", szFieldName );
	ASSERT_EQ( result.names.size(), 1u );
	EXPECT_EQ( Portable( result.names[0] ), szFieldName );
	EXPECT_EQ( result.szContent, S_CONTENT );
}

// A field whose checksum is for another name is stale and is ignored
TEST( ZipEntryNames, StaleUnicodePathFieldIsIgnored )
{
	const std::string szFieldName = "Units/Sturmm\xC3\xB8rser/1.xdb";
	const SReadResult result =
			ReadThroughVFS( StoredZip( S_CP437_NAME, 0, UnicodePathField( "Units/Other/1.xdb", szFieldName ) ),
	                    "obk2_zip_upath_stale",
	                    S_UTF8_NAME );
	ASSERT_EQ( result.names.size(), 1u );
	EXPECT_EQ( Portable( result.names[0] ), S_UTF8_NAME );
	EXPECT_EQ( result.szContent, S_CONTENT );
}

// A name that is not UTF-8 in any form, a lone continuation byte, comes out as
// valid UTF-8 rather than ending the process when it is listed or opened
TEST( ZipEntryNames, InvalidBytesBecomeValidUTF8 )
{
	const std::string szRaw = "Units/a\x80"
														"b/1.xdb";
	const std::string szExpected = "Units/a\xC3\x87"
																 "b/1.xdb"; // CP437 0x80 is C-cedilla
	const SReadResult result = ReadThroughVFS( StoredZip( szRaw, 0, "" ), "obk2_zip_invalid", szExpected );
	ASSERT_EQ( result.names.size(), 1u );
	EXPECT_EQ( Portable( result.names[0] ), szExpected );
	EXPECT_EQ( result.szContent, S_CONTENT );
}

} // namespace
