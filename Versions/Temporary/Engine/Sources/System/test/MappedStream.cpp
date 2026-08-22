// Pins down what CFileStream does when a write runs past the end of the
// mapping, which is the behaviour any reimplementation of the mapping layer
// has to preserve.
//
// The stream is not a stream: CDataStream keeps four raw pointers into a
// buffer and writes with memcpy. When a write passes the end, FixupBuf picks a
// larger size and calls AllocBuf, which for a mapped stream unmaps the view,
// resizes the file, maps a new one, and re-derives every pointer from a new
// base address. Growth therefore moves the buffer, and the file is extended as
// a side effect of creating the mapping.
//
// These tests exist because that sequence is easy to get subtly wrong while
// leaving a game that still starts: a file can come back short, or a read can
// run off the end of a mapping that is smaller than the buffer claims. Each
// test writes a pattern whose every byte is a function of its offset, so a
// short read, a duplicated block or a stale mapping shows up as a specific
// mismatch rather than a crash.

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

// Streams.h uses ASSERT and std::string without including either
#include "Misc/Asserts.h"
#include "System/Streams.h"

#include <gtest/gtest.h>

namespace {

// Every byte is derived from its offset, so a wrong byte names the offset it
// was really taken from.
unsigned char ByteAt( size_t nOffset )
{
	return static_cast<unsigned char>( ( nOffset * 31u + ( nOffset >> 13 ) ) & 0xff );
}

std::filesystem::path TempFile( const char *pszName )
{
	const std::filesystem::path dir =
		std::filesystem::temp_directory_path() / "bk2_mappedstream_test";
	std::error_code ec;
	std::filesystem::create_directories( dir, ec );
	return dir / pszName;
}

// Write nTotal bytes in chunks of nChunk, so the number of growths is
// controlled by the caller rather than by one large write.
void WritePattern( const std::filesystem::path &path, size_t nTotal, size_t nChunk )
{
	CFileStream f( path.string(), CFileStream::WIN_CREATE );
	ASSERT_TRUE( f.IsOk() ) << "could not create " << path.string();

	std::vector<unsigned char> chunk( nChunk );
	for ( size_t nWritten = 0; nWritten < nTotal; )
	{
		const size_t n = (std::min)( nChunk, nTotal - nWritten );
		for ( size_t i = 0; i < n; ++i )
		{
			chunk[i] = ByteAt( nWritten + i );
		}
		f.Write( chunk.data(), static_cast<int>( n ) );
		ASSERT_TRUE( f.IsOk() ) << "stream broke after " << nWritten << " bytes";
		nWritten += n;
	}
}

// Read the file back through the mapped reader and report the first byte that
// disagrees, which localises a short read or a stale mapping.
void ExpectPattern( const std::filesystem::path &path, size_t nTotal )
{
	CFileStream f( path.string(), CFileStream::WIN_READ_ONLY );
	ASSERT_TRUE( f.IsOk() ) << "could not open " << path.string();
	ASSERT_EQ( static_cast<size_t>( f.GetSize() ), nTotal ) << "file came back a different size";

	const unsigned char *p = f.GetBuffer();
	if ( nTotal == 0 )
	{
		// an empty file maps to no buffer at all, see EmptyFileHasNoBuffer
		return;
	}
	ASSERT_NE( p, nullptr );
	for ( size_t i = 0; i < nTotal; ++i )
	{
		if ( p[i] != ByteAt( i ) )
		{
			FAIL() << "first mismatch at offset " << i
			       << ": got " << static_cast<int>( p[i] )
			       << ", expected " << static_cast<int>( ByteAt( i ) );
		}
	}
}

} // namespace

// One write, no growth beyond the first allocation.
TEST( MappedStream, SmallFileRoundTrips )
{
	const auto path = TempFile( "small.bin" );
	WritePattern( path, 100, 100 );
	ExpectPattern( path, 100 );
}

// FixupBuf rounds the new size down to a multiple of 4096 and never goes below
// 4096, so the sizes either side of that boundary are the interesting ones.
TEST( MappedStream, SizesAroundTheGrowthBoundary )
{
	for ( const size_t nTotal : { size_t( 0 ), size_t( 1 ), size_t( 4095 ), size_t( 4096 ),
	                              size_t( 4097 ), size_t( 8192 ), size_t( 65535 ),
	                              size_t( 65536 ), size_t( 65537 ) } )
	{
		const auto path = TempFile( "boundary.bin" );
		WritePattern( path, nTotal, 128 );
		{
			SCOPED_TRACE( "total = " + std::to_string( nTotal ) );
			ExpectPattern( path, nTotal );
		}
	}
}

