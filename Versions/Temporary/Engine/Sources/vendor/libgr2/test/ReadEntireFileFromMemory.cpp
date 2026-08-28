// The public loading entry points, and the contract they have to meet.
//
// GrannyReadEntireFileFromMemory is the first Granny function the game calls,
// measured rather than assumed: a run against the stub logged it as call 1.
// Nothing runs before it, not even the allocator. InitializeGrannyMemoryMap in
// 3Dmotor/GrannyMemoryMap.cpp is the only caller of GrannyGetAllocator and
// GrannySetAllocator, and nothing calls it, so those two are linked and never
// reached.
//
// The engine arrives here from CGrannyMemFileLoader::RecalcValue, which has
// already pulled the resource out of a .pak into a CMemoryStream, so the
// from-memory form is the one that matters. GrannyReadEntireFile is the editor's
// path and SceneB2/TerraTools.cpp's, never the game's.
//
// Most of what is below is refusal, which is the right proportion. Entries under
// bin/Geometries/ and its siblings are addressed by extensionless GUID and a
// handful of them are not GR2 at all, and per port/PORT_ROADMAP.md a failed
// resource load currently crashes the engine, so every one of these is a live
// hazard rather than a theoretical one.
//
// What is not here is a positive test on real data. The corpus is 83,184 files of
// Nival's copyrighted data and cannot be committed; the sweep at the bottom takes
// a directory from the environment instead and skips when it is unset. It also
// waits on the two Oodle codecs, without which almost no shipped file loads. See
// docs/GrannyReplacement.md, "Test harness design".

#include "MinimalGr2.h"

#include <gr2/granny.h>

#include <cstdio>
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

//! Read and free, for a test that only cares whether the file was accepted.
bool Accepts( const std::vector<uint8_t> &bytes )
{
	granny_file *pFile = Read( bytes );
	GrannyFreeFile( pFile );
	return pFile != 0;
}

}

// A builder that produced the wrong layout would make every test below pass for
// the wrong reason, so it is checked against the arithmetic that measured it.

TEST( HeaderShapedFile, MatchesTheShippedLayout )
{
	const CHeaderShapedFile file;

	EXPECT_EQ( 440u, file.Bytes().size() ) << "8 sections, as in almost every shipped file";
	EXPECT_EQ( 0, memcmp( file.Data(), g_Magic, MAGIC_SIZE ) );
	EXPECT_EQ( 440u, file.GetU32( OFF_HEADER_SIZE ) );
	EXPECT_EQ( 440u, file.GetU32( OFF_TOTAL_SIZE ) );
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

	// Writing the last field of section 7 must land inside the buffer and must not
	// touch section 6, which is what a wrong SECTION_RECORD_SIZE would do.
	file.SetSectionField( 7, SEC_MIXED_MARSHALLING_COUNT, 0xdeadbeef );
	EXPECT_EQ( 0xdeadbeefu, file.GetSectionField( 7, SEC_MIXED_MARSHALLING_COUNT ) );
	EXPECT_NE( 0xdeadbeefu, file.GetSectionField( 6, SEC_MIXED_MARSHALLING_COUNT ) );
	EXPECT_EQ( file.Bytes().size(), file.SectionArrayOffset() + SECTION_RECORD_SIZE * 8 );
}

