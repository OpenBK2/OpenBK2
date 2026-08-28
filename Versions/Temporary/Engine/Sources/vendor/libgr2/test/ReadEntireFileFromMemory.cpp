// The entry point M1 starts at, and the contract it has to meet.
//
// It is the first Granny function the game calls, measured rather than assumed:
// a run against the stub logs GrannyReadEntireFileFromMemory as call 1. Nothing
// runs before it, not even the allocator. InitializeGrannyMemoryMap in
// 3Dmotor/GrannyMemoryMap.cpp is the only caller of GrannyGetAllocator and
// GrannySetAllocator, and nothing calls it, so those two are linked and never
// reached. Every other milestone is downstream of this one as well: nothing in
// M2, M3 or M4 can be exercised until a file parses.
//
// The engine reaches it from CGrannyMemFileLoader::RecalcValue, which has already
// pulled the resource out of a .pak into a CMemoryStream, so the from-memory form
// is the one that matters. GrannyReadEntireFile is the editor's path and
// SceneB2/TerraTools.cpp's, never the game's.
//
// Two groups of tests below, and the difference between them is the point.
//
// The REJECTION tests run today and pass today, because a stub that returns null
// rejects everything. They are still worth having now: they are the specification
// of what M1 must keep doing once it starts returning something, and they were
// cheaper to write while the shape of the header was fresh. StillAStub is what
// keeps that honest. It asserts the library has not been implemented yet, so the
// day M1 lands it fails and forces the DISABLED_ group to be enabled with it.
//
// The DISABLED_ tests are the acceptance criteria for M1. They cannot pass
// against a stub and are switched off rather than left red, so that a red suite
// means a regression rather than the normal state of affairs.
//
// What is deliberately not here is a positive test on real data. The corpus is
// 83,184 files of Nival's copyrighted data and cannot be committed; the sweep at
// the bottom takes a directory from the environment instead, and skips when it is
// not set. See docs/GrannyReplacement.md, "Test harness design".

#include "MinimalGr2.h"

#include <gr2/granny.h>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace NGr2Test;

namespace
{

granny_file *Read( const std::vector<uint8_t> &bytes )
{
	return GrannyReadEntireFileFromMemory( static_cast<granny_int32x>( bytes.size() ),
	                                       bytes.data() );
}

}

// A builder that produced the wrong layout would make every test below pass for
// the wrong reason, so it is checked against the arithmetic that measured it.

TEST( HeaderShapedFile, MatchesTheShippedLayout )
{
	const CHeaderShapedFile file;

	EXPECT_EQ( 440u, file.Bytes().size() ) << "8 sections, as in 7,127 of 7,442 shipped files";
	EXPECT_EQ( 0, memcmp( file.Data(), g_Magic, MAGIC_SIZE ) );
	EXPECT_EQ( 440u, file.GetU32( OFF_HEADER_SIZE ) );
	EXPECT_EQ( 0u, file.GetU32( OFF_HEADER_FORMAT ) );
	EXPECT_EQ( 6u, file.GetU32( OFF_VERSION ) );
	EXPECT_EQ( 56u, file.GetU32( OFF_SECTION_ARRAY_OFFSET ) )
		<< "relative to the header, not to the file";
	EXPECT_EQ( 88u, file.SectionArrayOffset() );
	EXPECT_EQ( 8u, file.GetU32( OFF_SECTION_ARRAY_COUNT ) );
	EXPECT_EQ( TYPE_TAG_13, file.GetU32( OFF_TYPE_TAG ) );

	// The 0x8000000f files are the ones with six.
	const CHeaderShapedFile six( 6 );
	EXPECT_EQ( 32u + 56u + 44u * 6u, six.Bytes().size() );
	EXPECT_EQ( six.Bytes().size(), six.GetU32( OFF_HEADER_SIZE ) );
}

