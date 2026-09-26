// Pins Float2Int to what the original fld/fistp pair computed.
//
// That is: round by the current rounding mode, ties to even under the default
// mode, truncate under FE_TOWARDZERO (the DB loader and console SetVar run
// there), and 0x80000000 for NaN and anything outside int range. Any of these
// changing is a desync against the 2005 build and between our own platforms.
//
// Inputs go through a volatile so the compiler cannot fold the conversion at
// compile time under the default mode, which would hide the rounding mode tests.

#include <cfenv>
#include <cstdint>
#include <limits>

#include "Misc/Tools.h"

#include <gtest/gtest.h>

namespace {

int Convert( const float fVal )
{
	volatile float fIn = fVal;
	return Float2Int( fIn );
}

// switches the rounding mode for one test and puts the old one back
class CRoundingModeGuard
{
	const int nPrevMode;
public:
	explicit CRoundingModeGuard( const int nMode ) : nPrevMode( std::fegetround() ) { std::fesetround( nMode ); }
	~CRoundingModeGuard() { std::fesetround( nPrevMode ); }
};

TEST( Float2Int, NearestRoundsTiesToEven )
{
	CRoundingModeGuard guard( FE_TONEAREST );
	EXPECT_EQ( Convert( 0.5f ), 0 );
	EXPECT_EQ( Convert( 1.5f ), 2 );
	EXPECT_EQ( Convert( 2.5f ), 2 );
	EXPECT_EQ( Convert( 3.5f ), 4 );
	EXPECT_EQ( Convert( -0.5f ), 0 );
	EXPECT_EQ( Convert( -1.5f ), -2 );
	EXPECT_EQ( Convert( -2.5f ), -2 );
	EXPECT_EQ( Convert( 2.7f ), 3 );
	EXPECT_EQ( Convert( -2.7f ), -3 );
	EXPECT_EQ( Convert( 2.3f ), 2 );
	// the largest float below 0.5 must not round up
	EXPECT_EQ( Convert( 0.49999997f ), 0 );
}

TEST( Float2Int, ChopTruncates )
{
	CRoundingModeGuard guard( FE_TOWARDZERO );
	EXPECT_EQ( Convert( 2.7f ), 2 );
	EXPECT_EQ( Convert( -2.7f ), -2 );
	EXPECT_EQ( Convert( 3.5f ), 3 );
}

TEST( Float2Int, DownAndUp )
{
	{
		CRoundingModeGuard guard( FE_DOWNWARD );
		EXPECT_EQ( Convert( 2.3f ), 2 );
		EXPECT_EQ( Convert( -2.3f ), -3 );
	}
	{
		CRoundingModeGuard guard( FE_UPWARD );
		EXPECT_EQ( Convert( 2.3f ), 3 );
		EXPECT_EQ( Convert( -2.3f ), -2 );
	}
}

TEST( Float2Int, OutOfRangeIsIntegerIndefinite )
{
	CRoundingModeGuard guard( FE_TONEAREST );
	// the extremes that do fit
	EXPECT_EQ( Convert( -2147483648.0f ), INT32_MIN );
	EXPECT_EQ( Convert( 2147483520.0f ), 2147483520 );
	// 2^31 is the first float that does not
	EXPECT_EQ( Convert( 2147483648.0f ), INT32_MIN );
	EXPECT_EQ( Convert( -2147483904.0f ), INT32_MIN );
	EXPECT_EQ( Convert( 1e20f ), INT32_MIN );
	EXPECT_EQ( Convert( std::numeric_limits<float>::infinity() ), INT32_MIN );
	EXPECT_EQ( Convert( -std::numeric_limits<float>::infinity() ), INT32_MIN );
	EXPECT_EQ( Convert( std::numeric_limits<float>::quiet_NaN() ), INT32_MIN );
}

}
