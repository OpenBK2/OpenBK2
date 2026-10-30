// Oodle1 against every Oodle1 section this repository's data holds, locked to a
// checksum.
//
// Oodle1.cpp has unit tests already, in Oodle1.cpp beside this file: streams the
// real compressor produced, committed as hex, runnable with no data and no DLL.
// They cover the paths through the decoder. They do not cover the thing that
// actually goes wrong when somebody optimises it, which is a change that decodes
// almost everything correctly and one section differently.
//
// So this is the other half: 4,246 sections, 59.3 MB expanded, reduced to one
// 64-bit number. Any change to the decoder that alters a single output byte moves
// it. That is the whole purpose, and it is why the expected value is written
// here as a literal rather than computed from anything.
//
// The checksum covers each section's success flag and expanded length as well as
// its bytes, so a decoder that gives up early on a section cannot match it by
// returning fewer bytes that happen to hash the same. The output buffer is
// poisoned before each section, so a decoder that reads uninitialised output
// shows up here rather than depending on what the allocator last left behind.
//
// Needs the repository's data, so like the Oodle0 checksum test it skips when
// LIBGR2_REPO_DATA_DIR is undefined, which is what lets an extracted copy of
// this library still compile and pass. scripts/port/oodle1-corpus-bench.cpp is
// the same walk with timing instead of assertions, for working on a data tree
// this test does not know about.

#include "Oodle1.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

