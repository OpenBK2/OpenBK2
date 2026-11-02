#pragma once
// Position skinning: every vertex of a skinned part is moved by a weighted blend of
// up to four bone matrices. This is what TransformPosition( srcPos, pRes, pWeight,
// blends ) in GCombiner.cpp does through SSESkinning (GSSEtransform.h), and on a
// replay with dozens of tanks it is about a fifth of the main thread.
//
// Two kernels, for the two kinds of part the census of the shipped data found:
//
//   TransformPositions  - rigid parts: every position has one influence of weight
//                         exactly 1 and the same bone. 98.8% of the 32,333 render
//                         parts and 94.4% of their positions. One matrix for the
//                         call, its columns kept in registers, weights never read.
//                         TransformPositionsBounds also returns the box of the
//                         results, fused into the same pass.
//   SkinPositions       - everything else (infantry, flags): per vertex, blend the
//                         three used rows of up to four SHMatrix (12 floats, not
//                         SSESkinning's 16 after four transposes), then three dot
//                         products. One influence and two influences in slots 0
//                         and 1 have straight-line paths; anything else, including
//                         zero weights in the middle, goes through a loop that skips
//                         zero weights exactly as SSESkinning does.
//
// SkinPositions reads the matrices where they are, so it needs no scratch and no
// per-call preparation. That matters: a kernel that first converted every bone of the
// skeleton to column form was 20% faster on large parts of a 16-bone skeleton, but
// with 241 bones (the largest skeleton shipped) the conversion made a 16-position
// part 3.5x slower than SSESkinning. test/SkinningVariants.h keeps that one and the
// other candidates; benchmark/Skinning.cpp compares them, test/Skinning.cpp holds
// them all to a double precision oracle.
//
// The results only feed rendering, culling bounds and shadows, never the lockstep
// simulation, so they agree with SSESkinning within float rounding (about 1.1 to 1.8
// FLT_EPSILON of the summed magnitudes, 1e-4 absolute at translations near 1000),
// not bit for bit.
//
// The header deliberately knows no engine types, so it can be compiled and tested
// on its own: positions are packed x, y, z floats (CVec3 is exactly that), matrices
// are sixteen row-major floats with the translation in _14, _24, _34 (SHMatrix), and
// SSkinWeights has the layout of SRealVertexWeight / SSSEVertexWeight. Matrices need
// no particular alignment.
//
// SSE2 only: the x86-64 baseline, and what MSVC x86 targets by default. Where SSE2
// is not available at compile time the scalar versions are used.

#include <cstdint>

#if defined( __SSE2__ ) || defined( _M_X64 ) || ( defined( _M_IX86_FP ) && _M_IX86_FP >= 2 )
#define BK2_SKINNING_SSE2 1
#include <emmintrin.h>
#else
#define BK2_SKINNING_SSE2 0
#endif

// The per-vertex helpers are called from two places in each kernel (the loop and the
// exact last vertex), and GCC at -O2 then kept them out of line: a call per vertex.
// Forcing them inline removes it.
#if defined( _MSC_VER )
#define BK2_SKIN_INLINE __forceinline
#else
#define BK2_SKIN_INLINE inline __attribute__( ( always_inline ) )
#endif

