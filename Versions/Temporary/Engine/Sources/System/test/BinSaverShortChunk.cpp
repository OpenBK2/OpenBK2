// Pins what the binary saver does with a raw chunk shorter than the member it
// is being read into.
//
// CStructureSaver::DataChunk guarded that case with
// ASSERT( res.nLength == nSize ), and ASSERT does not evaluate its argument in
// this configuration, so the copy took nSize bytes whatever the stored length
// was. Two consequences, both on the save, replay and packet path:
//
//   the tail of the destination came from whatever followed the chunk in the
//   stream, so the value depended on its neighbours rather than on itself;
//
//   and once nStart + nSize left the buffer it read past the end of it
//   outright. ReadShortChunk bounds the stored length against its parent
//   chunk, which makes nStart + nLength safe and says nothing about
//   nStart + nSize.
//
// It now copies what is there and zeroes the rest, so a well-formed save is
// byte for byte what it always was and a damaged one reads the same way every
// time. A second chunk follows the raw one here on purpose: it is the
// neighbouring bytes, and the test is that they do not appear in the tail.

// The standard headers the engine's stdafx.h prelude would supply, which
// BinSaver.h relies on without including
#include "Misc/Asserts.h"
#include "Misc/Tools.h"
#include "System/Basic.h"
#include "System/BinSaver.h"
#include "System/det_map.h"
#include "System/Streams.h"
#include "System/System.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{

constexpr int RAW_CAPACITY = 16;
constexpr uint8_t WRITTEN = 0xA5;
constexpr uint8_t NEIGHBOUR = 0xEE;
constexpr uint8_t UNTOUCHED = 0x11;

// How many bytes of the raw chunk this record asks for. Writing with one value
// and reading with a larger one is how a short chunk is produced without
// having to fabricate the stream by hand.
int g_nRawSize = 4;

class CRawRecord : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CRawRecord );

public:
	uint8_t raw[RAW_CAPACITY];
	uint8_t neighbour[RAW_CAPACITY];

	CRawRecord()
	{
		std::memset( raw, UNTOUCHED, sizeof( raw ) );
		std::memset( neighbour, UNTOUCHED, sizeof( neighbour ) );
	}

	int operator&( IBinSaver &saver ) override
	{
		saver.AddRawData( 1, raw, g_nRawSize );
		// Follows the raw chunk in the stream, so its bytes are what an
		// unbounded copy would drag into the tail of raw.
		saver.AddRawData( 2, neighbour, sizeof( neighbour ) );
		return 0;
	}
};

} // namespace

// REGISTER_SAVELOAD_CLASS names an export macro after its module argument; a
// test executable exports nothing
#define BINSAVERSHORTCHUNK_EXPORT
REGISTER_SAVELOAD_CLASS( BINSAVERSHORTCHUNK, 0x7E57AB02, CRawRecord )

namespace
{

std::vector<char> Save( const int nRawSize )
{
	g_nRawSize = nRawSize;
	CObj<CRawRecord> pRecord = new CRawRecord;
	std::memset( pRecord->raw, WRITTEN, sizeof( pRecord->raw ) );
	std::memset( pRecord->neighbour, NEIGHBOUR, sizeof( pRecord->neighbour ) );

	CMemoryStream stream;
	{
		CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		pSaver->Add( 1, &pRecord );
	}
	return std::vector<char>( stream.GetBuffer(), stream.GetBuffer() + stream.GetSize() );
}

CObj<CRawRecord> Load( const std::vector<char> &bytes, const int nRawSize )
{
	g_nRawSize = nRawSize;
	CMemoryStream stream;
	stream.Write( bytes.data(), static_cast<int>( bytes.size() ) );
	stream.Seek( 0 );
	CObj<CRawRecord> pRecord;
	{
		CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_READ );
		pSaver->Add( 1, &pRecord );
	}
	return pRecord;
}

// The ordinary case, unchanged: written and read at the same length.
TEST( BinSaverShortChunk, MatchingLengthRoundTrips )
{
	const std::vector<char> bytes = Save( 4 );
	const CObj<CRawRecord> pRecord = Load( bytes, 4 );
	ASSERT_TRUE( pRecord != 0 );
	for ( int i = 0; i < 4; ++i )
	{
		EXPECT_EQ( WRITTEN, pRecord->raw[i] ) << "at " << i;
	}
	for ( int i = 0; i < RAW_CAPACITY; ++i )
	{
		EXPECT_EQ( NEIGHBOUR, pRecord->neighbour[i] ) << "at " << i;
	}
}

// The regression: written as 4 bytes, read as 12. The first four come from the
// chunk and the rest must be zeroed, not taken from the chunk that follows.
TEST( BinSaverShortChunk, ShortChunkZeroesTheTailInsteadOfReadingOn )
{
	const std::vector<char> bytes = Save( 4 );
	const CObj<CRawRecord> pRecord = Load( bytes, 12 );
	ASSERT_TRUE( pRecord != 0 );
	for ( int i = 0; i < 4; ++i )
	{
		EXPECT_EQ( WRITTEN, pRecord->raw[i] ) << "at " << i;
	}
	for ( int i = 4; i < 12; ++i )
	{
		EXPECT_EQ( 0, pRecord->raw[i] ) << "at " << i;
		EXPECT_NE( NEIGHBOUR, pRecord->raw[i] ) << "neighbouring chunk reached the tail, at " << i;
	}
	// Past what was asked for, the destination is left alone.
	for ( int i = 12; i < RAW_CAPACITY; ++i )
	{
		EXPECT_EQ( UNTOUCHED, pRecord->raw[i] ) << "at " << i;
	}
}

// A chunk longer than the member is the other direction and was always safe;
// pinned so the clamp cannot be written the wrong way round.
TEST( BinSaverShortChunk, LongChunkFillsTheMemberAndNoMore )
{
	const std::vector<char> bytes = Save( 12 );
	const CObj<CRawRecord> pRecord = Load( bytes, 4 );
	ASSERT_TRUE( pRecord != 0 );
	for ( int i = 0; i < 4; ++i )
	{
		EXPECT_EQ( WRITTEN, pRecord->raw[i] ) << "at " << i;
	}
	for ( int i = 4; i < RAW_CAPACITY; ++i )
	{
		EXPECT_EQ( UNTOUCHED, pRecord->raw[i] ) << "at " << i;
	}
}

// A missing chunk zeroes the member, which is the behaviour the short-chunk
// case now matches.
TEST( BinSaverShortChunk, MissingChunkZeroesTheMember )
{
	// Saved with chunk 1 absent, by asking for nothing, then read as 8 bytes.
	const std::vector<char> bytes = Save( 0 );
	const CObj<CRawRecord> pRecord = Load( bytes, 8 );
	ASSERT_TRUE( pRecord != 0 );
	for ( int i = 0; i < 8; ++i )
	{
		EXPECT_EQ( 0, pRecord->raw[i] ) << "at " << i;
	}
}

} // namespace