TEST( HeaderShapedFile, SectionContentIsLaidOutAfterTheArray )
{
	// Fixups first, then the marshalling array, then the data, per section, which
	// is the order shipped files use.
	CHeaderShapedFile file;
	file.SetSectionData( 0, Pattern( 64 ) );
	file.AddPointerFixup( 0, 8, 0, 32 );

	const uint32_t nFixups = file.GetSectionField( 0, SEC_POINTER_FIXUP_OFFSET );
	const uint32_t nData = file.GetSectionField( 0, SEC_DATA_OFFSET );
	EXPECT_EQ( 440u, nFixups ) << "the first section's fixups start where the array ends";
	EXPECT_EQ( nFixups + POINTER_FIXUP_SIZE, file.GetSectionField( 0, SEC_MIXED_MARSHALLING_OFFSET ) );
	EXPECT_EQ( nFixups + POINTER_FIXUP_SIZE, nData );
	EXPECT_EQ( 64u, file.GetSectionField( 0, SEC_DATA_SIZE ) );
	EXPECT_EQ( 64u, file.GetSectionField( 0, SEC_EXPANDED_DATA_SIZE ) );
	EXPECT_EQ( nData + 64u, file.GetSectionField( 1, SEC_POINTER_FIXUP_OFFSET ) );
	EXPECT_EQ( file.Bytes().size(), file.GetU32( OFF_TOTAL_SIZE ) );
}

// Acceptance.

TEST( ReadEntireFileFromMemory, AcceptsAWellShapedFile )
{
	EXPECT_TRUE( Accepts( CHeaderShapedFile().Bytes() ) );
}

TEST( ReadEntireFileFromMemory, AcceptsAFileWithContentAndReferences )
{
	CHeaderShapedFile file;
	file.SetSectionData( 0, Pattern( 128 ) );
	file.SetSectionData( 6, Pattern( 64, 0x40 ) );
	file.AddPointerFixup( 0, 16, 6, 32 );
	file.AddPointerFixup( 0, 4, 0, 64 );
	EXPECT_TRUE( Accepts( file.Bytes() ) );
}

TEST( ReadEntireFileFromMemory, AcceptsEveryTagInTheCorpus )
{
	// Nothing dispatches on the tag yet. It selects the struct layouts the type
	// tree describes, and the type tree is what says what those are, so a reader
	// that resolves members by name does not need to branch on it.
	for ( uint32_t nTag : { TYPE_TAG_0F, TYPE_TAG_10, TYPE_TAG_11, TYPE_TAG_13 } )
	{
		CHeaderShapedFile file( nTag == TYPE_TAG_0F ? 6 : 8 );
		file.SetTypeTag( nTag );
		EXPECT_TRUE( Accepts( file.Bytes() ) ) << "tag " << std::hex << nTag;
	}
}

TEST( ReadEntireFileFromMemory, AcceptsAPointerTargetOnePastTheEnd )
{
	// A pointer to an empty array points at the byte after the last one, so this
	// is the one bound that cannot be strict.
	CHeaderShapedFile file;
	file.SetSectionData( 2, Pattern( 32 ) );
	file.SetSectionData( 0, Pattern( 16 ) );
	file.AddPointerFixup( 0, 0, 2, 32 );
	EXPECT_TRUE( Accepts( file.Bytes() ) );
}

TEST( ReadEntireFileFromMemory, RefusesCompressedSectionsUntilTheCodecsExist )
{
	// Oodle0 is still ahead, and this is what says so out loud. Of the 13,582 unique
	// GR2 in the retail install, 6,016 are entirely Oodle0, so this refusal is why
	// not quite half of the game still fails to load. When the codec lands, delete
	// this and bring back the DISABLED_ sweep.
	//
	// The Oodle1 case here is not a refusal of the codec, it is a refusal of these
	// particular bytes, which are a pattern rather than a stream. Oodle1 against
	// real streams is Oodle1.cpp's job.
	for ( uint32_t nCodec : { COMPRESSION_OODLE0, COMPRESSION_OODLE1 } )
	{
		CHeaderShapedFile file;
		file.SetSectionData( 0, Pattern( 32 ) );
		file.SetSectionField( 0, SEC_COMPRESSION, nCodec );
		file.SetSectionField( 0, SEC_EXPANDED_DATA_SIZE, 96 );
		EXPECT_FALSE( Accepts( file.Bytes() ) ) << "codec " << nCodec;
	}
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
	// Every byte of the magic has to be checked, not just the first word. The other
	// Granny dialects, big endian and 64-bit pointer, differ from this one in the
	// later words, and this library implements neither.
	for ( uint32_t i = 0; i < MAGIC_SIZE; ++i )
	{
		CHeaderShapedFile file;
		file.Bytes()[i] ^= 0x01;
		EXPECT_EQ( nullptr, Read( file.Bytes() ) ) << "flipped bit 0 of magic byte " << i;
	}
}

