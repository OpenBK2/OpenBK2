// Verifies the boost::core-backed bit helpers in Misc/Tools.h are bit-exact
// with the hand-rolled routines they replaced.
//
// The 8- and 16-bit domains are small enough to cover exhaustively, which is
// stronger than sampling -- it pins down the zero cases, where the old
// per-width overloads deliberately returned different values (GetLSB gave
// 31 / 15 / 7 for the 32 / 16 / 8-bit overloads).

#include "Misc/Tools.h"

#include "random.h"

#include <gtest/gtest.h>

#include <climits>

// The implementations as they stood before the boost::core rewrite, copied
// verbatim so the comparison is against real old behaviour rather than a
// restatement of the new one. Being byte-identical to the originals is the
// point, so this block is deliberately exempt from the always-brace rule.
namespace original
{
	inline int GetNextPow2( uint32_t n )
	{
		n -= 1;

		n |= n >> 16;
		n |= n >> 8;
		n |= n >> 4;
		n |= n >> 2;
		n |= n >> 1;

		return n + 1;
	}
	inline int GetNextPow2( int n ) { return GetNextPow2( uint32_t(n) ); }

	inline int GetMSB( uint32_t n )
	{
		int k = 0;
		if ( n & 0xFFFF0000 ) k = 16, n >>= 16;
		if ( n & 0x0000FF00 ) k += 8, n >>= 8;
		if ( n & 0x000000F0 ) k += 4, n >>= 4;
		if ( n & 0x0000000C ) k += 2, n >>= 2;
		if ( n & 0x00000002 ) k += 1;
		return k;
	}
	inline int GetMSB( int n ) { return GetMSB( uint32_t(n) ); }
	inline int GetMSB( uint16_t n )
	{
		int k = 0;
		if ( n & 0xFF00 ) k  = 8, n >>= 8;
		if ( n & 0x00F0 ) k += 4, n >>= 4;
		if ( n & 0x000C ) k += 2, n >>= 2;
		if ( n & 0x0002 ) k += 1;
		return k;
	}
	inline int GetMSB( uint8_t n )
	{
		int k = 0;
		if ( n & 0xF0 ) k  = 4, n >>= 4;
		if ( n & 0x0C ) k += 2, n >>= 2;
		if ( n & 0x02 ) k += 1;
		return k;
	}

	inline int GetLSB( uint32_t n )
	{
		int k = 0;
		if ( (n & 0x0000FFFF) == 0 ) k = 16, n >>= 16;
		if ( (n & 0x000000FF) == 0 ) k += 8, n >>= 8;
		if ( (n & 0x0000000F) == 0 ) k += 4, n >>= 4;
		if ( (n & 0x00000003) == 0 ) k += 2, n >>= 2;
		if ( (n & 0x00000001) == 0 ) k += 1;
		return k;
	}
	inline int GetLSB( int n ) { return GetLSB( uint32_t(n) ); }
	inline int GetLSB( uint16_t n )
	{
		int k = 0;
		if ( (n & 0x00FF) == 0 ) k  = 8, n >>= 8;
		if ( (n & 0x000F) == 0 ) k += 4, n >>= 4;
		if ( (n & 0x0003) == 0 ) k += 2, n >>= 2;
		if ( (n & 0x0001) == 0 ) k += 1;
		return k;
	}
	inline int GetLSB( uint8_t n )
	{
		int k = 0;
		if ( (n & 0x0F) == 0 ) k  = 4, n >>= 4;
		if ( (n & 0x03) == 0 ) k += 2, n >>= 2;
		if ( (n & 0x01) == 0 ) k += 1;
		return k;
	}

	inline int GetNumBits( uint32_t v )
	{
		v = (v & 0x49249249ul) + ((v >> 1) & 0x49249249ul) + ((v >> 2) & 0x49249249ul);
		v = ((v + (v >> 3)) & 0x381c0e07ul) + ((v >> 6) & 0x381c0e07ul);
		return int( (v + (v >> 9) + (v >> 18) + (v >> 27)) & 0x3f );
	}
	inline int GetNumBits( int v ) { return GetNumBits( uint32_t(v) ); }
	inline int GetNumBits( uint8_t v )
	{
		v = (v & 0x55) + ((v >> 1) & 0x55);
		v = (v & 0x33) + ((v >> 2) & 0x33);
		return int( (v & 0x0f) + ((v >> 4) & 0x0f) );
	}
}

enum { random_iterations = 200000 };

TEST(BitOps, GetMSB_Exhaustive8) {
	for ( int i = 0; i <= 0xFF; ++i ) {
		const uint8_t v = static_cast<uint8_t>( i );
		ASSERT_EQ( GetMSB( v ), original::GetMSB( v ) ) << "uint8_t input " << i;
	}
}