// Many small writes force many remaps, each moving the buffer. This is the
// case that breaks when the file is not extended before the new mapping is
// created, or when the new mapping is smaller than the buffer claims.
TEST( MappedStream, GrowsAcrossManyRemaps )
{
	const auto path = TempFile( "grow.bin" );
	const size_t nTotal = 4u * 1024u * 1024u;
	WritePattern( path, nTotal, 977 );	// deliberately not a power of two
	ExpectPattern( path, nTotal );
}

// Growth is min(size + 1MB, size * 2), so crossing the point where the
// doubling stops and the fixed step takes over exercises both branches.
TEST( MappedStream, CrossesTheDoublingThreshold )
{
	const auto path = TempFile( "threshold.bin" );
	const size_t nTotal = 3u * 1024u * 1024u + 12345u;
	WritePattern( path, nTotal, 64u * 1024u );
	ExpectPattern( path, nTotal );
}

// Seeking back and overwriting must not disturb the rest of the file, and must
// not change its size.
TEST( MappedStream, SeekBackAndOverwriteKeepsTheRest )
{
	const auto path = TempFile( "seek.bin" );
	const size_t nTotal = 256u * 1024u;
	WritePattern( path, nTotal, 1024 );

	{
		CFileStream f( path.string(), CFileStream::WIN_CREATE );
		ASSERT_TRUE( f.IsOk() );
		// WIN_CREATE truncates logically, so rebuild the file, then go back
		// and rewrite one block in the middle with the same bytes
		std::vector<unsigned char> all( nTotal );
		for ( size_t i = 0; i < nTotal; ++i )
		{
			all[i] = ByteAt( i );
		}
		f.Write( all.data(), static_cast<int>( nTotal ) );
		ASSERT_TRUE( f.Seek( static_cast<int>( nTotal / 2 ) ) );
		f.Write( all.data() + nTotal / 2, 4096 );
		ASSERT_TRUE( f.IsOk() );
	}

	ExpectPattern( path, nTotal );
}

// Two streams growing alternately. Each one remaps while the other holds a
// mapping open, which is the shape that fails when the implementation cannot
// resize a file while any view of it is alive.
TEST( MappedStream, TwoStreamsGrowingAtOnce )
{
	const auto pathA = TempFile( "interleaved_a.bin" );
	const auto pathB = TempFile( "interleaved_b.bin" );
	const size_t nTotal = 512u * 1024u;
	const size_t nChunk = 1000;

	{
		CFileStream a( pathA.string(), CFileStream::WIN_CREATE );
		CFileStream b( pathB.string(), CFileStream::WIN_CREATE );
		ASSERT_TRUE( a.IsOk() );
		ASSERT_TRUE( b.IsOk() );

		std::vector<unsigned char> chunk( nChunk );
		for ( size_t nWritten = 0; nWritten < nTotal; nWritten += nChunk )
		{
			const size_t n = (std::min)( nChunk, nTotal - nWritten );
			for ( size_t i = 0; i < n; ++i )
			{
				chunk[i] = ByteAt( nWritten + i );
			}
			a.Write( chunk.data(), static_cast<int>( n ) );
			b.Write( chunk.data(), static_cast<int>( n ) );
			ASSERT_TRUE( a.IsOk() ) << "stream a broke at " << nWritten;
			ASSERT_TRUE( b.IsOk() ) << "stream b broke at " << nWritten;
		}
	}

	ExpectPattern( pathA, nTotal );
	ExpectPattern( pathB, nTotal );
}

// A file of no length is readable and not broken, and its buffer is null.
// AllocBufImpl gets there by not asking for a mapping at all when the size is
// zero, and that skip is load-bearing: asking a mapping API for a zero length
// region is an error in most of them, so an implementation that always maps
// will fail here rather than return this.
TEST( MappedStream, EmptyFileHasNoBuffer )
{
	const auto path = TempFile( "empty.bin" );
	{
		CFileStream f( path.string(), CFileStream::WIN_CREATE );
		ASSERT_TRUE( f.IsOk() );
	}
	ASSERT_TRUE( std::filesystem::exists( path ) );
	EXPECT_EQ( std::filesystem::file_size( path ), 0u );

	CFileStream f( path.string(), CFileStream::WIN_READ_ONLY );
	EXPECT_TRUE( f.IsOk() ) << "an empty file is not a broken one";
	EXPECT_EQ( f.GetSize(), 0 );
	EXPECT_EQ( f.GetBuffer(), nullptr );
}

// A reader opened on a file that is not there must report itself broken rather
// than raise, because every caller tests IsOk and none catches.
TEST( MappedStream, MissingFileReportsBrokenRatherThanThrowing )
{
	const auto path = TempFile( "definitely_absent.bin" );
	std::error_code ec;
	std::filesystem::remove( path, ec );

	CFileStream f( path.string(), CFileStream::WIN_READ_ONLY );
	EXPECT_FALSE( f.IsOk() );
}