TEST( HeaderShapedFile, SectionFieldsAreWhereTheRecordSaysTheyAre )
{
	CHeaderShapedFile file;

	// Writing the last field of section 7 must land inside the buffer and must
	// not touch section 6, which is what a wrong SECTION_RECORD_SIZE would do.
	file.SetSectionField( 7, SEC_MIXED_MARSHALLING_COUNT, 0xdeadbeef );
	EXPECT_EQ( 0xdeadbeefu, file.GetSectionField( 7, SEC_MIXED_MARSHALLING_COUNT ) );
	EXPECT_NE( 0xdeadbeefu, file.GetSectionField( 6, SEC_MIXED_MARSHALLING_COUNT ) );
	EXPECT_EQ( file.Bytes().size(),
	           file.SectionArrayOffset() + SECTION_RECORD_SIZE * 8 );
}

// Rejection: the loader has to survive input it cannot use.

TEST( ReadEntireFileFromMemory, RejectsNullMemory )
{
	EXPECT_EQ( nullptr, GrannyReadEntireFileFromMemory( 1024, nullptr ) );
}

TEST( ReadEntireFileFromMemory, RejectsZeroSize )
{
	const CHeaderShapedFile file;
	EXPECT_EQ( nullptr, GrannyReadEntireFileFromMemory( 0, file.Data() ) );
}

TEST( ReadEntireFileFromMemory, RejectsNegativeSize )
{
	// granny_int32x is signed, and CGrannyMemFileLoader::RecalcValue passes it
	// straight through from CFileStream::GetSize. A failed stat that returns -1
	// arrives here, and reading it as unsigned would ask for four gigabytes.
	const CHeaderShapedFile file;
	EXPECT_EQ( nullptr, GrannyReadEntireFileFromMemory( -1, file.Data() ) );
}

TEST( ReadEntireFileFromMemory, RejectsBufferShorterThanTheMagic )
{
	const CHeaderShapedFile file;
	for ( granny_int32x n = 1; n < static_cast<granny_int32x>( MAGIC_SIZE ); ++n )
	{
		EXPECT_EQ( nullptr, GrannyReadEntireFileFromMemory( n, file.Data() ) ) << "size " << n;
	}
}

TEST( ReadEntireFileFromMemory, RejectsForeignMagic )
{
	// Not a corrupted GR2, a different file type sitting in the GR2 directory.
	EXPECT_EQ( nullptr, Read( ForeignMagicFile() ) );
}

TEST( ReadEntireFileFromMemory, RejectsAMagicThatIsOffByOneBit )
{
	// Every byte of the magic has to be checked, not just the first word. The
	// other Granny dialects, big endian and 64-bit pointer, differ from this one
	// in the later words, and this library implements neither.
	for ( uint32_t i = 0; i < MAGIC_SIZE; ++i )
	{
		CHeaderShapedFile file;
		file.Bytes()[i] ^= 0x01;
		EXPECT_EQ( nullptr, Read( file.Bytes() ) ) << "flipped bit 0 of magic byte " << i;
	}
}

TEST( ReadEntireFileFromMemory, RejectsAnUnsupportedVersion )
{
	// Format 6 in all 83,184 files surveyed. Anything else is a file this
	// library has never seen and must not guess at.
	for ( uint32_t nVersion : { 0u, 1u, 5u, 7u, 0xffffffffu } )
	{
		CHeaderShapedFile file;
		file.SetU32( OFF_VERSION, nVersion );
		EXPECT_EQ( nullptr, Read( file.Bytes() ) ) << "version " << nVersion;
	}
}

TEST( ReadEntireFileFromMemory, RejectsATruncatedSectionArray )
{
	// The header promises 8 sections and the buffer holds 7 and a half.
	CHeaderShapedFile file;
	file.TruncateTo( static_cast<uint32_t>( file.Bytes().size() ) - SECTION_RECORD_SIZE / 2 );
	EXPECT_EQ( nullptr, Read( file.Bytes() ) );
}

