// Times Oodle1 decompression over every Oodle1 section the game data holds, and
// fingerprints the output so a faster decoder can be proved to be the same
// decoder.
//
// This is the harness the 2026-10-30 decode work was measured on. It is kept so
// the numbers can be re-measured rather than re-argued, and because the next
// person to touch Oodle1.cpp needs exactly this: a way to find out in one command
// whether they changed the bytes.
//
// Build and run standalone, no CMake:
//
//   SRC=Versions/Temporary/Engine/Sources/vendor/libgr2/src
//   g++ -O2 -g -std=c++17 -I$SRC -o /tmp/oodle1bench
//       scripts/port/oodle1-corpus-bench.cpp $SRC/Oodle1.cpp
//   (one line; split here only because a trailing backslash in a // comment is
//    a -Wcomment warning)
//   /tmp/oodle1bench Versions/Current/Data/bin          # verify only
//   /tmp/oodle1bench Versions/Current/Data/bin 9        # and time 9 reps
//
// THE CHECKSUM IS THE POINT. It is FNV-1a over every output byte of every
// section in corpus order, plus each section's success flag and expanded length,
// so a variant that gives up on a section early or writes one byte differently
// cannot match it by accident. Oodle1 is half of a file format whose other half
// we do not have, so the decoder has to stay bit exact; a faster decoder that
// prints a different checksum is not a faster decoder.
//
// On the data in this repository as of 2026-10-30 the answer is
//
//   sections 4246  expanded 59.3 MB  failures 0
//   checksum 3176dbbfc52a3299
//
// and libgr2's Oodle1Corpus_test asserts that same value, so a regression is
// caught by ctest without anyone running this by hand. This program exists for
// the timing, and for working on a data tree the test does not know about.
//
// MEASURING WITH IT. Take the minimum, not the mean: interference on
// deterministic single threaded code is one sided, it can only make a run
// slower, so the minimum is the cleanest estimate. On the machine this was
// written on, individual runs varied by 17 per cent while the minimum of nine
// was stable to 0.2 per cent. To compare two builds use
// scripts/port/oodle1-measure.sh, which interleaves them rather than running one
// after the other; that script's header says why that matters.

#include "Oodle1.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>
#include <vector>