namespace NSkinning
{

// Same layout as SRealVertexWeight (3DLib/GGeometry.h) and SSSEVertexWeight
// (GSSEtransform.h): float weights, the byte weights the scalar engine path tests,
// and bone indices into the matrix array. A zero float weight means "no influence";
// its bone index is never read, so it may hold anything.
struct SSkinWeights
{
	float fWeights[4];
	uint8_t nWeights[4];
	uint8_t cBoneIndices[4];
};

static_assert( sizeof( SSkinWeights ) == 24, "must match SRealVertexWeight" );

// Portable versions: the same sums one float at a time.
inline void TransformPositionsScalar( const float *pSrc, float *pDst, int nCount, const float *pMatrix )
{
	const float *m = pMatrix;
	for ( int k = 0; k < nCount; ++k )
	{
		const float x = pSrc[3 * k + 0], y = pSrc[3 * k + 1], z = pSrc[3 * k + 2];
		pDst[3 * k + 0] = m[0] * x + m[1] * y + m[2] * z + m[3];
		pDst[3 * k + 1] = m[4] * x + m[5] * y + m[6] * z + m[7];
		pDst[3 * k + 2] = m[8] * x + m[9] * y + m[10] * z + m[11];
	}
}

inline void
SkinPositionsScalar( const float *pSrc, float *pDst, const SSkinWeights *pWeights, int nCount, const float *pMatrices )
{
	for ( int k = 0; k < nCount; ++k )
	{
		const float x = pSrc[3 * k + 0], y = pSrc[3 * k + 1], z = pSrc[3 * k + 2];
		float r[3] = { 0.0f, 0.0f, 0.0f };
		for ( int i = 0; i < 4; ++i )
		{
			const float w = pWeights[k].fWeights[i];
			if ( w == 0.0f )
			{
				continue;
			}
			const float *m = pMatrices + 16 * pWeights[k].cBoneIndices[i];
			for ( int j = 0; j < 3; ++j )
			{
				r[j] += w * ( m[4 * j] * x + m[4 * j + 1] * y + m[4 * j + 2] * z + m[4 * j + 3] );
			}
		}
		pDst[3 * k + 0] = r[0];
		pDst[3 * k + 1] = r[1];
		pDst[3 * k + 2] = r[2];
	}
}

#if BK2_SKINNING_SSE2

namespace NDetail
{

	// x, y, z of the last vertex without touching the four bytes after it: two floats,
	// then the third, so a part that ends at the end of a page cannot fault.
	BK2_SKIN_INLINE __m128 LoadXYZSafe( const float *p )
	{
		const __m128 xy = _mm_castpd_ps( _mm_load_sd( reinterpret_cast<const double *>( p ) ) );
		return _mm_movelh_ps( xy, _mm_load_ss( p + 2 ) );
	}

	// The counterpart for the last result: eight bytes, then four, nothing beyond.
	BK2_SKIN_INLINE void StoreXYZSafe( float *p, const __m128 &v )
	{
		_mm_store_sd( reinterpret_cast<double *>( p ), _mm_castps_pd( v ) );
		_mm_store_ss( p + 2, _mm_movehl_ps( v, v ) );
	}

	// ( dot( t0 ), dot( t1 ), dot( t2 ), 0 ) where dot sums a vector's four lanes, as
	// ( x + z ) + ( y + w ): the three row dot products of the row form at once.
	BK2_SKIN_INLINE __m128 ReduceRows( const __m128 &t0, const __m128 &t1, const __m128 &t2 )
	{
		const __m128 zero = _mm_setzero_ps();
		const __m128 s01 = _mm_add_ps( _mm_unpacklo_ps( t0, t1 ), _mm_unpackhi_ps( t0, t1 ) );
		const __m128 s2 = _mm_add_ps( _mm_unpacklo_ps( t2, zero ), _mm_unpackhi_ps( t2, zero ) );
		return _mm_add_ps( _mm_movelh_ps( s01, s2 ), _mm_movehl_ps( s2, s01 ) );
	}

	// ( x, y, z, 1 ) from a load whose fourth lane is the next vertex's x.
	BK2_SKIN_INLINE __m128 MakeHomogeneous( const __m128 &p )
	{
		const __m128 xyz = _mm_castsi128_ps( _mm_set_epi32( 0, -1, -1, -1 ) );
		return _mm_or_ps( _mm_and_ps( p, xyz ), _mm_set_ps( 1.0f, 0.0f, 0.0f, 0.0f ) );
	}

