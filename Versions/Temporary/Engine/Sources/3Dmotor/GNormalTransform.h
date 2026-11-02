#pragma once
// Normal transform for rigid parts: every normal of the part goes through the same
// matrix. This is what SimpleTransform and the rigid case of SingleSkinTransform in
// GCombiner.cpp do one normal at a time through the SSE2 MMXTransformVector
// (GSSEtransform.h), which on a replay with dozens of tanks is about a sixth of the
// main thread. A census of the shipped models found 98.8% of render parts rigid.
//
// The result is BIT-EXACT against that MMXTransformVector for every input: every
// lane of every normal goes through the same IEEE operations in the same order,
//
//   s      = ( float( byte ) - 128 ) / 127                    (divps, as before)
//   t[i]   = ( ( m[i][0] sx + m[i][1] sy ) + m[i][2] sz ) + m[i][3] * 0
//   len2   = ( tx tx + ty ty ) + tz tz
//   inv    = 1 / sqrt( len2 )                                  (sqrtps, divps)
//   out[i] = clamp( int( ( t[i] inv ) 127 ) + 128, 0, 255 )   (cvtt, packs, packus)
//   out.w  = src.w
//
// only four normals at a time instead of one, with the matrix broadcast once per call
// instead of transposed per normal. sqrtps and divps are correctly rounded per lane,
// so the packed forms give the bits the _ss forms gave. The m[i][3] * 0 term is kept
// (a precomputed signed zero) so that even a matrix with an infinite or NaN
// translation produces the bytes it produced before. Degenerate normals (bytes
// 128, 128, 128 give a zero vector, 1/sqrt( 0 ) is inf, inf * 0 is NaN) come out as
// the same bytes too: cvttps turns NaN into 0x80000000 in both, and the saturating
// packs make that 0.
//
// TransformNormalBlend2/3 do the same for the blended vertices of non-rigid parts:
// bit-exact replacements for the glm MMXTransformVector2/3, one normal at a time,
// with glm's own order of operations (which differs from MMXTransformVector's).
//
// test/NormalTransform.cpp checks this against a frozen copy of MMXTransformVector
// and against the live one, all 2^24 xyz byte patterns with several matrices plus
// random parts; test/NormalTransformVariants.h keeps the other candidates measured
// (one-wide with the transpose hoisted, table decode, AVX2 eight-wide) and
// benchmark/NormalTransform.cpp compares them.
//
// Exactness assumes the compiler does not fuse multiplies and adds. That holds for
// the default x86 and x64 builds (no FMA in the instruction set); a build with
// -mfma and GCC's default -ffp-contract=fast could fuse inside either function, so it
// would have to be rechecked there, as would any other floating point code compared
// bit for bit.
//
// The header knows no engine types: a normal is the four bytes z, y, x, w of
// NGfx::SCompactVector, read at a byte stride so that the normal field inside an
// array of SUVInfo can be passed directly, and the matrix is sixteen row-major
// floats (SHMatrix). No alignment is needed for either.
//
// SSE2 only: the x86-64 baseline, and what MSVC x86 targets by default. Without SSE2
// at compile time a scalar version is used; it does the same operations, so it is
// exact wherever float arithmetic is IEEE single precision, but on x87 (excess
// precision) it is not.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined( __SSE2__ ) || defined( _M_X64 ) || ( defined( _M_IX86_FP ) && _M_IX86_FP >= 2 )
#define BK2_NORMALS_SSE2 1
#include <emmintrin.h>
#else
#define BK2_NORMALS_SSE2 0
#endif

// Same reason as BK2_SKIN_INLINE in GSkinning.h: GCC at -O2 keeps small helpers that
// are called from two places out of line, which costs a call per block.
#if defined( _MSC_VER )
#define BK2_NORMALS_INLINE __forceinline
#else
#define BK2_NORMALS_INLINE inline __attribute__( ( always_inline ) )
#endif

namespace NNormals
{

namespace NDetail
{

	// Four bytes from anywhere, without alignment or aliasing assumptions.
	BK2_NORMALS_INLINE uint32_t Load32( const uint8_t *p )
	{
		uint32_t v;
		std::memcpy( &v, p, sizeof( v ) );
		return v;
	}

