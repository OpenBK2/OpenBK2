#pragma once
// Every rigid normal transform kernel that was measured against the engine's SSE2
// MMXTransformVector (GSSEtransform.h), shared by the test that holds them all to it
// bit for bit (test/NormalTransform.cpp) and the benchmark that compares them
// (benchmark/NormalTransform.cpp). The candidate for the engine is
// NNormals::TransformNormalsRigid in GNormalTransform.h (here SSE4); the rest are
// kept so the comparison can be rerun on another compiler or CPU.
//
// Like GNormalTransform.h this header knows no engine types (normals are uint32_t
// with bytes z, y, x, w, read at a byte stride; matrices are sixteen row-major
// floats), so it compiles on its own, which is how MSVC x86/x64 was checked.
//
// The AVX2 variants are compiled with a per-function target attribute on GCC and
// Clang, which only test and benchmark code may do: callers must check
// NGScene::IsAVX2Present() first. "avx2" only, not "fma": with FMA available GCC
// would be free to fuse a multiply and an add, and the result would no longer be
// bit-exact. MSVC accepts the intrinsics without any /arch flag.

#include "3Dmotor/GNormalTransform.h"

#include <cstddef>
#include <cstdint>

#if BK2_NORMALS_SSE2
#include <immintrin.h>
#endif

#if defined( __GNUC__ ) || defined( __clang__ )
#define BK2_NORMALS_TARGET_AVX2 __attribute__( ( target( "avx2" ) ) )
#else
#define BK2_NORMALS_TARGET_AVX2
#endif

