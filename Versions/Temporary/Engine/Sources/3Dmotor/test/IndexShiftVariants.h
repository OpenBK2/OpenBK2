#pragma once
// Every implementation of the 16-bit to 32-bit index shift that was measured for
// CDynamicTrisBase::AddPrimitiveGeometry (GfxBuffers.cpp), shared by the test that
// holds them all to the scalar reference (test/IndexShift.cpp) and the benchmark that
// compares them (benchmark/IndexShift.cpp). The one the engine ships is
// NGfx::ShiftIndicesSSE2 in GIndexShift.h (aligned stores); the rest are kept so the
// comparison can be rerun, for instance on a new compiler or a different CPU.
//
// The AVX2 and SSE4.1 variants are compiled with a per-function target attribute on
// GCC and Clang, which only test and benchmark code may do: callers must check
// NGScene::IsAVX2Present() first. MSVC accepts the intrinsics without any /arch flag.
// No lambdas inside those functions, because a GCC lambda does not inherit the
// enclosing function's target and then fails to inline the intrinsics.

#include "3Dmotor/GIndexShift.h"

#include <algorithm>
#include <cstdint>

#if BK2_INDEX_SHIFT_SSE2
#include <immintrin.h>
#endif

#if defined( __GNUC__ ) && !defined( __clang__ ) && __has_include( <experimental/simd>)
#define BK2_INDEX_SHIFT_STDX 1
#include <experimental/simd>
#else
#define BK2_INDEX_SHIFT_STDX 0
#endif

#if defined( __GNUC__ ) || defined( __clang__ )
#define BK2_TARGET_SSE41 __attribute__( ( target( "sse4.1" ) ) )
#define BK2_TARGET_AVX2 __attribute__( ( target( "avx2" ) ) )
#else
#define BK2_TARGET_SSE41
#define BK2_TARGET_AVX2
#endif

