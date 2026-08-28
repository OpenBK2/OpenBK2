// What the loader built, checked from the inside.
//
// GrannyReadEntireFileFromMemory answers yes or no, and the tests next door check
// that answer. These check what a yes produced: the right bytes in the right
// section, the right root object, and pointers that resolve to what the fixup
// array said rather than to what the pointer slots happen to contain.
//
// That last point is the one worth stating. A GR2 stores 32-bit pointers, and on
// x64 an eight byte pointer does not fit the four byte slot, so nothing is fixed
// up in place. The bytes stay as the file had them and the resolved fixups live
// beside them, which is both what lets x86 and x64 run the same code and the only
// correct reading: the slots hold whatever the exporter left there, and the fixup
// array is what says which of them are pointers at all.
//
// Reaching granny_file's members needs the library statically, which is why the
// tests link gr2_static rather than the DLL. None of this is among the 54 exports
// and none of it ever will be.

#include "MinimalGr2.h"

#include "File.h"

#include <gr2/granny.h>

#include <cstring>
#include <vector>

#include <gtest/gtest.h>

using namespace NGr2Test;

namespace
{

//! Load, or fail the calling test. The caller frees.
granny_file *Load( const CHeaderShapedFile &file )
{
	granny_file *pFile = GrannyReadEntireFileFromMemory(
		static_cast<granny_int32x>( file.Bytes().size() ), file.Bytes().data() );
	EXPECT_NE( nullptr, pFile );
	return pFile;
}

}

TEST( FileContainer, KeepsEverySection )
{
	CHeaderShapedFile file;
	granny_file *pFile = Load( file );
	ASSERT_NE( nullptr, pFile );

	EXPECT_EQ( 8u, pFile->SectionCount() );
	for ( uint32_t i = 0; i < pFile->SectionCount(); ++i )
	{
		EXPECT_TRUE( pFile->SectionData( i ).empty() ) << "section " << i;
		EXPECT_EQ( NGr2::COMPRESSION_NONE, pFile->Section( i ).nCompression );
	}
	GrannyFreeFile( pFile );
}

TEST( FileContainer, CopiesSectionBytesOutOfTheBuffer )
{
	const std::vector<uint8_t> first = Pattern( 128 );
	const std::vector<uint8_t> second = Pattern( 64, 0x40 );

	CHeaderShapedFile file;
	file.SetSectionData( 0, first );
	file.SetSectionData( 5, second );

	granny_file *pFile = Load( file );
	ASSERT_NE( nullptr, pFile );

	EXPECT_EQ( first, pFile->SectionData( 0 ) );
	EXPECT_EQ( second, pFile->SectionData( 5 ) );
	EXPECT_TRUE( pFile->SectionData( 1 ).empty() );
	EXPECT_EQ( 128u, pFile->Section( 0 ).nDataSize );
	EXPECT_EQ( 128u, pFile->Section( 0 ).nExpandedDataSize );
	GrannyFreeFile( pFile );
}

TEST( FileContainer, DoesNotRetainTheCallersBuffer )
{
	// CGrannyMemFileLoader::RecalcValue reads into a CMemoryStream that dies at the
	// end of the function, so anything the loader kept a pointer into would be
	// freed before the first mesh is drawn.
	const std::vector<uint8_t> content = Pattern( 96, 0x11 );

	granny_file *pFile = 0;
	{
		CHeaderShapedFile file;
		file.SetSectionData( 3, content );
		std::vector<uint8_t> bytes = file.Bytes();
		pFile = GrannyReadEntireFileFromMemory( static_cast<granny_int32x>( bytes.size() ),
		                                        bytes.data() );
		ASSERT_NE( nullptr, pFile );
		// Scribble over the source before reading anything back out.
		memset( bytes.data(), 0xcd, bytes.size() );
	}

	EXPECT_EQ( content, pFile->SectionData( 3 ) );
	GrannyFreeFile( pFile );
}