TEST(BitOps, GetMSB_Exhaustive16) {
	for ( int i = 0; i <= 0xFFFF; ++i ) {
		const uint16_t v = static_cast<uint16_t>( i );
		ASSERT_EQ( GetMSB( v ), original::GetMSB( v ) ) << "uint16_t input " << i;
	}
}

TEST(BitOps, GetLSB_Exhaustive8) {
	for ( int i = 0; i <= 0xFF; ++i ) {
		const uint8_t v = static_cast<uint8_t>( i );
		ASSERT_EQ( GetLSB( v ), original::GetLSB( v ) ) << "uint8_t input " << i;
	}
}

TEST(BitOps, GetLSB_Exhaustive16) {
	for ( int i = 0; i <= 0xFFFF; ++i ) {
		const uint16_t v = static_cast<uint16_t>( i );
		ASSERT_EQ( GetLSB( v ), original::GetLSB( v ) ) << "uint16_t input " << i;
	}
}

TEST(BitOps, GetNumBits_Exhaustive8) {
	for ( int i = 0; i <= 0xFF; ++i ) {
		const uint8_t v = static_cast<uint8_t>( i );
		ASSERT_EQ( GetNumBits( v ), original::GetNumBits( v ) ) << "uint8_t input " << i;
	}
}

TEST(BitOps, GetNextPow2_Exhaustive16) {
	for ( int i = 0; i <= 0xFFFF; ++i ) {
		ASSERT_EQ( GetNextPow2( i ), original::GetNextPow2( i ) ) << "int input " << i;
	}
}

// These are the cases that differed before the helpers were made bit-exact,
// so they are worth pinning down by hand as well as by the sweeps above.
TEST(BitOps, ZeroCases) {
	EXPECT_EQ( GetNextPow2( 0 ), 0 );
	EXPECT_EQ( GetMSB( uint32_t( 0 ) ), 0 );
	EXPECT_EQ( GetMSB( uint16_t( 0 ) ), 0 );
	EXPECT_EQ( GetMSB( uint8_t( 0 ) ), 0 );
	// width-dependent: every shift step fired, leaving k at width-1
	EXPECT_EQ( GetLSB( uint32_t( 0 ) ), 31 );
	EXPECT_EQ( GetLSB( uint16_t( 0 ) ), 15 );
	EXPECT_EQ( GetLSB( uint8_t( 0 ) ), 7 );
	EXPECT_EQ( GetNumBits( uint32_t( 0 ) ), 0 );
}

// A negative int must be reinterpreted as uint32_t, not widened to uint64_t.
TEST(BitOps, NegativeIntNarrowsTo32Bit) {
	for ( int v : { -1, -2, -256, -65536, INT_MIN } ) {
		ASSERT_EQ( GetMSB( v ), original::GetMSB( v ) ) << "int input " << v;
		ASSERT_EQ( GetLSB( v ), original::GetLSB( v ) ) << "int input " << v;
		ASSERT_EQ( GetNumBits( v ), original::GetNumBits( v ) ) << "int input " << v;
	}
	EXPECT_EQ( GetMSB( -1 ), 31 );
	EXPECT_EQ( GetNumBits( -1 ), 32 );
}

// bit_ceil is undefined when the result is not representable; the old code
// wrapped to 0 there.
TEST(BitOps, GetNextPow2_AboveHalfRange) {
	EXPECT_EQ( GetNextPow2( uint32_t( 0x80000000u ) ),
	           original::GetNextPow2( uint32_t( 0x80000000u ) ) );
	EXPECT_EQ( GetNextPow2( uint32_t( 0x80000001u ) ), 0 );
	EXPECT_EQ( GetNextPow2( uint32_t( 0xFFFFFFFFu ) ), 0 );
}

TEST(BitOps, Random32) {
	for ( size_t i = 0; i < random_iterations; ++i ) {
		const uint32_t v = random_uint32();
		ASSERT_EQ( GetMSB( v ), original::GetMSB( v ) ) << "uint32_t input " << v;
		ASSERT_EQ( GetLSB( v ), original::GetLSB( v ) ) << "uint32_t input " << v;
		ASSERT_EQ( GetNumBits( v ), original::GetNumBits( v ) ) << "uint32_t input " << v;
		// keep below the representable limit for bit_ceil
		const uint32_t w = v >> 1;
		ASSERT_EQ( GetNextPow2( w ), original::GetNextPow2( w ) ) << "uint32_t input " << w;
	}
}
