// Covers the two mapping paths CFileStream does not reach.
//
// CMemoryMappedFileFragment maps a window of an already-mapped file, and the
// offset it is given is arbitrary while the underlying call can only map at a
// multiple of the allocation granularity. Whoever implements it therefore maps
// from the aligned offset below and hands back a pointer shifted forward, and
// getting that shift wrong returns real bytes from the wrong place, which no
// crash reports and no checksum in this codebase would notice. Every archive
// read goes through here.
//
// The second path is opening a file for writing without meaning to destroy it.
// CFileStream's WIN_CREATE truncates on purpose, so it cannot tell the
// difference; the VFS opens read-write and expects the contents to survive.

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "Misc/Asserts.h"
// FileReaders.h still calls MapViewOfFile and friends from its inline methods,
// so it needs the Windows headers in scope. Replacing that with a portable
// mapping API is what removes this include.
#include <windows.h>

#include "System/FileReaders.h"

#include <gtest/gtest.h>

namespace {

unsigned char ByteAt( size_t nOffset )
{
	return static_cast<unsigned char>( ( nOffset * 31u + ( nOffset >> 13 ) ) & 0xff );
}

std::filesystem::path TempFile( const char *pszName )
{
	const std::filesystem::path dir =
		std::filesystem::temp_directory_path() / "bk2_mappedfragment_test";
	std::error_code ec;
	std::filesystem::create_directories( dir, ec );
	return dir / pszName;
}

// Written with plain stdio so the file under test owes nothing to the code
// under test.
void MakeFile( const std::filesystem::path &path, size_t nSize )
{
	std::vector<unsigned char> all( nSize );
	for ( size_t i = 0; i < nSize; ++i )
	{
		all[i] = ByteAt( i );
	}
	std::FILE *f = std::fopen( path.string().c_str(), "wb" );
	ASSERT_NE( f, nullptr );
	if ( nSize )
	{
		ASSERT_EQ( std::fwrite( all.data(), 1, nSize, f ), nSize );
	}
	std::fclose( f );
}

} // namespace

// A window at an arbitrary offset must contain exactly the file's bytes for
// that range. The offsets below straddle both the 4K page and the 64K
// allocation granularity, which is where a mistaken shift shows up.
TEST( MappedFragment, WindowsAtManyOffsetsReadTheRightBytes )
{
	const auto path = TempFile( "fragments.bin" );
	const size_t nFileSize = 300u * 1024u;
	MakeFile( path, nFileSize );

	CMMFile file( path.string().c_str(), STREAM_ACCESS_READ );
	ASSERT_TRUE( file.IsOk() );
	ASSERT_EQ( static_cast<size_t>( file.GetFileSize() ), nFileSize );
	file.MapFile( static_cast<int>( nFileSize ), false );

	const size_t offsets[] = { 0, 1, 17, 4095, 4096, 4097, 65535, 65536, 65537,
	                           131072, 131073, 200000, nFileSize - 1 };
	for ( const size_t nOffset : offsets )
	{
		for ( const size_t nWanted : { size_t( 1 ), size_t( 100 ), size_t( 5000 ),
		                               size_t( 70000 ) } )
		{
			if ( nOffset + nWanted > nFileSize )
			{
				continue;
			}
			SCOPED_TRACE( "offset " + std::to_string( nOffset ) +
			              ", length " + std::to_string( nWanted ) );

			CMemoryMappedFileFragment frag( &file, static_cast<int>( nOffset ),
			                                static_cast<int>( nWanted ) );
			ASSERT_TRUE( frag.IsOk() );
			ASSERT_EQ( static_cast<size_t>( frag.GetSize() ), nWanted );

			const unsigned char *p = frag.GetBuffer();
			ASSERT_NE( p, nullptr );
			for ( size_t i = 0; i < nWanted; ++i )
			{
				if ( p[i] != ByteAt( nOffset + i ) )
				{
					FAIL() << "byte " << i << " of the window came from file offset "
					       << "somewhere other than " << ( nOffset + i );
				}
			}
		}
	}

	file.UnmapFile();
}

// Several windows alive at once, which is what reading an archive does: the
// central directory stays mapped while individual entries are opened.
TEST( MappedFragment, OverlappingWindowsCoexist )
{
	const auto path = TempFile( "overlap.bin" );
	const size_t nFileSize = 128u * 1024u;
	MakeFile( path, nFileSize );

	CMMFile file( path.string().c_str(), STREAM_ACCESS_READ );
	ASSERT_TRUE( file.IsOk() );
	file.MapFile( static_cast<int>( nFileSize ), false );

	CMemoryMappedFileFragment a( &file, 0, 1024 );
	CMemoryMappedFileFragment b( &file, 70000, 1024 );
	CMemoryMappedFileFragment c( &file, 70500, 1024 );	// overlaps b

	ASSERT_TRUE( a.IsOk() && b.IsOk() && c.IsOk() );
	for ( size_t i = 0; i < 1024; ++i )
	{
		ASSERT_EQ( a.GetBuffer()[i], ByteAt( i ) ) << "window a, byte " << i;
		ASSERT_EQ( b.GetBuffer()[i], ByteAt( 70000 + i ) ) << "window b, byte " << i;
		ASSERT_EQ( c.GetBuffer()[i], ByteAt( 70500 + i ) ) << "window c, byte " << i;
	}

	file.UnmapFile();
}

// Opening for writing must not empty the file. The VFS opens existing files
// read-write and reads them, so a truncating open loses the contents and
// everything downstream sees a file of no length.
TEST( MappedFragment, OpeningForWritingKeepsTheContents )
{
	const auto path = TempFile( "preserve.bin" );
	const size_t nFileSize = 8192;
	MakeFile( path, nFileSize );

	{
		CMMFile file( path.string().c_str(), STREAM_ACCESS_READ_WRITE );
		ASSERT_TRUE( file.IsOk() );
		EXPECT_EQ( static_cast<size_t>( file.GetFileSize() ), nFileSize )
			<< "opening for writing truncated the file";
	}

	EXPECT_EQ( std::filesystem::file_size( path ), nFileSize )
		<< "the file was still there after the handle closed";
}

// The same thing through the stream, which is how the VFS reaches it: a
// read-write stream on an existing file can read what was already in it.
TEST( MappedFragment, ReadWriteStreamSeesExistingContents )
{
	const auto path = TempFile( "rwstream.bin" );
	const size_t nFileSize = 4096;
	MakeFile( path, nFileSize );

	CMemoryMappedFile f( path.string().c_str(), STREAM_ACCESS_READ_WRITE );
	ASSERT_TRUE( f.IsOk() );
	ASSERT_EQ( static_cast<size_t>( f.GetSize() ), nFileSize )
		<< "the stream reports no contents, so the open truncated";

	const unsigned char *p = f.GetBuffer();
	ASSERT_NE( p, nullptr );
	for ( size_t i = 0; i < nFileSize; ++i )
	{
		ASSERT_EQ( p[i], ByteAt( i ) ) << "byte " << i;
	}
}

// A file that is not there is a broken stream, not an exception.
TEST( MappedFragment, MissingFileIsNotOk )
{
	const auto path = TempFile( "absent.bin" );
	std::error_code ec;
	std::filesystem::remove( path, ec );

	CMMFile file( path.string().c_str(), STREAM_ACCESS_READ );
	EXPECT_FALSE( file.IsOk() );
}
