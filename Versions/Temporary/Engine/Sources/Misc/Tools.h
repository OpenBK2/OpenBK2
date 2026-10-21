#pragma once

#include "Misc_export.h"

// ASSERT, used inline below. Asserts.h deliberately does not include this
// header back, declaring the one thing it needs from it instead, so naming it
// here is the direction that does not form a cycle.
#include "Asserts.h"
#include "port/stdcall.h"

// CHAR_BIT and the float format limits, for the platform assertions below.
// <cfloat> rather than <limits>, deliberately: both are macro-only C headers,
// where <limits> declares members named min and max and would break in any
// translation unit that saw windows.h first, since NOMINMAX is not set
// globally here.
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdint>
#include <type_traits>

#include <boost/config.hpp>
#include <boost/core/bit.hpp>
#include <boost/predef/architecture.h>
// BOOST_ENDIAN_LITTLE_BYTE, for the platform assertions below
#include <boost/predef/other/endian.h>
// cvtss2si, for Float2Int
#if BOOST_ARCH_X86
#include <xmmintrin.h>
#endif
#include <boost/math/special_functions/sign.hpp>
#include <fmt/printf.h>

// ************************************************************************************************************************ //
// **
// ** what this engine assumes about the machine
// **
// ** Here because this header reaches almost every translation unit, so these
// ** are actually compiled. A header nobody includes asserts nothing.
// **
// ************************************************************************************************************************ //

// The standard permits far more than this: int need only hold 16 bits, bool's
// size is unspecified, nothing guarantees IEEE 754 or two's complement before
// C++20, and CHAR_BIT may exceed 8. Every target this game ships on is x86 or
// x64 with MSVC, GCC or clang, where all of the below holds, and the code is
// built on it in two places that cannot tolerate being wrong quietly:
//
//  - Serialization. IBinSaver's scalar path is DataChunk( id, p, sizeof( T ) )
//    and every saver writes the object's bytes with memcpy, so the width of a
//    type, the number of bits in a byte and the byte order are all part of the
//    save, the replay and the network packet. 639 serialized members are a
//    plain int and 552 are a bool.
//  - Lockstep. AILogic compares float state between clients by checksumming
//    its bytes, so the representation is part of the protocol, not just of the
//    arithmetic.
//
// So these say out loud what the formats depend on, and a port to a machine
// where one of them fails gets a compile error at the place that explains why
// rather than a desynchronising save. docs/SerializedTypeWidths.md has the
// measurements, and NBinSaverDetail::TIsWidthVarying in System/BinSaver.h
// refuses the two types that differ between the platforms we already ship.
static_assert( CHAR_BIT == 8, "every serialized size is a byte count and every saver memcpy's object bytes" );
static_assert( sizeof( short ) == 2 && sizeof( int ) == 4 && sizeof( long long ) == 8,
               "these widths are in the save format; int in 639 serialized members alone" );
static_assert( sizeof( bool ) == 1, "552 serialized members are a bool" );
static_assert( sizeof( float ) == 4 && sizeof( double ) == 8, "both are serialized and checksummed by their bytes" );
// binary32 and binary64, which is to say IEEE 754: radix two, a 24 and 53 bit
// significand, and the matching exponent ranges.
static_assert( FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024,
               "the lockstep checksum compares float state as bytes, so the representation is part of the protocol" );
static_assert( -1 == ~0, "two's complement, which only C++20 mandates and which negative values in saves rely on" );
static_assert( BOOST_ENDIAN_LITTLE_BYTE,
               "scalars are written by memcpy, so the byte order is the format rather than an implementation detail" );

// square root of the 2 and 3
#define SQRT_2 1.41421356237309504880
#define SQRT_3 1.73205080756887729353
#define FP_SQRT_2 1.41421356f
#define FP_SQRT_3 1.73205081f
// different constants with 'pi'
#define PI 3.14159265358979323846
#define FP_PI 3.14159265f
#define FP_2PI 6.28318531f
#define FP_4PI 12.56637061f
#define FP_8PI 25.13274123f
#define FP_PI2 1.57079633f
#define FP_PI4 0.78539816f
#define FP_PI8 0.39269908f
#define FP_INV_PI 0.31830989f
#define FP_EPSILON 1e-12f
#define FP_EPSILON2 1e-24f