	// One blended vertex; p holds x, y, z in its low lanes. w * row . ( x, y, z, 1 )
	// is computed as row . ( w x, w y, w z, w ), so blending the rows costs three
	// multiplies and three adds per influence and the dot products are done once.
	BK2_SKIN_INLINE __m128 SkinVertex( const __m128 &p, const SSkinWeights &w, const float *pMatrices )
	{
		const __m128 p1 = MakeHomogeneous( p );
		const __m128 weights = _mm_loadu_ps( w.fWeights );
		const int nMask = _mm_movemask_ps( _mm_cmpneq_ps( weights, _mm_setzero_ps() ) );
		if ( nMask == 1 )
		{
			const float *m = pMatrices + 16 * w.cBoneIndices[0];
			const __m128 wp = _mm_mul_ps( p1, _mm_shuffle_ps( weights, weights, _MM_SHUFFLE( 0, 0, 0, 0 ) ) );
			return ReduceRows( _mm_mul_ps( _mm_loadu_ps( m ), wp ),
			                   _mm_mul_ps( _mm_loadu_ps( m + 4 ), wp ),
			                   _mm_mul_ps( _mm_loadu_ps( m + 8 ), wp ) );
		}
		if ( nMask == 3 )
		{
			const float *m0 = pMatrices + 16 * w.cBoneIndices[0];
			const float *m1 = pMatrices + 16 * w.cBoneIndices[1];
			const __m128 wp0 = _mm_mul_ps( p1, _mm_shuffle_ps( weights, weights, _MM_SHUFFLE( 0, 0, 0, 0 ) ) );
			const __m128 wp1 = _mm_mul_ps( p1, _mm_shuffle_ps( weights, weights, _MM_SHUFFLE( 1, 1, 1, 1 ) ) );
			return ReduceRows(
					_mm_add_ps( _mm_mul_ps( _mm_loadu_ps( m0 ), wp0 ), _mm_mul_ps( _mm_loadu_ps( m1 ), wp1 ) ),
					_mm_add_ps( _mm_mul_ps( _mm_loadu_ps( m0 + 4 ), wp0 ), _mm_mul_ps( _mm_loadu_ps( m1 + 4 ), wp1 ) ),
					_mm_add_ps( _mm_mul_ps( _mm_loadu_ps( m0 + 8 ), wp0 ), _mm_mul_ps( _mm_loadu_ps( m1 + 8 ), wp1 ) ) );
		}
		__m128 t0 = _mm_setzero_ps(), t1 = _mm_setzero_ps(), t2 = _mm_setzero_ps();
		for ( int i = 0; i < 4; ++i )
		{
			if ( nMask & ( 1 << i ) )
			{
				const float *m = pMatrices + 16 * w.cBoneIndices[i];
				const __m128 wp = _mm_mul_ps( p1, _mm_set1_ps( w.fWeights[i] ) );
				t0 = _mm_add_ps( t0, _mm_mul_ps( _mm_loadu_ps( m ), wp ) );
				t1 = _mm_add_ps( t1, _mm_mul_ps( _mm_loadu_ps( m + 4 ), wp ) );
				t2 = _mm_add_ps( t2, _mm_mul_ps( _mm_loadu_ps( m + 8 ), wp ) );
			}
		}
		return ReduceRows( t0, t1, t2 );
	}

	// One matrix as four columns ( _11 _21 _31 0 ), ( _12 ... ), ( _13 ... ),
	// ( _14 _24 _34 0 ), for the rigid kernels, which keep them in registers.
	struct SColumns
	{
		__m128 c0, c1, c2, c3;
	};

	BK2_SKIN_INLINE SColumns LoadColumns( const float *m )
	{
		SColumns c;
		c.c0 = _mm_loadu_ps( m + 0 );
		c.c1 = _mm_loadu_ps( m + 4 );
		c.c2 = _mm_loadu_ps( m + 8 );
		c.c3 = _mm_setzero_ps();
		_MM_TRANSPOSE4_PS( c.c0, c.c1, c.c2, c.c3 );
		return c;
	}

