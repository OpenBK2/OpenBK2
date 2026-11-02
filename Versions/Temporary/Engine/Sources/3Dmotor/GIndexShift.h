#pragma once
// Widening copy of 16-bit triangle indices into a 32-bit index buffer, adding the
// vertex offset of the mesh inside the shared dynamic vertex buffer. This is what
// CDynamicTrisBase::AddPrimitiveGeometry (GfxBuffers.cpp) runs for every mesh it
// batches, and on a heavy replay it was about 9% of the main thread.
//
// GCC at -O2 does not vectorise the plain loop (its "very cheap" cost model refuses
// any loop that needs an epilogue), so it ran one movzwl/add/mov per index. The SSE2
// version below does eight indices per iteration and needs no CPUID check: SSE2 is
// the x86-64 baseline, and the x86 builds already assume it (MSVC /arch:SSE2 is its
// default). Where SSE2 is not available at compile time the scalar loop is used.
//
// The candidates that lost or were not worth their cost (AVX2, SSE4.1, non-temporal
// stores, unaligned stores, std::experimental::simd, forcing the auto-vectoriser)
// live in test/IndexShiftVariants.h together with their benchmark
// (benchmark/IndexShift.cpp) and the test that holds every variant to the scalar
// reference bit for bit (test/IndexShift.cpp).

#include <cstdint>

#if defined( __SSE2__ ) || defined( _M_X64 ) || ( defined( _M_IX86_FP ) && _M_IX86_FP >= 2 )
#define BK2_INDEX_SHIFT_SSE2 1
#include <emmintrin.h>
#else
#define BK2_INDEX_SHIFT_SSE2 0
#endif

namespace NGfx
{

// The reference: exactly what the original std::transform computed, with the
// addition done in unsigned arithmetic so that it is defined for every input. The
// SIMD paddd wraps the same way, so the two agree even past INT_MAX, which the
// engine never reaches (the dynamic vertex buffer holds far fewer vertices).
inline void ShiftIndicesRef( const uint16_t *pSrc, int32_t *pDst, int nSize, int32_t nShift )
{
	for ( int i = 0; i < nSize; ++i )
	{
		pDst[i] = static_cast<int32_t>( static_cast<uint32_t>( pSrc[i] ) + static_cast<uint32_t>( nShift ) );
	}
}

#if BK2_INDEX_SHIFT_SSE2
// Eight indices per iteration: one unaligned 16-byte load, zero-extend both halves
// with punpcklwd/punpckhwd against zero, add the offset, two aligned 16-byte stores.
//
// The stores are aligned on purpose. The destination is a locked dynamic index
// buffer that is only 4-byte aligned, so up to three indices are written one by one
// first to reach a 16-byte boundary. Unaligned 16-byte stores split a cache line on
// every fourth store, and in the benchmark that made the plain storeu loop twice as
// slow once the destination no longer fit in L1. If the buffer is write-combining
// memory the Intel optimization manual (6.7.2.3) gives the same advice: a line-
// crossing store becomes two bus transactions. The tail is written forward with no
// overlap, so no line is written twice and nothing reads the destination, which on
// write-combining memory would be very slow.
//
// Ordinary stores, not movntdq: on write-combining memory the non-temporal hint
// changes nothing (manual 9.4.1.3), and on cacheable memory it lost badly whenever
// the buffer was still in cache.
inline void ShiftIndicesSSE2( const uint16_t *pSrc, int32_t *pDst, int nSize, int32_t nShift )
{
	int i = 0;
	// Head: at most three, since pDst is always 4-byte aligned.
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
		_mm_store_si128( reinterpret_cast<__m128i *>( pDst + i ), _mm_add_epi32( _mm_unpacklo_epi16( v, vZero ), vShift ) );
		_mm_store_si128( reinterpret_cast<__m128i *>( pDst + i + 4 ),
		                 _mm_add_epi32( _mm_unpackhi_epi16( v, vZero ), vShift ) );
	}
	// One more aligned block of four if that many are left, so the scalar tail is at
	// most three as well.
	if ( i + 4 <= nSize )
	{
		const __m128i v = _mm_loadl_epi64( reinterpret_cast<const __m128i *>( pSrc + i ) );
		_mm_store_si128( reinterpret_cast<__m128i *>( pDst + i ), _mm_add_epi32( _mm_unpacklo_epi16( v, vZero ), vShift ) );
		i += 4;
	}
	for ( ; i < nSize; ++i )
	{
		pDst[i] = static_cast<int32_t>( static_cast<uint32_t>( pSrc[i] ) + static_cast<uint32_t>( nShift ) );
	}
}
#endif

// The variant the engine uses.
inline void ShiftIndices( const uint16_t *pSrc, int32_t *pDst, int nSize, int32_t nShift )
{
#if BK2_INDEX_SHIFT_SSE2
	ShiftIndicesSSE2( pSrc, pDst, nSize, nShift );
#else
	ShiftIndicesRef( pSrc, pDst, nSize, nShift );
#endif
}

} // namespace NGfx