#define FP_MAX_VALUE 3.402823466e+38f
#define FP_MIN_VALUE 1.175494351e-38f

#define EPS_VALUE 1.192092896e-7f

// ************************************************************************************************************************ //
// **
// ** some FP tricks
// **
// **
// **
// ************************************************************************************************************************ //

// access float as uint32_t
//
// A strict aliasing violation, and the class that silently miscompiled the DXT
// decoder, but it is used as an lvalue at some of its nine call sites
// (FastMath, DiscretePos, Geom.h), so replacing it with bit_cast below is a
// change of its own rather than a deletion.
#define FP_BITS( fp ) ( *(uint32_t *)( &( fp ) ) )

// FP_BITS_CONST and FP_ONE_BITS stood here and are gone with the last of the
// unused bit-twiddling macros in GeomMisc.h: those were their only users.

// ************************************************************************************************************************ //
// **
// ** pack/unpack
// **
// **
// **
// ************************************************************************************************************************ //

// побитовое приведение типа
//
// This was *reinterpret_cast<const TYPE_OUT *>( &val ), which is a
// strict-aliasing violation: a read through TYPE_OUT does not alias the TYPE_IN
// object, so the compiler may assume the store into it never happened. That is
// not theoretical here. GCC reports exactly that at every site in this tree
// that punned a float by hand, as "'f' is used uninitialized", and the same
// class silently miscompiled the DXT decoder under clang at -O2 while GCC
// happened to produce the right answer.
//
// boost::core::bit_cast is the defined spelling, already available through the
// boost/core/bit.hpp included above. It lowers to __builtin_bit_cast on both
// compilers here, so this costs nothing, and it static_asserts that the two
// types are the same size, which the hand-rolled cast did not: Image's
// FP_NORM_TO_BYTE3 was reading an 8-byte unsigned long out of a 4-byte float
// under LP64.
template <class TYPE_OUT, class TYPE_IN>
inline TYPE_OUT bit_cast( const TYPE_IN &val )
{
	return boost::core::bit_cast<TYPE_OUT>( val );
}

// ************************************************************************************************************************ //
// трюки с битами
// ************************************************************************************************************************ //

// Return the next power of 2 higher than the input
// If the input is already a power of 2, the output will be the same as the input.
// Got this from Brian Sharp's sweng mailing list.
// The hand-rolled routines these replaced had per-width overloads, and their
// results for zero depended on that width. make_unsigned_t<T> reproduces the
// original 32/16/8-bit domains exactly, rather than widening everything to
// uint64_t -- which would also turn a negative int argument into a huge value.
template <typename T>
inline int GetNextPow2( T n )
{
	// only 32-bit overloads existed (uint32_t and int), so narrower arguments
	// were promoted; reproduce that instead of letting bit_ceil overflow a
	// narrow type. Old code did n-1, smeared the bits down, then +1, which
	// wrapped back to 0 for n == 0.
	using U = std::make_unsigned_t<T>;
	using P = std::conditional_t<( sizeof( U ) < sizeof( uint32_t ) ), uint32_t, U>;
	const P u = static_cast<P>( static_cast<U>( n ) );
	// bit_ceil is undefined when the result is not representable; the old code
	// smeared to all-ones and wrapped to 0 there, so reproduce that.
	constexpr P highest = P( 1 ) << ( sizeof( P ) * 8 - 1 );
	if ( u == 0 || u > highest )
	{
		return 0;
	}
	return static_cast<int>( boost::core::bit_ceil( u ) );
}

// получить старший включенный бит
template <typename T>
inline int GetMSB( T n )
{
	// old code left k at 0 when no bit was set, so 0 and 1 both gave 0
	using U = std::make_unsigned_t<T>;
	const U u = static_cast<U>( n );
	return u == 0 ? 0 : boost::core::bit_width( u ) - 1;
}

// получить младший включенный бит
template <typename T>
inline int GetLSB( T n )
{
	// every shift step fired for zero, leaving k at width-1: 31 / 15 / 7 for
	// the 32 / 16 / 8-bit overloads respectively
	using U = std::make_unsigned_t<T>;
	const U u = static_cast<U>( n );
	return u == 0 ? static_cast<int>( sizeof( U ) * 8 - 1 ) : boost::core::countr_zero( u );
}

