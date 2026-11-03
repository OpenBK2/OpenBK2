#pragma once
// War fog sampling for per-vertex lighting, fused: positions in, fog bytes out, for
// one or two fog grids in a single pass.
//
// SPVLightCalcer::CalcLight (GLightPerVertex.cpp) samples the fog at every position of
// a part whenever the part's vertices are rebuilt, which on a replay with dozens of
// moving tanks is every position of every moving part every frame. It does that
// through the kernel table (GLightPerVertexKernels.h) in two or three calls:
// pSampleWarFogCoords turns the positions into 16.14 fixed point, and
// pSampleWarFogInt bilinearly samples a grid at them, once for the new grid and once
// more for the old. The shipped AVX2 pSampleWarFogInt is only nominally vector code:
// every position goes through a scalar helper (four byte loads, two weight words) and
// the four results are packed into a register with _mm_set_epi32 before one madd.
//
// These kernels do the same arithmetic with every step vectorised:
//
//   ix, iy = cvtps2dq( x * ( fScale * 0x4000 ) ), likewise y   (round to nearest even)
//   cx = ( ix >> 14 ) & ( nFogSize - 2 ), fx = ix & 0x3fff      (likewise cy, fy)
//   up   = p[cy][cx], p[cy][cx + 1];  down = p[cy + 1][cx], p[cy + 1][cx + 1]
//   h    = ( a * ( 0x4000 - fx ) + b * fx ) >> 14               (for up and down)
//   out  = ( hUp * ( 0x4000 - fy ) + hDown * fy ) >> 14
//
// - The coordinates of eight positions are converted at once: three loads per four
//   positions and four shuffles separate x and y out of the packed xyz triples.
// - The two bytes of a row are one 16-bit load ("two-row trick"): two loads per
//   position per grid instead of four. The AVX2 kernel issues them as two
//   vpgatherdd (one per row, merged with vpblendw), the SSE2 kernel as pinsrw.
// - Both grids are sampled from one set of coordinates, cells and weights; SetWarFog
//   keeps the old and the new grid the same size, so the cell index is shared too.
//
// BIT-EXACT against the shipped kernels for every input: the coordinate step is the
// same IEEE multiply and the same cvtps2dq the SSE2 and AVX2 kernel sets use (so a
// NaN or out-of-range product becomes INT_MIN in both), and everything after it is
// integer arithmetic whose intermediates never saturate (each step is a weighted mean
// of bytes with weights summing to 0x4000), so any order gives the same bytes.
// Out-of-grid and negative coordinates wrap through the same "& ( nFogSize - 2 )".
// test/WarFogSample.cpp checks this exhaustively; benchmark/WarFogSample.cpp measures.
//
// The scalar reference set (GLightPerVertexRef.cpp) rounds with lrintf instead. That
// agrees with cvtps2dq wherever the product fits in an int. Where it does not (NaN,
// inf, |product| >= 2^31) lrintf is undefined; on LP64 (Linux) it returns a 64-bit
// value whose truncation differs from INT_MIN. Only the kernel sets the dispatcher
// actually hands out (SSE2, AVX2) are matched here; no real map reaches those inputs.
//
// No engine types: a position is three packed floats (CVec3), a grid is nFogSize *
// nFogSize contiguous bytes (CArray2D<unsigned char>), as in the kernel table.
//
// Memory: every kernel reads exactly the nCount * 12 position bytes, writes exactly
// nCount result bytes per grid, and reads the grid only inside its nFogSize *
// nFogSize bytes (the gather of the lower row starts two bytes early instead of
// running two bytes past the end).
//
// Include this header only from translation units compiled for the baseline ISA (the
// engine's GLightPerVertex.cpp, not GLightPerVertexAVX2.cpp): the AVX2 entry points
// carry a per-function target attribute on GCC and clang, and MSVC accepts AVX2
// intrinsics without /arch:AVX2, so no AVX2 flag is needed and no baseline inline
// function gets compiled for AVX2. Call the AVX2 kernel only after IsAVX2Present().

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined( __SSE2__ ) || defined( _M_X64 ) || ( defined( _M_IX86_FP ) && _M_IX86_FP >= 2 )
#define BK2_FOG_SSE2 1
#include <emmintrin.h>
#else
#define BK2_FOG_SSE2 0
#endif

// AVX2 code is available on x86 and x64 with every compiler the project uses: GCC
// and clang through the target attribute, MSVC unconditionally.
#if BK2_FOG_SSE2 && ( defined( __GNUC__ ) || defined( __clang__ ) )
#define BK2_FOG_AVX2 1
#define BK2_FOG_TARGET_AVX2 __attribute__( ( target( "avx2" ) ) )
#include <immintrin.h>
#elif BK2_FOG_SSE2 && defined( _MSC_VER )
#define BK2_FOG_AVX2 1
#define BK2_FOG_TARGET_AVX2
#include <immintrin.h>
#else
#define BK2_FOG_AVX2 0
#define BK2_FOG_TARGET_AVX2
#endif

// Same reason as BK2_NORMALS_INLINE in GNormalTransform.h: GCC at -O2 keeps small
// helpers that are called from two places out of line, which costs a call per block.
#if defined( _MSC_VER )
#define BK2_FOG_INLINE __forceinline
#else
#define BK2_FOG_INLINE inline __attribute__( ( always_inline ) )
#endif

namespace NWarFog
{

// How the AVX2 kernel fetches the 2x2 neighbourhood. Gather is the recommended one;
// the others are kept so the benchmark can show why.
enum class EAVX2Load
{
	Gather,           // two vpgatherdd per grid (upper row, lower row), merged with vpblendw
	Insert,           // sixteen 16-bit loads per grid through pinsrw, as the SSE2 kernel does
	GatherBlockCheck, // Gather, but when all eight positions share one cell, one load
};

namespace NDetail
{

	BK2_FOG_INLINE uint32_t Load16( const uint8_t *p )
	{
		uint16_t v;
		std::memcpy( &v, p, sizeof( v ) );
		return v;
	}

	// The 16.14 scale exactly as the shipped kernels form it: a float times 0x4000.
	BK2_FOG_INLINE float FixedScale( float fScale )
	{
		return fScale * 0x4000;
	}

	// float -> int the way cvtps2dq does it: round to nearest even under the default
	// MXCSR, NaN and out-of-range values become INT_MIN.
	BK2_FOG_INLINE int32_t RoundLikeSSE( float f )
	{
#if BK2_FOG_SSE2
		return _mm_cvtss_si32( _mm_set_ss( f ) );
#else
		if ( f >= -2147483648.0f && f < 2147483648.0f )
		{
			return static_cast<int32_t>( std::nearbyint( f ) );
		}
		return INT32_MIN;
#endif
	}

