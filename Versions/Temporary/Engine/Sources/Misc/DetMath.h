#pragma once

// Transcendental functions that give the same bits on every platform, for the
// whole engine: NDetMath::Sin, Cos, Tan, ASin, ACos, ATan, ATan2, Exp, ExpM1,
// Log and Pow. Use these, never std::sin and friends or the C names (sinf).
//
// MSVC's CRT and glibc are each accurate to within an ulp, but not the same
// ulp, so the simulation computed with one diverges from the other: a campaign
// replay recorded on Windows went out of sync on Linux at a single cosf. These
// call musl's libm, vendored in vendor/musl-libm and built by each platform's
// own compiler, which agree bit for bit. See vendor/musl-libm/README.md and
// docs/Determinism.md.
//
// The overloads follow std's rules exactly, so replacing std::sin with
// NDetMath::Sin changes the library and nothing else: a float argument gives
// float arithmetic and a float result, a double or an integer gives double,
// and the two-argument functions take double if either argument is not
// float. long double does not compile: it is 64 bits on MSVC and 80 on gcc.
//
// sqrt, fabs, floor, ceil and fmod are exact by IEEE 754 in every CRT and stay
// std::.

#include "musl_libm.h"

#include <type_traits>

namespace NDetMath
{
namespace NImpl
{
	template <typename T>
	constexpr bool IS_REAL_ARG = std::is_arithmetic_v<T> && !std::is_same_v<T, long double>;

	// float for float, double for double and the integers, as std does.
	template <typename T>
	using TResult1 = std::enable_if_t<IS_REAL_ARG<T>, std::conditional_t<std::is_same_v<T, float>, float, double>>;

	// float only when both are float, as std does.
	template <typename T1, typename T2>
	using TResult2 =
			std::enable_if_t<IS_REAL_ARG<T1> && IS_REAL_ARG<T2>,
	                     std::conditional_t<std::is_same_v<T1, float> && std::is_same_v<T2, float>, float, double>>;
} // namespace NImpl

template <typename T>
inline NImpl::TResult1<T> Sin( const T x )
{
	if constexpr ( std::is_same_v<T, float> )
	{
		return musl_sinf( x );
	}
	else
	{
		return musl_sin( static_cast<double>( x ) );
	}
}

template <typename T>
inline NImpl::TResult1<T> Cos( const T x )
{
	if constexpr ( std::is_same_v<T, float> )
	{
		return musl_cosf( x );
	}
	else
	{
		return musl_cos( static_cast<double>( x ) );
	}
}

template <typename T>
inline NImpl::TResult1<T> Tan( const T x )
{
	if constexpr ( std::is_same_v<T, float> )
	{
		return musl_tanf( x );
	}
	else
	{
		return musl_tan( static_cast<double>( x ) );
	}
}

template <typename T>
inline NImpl::TResult1<T> ASin( const T x )
{
	if constexpr ( std::is_same_v<T, float> )
	{
		return musl_asinf( x );
	}
	else
	{
		return musl_asin( static_cast<double>( x ) );
	}
}

template <typename T>
inline NImpl::TResult1<T> ACos( const T x )
{
	if constexpr ( std::is_same_v<T, float> )
	{
		return musl_acosf( x );
	}
	else
	{
		return musl_acos( static_cast<double>( x ) );
	}
}

template <typename T>
inline NImpl::TResult1<T> ATan( const T x )
{
	if constexpr ( std::is_same_v<T, float> )
	{
		return musl_atanf( x );
	}
	else
	{
		return musl_atan( static_cast<double>( x ) );
	}
}

template <typename T>
inline NImpl::TResult1<T> Exp( const T x )
{
	if constexpr ( std::is_same_v<T, float> )
	{
		return musl_expf( x );
	}
	else
	{
		return musl_exp( static_cast<double>( x ) );
	}
}

// exp( x ) - 1, without the cancellation near zero.
template <typename T>
inline NImpl::TResult1<T> ExpM1( const T x )
{
	if constexpr ( std::is_same_v<T, float> )
	{
		return musl_expm1f( x );
	}
	else
	{
		return musl_expm1( static_cast<double>( x ) );
	}
}

template <typename T>
inline NImpl::TResult1<T> Log( const T x )
{
	if constexpr ( std::is_same_v<T, float> )
	{
		return musl_logf( x );
	}
	else
	{
		return musl_log( static_cast<double>( x ) );
	}
}

template <typename T1, typename T2>
inline NImpl::TResult2<T1, T2> ATan2( const T1 y, const T2 x )
{
	if constexpr ( std::is_same_v<NImpl::TResult2<T1, T2>, float> )
	{
		return musl_atan2f( y, x );
	}
	else
	{
		return musl_atan2( static_cast<double>( y ), static_cast<double>( x ) );
	}
}

template <typename T1, typename T2>
inline NImpl::TResult2<T1, T2> Pow( const T1 x, const T2 y )
{
	if constexpr ( std::is_same_v<NImpl::TResult2<T1, T2>, float> )
	{
		return musl_powf( x, y );
	}
	else
	{
		return musl_pow( static_cast<double>( x ), static_cast<double>( y ) );
	}
}
} // namespace NDetMath