// подсчёт колличества ненулевых бит в числе
// 0x49249249ul // = 0100_1001_0010_0100_1001_0010_0100_1001
// 0x381c0e07ul // = 0011_1000_0001_1100_0000_1110_0000_0111
template <typename T>
inline int GetNumBits( T v )
{
	// popcount needs an unsigned type; the old int overload counted via
	// uint32_t(v), so a negative argument yielded 32
	return boost::core::popcount( static_cast<std::make_unsigned_t<T>>( v ) );
}

// ************************************************************************************************************************ //
// обнуление памяти по типу переменной
// ************************************************************************************************************************ //
template <class TYPE>
inline void Zero( TYPE &val )
{
	memset( &val, 0, sizeof( val ) );
}

// ************************************************************************************************************************ //
// radian <=> degree conversion functions
// ************************************************************************************************************************ //
inline float ToRadian( const float angle )
{
	return float( angle * ( FP_PI / 180.0f ) );
}

inline float ToDegree( const float angle )
{
	return float( angle * ( 180.0f / FP_PI ) );
}

inline float NormalizeAngleInDegree( const float angle )
{
	float fResult = fmod( double( angle ), 360.0 );
	if ( fResult < 0 )
	{
		fResult += 360.f;
	}
	ASSERT( 0.f <= fResult && fResult < 360.f );
	return fResult;
}

inline int NormalizeAngleInDegree( const int angle )
{
	return angle % 360;
}

inline float SignumNormalizeAngleInDegree( const float angle )
{
	return float( fmod( angle + 180 * boost::math::sign( angle ), 360 ) - 180 * boost::math::sign( angle ) );
}

inline float NormalizeAngleInRadian( const float angle )
{
	return float( fmod( double( angle ), 2.0 * PI ) + ( double( angle ) < 0 ? 2.0 * PI : 0 ) );
}

inline float SignumNormalizeAngleInRadian( const float angle )
{
	return float( fmod( double( angle ) + PI, 2.0 * PI ) + ( double( angle ) < -PI ? PI : -PI ) );
}

// ************************************************************************************************************************ //
// ** float-to-int преобразование с текущим состоянием процессора
// ************************************************************************************************************************ //
// very fast float-to-int conversion. WARNING: uses current FPU rounding state (!)
//
// The original was fld/fistp, which rounds by the current rounding mode, not by
// truncation, and callers depend on both halves of that: most run under the
// default round-to-nearest-even (GroupLogic::Segment asserts it), while the DB
// load and the console SetVar paths switch to RCM_CHOP on purpose, and
// VarIntHandler reaches Float2Int from inside the latter. A plain static_cast
// always truncates and so broke the first group; a mode-independent round
// (boost::math::iround, glm::round, glm::roundEven) would break the second.
//
// Rounding a float to an integer is an exact IEEE operation, so honouring the
// mode is bit exact on every platform provided the mode itself is the same,
// which CRoundingControl sees to. On x86 cvtss2si is the SSE equivalent of
// fistp, reading MXCSR (which fesetround sets alongside the x87 control word)
// and returning the integer indefinite 0x80000000 for NaN and for anything out
// of int range, as fistp does. Elsewhere nearbyint does the rounding and the
// out of range case is made explicit so the result matches x86.
BOOST_FORCEINLINE int Float2Int( const float fVal )
{
#if BOOST_ARCH_X86
	return _mm_cvt_ss2si( _mm_set_ss( fVal ) );
#else
	const float fRounded = std::nearbyint( fVal );
	// written so that NaN fails the test too
	if ( !( fRounded >= -2147483648.0f && fRounded < 2147483648.0f ) )
	{
		return INT32_MIN;
	}
	return static_cast<int>( fRounded );
#endif
}

// clamp - обрезать число с двух сторон (min/max)
template <class TYPE>
inline const TYPE Clamp( const TYPE tVal, const TYPE tMin, const TYPE tMax )
{
	// standard std::clamp asserts for invalid values in debug
	return (std::max)( tMin, (std::min)( tVal, tMax ) );
}