	// One sample from integer coordinates. This is the arithmetic of
	// GLightPerVertexRef.cpp's SampleWarFogInt written out without the MMX emulation.
	BK2_FOG_INLINE uint8_t SampleInt( int32_t nX, int32_t nY, const uint8_t *pFog, int nFogSize )
	{
		const int nMask = nFogSize - 2;
		const int nCX = ( nX >> 14 ) & nMask;
		const int nCY = ( nY >> 14 ) & nMask;
		const int nFX = nX & 0x3fff;
		const int nFY = nY & 0x3fff;
		const uint8_t *pUp = pFog + nCY * nFogSize + nCX;
		const uint8_t *pDown = pUp + nFogSize;
		const int nUp = ( pUp[0] * ( 0x4000 - nFX ) + pUp[1] * nFX ) >> 14;
		const int nDown = ( pDown[0] * ( 0x4000 - nFX ) + pDown[1] * nFX ) >> 14;
		return static_cast<uint8_t>( ( nUp * ( 0x4000 - nFY ) + nDown * nFY ) >> 14 );
	}

	// pDst[0..n) = pSrc[0..n) for n < 8, in fixed-size pieces (no library call).
	BK2_FOG_INLINE void StoreShort( uint8_t *pDst, const uint8_t *pSrc, int n )
	{
		int i = 0;
		if ( n & 4 )
		{
			std::memcpy( pDst, pSrc, 4 );
			i = 4;
		}
		if ( n & 2 )
		{
			std::memcpy( pDst + i, pSrc + i, 2 );
			i += 2;
		}
		if ( n & 1 )
		{
			pDst[i] = pSrc[i];
		}
	}

} // namespace NDetail

// Portable version, one position at a time. pPos holds nCount packed xyz triples;
// pFogA and pResA are required, pFogB / pResB may both be null to sample one grid.
// Both grids are nFogSize * nFogSize bytes with nFogSize >= 2.
inline void SampleWarFogScalar( const float *pPos,
                                int nCount,
                                float fScale,
                                int nFogSize,
                                const uint8_t *pFogA,
                                uint8_t *pResA,
                                const uint8_t *pFogB,
                                uint8_t *pResB )
{
	const float fFixed = NDetail::FixedScale( fScale );
	for ( int k = 0; k < nCount; ++k )
	{
		const int32_t nX = NDetail::RoundLikeSSE( pPos[3 * k] * fFixed );
		const int32_t nY = NDetail::RoundLikeSSE( pPos[3 * k + 1] * fFixed );
		pResA[k] = NDetail::SampleInt( nX, nY, pFogA, nFogSize );
		if ( pFogB )
		{
			pResB[k] = NDetail::SampleInt( nX, nY, pFogB, nFogSize );
		}
	}
}

#if BK2_FOG_SSE2

namespace NDetailSSE2
{

	// Coordinates and weights of four positions, shared by both grids.
	struct SCells4
	{
		alignas( 16 ) int32_t nIndex[4]; // cy * nFogSize + cx
		__m128i xWeights;                // per position, words ( 0x4000 - fx, fx )
		__m128i yWeights;                // per position, words ( 0x4000 - fy, fy )
	};

	// x and y of four packed xyz triples (twelve floats): three loads, four shuffles.
	BK2_FOG_INLINE void LoadXY4( const float *p, __m128 &x, __m128 &y )
	{
		const __m128 m0 = _mm_loadu_ps( p );                                   // x0 y0 z0 x1
		const __m128 m1 = _mm_loadu_ps( p + 4 );                               // y1 z1 x2 y2
		const __m128 m2 = _mm_loadu_ps( p + 8 );                               // z2 x3 y3 z3
		const __m128 xy = _mm_shuffle_ps( m1, m2, _MM_SHUFFLE( 2, 1, 3, 2 ) ); // x2 y2 x3 y3
		const __m128 yz = _mm_shuffle_ps( m0, m1, _MM_SHUFFLE( 1, 0, 2, 1 ) ); // y0 z0 y1 z1
		x = _mm_shuffle_ps( m0, xy, _MM_SHUFFLE( 2, 0, 3, 0 ) );
		y = _mm_shuffle_ps( yz, xy, _MM_SHUFFLE( 3, 1, 2, 0 ) );
	}

	// ( 0x4000 - f ) | ( f << 16 ) per lane, the madd weights for fraction f.
	BK2_FOG_INLINE __m128i Weights( __m128i f )
	{
		return _mm_or_si128( _mm_sub_epi32( _mm_set1_epi32( 0x4000 ), f ), _mm_slli_epi32( f, 16 ) );
	}

	// Cells and weights from four integer x and four integer y coordinates.
	BK2_FOG_INLINE void Cells4( __m128i ix, __m128i iy, int nFogSize, SCells4 &c )
	{
		const __m128i mask = _mm_set1_epi32( nFogSize - 2 );
		const __m128i fracMask = _mm_set1_epi32( 0x3fff );
		const __m128i cx = _mm_and_si128( _mm_srai_epi32( ix, 14 ), mask );
		const __m128i cy = _mm_and_si128( _mm_srai_epi32( iy, 14 ), mask );
		// cy * nFogSize without pmulld (SSE4.1): both factors are below 65536, so the
		// product is the low and high halves of a 16 by 16 bit multiply.
		const __m128i size = _mm_set1_epi32( nFogSize );
		const __m128i rowLo = _mm_mullo_epi16( cy, size );
		const __m128i rowHi = _mm_mulhi_epu16( cy, size );
		const __m128i row = _mm_or_si128( rowLo, _mm_slli_epi32( rowHi, 16 ) );
		_mm_store_si128( reinterpret_cast<__m128i *>( c.nIndex ), _mm_add_epi32( row, cx ) );
		c.xWeights = Weights( _mm_and_si128( ix, fracMask ) );
		c.yWeights = Weights( _mm_and_si128( iy, fracMask ) );
	}