TEST( ReadEntireFileFromMemory, RejectsASectionCountThatCannotFit )
{
	// The multiplication that sizes the section array is the classic overflow
	// site: 0x0fffffff * 44 wraps on 32 bits, and a loader that trusts the wrapped
	// product reads far past the buffer.
	for ( uint32_t nCount : { 9u, 0x00ffffffu, 0x0fffffffu, 0xffffffffu } )
	{
		CHeaderShapedFile file;
		file.SetU32( OFF_SECTION_ARRAY_COUNT, nCount );
		EXPECT_EQ( nullptr, Read( file.Bytes() ) ) << "sectionArrayCount " << nCount;
	}
}

TEST( ReadEntireFileFromMemory, RejectsASectionArrayOffsetPastTheBuffer )
{
	for ( uint32_t nOffset : { 1024u, 0x7fffffffu, 0xffffffffu } )
	{
		CHeaderShapedFile file;
		file.SetU32( OFF_SECTION_ARRAY_OFFSET, nOffset );
		EXPECT_EQ( nullptr, Read( file.Bytes() ) ) << "sectionArrayOffset " << nOffset;
	}
}

TEST( ReadEntireFileFromMemory, RejectsSectionDataPastTheBuffer )
{
	CHeaderShapedFile file;
	file.SetSectionField( 3, SEC_DATA_OFFSET, 0x7fff0000 );
	file.SetSectionField( 3, SEC_DATA_SIZE, 64 );
	file.SetSectionField( 3, SEC_EXPANDED_DATA_SIZE, 64 );
	EXPECT_EQ( nullptr, Read( file.Bytes() ) );
}

TEST( ReadEntireFileFromMemory, RejectsAnUnknownCompressionCodec )
{
	// 0, 1 and 2 are what the corpus contains. A fourth value means either a
	// corrupt file or a dialect with a codec this library does not have, and
	// both have to fail rather than fall through to whichever branch is last.
	CHeaderShapedFile file;
	file.SetSectionField( 0, SEC_COMPRESSION, COMPRESSION_MAX_KNOWN + 1 );
	file.SetSectionField( 0, SEC_DATA_SIZE, 16 );
	file.SetSectionField( 0, SEC_EXPANDED_DATA_SIZE, 32 );
	EXPECT_EQ( nullptr, Read( file.Bytes() ) );
}

TEST( ReadEntireFileFromMemory, SurvivesEveryPrefixOfAWellShapedFile )
{
	// A .pak entry that was written short, or a read that stopped early, arrives
	// as a prefix. Every one of them has to return rather than walk off the end,
	// and the loop is cheap enough to be exhaustive.
	const CHeaderShapedFile file;
	const uint32_t nTotal = static_cast<uint32_t>( file.Bytes().size() );
	for ( uint32_t n = 0; n <= nTotal; ++n )
	{
		const std::vector<uint8_t> prefix(
			file.Bytes().begin(), file.Bytes().begin() + static_cast<ptrdiff_t>( n ) );
		if ( n < nTotal )
		{
			EXPECT_EQ( nullptr, Read( prefix ) ) << "prefix of " << n << " bytes";
		}
		else
		{
			// The whole thing. What it returns is the M1 question; that it
			// returns at all is this test's.
			GrannyFreeFile( Read( prefix ) );
		}
	}
}

TEST( ReadEntireFileFromMemory, TheNullItReturnsCanBeFedBackIn )
{
	// CGrannyMemFileLoader::RecalcValue stores the result unconditionally and
	// CGrannyFile::~CGrannyFile frees it, so a rejected file becomes a free of
	// whatever was returned. GrannyGetFileInfo is reached the same way from
	// GAnimFormat.cpp.
	GrannyFreeFile( nullptr );
	EXPECT_EQ( nullptr, GrannyGetFileInfo( nullptr ) );
}


TEST( ReadEntireFileFromMemory, StillAStub )
{
	// Delete this when M1 lands, and enable the DISABLED_ tests below in the same
	// commit. Until then it is what says the rejection tests above pass for the
	// trivial reason, so that nobody reads a green suite as a working loader.
	const CHeaderShapedFile file;
	EXPECT_EQ( nullptr, Read( file.Bytes() ) )
		<< "GrannyReadEntireFileFromMemory returned something: M1 has been "
		   "implemented, so enable the DISABLED_ tests and delete this one";
}

