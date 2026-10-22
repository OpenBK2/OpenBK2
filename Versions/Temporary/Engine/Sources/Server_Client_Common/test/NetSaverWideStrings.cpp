// Pins the encoding of wide strings in multiplayer packets.
//
// CNetSaver stored a wide string as a four-byte count of UTF-16 code units
// followed by two bytes per unit, by memcpy out of a std::wstring, which is
// only the same thing where wchar_t is two bytes. Off Windows it is four, so
// the saver wrote the first half of the buffer's bytes and read two bytes into
// each four-byte character: every chat message, system broadcast, server name
// and map name on the wire was corrupt, and a Linux client could not exchange
// any of them with a Windows one.
//
// The bytes are required exactly rather than only round-tripped, because a
// round trip passes just as happily when both ends agree on the wrong
// encoding, and what has to hold is agreement with what Windows writes.
//
// The same format is pinned for the file and in-memory savers by
// System/test/WideStringChunks.

#include "Misc/Asserts.h"
#include "Misc/Tools.h"
#include "Server_Client_Common/NetSaver.h"
#include "System/Basic.h"
#include "System/BinSaver.h"
#include "System/Streams.h"
#include "System/System.h"

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{

struct SWideHolder
{
	std::wstring wszValue;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &wszValue );
		return 0;
	}
};

std::vector<uint8_t> Save( const std::wstring &wszValue )
{
	SWideHolder holder;
	holder.wszValue = wszValue;

	CMemoryStream stream;
	{
		CPtr<IBinSaver> pSaver = CreateNetSaver( &stream, SAVER_MODE_WRITE );
		pSaver->Add( 1, &holder );
	}

	stream.Seek( 0 );
	std::vector<uint8_t> bytes( stream.GetSize() );
	if ( !bytes.empty() )
	{
		stream.Read( &bytes[0], int( bytes.size() ) );
	}
	return bytes;
}

std::wstring Load( const std::vector<uint8_t> &bytes )
{
	CMemoryStream stream;
	if ( !bytes.empty() )
	{
		stream.Write( &bytes[0], int( bytes.size() ) );
	}
	stream.Seek( 0 );

	SWideHolder holder;
	CPtr<IBinSaver> pSaver = CreateNetSaver( &stream, SAVER_MODE_READ );
	pSaver->Add( 1, &holder );
	return holder.wszValue;
}

//! The count and the code units, whatever wchar_t is locally.
std::vector<uint8_t> Expected( const std::vector<uint16_t> &units )
{
	std::vector<uint8_t> bytes;
	const uint32_t nCount = uint32_t( units.size() );
	for ( int i = 0; i < 4; ++i )
	{
		bytes.push_back( uint8_t( ( nCount >> ( i * 8 ) ) & 0xFF ) );
	}
	for ( const uint16_t unit : units )
	{
		bytes.push_back( uint8_t( unit & 0xFF ) );
		bytes.push_back( uint8_t( unit >> 8 ) );
	}
	return bytes;
}

TEST( NetSaverWideStrings, AsciiIsTwoBytesPerCharacterLittleEndian )
{
	EXPECT_EQ( Save( L"AB" ), Expected( { 0x0041, 0x0042 } ) );
}

TEST( NetSaverWideStrings, CyrillicIsTwoBytesPerCharacterLittleEndian )
{
	// a chat message in the language the game ships in
	EXPECT_EQ( Save( L"\x043F\x0440\x0438\x0432\x0435\x0442" ),
	           Expected( { 0x043F, 0x0440, 0x0438, 0x0432, 0x0435, 0x0442 } ) );
}

TEST( NetSaverWideStrings, OutsideTheBasicPlaneBecomesASurrogatePair )
{
	// Spelled \U, a code point, rather than \x, a single code unit:
	// \x0001D11E does not fit a 16-bit wchar_t and MSVC rejects it (C7744),
	// while \U0001D11E is the surrogate pair there and one unit elsewhere.
	EXPECT_EQ( Save( L"\U0001D11E" ), Expected( { 0xD834, 0xDD1E } ) );
}

TEST( NetSaverWideStrings, EmptyIsACountAndNothingElse )
{
	EXPECT_EQ( Save( L"" ), Expected( {} ) );
}

TEST( NetSaverWideStrings, RoundTrips )
{
	const std::wstring wszValues[] = {
		L"",
		L"AB",
		L"\x043F\x0440\x0438\x0432\x0435\x0442",
		L"\U0001D11E",
		L"mixed \x0414 \U0001D11E end",
	};
	for ( const std::wstring &wszValue : wszValues )
	{
		EXPECT_EQ( Load( Save( wszValue ) ), wszValue );
	}
}

TEST( NetSaverWideStrings, ReadsBytesWrittenByAWindowsPeer )
{
	EXPECT_EQ( Load( Expected( { 0x043F, 0x0440, 0x0438 } ) ), std::wstring( L"\x043F\x0440\x0438" ) );
	EXPECT_EQ( Load( Expected( { 0xD834, 0xDD1E } ) ), std::wstring( L"\U0001D11E" ) );
}

} // namespace
