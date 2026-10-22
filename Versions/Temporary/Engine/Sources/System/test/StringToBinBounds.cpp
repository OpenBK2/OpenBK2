// Pins NStr::StringToBin to staying inside both of its buffers.
//
// It used to be bounded only by the input's terminator, and it stepped two
// characters at a time while testing one, so it overran in two ways:
//
//   odd length   the terminator sits on a position the test never looks at, so
//                it was consumed as a data nibble and the loop then read one
//                byte past the end of the string and wrote one past the end of
//                the output. Three characters were enough.
//   long text    with no output size there was nothing else to stop it, so a
//                string longer than the caller's buffer overran it by as much
//                as the text asked for. The one caller that tried to guard
//                that used NI_ASSERT, which does not evaluate its argument in
//                this configuration.
//
// Both inputs here come from data files, so neither length is the engine's to
// trust. The guard bytes are what the test is really about: under ASan the
// overruns are caught without them, and in an ordinary build they are the only
// thing that notices.

#include "Misc/StrProc.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{

constexpr uint8_t GUARD = 0xCD;
constexpr int GUARD_BYTES = 8;

// Converts into a buffer of exactly nCapacity usable bytes, followed by guard
// bytes, and fails if anything past the capacity was touched.
struct SConversion
{
	std::vector<uint8_t> output;
	int nWritten = 0;
	bool bGuardIntact = true;
};

SConversion Convert( const std::string &szText, const int nCapacity )
{
	SConversion res;
	res.output.assign( nCapacity + GUARD_BYTES, GUARD );
	// The text is copied into its own exactly sized allocation so that a read
	// past the terminator lands outside a heap block rather than inside the
	// rest of some larger buffer, which is what makes ASan able to see it.
	std::vector<char> text( szText.begin(), szText.end() );
	text.push_back( '\0' );

	NStr::StringToBin( text.data(), res.output.data(), nCapacity, &res.nWritten );

	for ( int i = 0; i < GUARD_BYTES; ++i )
	{
		if ( res.output[nCapacity + i] != GUARD )
		{
			res.bGuardIntact = false;
		}
	}
	return res;
}

TEST( StringToBinBounds, EvenLengthRoundTrips )
{
	const SConversion res = Convert( "deadbeef", 4 );
	EXPECT_TRUE( res.bGuardIntact );
	EXPECT_EQ( 4, res.nWritten );
	EXPECT_EQ( 0xDE, res.output[0] );
	EXPECT_EQ( 0xAD, res.output[1] );
	EXPECT_EQ( 0xBE, res.output[2] );
	EXPECT_EQ( 0xEF, res.output[3] );
}

// The regression. "abc" with a one byte buffer used to write two bytes and
// read the terminator and one byte beyond it.
TEST( StringToBinBounds, OddLengthStopsBeforeTheTrailingHalfByte )
{
	const SConversion res = Convert( "abc", 1 );
	EXPECT_TRUE( res.bGuardIntact );
	EXPECT_EQ( 1, res.nWritten );
	EXPECT_EQ( 0xAB, res.output[0] );
}

TEST( StringToBinBounds, OddLengthWithRoomToSpareStillStops )
{
	// Capacity is not the limit here, the damaged text is, so a short count is
	// how the caller finds out.
	const SConversion res = Convert( "abcde", 16 );
	EXPECT_TRUE( res.bGuardIntact );
	EXPECT_EQ( 2, res.nWritten );
}

// What XMLChunkSaver's dead NI_ASSERT was meant to prevent.
TEST( StringToBinBounds, TextLongerThanTheBufferIsTruncated )
{
	const SConversion res = Convert( "00112233445566778899", 3 );
	EXPECT_TRUE( res.bGuardIntact );
	EXPECT_EQ( 3, res.nWritten );
	EXPECT_EQ( 0x00, res.output[0] );
	EXPECT_EQ( 0x11, res.output[1] );
	EXPECT_EQ( 0x22, res.output[2] );
}

TEST( StringToBinBounds, EmptyTextWritesNothing )
{
	const SConversion res = Convert( "", 4 );
	EXPECT_TRUE( res.bGuardIntact );
	EXPECT_EQ( 0, res.nWritten );
}

TEST( StringToBinBounds, ZeroCapacityWritesNothing )
{
	const SConversion res = Convert( "deadbeef", 0 );
	EXPECT_TRUE( res.bGuardIntact );
	EXPECT_EQ( 0, res.nWritten );
}

// BinToString is the only producer of the text StringToBin reads, so the pair
// has to agree over every byte value.
TEST( StringToBinBounds, RoundTripsEveryByteValue )
{
	std::vector<uint8_t> original( 256 );
	for ( int i = 0; i < 256; ++i )
	{
		original[i] = uint8_t( i );
	}
	std::string szText;
	szText.resize( original.size() * 2 );
	NStr::BinToString( original.data(), int( original.size() ), &szText[0] );

	const SConversion res = Convert( szText, int( original.size() ) );
	EXPECT_TRUE( res.bGuardIntact );
	ASSERT_EQ( int( original.size() ), res.nWritten );
	EXPECT_EQ( 0, std::memcmp( original.data(), res.output.data(), original.size() ) );
}

} // namespace
