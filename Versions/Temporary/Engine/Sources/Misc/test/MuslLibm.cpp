// musl-libm has to give the same bits on every platform, which is the whole
// reason it is built instead of calling the system libm. This hashes each
// function's results over a fixed set of inputs and compares the hash with one
// recorded from both the MSVC and the gcc build. A mismatch means the results
// changed: an update of the copied musl files, a compiler or flag change, or a
// platform that rounds differently. Any of those changes the simulation, so a
// changed hash is never just re-recorded; see "Updating" in README.md.
//
// The inputs:
//   float, one argument    every 509th of the 2^32 bit patterns (8.4 million),
//                          which covers every exponent, sign and NaN class
//   float, two arguments   the same sweep against a fixed set of second
//                          arguments, plus pseudo-random pairs
//   double                 pseudo-random bit patterns over the whole range,
//                          plus pseudo-random values in the range the game uses
//
// Every NaN hashes alike. The payload and sign of a NaN depend on the
// hardware's default NaN rather than on musl, and no simulation can depend on
// them, so pinning them would only make the test fail on another architecture.

#include "musl_libm.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include <gtest/gtest.h>

namespace
{
uint32_t Bits( const float f )
{
	uint32_t u;
	std::memcpy( &u, &f, sizeof( u ) );
	return u;
}

uint64_t Bits( const double d )
{
	uint64_t u;
	std::memcpy( &u, &d, sizeof( u ) );
	return u;
}

float FloatFromBits( const uint32_t u )
{
	float f;
	std::memcpy( &f, &u, sizeof( f ) );
	return f;
}

double DoubleFromBits( const uint64_t u )
{
	double d;
	std::memcpy( &d, &u, sizeof( d ) );
	return d;
}

// FNV-1a over the result bits, a byte at a time so that the hash does not
// depend on anything but the values.
class CHash
{
	uint64_t nValue = 14695981039346656037ull;

	void Byte( const uint8_t b )
	{
		nValue = ( nValue ^ b ) * 1099511628211ull;
	}

	void Word( uint64_t u, const int nBytes )
	{
		for ( int i = 0; i < nBytes; ++i )
		{
			Byte( static_cast<uint8_t>( u ) );
			u >>= 8;
		}
	}

public:
	void Add( const float f )
	{
		Word( f != f ? 0x7fc00000u : Bits( f ), 4 );
	}

	void Add( const double d )
	{
		Word( d != d ? 0x7ff8000000000000ull : Bits( d ), 8 );
	}

	uint64_t Get() const
	{
		return nValue;
	}
};

// splitmix64: a fixed, portable sequence, so the inputs are the same on
// every platform without storing them.
class CSequence
{
	uint64_t nState;

public:
	explicit CSequence( const uint64_t nSeed )
		: nState( nSeed )
	{
	}

	uint64_t Next()
	{
		uint64_t z = ( nState += 0x9e3779b97f4a7c15ull );
		z = ( z ^ ( z >> 30 ) ) * 0xbf58476d1ce4e5b9ull;
		z = ( z ^ ( z >> 27 ) ) * 0x94d049bb133111ebull;
		return z ^ ( z >> 31 );
	}

	// Uniform in [-fRange, fRange), from the top 53 bits. Three IEEE
	// operations, the first two exact, so the same value on every platform.
	double Uniform( const double fRange )
	{
		const double f01 = static_cast<double>( Next() >> 11 ) * 0x1p-53;
		return ( 2.0 * f01 - 1.0 ) * fRange;
	}
};

constexpr uint64_t FLOAT_STRIDE = 509;
constexpr int DOUBLE_SAMPLES = 1 << 20;

// The second arguments for atan2f and powf. Signed zeros, the infinities and
// NaN, the integers and halves where pow has special cases, and the ranges
// the game passes.
const float SECOND_ARGS[] = {
	0.0f,
	-0.0f,
	1.0f,
	-1.0f,
	0.5f,
	-0.5f,
	2.0f,
	-2.0f,
	3.0f,
	1.5f,
	0.1f,
	10.0f,
	1e-30f,
	1e30f,
	INFINITY,
	-INFINITY,
	NAN,
};

template <typename TFunc>
uint64_t HashFloat1( TFunc func )
{
	CHash hash;
	for ( uint64_t u = 0; u < 0x100000000ull; u += FLOAT_STRIDE )
	{
		hash.Add( func( FloatFromBits( static_cast<uint32_t>( u ) ) ) );
	}
	return hash.Get();
}

template <typename TFunc>
uint64_t HashFloat2( TFunc func )
{
	CHash hash;
	// Every 16th input of the one-argument sweep, against each fixed
	// second argument, and the swap of the two.
	for ( uint64_t u = 0; u < 0x100000000ull; u += FLOAT_STRIDE * 16 )
	{
		const float x = FloatFromBits( static_cast<uint32_t>( u ) );
		for ( const float y : SECOND_ARGS )
		{
			hash.Add( func( x, y ) );
			hash.Add( func( y, x ) );
		}
	}
	CSequence seq( 2 );
	for ( int i = 0; i < DOUBLE_SAMPLES; ++i )
	{
		const float x = static_cast<float>( seq.Uniform( 100.0 ) );
		const float y = static_cast<float>( seq.Uniform( 100.0 ) );
		hash.Add( func( x, y ) );
		hash.Add( func( FloatFromBits( static_cast<uint32_t>( seq.Next() ) ), y ) );
	}
	return hash.Get();
}

template <typename TFunc>
uint64_t HashDouble1( TFunc func )
{
	CHash hash;
	CSequence seq( 1 );
	for ( int i = 0; i < DOUBLE_SAMPLES; ++i )
	{
		hash.Add( func( DoubleFromBits( seq.Next() ) ) );
		hash.Add( func( seq.Uniform( 1.0 ) ) );
		hash.Add( func( seq.Uniform( 1000.0 ) ) );
	}
	return hash.Get();
}

template <typename TFunc>
uint64_t HashDouble2( TFunc func )
{
	CHash hash;
	CSequence seq( 3 );
	for ( int i = 0; i < DOUBLE_SAMPLES; ++i )
	{
		// Each draw into its own variable, first argument first. The order in which
		// a call's arguments are evaluated is unspecified: gcc and MSVC go right to
		// left and clang left to right, so func( seq.Next(), seq.Next() ) passed the
		// pair swapped under clang and its hashes differed with musl agreeing.
		const double xBits = DoubleFromBits( seq.Next() );
		const double yBits = DoubleFromBits( seq.Next() );
		hash.Add( func( xBits, yBits ) );
		const double xRange = seq.Uniform( 100.0 );
		const double yRange = seq.Uniform( 100.0 );
		hash.Add( func( xRange, yRange ) );
		for ( const float y : SECOND_ARGS )
		{
			hash.Add( func( seq.Uniform( 10.0 ), static_cast<double>( y ) ) );
		}
	}
	return hash.Get();
}

// On a mismatch, print the value in the form the table below takes, so
// that a deliberate change is one copy, and a platform difference shows
// which functions it reaches.
void ExpectHash( const char *pszName, const uint64_t nGot, const uint64_t nExpected )
{
	if ( nGot != nExpected )
	{
		std::printf( "  %s: 0x%016llxull\n", pszName, static_cast<unsigned long long>( nGot ) );
	}
	EXPECT_EQ( nGot, nExpected ) << pszName;
}
} // namespace