	// float( b ) - 128 is exact for every byte, so this is the single rounding the
	// engine's ( v - 128 ) / 127 makes.
	BK2_NORMALS_INLINE float DecodeScalar( uint32_t b )
	{
		return ( static_cast<float>( static_cast<int>( b ) ) - 128.0f ) / 127.0f;
	}

	// static_cast<int> as cvttss2si does it: NaN and anything out of range become
	// INT_MIN ("integer indefinite") instead of undefined behaviour.
	BK2_NORMALS_INLINE int32_t TruncateLikeSSE( float f )
	{
		if ( f >= -2147483648.0f && f < 2147483648.0f )
		{
			return static_cast<int32_t>( f );
		}
		return INT32_MIN;
	}

	// int32 -> saturate to int16 (packssdw) -> saturate to uint8 (packuswb), which
	// together are a clamp to 0..255. The + 128 wraps the way paddd does.
	BK2_NORMALS_INLINE uint32_t PackComponent( float f )
	{
		const int32_t v = static_cast<int32_t>( static_cast<uint32_t>( TruncateLikeSSE( f ) ) + 128u );
		return static_cast<uint32_t>( v < 0 ? 0 : ( v > 255 ? 255 : v ) );
	}

} // namespace NDetail

// Portable version, one normal at a time, operation for operation the same as the
// SSE2 one. pSrc points at the first normal, nStride is the distance between normals
// in bytes (sizeof( SUVInfo ) for &pUV[0].normal, 4 for a packed array), pDst
// receives nCount packed normals, pMatrix is sixteen row-major floats.
inline void
TransformNormalsRigidScalar( const void *pSrc, int nStride, uint32_t *pDst, int nCount, const float *pMatrix )
{
	const float *m = pMatrix;
	// The w input of the transform is always +0, so the fourth column contributes
	// m[i][3] * 0: a signed zero for any finite matrix, NaN for an infinite one.
	const float k0 = m[3] * 0.0f, k1 = m[7] * 0.0f, k2 = m[11] * 0.0f;
	const uint8_t *p = static_cast<const uint8_t *>( pSrc );
	for ( int n = 0; n < nCount; ++n, p += nStride )
	{
		const uint32_t d = NDetail::Load32( p );
		const float sz = NDetail::DecodeScalar( d & 0xff );
		const float sy = NDetail::DecodeScalar( ( d >> 8 ) & 0xff );
		const float sx = NDetail::DecodeScalar( ( d >> 16 ) & 0xff );
		const float tx = ( ( m[0] * sx + m[1] * sy ) + m[2] * sz ) + k0;
		const float ty = ( ( m[4] * sx + m[5] * sy ) + m[6] * sz ) + k1;
		const float tz = ( ( m[8] * sx + m[9] * sy ) + m[10] * sz ) + k2;
		const float inv = 1.0f / std::sqrt( ( tx * tx + ty * ty ) + tz * tz );
		const uint32_t bx = NDetail::PackComponent( ( tx * inv ) * 127.0f );
		const uint32_t by = NDetail::PackComponent( ( ty * inv ) * 127.0f );
		const uint32_t bz = NDetail::PackComponent( ( tz * inv ) * 127.0f );
		pDst[n] = bz | ( by << 8 ) | ( bx << 16 ) | ( d & 0xff000000u );
	}
}

#if BK2_NORMALS_SSE2

namespace NDetail
{

	// One output row of the matrix the way the four-wide kernel wants it: its three
	// 3x3 elements broadcast to all lanes, plus its m[i][3] * 0 term.
	struct SBroadcastRow
	{
		__m128 a, b, c, k;
	};

	// Named rows rather than loops over an array: GCC at -O2 left a three-row loop
	// rolled and kept the transformed components in memory, a store and a reload per
	// row per step.
	struct SBroadcastMatrix
	{
		SBroadcastRow x, y, z;
	};

	BK2_NORMALS_INLINE SBroadcastRow LoadBroadcastRow( const float *pRow )
	{
		SBroadcastRow r;
		r.a = _mm_set1_ps( pRow[0] );
		r.b = _mm_set1_ps( pRow[1] );
		r.c = _mm_set1_ps( pRow[2] );
		r.k = _mm_set1_ps( pRow[3] * 0.0f );
		return r;
	}

