// NDetMath has to be a drop-in for std's overloads, so that switching a call
// from std::sin to NDetMath::Sin changes the library and nothing else: the
// same argument types must give the same arithmetic type. The result types
// are pinned at compile time against std's own; the values against musl's.

#include "Misc/DetMath.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include <gtest/gtest.h>

namespace
{
template <typename TA, typename TB>
constexpr bool SAME_TYPE = std::is_same_v<TA, TB>;

// One argument: float stays float, double and the integers give double.
static_assert( SAME_TYPE<decltype( NDetMath::Sin( 1.0f ) ), decltype( std::sin( 1.0f ) )> );
static_assert( SAME_TYPE<decltype( NDetMath::Sin( 1.0 ) ), decltype( std::sin( 1.0 ) )> );
static_assert( SAME_TYPE<decltype( NDetMath::Sin( 1 ) ), decltype( std::sin( 1 ) )> );
static_assert( SAME_TYPE<decltype( NDetMath::ExpM1( 1.0f ) ), decltype( std::expm1( 1.0f ) )> );
static_assert( SAME_TYPE<decltype( NDetMath::Log( 2u ) ), decltype( std::log( 2u ) )> );

// Two arguments: float only when both are.
static_assert( SAME_TYPE<decltype( NDetMath::ATan2( 1.0f, 1.0f ) ), decltype( std::atan2( 1.0f, 1.0f ) )> );
static_assert( SAME_TYPE<decltype( NDetMath::ATan2( 1.0f, 1.0 ) ), decltype( std::atan2( 1.0f, 1.0 ) )> );
static_assert( SAME_TYPE<decltype( NDetMath::ATan2( 1, 1.0f ) ), decltype( std::atan2( 1, 1.0f ) )> );
static_assert( SAME_TYPE<decltype( NDetMath::Pow( 1.0f, 2 ) ), decltype( std::pow( 1.0f, 2 ) )> );
static_assert( SAME_TYPE<decltype( NDetMath::Pow( 1.0f, 2.0f ) ), decltype( std::pow( 1.0f, 2.0f ) )> );

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
} // namespace

// Each wrapper reaches the musl function of its precision, not the other one.
TEST( DetMath, CallsTheMuslFunctionOfItsPrecision )
{
	const float f = 0.7f;
	const double d = 0.7;
	EXPECT_EQ( Bits( NDetMath::Sin( f ) ), Bits( musl_sinf( f ) ) );
	EXPECT_EQ( Bits( NDetMath::Sin( d ) ), Bits( musl_sin( d ) ) );
	EXPECT_EQ( Bits( NDetMath::Cos( f ) ), Bits( musl_cosf( f ) ) );
	EXPECT_EQ( Bits( NDetMath::Cos( d ) ), Bits( musl_cos( d ) ) );
	EXPECT_EQ( Bits( NDetMath::Tan( f ) ), Bits( musl_tanf( f ) ) );
	EXPECT_EQ( Bits( NDetMath::Tan( d ) ), Bits( musl_tan( d ) ) );
	EXPECT_EQ( Bits( NDetMath::ASin( f ) ), Bits( musl_asinf( f ) ) );
	EXPECT_EQ( Bits( NDetMath::ASin( d ) ), Bits( musl_asin( d ) ) );
	EXPECT_EQ( Bits( NDetMath::ACos( f ) ), Bits( musl_acosf( f ) ) );
	EXPECT_EQ( Bits( NDetMath::ACos( d ) ), Bits( musl_acos( d ) ) );
	EXPECT_EQ( Bits( NDetMath::ATan( f ) ), Bits( musl_atanf( f ) ) );
	EXPECT_EQ( Bits( NDetMath::ATan( d ) ), Bits( musl_atan( d ) ) );
	EXPECT_EQ( Bits( NDetMath::Exp( f ) ), Bits( musl_expf( f ) ) );
	EXPECT_EQ( Bits( NDetMath::Exp( d ) ), Bits( musl_exp( d ) ) );
	EXPECT_EQ( Bits( NDetMath::ExpM1( f ) ), Bits( musl_expm1f( f ) ) );
	EXPECT_EQ( Bits( NDetMath::ExpM1( d ) ), Bits( musl_expm1( d ) ) );
	EXPECT_EQ( Bits( NDetMath::Log( f ) ), Bits( musl_logf( f ) ) );
	EXPECT_EQ( Bits( NDetMath::Log( d ) ), Bits( musl_log( d ) ) );
	EXPECT_EQ( Bits( NDetMath::ATan2( f, f ) ), Bits( musl_atan2f( f, f ) ) );
	EXPECT_EQ( Bits( NDetMath::ATan2( d, d ) ), Bits( musl_atan2( d, d ) ) );
	EXPECT_EQ( Bits( NDetMath::Pow( f, f ) ), Bits( musl_powf( f, f ) ) );
	EXPECT_EQ( Bits( NDetMath::Pow( d, d ) ), Bits( musl_pow( d, d ) ) );

	// A mixed pair goes to double, as std::atan2( float, double ) does.
	EXPECT_EQ( Bits( NDetMath::ATan2( f, d ) ), Bits( musl_atan2( static_cast<double>( f ), d ) ) );
	EXPECT_EQ( Bits( NDetMath::Pow( f, 3 ) ), Bits( musl_pow( static_cast<double>( f ), 3.0 ) ) );
}