TEST( ReadEntireFileFromMemory, RejectsACompressedHeader )
{
	// headerFormat is 0 in all 7,844 files measured, so there is nothing to test an
	// implementation of the other value against.
	CHeaderShapedFile file;
	file.SetU32( OFF_HEADER_FORMAT, 1 );
	EXPECT_EQ( nullptr, Read( file.Bytes() ) );
}

TEST( ReadEntireFileFromMemory, RejectsAnUnsupportedVersion )
{
	// Format 6 in all 83,184 files surveyed. Anything else is a file this library
	// has never seen and must not guess at.
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
	// The multiplication that sizes the section array is the classic overflow site:
	// 0x0fffffff * 44 wraps on 32 bits, and a loader that trusts the wrapped
	// product reads far past the buffer.
	for ( uint32_t nCount : { 9u, 0x00ffffffu, 0x0fffffffu, 0xffffffffu } )
	{
		CHeaderShapedFile file;
		file.SetU32( OFF_SECTION_ARRAY_COUNT, nCount );
		EXPECT_EQ( nullptr, Read( file.Bytes() ) ) << "sectionArrayCount " << nCount;
	}
}

TEST( ReadEntireFileFromMemory, RejectsNoSectionsAtAll )
{
	// Nowhere for the root object to live.
	CHeaderShapedFile file;
	file.SetU32( OFF_SECTION_ARRAY_COUNT, 0 );
	EXPECT_EQ( nullptr, Read( file.Bytes() ) );
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

TEST( ReadEntireFileFromMemory, RejectsAHeaderSizeThatDisagreesWithTheSectionArray )
{
	// Two descriptions of the same boundary. They agree in all 7,844 measured
	// files, so a disagreement means neither can be trusted.
	for ( uint32_t nHeaderSize : { 439u, 441u, 0u } )
	{
		CHeaderShapedFile file;
		file.SetU32( OFF_HEADER_SIZE, nHeaderSize );
		EXPECT_EQ( nullptr, Read( file.Bytes() ) ) << "headerSize " << nHeaderSize;
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

TEST( ReadEntireFileFromMemory, RejectsAnUncompressedSectionThatClaimsToExpand )
{
	CHeaderShapedFile file;
	file.SetSectionData( 2, Pattern( 32 ) );
	file.SetSectionField( 2, SEC_EXPANDED_DATA_SIZE, 64 );
	EXPECT_EQ( nullptr, Read( file.Bytes() ) );
}

TEST( ReadEntireFileFromMemory, RejectsAnUnknownCompressionCodec )
{
	// 0, 1 and 2 are what the corpus contains. A fourth value means either a
	// corrupt file or a dialect with a codec this library does not have, and both
	// have to fail rather than fall through to whichever branch is last.
	CHeaderShapedFile file;
	file.SetSectionField( 0, SEC_COMPRESSION, 3 );
	EXPECT_EQ( nullptr, Read( file.Bytes() ) );
}

TEST( ReadEntireFileFromMemory, RejectsAFixupArrayPastTheBuffer )
{
	CHeaderShapedFile file;
	file.SetSectionField( 1, SEC_POINTER_FIXUP_COUNT, 0x20000000 );
	EXPECT_EQ( nullptr, Read( file.Bytes() ) );
}

TEST( ReadEntireFileFromMemory, RejectsAMarshallingArrayPastTheBuffer )
{
	// Bounds checked and then ignored: the marshalling fixups say what to byte swap
	// when the file's byte order is not the host's, and this dialect is read little
	// endian on little endian only. Bounds still have to hold.
	CHeaderShapedFile file;
	file.SetSectionField( 1, SEC_MIXED_MARSHALLING_COUNT, 0x20000000 );
	EXPECT_EQ( nullptr, Read( file.Bytes() ) );
}

TEST( ReadEntireFileFromMemory, RejectsAFixupSlotOutsideItsSection )
{
	// The slot is four bytes wide in the file whatever the host is, so the whole
	// slot has to fit, not just its first byte.
	CHeaderShapedFile file;
	file.SetSectionData( 0, Pattern( 32 ) );
	file.AddPointerFixup( 0, 30, 0, 0 );
	EXPECT_EQ( nullptr, Read( file.Bytes() ) );
}

TEST( ReadEntireFileFromMemory, RejectsAFixupIntoASectionThatDoesNotExist )
{
	CHeaderShapedFile file;
	file.SetSectionData( 0, Pattern( 32 ) );
	file.AddPointerFixup( 0, 0, 8, 0 );
	EXPECT_EQ( nullptr, Read( file.Bytes() ) );
}

TEST( ReadEntireFileFromMemory, RejectsAFixupTargetPastItsSection )
{
	CHeaderShapedFile file;
	file.SetSectionData( 0, Pattern( 32 ) );
	file.SetSectionData( 4, Pattern( 16 ) );
	file.AddPointerFixup( 0, 0, 4, 17 );
	EXPECT_EQ( nullptr, Read( file.Bytes() ) );
}

TEST( ReadEntireFileFromMemory, RejectsARootObjectInASectionThatDoesNotExist )
{
	CHeaderShapedFile file;
	file.SetU32( OFF_ROOT_OBJECT_SECTION, 8 );
	EXPECT_EQ( nullptr, Read( file.Bytes() ) );

	CHeaderShapedFile typeFile;
	typeFile.SetU32( OFF_ROOT_OBJECT_TYPE_SECTION, 99 );
	EXPECT_EQ( nullptr, Read( typeFile.Bytes() ) );
}

TEST( ReadEntireFileFromMemory, SurvivesEveryPrefixOfAWellShapedFile )
{
	// A .pak entry that was written short, or a read that stopped early, arrives as
	// a prefix. Every one of them has to be refused rather than walked off the end
	// of, and the loop is cheap enough to be exhaustive.
	CHeaderShapedFile file;
	file.SetSectionData( 0, Pattern( 64 ) );
	file.AddPointerFixup( 0, 12, 0, 40 );

	const uint32_t nTotal = static_cast<uint32_t>( file.Bytes().size() );
	for ( uint32_t n = 0; n < nTotal; ++n )
	{
		const std::vector<uint8_t> prefix( file.Bytes().begin(),
		                                   file.Bytes().begin() + static_cast<ptrdiff_t>( n ) );
		EXPECT_EQ( nullptr, Read( prefix ) ) << "prefix of " << n << " bytes";
	}
	EXPECT_TRUE( Accepts( file.Bytes() ) ) << "and the whole thing still loads";
}

TEST( ReadEntireFileFromMemory, TheNullItReturnsCanBeFedBackIn )
{
	// CGrannyMemFileLoader::RecalcValue stores the result unconditionally and
	// CGrannyFile::~CGrannyFile frees it, so a refused file becomes a free of
	// whatever was returned. GrannyGetFileInfo is reached the same way from
	// GAnimFormat.cpp.
	GrannyFreeFile( nullptr );
	EXPECT_EQ( nullptr, GrannyGetFileInfo( nullptr ) );
}

TEST( GetFileInfo, StillReturnsNull )
{
	// Delete this when the type tree walk lands, and enable the DISABLED_ test
	// below with it. The file is loaded and the root object's location is known;
	// the layout to read it with is what is missing.
	CHeaderShapedFile file;
	granny_file *pFile = Read( file.Bytes() );
	ASSERT_NE( nullptr, pFile );
	EXPECT_EQ( nullptr, GrannyGetFileInfo( pFile ) )
		<< "GrannyGetFileInfo returned something: enable the DISABLED_ test and "
		   "delete this one";
	GrannyFreeFile( pFile );
}

TEST( GetFileInfo, DISABLED_ReturnsTheRootObject )
{
	CHeaderShapedFile file;
	granny_file *pFile = Read( file.Bytes() );
	ASSERT_NE( nullptr, pFile );
	EXPECT_NE( nullptr, GrannyGetFileInfo( pFile ) );
	GrannyFreeFile( pFile );
}

// GrannyReadEntireFile, the same loader with a file name in front of it.

TEST( ReadEntireFile, RejectsANullName )
{
	EXPECT_EQ( nullptr, GrannyReadEntireFile( nullptr ) );
}

TEST( ReadEntireFile, RejectsAFileThatIsNotThere )
{
	const std::filesystem::path path =
		std::filesystem::temp_directory_path() / "libgr2_no_such_file.gr2";
	std::filesystem::remove( path );
	EXPECT_EQ( nullptr, GrannyReadEntireFile( path.string().c_str() ) );
}

TEST( ReadEntireFile, ReadsWhatTheFromMemoryPathWouldHaveRead )
{
	CHeaderShapedFile file;
	file.SetSectionData( 0, Pattern( 96 ) );
	file.AddPointerFixup( 0, 20, 0, 48 );

	const std::filesystem::path path =
		std::filesystem::temp_directory_path() / "libgr2_readentirefile.gr2";
	{
		std::ofstream out( path, std::ios::binary );
		ASSERT_TRUE( out.good() );
		out.write( reinterpret_cast<const char *>( file.Bytes().data() ),
		           static_cast<std::streamsize>( file.Bytes().size() ) );
	}

	granny_file *pFile = GrannyReadEntireFile( path.string().c_str() );
	EXPECT_NE( nullptr, pFile );
	GrannyFreeFile( pFile );
	std::filesystem::remove( path );
}

TEST( ReadEntireFile, RejectsAFileThatIsNotAGr2 )
{
	const std::filesystem::path path =
		std::filesystem::temp_directory_path() / "libgr2_not_a_gr2.txt";
	{
		const std::vector<uint8_t> bytes = ForeignMagicFile();
		std::ofstream out( path, std::ios::binary );
		ASSERT_TRUE( out.good() );
		out.write( reinterpret_cast<const char *>( bytes.data() ),
		           static_cast<std::streamsize>( bytes.size() ) );
	}

	EXPECT_EQ( nullptr, GrannyReadEntireFile( path.string().c_str() ) );
	std::filesystem::remove( path );
}

// Corpus sweep.

TEST( ReadEntireFileFromMemory, DISABLED_RejectsWrongCrc )
{
	// Open question, not a known requirement. The header carries a CRC at offset 40
	// and nobody has established whether the real DLL checks it, over which range,
	// or with which polynomial. Answer that against the oracle before enabling
	// this, and if the answer is that it does not check, delete the test rather
	// than implementing a check the engine has never depended on.
	CHeaderShapedFile file;
	file.SetU32( OFF_CRC, 0x12345678 );
	EXPECT_EQ( nullptr, Read( file.Bytes() ) );
}

TEST( ReadEntireFileFromMemory, DISABLED_ReadsEveryFileInADirectory )
{
	// Waiting on Oodle0. Oodle1 files load now, but 6,016 of the retail install's
	// 13,582 take the other codec, so a sweep over a whole directory still fails on
	// almost half of it for the one reason that is already known. Enable it with
	// the second codec.
	//
	// Point LIBGR2_TEST_GR2_DIR at a directory of extracted .gr2 resources. For
	// anything derived purely from a file the corpus is the input space, so
	// enumeration is exhaustive rather than a sample. The directory is not
	// committed and never can be; extract one from a .pak, which is an ordinary ZIP
	// archive.
	//
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
			// The non-GR2 files that share these directories. Their presence is the
			// point of RejectsForeignMagic, not of this sweep.
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
