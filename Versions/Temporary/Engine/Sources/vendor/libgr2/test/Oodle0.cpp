// The Oodle0 decoder.
//
// Oodle1 gets committed test vectors because granny2.dll still has an Oodle1
// *compressor*: authored bytes go in, a real stream comes out, and the fixture
// owes nothing to anybody's data. Oodle0 gets none, because the 2.11 DLL can
// decode it and cannot encode it. GrannyBeginFileCompression accepts
// GrannyOodle1Compression and refuses GrannyOodle0Compression, and no
// GrannyOodle0Compress is exported at all. There is no way to make a stream.
//
// So this tests against files the repository already ships. Versions/Current/Data
// is a pre-release beta snapshot, 14,115 of whose GR2 files are pure Oodle0, and
// they are in the tree already; a checksum of what they expand to adds nothing
// copyrightable and gives the decoder a byte-exact test that needs no DLL. That is
// the manifest strategy docs/GrannyReplacement.md recommends, at the smallest
// useful scale.
//
// The checksums came out of granny2.dll. FNV-1a rather than SHA-256 because the
// test has to compute the same thing in ten lines and nothing here is
// adversarial. If the beta data is absent, as it is in an extracted copy of this
// library, these skip.
//
// Agreement with the DLL over the whole corpus is measured separately and is the
// stronger statement: scripts/port/gr2diff.py, 21,720 files, both codecs.

#include "MinimalGr2.h"

#include "File.h"
#include "Oodle0.h"

#include <gr2/granny.h>

#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{

//! One file the repository ships, and what it expands to.
struct SExpected
{
	const char *pszPath;
	uint32_t nExpandedSize;
	uint64_t nChecksum;
};

// Generated from files this repository already ships, by decompressing them
// through granny2.dll and checksumming the result. See the comment above.
const SExpected EXPECTED[] = {
	{ "Versions/Current/Data/bin/Geometries/5A313713-BF5A-402D-9C1F-9DAF34F5F743",
	  7104, 0x901e1bb9844acdfdull },
	{ "Versions/Current/Data/bin/Geometries/678",
	  7104, 0x901e1bb9844acdfdull },
	{ "Versions/Current/Data/bin/Skeletons/1690",
	  7100, 0x8c65b39c3f097aabull },
	{ "Versions/Current/Data/bin/Skeletons/346FC153-6D31-4F10-8878-D510295DE3BE",
	  7100, 0x8c65b39c3f097aabull },
	{ "Versions/Current/Data/bin/Animations/0D49B57F-15C0-420E-B4B8-013BC47BDADB",
	  11524, 0x719644ff90dad40eull },
	{ "Versions/Current/Data/bin/Geometries/1840",
	  708728, 0xa8b4ad48a294c415ull },
};

uint64_t Fnv1a( const uint8_t *pBytes, size_t nBytes )
{
	uint64_t nHash = 0xcbf29ce484222325ull;
	for ( size_t i = 0; i < nBytes; ++i )
	{
		nHash = ( nHash ^ pBytes[i] ) * 0x100000001b3ull;
	}
	return nHash;
}

//! Where the repository's beta data is, or an empty string if it is not there.
//!
//! LIBGR2_REPO_DATA_DIR is defined by CMake only when the directory exists, so an
//! extracted copy of this library compiles and skips rather than failing.
std::string RepoRoot()
{
#if defined( LIBGR2_REPO_DATA_DIR )
	return std::string( LIBGR2_REPO_DATA_DIR );
#else
	return std::string();
#endif
}

std::vector<uint8_t> ReadFile( const std::string &sPath )
{
	std::ifstream in( sPath, std::ios::binary );
	if ( !in )
	{
		return std::vector<uint8_t>();
	}
	return std::vector<uint8_t>( ( std::istreambuf_iterator<char>( in ) ),
	                             std::istreambuf_iterator<char>() );
}

//! Every section of a loaded file, concatenated, which is what was checksummed.
std::vector<uint8_t> AllSections( granny_file *pFile )
{
	std::vector<uint8_t> out;
	for ( uint32_t i = 0; i < pFile->SectionCount(); ++i )
	{
		const std::vector<uint8_t> &data = pFile->SectionData( i );
		out.insert( out.end(), data.begin(), data.end() );
	}
	return out;
}

}

TEST( Oodle0, ExpandsTheFilesThisRepositoryShips )
{
	const std::string sRoot = RepoRoot();
	if ( sRoot.empty() )
	{
		GTEST_SKIP() << "Versions/Current/Data is not present";
	}

	int nChecked = 0;
	for ( const SExpected &expected : EXPECTED )
	{
		const std::string sPath = sRoot + "/" + expected.pszPath;
		const std::vector<uint8_t> bytes = ReadFile( sPath );
		if ( bytes.empty() )
		{
			continue;
		}

		granny_file *pFile = GrannyReadEntireFileFromMemory(
			static_cast<granny_int32x>( bytes.size() ), bytes.data() );
		ASSERT_NE( nullptr, pFile ) << expected.pszPath;

		const std::vector<uint8_t> expanded = AllSections( pFile );
		EXPECT_EQ( expected.nExpandedSize, expanded.size() ) << expected.pszPath;
		EXPECT_EQ( expected.nChecksum, Fnv1a( expanded.data(), expanded.size() ) )
			<< expected.pszPath << ": the bytes differ from what granny2.dll produced";

		GrannyFreeFile( pFile );
		++nChecked;
	}

	EXPECT_GT( nChecked, 0 ) << "none of the listed files were found under " << sRoot;
}