TEST( FileContainer, RecordsTheRootObjectAndItsType )
{
	CHeaderShapedFile file;
	file.SetSectionData( 2, Pattern( 64 ) );
	file.SetRootObject( 2, 16 );

	granny_file *pFile = Load( file );
	ASSERT_NE( nullptr, pFile );

	EXPECT_EQ( 2u, pFile->RootObject().nSection );
	EXPECT_EQ( 16u, pFile->RootObject().nOffset );
	EXPECT_EQ( 6u, pFile->RootObjectType().nSection ) << "where shipped files put it";
	EXPECT_EQ( TYPE_TAG_13, pFile->TypeTag() );
	GrannyFreeFile( pFile );
}

TEST( FileContainer, ResolvesAPointerToWhatTheFixupSays )
{
	CHeaderShapedFile file;
	file.SetSectionData( 0, Pattern( 64 ) );
	file.SetSectionData( 6, Pattern( 32, 0x80 ) );
	file.AddPointerFixup( 0, 8, 6, 12 );

	granny_file *pFile = Load( file );
	ASSERT_NE( nullptr, pFile );

	NGr2::SReference target;
	ASSERT_TRUE( pFile->ResolvePointer( 0, 8, &target ) );
	EXPECT_EQ( 6u, target.nSection );
	EXPECT_EQ( 12u, target.nOffset );
	GrannyFreeFile( pFile );
}

TEST( FileContainer, ASlotWithNoFixupIsANullPointer )
{
	// The slot at offset 8 holds pattern bytes, not zero, and it still has to read
	// as null: what a GR2 puts in an unfixed slot means nothing.
	CHeaderShapedFile file;
	file.SetSectionData( 0, Pattern( 64 ) );
	file.AddPointerFixup( 0, 8, 0, 0 );

	granny_file *pFile = Load( file );
	ASSERT_NE( nullptr, pFile );

	EXPECT_TRUE( pFile->ResolvePointer( 0, 8, 0 ) );
	EXPECT_FALSE( pFile->ResolvePointer( 0, 12, 0 ) ) << "no fixup covers this slot";
	EXPECT_FALSE( pFile->ResolvePointer( 0, 4, 0 ) );
	EXPECT_FALSE( pFile->ResolvePointer( 1, 8, 0 ) ) << "a different section";
	EXPECT_FALSE( pFile->ResolvePointer( 99, 8, 0 ) ) << "no such section";
	GrannyFreeFile( pFile );
}

TEST( FileContainer, OnlyTheExactSlotOffsetResolves )
{
	// A pointer occupies four bytes and the fixup names the first of them. Reading
	// one byte into it is a different member, not the same pointer.
	CHeaderShapedFile file;
	file.SetSectionData( 0, Pattern( 64 ) );
	file.AddPointerFixup( 0, 16, 0, 32 );

	granny_file *pFile = Load( file );
	ASSERT_NE( nullptr, pFile );

	EXPECT_TRUE( pFile->ResolvePointer( 0, 16, 0 ) );
	EXPECT_FALSE( pFile->ResolvePointer( 0, 15, 0 ) );
	EXPECT_FALSE( pFile->ResolvePointer( 0, 17, 0 ) );
	GrannyFreeFile( pFile );
}