	// M * ( x, y, z, 1 ) as ( c0 x + c1 y ) + ( c2 z + c3 ), so the halves do not
	// wait on each other.
	BK2_SKIN_INLINE __m128 TransformByColumns( const SColumns &c, const __m128 &p )
	{
		const __m128 x = _mm_shuffle_ps( p, p, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		const __m128 y = _mm_shuffle_ps( p, p, _MM_SHUFFLE( 1, 1, 1, 1 ) );
		const __m128 z = _mm_shuffle_ps( p, p, _MM_SHUFFLE( 2, 2, 2, 2 ) );
		const __m128 xy = _mm_add_ps( _mm_mul_ps( c.c0, x ), _mm_mul_ps( c.c1, y ) );
		const __m128 zt = _mm_add_ps( _mm_mul_ps( c.c2, z ), c.c3 );
		return _mm_add_ps( xy, zt );
	}

} // namespace NDetail

// Skins nCount vertices: pSrc/pDst are packed x, y, z triples and must not overlap,
// pMatrices the bone array the weights' indices refer to. All but the last vertex
// are read and written sixteen bytes at a time: the extra four bytes read belong to
// the next vertex, and the extra four written are overwritten by the next vertex's
// result. The last vertex is read and written exactly, so nothing outside the
// arrays is touched.
inline void
SkinPositions( const float *pSrc, float *pDst, const SSkinWeights *pWeights, int nCount, const float *pMatrices )
{
	if ( nCount <= 0 )
	{
		return;
	}
	const int nLast = nCount - 1;
	for ( int k = 0; k < nLast; ++k )
	{
		_mm_storeu_ps( pDst + 3 * k, NDetail::SkinVertex( _mm_loadu_ps( pSrc + 3 * k ), pWeights[k], pMatrices ) );
	}
	NDetail::StoreXYZSafe( pDst + 3 * nLast,
	                       NDetail::SkinVertex( NDetail::LoadXYZSafe( pSrc + 3 * nLast ), pWeights[nLast], pMatrices ) );
}

// Rigid parts: the whole part through one matrix (pMatrix, sixteen row-major
// floats), the same load/store scheme as SkinPositions.
inline void TransformPositions( const float *pSrc, float *pDst, int nCount, const float *pMatrix )
{
	if ( nCount <= 0 )
	{
		return;
	}
	const NDetail::SColumns cols = NDetail::LoadColumns( pMatrix );
	const int nLast = nCount - 1;
	for ( int k = 0; k < nLast; ++k )
	{
		_mm_storeu_ps( pDst + 3 * k, NDetail::TransformByColumns( cols, _mm_loadu_ps( pSrc + 3 * k ) ) );
	}
	NDetail::StoreXYZSafe( pDst + 3 * nLast,
	                       NDetail::TransformByColumns( cols, NDetail::LoadXYZSafe( pSrc + 3 * nLast ) ) );
}

// TransformPositions plus the axis-aligned box of the results, which XFormPosition
// otherwise computes in a second pass over the array. pMin/pMax receive three floats
// each and are left untouched when nCount is 0; the caller applies its own seeding
// (XFormPosition starts ptMax at the origin).
inline void
TransformPositionsBounds( const float *pSrc, float *pDst, int nCount, const float *pMatrix, float *pMin, float *pMax )
{
	if ( nCount <= 0 )
	{
		return;
	}
	const NDetail::SColumns cols = NDetail::LoadColumns( pMatrix );
	const int nLast = nCount - 1;
	__m128 vMin = _mm_set1_ps( 3.402823466e+38f ), vMax = _mm_set1_ps( -3.402823466e+38f );
	for ( int k = 0; k < nLast; ++k )
	{
		const __m128 r = NDetail::TransformByColumns( cols, _mm_loadu_ps( pSrc + 3 * k ) );
		vMin = _mm_min_ps( vMin, r );
		vMax = _mm_max_ps( vMax, r );
		_mm_storeu_ps( pDst + 3 * k, r );
	}
	const __m128 r = NDetail::TransformByColumns( cols, NDetail::LoadXYZSafe( pSrc + 3 * nLast ) );
	NDetail::StoreXYZSafe( pDst + 3 * nLast, r );
	NDetail::StoreXYZSafe( pMin, _mm_min_ps( vMin, r ) );
	NDetail::StoreXYZSafe( pMax, _mm_max_ps( vMax, r ) );
}

#else

inline void
SkinPositions( const float *pSrc, float *pDst, const SSkinWeights *pWeights, int nCount, const float *pMatrices )
{
	SkinPositionsScalar( pSrc, pDst, pWeights, nCount, pMatrices );
}

inline void TransformPositions( const float *pSrc, float *pDst, int nCount, const float *pMatrix )
{
	TransformPositionsScalar( pSrc, pDst, nCount, pMatrix );
}

inline void
TransformPositionsBounds( const float *pSrc, float *pDst, int nCount, const float *pMatrix, float *pMin, float *pMax )
{
	TransformPositionsScalar( pSrc, pDst, nCount, pMatrix );
	for ( int k = 0; k < nCount; ++k )
	{
		for ( int j = 0; j < 3; ++j )
		{
			const float v = pDst[3 * k + j];
			if ( k == 0 || v < pMin[j] )
			{
				pMin[j] = v;
			}
			if ( k == 0 || v > pMax[j] )
			{
				pMax[j] = v;
			}
		}
	}
}

#endif

} // namespace NSkinning