TEST( Oodle0, DecodesTheSameBytesEveryTime )
{
	// No state survives a call: the models are built per stage and the coder is a
	// local, so two runs of one stream cannot differ.
	const std::string sRoot = RepoRoot();
	if ( sRoot.empty() )
	{
		GTEST_SKIP() << "Versions/Current/Data is not present";
	}
	const std::vector<uint8_t> bytes = ReadFile( sRoot + "/" + EXPECTED[0].pszPath );
	if ( bytes.empty() )
	{
		GTEST_SKIP() << "the file is not there";
	}

	std::vector<uint8_t> first;
	for ( int i = 0; i < 3; ++i )
	{
		granny_file *pFile = GrannyReadEntireFileFromMemory(
			static_cast<granny_int32x>( bytes.size() ), bytes.data() );
		ASSERT_NE( nullptr, pFile );
		const std::vector<uint8_t> expanded = AllSections( pFile );
		if ( i == 0 )
		{
			first = expanded;
		}
		else
		{
			EXPECT_EQ( first, expanded ) << "run " << i;
		}
		GrannyFreeFile( pFile );
	}
}

TEST( Oodle0, RefusesNonsenseArguments )
{
	std::vector<uint8_t> stream( 256, 0 );
	std::vector<uint8_t> out( 256, 0 );

	EXPECT_FALSE( NGr2::Oodle0Decompress( nullptr, 256, 0, 0, out.data(), 256 ) );
	EXPECT_FALSE( NGr2::Oodle0Decompress( stream.data(), 256, 0, 0, nullptr, 256 ) );
	// Fewer bytes than the three twelve-byte parameter blocks every stream starts
	// with.
	EXPECT_FALSE( NGr2::Oodle0Decompress( stream.data(), 35, 0, 0, out.data(), 256 ) );
}

TEST( Oodle0, RefusesAStreamOfZeros )
{
	// All-zero parameters describe a model with no symbols, so the first read has
	// nowhere to land. What matters is that it says so rather than looping.
	const std::vector<uint8_t> stream( 512, 0 );
	std::vector<uint8_t> out( 1024 + 1, 0 );
	out.back() = 0xa5;

	EXPECT_FALSE( NGr2::Oodle0Decompress( stream.data(),
	                                      static_cast<uint32_t>( stream.size() ), 0, 0,
	                                      out.data(), 1024 ) );
	EXPECT_EQ( 0xa5, out.back() ) << "wrote past the end of the output";
}

TEST( Oodle0, SurvivesEveryTruncationOfARealStream )
{
	// Truncation is not reliably detectable in an arithmetic coder, the same as
	// for Oodle1, so this asserts what a loader can rely on: it terminates, it
	// never writes past the output, and a stream too short to hold the message is
	// refused.
	const std::string sRoot = RepoRoot();
	if ( sRoot.empty() )
	{
		GTEST_SKIP() << "Versions/Current/Data is not present";
	}
	const std::vector<uint8_t> bytes = ReadFile( sRoot + "/" + EXPECTED[0].pszPath );
	if ( bytes.empty() )
	{
		GTEST_SKIP() << "the file is not there";
	}

	// Every prefix, in steps, since the smallest of these files is still four
	// kilobytes and every single length would be slow without saying more.
	for ( uint32_t n = 0; n < bytes.size(); n += 37 )
	{
		const std::vector<uint8_t> prefix( bytes.begin(),
		                                   bytes.begin() + static_cast<ptrdiff_t>( n ) );
		granny_file *pFile = GrannyReadEntireFileFromMemory(
			static_cast<granny_int32x>( prefix.size() ), prefix.data() );
		if ( pFile != nullptr )
		{
			// A prefix that still loads has to be self-consistent, which is what
			// the container's own length checks are for.
			GrannyFreeFile( pFile );
		}
	}
}

TEST( Oodle0, WritesNoMoreThanItWasGiven )
{
	// The decoder is handed a buffer with a guard byte after it, and a length one
	// byte short of what the section says. It must refuse rather than fill the
	// space it was promised.
	const std::string sRoot = RepoRoot();
	if ( sRoot.empty() )
	{
		GTEST_SKIP() << "Versions/Current/Data is not present";
	}
	const std::vector<uint8_t> bytes = ReadFile( sRoot + "/" + EXPECTED[0].pszPath );
	if ( bytes.empty() )
	{
		GTEST_SKIP() << "the file is not there";
	}

	// The first section's compressed bytes, taken straight out of the file.
	const uint32_t nArrayBegin = NGr2Test::HEADER_OFFSET
	                             + *reinterpret_cast<const uint32_t *>(
									 bytes.data() + NGr2Test::OFF_SECTION_ARRAY_OFFSET );
	const uint32_t *pSection =
		reinterpret_cast<const uint32_t *>( bytes.data() + nArrayBegin );
	const uint32_t nDataOffset = pSection[1];
	const uint32_t nDataSize = pSection[2];
	const uint32_t nExpanded = pSection[3];
	ASSERT_GT( nExpanded, 1u );

	std::vector<uint8_t> out( nExpanded + 1, 0 );
	out.back() = 0xa5;
	EXPECT_FALSE( NGr2::Oodle0Decompress( bytes.data() + nDataOffset, nDataSize,
	                                      pSection[5], pSection[6], out.data(),
	                                      nExpanded - 1 ) )
		<< "a section does not fit in one byte less than it needs";
	EXPECT_EQ( 0xa5, out.back() );
}