	BK2_NORMALS_INLINE SBroadcastMatrix LoadBroadcastMatrix( const float *pMatrix )
	{
		SBroadcastMatrix r;
		r.x = LoadBroadcastRow( pMatrix );
		r.y = LoadBroadcastRow( pMatrix + 4 );
		r.z = LoadBroadcastRow( pMatrix + 8 );
		return r;
	}

	// ( ( a sx + b sy ) + c sz ) + k: one transformed component of four normals, in
	// MMXTransformVector's order.
	BK2_NORMALS_INLINE __m128 TransformRow4( const SBroadcastRow &r,
	                                         const __m128 &sx,
	                                         const __m128 &sy,
	                                         const __m128 &sz )
	{
		const __m128 xy = _mm_add_ps( _mm_mul_ps( r.a, sx ), _mm_mul_ps( r.b, sy ) );
		return _mm_add_ps( _mm_add_ps( xy, _mm_mul_ps( r.c, sz ) ), r.k );
	}

	// Four normals from four strided addresses, normal j in lane j.
	BK2_NORMALS_INLINE __m128i Gather4( const uint8_t *p0, const uint8_t *p1, const uint8_t *p2, const uint8_t *p3 )
	{
		const __m128i a = _mm_unpacklo_epi32( _mm_cvtsi32_si128( static_cast<int>( Load32( p0 ) ) ),
		                                      _mm_cvtsi32_si128( static_cast<int>( Load32( p1 ) ) ) );
		const __m128i b = _mm_unpacklo_epi32( _mm_cvtsi32_si128( static_cast<int>( Load32( p2 ) ) ),
		                                      _mm_cvtsi32_si128( static_cast<int>( Load32( p3 ) ) ) );
		return _mm_unpacklo_epi64( a, b );
	}

	// ( float( b ) - 128 ) / 127 on four bytes held as int32, exactly the
	// MMXTransformVector decode (cvtdq2ps, subps, divps).
	BK2_NORMALS_INLINE __m128 Decode4( const __m128i &bytes )
	{
		return _mm_div_ps( _mm_sub_ps( _mm_cvtepi32_ps( bytes ), _mm_set1_ps( 128.0f ) ), _mm_set1_ps( 127.0f ) );
	}

	// Four normals' result bytes (int32 lanes, not yet clamped) and the source dwords
	// back into four packed normals. Each lane goes through packssdw and packuswb as
	// in MMXTransformVector; then the bytes, now z0..z3 y0..y3 x0..x3, are
	// interleaved back into z, y, x, w order and the source w byte is put back.
	BK2_NORMALS_INLINE __m128i Pack4( const __m128i &x, const __m128i &y, const __m128i &z, const __m128i &src )
	{
		const __m128i zero = _mm_setzero_si128();
		const __m128i bytes = _mm_packus_epi16( _mm_packs_epi32( z, y ), _mm_packs_epi32( x, zero ) );
		const __m128i zy = _mm_unpacklo_epi8( bytes, _mm_srli_si128( bytes, 4 ) );
		const __m128i x0 = _mm_unpacklo_epi8( _mm_srli_si128( bytes, 8 ), zero );
		const __m128i w = _mm_and_si128( src, _mm_set1_epi32( static_cast<int>( 0xff000000u ) ) );
		return _mm_or_si128( _mm_unpacklo_epi16( zy, x0 ), w );
	}