TEST( FileContainer, SortsFixupsThatArrivedOutOfOrder )
{
	// 15,287 of the 15,373 fixup arrays measured are unsorted, so the sort on load
	// is not a precaution, it is the normal case. Every one of these still has to
	// resolve, which is what a lookup on a wrongly sorted array would break.
	const uint32_t offsets[] = { 40, 4, 28, 0, 16, 36, 8, 24, 12, 32, 20 };

	CHeaderShapedFile file;
	file.SetSectionData( 0, Pattern( 64 ) );
	for ( uint32_t nFrom : offsets )
	{
		file.AddPointerFixup( 0, nFrom, 0, nFrom + 8 );
	}

	granny_file *pFile = Load( file );
	ASSERT_NE( nullptr, pFile );

	for ( uint32_t nFrom : offsets )
	{
		NGr2::SReference target;
		EXPECT_TRUE( pFile->ResolvePointer( 0, nFrom, &target ) ) << "slot " << nFrom;
		EXPECT_EQ( nFrom + 8, target.nOffset ) << "slot " << nFrom;
	}
	EXPECT_FALSE( pFile->ResolvePointer( 0, 44, 0 ) );

	const std::vector<NGr2::SPointerFixup> &fixups = pFile->PointerFixups( 0 );
	ASSERT_EQ( sizeof( offsets ) / sizeof( offsets[0] ), fixups.size() );
	for ( size_t i = 1; i < fixups.size(); ++i )
	{
		EXPECT_LT( fixups[i - 1].nFromOffset, fixups[i].nFromOffset );
	}
	GrannyFreeFile( pFile );
}

TEST( FileContainer, FixupsBelongToTheSectionThatCarriesThem )
{
	// nFromOffset is relative to the owning section, so the same offset in two
	// sections is two different slots.
	CHeaderShapedFile file;
	file.SetSectionData( 0, Pattern( 32 ) );
	file.SetSectionData( 1, Pattern( 32, 0x50 ) );
	file.AddPointerFixup( 0, 4, 1, 8 );
	file.AddPointerFixup( 1, 4, 0, 16 );

	granny_file *pFile = Load( file );
	ASSERT_NE( nullptr, pFile );

	NGr2::SReference fromZero;
	NGr2::SReference fromOne;
	ASSERT_TRUE( pFile->ResolvePointer( 0, 4, &fromZero ) );
	ASSERT_TRUE( pFile->ResolvePointer( 1, 4, &fromOne ) );
	EXPECT_EQ( 1u, fromZero.nSection );
	EXPECT_EQ( 8u, fromZero.nOffset );
	EXPECT_EQ( 0u, fromOne.nSection );
	EXPECT_EQ( 16u, fromOne.nOffset );
	GrannyFreeFile( pFile );
}

TEST( FileContainer, KeepsAPointerTargetOnePastTheEnd )
{
	CHeaderShapedFile file;
	file.SetSectionData( 0, Pattern( 32 ) );
	file.SetSectionData( 4, Pattern( 16 ) );
	file.AddPointerFixup( 0, 0, 4, 16 );

	granny_file *pFile = Load( file );
	ASSERT_NE( nullptr, pFile );

	NGr2::SReference target;
	ASSERT_TRUE( pFile->ResolvePointer( 0, 0, &target ) );
	EXPECT_EQ( 4u, target.nSection );
	EXPECT_EQ( 16u, target.nOffset ) << "the byte after the last, where an empty array points";
	GrannyFreeFile( pFile );
}

TEST( FileContainer, LoadsAndFreesRepeatedly )
{
	// Nothing here is shared or cached between files, so a second load must be
	// independent of the first and freeing one must not touch the other.
	CHeaderShapedFile first;
	first.SetSectionData( 0, Pattern( 64, 0x01 ) );
	CHeaderShapedFile second;
	second.SetSectionData( 0, Pattern( 64, 0x02 ) );

	granny_file *pFirst = Load( first );
	granny_file *pSecond = Load( second );
	ASSERT_NE( nullptr, pFirst );
	ASSERT_NE( nullptr, pSecond );
	EXPECT_NE( pFirst, pSecond );
	EXPECT_NE( pFirst->SectionData( 0 ), pSecond->SectionData( 0 ) );

	GrannyFreeFile( pFirst );
	EXPECT_EQ( Pattern( 64, 0x02 ), pSecond->SectionData( 0 ) );
	GrannyFreeFile( pSecond );

	for ( int i = 0; i < 64; ++i )
	{
		GrannyFreeFile( Load( first ) );
	}
}