namespace NIndexShift
{

// The code as it was before: std::transform with a lambda.
inline void Original( const uint16_t *pSrc, int32_t *pDst, int nSize, int32_t nShift )
{
	std::transform( pSrc, pSrc + nSize, pDst, [nShift]( uint16_t value ) { return value + nShift; } );
}

// Plain loop with __restrict and an ivdep hint, to see whether telling the compiler
// there is no aliasing is enough on its own. At GCC -O2 it is not: the refusal comes
// from the epilogue in the cost model, not from aliasing.
inline void RestrictIvdep( const uint16_t *__restrict pSrc, int32_t *__restrict pDst, int nSize, int32_t nShift )
{
#if defined( __GNUC__ ) && !defined( __clang__ )
#pragma GCC ivdep
#endif
	for ( int i = 0; i < nSize; ++i )
	{
		pDst[i] = static_cast<int32_t>( static_cast<uint32_t>( pSrc[i] ) + static_cast<uint32_t>( nShift ) );
	}
}

#if defined( __GNUC__ ) && !defined( __clang__ )
#define BK2_INDEX_SHIFT_AUTOVEC 1

// The same loop with GCC's auto-vectoriser given the dynamic cost model, which is
// what -O3 would do. GCC documents the optimize attribute as a debugging aid, so
// this is a measurement, not something to ship.
__attribute__( ( optimize( "tree-vectorize", "vect-cost-model=dynamic" ) ) ) inline void
AutoVec( const uint16_t *__restrict pSrc, int32_t *__restrict pDst, int nSize, int32_t nShift )
{
	for ( int i = 0; i < nSize; ++i )
	{
		pDst[i] = static_cast<int32_t>( static_cast<uint32_t>( pSrc[i] ) + static_cast<uint32_t>( nShift ) );
	}
}
#else
#define BK2_INDEX_SHIFT_AUTOVEC 0
#endif

#if BK2_INDEX_SHIFT_SSE2
// The first SSE2 version: eight per iteration with unaligned stores, and instead of a
// scalar tail the last eight indices are converted again, ending exactly at nSize,
// overlapping what the loop already wrote. Fewer instructions than the shipped
// ShiftIndicesSSE2 at small sizes, but a 16-byte store at a 4-byte aligned address
// splits a cache line every fourth store, and once the destination left L1 that made
// it twice as slow. It also writes some lines twice, which write-combining memory
// would not like.
inline void SSE2Unaligned( const uint16_t *pSrc, int32_t *pDst, int nSize, int32_t nShift )
{
	if ( nSize < 8 )
	{
		NGfx::ShiftIndicesRef( pSrc, pDst, nSize, nShift );
		return;
	}
	const __m128i vZero = _mm_setzero_si128();
	const __m128i vShift = _mm_set1_epi32( nShift );
	int i = 0;
	for ( ;; )
	{
		if ( i + 8 > nSize )
		{
			if ( i == nSize )
			{
				break;
			}
			i = nSize - 8;
		}
		const __m128i v = _mm_loadu_si128( reinterpret_cast<const __m128i *>( pSrc + i ) );
		_mm_storeu_si128( reinterpret_cast<__m128i *>( pDst + i ),
		                  _mm_add_epi32( _mm_unpacklo_epi16( v, vZero ), vShift ) );
		_mm_storeu_si128( reinterpret_cast<__m128i *>( pDst + i + 4 ),
		                  _mm_add_epi32( _mm_unpackhi_epi16( v, vZero ), vShift ) );
		i += 8;
	}
}

// SSE2Unaligned with sixteen indices per iteration instead of eight, to see whether
// the loop overhead matters.
inline void SSE2x16( const uint16_t *pSrc, int32_t *pDst, int nSize, int32_t nShift )
{
	if ( nSize < 16 )
	{
		SSE2Unaligned( pSrc, pDst, nSize, nShift );
		return;
	}
	const __m128i vZero = _mm_setzero_si128();
	const __m128i vShift = _mm_set1_epi32( nShift );
	int i = 0;
	for ( ;; )
	{
		// The last partial block reruns the final sixteen, as in SSE2Unaligned.
		if ( i + 16 > nSize )
		{
			if ( i == nSize )
			{
				break;
			}
			i = nSize - 16;
		}
		const __m128i a = _mm_loadu_si128( reinterpret_cast<const __m128i *>( pSrc + i ) );
		const __m128i b = _mm_loadu_si128( reinterpret_cast<const __m128i *>( pSrc + i + 8 ) );
		_mm_storeu_si128( reinterpret_cast<__m128i *>( pDst + i ),
		                  _mm_add_epi32( _mm_unpacklo_epi16( a, vZero ), vShift ) );
		_mm_storeu_si128( reinterpret_cast<__m128i *>( pDst + i + 4 ),
		                  _mm_add_epi32( _mm_unpackhi_epi16( a, vZero ), vShift ) );
		_mm_storeu_si128( reinterpret_cast<__m128i *>( pDst + i + 8 ),
		                  _mm_add_epi32( _mm_unpacklo_epi16( b, vZero ), vShift ) );
		_mm_storeu_si128( reinterpret_cast<__m128i *>( pDst + i + 12 ),
		                  _mm_add_epi32( _mm_unpackhi_epi16( b, vZero ), vShift ) );
		i += 16;
	}
}

// SSE2 with non-temporal stores. movntdq needs a 16-byte aligned destination, so
// a scalar head runs until pDst is aligned (the destination is always 4-byte aligned,
// so that is at most three indices) and a scalar tail finishes the rest. The sfence
// orders the streaming stores before the caller unlocks the buffer.
inline void SSE2Stream( const uint16_t *pSrc, int32_t *pDst, int nSize, int32_t nShift )
{
	int i = 0;
	while ( i < nSize && ( reinterpret_cast<uintptr_t>( pDst + i ) & 15 ) != 0 )
	{
		pDst[i] = static_cast<int32_t>( static_cast<uint32_t>( pSrc[i] ) + static_cast<uint32_t>( nShift ) );
		++i;
	}
	const __m128i vZero = _mm_setzero_si128();
	const __m128i vShift = _mm_set1_epi32( nShift );
	for ( ; i + 8 <= nSize; i += 8 )
	{
		const __m128i v = _mm_loadu_si128( reinterpret_cast<const __m128i *>( pSrc + i ) );
		_mm_stream_si128( reinterpret_cast<__m128i *>( pDst + i ),
		                  _mm_add_epi32( _mm_unpacklo_epi16( v, vZero ), vShift ) );
		_mm_stream_si128( reinterpret_cast<__m128i *>( pDst + i + 4 ),
		                  _mm_add_epi32( _mm_unpackhi_epi16( v, vZero ), vShift ) );
	}
	for ( ; i < nSize; ++i )
	{
		pDst[i] = static_cast<int32_t>( static_cast<uint32_t>( pSrc[i] ) + static_cast<uint32_t>( nShift ) );
	}
	_mm_sfence();
}

// SSE4.1 pmovzxwd: four indices per instruction straight from memory, no zero
// register and no unpack. Eight per iteration, overlapping remainder.
BK2_TARGET_SSE41 inline void SSE41( const uint16_t *pSrc, int32_t *pDst, int nSize, int32_t nShift )
{
	if ( nSize < 8 )
	{
		NGfx::ShiftIndicesRef( pSrc, pDst, nSize, nShift );
		return;
	}
	const __m128i vShift = _mm_set1_epi32( nShift );
	int i = 0;
	for ( ;; )
	{
		if ( i + 8 > nSize )
		{
			if ( i == nSize )
			{
				break;
			}
			i = nSize - 8;
		}
		const __m128i lo = _mm_cvtepu16_epi32( _mm_loadl_epi64( reinterpret_cast<const __m128i *>( pSrc + i ) ) );
		const __m128i hi = _mm_cvtepu16_epi32( _mm_loadl_epi64( reinterpret_cast<const __m128i *>( pSrc + i + 4 ) ) );
		_mm_storeu_si128( reinterpret_cast<__m128i *>( pDst + i ), _mm_add_epi32( lo, vShift ) );
		_mm_storeu_si128( reinterpret_cast<__m128i *>( pDst + i + 4 ), _mm_add_epi32( hi, vShift ) );
		i += 8;
	}
}

// AVX2 vpmovzxwd: eight indices per 256-bit register, sixteen per iteration,
// overlapping remainder; below sixteen it falls back to SSE2. The vzeroupper the
// compiler adds on return keeps the caller's SSE code free of the transition penalty.
BK2_TARGET_AVX2 inline void AVX2( const uint16_t *pSrc, int32_t *pDst, int nSize, int32_t nShift )
{
	if ( nSize < 16 )
	{
		NGfx::ShiftIndicesSSE2( pSrc, pDst, nSize, nShift );
		return;
	}
	const __m256i vShift = _mm256_set1_epi32( nShift );
	int i = 0;
	for ( ;; )
	{
		if ( i + 16 > nSize )
		{
			if ( i == nSize )
			{
				break;
			}
			i = nSize - 16;
		}
		const __m256i a = _mm256_cvtepu16_epi32( _mm_loadu_si128( reinterpret_cast<const __m128i *>( pSrc + i ) ) );
		const __m256i b = _mm256_cvtepu16_epi32( _mm_loadu_si128( reinterpret_cast<const __m128i *>( pSrc + i + 8 ) ) );
		_mm256_storeu_si256( reinterpret_cast<__m256i *>( pDst + i ), _mm256_add_epi32( a, vShift ) );
		_mm256_storeu_si256( reinterpret_cast<__m256i *>( pDst + i + 8 ), _mm256_add_epi32( b, vShift ) );
		i += 16;
	}
}

// AVX2 with ordinary aligned 32-byte stores, for the same write-combining reason as
// NGfx::ShiftIndicesSSE2: scalar head to a 32-byte boundary, scalar tail.
BK2_TARGET_AVX2 inline void AVX2Aligned( const uint16_t *pSrc, int32_t *pDst, int nSize, int32_t nShift )
{
	int i = 0;
	while ( i < nSize && ( reinterpret_cast<uintptr_t>( pDst + i ) & 31 ) != 0 )
	{
		pDst[i] = static_cast<int32_t>( static_cast<uint32_t>( pSrc[i] ) + static_cast<uint32_t>( nShift ) );
		++i;
	}
	const __m256i vShift = _mm256_set1_epi32( nShift );
	for ( ; i + 8 <= nSize; i += 8 )
	{
		const __m256i a = _mm256_cvtepu16_epi32( _mm_loadu_si128( reinterpret_cast<const __m128i *>( pSrc + i ) ) );
		_mm256_store_si256( reinterpret_cast<__m256i *>( pDst + i ), _mm256_add_epi32( a, vShift ) );
	}
	for ( ; i < nSize; ++i )
	{
		pDst[i] = static_cast<int32_t>( static_cast<uint32_t>( pSrc[i] ) + static_cast<uint32_t>( nShift ) );
	}
}

// AVX2 with 32-byte non-temporal stores after a scalar head that aligns pDst.
BK2_TARGET_AVX2 inline void AVX2Stream( const uint16_t *pSrc, int32_t *pDst, int nSize, int32_t nShift )
{
	int i = 0;
	while ( i < nSize && ( reinterpret_cast<uintptr_t>( pDst + i ) & 31 ) != 0 )
	{
		pDst[i] = static_cast<int32_t>( static_cast<uint32_t>( pSrc[i] ) + static_cast<uint32_t>( nShift ) );
		++i;
	}
	const __m256i vShift = _mm256_set1_epi32( nShift );
	for ( ; i + 8 <= nSize; i += 8 )
	{
		const __m256i a = _mm256_cvtepu16_epi32( _mm_loadu_si128( reinterpret_cast<const __m128i *>( pSrc + i ) ) );
		_mm256_stream_si256( reinterpret_cast<__m256i *>( pDst + i ), _mm256_add_epi32( a, vShift ) );
	}
	for ( ; i < nSize; ++i )
	{
		pDst[i] = static_cast<int32_t>( static_cast<uint32_t>( pSrc[i] ) + static_cast<uint32_t>( nShift ) );
	}
	_mm_sfence();
}
#endif

#if BK2_INDEX_SHIFT_STDX
// std::experimental::simd (GCC's Parallelism TS v2 implementation), the native
// width for 32-bit lanes, scalar remainder. GCC only; MSVC does not ship it.
inline void Stdx( const uint16_t *pSrc, int32_t *pDst, int nSize, int32_t nShift )
{
	namespace stdx = std::experimental;
	using VInt = stdx::native_simd<uint32_t>;
	using VShort = stdx::fixed_size_simd<uint16_t, VInt::size()>;
	const int nWidth = static_cast<int>( VInt::size() );
	const VInt vShift = static_cast<uint32_t>( nShift );
	int i = 0;
	for ( ; i + nWidth <= nSize; i += nWidth )
	{
		const VShort s( pSrc + i, stdx::element_aligned );
		const VInt v = stdx::static_simd_cast<VInt>( s ) + vShift;
		v.copy_to( reinterpret_cast<uint32_t *>( pDst + i ), stdx::element_aligned );
	}
	for ( ; i < nSize; ++i )
	{
		pDst[i] = static_cast<int32_t>( static_cast<uint32_t>( pSrc[i] ) + static_cast<uint32_t>( nShift ) );
	}
}
#endif

} // namespace NIndexShift