	// The 2x2 neighbourhoods of four cells as dwords of bytes ( up0, up1, down0, down1 ).
	BK2_FOG_INLINE __m128i Fetch4( const SCells4 &c, const uint8_t *pFog, int nFogSize )
	{
		const uint8_t *pDown = pFog + nFogSize;
		__m128i d = _mm_cvtsi32_si128( static_cast<int>( NDetail::Load16( pFog + c.nIndex[0] ) ) );
		d = _mm_insert_epi16( d, static_cast<int>( NDetail::Load16( pDown + c.nIndex[0] ) ), 1 );
		d = _mm_insert_epi16( d, static_cast<int>( NDetail::Load16( pFog + c.nIndex[1] ) ), 2 );
		d = _mm_insert_epi16( d, static_cast<int>( NDetail::Load16( pDown + c.nIndex[1] ) ), 3 );
		d = _mm_insert_epi16( d, static_cast<int>( NDetail::Load16( pFog + c.nIndex[2] ) ), 4 );
		d = _mm_insert_epi16( d, static_cast<int>( NDetail::Load16( pDown + c.nIndex[2] ) ), 5 );
		d = _mm_insert_epi16( d, static_cast<int>( NDetail::Load16( pFog + c.nIndex[3] ) ), 6 );
		d = _mm_insert_epi16( d, static_cast<int>( NDetail::Load16( pDown + c.nIndex[3] ) ), 7 );
		return d;
	}

	// Bilinear blend of four neighbourhoods; the four result bytes in the low dword.
	BK2_FOG_INLINE int32_t Blend4( __m128i d, __m128i xWeights, __m128i yWeights )
	{
		const __m128i zero = _mm_setzero_si128();
		// Words ( up0, up1, down0, down1 ) of positions 0, 1 and of 2, 3.
		const __m128i lo = _mm_unpacklo_epi8( d, zero );
		const __m128i hi = _mm_unpackhi_epi8( d, zero );
		// Each position's x weights twice, once for its upper and once for its lower row.
		const __m128i wLo = _mm_unpacklo_epi32( xWeights, xWeights );
		const __m128i wHi = _mm_unpackhi_epi32( xWeights, xWeights );
		const __m128i hLo = _mm_srai_epi32( _mm_madd_epi16( lo, wLo ), 14 );
		const __m128i hHi = _mm_srai_epi32( _mm_madd_epi16( hi, wHi ), 14 );
		// Words ( up, down ) per position, every value 0..255, so packs cannot saturate.
		const __m128i h = _mm_packs_epi32( hLo, hHi );
		__m128i v = _mm_srai_epi32( _mm_madd_epi16( h, yWeights ), 14 );
		v = _mm_packs_epi32( v, v );
		v = _mm_packus_epi16( v, v );
		return _mm_cvtsi128_si32( v );
	}

	BK2_FOG_INLINE void Store4( uint8_t *p, int32_t v )
	{
		std::memcpy( p, &v, sizeof( v ) );
	}

	// Four positions from twelve floats, both grids.
	BK2_FOG_INLINE void Sample4( const float *pPos,
	                             __m128 scale,
	                             int nFogSize,
	                             const uint8_t *pFogA,
	                             const uint8_t *pFogB,
	                             int32_t &nResA,
	                             int32_t &nResB )
	{
		__m128 x, y;
		LoadXY4( pPos, x, y );
		SCells4 c;
		Cells4( _mm_cvtps_epi32( _mm_mul_ps( x, scale ) ), _mm_cvtps_epi32( _mm_mul_ps( y, scale ) ), nFogSize, c );
		nResA = Blend4( Fetch4( c, pFogA, nFogSize ), c.xWeights, c.yWeights );
		if ( pFogB )
		{
			nResB = Blend4( Fetch4( c, pFogB, nFogSize ), c.xWeights, c.yWeights );
		}
	}

} // namespace NDetailSSE2

// SSE2, four positions per step. Same contract as SampleWarFogScalar.
inline void SampleWarFogSSE2( const float *pPos,
                              int nCount,
                              float fScale,
                              int nFogSize,
                              const uint8_t *pFogA,
                              uint8_t *pResA,
                              const uint8_t *pFogB,
                              uint8_t *pResB )
{
	const __m128 scale = _mm_set1_ps( NDetail::FixedScale( fScale ) );
	int32_t nResA = 0, nResB = 0;
	int k = 0;
	for ( ; k + 4 <= nCount; k += 4 )
	{
		NDetailSSE2::Sample4( pPos + 3 * k, scale, nFogSize, pFogA, pFogB, nResA, nResB );
		NDetailSSE2::Store4( pResA + k, nResA );
		if ( pFogB )
		{
			NDetailSSE2::Store4( pResB + k, nResB );
		}
	}
	if ( k < nCount )
	{
		// The last one to three positions go through the same four-wide code from a
		// zero-padded copy, so the tail is computed exactly like the body and nothing
		// past either array is touched. A zero position is cell 0, inside the grid.
		// Fixed-size copies rather than memcpy with a variable length, which is a
		// library call that cost as much as the samples on short parts.
		const int nLeft = nCount - k;
		const float *pTail = pPos + 3 * k;
		float tail[12] = {};
		std::memcpy( tail, pTail, 3 * sizeof( float ) );
		if ( nLeft > 1 )
		{
			std::memcpy( tail + 3, pTail + 3, 3 * sizeof( float ) );
		}
		if ( nLeft > 2 )
		{
			std::memcpy( tail + 6, pTail + 6, 3 * sizeof( float ) );
		}
		NDetailSSE2::Sample4( tail, scale, nFogSize, pFogA, pFogB, nResA, nResB );
		NDetail::StoreShort( pResA + k, reinterpret_cast<const uint8_t *>( &nResA ), nLeft );
		if ( pFogB )
		{
			NDetail::StoreShort( pResB + k, reinterpret_cast<const uint8_t *>( &nResB ), nLeft );
		}
	}
}

// Drop-in for the kernel table's pSampleWarFogInt: interleaved x, y 16.14
// coordinates in, one grid. Exposed so the integer stage can be tested on its own,
// over coordinates no float position produces.
inline void SampleWarFogIntSSE2( const int32_t *pCoords, const uint8_t *pFog, int nFogSize, uint8_t *pRes, int nCount )
{
	int k = 0;
	for ( ; k + 4 <= nCount; k += 4 )
	{
		const __m128i c01 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( pCoords + 2 * k ) );
		const __m128i c23 = _mm_loadu_si128( reinterpret_cast<const __m128i *>( pCoords + 2 * k + 4 ) );
		// Even dwords are x, odd are y.
		const __m128 f01 = _mm_castsi128_ps( c01 ), f23 = _mm_castsi128_ps( c23 );
		const __m128i ix = _mm_castps_si128( _mm_shuffle_ps( f01, f23, _MM_SHUFFLE( 2, 0, 2, 0 ) ) );
		const __m128i iy = _mm_castps_si128( _mm_shuffle_ps( f01, f23, _MM_SHUFFLE( 3, 1, 3, 1 ) ) );
		NDetailSSE2::SCells4 c;
		NDetailSSE2::Cells4( ix, iy, nFogSize, c );
		NDetailSSE2::Store4( pRes + k,
		                     NDetailSSE2::Blend4( NDetailSSE2::Fetch4( c, pFog, nFogSize ), c.xWeights, c.yWeights ) );
	}
	for ( ; k < nCount; ++k )
	{
		pRes[k] = NDetail::SampleInt( pCoords[2 * k], pCoords[2 * k + 1], pFog, nFogSize );
	}
}