// Acceptance: what M1 has to do.

TEST( ReadEntireFileFromMemory, DISABLED_AcceptsAWellShapedFile )
{
	const CHeaderShapedFile file;
	granny_file *pFile = Read( file.Bytes() );
	ASSERT_NE( nullptr, pFile );
	EXPECT_NE( nullptr, GrannyGetFileInfo( pFile ) );
	GrannyFreeFile( pFile );
}

TEST( ReadEntireFileFromMemory, DISABLED_AcceptsEveryTagInTheCorpus )
{
	for ( uint32_t nTag : { TYPE_TAG_0F, TYPE_TAG_10, TYPE_TAG_11, TYPE_TAG_13 } )
	{
		CHeaderShapedFile file( nTag == TYPE_TAG_0F ? 6 : 8 );
		file.SetU32( OFF_TYPE_TAG, nTag );
		granny_file *pFile = Read( file.Bytes() );
		EXPECT_NE( nullptr, pFile ) << "tag " << std::hex << nTag;
		GrannyFreeFile( pFile );
	}
}

TEST( ReadEntireFileFromMemory, DISABLED_RejectsWrongCrc )
{
	// Open question, not a known requirement. The header carries a CRC at offset
	// 40 and nobody has established whether the real DLL checks it, over which
	// range, or with which polynomial. Answer that against the oracle before
	// enabling this, and if the answer is that it does not check, delete the test
	// rather than implementing a check the engine has never depended on.
	CHeaderShapedFile file;
	file.SetU32( OFF_CRC, 0x12345678 );
	EXPECT_EQ( nullptr, Read( file.Bytes() ) );
}


TEST( ReadEntireFileFromMemory, DISABLED_ReadsEveryFileInADirectory )
{
	// Point LIBGR2_TEST_GR2_DIR at a directory of extracted .gr2 resources. For
	// anything derived purely from a file the corpus is the input space, so
	// enumeration is exhaustive rather than a sample. The directory is not
	// committed and never can be; extract one from a .pak, which is an ordinary
	// ZIP archive.
	// push/disable/pop rather than warning(suppress), which applies to the next
	// line only and so would land on the #endif instead of on the call.
#if defined( _MSC_VER )
	#pragma warning( push )
	#pragma warning( disable : 4996 )
#endif
	const char *pszDir = std::getenv( "LIBGR2_TEST_GR2_DIR" );
#if defined( _MSC_VER )
	#pragma warning( pop )
#endif
	if ( pszDir == nullptr )
	{
		GTEST_SKIP() << "LIBGR2_TEST_GR2_DIR is not set";
	}

	int nRead = 0;
	int nRejected = 0;
	for ( const auto &entry : std::filesystem::recursive_directory_iterator( pszDir ) )
	{
		if ( !entry.is_regular_file() )
		{
			continue;
		}

		std::ifstream in( entry.path(), std::ios::binary );
		const std::vector<uint8_t> bytes( ( std::istreambuf_iterator<char>( in ) ),
		                                  std::istreambuf_iterator<char>() );
		if ( bytes.size() < MAGIC_SIZE || memcmp( bytes.data(), g_Magic, MAGIC_SIZE ) != 0 )
		{
			// The non-GR2 files that share these directories. Their presence is
			// the point of RejectsForeignMagic, not of this sweep.
			continue;
		}

		granny_file *pFile = Read( bytes );
		if ( pFile != nullptr )
		{
			++nRead;
			GrannyFreeFile( pFile );
		}
		else
		{
			++nRejected;
			ADD_FAILURE() << "rejected " << entry.path().string();
		}
	}

	EXPECT_GT( nRead, 0 ) << "no GR2 files found under " << pszDir;
	EXPECT_EQ( 0, nRejected );
}