// ************************************************************************************************************************ //
// получение модуля от разных величин
// ************************************************************************************************************************ //
inline float fabs2( const float x, const float y, const float z, const float w )
{
	return ( x * x + y * y + z * z + w * w );
}

inline float fabs( const float x, const float y, const float z, const float w )
{
	return static_cast<float>( sqrt( fabs2( x, y, z, w ) ) );
}

inline float fabs2( const float x, const float y, const float z )
{
	return ( x * x + y * y + z * z );
}

inline float fabs( const float x, const float y, const float z )
{
	return static_cast<float>( sqrt( fabs2( x, y, z ) ) );
}

inline float fabs2( const float x, const float y )
{
	return ( x * x + y * y );
}

inline float fabs( const float x, const float y )
{
	return static_cast<float>( sqrt( fabs2( x, y ) ) );
}

inline float fabs2( const float x )
{
	return x * x;
}

// legacy functions to be compatible with A5 and A7
inline float sqr( const float x )
{
	return x * x;
}

inline float triple( const float x )
{
	return x * x * x;
}

template <class TYPE>
inline TYPE square( const TYPE x )
{
	return x * x;
}

template <class TYPE>
inline bool Normalize( TYPE &x, TYPE &y )
{
	TYPE u = fabs2( x, y );
	if ( fabs( u - TYPE( 1 ) ) < FP_EPSILON )
	{
		return true; // already normalized
	}
	if ( u < FP_EPSILON2 )
	{
		return false; // can't normalize
	}
	u = static_cast<TYPE>( TYPE( 1 ) / sqrt( u ) );
	x *= u;
	y *= u;
	return true;
}

template <class TYPE>
inline bool Normalize( TYPE &x, TYPE &y, TYPE &z )
{
	TYPE u = fabs2( x, y, z );
	if ( fabs( u - TYPE( 1 ) ) < FP_EPSILON )
	{
		return true; // already normalized
	}
	if ( u < FP_EPSILON2 )
	{
		return false; // can't normalize
	}
	u = static_cast<TYPE>( TYPE( 1 ) / sqrt( u ) );
	x *= u;
	y *= u;
	z *= u;
	return true;
}

template <class TYPE>
inline bool Normalize( TYPE &x, TYPE &y, TYPE &z, TYPE &w )
{
	TYPE u = fabs2( x, y, z, w );
	if ( fabs( u - TYPE( 1 ) ) < FP_EPSILON )
	{
		return true; // already normalized
	}
	if ( u < FP_EPSILON2 )
	{
		return false; // can't normalize
	}
	u = static_cast<TYPE>( TYPE( 1 ) / sqrt( u ) );
	x *= u;
	y *= u;
	z *= u;
	w *= u;
	return true;
}

// fmt::printf_args is a type erased argument pack: it crosses the DLL boundary the
// way the old va_list did, except that every argument still carries its own type.
// That is what lets "%d" with a size_t print the right number instead of reading the
// wrong number of bytes off the stack. Formatting itself stays inside Misc, so the
// call sites instantiate nothing but the two line wrapper below.
MISC_EXPORT void PORT_STDCALL DbgTrcArgs( fmt::string_view fmtStr, fmt::printf_args args );

// Writes the text exactly as given, with no newline of its own. For the few callers
// that already place their own line breaks, or that deliberately leave a line open.
// Everything else wants DbgTrc, which terminates each call.
MISC_EXPORT void PORT_STDCALL DbgTrcRaw( const char *pszText );

template <typename... TArgs>
void DbgTrc( fmt::string_view fmtStr, const TArgs &...args )
{
	DbgTrcArgs( fmtStr, fmt::make_printf_args( args... ) );
}

#ifndef _FINALRELEASE
#define DebugTrace DbgTrc
#else
// Still a template taking the same arguments, so a call that compiles in one
// configuration compiles in the other.
template <typename... TArgs>
void DebugTrace( fmt::string_view, const TArgs &... )
{
}
#endif // _FINALRELEASE

#ifdef NIVAL_DLL
#define EXTERNVAR __declspec( dllimport ) extern
#else
#define EXTERNVAR extern
#endif