	// Transform, normalise and pack four decoded normals. The order of every multiply
	// and add is MMXTransformVector's, lane for lane.
	BK2_NORMALS_INLINE __m128i
	Finish4( const SBroadcastMatrix &M, const __m128 &sx, const __m128 &sy, const __m128 &sz, const __m128i &src )
	{
		const __m128 tx = TransformRow4( M.x, sx, sy, sz );
		const __m128 ty = TransformRow4( M.y, sx, sy, sz );
		const __m128 tz = TransformRow4( M.z, sx, sy, sz );
		const __m128 len2 = _mm_add_ps( _mm_add_ps( _mm_mul_ps( tx, tx ), _mm_mul_ps( ty, ty ) ), _mm_mul_ps( tz, tz ) );
		const __m128 inv = _mm_div_ps( _mm_set1_ps( 1.0f ), _mm_sqrt_ps( len2 ) );
		const __m128 scale = _mm_set1_ps( 127.0f );
		const __m128i bias = _mm_set1_epi32( 128 );
		const __m128i x = _mm_add_epi32( _mm_cvttps_epi32( _mm_mul_ps( _mm_mul_ps( tx, inv ), scale ) ), bias );
		const __m128i y = _mm_add_epi32( _mm_cvttps_epi32( _mm_mul_ps( _mm_mul_ps( ty, inv ), scale ) ), bias );
		const __m128i z = _mm_add_epi32( _mm_cvttps_epi32( _mm_mul_ps( _mm_mul_ps( tz, inv ), scale ) ), bias );
		return Pack4( x, y, z, src );
	}

	// Four packed normals (z, y, x, w bytes per lane) in, four out.
	BK2_NORMALS_INLINE __m128i Transform4( const SBroadcastMatrix &M, const __m128i &src )
	{
		const __m128i byteMask = _mm_set1_epi32( 0xff );
		const __m128 sz = Decode4( _mm_and_si128( src, byteMask ) );
		const __m128 sy = Decode4( _mm_and_si128( _mm_srli_epi32( src, 8 ), byteMask ) );
		const __m128 sx = Decode4( _mm_and_si128( _mm_srli_epi32( src, 16 ), byteMask ) );
		return Finish4( M, sx, sy, sz, src );
	}

} // namespace NDetail

// Transforms nCount normals by one matrix, bit-exact against MMXTransformVector.
// pSrc points at the first normal and nStride is the byte distance between normals
// (sizeof( SUVInfo ) when pSrc is &pUV[0].normal); pDst receives nCount packed
// normals and must not overlap the source. pMatrix is sixteen row-major floats; only
// the upper 3x4 is read.
//
// Four normals per step. A part whose count is not a multiple of four ends with one
// more step over its last four normals, recomputing up to three of them (the same
// bytes again), so nothing is read or written outside the arrays; parts of fewer than
// four normals repeat their last normal in the unused lanes and store only their own.
inline void TransformNormalsRigid( const void *pSrc, int nStride, uint32_t *pDst, int nCount, const float *pMatrix )
{
	if ( nCount <= 0 )
	{
		return;
	}
	const NDetail::SBroadcastMatrix M = NDetail::LoadBroadcastMatrix( pMatrix );
	const uint8_t *p = static_cast<const uint8_t *>( pSrc );
	const ptrdiff_t s = nStride;
	if ( nCount < 4 )
	{
		const ptrdiff_t s1 = nCount > 1 ? s : 0;
		const ptrdiff_t s2 = nCount > 2 ? 2 * s : s1;
		const __m128i r = NDetail::Transform4( M, NDetail::Gather4( p, p + s1, p + s2, p + s2 ) );
		pDst[0] = static_cast<uint32_t>( _mm_cvtsi128_si32( r ) );
		if ( nCount > 1 )
		{
			pDst[1] = static_cast<uint32_t>( _mm_cvtsi128_si32( _mm_srli_si128( r, 4 ) ) );
		}
		if ( nCount > 2 )
		{
			pDst[2] = static_cast<uint32_t>( _mm_cvtsi128_si32( _mm_srli_si128( r, 8 ) ) );
		}
		return;
	}
	int k = 0;
	for ( ; k + 4 <= nCount; k += 4 )
	{
		const uint8_t *q = p + k * s;
		_mm_storeu_si128( reinterpret_cast<__m128i *>( pDst + k ),
		                  NDetail::Transform4( M, NDetail::Gather4( q, q + s, q + 2 * s, q + 3 * s ) ) );
	}
	if ( k < nCount )
	{
		k = nCount - 4;
		const uint8_t *q = p + k * s;
		_mm_storeu_si128( reinterpret_cast<__m128i *>( pDst + k ),
		                  NDetail::Transform4( M, NDetail::Gather4( q, q + s, q + 2 * s, q + 3 * s ) ) );
	}
}

namespace NDetail
{