// The case that started this: the half angle of a unit direction, where glibc's
// cosf is one ulp off the correctly rounded value MSVC's CRT returns, and a
// replay recorded on Windows diverged on Linux 400 ms in. musl agrees with the
// correctly rounded value here, on both.
TEST( MuslLibm, CosfOfTheReplayDivergence )
{
	EXPECT_EQ( Bits( musl_cosf( FloatFromBits( 0x40072014u ) ) ), 0xbf03bc93u );
}

// Recorded 2026-11-04 from musl 7813e911, and identical from MSVC x64
// RelWithDebInfo, gcc x64 -O2 and clang 19 and 21 -O2, and from gcc with
// -march=haswell, the target on which compat/musl_libm_prefix.h has to undefine
// __FP_FAST_FMA (without it log and pow change).
TEST( MuslLibm, FloatFunctions )
{
	ExpectHash( "sinf", HashFloat1( musl_sinf ), 0xd24919d797bcc2efull );
	ExpectHash( "cosf", HashFloat1( musl_cosf ), 0x89cde0c1b1c8e710ull );
	ExpectHash( "tanf", HashFloat1( musl_tanf ), 0x42da6eb7dd67e93cull );
	ExpectHash( "asinf", HashFloat1( musl_asinf ), 0xc76d0ec41696f296ull );
	ExpectHash( "acosf", HashFloat1( musl_acosf ), 0x35343fbbebcbdd98ull );
	ExpectHash( "atanf", HashFloat1( musl_atanf ), 0x9edccb49e5a794aeull );
	ExpectHash( "expf", HashFloat1( musl_expf ), 0x12ae8ba3cc0b7543ull );
	ExpectHash( "expm1f", HashFloat1( musl_expm1f ), 0x177ea42264fd1445ull );
	ExpectHash( "logf", HashFloat1( musl_logf ), 0xe7f8141057a4e85bull );
	ExpectHash( "atan2f", HashFloat2( musl_atan2f ), 0x1041d28f22fb2accull );
	ExpectHash( "powf", HashFloat2( musl_powf ), 0xc78abc201136468aull );
}

TEST( MuslLibm, DoubleFunctions )
{
	ExpectHash( "sin", HashDouble1( musl_sin ), 0x72a8b17ac3f8e3ebull );
	ExpectHash( "cos", HashDouble1( musl_cos ), 0x36d76ecc49d9e141ull );
	ExpectHash( "tan", HashDouble1( musl_tan ), 0x42e28ca0353ff107ull );
	ExpectHash( "asin", HashDouble1( musl_asin ), 0x2ad24f2a139b6da6ull );
	ExpectHash( "acos", HashDouble1( musl_acos ), 0x2165b1e3a5b5f45full );
	ExpectHash( "atan", HashDouble1( musl_atan ), 0x34da0576acab012eull );
	ExpectHash( "exp", HashDouble1( musl_exp ), 0xd9149bb3c260124dull );
	ExpectHash( "expm1", HashDouble1( musl_expm1 ), 0x5a4d23a99b205811ull );
	ExpectHash( "log", HashDouble1( musl_log ), 0x6d69531076d339beull );
	ExpectHash( "atan2", HashDouble2( musl_atan2 ), 0x1ec429fc39d324caull );
	ExpectHash( "pow", HashDouble2( musl_pow ), 0x88511accbe8d4219ull );
}