namespace NNormalVariants
{

// One call's arguments, the same for every variant.
struct SNormalCall
{
	const void *pSrc;
	int nStride;
	uint32_t *pDst;
	int nCount;
	const float *pMatrix;
};

inline void Scalar( const SNormalCall &c )
{
	NNormals::TransformNormalsRigidScalar( c.pSrc, c.nStride, c.pDst, c.nCount, c.pMatrix );
}

#if BK2_NORMALS_SSE2

using NNormals::NDetail::Load32;

// A frozen copy of the SSE2 MMXTransformVector( SCompactVector &, const
// SCompactVector &, const SHMatrix & ) from GSSEtransform.h, as it stood when these
// kernels were written. The only edits are the types: the normal is its uint32_t and
// the matrix sixteen floats. Every kernel here must reproduce its bytes.
inline uint32_t ReferenceTransform( uint32_t src, const float *m )
{
	const __m128i zero = _mm_setzero_si128();
	const __m128i packedSource = _mm_cvtsi32_si128( static_cast<int>( src ) );
	const __m128i sourceWords = _mm_unpacklo_epi8( packedSource, zero );
	__m128i sourceDwords = _mm_unpacklo_epi16( sourceWords, zero );

	sourceDwords = _mm_shuffle_epi32( sourceDwords, _MM_SHUFFLE( 3, 0, 1, 2 ) );
	__m128 source = _mm_cvtepi32_ps( sourceDwords );
	source = _mm_div_ps( _mm_sub_ps( source, _mm_set1_ps( 128.0f ) ), _mm_set1_ps( 127.0f ) );
	source = _mm_and_ps( source, _mm_castsi128_ps( _mm_set_epi32( 0, -1, -1, -1 ) ) );

	__m128 column0 = _mm_loadu_ps( m + 0 );
	__m128 column1 = _mm_loadu_ps( m + 4 );
	__m128 column2 = _mm_loadu_ps( m + 8 );
	__m128 column3 = _mm_loadu_ps( m + 12 );
	_MM_TRANSPOSE4_PS( column0, column1, column2, column3 );

	__m128 transformed = _mm_mul_ps( column0, _mm_shuffle_ps( source, source, _MM_SHUFFLE( 0, 0, 0, 0 ) ) );
	transformed =
			_mm_add_ps( transformed, _mm_mul_ps( column1, _mm_shuffle_ps( source, source, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ) );
	transformed =
			_mm_add_ps( transformed, _mm_mul_ps( column2, _mm_shuffle_ps( source, source, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ) );
	transformed =
			_mm_add_ps( transformed, _mm_mul_ps( column3, _mm_shuffle_ps( source, source, _MM_SHUFFLE( 3, 3, 3, 3 ) ) ) );

	const __m128 squared = _mm_mul_ps( transformed, transformed );
	__m128 lengthSquared = _mm_add_ss( squared, _mm_shuffle_ps( squared, squared, _MM_SHUFFLE( 1, 1, 1, 1 ) ) );
	lengthSquared = _mm_add_ss( lengthSquared, _mm_shuffle_ps( squared, squared, _MM_SHUFFLE( 2, 2, 2, 2 ) ) );
	__m128 inverseLength = _mm_div_ss( _mm_set_ss( 1.0f ), _mm_sqrt_ss( lengthSquared ) );
	inverseLength = _mm_shuffle_ps( inverseLength, inverseLength, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	const __m128 normal = _mm_mul_ps( transformed, inverseLength );

	__m128i resultDwords = _mm_cvttps_epi32( _mm_mul_ps( normal, _mm_set1_ps( 127.0f ) ) );
	resultDwords = _mm_add_epi32( resultDwords, _mm_set1_epi32( 128 ) );
	resultDwords = _mm_shuffle_epi32( resultDwords, _MM_SHUFFLE( 3, 0, 1, 2 ) );
	const __m128i resultWords = _mm_packs_epi32( resultDwords, resultDwords );
	const __m128i resultBytes = _mm_packus_epi16( resultWords, resultWords );

	return ( static_cast<uint32_t>( _mm_cvtsi128_si32( resultBytes ) ) & 0x00ffffffu ) | ( src & 0xff000000u );
}

// What the engine does today: one MMXTransformVector call per normal.
inline void Ref( const SNormalCall &c )
{
	const uint8_t *p = static_cast<const uint8_t *>( c.pSrc );
	for ( int k = 0; k < c.nCount; ++k, p += c.nStride )
	{
		c.pDst[k] = ReferenceTransform( Load32( p ), c.pMatrix );
	}
}

// ( float( b ) - 128 ) / 127 for every byte, computed with the same divps, so a
// lookup gives the decode's bits without the division.
inline const float *DecodeTable()
{
	struct STable
	{
		alignas( 64 ) float f[256];

		STable()
		{
			for ( int b = 0; b < 256; b += 4 )
			{
				_mm_store_ps( f + b, NNormals::NDetail::Decode4( _mm_setr_epi32( b, b + 1, b + 2, b + 3 ) ) );
			}
		}
	};

	static const STable table;
	return table.f;
}

// One normal per step, as the reference, but with the transpose done once per call
// and the m[i][3] * 0 column folded into a constant.
inline void Batch1( const SNormalCall &c )
{
	__m128 c0 = _mm_loadu_ps( c.pMatrix + 0 );
	__m128 c1 = _mm_loadu_ps( c.pMatrix + 4 );
	__m128 c2 = _mm_loadu_ps( c.pMatrix + 8 );
	__m128 c3 = _mm_loadu_ps( c.pMatrix + 12 );
	_MM_TRANSPOSE4_PS( c0, c1, c2, c3 );
	const __m128 k = _mm_mul_ps( c3, _mm_setzero_ps() );
	const uint8_t *p = static_cast<const uint8_t *>( c.pSrc );
	const __m128i zero = _mm_setzero_si128();
	for ( int n = 0; n < c.nCount; ++n, p += c.nStride )
	{
		const uint32_t d = Load32( p );
		__m128i dw = _mm_unpacklo_epi16( _mm_unpacklo_epi8( _mm_cvtsi32_si128( static_cast<int>( d ) ), zero ), zero );
		dw = _mm_shuffle_epi32( dw, _MM_SHUFFLE( 3, 0, 1, 2 ) );
		const __m128 s = NNormals::NDetail::Decode4( dw );
		__m128 t = _mm_mul_ps( c0, _mm_shuffle_ps( s, s, _MM_SHUFFLE( 0, 0, 0, 0 ) ) );
		t = _mm_add_ps( t, _mm_mul_ps( c1, _mm_shuffle_ps( s, s, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ) );
		t = _mm_add_ps( t, _mm_mul_ps( c2, _mm_shuffle_ps( s, s, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ) );
		t = _mm_add_ps( t, k );
		const __m128 sq = _mm_mul_ps( t, t );
		__m128 len2 = _mm_add_ss( sq, _mm_shuffle_ps( sq, sq, _MM_SHUFFLE( 1, 1, 1, 1 ) ) );
		len2 = _mm_add_ss( len2, _mm_shuffle_ps( sq, sq, _MM_SHUFFLE( 2, 2, 2, 2 ) ) );
		__m128 inv = _mm_div_ss( _mm_set_ss( 1.0f ), _mm_sqrt_ss( len2 ) );
		inv = _mm_shuffle_ps( inv, inv, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		__m128i r = _mm_cvttps_epi32( _mm_mul_ps( _mm_mul_ps( t, inv ), _mm_set1_ps( 127.0f ) ) );
		r = _mm_add_epi32( r, _mm_set1_epi32( 128 ) );
		r = _mm_shuffle_epi32( r, _MM_SHUFFLE( 3, 0, 1, 2 ) );
		r = _mm_packus_epi16( _mm_packs_epi32( r, r ), zero );
		c.pDst[n] = ( static_cast<uint32_t>( _mm_cvtsi128_si32( r ) ) & 0x00ffffffu ) | ( d & 0xff000000u );
	}
}

// The shipped kernel: four normals per step, decode by divps.
inline void SSE4( const SNormalCall &c )
{
	NNormals::TransformNormalsRigid( c.pSrc, c.nStride, c.pDst, c.nCount, c.pMatrix );
}

// Four normals per step, decode through the table: twelve scalar loads per step
// instead of three divps, which leaves the divider to the sqrt and 1/len.
inline void SSE4Lut( const SNormalCall &c )
{
	using namespace NNormals::NDetail;
	if ( c.nCount <= 0 )
	{
		return;
	}
	const float *lut = DecodeTable();
	const SBroadcastMatrix M = LoadBroadcastMatrix( c.pMatrix );
	const uint8_t *p = static_cast<const uint8_t *>( c.pSrc );
	const ptrdiff_t s = c.nStride;
	for ( int k = 0; k < c.nCount; k += 4 )
	{
		// The tail step clamps its indices to the last normal rather than overlapping.
		const int n = c.nCount - k;
		const uint32_t d0 = Load32( p + k * s );
		const uint32_t d1 = Load32( p + ( k + ( n > 1 ? 1 : 0 ) ) * s );
		const uint32_t d2 = Load32( p + ( k + ( n > 2 ? 2 : n - 1 ) ) * s );
		const uint32_t d3 = Load32( p + ( k + ( n > 3 ? 3 : n - 1 ) ) * s );
		const __m128 sz = _mm_setr_ps( lut[d0 & 0xff], lut[d1 & 0xff], lut[d2 & 0xff], lut[d3 & 0xff] );
		const __m128 sy = _mm_setr_ps(
				lut[( d0 >> 8 ) & 0xff], lut[( d1 >> 8 ) & 0xff], lut[( d2 >> 8 ) & 0xff], lut[( d3 >> 8 ) & 0xff] );
		const __m128 sx = _mm_setr_ps(
				lut[( d0 >> 16 ) & 0xff], lut[( d1 >> 16 ) & 0xff], lut[( d2 >> 16 ) & 0xff], lut[( d3 >> 16 ) & 0xff] );
		const __m128i src = _mm_setr_epi32(
				static_cast<int>( d0 ), static_cast<int>( d1 ), static_cast<int>( d2 ), static_cast<int>( d3 ) );
		const __m128i r = Finish4( M, sx, sy, sz, src );
		if ( n >= 4 )
		{
			_mm_storeu_si128( reinterpret_cast<__m128i *>( c.pDst + k ), r );
		}
		else
		{
			alignas( 16 ) uint32_t tmp[4];
			_mm_store_si128( reinterpret_cast<__m128i *>( tmp ), r );
			for ( int j = 0; j < n; ++j )
			{
				c.pDst[k + j] = tmp[j];
			}
		}
	}
}

// ( b - 128 ) / 127 without a division: ( b - 128 ) * ( 1 / 127 ) in double, then
// rounded to float. The double product is within about 2^-52 relative of the true
// quotient, and a quotient a / 127 ( a != 0 ) is never closer than about 2^-32
// relative to a float rounding midpoint, so the one rounding to float lands where
// the correctly rounded divps does. The exhaustive test checks it for all 256 bytes.
BK2_NORMALS_INLINE __m128 Decode4Double( const __m128i &bytes )
{
	const __m128i a = _mm_sub_epi32( bytes, _mm_set1_epi32( 128 ) );
	const __m128d r = _mm_set1_pd( 1.0 / 127.0 );
	const __m128d lo = _mm_mul_pd( _mm_cvtepi32_pd( a ), r );
	const __m128d hi = _mm_mul_pd( _mm_cvtepi32_pd( _mm_unpackhi_epi64( a, a ) ), r );
	return _mm_movelh_ps( _mm_cvtpd_ps( lo ), _mm_cvtpd_ps( hi ) );
}

// SSE4 with the decode chosen per component: nDoubleMask bit 0/1/2 decodes x/y/z
// through Decode4Double instead of divps, moving work off the divider, which the
// sqrt and 1/len already keep busy.
template <int nDoubleMask>
BK2_NORMALS_INLINE void SSE4DecodeMix( const SNormalCall &c )
{
	using namespace NNormals::NDetail;
	if ( c.nCount < 4 )
	{
		NNormals::TransformNormalsRigid( c.pSrc, c.nStride, c.pDst, c.nCount, c.pMatrix );
		return;
	}
	const SBroadcastMatrix M = LoadBroadcastMatrix( c.pMatrix );
	const uint8_t *p = static_cast<const uint8_t *>( c.pSrc );
	const ptrdiff_t s = c.nStride;
	const __m128i byteMask = _mm_set1_epi32( 0xff );
	for ( int k = 0;; k += 4 )
	{
		// Overlapping last step, as TransformNormalsRigid does.
		if ( k + 4 > c.nCount )
		{
			if ( k == c.nCount )
			{
				break;
			}
			k = c.nCount - 4;
		}
		const uint8_t *q = p + k * s;
		const __m128i src = Gather4( q, q + s, q + 2 * s, q + 3 * s );
		const __m128i iz = _mm_and_si128( src, byteMask );
		const __m128i iy = _mm_and_si128( _mm_srli_epi32( src, 8 ), byteMask );
		const __m128i ix = _mm_and_si128( _mm_srli_epi32( src, 16 ), byteMask );
		const __m128 sx = ( nDoubleMask & 1 ) ? Decode4Double( ix ) : Decode4( ix );
		const __m128 sy = ( nDoubleMask & 2 ) ? Decode4Double( iy ) : Decode4( iy );
		const __m128 sz = ( nDoubleMask & 4 ) ? Decode4Double( iz ) : Decode4( iz );
		_mm_storeu_si128( reinterpret_cast<__m128i *>( c.pDst + k ), Finish4( M, sx, sy, sz, src ) );
		if ( k + 4 == c.nCount )
		{
			break;
		}
	}
}

inline void SSE4Dbl( const SNormalCall &c )
{
	SSE4DecodeMix<7>( c );
}

inline void SSE4Dbl1( const SNormalCall &c )
{
	SSE4DecodeMix<1>( c );
}

inline void SSE4Dbl2( const SNormalCall &c )
{
	SSE4DecodeMix<3>( c );
}

// Eight-wide AVX2. Everything after the load is SSE4's sequence on ymm registers;
// the packs and byte shuffles work within 128-bit lanes, so each half is exactly
// four normals done the SSE2 way. EGather selects how the eight normals and their
// decoded components are fetched.
enum class EAVX2Load
{
	GatherDiv,  // vpgatherdd for the normals, divps decode
	InsertDiv,  // eight movd/punpck loads, divps decode
	InsertLut,  // eight loads, decode by three vgatherdps from the table
	InsertMix,  // eight loads, x from the table, y and z by divps
	InsertDbl,  // eight loads, all three decoded through double (Decode4Double)
	InsertDbl1, // eight loads, x through double, y and z by divps
};

namespace NDetailAVX2
{

	// SBroadcastRow / SBroadcastMatrix of GNormalTransform.h at eight lanes, named
	// rows for the same reason (GCC left an indexed row loop rolled).
	struct SBroadcastRow8
	{
		__m256 a, b, c, k;
	};

	struct SBroadcastMatrix8
	{
		SBroadcastRow8 x, y, z;
	};

	BK2_NORMALS_TARGET_AVX2 BK2_NORMALS_INLINE SBroadcastRow8 LoadRow8( const float *pRow )
	{
		SBroadcastRow8 r;
		r.a = _mm256_set1_ps( pRow[0] );
		r.b = _mm256_set1_ps( pRow[1] );
		r.c = _mm256_set1_ps( pRow[2] );
		r.k = _mm256_set1_ps( pRow[3] * 0.0f );
		return r;
	}

	BK2_NORMALS_TARGET_AVX2 BK2_NORMALS_INLINE SBroadcastMatrix8 LoadMatrix8( const float *pMatrix )
	{
		SBroadcastMatrix8 r;
		r.x = LoadRow8( pMatrix );
		r.y = LoadRow8( pMatrix + 4 );
		r.z = LoadRow8( pMatrix + 8 );
		return r;
	}

	BK2_NORMALS_TARGET_AVX2 BK2_NORMALS_INLINE __m256 TransformRow8( const SBroadcastRow8 &r,
	                                                                 const __m256 &sx,
	                                                                 const __m256 &sy,
	                                                                 const __m256 &sz )
	{
		const __m256 xy = _mm256_add_ps( _mm256_mul_ps( r.a, sx ), _mm256_mul_ps( r.b, sy ) );
		return _mm256_add_ps( _mm256_add_ps( xy, _mm256_mul_ps( r.c, sz ) ), r.k );
	}

	BK2_NORMALS_TARGET_AVX2 BK2_NORMALS_INLINE __m256 Decode8( const __m256i &bytes )
	{
		return _mm256_div_ps( _mm256_sub_ps( _mm256_cvtepi32_ps( bytes ), _mm256_set1_ps( 128.0f ) ),
		                      _mm256_set1_ps( 127.0f ) );
	}

	// Decode4Double at eight lanes: two four-double products.
	BK2_NORMALS_TARGET_AVX2 BK2_NORMALS_INLINE __m256 Decode8Double( const __m256i &bytes )
	{
		const __m256i a = _mm256_sub_epi32( bytes, _mm256_set1_epi32( 128 ) );
		const __m256d r = _mm256_set1_pd( 1.0 / 127.0 );
		const __m128 lo = _mm256_cvtpd_ps( _mm256_mul_pd( _mm256_cvtepi32_pd( _mm256_castsi256_si128( a ) ), r ) );
		const __m128 hi = _mm256_cvtpd_ps( _mm256_mul_pd( _mm256_cvtepi32_pd( _mm256_extracti128_si256( a, 1 ) ), r ) );
		return _mm256_insertf128_ps( _mm256_castps128_ps256( lo ), hi, 1 );
	}

	BK2_NORMALS_TARGET_AVX2 BK2_NORMALS_INLINE __m256i Load8( const uint8_t *p,
	                                                          ptrdiff_t s,
	                                                          const __m256i &offsets,
	                                                          bool bGather )
	{
		if ( bGather )
		{
			return _mm256_i32gather_epi32( reinterpret_cast<const int *>( p ), offsets, 1 );
		}
		const __m128i lo = NNormals::NDetail::Gather4( p, p + s, p + 2 * s, p + 3 * s );
		const __m128i hi = NNormals::NDetail::Gather4( p + 4 * s, p + 5 * s, p + 6 * s, p + 7 * s );
		return _mm256_inserti128_si256( _mm256_castsi128_si256( lo ), hi, 1 );
	}

	BK2_NORMALS_TARGET_AVX2 BK2_NORMALS_INLINE __m256i Transform8( const SBroadcastMatrix8 &M,
	                                                               const __m256i &src,
	                                                               const float *lut,
	                                                               EAVX2Load eLoad )
	{
		const __m256i byteMask = _mm256_set1_epi32( 0xff );
		const __m256i iz = _mm256_and_si256( src, byteMask );
		const __m256i iy = _mm256_and_si256( _mm256_srli_epi32( src, 8 ), byteMask );
		const __m256i ix = _mm256_and_si256( _mm256_srli_epi32( src, 16 ), byteMask );
		__m256 sx, sy, sz;
		if ( eLoad == EAVX2Load::InsertLut )
		{
			sx = _mm256_i32gather_ps( lut, ix, 4 );
			sy = _mm256_i32gather_ps( lut, iy, 4 );
			sz = _mm256_i32gather_ps( lut, iz, 4 );
		}
		else if ( eLoad == EAVX2Load::InsertMix )
		{
			sx = _mm256_i32gather_ps( lut, ix, 4 );
			sy = Decode8( iy );
			sz = Decode8( iz );
		}
		else if ( eLoad == EAVX2Load::InsertDbl )
		{
			sx = Decode8Double( ix );
			sy = Decode8Double( iy );
			sz = Decode8Double( iz );
		}
		else if ( eLoad == EAVX2Load::InsertDbl1 )
		{
			sx = Decode8Double( ix );
			sy = Decode8( iy );
			sz = Decode8( iz );
		}
		else
		{
			sx = Decode8( ix );
			sy = Decode8( iy );
			sz = Decode8( iz );
		}
		const __m256 tx = TransformRow8( M.x, sx, sy, sz );
		const __m256 ty = TransformRow8( M.y, sx, sy, sz );
		const __m256 tz = TransformRow8( M.z, sx, sy, sz );
		const __m256 len2 =
				_mm256_add_ps( _mm256_add_ps( _mm256_mul_ps( tx, tx ), _mm256_mul_ps( ty, ty ) ), _mm256_mul_ps( tz, tz ) );
		const __m256 inv = _mm256_div_ps( _mm256_set1_ps( 1.0f ), _mm256_sqrt_ps( len2 ) );
		const __m256 scale = _mm256_set1_ps( 127.0f );
		const __m256i bias = _mm256_set1_epi32( 128 );
		const __m256i x = _mm256_add_epi32( _mm256_cvttps_epi32( _mm256_mul_ps( _mm256_mul_ps( tx, inv ), scale ) ), bias );
		const __m256i y = _mm256_add_epi32( _mm256_cvttps_epi32( _mm256_mul_ps( _mm256_mul_ps( ty, inv ), scale ) ), bias );
		const __m256i z = _mm256_add_epi32( _mm256_cvttps_epi32( _mm256_mul_ps( _mm256_mul_ps( tz, inv ), scale ) ), bias );
		const __m256i zero = _mm256_setzero_si256();
		const __m256i bytes = _mm256_packus_epi16( _mm256_packs_epi32( z, y ), _mm256_packs_epi32( x, zero ) );
		const __m256i zy = _mm256_unpacklo_epi8( bytes, _mm256_srli_si256( bytes, 4 ) );
		const __m256i x0 = _mm256_unpacklo_epi8( _mm256_srli_si256( bytes, 8 ), zero );
		const __m256i w = _mm256_and_si256( src, _mm256_set1_epi32( static_cast<int>( 0xff000000u ) ) );
		return _mm256_or_si256( _mm256_unpacklo_epi16( zy, x0 ), w );
	}

	// Eight per step and one overlapping step at the end; four to seven normals
	// take two overlapping four-wide steps (VEX encoded here, since the SSE2 helpers
	// inline into this function), fewer than four go to the SSE2 kernel's small path.
	BK2_NORMALS_TARGET_AVX2 BK2_NORMALS_INLINE void Run( const SNormalCall &c, EAVX2Load eLoad )
	{
		if ( c.nCount <= 0 )
		{
			return;
		}
		const uint8_t *p = static_cast<const uint8_t *>( c.pSrc );
		const ptrdiff_t s = c.nStride;
		if ( c.nCount < 8 )
		{
			using namespace NNormals::NDetail;
			if ( c.nCount < 4 )
			{
				NNormals::TransformNormalsRigid( c.pSrc, c.nStride, c.pDst, c.nCount, c.pMatrix );
				return;
			}
			const SBroadcastMatrix M = LoadBroadcastMatrix( c.pMatrix );
			const uint8_t *q = p + ( c.nCount - 4 ) * s;
			const __m128i r0 = Transform4( M, Gather4( p, p + s, p + 2 * s, p + 3 * s ) );
			const __m128i r1 = Transform4( M, Gather4( q, q + s, q + 2 * s, q + 3 * s ) );
			_mm_storeu_si128( reinterpret_cast<__m128i *>( c.pDst ), r0 );
			_mm_storeu_si128( reinterpret_cast<__m128i *>( c.pDst + c.nCount - 4 ), r1 );
			return;
		}
		const float *lut = eLoad == EAVX2Load::InsertLut || eLoad == EAVX2Load::InsertMix ? DecodeTable() : nullptr;
		const SBroadcastMatrix8 M = LoadMatrix8( c.pMatrix );
		const bool bGather = eLoad == EAVX2Load::GatherDiv;
		const __m256i offsets =
				_mm256_mullo_epi32( _mm256_setr_epi32( 0, 1, 2, 3, 4, 5, 6, 7 ), _mm256_set1_epi32( static_cast<int>( s ) ) );
		int k = 0;
		for ( ; k + 8 <= c.nCount; k += 8 )
		{
			_mm256_storeu_si256( reinterpret_cast<__m256i *>( c.pDst + k ),
			                     Transform8( M, Load8( p + k * s, s, offsets, bGather ), lut, eLoad ) );
		}
		if ( k < c.nCount )
		{
			k = c.nCount - 8;
			_mm256_storeu_si256( reinterpret_cast<__m256i *>( c.pDst + k ),
			                     Transform8( M, Load8( p + k * s, s, offsets, bGather ), lut, eLoad ) );
		}
	}

} // namespace NDetailAVX2

BK2_NORMALS_TARGET_AVX2 inline void AVX2GatherDiv( const SNormalCall &c )
{
	NDetailAVX2::Run( c, EAVX2Load::GatherDiv );
}

BK2_NORMALS_TARGET_AVX2 inline void AVX2InsertDiv( const SNormalCall &c )
{
	NDetailAVX2::Run( c, EAVX2Load::InsertDiv );
}

BK2_NORMALS_TARGET_AVX2 inline void AVX2InsertLut( const SNormalCall &c )
{
	NDetailAVX2::Run( c, EAVX2Load::InsertLut );
}

BK2_NORMALS_TARGET_AVX2 inline void AVX2InsertMix( const SNormalCall &c )
{
	NDetailAVX2::Run( c, EAVX2Load::InsertMix );
}

BK2_NORMALS_TARGET_AVX2 inline void AVX2InsertDbl( const SNormalCall &c )
{
	NDetailAVX2::Run( c, EAVX2Load::InsertDbl );
}

BK2_NORMALS_TARGET_AVX2 inline void AVX2InsertDbl1( const SNormalCall &c )
{
	NDetailAVX2::Run( c, EAVX2Load::InsertDbl1 );
}

#endif

// Every variant with its name, for the test and the benchmark.
struct SNormalVariant
{
	const char *pszName;
	void ( *pFunc )( const SNormalCall & );
	bool bNeedsAVX2;
};

inline const SNormalVariant *GetNormalVariants( int *pnCount )
{
	static const SNormalVariant variants[] = {
#if BK2_NORMALS_SSE2
		{ "Ref", &Ref, false },
		{ "Batch1", &Batch1, false },
		{ "SSE4", &SSE4, false },
		{ "SSE4Lut", &SSE4Lut, false },
		{ "SSE4Dbl", &SSE4Dbl, false },
		{ "SSE4Dbl1", &SSE4Dbl1, false },
		{ "SSE4Dbl2", &SSE4Dbl2, false },
		{ "AVX2GatherDiv", &AVX2GatherDiv, true },
		{ "AVX2InsertDiv", &AVX2InsertDiv, true },
		{ "AVX2InsertLut", &AVX2InsertLut, true },
		{ "AVX2InsertMix", &AVX2InsertMix, true },
		{ "AVX2InsertDbl", &AVX2InsertDbl, true },
		{ "AVX2InsertDbl1", &AVX2InsertDbl1, true },
#endif
		{ "Scalar", &Scalar, false },
	};
	*pnCount = static_cast<int>( sizeof( variants ) / sizeof( variants[0] ) );
	return variants;
}

} // namespace NNormalVariants