	// glm's mat4 * vec4 for one matrix and one normal, as MMXTransformVector2/3 run
	// it: columns ( _1j, _2j, _3j, _4j ), summed as ( c0 x + c1 y ) + ( c2 z + c3 w )
	// with w = +0. Unlike MMXTransformVector the two halves are summed separately.
	BK2_NORMALS_INLINE __m128 GlmTransform( const float *m, const __m128 &sx, const __m128 &sy, const __m128 &sz )
	{
		__m128 c0 = _mm_loadu_ps( m + 0 );
		__m128 c1 = _mm_loadu_ps( m + 4 );
		__m128 c2 = _mm_loadu_ps( m + 8 );
		__m128 c3 = _mm_loadu_ps( m + 12 );
		_MM_TRANSPOSE4_PS( c0, c1, c2, c3 );
		const __m128 xy = _mm_add_ps( _mm_mul_ps( c0, sx ), _mm_mul_ps( c1, sy ) );
		const __m128 zw = _mm_add_ps( _mm_mul_ps( c2, sz ), _mm_mul_ps( c3, _mm_setzero_ps() ) );
		return _mm_add_ps( xy, zw );
	}

	// The blended normal: decode (glm's ( b - 128.0 ) / 127.0f in double rounds to
	// the same float as the divps, for all 256 bytes), two or three glm transforms,
	// weights b / 255.f, summed left to right, glm::normalize ( ( x x + y y ) + z z,
	// then 1 / sqrt ), and SaveCompactVector's int( c * 127 ) + 128 clamped to 0..255
	// (the two saturating packs are that clamp). m3 == nullptr means two influences.
	BK2_NORMALS_INLINE uint32_t TransformBlend(
			uint32_t src, const float *m1, uint32_t nW1, const float *m2, uint32_t nW2, const float *m3, uint32_t nW3 )
	{
		const __m128 s = Decode4( _mm_setr_epi32( static_cast<int>( ( src >> 16 ) & 0xff ),
		                                          static_cast<int>( ( src >> 8 ) & 0xff ),
		                                          static_cast<int>( src & 0xff ),
		                                          0 ) );
		const __m128 sx = _mm_shuffle_ps( s, s, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		const __m128 sy = _mm_shuffle_ps( s, s, _MM_SHUFFLE( 1, 1, 1, 1 ) );
		const __m128 sz = _mm_shuffle_ps( s, s, _MM_SHUFFLE( 2, 2, 2, 2 ) );
		const __m128 w = _mm_div_ps( _mm_cvtepi32_ps( _mm_setr_epi32(
																		 static_cast<int>( nW1 ), static_cast<int>( nW2 ), static_cast<int>( nW3 ), 0 ) ),
		                             _mm_set1_ps( 255.0f ) );
		const __m128 v1 = _mm_mul_ps( GlmTransform( m1, sx, sy, sz ), _mm_shuffle_ps( w, w, _MM_SHUFFLE( 0, 0, 0, 0 ) ) );
		const __m128 v2 = _mm_mul_ps( GlmTransform( m2, sx, sy, sz ), _mm_shuffle_ps( w, w, _MM_SHUFFLE( 1, 1, 1, 1 ) ) );
		__m128 v = _mm_add_ps( v1, v2 );
		if ( m3 )
		{
			v = _mm_add_ps( v,
			                _mm_mul_ps( GlmTransform( m3, sx, sy, sz ), _mm_shuffle_ps( w, w, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ) );
		}
		const __m128 sq = _mm_mul_ps( v, v );
		__m128 len2 = _mm_add_ss( sq, _mm_shuffle_ps( sq, sq, _MM_SHUFFLE( 1, 1, 1, 1 ) ) );
		len2 = _mm_add_ss( len2, _mm_shuffle_ps( sq, sq, _MM_SHUFFLE( 2, 2, 2, 2 ) ) );
		__m128 inv = _mm_div_ss( _mm_set_ss( 1.0f ), _mm_sqrt_ss( len2 ) );
		inv = _mm_shuffle_ps( inv, inv, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		__m128i r = _mm_cvttps_epi32( _mm_mul_ps( _mm_mul_ps( v, inv ), _mm_set1_ps( 127.0f ) ) );
		r = _mm_add_epi32( r, _mm_set1_epi32( 128 ) );
		r = _mm_shuffle_epi32( r, _MM_SHUFFLE( 3, 0, 1, 2 ) );
		r = _mm_packus_epi16( _mm_packs_epi32( r, r ), _mm_setzero_si128() );
		return ( static_cast<uint32_t>( _mm_cvtsi128_si32( r ) ) & 0x00ffffffu ) | ( src & 0xff000000u );
	}

} // namespace NDetail

// Bit-exact replacements for MMXTransformVector2 and MMXTransformVector3
// (GSSEtransform.h, glm), one normal at a time: src is the packed normal, m1..m3
// sixteen row-major floats each, nW1..nW3 the byte weights. For the blended
// vertices of non-rigid parts, which change bones from vertex to vertex.
inline uint32_t TransformNormalBlend2( uint32_t src, const float *m1, uint32_t nW1, const float *m2, uint32_t nW2 )
{
	return NDetail::TransformBlend( src, m1, nW1, m2, nW2, nullptr, 0 );
}

inline uint32_t TransformNormalBlend3(
		uint32_t src, const float *m1, uint32_t nW1, const float *m2, uint32_t nW2, const float *m3, uint32_t nW3 )
{
	return NDetail::TransformBlend( src, m1, nW1, m2, nW2, m3, nW3 );
}

#else

inline void TransformNormalsRigid( const void *pSrc, int nStride, uint32_t *pDst, int nCount, const float *pMatrix )
{
	TransformNormalsRigidScalar( pSrc, nStride, pDst, nCount, pMatrix );
}

#endif

namespace NDetail
{

	// The scalar form of TransformBlend, the same operations in the same order; the
	// fallback without SSE2, and a second opinion in the test.
	inline uint32_t TransformBlendScalar(
			uint32_t src, const float *m1, uint32_t nW1, const float *m2, uint32_t nW2, const float *m3, uint32_t nW3 )
	{
		const float s[3] = {
			DecodeScalar( ( src >> 16 ) & 0xff ), DecodeScalar( ( src >> 8 ) & 0xff ), DecodeScalar( src & 0xff ) };
		const float *m[3] = { m1, m2, m3 };
		const float w[3] = { static_cast<float>( static_cast<int>( nW1 ) ) / 255.0f,
		                     static_cast<float>( static_cast<int>( nW2 ) ) / 255.0f,
		                     static_cast<float>( static_cast<int>( nW3 ) ) / 255.0f };
		float v[3] = { 0.0f, 0.0f, 0.0f };
		const int nBones = m3 ? 3 : 2;
		for ( int b = 0; b < nBones; ++b )
		{
			for ( int i = 0; i < 3; ++i )
			{
				const float *r = m[b] + 4 * i;
				const float t = ( r[0] * s[0] + r[1] * s[1] ) + ( r[2] * s[2] + r[3] * 0.0f );
				v[i] = b == 0 ? t * w[0] : v[i] + t * w[b];
			}
		}
		const float inv = 1.0f / std::sqrt( ( v[0] * v[0] + v[1] * v[1] ) + v[2] * v[2] );
		return PackComponent( ( v[2] * inv ) * 127.0f ) | ( PackComponent( ( v[1] * inv ) * 127.0f ) << 8 ) |
		       ( PackComponent( ( v[0] * inv ) * 127.0f ) << 16 ) | ( src & 0xff000000u );
	}

} // namespace NDetail

#if !BK2_NORMALS_SSE2

inline uint32_t TransformNormalBlend2( uint32_t src, const float *m1, uint32_t nW1, const float *m2, uint32_t nW2 )
{
	return NDetail::TransformBlendScalar( src, m1, nW1, m2, nW2, nullptr, 0 );
}

inline uint32_t TransformNormalBlend3(
		uint32_t src, const float *m1, uint32_t nW1, const float *m2, uint32_t nW2, const float *m3, uint32_t nW3 )
{
	return NDetail::TransformBlendScalar( src, m1, nW1, m2, nW2, m3, nW3 );
}

#endif

} // namespace NNormals
