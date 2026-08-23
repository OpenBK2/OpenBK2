#pragma once

#include "Misc_export.h"

#include "port/stdcall.h"

#include <cmath>
#include <cstdint>
#include <type_traits>

#include <boost/config.hpp>
#include <boost/core/bit.hpp>
#include <boost/math/special_functions/sign.hpp>
#include <fmt/printf.h>

// square root of the 2 and 3
#define SQRT_2		1.41421356237309504880
#define SQRT_3		1.73205080756887729353
#define FP_SQRT_2	1.41421356f
#define FP_SQRT_3	1.73205081f
// different constants with 'pi'
#define PI					3.14159265358979323846
#define FP_PI				3.14159265f
#define FP_2PI			6.28318531f
#define FP_4PI			12.56637061f
#define FP_8PI			25.13274123f
#define FP_PI2			1.57079633f
#define FP_PI4			0.78539816f
#define FP_PI8			0.39269908f
#define FP_INV_PI		0.31830989f
#define FP_EPSILON	1e-12f
#define FP_EPSILON2	1e-24f

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
#define FP_BITS( fp ) ( *(uint32_t*)(&(fp)) )
#define FP_BITS_CONST( fp ) ( *(const uint32_t*)(&(fp)) )
// floating pt 1.0
#define FP_ONE_BITS 0x3F800000

// ************************************************************************************************************************ //
// **
// ** pack/unpack
// **
// **
// **
// ************************************************************************************************************************ //

// побитовое приведение типа
template <class TYPE_OUT, class TYPE_IN>
inline TYPE_OUT bit_cast( const TYPE_IN &val )
{
	return *reinterpret_cast<const TYPE_OUT*>( &val );
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
template<typename T>
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
template<typename T>
inline int GetMSB( T n )
{
	// old code left k at 0 when no bit was set, so 0 and 1 both gave 0
	using U = std::make_unsigned_t<T>;
	const U u = static_cast<U>( n );
	return u == 0 ? 0 : boost::core::bit_width( u ) - 1;
}

// получить младший включенный бит
template<typename T>
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
template<typename T>
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
	memset( &val, 0, sizeof(val) );
}

// ************************************************************************************************************************ //
// radian <=> degree conversion functions
// ************************************************************************************************************************ //
inline float ToRadian( const float angle )
{
	return float( angle * (FP_PI/180.0f) );
}
inline float ToDegree( const float angle )
{
	return float( angle * (180.0f/FP_PI) );
}
inline float NormalizeAngleInDegree( const float angle )
{
	float fResult = fmod( double(angle), 360.0 );
	if ( fResult < 0 )
		fResult += 360.f;
	ASSERT( 0.f <= fResult && fResult < 360.f );
	return fResult;
}
inline int NormalizeAngleInDegree( const int angle )
{
	return angle % 360;
}
inline float SignumNormalizeAngleInDegree( const float angle )
{
	return float( fmod( angle + 180*boost::math::sign(angle),  360 ) - 180 * boost::math::sign( angle ) );
}
inline float NormalizeAngleInRadian( const float angle )
{
	return float( fmod( double(angle), 2.0*PI ) + ( double(angle) < 0 ? 2.0*PI : 0 ) );
}
inline float SignumNormalizeAngleInRadian( const float angle )
{
	return float( fmod( double(angle) + PI, 2.0*PI ) + (double(angle) < -PI ? PI : -PI ) );
}

// ************************************************************************************************************************ //
// ** float-to-int преобразование с текущим состоянием процессора
// ************************************************************************************************************************ //
// very fast float-to-int conversion. WARNING: uses current FPU rounding state (!)
BOOST_FORCEINLINE int Float2Int( const float fVal )
{
	return static_cast<int>(fVal);
}

// clamp - обрезать число с двух сторон (min/max)
template <class TYPE>
inline const TYPE Clamp( const TYPE tVal, const TYPE tMin, const TYPE tMax )
{
	// standard std::clamp asserts for invalid values in debug
	return (std::max)( tMin, (std::min)(tVal, tMax) );
}

// ************************************************************************************************************************ //
// получение модуля от разных величин
// ************************************************************************************************************************ //
inline float fabs2( const float x, const float y, const float z, const float w )
{
	return ( x*x + y*y + z*z + w*w );
}
inline float fabs( const float x, const float y, const float z, const float w )
{
	return static_cast<float>( sqrt( fabs2(x, y, z, w) ) );
}
inline float fabs2( const float x, const float y, const float z )
{
	return ( x*x + y*y + z*z );
}
inline float fabs( const float x, const float y, const float z )
{
	return static_cast<float>( sqrt( fabs2(x, y, z) ) );
}
inline float fabs2( const float x, const float y )
{
	return ( x*x + y*y );
}
inline float fabs( const float x, const float y )
{
	return static_cast<float>( sqrt( fabs2(x, y) ) );
}
inline float fabs2( const float x )
{
	return x*x;
}
// legacy functions to be compatible with A5 and A7
inline float sqr( const float x ) { return x * x; }
inline float triple( const float x ) { return x * x * x; }
template <class TYPE>
inline TYPE square( const TYPE x )
{
	return x*x;
}

template <class TYPE> 
inline bool Normalize( TYPE &x, TYPE &y )
{
  TYPE u = fabs2( x, y );
  if ( fabs(u - TYPE(1)) < FP_EPSILON )
    return true;                        // already normalized
  if ( u < FP_EPSILON2 )
    return false;                       // can't normalize
  u = static_cast<TYPE>( TYPE(1) / sqrt( u ) );
  x *= u;
  y *= u;
  return true;
}
template <class TYPE> 
inline bool Normalize( TYPE &x, TYPE &y, TYPE &z )
{
  TYPE u = fabs2( x, y, z );
  if ( fabs(u - TYPE(1)) < FP_EPSILON )
    return true;                        // already normalized
  if ( u < FP_EPSILON2 )
    return false;                       // can't normalize
  u = static_cast<TYPE>( TYPE(1) / sqrt( u ) );
  x *= u;
  y *= u;
  z *= u;
  return true;
}
template <class TYPE> 
inline bool Normalize( TYPE &x, TYPE &y, TYPE &z, TYPE &w )
{
  TYPE u = fabs2( x, y, z, w );
  if ( fabs(u - TYPE(1)) < FP_EPSILON )
    return true;                        // already normalized
  if ( u < FP_EPSILON2 )
    return false;                       // can't normalize
  u = static_cast<TYPE>( TYPE(1) / sqrt( u ) );  
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

template < typename... TArgs >
void DbgTrc( fmt::string_view fmtStr, const TArgs &... args )
{
	DbgTrcArgs( fmtStr, fmt::make_printf_args( args... ) );
}

#ifndef _FINALRELEASE
#define DebugTrace DbgTrc
#else
// Still a template taking the same arguments, so a call that compiles in one
// configuration compiles in the other.
template < typename... TArgs >
void DebugTrace( fmt::string_view, const TArgs &... ) {  }
#endif // _FINALRELEASE

#ifdef NIVAL_DLL
#define EXTERNVAR __declspec(dllimport) extern
#else
#define EXTERNVAR extern
#endif