// Drop-in for the kernel table's pSampleWarFogCoords, through the same LoadXY4 and
// conversion SampleWarFogSSE2 uses. Exposed so the coordinate stage can be compared
// on its own (the bytes alone would hide a coordinate off by one).
inline void SampleWarFogCoordsSSE2( const float *pPos, int nCount, float fScale, int32_t *pCoords )
{
	const __m128 scale = _mm_set1_ps( NDetail::FixedScale( fScale ) );
	int k = 0;
	for ( ; k + 4 <= nCount; k += 4 )
	{
		__m128 x, y;
		NDetailSSE2::LoadXY4( pPos + 3 * k, x, y );
		const __m128 ix = _mm_castsi128_ps( _mm_cvtps_epi32( _mm_mul_ps( x, scale ) ) );
		const __m128 iy = _mm_castsi128_ps( _mm_cvtps_epi32( _mm_mul_ps( y, scale ) ) );
		_mm_storeu_ps( reinterpret_cast<float *>( pCoords + 2 * k ), _mm_unpacklo_ps( ix, iy ) );
		_mm_storeu_ps( reinterpret_cast<float *>( pCoords + 2 * k + 4 ), _mm_unpackhi_ps( ix, iy ) );
	}
	const float fFixed = NDetail::FixedScale( fScale );
	for ( ; k < nCount; ++k )
	{
		pCoords[2 * k] = NDetail::RoundLikeSSE( pPos[3 * k] * fFixed );
		pCoords[2 * k + 1] = NDetail::RoundLikeSSE( pPos[3 * k + 1] * fFixed );
	}
}

#endif // BK2_FOG_SSE2

// ---------------------------------------------------------------------------------
// Uniform fog shortcut.
//
// Most of the map is either fully seen (255 around one's own units) or fully hidden,
// so a part's whole neighbourhood often holds a single fog value. When every byte of
// the block of cells a part's positions fall in, plus the +1 neighbour row and
// column the bilinear blend reads, is the same value v, every sample is exactly v:
// ( v * ( 0x4000 - f ) + v * f ) >> 14 == v at both stages. FindUniformFog proves
// that for a part (and for both grids when two are sampled) and the caller fills the
// results with v instead of sampling.
//
// The proof needs the cells of the extreme positions only: x * scale and the
// round-to-nearest conversion are both monotonic, so the cell of every position lies
// between the cells of the smallest and the largest coordinate (in either order,
// for a negative scale). It gives up, and the caller samples normally, when:
// - any x or y is NaN (min/max cannot bound it and its cell is INT_MIN's);
// - an extreme converts out of range (INT_MIN, a negative cell) or the block would
//   cross the grid edge, where "& ( nFogSize - 2 )" wraps and the cells stop being
//   contiguous; nFogSize - 2 not of the form 2^k - 1 is refused for the same reason;
// - the block is larger than N_UNIFORM_MAX_CELLS on a side (a check that long would
//   cost more than it saves) or any byte in it differs.
// So a "true" is always exact; a "false" only costs the check.

// Blocks wider or taller than this many cells are not scanned.
const int N_UNIFORM_MAX_CELLS = 16;

// Optional tally the engine can keep per frame to measure the hit rate: call Add
// after each FindUniformFog, read the counts, Reset at the frame start.
struct SUniformFogCounter
{
	uint64_t nParts = 0;
	uint64_t nPartsHit = 0;
	uint64_t nPositions = 0;
	uint64_t nPositionsHit = 0;

	void Add( bool bHit, int nCount )
	{
		++nParts;
		nPositions += static_cast<uint64_t>( nCount );
		if ( bHit )
		{
			++nPartsHit;
			nPositionsHit += static_cast<uint64_t>( nCount );
		}
	}

	void Reset()
	{
		*this = SUniformFogCounter();
	}
};

namespace NDetail
{