namespace
{

// From src/File.h. The section table is read directly here: the loader in
// File.cpp also builds the type tree and applies pointer fixups, and this test is
// about the decoder alone.
constexpr uint32_t MAGIC_SIZE = 16;
constexpr uint32_t HEADER_OFFSET = 32;
constexpr uint32_t SECTION_RECORD_SIZE = 44;
constexpr uint32_t COMPRESSION_OODLE1 = 2;

const uint8_t FILE_MAGIC[MAGIC_SIZE] = { 0xb8, 0x67, 0xb0, 0xca, 0xf8, 0x6d, 0xb1, 0x0f,
                                         0x84, 0x72, 0x8c, 0x7e, 0x5e, 0x19, 0x00, 0x1e };

//! What Versions/Current/Data/bin decodes to, measured 2026-10-30.
//!
//! If this test fails, the decoder's output changed. That is either a bug or a
//! deliberate correctness fix; it is never a performance change, because a
//! performance change that moves this number is wrong by definition. Do not
//! update the constant to make the test pass without establishing which of the
//! two old and new answers agrees with granny2.dll, via
//! scripts/port/gr2diff.py.
constexpr uint64_t EXPECTED_CHECKSUM = 0x3176dbbfc52a3299ull;

//! How many sections that data held when the checksum was taken.
//!
//! Checked separately so that data changing underneath is distinguishable from
//! the decoder changing. A different count fails with its own message instead of
//! looking like a decoder regression.
constexpr size_t EXPECTED_SECTIONS = 4246;

std::string RepoRoot()
{
#if defined( LIBGR2_REPO_DATA_DIR )
	return std::string( LIBGR2_REPO_DATA_DIR );
#else
	return std::string();
#endif
}

uint32_t ReadU32( const std::vector<uint8_t> &bytes, size_t nAt )
{
	uint32_t nValue = 0;
	memcpy( &nValue, bytes.data() + nAt, sizeof( nValue ) );
	return nValue;
}

inline void Hash( uint64_t &nHash, const uint8_t *pBytes, size_t nCount )
{
	for ( size_t i = 0; i < nCount; ++i )
	{
		nHash = ( nHash ^ pBytes[i] ) * 0x100000001b3ull;
	}
}

inline void HashU32( uint64_t &nHash, uint32_t nValue )
{
	uint8_t bytes[4];
	memcpy( bytes, &nValue, sizeof( bytes ) );
	Hash( nHash, bytes, sizeof( bytes ) );
}

struct SResult
{
	uint64_t nHash = 0xcbf29ce484222325ull;
	size_t nSections = 0;
	size_t nFailures = 0;
	uint64_t nExpandedBytes = 0;
	//! The first section that failed to decode, for a message worth reading.
	std::string sFirstFailure;
};

//! Decode every Oodle1 section under a directory, in directory iteration order.
//!
//! Order matters to the checksum, and recursive_directory_iterator is ordered
//! consistently for an unchanged tree on a given filesystem, which is enough for
//! a regression test on one repository. It is not a stable ordering across
//! machines, so a mismatch on a new machine with the same data means the ordering
//! differed rather than the decoder: the per-section count above is the first
//! thing to check.
SResult DecodeEverything( const std::string &sDir )
{
	SResult result;
	std::vector<uint8_t> out;

	std::error_code error;
	std::filesystem::recursive_directory_iterator walk( sDir, error );
	if ( error )
	{
		return result;
	}

	for ( const auto &entry : walk )
	{
		if ( !entry.is_regular_file() )
		{
			continue;
		}

		std::ifstream in( entry.path(), std::ios::binary );
		const std::vector<uint8_t> bytes( ( std::istreambuf_iterator<char>( in ) ),
		                                  std::istreambuf_iterator<char>() );
		if ( bytes.size() < HEADER_OFFSET + 20
		     || memcmp( bytes.data(), FILE_MAGIC, MAGIC_SIZE ) != 0 )
		{
			continue;
		}

		const uint32_t nSectionArrayOffset = ReadU32( bytes, HEADER_OFFSET + 12 );
		const uint32_t nSectionCount = ReadU32( bytes, HEADER_OFFSET + 16 );
		const size_t nArrayBegin = HEADER_OFFSET + nSectionArrayOffset;
		if ( nArrayBegin + size_t( nSectionCount ) * SECTION_RECORD_SIZE > bytes.size() )
		{
			continue;
		}

		for ( uint32_t i = 0; i < nSectionCount; ++i )
		{
			const size_t nBase = nArrayBegin + SECTION_RECORD_SIZE * i;
			const uint32_t nCompression = ReadU32( bytes, nBase + 0 );
			const uint32_t nDataOffset = ReadU32( bytes, nBase + 4 );
			const uint32_t nCompressedSize = ReadU32( bytes, nBase + 8 );
			const uint32_t nExpandedSize = ReadU32( bytes, nBase + 12 );
			// first16Bit and first8Bit, the two stage boundaries the codec needs.
			const uint32_t nStop0 = ReadU32( bytes, nBase + 20 );
			const uint32_t nStop1 = ReadU32( bytes, nBase + 24 );

			if ( nCompression != COMPRESSION_OODLE1 || nExpandedSize == 0
			     || nDataOffset + size_t( nCompressedSize ) > bytes.size() )
			{
				continue;
			}

			// 0xcd rather than 0, so output the decoder failed to write is visible in
			// the checksum instead of silently matching a zeroed buffer.
			out.assign( nExpandedSize, 0xcd );
			const bool bOk =
				NGr2::Oodle1Decompress( bytes.data() + nDataOffset, nCompressedSize, nStop0,
				                        nStop1, out.data(), nExpandedSize );
			if ( !bOk )
			{
				++result.nFailures;
				if ( result.sFirstFailure.empty() )
				{
					result.sFirstFailure =
						entry.path().string() + " section " + std::to_string( i );
				}
			}

			HashU32( result.nHash, bOk ? 1u : 0u );
			HashU32( result.nHash, nExpandedSize );
			Hash( result.nHash, out.data(), nExpandedSize );
			++result.nSections;
			result.nExpandedBytes += nExpandedSize;
		}
	}
	return result;
}

TEST( Oodle1Corpus, DecodesTheShippedDataToTheSameBytes )
{
	const std::string sRoot = RepoRoot();
	if ( sRoot.empty() )
	{
		GTEST_SKIP() << "built without LIBGR2_REPO_DATA_DIR, so there is no data to read";
	}
	const std::string sDir = sRoot + "/Versions/Current/Data/bin";
	if ( !std::filesystem::is_directory( sDir ) )
	{
		GTEST_SKIP() << sDir << " is not there";
	}

	const SResult result = DecodeEverything( sDir );

	ASSERT_GT( result.nSections, 0u ) << "no Oodle1 sections found under " << sDir;
	EXPECT_EQ( result.nFailures, 0u )
		<< result.nFailures << " sections did not decode, first at " << result.sFirstFailure;
	EXPECT_EQ( result.nSections, EXPECTED_SECTIONS )
		<< "the data changed, not necessarily the decoder: " << result.nSections
		<< " Oodle1 sections now against " << EXPECTED_SECTIONS << " when the checksum "
		<< "below was taken. Check that before reading the checksum result.";
	EXPECT_EQ( result.nHash, EXPECTED_CHECKSUM )
		<< "Oodle1 decodes this data differently than it did on 2026-10-30. "
		<< "Decoded " << result.nExpandedBytes << " bytes from " << result.nSections
		<< " sections. See the comment on EXPECTED_CHECKSUM before touching it.";
}

}
