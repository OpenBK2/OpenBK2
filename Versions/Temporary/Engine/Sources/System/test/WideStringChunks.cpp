// Pins the on-the-wire and in-stream encoding of wide strings.
//
// The binary savers store a wide string as a four-byte count of UTF-16 code
// units followed by two bytes per unit, which is what a Windows wstring's
// memory looks like, so the original code wrote and read it by memcpy. Off
// Windows wchar_t is four bytes, and the same memcpy wrote the first half of
// the buffer's bytes and read two bytes into each four-byte character, so chat
// messages, server names, map names and anything else wide came out corrupt and
// no Linux client could exchange them with a Windows one.
//
// These tests require the exact bytes rather than a round trip alone, because a
// round trip would pass just as happily if both ends agreed on the wrong
// encoding; it is the byte layout that has to match what Windows always wrote.

#include "Misc/Asserts.h"
#include "Misc/Tools.h"
#include "System/Basic.h"
#include "System/BinSaver.h"
#include "System/ChunklessSaver.h"
#include "System/Streams.h"
#include "System/System.h"

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{

// A serializable holder, since a saver reaches a string through a chunk rather
// than on its own.
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
		CPtr<IBinSaver> pSaver = CreateChunklessSaver( 0, &stream, SAVER_MODE_WRITE );
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
	CPtr<IBinSaver> pSaver = CreateChunklessSaver( 0, &stream, SAVER_MODE_READ );
	pSaver->Add( 1, &holder );
	return holder.wszValue;
}

// The count and the code units, independent of what wchar_t is locally.
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

TEST( WideStringChunks, AsciiIsTwoBytesPerCharacterLittleEndian )
{
	EXPECT_EQ( Save( L"AB" ), Expected( { 0x0041, 0x0042 } ) );
}

TEST( WideStringChunks, CyrillicIsTwoBytesPerCharacterLittleEndian )
{
	// the text this game actually carries
	EXPECT_EQ( Save( L"\x0414\x0430" ), Expected( { 0x0414, 0x0430 } ) );
}

TEST( WideStringChunks, OutsideTheBasicPlaneBecomesASurrogatePair )
{
	// U+1D11E, two UTF-16 code units, so the count is 2 even where one wchar_t
	// holds the whole code point
	EXPECT_EQ( Save( L"\x0001D11E" ), Expected( { 0xD834, 0xDD1E } ) );
}

TEST( WideStringChunks, EmptyIsACountAndNothingElse )
{
	EXPECT_EQ( Save( L"" ), Expected( {} ) );
}

TEST( WideStringChunks, RoundTrips )
{
	const std::wstring wszValues[] = {
		L"",
		L"AB",
		L"\x0414\x0430 \x0442\x0435\x043A\x0441\x0442",
		L"\x0001D11E",
		L"mixed \x0414 \x0001D11E end",
	};
	for ( const std::wstring &wszValue : wszValues )
	{
		EXPECT_EQ( Load( Save( wszValue ) ), wszValue );
	}
}

TEST( WideStringChunks, ReadsBytesWrittenByTheWindowsFormat )
{
	// What a Windows build wrote: the count, then the code units. Must decode
	// to the same text here, which is the whole point of the encoding.
	EXPECT_EQ( Load( Expected( { 0x0414, 0x0430 } ) ), std::wstring( L"\x0414\x0430" ) );
	EXPECT_EQ( Load( Expected( { 0xD834, 0xDD1E } ) ), std::wstring( L"\x0001D11E" ) );
}

} // namespace