	// Smallest and largest x and y of the part; false if any x or y is NaN.
	inline bool PositionBounds( const float *pPos, int nCount, float &fMinX, float &fMaxX, float &fMinY, float &fMaxY )
	{
		int k = 0;
		fMinX = fMaxX = pPos[0];
		fMinY = fMaxY = pPos[1];
		bool bNaN = false;
#if BK2_FOG_SSE2
		if ( nCount >= 8 )
		{
			// Eight positions per iteration into two sets of accumulators: minps and
			// maxps have a four-cycle latency, and with one set that chain, not the
			// loads, was the limit.
			__m128 minX0 = _mm_set1_ps( fMinX ), maxX0 = minX0, minY0 = _mm_set1_ps( fMinY ), maxY0 = minY0;
			__m128 minX1 = minX0, maxX1 = minX0, minY1 = minY0, maxY1 = minY0;
			__m128 nan = _mm_setzero_ps();
			for ( ; k + 8 <= nCount; k += 8 )
			{
				__m128 x0, y0, x1, y1;
				NDetailSSE2::LoadXY4( pPos + 3 * k, x0, y0 );
				NDetailSSE2::LoadXY4( pPos + 3 * k + 12, x1, y1 );
				minX0 = _mm_min_ps( minX0, x0 );
				maxX0 = _mm_max_ps( maxX0, x0 );
				minY0 = _mm_min_ps( minY0, y0 );
				maxY0 = _mm_max_ps( maxY0, y0 );
				minX1 = _mm_min_ps( minX1, x1 );
				maxX1 = _mm_max_ps( maxX1, x1 );
				minY1 = _mm_min_ps( minY1, y1 );
				maxY1 = _mm_max_ps( maxY1, y1 );
				const __m128 nan0 = _mm_or_ps( _mm_cmpunord_ps( x0, x0 ), _mm_cmpunord_ps( y0, y0 ) );
				const __m128 nan1 = _mm_or_ps( _mm_cmpunord_ps( x1, x1 ), _mm_cmpunord_ps( y1, y1 ) );
				nan = _mm_or_ps( nan, _mm_or_ps( nan0, nan1 ) );
			}
			bNaN = _mm_movemask_ps( nan ) != 0;
			// Horizontal reduction in registers. NaN never reaches here (it is
			// reported above), so min and max are ordinary comparisons.
			__m128 minX = _mm_min_ps( minX0, minX1 ), maxX = _mm_max_ps( maxX0, maxX1 );
			__m128 minY = _mm_min_ps( minY0, minY1 ), maxY = _mm_max_ps( maxY0, maxY1 );
			minX = _mm_min_ps( minX, _mm_shuffle_ps( minX, minX, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			maxX = _mm_max_ps( maxX, _mm_shuffle_ps( maxX, maxX, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			minY = _mm_min_ps( minY, _mm_shuffle_ps( minY, minY, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			maxY = _mm_max_ps( maxY, _mm_shuffle_ps( maxY, maxY, _MM_SHUFFLE( 1, 0, 3, 2 ) ) );
			minX = _mm_min_ss( minX, _mm_shuffle_ps( minX, minX, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
			maxX = _mm_max_ss( maxX, _mm_shuffle_ps( maxX, maxX, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
			minY = _mm_min_ss( minY, _mm_shuffle_ps( minY, minY, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
			maxY = _mm_max_ss( maxY, _mm_shuffle_ps( maxY, maxY, _MM_SHUFFLE( 2, 3, 0, 1 ) ) );
			fMinX = _mm_cvtss_f32( minX );
			fMaxX = _mm_cvtss_f32( maxX );
			fMinY = _mm_cvtss_f32( minY );
			fMaxY = _mm_cvtss_f32( maxY );
		}
#endif
		for ( ; k < nCount; ++k )
		{
			const float x = pPos[3 * k], y = pPos[3 * k + 1];
			bNaN |= x != x || y != y;
			fMinX = x < fMinX ? x : fMinX;
			fMaxX = x > fMaxX ? x : fMaxX;
			fMinY = y < fMinY ? y : fMinY;
			fMaxY = y > fMaxY ? y : fMaxY;
		}
		return !bNaN;
	}

	// True if every byte of columns nX0..nX1 and rows nY0..nY1 equals the first;
	// that value goes to nValue.
	inline bool BlockUniform( const uint8_t *pFog, int nFogSize, int nX0, int nX1, int nY0, int nY1, uint8_t &nValue )
	{
		const uint8_t v = pFog[nY0 * nFogSize + nX0];
		nValue = v;
		const int nWidth = nX1 - nX0 + 1;
		// Blocks up to eight bytes wide (parts up to about five cells across) are
		// checked a row at a time with one 8-byte load, masked to the block width.
		// The load may extend past the block but never past the grid: a row whose
		// eight bytes would run beyond the last byte takes the byte loop instead.
		// A byte loop per row had a mispredicted exit per row and cost more than
		// sampling a short part outright.
		if ( nWidth <= 8 )
		{
			const uint64_t nMaskBytes = nWidth == 8 ? ~uint64_t( 0 ) : ( uint64_t( 1 ) << ( 8 * nWidth ) ) - 1;
			// Splatted through 32 bits: a 64-bit multiply is a library call (__allmul) on x86.
			const uint32_t nSplat32 = v * 0x01010101u;
			const uint64_t nSplat = ( uint64_t( nSplat32 ) << 32 ) | nSplat32;
			const size_t nGridBytes = size_t( nFogSize ) * nFogSize;
			uint64_t nDiff = 0;
			for ( int y = nY0; y <= nY1; ++y )
			{
				const size_t nRowStart = size_t( y ) * nFogSize + nX0;
				if ( nRowStart + 8 <= nGridBytes )
				{
					uint64_t nRow;
					std::memcpy( &nRow, pFog + nRowStart, sizeof( nRow ) );
					// Little-endian: the block's first byte is the low byte.
					nDiff |= ( nRow ^ nSplat ) & nMaskBytes;
				}
				else
				{
					for ( int x = 0; x < nWidth; ++x )
					{
						nDiff |= static_cast<uint64_t>( pFog[nRowStart + x] ^ v );
					}
				}
			}
			return nDiff == 0;
		}
		unsigned nDiff = 0;
		for ( int y = nY0; y <= nY1; ++y )
		{
			const uint8_t *pRow = pFog + y * nFogSize;
			for ( int x = nX0; x <= nX1; ++x )
			{
				nDiff |= static_cast<unsigned>( pRow[x] ^ v );
			}
		}
		return nDiff == 0;
	}

	// pRes[0..nCount) = v without a library call for the short parts that dominate.
	inline void Fill( uint8_t *pRes, uint8_t v, int nCount )
	{
#if BK2_FOG_SSE2
		if ( nCount >= 16 )
		{
			// Sixteen at a time, the last store overlapping the previous one.
			const __m128i fill = _mm_set1_epi8( static_cast<char>( v ) );
			for ( int k = 0; k + 16 < nCount; k += 16 )
			{
				_mm_storeu_si128( reinterpret_cast<__m128i *>( pRes + k ), fill );
			}
			_mm_storeu_si128( reinterpret_cast<__m128i *>( pRes + nCount - 16 ), fill );
			return;
		}
#endif
		for ( int k = 0; k < nCount; ++k )
		{
			pRes[k] = v;
		}
	}

} // namespace NDetail

// The per-part check, to call before a sampling kernel. Returns true when every
// sample of the part in grid A (and in grid B, if pFogB is not null) is provably one
// value, which goes to nValueA (nValueB); the results can then be filled with it.
// Same position and grid contract as the sampling kernels; works on any CPU.
inline bool FindUniformFog( const float *pPos,
                            int nCount,
                            float fScale,
                            int nFogSize,
                            const uint8_t *pFogA,
                            uint8_t &nValueA,
                            const uint8_t *pFogB,
                            uint8_t &nValueB )
{
	const int nMask = nFogSize - 2;
	if ( nCount <= 0 || nMask < 0 || ( nMask & ( nMask + 1 ) ) != 0 )
	{
		return false;
	}
	float fMinX, fMaxX, fMinY, fMaxY;
	if ( !NDetail::PositionBounds( pPos, nCount, fMinX, fMaxX, fMinY, fMaxY ) )
	{
		return false;
	}
	const float fFixed = NDetail::FixedScale( fScale );
	const int32_t nXa = NDetail::RoundLikeSSE( fMinX * fFixed ) >> 14;
	const int32_t nXb = NDetail::RoundLikeSSE( fMaxX * fFixed ) >> 14;
	const int32_t nYa = NDetail::RoundLikeSSE( fMinY * fFixed ) >> 14;
	const int32_t nYb = NDetail::RoundLikeSSE( fMaxY * fFixed ) >> 14;
	const int32_t nX0 = nXa < nXb ? nXa : nXb, nX1 = nXa < nXb ? nXb : nXa;
	const int32_t nY0 = nYa < nYb ? nYa : nYb, nY1 = nYa < nYb ? nYb : nYa;
	if ( nX0 < 0 || nY0 < 0 || nX1 > nMask || nY1 > nMask || nX1 - nX0 >= N_UNIFORM_MAX_CELLS ||
	     nY1 - nY0 >= N_UNIFORM_MAX_CELLS )
	{
		return false;
	}
	// + 1: the right and lower neighbours the blend reads. nX1 + 1 <= nFogSize - 1.
	if ( !NDetail::BlockUniform( pFogA, nFogSize, nX0, nX1 + 1, nY0, nY1 + 1, nValueA ) )
	{
		return false;
	}
	if ( pFogB && !NDetail::BlockUniform( pFogB, nFogSize, nX0, nX1 + 1, nY0, nY1 + 1, nValueB ) )
	{
		return false;
	}
	return true;
}

#if BK2_FOG_AVX2

namespace NDetailAVX2
{

	// Cells and weights of eight positions, shared by both grids.
	struct SCells8
	{
		__m256i index;    // cy * nFogSize + cx
		__m256i xWeights; // per position, words ( 0x4000 - fx, fx )
		__m256i yWeights; // per position, words ( 0x4000 - fy, fy )
	};

	// x and y of eight packed xyz triples (24 floats): six 16-byte loads, four
	// shuffles, the SSE2 LoadXY4 on both 128-bit lanes (positions 0-3 and 4-7).
	BK2_FOG_TARGET_AVX2 BK2_FOG_INLINE void LoadXY8( const float *p, __m256 &x, __m256 &y )
	{
		const __m256 m03 = _mm256_insertf128_ps( _mm256_castps128_ps256( _mm_loadu_ps( p ) ), _mm_loadu_ps( p + 12 ), 1 );
		const __m256 m14 =
				_mm256_insertf128_ps( _mm256_castps128_ps256( _mm_loadu_ps( p + 4 ) ), _mm_loadu_ps( p + 16 ), 1 );
		const __m256 m25 =
				_mm256_insertf128_ps( _mm256_castps128_ps256( _mm_loadu_ps( p + 8 ) ), _mm_loadu_ps( p + 20 ), 1 );
		const __m256 xy = _mm256_shuffle_ps( m14, m25, _MM_SHUFFLE( 2, 1, 3, 2 ) );
		const __m256 yz = _mm256_shuffle_ps( m03, m14, _MM_SHUFFLE( 1, 0, 2, 1 ) );
		x = _mm256_shuffle_ps( m03, xy, _MM_SHUFFLE( 2, 0, 3, 0 ) );
		y = _mm256_shuffle_ps( yz, xy, _MM_SHUFFLE( 3, 1, 2, 0 ) );
	}

	BK2_FOG_TARGET_AVX2 BK2_FOG_INLINE __m256i Weights( __m256i f )
	{
		return _mm256_or_si256( _mm256_sub_epi32( _mm256_set1_epi32( 0x4000 ), f ), _mm256_slli_epi32( f, 16 ) );
	}

	BK2_FOG_TARGET_AVX2 BK2_FOG_INLINE void Cells8( __m256i ix, __m256i iy, int nFogSize, SCells8 &c )
	{
		const __m256i mask = _mm256_set1_epi32( nFogSize - 2 );
		const __m256i fracMask = _mm256_set1_epi32( 0x3fff );
		const __m256i cx = _mm256_and_si256( _mm256_srai_epi32( ix, 14 ), mask );
		const __m256i cy = _mm256_and_si256( _mm256_srai_epi32( iy, 14 ), mask );
		c.index = _mm256_add_epi32( _mm256_mullo_epi32( cy, _mm256_set1_epi32( nFogSize ) ), cx );
		c.xWeights = Weights( _mm256_and_si256( ix, fracMask ) );
		c.yWeights = Weights( _mm256_and_si256( iy, fracMask ) );
	}

	// Two pinsrw-built 128-bit halves, as NDetailSSE2::Fetch4 does it.
	BK2_FOG_TARGET_AVX2 BK2_FOG_INLINE __m128i Insert4( const int32_t *pIndex, const uint8_t *pFog, int nFogSize )
	{
		const uint8_t *pDown = pFog + nFogSize;
		__m128i d = _mm_cvtsi32_si128( static_cast<int>( NDetail::Load16( pFog + pIndex[0] ) ) );
		d = _mm_insert_epi16( d, static_cast<int>( NDetail::Load16( pDown + pIndex[0] ) ), 1 );
		d = _mm_insert_epi16( d, static_cast<int>( NDetail::Load16( pFog + pIndex[1] ) ), 2 );
		d = _mm_insert_epi16( d, static_cast<int>( NDetail::Load16( pDown + pIndex[1] ) ), 3 );
		d = _mm_insert_epi16( d, static_cast<int>( NDetail::Load16( pFog + pIndex[2] ) ), 4 );
		d = _mm_insert_epi16( d, static_cast<int>( NDetail::Load16( pDown + pIndex[2] ) ), 5 );
		d = _mm_insert_epi16( d, static_cast<int>( NDetail::Load16( pFog + pIndex[3] ) ), 6 );
		d = _mm_insert_epi16( d, static_cast<int>( NDetail::Load16( pDown + pIndex[3] ) ), 7 );
		return d;
	}

	// The 2x2 neighbourhoods of eight cells as dwords of bytes ( up0, up1, down0, down1 ).
	template <EAVX2Load eLoad>
	BK2_FOG_TARGET_AVX2 BK2_FOG_INLINE __m256i Fetch8( const SCells8 &c, const uint8_t *pFog, int nFogSize )
	{
		if ( eLoad == EAVX2Load::Insert )
		{
			alignas( 32 ) int32_t nIndex[8];
			_mm256_store_si256( reinterpret_cast<__m256i *>( nIndex ), c.index );
			return _mm256_inserti128_si256(
					_mm256_castsi128_si256( Insert4( nIndex, pFog, nFogSize ) ), Insert4( nIndex + 4, pFog, nFogSize ), 1 );
		}
		if ( eLoad == EAVX2Load::GatherBlockCheck )
		{
			// All eight in one cell: one load each for the two rows, broadcast.
			const __m256i first = _mm256_broadcastd_epi32( _mm256_castsi256_si128( c.index ) );
			if ( _mm256_movemask_epi8( _mm256_cmpeq_epi32( c.index, first ) ) == -1 )
			{
				const int nIndex = _mm_cvtsi128_si32( _mm256_castsi256_si128( c.index ) );
				return _mm256_set1_epi32( static_cast<int>( NDetail::Load16( pFog + nIndex ) |
				                                            ( NDetail::Load16( pFog + nIndex + nFogSize ) << 16 ) ) );
			}
		}
		// Upper row: the dword at the cell, whose low two bytes are p[cy][cx], p[cy][cx + 1].
		// Lower row: the dword ending at p[cy + 1][cx + 1], i.e. starting two bytes before
		// p[cy + 1][cx], whose high two bytes are the pair. Reading it that way round
		// keeps both inside the grid: the upper row is at most nFogSize - 2, so its
		// dword ends before the last row does, and the lower dword starts at offset
		// nFogSize - 2 >= 0 and ends at most at the last byte of the grid.
		const __m256i up = _mm256_i32gather_epi32( reinterpret_cast<const int *>( pFog ), c.index, 1 );
		const __m256i down = _mm256_i32gather_epi32( reinterpret_cast<const int *>( pFog + nFogSize - 2 ), c.index, 1 );
		return _mm256_blend_epi16( up, down, 0xaa );
	}

	// Bilinear blend of eight neighbourhoods, NDetailSSE2::Blend4 on each 128-bit lane.
	// Returns the eight result bytes.
	BK2_FOG_TARGET_AVX2 BK2_FOG_INLINE __m128i Blend8( __m256i d, __m256i xWeights, __m256i yWeights )
	{
		const __m256i zero = _mm256_setzero_si256();
		const __m256i lo = _mm256_unpacklo_epi8( d, zero ); // positions 0, 1 | 4, 5
		const __m256i hi = _mm256_unpackhi_epi8( d, zero ); // positions 2, 3 | 6, 7
		const __m256i wLo = _mm256_unpacklo_epi32( xWeights, xWeights );
		const __m256i wHi = _mm256_unpackhi_epi32( xWeights, xWeights );
		const __m256i hLo = _mm256_srai_epi32( _mm256_madd_epi16( lo, wLo ), 14 );
		const __m256i hHi = _mm256_srai_epi32( _mm256_madd_epi16( hi, wHi ), 14 );
		const __m256i h = _mm256_packs_epi32( hLo, hHi ); // ( up, down ) words, 0..3 | 4..7
		__m256i v = _mm256_srai_epi32( _mm256_madd_epi16( h, yWeights ), 14 );
		v = _mm256_packs_epi32( v, v );
		v = _mm256_packus_epi16( v, v ); // bytes 0..3 in dword 0, 4..7 in dword 4
		v = _mm256_permutevar8x32_epi32( v, _mm256_setr_epi32( 0, 4, 0, 4, 0, 4, 0, 4 ) );
		return _mm256_castsi256_si128( v );
	}

	template <EAVX2Load eLoad>
	BK2_FOG_TARGET_AVX2 BK2_FOG_INLINE void Sample8( const float *pPos,
	                                                 __m256 scale,
	                                                 int nFogSize,
	                                                 const uint8_t *pFogA,
	                                                 const uint8_t *pFogB,
	                                                 __m128i &resA,
	                                                 __m128i &resB )
	{
		__m256 x, y;
		LoadXY8( pPos, x, y );
		SCells8 c;
		Cells8(
				_mm256_cvtps_epi32( _mm256_mul_ps( x, scale ) ), _mm256_cvtps_epi32( _mm256_mul_ps( y, scale ) ), nFogSize, c );
		resA = Blend8( Fetch8<eLoad>( c, pFogA, nFogSize ), c.xWeights, c.yWeights );
		if ( pFogB )
		{
			resB = Blend8( Fetch8<eLoad>( c, pFogB, nFogSize ), c.xWeights, c.yWeights );
		}
	}

} // namespace NDetailAVX2

// AVX2, eight positions per step, with the neighbourhood fetch chosen by eLoad.
// Same contract as SampleWarFogScalar.
template <EAVX2Load eLoad>
BK2_FOG_TARGET_AVX2 inline void SampleWarFogAVX2T( const float *pPos,
                                                   int nCount,
                                                   float fScale,
                                                   int nFogSize,
                                                   const uint8_t *pFogA,
                                                   uint8_t *pResA,
                                                   const uint8_t *pFogB,
                                                   uint8_t *pResB )
{
	const __m256 scale = _mm256_set1_ps( NDetail::FixedScale( fScale ) );
	__m128i resA = _mm_setzero_si128(), resB = _mm_setzero_si128();
	int k = 0;
	for ( ; k + 8 <= nCount; k += 8 )
	{
		NDetailAVX2::Sample8<eLoad>( pPos + 3 * k, scale, nFogSize, pFogA, pFogB, resA, resB );
		_mm_storel_epi64( reinterpret_cast<__m128i *>( pResA + k ), resA );
		if ( pFogB )
		{
			_mm_storel_epi64( reinterpret_cast<__m128i *>( pResB + k ), resB );
		}
	}
	if ( k < nCount )
	{
		// One to seven positions left: the same eight-wide code on a zero-padded copy,
		// as in SampleWarFogSSE2. A zero position is cell 0, inside the grid.
		// The copy is three masked loads (masked-off lanes are never read, so nothing
		// past the positions is touched) and the results go out in fixed-size pieces:
		// memcpy with a variable length was a library call per tail, which on short
		// parts cost as much as the samples.
		const int nLeft = nCount - k;
		const int nFloats = 3 * nLeft;
		const float *pTail = pPos + 3 * k;
		const __m256i lane = _mm256_setr_epi32( 0, 1, 2, 3, 4, 5, 6, 7 );
		alignas( 32 ) float tail[24];
		for ( int nOffset = 0; nOffset < 24; nOffset += 8 )
		{
			__m256 v = _mm256_setzero_ps();
			if ( nOffset < nFloats )
			{
				const __m256i mask = _mm256_cmpgt_epi32( _mm256_set1_epi32( nFloats - nOffset ), lane );
				v = _mm256_maskload_ps( pTail + nOffset, mask );
			}
			_mm256_store_ps( tail + nOffset, v );
		}
		NDetailAVX2::Sample8<eLoad>( tail, scale, nFogSize, pFogA, pFogB, resA, resB );
		alignas( 16 ) uint8_t bytes[16];
		_mm_store_si128( reinterpret_cast<__m128i *>( bytes ), resA );
		NDetail::StoreShort( pResA + k, bytes, nLeft );
		if ( pFogB )
		{
			_mm_store_si128( reinterpret_cast<__m128i *>( bytes ), resB );
			NDetail::StoreShort( pResB + k, bytes, nLeft );
		}
	}
}

// The recommended AVX2 kernel. Call only when IsAVX2Present().
BK2_FOG_TARGET_AVX2 inline void SampleWarFogAVX2( const float *pPos,
                                                  int nCount,
                                                  float fScale,
                                                  int nFogSize,
                                                  const uint8_t *pFogA,
                                                  uint8_t *pResA,
                                                  const uint8_t *pFogB,
                                                  uint8_t *pResB )
{
	SampleWarFogAVX2T<EAVX2Load::Gather>( pPos, nCount, fScale, nFogSize, pFogA, pResA, pFogB, pResB );
}

// Drop-in for the kernel table's pSampleWarFogInt, AVX2 with gathers. Exposed so the
// integer stage can be tested on its own, over coordinates no float position produces.
BK2_FOG_TARGET_AVX2 inline void
SampleWarFogIntAVX2( const int32_t *pCoords, const uint8_t *pFog, int nFogSize, uint8_t *pRes, int nCount )
{
	int k = 0;
	for ( ; k + 8 <= nCount; k += 8 )
	{
		const __m256i c0 = _mm256_loadu_si256( reinterpret_cast<const __m256i *>( pCoords + 2 * k ) );
		const __m256i c1 = _mm256_loadu_si256( reinterpret_cast<const __m256i *>( pCoords + 2 * k + 8 ) );
		// Even dwords are x, odd are y; the shuffle works per 128-bit lane, so lane 0
		// holds positions 0, 1, 4, 5 and lane 1 positions 2, 3, 6, 7, which the
		// permute puts back in order.
		const __m256 f0 = _mm256_castsi256_ps( c0 ), f1 = _mm256_castsi256_ps( c1 );
		const __m256i order = _mm256_setr_epi32( 0, 1, 4, 5, 2, 3, 6, 7 );
		const __m256i ix = _mm256_permutevar8x32_epi32(
				_mm256_castps_si256( _mm256_shuffle_ps( f0, f1, _MM_SHUFFLE( 2, 0, 2, 0 ) ) ), order );
		const __m256i iy = _mm256_permutevar8x32_epi32(
				_mm256_castps_si256( _mm256_shuffle_ps( f0, f1, _MM_SHUFFLE( 3, 1, 3, 1 ) ) ), order );
		NDetailAVX2::SCells8 c;
		NDetailAVX2::Cells8( ix, iy, nFogSize, c );
		_mm_storel_epi64(
				reinterpret_cast<__m128i *>( pRes + k ),
				NDetailAVX2::Blend8( NDetailAVX2::Fetch8<EAVX2Load::Gather>( c, pFog, nFogSize ), c.xWeights, c.yWeights ) );
	}
	for ( ; k < nCount; ++k )
	{
		pRes[k] = NDetail::SampleInt( pCoords[2 * k], pCoords[2 * k + 1], pFog, nFogSize );
	}
}

// Drop-in for the kernel table's pSampleWarFogCoords through LoadXY8, for the same
// reason as SampleWarFogCoordsSSE2.
BK2_FOG_TARGET_AVX2 inline void SampleWarFogCoordsAVX2( const float *pPos, int nCount, float fScale, int32_t *pCoords )
{
	const __m256 scale = _mm256_set1_ps( NDetail::FixedScale( fScale ) );
	int k = 0;
	for ( ; k + 8 <= nCount; k += 8 )
	{
		__m256 x, y;
		NDetailAVX2::LoadXY8( pPos + 3 * k, x, y );
		const __m256 ix = _mm256_castsi256_ps( _mm256_cvtps_epi32( _mm256_mul_ps( x, scale ) ) );
		const __m256 iy = _mm256_castsi256_ps( _mm256_cvtps_epi32( _mm256_mul_ps( y, scale ) ) );
		// unpack works per lane: lo is positions 0, 1 | 4, 5 and hi 2, 3 | 6, 7.
		const __m256 lo = _mm256_unpacklo_ps( ix, iy );
		const __m256 hi = _mm256_unpackhi_ps( ix, iy );
		_mm256_storeu_ps( reinterpret_cast<float *>( pCoords + 2 * k ), _mm256_permute2f128_ps( lo, hi, 0x20 ) );
		_mm256_storeu_ps( reinterpret_cast<float *>( pCoords + 2 * k + 8 ), _mm256_permute2f128_ps( lo, hi, 0x31 ) );
	}
	const float fFixed = NDetail::FixedScale( fScale );
	for ( ; k < nCount; ++k )
	{
		pCoords[2 * k] = NDetail::RoundLikeSSE( pPos[3 * k] * fFixed );
		pCoords[2 * k + 1] = NDetail::RoundLikeSSE( pPos[3 * k + 1] * fFixed );
	}
}

// FindUniformFog, then SampleWarFogAVX2 only when it fails: the whole shortcut in one
// call, as the benchmark measures it. Call only when IsAVX2Present().
BK2_FOG_TARGET_AVX2 inline void SampleWarFogAVX2Uniform( const float *pPos,
                                                         int nCount,
                                                         float fScale,
                                                         int nFogSize,
                                                         const uint8_t *pFogA,
                                                         uint8_t *pResA,
                                                         const uint8_t *pFogB,
                                                         uint8_t *pResB )
{
	uint8_t nValueA = 0, nValueB = 0;
	if ( FindUniformFog( pPos, nCount, fScale, nFogSize, pFogA, nValueA, pFogB, nValueB ) )
	{
		NDetail::Fill( pResA, nValueA, nCount );
		if ( pFogB )
		{
			NDetail::Fill( pResB, nValueB, nCount );
		}
		return;
	}
	SampleWarFogAVX2T<EAVX2Load::Gather>( pPos, nCount, fScale, nFogSize, pFogA, pResA, pFogB, pResB );
}

#endif // BK2_FOG_AVX2

} // namespace NWarFog
