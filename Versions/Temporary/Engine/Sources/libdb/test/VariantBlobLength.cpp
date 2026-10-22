// Pins what a CVariant blob does with the length it reads out of a stream.
//
// CVariant::SBlob::operator& serialized its own length and then allocated from
// it with nothing in between:
//
//     saver.Add( 1, &m_nSize );
//     if ( saver.IsReading() ) { m_ptr = new uint8_t[m_nSize]; }
//     saver.AddRawData( 2, m_ptr, m_nSize );
//
// m_nSize is a uint32_t off the stream, so a damaged or hostile record asked
// for up to four gigabytes, and AddRawData takes its size as an int, so any
// length from two gigabytes up arrived there negative and became a memcpy or a
// memset of an enormous size_t. This is the only place in the tree that hands
// AddRawData a length that did not come from a sizeof, which is why the savers
// never saw a bad one from anywhere else.
//
// The corrupt case is produced by serializing a blob and then overwriting the
// stored length in the buffer. The test asserts the length appears exactly
// once before patching it, so the patch cannot silently hit something else and
// go on passing.

#include "libdb/Variant.h"

#include <cstdint>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>

namespace
{

std::vector<char> Save( CVariant &var )
{
	CMemoryStream stream;
	{
		CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_WRITE );
		pSaver->Add( 1, &var );
	}
	return std::vector<char>( stream.GetBuffer(), stream.GetBuffer() + stream.GetSize() );
}

CVariant Load( const std::vector<char> &bytes )
{
	CMemoryStream stream;
	stream.Write( bytes.data(), static_cast<int>( bytes.size() ) );
	stream.Seek( 0 );
	CVariant var;
	{
		CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_READ );
		pSaver->Add( 1, &var );
	}
	return var;
}

// Replaces the one occurrence of uOld, little endian, with uNew. Fails the
// calling test if it is not there exactly once.
void PatchLength( std::vector<char> &bytes, const uint32_t uOld, const uint32_t uNew )
{
	uint8_t oldBytes[4];
	uint8_t newBytes[4];
	for ( int i = 0; i < 4; ++i )
	{
		oldBytes[i] = uint8_t( ( uOld >> ( i * 8 ) ) & 0xFF );
		newBytes[i] = uint8_t( ( uNew >> ( i * 8 ) ) & 0xFF );
	}

	std::vector<size_t> found;
	for ( size_t i = 0; i + 4 <= bytes.size(); ++i )
	{
		if ( std::memcmp( &bytes[i], oldBytes, 4 ) == 0 )
		{
			found.push_back( i );
		}
	}
	ASSERT_EQ( size_t( 1 ), found.size() ) << "the stored length is not uniquely locatable, the patch would be a guess";
	std::memcpy( &bytes[found[0]], newBytes, 4 );
}

// A distinctive length, so that PatchLength can find it unambiguously, with
// body bytes that cannot be mistaken for it.
constexpr uint32_t BLOB_SIZE = 0x00A1B2C3u % 977u + 601u; // 0x2BD, 701 bytes
constexpr uint8_t BLOB_FILL = 0xC7;

CVariant MakeBlob( const size_t nSize )
{
	std::vector<uint8_t> payload( nSize, BLOB_FILL );
	return CVariant( payload.empty() ? (const void *)&BLOB_FILL : (const void *)payload.data(), nSize );
}

TEST( VariantBlobLength, OrdinaryBlobRoundTrips )
{
	CVariant var = MakeBlob( BLOB_SIZE );
	const std::vector<char> bytes = Save( var );
	const CVariant loaded = Load( bytes );

	ASSERT_EQ( size_t( BLOB_SIZE ), loaded.GetBlobSize() );
	ASSERT_TRUE( loaded.GetPtr() != 0 );
	const uint8_t *const pLoaded = static_cast<const uint8_t *>( loaded.GetPtr() );
	for ( uint32_t i = 0; i < BLOB_SIZE; ++i )
	{
		ASSERT_EQ( BLOB_FILL, pLoaded[i] ) << "at " << i;
	}
}

// A zero length blob is valid data, not a defect: an empty binary field in a
// record produces one, and CVariant::operator int handles GetBlobSize() == 0
// as a case of its own. It has to survive the savers' size guards.
TEST( VariantBlobLength, ZeroLengthBlobRoundTrips )
{
	CVariant var = MakeBlob( 0 );
	ASSERT_EQ( size_t( 0 ), var.GetBlobSize() );
	const std::vector<char> bytes = Save( var );
	const CVariant loaded = Load( bytes );
	EXPECT_EQ( size_t( 0 ), loaded.GetBlobSize() );
}

// The regression. A length past what AddRawData can express used to allocate
// from it and then hand the saver a negative size.
TEST( VariantBlobLength, LengthPastIntMaxLoadsEmpty )
{
	CVariant var = MakeBlob( BLOB_SIZE );
	std::vector<char> bytes = Save( var );
	ASSERT_NO_FATAL_FAILURE( PatchLength( bytes, BLOB_SIZE, 0xFFFFFFFFu ) );

	const CVariant loaded = Load( bytes );
	EXPECT_EQ( size_t( 0 ), loaded.GetBlobSize() );
}

// Just over the boundary, which is the interesting edge: still a plausible
// allocation, still not expressible as the int the saver takes.
TEST( VariantBlobLength, LengthJustPastIntMaxLoadsEmpty )
{
	CVariant var = MakeBlob( BLOB_SIZE );
	std::vector<char> bytes = Save( var );
	ASSERT_NO_FATAL_FAILURE( PatchLength( bytes, BLOB_SIZE, uint32_t( INT32_MAX ) + 1u ) );

	const CVariant loaded = Load( bytes );
	EXPECT_EQ( size_t( 0 ), loaded.GetBlobSize() );
}

} // namespace