namespace
{

// From libgr2/src/File.h. Duplicated rather than included because this reads the
// section table directly: File.cpp's loader also builds the type tree and applies
// pointer fixups, neither of which a decode benchmark wants in its timing.
constexpr uint32_t MAGIC_SIZE = 16;
constexpr uint32_t HEADER_OFFSET = 32;
constexpr uint32_t SECTION_RECORD_SIZE = 44;
constexpr uint32_t COMPRESSION_OODLE1 = 2;

const uint8_t FILE_MAGIC[MAGIC_SIZE] = {
	0xb8, 0x67, 0xb0, 0xca, 0xf8, 0x6d, 0xb1, 0x0f, 0x84, 0x72, 0x8c, 0x7e, 0x5e, 0x19, 0x00, 0x1e };

struct SSection
{
	uint32_t nCompressedSize = 0;
	uint32_t nStop0 = 0;
	uint32_t nStop1 = 0;
	uint32_t nExpandedSize = 0;
	size_t nBlobAt = 0;
};

uint32_t ReadU32( const std::vector<uint8_t> &bytes, size_t nAt )
{
	uint32_t nValue = 0;
	memcpy( &nValue, bytes.data() + nAt, sizeof( nValue ) );
	return nValue;
}

//! Every Oodle1 section under a directory, compressed bytes copied into one blob.
//!
//! Collected up front so the timed loop touches nothing but the decoder: parsing
//! 15,000 files per repetition would otherwise dominate, and the page cache would
//! land in the numbers.
bool Collect( const char *szDir, std::vector<SSection> &sections, std::vector<uint8_t> &blobs )
{
	std::error_code error;
	std::filesystem::recursive_directory_iterator walk( szDir, error );
	if ( error )
	{
		fprintf( stderr, "cannot read %s: %s\n", szDir, error.message().c_str() );
		return false;
	}

	for ( const auto &entry : walk )
	{
		if ( !entry.is_regular_file() )
		{
			continue;
		}

		std::ifstream in( entry.path(), std::ios::binary );
		const std::vector<uint8_t> bytes( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
		if ( bytes.size() < HEADER_OFFSET + 20 || memcmp( bytes.data(), FILE_MAGIC, MAGIC_SIZE ) != 0 )
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

			SSection section;
			section.nCompressedSize = ReadU32( bytes, nBase + 8 );
			section.nExpandedSize = ReadU32( bytes, nBase + 12 );
			// first16Bit and first8Bit, the two stage boundaries the codec needs.
			section.nStop0 = ReadU32( bytes, nBase + 20 );
			section.nStop1 = ReadU32( bytes, nBase + 24 );

			if ( nCompression != COMPRESSION_OODLE1 || section.nExpandedSize == 0 ||
			     nDataOffset + size_t( section.nCompressedSize ) > bytes.size() )
			{
				continue;
			}

			section.nBlobAt = blobs.size();
			blobs.insert( blobs.end(), bytes.data() + nDataOffset, bytes.data() + nDataOffset + section.nCompressedSize );
			// Only to keep sections from abutting. The decoder reads a few bytes past
			// the compressed length, which is what GrannyGetCompressedBytesPaddingSize
			// reporting 4 is about, but it copies into its own padded buffer first.
			blobs.resize( ( blobs.size() + 7 ) & ~size_t( 7 ) );
			sections.push_back( section );
		}
	}
	return true;
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

//! One pass over every section. Returns elapsed nanoseconds.
//!
//! bFingerprint poisons the output buffer and hashes the result, which is the
//! verification pass; the timed passes skip both, so neither the memset nor the
//! hash lands in the measurement.
uint64_t DecodeAll( const std::vector<SSection> &sections,
                    const std::vector<uint8_t> &blobs,
                    std::vector<uint8_t> &out,
                    bool bFingerprint,
                    uint64_t &nHash,
                    uint32_t &nFailures )
{
	nHash = 0xcbf29ce484222325ull;
	nFailures = 0;

	const auto tStart = std::chrono::steady_clock::now();
	for ( const SSection &section : sections )
	{
		if ( bFingerprint )
		{
			// The decoder writes every byte it reports, so a variant that reads
			// uninitialised output shows up as a changed checksum rather than as a
			// result that depends on what the allocator last left here.
			memset( out.data(), 0xcd, section.nExpandedSize );
		}
		const bool bOk = NGr2::Oodle1Decompress( blobs.data() + section.nBlobAt,
		                                         section.nCompressedSize,
		                                         section.nStop0,
		                                         section.nStop1,
		                                         out.data(),
		                                         section.nExpandedSize );
		if ( !bOk )
		{
			++nFailures;
		}
		if ( bFingerprint )
		{
			HashU32( nHash, bOk ? 1u : 0u );
			HashU32( nHash, section.nExpandedSize );
			Hash( nHash, out.data(), section.nExpandedSize );
		}
	}
	const auto tEnd = std::chrono::steady_clock::now();
	return static_cast<uint64_t>( std::chrono::duration_cast<std::chrono::nanoseconds>( tEnd - tStart ).count() );
}

} // namespace

int main( int argc, char **argv )
{
	if ( argc < 2 )
	{
		fprintf( stderr, "usage: oodle1-corpus-bench <gr2-dir> [reps]\n" );
		fprintf( stderr, "  e.g. oodle1-corpus-bench Versions/Current/Data/bin 9\n" );
		return 2;
	}

	std::vector<SSection> sections;
	std::vector<uint8_t> blobs;
	if ( !Collect( argv[1], sections, blobs ) )
	{
		return 1;
	}
	if ( sections.empty() )
	{
		fprintf( stderr, "no Oodle1 sections under %s\n", argv[1] );
		return 1;
	}

	uint32_t nMaxExpanded = 0;
	uint64_t nTotalExpanded = 0;
	for ( const SSection &section : sections )
	{
		nMaxExpanded = section.nExpandedSize > nMaxExpanded ? section.nExpandedSize : nMaxExpanded;
		nTotalExpanded += section.nExpandedSize;
	}
	const double fMegabytes = double( nTotalExpanded ) / 1048576.0;

	std::vector<uint8_t> out( nMaxExpanded );
	uint64_t nHash = 0;
	uint32_t nFailures = 0;

	// Always verify, whatever the mode. A fast wrong decoder is not a result.
	DecodeAll( sections, blobs, out, true, nHash, nFailures );
	printf( "sections %zu  expanded %.1f MB  failures %u\n", sections.size(), fMegabytes, nFailures );
	printf( "checksum %016llx\n", static_cast<unsigned long long>( nHash ) );

	const int nReps = argc > 2 ? atoi( argv[2] ) : 0;
	if ( nReps <= 0 )
	{
		return nFailures == 0 ? 0 : 1;
	}

	uint64_t nBest = UINT64_MAX;
	for ( int nRep = 0; nRep < nReps; ++nRep )
	{
		uint64_t nIgnoredHash = 0;
		uint32_t nIgnoredFailures = 0;
		const uint64_t nElapsed = DecodeAll( sections, blobs, out, false, nIgnoredHash, nIgnoredFailures );
		nBest = nElapsed < nBest ? nElapsed : nBest;
		printf(
				"rep %-3d %9.1f ms  %6.1f MB/s\n", nRep, double( nElapsed ) / 1e6, fMegabytes / ( double( nElapsed ) / 1e9 ) );
	}
	// The line to read. Minimum, for the reason in the header comment.
	printf( "best    %9.1f ms  %6.1f MB/s\n", double( nBest ) / 1e6, fMegabytes / ( double( nBest ) / 1e9 ) );
	return nFailures == 0 ? 0 : 1;
}
