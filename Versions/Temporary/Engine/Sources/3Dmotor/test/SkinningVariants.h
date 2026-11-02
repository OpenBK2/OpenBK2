#pragma once
// Every position skinning kernel that was measured against SSESkinning
// (GSSEtransform.h), shared by the test that holds them all to a double precision
// oracle (test/Skinning.cpp) and the benchmark that compares them
// (benchmark/Skinning.cpp). The candidates for the engine are in GSkinning.h:
// NSkinning::SkinPositions (here RowsBlendFast) and NSkinning::TransformPositions
// (RigidSSE). The rest are kept so the comparison can be rerun on another compiler or
// CPU.
//
// Like GSkinning.h this header knows no engine types (packed x, y, z floats, sixteen
// row-major floats per matrix, SSkinWeights), so it compiles on its own, which is how
// MSVC x86/x64 was checked. The reference copy of SSESkinning needs glm and the
// engine's CVec3/SHMatrix, so it lives apart in test/SkinningReference.h.
//
// The AVX2 variants are compiled with a per-function target attribute on GCC and
// Clang, which only test and benchmark code may do: callers must check
// NGScene::IsAVX2Present() first. MSVC accepts the intrinsics without any /arch flag.
// No lambdas inside those functions, because a GCC lambda does not inherit the
// enclosing function's target and then fails to inline the intrinsics.

#include "3Dmotor/GSkinning.h"

#include <cstdint>
#include <vector>

#if BK2_SKINNING_SSE2
#include <immintrin.h>
#endif

#if defined( __GNUC__ ) || defined( __clang__ )
#define BK2_SKIN_TARGET_AVX2 __attribute__( ( target( "avx2" ) ) )
#else
#define BK2_SKIN_TARGET_AVX2
#endif

namespace NSkinningVariants
{

using NSkinning::SSkinWeights;

// One bone as four columns of its affine 3x4 part: c[0] = ( _11, _21, _31, 0 ),
// c[1] = ( _12, _22, _32, 0 ), c[2] = ( _13, _23, _33, 0 ), c[3] = ( _14, _24, _34, 0 ).
// The column-form variants convert every bone of the skeleton into a scratch array of
// these once per call; that conversion is what made them lose on big skeletons.
struct alignas( 16 ) SBoneColumns
{
	float c[4][4];
};

// A run of consecutive vertices that share the same set of influences (same nonzero
// weight slots, same bones in them). A load-time reorder that groups vertices by bone
// set would make these long; SkinRuns* hoist the bone lookup out of the vertex loop.
struct SSkinRun
{
	int nStart;
	int nCount;
	uint8_t nMask;
	uint8_t cBones[4];
	// One influence whose weight is exactly 1 for every vertex of the run: a plain
	// rigid transform, no weight multiply.
	bool bUnitWeight;
};

// Everything a variant gets for one call, so the test and the benchmark can treat
// them uniformly. pScratch has room for nBones columns; variants that need columns
// prepare them inside the call, because that is a per-call cost in the engine too.
// pRuns / nRuns are only read by the Runs variants and are built at "load time" by
// BuildRuns, outside any timing.
struct SSkinCall
{
	const float *pSrc;
	float *pDst;
	const SSkinWeights *pWeights;
	int nCount;
	const float *pMatrices;
	int nBones;
	SBoneColumns *pScratch;
	const SSkinRun *pRuns;
	int nRuns;
};

inline int WeightMask( const SSkinWeights &w )
{
	int nMask = 0;
	for ( int i = 0; i < 4; ++i )
	{
		if ( w.fWeights[i] != 0.0f )
		{
			nMask |= 1 << i;
		}
	}
	return nMask;
}

// Splits the vertex array into maximal runs of identical influence sets. Load-time
// work: the engine would do it once per mesh, so it is never timed.
inline void BuildRuns( const SSkinWeights *pWeights, int nCount, std::vector<SSkinRun> &runs )
{
	runs.clear();
	for ( int k = 0; k < nCount; ++k )
	{
		const int nMask = WeightMask( pWeights[k] );
		SSkinRun cur{};
		cur.nStart = k;
		cur.nCount = 1;
		cur.nMask = static_cast<uint8_t>( nMask );
		for ( int i = 0; i < 4; ++i )
		{
			cur.cBones[i] = ( nMask & ( 1 << i ) ) ? pWeights[k].cBoneIndices[i] : 0;
		}
		cur.bUnitWeight = nMask == 1 && pWeights[k].fWeights[0] == 1.0f;
		if ( !runs.empty() )
		{
			SSkinRun &last = runs.back();
			bool bSame = last.nMask == cur.nMask;
			for ( int i = 0; i < 4 && bSame; ++i )
			{
				bSame = last.cBones[i] == cur.cBones[i];
			}
			if ( bSame )
			{
				++last.nCount;
				last.bUnitWeight = last.bUnitWeight && cur.bUnitWeight;
				continue;
			}
		}
		runs.push_back( cur );
	}
}

// The portable kernel from GSkinning.h.
inline void Scalar( const SSkinCall &c )
{
	NSkinning::SkinPositionsScalar( c.pSrc, c.pDst, c.pWeights, c.nCount, c.pMatrices );
}

#if BK2_SKINNING_SSE2

namespace NDetail
{

	using NSkinning::NDetail::LoadXYZSafe;
	using NSkinning::NDetail::MakeHomogeneous;
	using NSkinning::NDetail::ReduceRows;
	using NSkinning::NDetail::StoreXYZSafe;

	// Rows -> columns for every bone, once per call: four unaligned loads and a 4x4
	// transpose per bone, the bottom row replaced by zero.
	inline void PrepareBoneColumns( const float *pMatrices, int nBones, SBoneColumns *pCols )
	{
		for ( int b = 0; b < nBones; ++b )
		{
			const float *m = pMatrices + 16 * b;
			__m128 r0 = _mm_loadu_ps( m + 0 );
			__m128 r1 = _mm_loadu_ps( m + 4 );
			__m128 r2 = _mm_loadu_ps( m + 8 );
			__m128 r3 = _mm_setzero_ps();
			_MM_TRANSPOSE4_PS( r0, r1, r2, r3 );
			_mm_store_ps( pCols[b].c[0], r0 );
			_mm_store_ps( pCols[b].c[1], r1 );
			_mm_store_ps( pCols[b].c[2], r2 );
			_mm_store_ps( pCols[b].c[3], r3 );
		}
	}

	// M * ( x, y, z, 1 ) from one bone's prepared columns.
	BK2_SKIN_INLINE __m128 TransformByColumns( const SBoneColumns &b, const __m128 &x, const __m128 &y, const __m128 &z )
	{
		const __m128 xy = _mm_add_ps( _mm_mul_ps( _mm_load_ps( b.c[0] ), x ), _mm_mul_ps( _mm_load_ps( b.c[1] ), y ) );
		const __m128 zt = _mm_add_ps( _mm_mul_ps( _mm_load_ps( b.c[2] ), z ), _mm_load_ps( b.c[3] ) );
		return _mm_add_ps( xy, zt );
	}

	// Variant 3a with the one- and two-influence paths (ColsXformFast): transform by
	// each influence's columns, then blend the results.
	BK2_SKIN_INLINE __m128 VertexColsXformFast( const __m128 &p, const SSkinWeights &w, const SBoneColumns *pCols )
	{
		const __m128 x = _mm_shuffle_ps( p, p, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		const __m128 y = _mm_shuffle_ps( p, p, _MM_SHUFFLE( 1, 1, 1, 1 ) );
		const __m128 z = _mm_shuffle_ps( p, p, _MM_SHUFFLE( 2, 2, 2, 2 ) );
		const __m128 weights = _mm_loadu_ps( w.fWeights );
		const int nMask = _mm_movemask_ps( _mm_cmpneq_ps( weights, _mm_setzero_ps() ) );
		if ( nMask == 1 )
		{
			const __m128 r = TransformByColumns( pCols[w.cBoneIndices[0]], x, y, z );
			return _mm_mul_ps( r, _mm_shuffle_ps( weights, weights, _MM_SHUFFLE( 0, 0, 0, 0 ) ) );
		}
		if ( nMask == 3 )
		{
			const __m128 r0 = TransformByColumns( pCols[w.cBoneIndices[0]], x, y, z );
			const __m128 r1 = TransformByColumns( pCols[w.cBoneIndices[1]], x, y, z );
			return _mm_add_ps( _mm_mul_ps( r0, _mm_shuffle_ps( weights, weights, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ),
			                   _mm_mul_ps( r1, _mm_shuffle_ps( weights, weights, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ) );
		}
		__m128 acc = _mm_setzero_ps();
		for ( int i = 0; i < 4; ++i )
		{
			if ( nMask & ( 1 << i ) )
			{
				const __m128 r = TransformByColumns( pCols[w.cBoneIndices[i]], x, y, z );
				acc = _mm_add_ps( acc, _mm_mul_ps( r, _mm_set1_ps( w.fWeights[i] ) ) );
			}
		}
		return acc;
	}

	// Variant 2: SSESkinning's algorithm (blend all of the matrix, then transform)
	// on columns prepared once per call, so no per-influence LoadMatrix transposes.
	BK2_SKIN_INLINE __m128 VertexColsBlend( const __m128 &p, const SSkinWeights &w, const SBoneColumns *pCols )
	{
		__m128 c0 = _mm_setzero_ps(), c1 = _mm_setzero_ps(), c2 = _mm_setzero_ps(), c3 = _mm_setzero_ps();
		for ( int i = 0; i < 4; ++i )
		{
			const float f = w.fWeights[i];
			if ( f == 0.0f )
			{
				continue;
			}
			const SBoneColumns &b = pCols[w.cBoneIndices[i]];
			const __m128 vw = _mm_set1_ps( f );
			c0 = _mm_add_ps( c0, _mm_mul_ps( vw, _mm_load_ps( b.c[0] ) ) );
			c1 = _mm_add_ps( c1, _mm_mul_ps( vw, _mm_load_ps( b.c[1] ) ) );
			c2 = _mm_add_ps( c2, _mm_mul_ps( vw, _mm_load_ps( b.c[2] ) ) );
			c3 = _mm_add_ps( c3, _mm_mul_ps( vw, _mm_load_ps( b.c[3] ) ) );
		}
		const __m128 x = _mm_shuffle_ps( p, p, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		const __m128 y = _mm_shuffle_ps( p, p, _MM_SHUFFLE( 1, 1, 1, 1 ) );
		const __m128 z = _mm_shuffle_ps( p, p, _MM_SHUFFLE( 2, 2, 2, 2 ) );
		return _mm_add_ps( _mm_add_ps( _mm_mul_ps( c0, x ), _mm_mul_ps( c1, y ) ), _mm_add_ps( _mm_mul_ps( c2, z ), c3 ) );
	}

	// Variant 3a: transform by each influence, then blend the results; the same
	// arithmetic as VertexColsXformFast below but without its one- and two-influence
	// paths, to measure what those are worth.
	BK2_SKIN_INLINE __m128 VertexColsXform( const __m128 &p, const SSkinWeights &w, const SBoneColumns *pCols )
	{
		const __m128 x = _mm_shuffle_ps( p, p, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		const __m128 y = _mm_shuffle_ps( p, p, _MM_SHUFFLE( 1, 1, 1, 1 ) );
		const __m128 z = _mm_shuffle_ps( p, p, _MM_SHUFFLE( 2, 2, 2, 2 ) );
		__m128 acc = _mm_setzero_ps();
		for ( int i = 0; i < 4; ++i )
		{
			const float f = w.fWeights[i];
			if ( f == 0.0f )
			{
				continue;
			}
			const __m128 r = TransformByColumns( pCols[w.cBoneIndices[i]], x, y, z );
			acc = _mm_add_ps( acc, _mm_mul_ps( r, _mm_set1_ps( f ) ) );
		}
		return acc;
	}

	// Variant 3b without the one- and two-influence paths (RowsBlend), to measure what
	// those are worth in NSkinning::SkinPositions.
	BK2_SKIN_INLINE __m128 VertexRows( const __m128 &p, const SSkinWeights &w, const float *pMatrices )
	{
		const __m128 p1 = MakeHomogeneous( p );
		__m128 t0 = _mm_setzero_ps(), t1 = _mm_setzero_ps(), t2 = _mm_setzero_ps();
		for ( int i = 0; i < 4; ++i )
		{
			const float f = w.fWeights[i];
			if ( f == 0.0f )
			{
				continue;
			}
			const float *m = pMatrices + 16 * w.cBoneIndices[i];
			const __m128 wp = _mm_mul_ps( p1, _mm_set1_ps( f ) );
			t0 = _mm_add_ps( t0, _mm_mul_ps( _mm_loadu_ps( m ), wp ) );
			t1 = _mm_add_ps( t1, _mm_mul_ps( _mm_loadu_ps( m + 4 ), wp ) );
			t2 = _mm_add_ps( t2, _mm_mul_ps( _mm_loadu_ps( m + 8 ), wp ) );
		}
		return ReduceRows( t0, t1, t2 );
	}

	// The per-vertex functions wrapped as function objects, so AoSLoop is
	// instantiated per variant and the vertex code inlines into its loop.
	struct FColsBlend
	{
		const SBoneColumns *pCols;

		__m128 operator()( const __m128 &p, const SSkinWeights &w ) const
		{
			return VertexColsBlend( p, w, pCols );
		}
	};

	struct FColsXform
	{
		const SBoneColumns *pCols;

		__m128 operator()( const __m128 &p, const SSkinWeights &w ) const
		{
			return VertexColsXform( p, w, pCols );
		}
	};

	struct FColsXformFast
	{
		const SBoneColumns *pCols;

		__m128 operator()( const __m128 &p, const SSkinWeights &w ) const
		{
			return VertexColsXformFast( p, w, pCols );
		}
	};

	struct FRows
	{
		const float *pMatrices;

		__m128 operator()( const __m128 &p, const SSkinWeights &w ) const
		{
			return VertexRows( p, w, pMatrices );
		}
	};

	template <class TVertex>
	inline void AoSLoop( const SSkinCall &c, int nFrom, const TVertex &vertex )
	{
		if ( nFrom >= c.nCount )
		{
			return;
		}
		const int nLast = c.nCount - 1;
		for ( int k = nFrom; k < nLast; ++k )
		{
			_mm_storeu_ps( c.pDst + 3 * k, vertex( _mm_loadu_ps( c.pSrc + 3 * k ), c.pWeights[k] ) );
		}
		StoreXYZSafe( c.pDst + 3 * nLast, vertex( LoadXYZSafe( c.pSrc + 3 * nLast ), c.pWeights[nLast] ) );
	}

	// SoA <-> AoS for four vertices (one 128-bit lane of the AVX2 version below).
	// In: a = x0 y0 z0 x1, b = y1 z1 x2 y2, c = z2 x3 y3 z3. Out: x0..x3, y0..y3, z0..z3.
	BK2_SKIN_INLINE void
	Deinterleave4( const __m128 &a, const __m128 &b, const __m128 &c, __m128 &x, __m128 &y, __m128 &z )
	{
		const __m128 xy = _mm_shuffle_ps( b, c, _MM_SHUFFLE( 2, 1, 3, 2 ) ); // x2 y2 x3 y3
		const __m128 yz = _mm_shuffle_ps( a, b, _MM_SHUFFLE( 1, 0, 2, 1 ) ); // y0 z0 y1 z1
		x = _mm_shuffle_ps( a, xy, _MM_SHUFFLE( 2, 0, 3, 0 ) );
		y = _mm_shuffle_ps( yz, xy, _MM_SHUFFLE( 3, 1, 2, 0 ) );
		z = _mm_shuffle_ps( yz, c, _MM_SHUFFLE( 3, 0, 3, 1 ) );
	}

	BK2_SKIN_INLINE void Interleave4( const __m128 &x, const __m128 &y, const __m128 &z, __m128 &a, __m128 &b, __m128 &c )
	{
		const __m128 rxy = _mm_shuffle_ps( x, y, _MM_SHUFFLE( 2, 0, 2, 0 ) ); // x0 x2 y0 y2
		const __m128 ryz = _mm_shuffle_ps( y, z, _MM_SHUFFLE( 3, 1, 3, 1 ) ); // y1 y3 z1 z3
		const __m128 rzx = _mm_shuffle_ps( z, x, _MM_SHUFFLE( 3, 1, 2, 0 ) ); // z0 z2 x1 x3
		a = _mm_shuffle_ps( rxy, rzx, _MM_SHUFFLE( 2, 0, 2, 0 ) );
		b = _mm_shuffle_ps( ryz, rxy, _MM_SHUFFLE( 3, 1, 2, 0 ) );
		c = _mm_shuffle_ps( rzx, ryz, _MM_SHUFFLE( 3, 1, 3, 1 ) );
	}

	// A bone that contributes nothing, for SoA lanes whose weight in the current slot
	// is zero: their bone index may be garbage and must not be followed.
	alignas( 16 ) inline const float zeroMatrix[16] = {};

} // namespace NDetail

inline void ColsBlend( const SSkinCall &c )
{
	NDetail::PrepareBoneColumns( c.pMatrices, c.nBones, c.pScratch );
	NDetail::AoSLoop( c, 0, NDetail::FColsBlend{ c.pScratch } );
}

inline void ColsXform( const SSkinCall &c )
{
	NDetail::PrepareBoneColumns( c.pMatrices, c.nBones, c.pScratch );
	NDetail::AoSLoop( c, 0, NDetail::FColsXform{ c.pScratch } );
}

// Column form with the fast paths; the best kernel on large parts of small
// skeletons, and the worst on small parts of big ones because of the preparation.
inline void ColsXformFast( const SSkinCall &c )
{
	NDetail::PrepareBoneColumns( c.pMatrices, c.nBones, c.pScratch );
	NDetail::AoSLoop( c, 0, NDetail::FColsXformFast{ c.pScratch } );
}

inline void RowsBlend( const SSkinCall &c )
{
	NDetail::AoSLoop( c, 0, NDetail::FRows{ c.pMatrices } );
}

// The engine candidate for blended parts, GSkinning.h.
inline void RowsBlendFast( const SSkinCall &c )
{
	NSkinning::SkinPositions( c.pSrc, c.pDst, c.pWeights, c.nCount, c.pMatrices );
}

// The remainder of the SoA variants: the vertices from nFrom on, through the
// shipped row kernel.
inline void RowsTail( const SSkinCall &c, int nFrom )
{
	NSkinning::SkinPositions( c.pSrc + 3 * nFrom, c.pDst + 3 * nFrom, c.pWeights + nFrom, c.nCount - nFrom, c.pMatrices );
}

// Variant 5, SSE: four vertices per pass in SoA registers. SSE2 has no gather, so
// each lane's three matrix rows are loaded and transposed into per-element vectors
// (three 4x4 transposes per influence slot per four vertices). Slots where no lane
// has a weight are skipped; lanes with a zero weight read the zero matrix. The
// remainder of fewer than four vertices goes through RowsBlendFast.
inline void SoA4( const SSkinCall &c )
{
	const int nBlocks = c.nCount / 4;
	for ( int blk = 0; blk < nBlocks; ++blk )
	{
		const int k = blk * 4;
		const float *s = c.pSrc + 3 * k;
		__m128 x, y, z;
		NDetail::Deinterleave4( _mm_loadu_ps( s ), _mm_loadu_ps( s + 4 ), _mm_loadu_ps( s + 8 ), x, y, z );
		const SSkinWeights *w = c.pWeights + k;
		__m128 w0 = _mm_loadu_ps( w[0].fWeights );
		__m128 w1 = _mm_loadu_ps( w[1].fWeights );
		__m128 w2 = _mm_loadu_ps( w[2].fWeights );
		__m128 w3 = _mm_loadu_ps( w[3].fWeights );
		// After the transpose wN holds slot N's weight for each of the four vertices.
		_MM_TRANSPOSE4_PS( w0, w1, w2, w3 );
		const __m128 slotWeights[4] = { w0, w1, w2, w3 };
		__m128 ax = _mm_setzero_ps(), ay = _mm_setzero_ps(), az = _mm_setzero_ps();
		for ( int i = 0; i < 4; ++i )
		{
			const int nLanes = _mm_movemask_ps( _mm_cmpneq_ps( slotWeights[i], _mm_setzero_ps() ) );
			if ( nLanes == 0 )
			{
				continue;
			}
			const float *m[4];
			for ( int l = 0; l < 4; ++l )
			{
				m[l] = ( nLanes & ( 1 << l ) ) ? c.pMatrices + 16 * w[l].cBoneIndices[i] : NDetail::zeroMatrix;
			}
			__m128 e[3][4];
			for ( int row = 0; row < 3; ++row )
			{
				__m128 r0 = _mm_loadu_ps( m[0] + 4 * row );
				__m128 r1 = _mm_loadu_ps( m[1] + 4 * row );
				__m128 r2 = _mm_loadu_ps( m[2] + 4 * row );
				__m128 r3 = _mm_loadu_ps( m[3] + 4 * row );
				_MM_TRANSPOSE4_PS( r0, r1, r2, r3 );
				e[row][0] = r0;
				e[row][1] = r1;
				e[row][2] = r2;
				e[row][3] = r3;
			}
			const __m128 wi = slotWeights[i];
			const __m128 rx = _mm_add_ps( _mm_add_ps( _mm_mul_ps( e[0][0], x ), _mm_mul_ps( e[0][1], y ) ),
			                              _mm_add_ps( _mm_mul_ps( e[0][2], z ), e[0][3] ) );
			const __m128 ry = _mm_add_ps( _mm_add_ps( _mm_mul_ps( e[1][0], x ), _mm_mul_ps( e[1][1], y ) ),
			                              _mm_add_ps( _mm_mul_ps( e[1][2], z ), e[1][3] ) );
			const __m128 rz = _mm_add_ps( _mm_add_ps( _mm_mul_ps( e[2][0], x ), _mm_mul_ps( e[2][1], y ) ),
			                              _mm_add_ps( _mm_mul_ps( e[2][2], z ), e[2][3] ) );
			ax = _mm_add_ps( ax, _mm_mul_ps( rx, wi ) );
			ay = _mm_add_ps( ay, _mm_mul_ps( ry, wi ) );
			az = _mm_add_ps( az, _mm_mul_ps( rz, wi ) );
		}
		__m128 a, b, cc;
		NDetail::Interleave4( ax, ay, az, a, b, cc );
		float *d = c.pDst + 3 * k;
		_mm_storeu_ps( d, a );
		_mm_storeu_ps( d + 4, b );
		_mm_storeu_ps( d + 8, cc );
	}
	RowsTail( c, nBlocks * 4 );
}

// Variant 6, SSE: runs of vertices with the same bone set (BuildRuns). Single-bone
// runs keep the bone's columns in registers; unit-weight ones skip the weight
// multiply, which is then exactly SSEBatchTransform. Mixed runs fall back to the
// per-vertex path of GSkinning.h.
inline void RunsSSE( const SSkinCall &c )
{
	NDetail::PrepareBoneColumns( c.pMatrices, c.nBones, c.pScratch );
	const int nLast = c.nCount - 1;
	for ( int r = 0; r < c.nRuns; ++r )
	{
		const SSkinRun &run = c.pRuns[r];
		const int nEnd = run.nStart + run.nCount;
		// The global last vertex is done outside the fast loop with exact loads/stores.
		const int nFastEnd = nEnd > nLast ? nLast : nEnd;
		if ( run.nMask == 1 )
		{
			const SBoneColumns &b = c.pScratch[run.cBones[0]];
			const __m128 c0 = _mm_load_ps( b.c[0] ), c1 = _mm_load_ps( b.c[1] );
			const __m128 c2 = _mm_load_ps( b.c[2] ), c3 = _mm_load_ps( b.c[3] );
			for ( int k = run.nStart; k < nFastEnd; ++k )
			{
				const __m128 p = _mm_loadu_ps( c.pSrc + 3 * k );
				const __m128 xy = _mm_add_ps( _mm_mul_ps( c0, _mm_shuffle_ps( p, p, _MM_SHUFFLE( 0, 0, 0, 0 ) ) ),
				                              _mm_mul_ps( c1, _mm_shuffle_ps( p, p, _MM_SHUFFLE( 1, 1, 1, 1 ) ) ) );
				const __m128 zt = _mm_add_ps( _mm_mul_ps( c2, _mm_shuffle_ps( p, p, _MM_SHUFFLE( 2, 2, 2, 2 ) ) ), c3 );
				__m128 res = _mm_add_ps( xy, zt );
				if ( !run.bUnitWeight )
				{
					res = _mm_mul_ps( res, _mm_set1_ps( c.pWeights[k].fWeights[0] ) );
				}
				_mm_storeu_ps( c.pDst + 3 * k, res );
			}
		}
		else
		{
			for ( int k = run.nStart; k < nFastEnd; ++k )
			{
				_mm_storeu_ps( c.pDst + 3 * k,
				               NDetail::VertexColsXformFast( _mm_loadu_ps( c.pSrc + 3 * k ), c.pWeights[k], c.pScratch ) );
			}
		}
	}
	if ( c.nCount > 0 )
	{
		NDetail::StoreXYZSafe(
				c.pDst + 3 * nLast,
				NDetail::VertexColsXformFast( NDetail::LoadXYZSafe( c.pSrc + 3 * nLast ), c.pWeights[nLast], c.pScratch ) );
	}
}

namespace NDetail
{

	// AVX2 SoA <-> AoS for eight vertices: the four-vertex shuffles in each 128-bit
	// lane, with vertices 0..3 in the low lane and 4..7 in the high one.
	BK2_SKIN_TARGET_AVX2 inline void Deinterleave8( const float *p, __m256 &x, __m256 &y, __m256 &z )
	{
		const __m256 a = _mm256_insertf128_ps( _mm256_castps128_ps256( _mm_loadu_ps( p ) ), _mm_loadu_ps( p + 12 ), 1 );
		const __m256 b = _mm256_insertf128_ps( _mm256_castps128_ps256( _mm_loadu_ps( p + 4 ) ), _mm_loadu_ps( p + 16 ), 1 );
		const __m256 c = _mm256_insertf128_ps( _mm256_castps128_ps256( _mm_loadu_ps( p + 8 ) ), _mm_loadu_ps( p + 20 ), 1 );
		const __m256 xy = _mm256_shuffle_ps( b, c, _MM_SHUFFLE( 2, 1, 3, 2 ) );
		const __m256 yz = _mm256_shuffle_ps( a, b, _MM_SHUFFLE( 1, 0, 2, 1 ) );
		x = _mm256_shuffle_ps( a, xy, _MM_SHUFFLE( 2, 0, 3, 0 ) );
		y = _mm256_shuffle_ps( yz, xy, _MM_SHUFFLE( 3, 1, 2, 0 ) );
		z = _mm256_shuffle_ps( yz, c, _MM_SHUFFLE( 3, 0, 3, 1 ) );
	}

	BK2_SKIN_TARGET_AVX2 inline void Interleave8( float *p, const __m256 &x, const __m256 &y, const __m256 &z )
	{
		const __m256 rxy = _mm256_shuffle_ps( x, y, _MM_SHUFFLE( 2, 0, 2, 0 ) );
		const __m256 ryz = _mm256_shuffle_ps( y, z, _MM_SHUFFLE( 3, 1, 3, 1 ) );
		const __m256 rzx = _mm256_shuffle_ps( z, x, _MM_SHUFFLE( 3, 1, 2, 0 ) );
		const __m256 a = _mm256_shuffle_ps( rxy, rzx, _MM_SHUFFLE( 2, 0, 2, 0 ) );
		const __m256 b = _mm256_shuffle_ps( ryz, rxy, _MM_SHUFFLE( 3, 1, 2, 0 ) );
		const __m256 c = _mm256_shuffle_ps( rzx, ryz, _MM_SHUFFLE( 3, 1, 3, 1 ) );
		_mm_storeu_ps( p, _mm256_castps256_ps128( a ) );
		_mm_storeu_ps( p + 4, _mm256_castps256_ps128( b ) );
		_mm_storeu_ps( p + 8, _mm256_castps256_ps128( c ) );
		_mm_storeu_ps( p + 12, _mm256_extractf128_ps( a, 1 ) );
		_mm_storeu_ps( p + 16, _mm256_extractf128_ps( b, 1 ) );
		_mm_storeu_ps( p + 20, _mm256_extractf128_ps( c, 1 ) );
	}

	// ( e0 x + e1 y ) + ( e2 z + e3 ), eight lanes.
	BK2_SKIN_TARGET_AVX2 inline __m256 Row8( const __m256 &e0,
	                                         const __m256 &e1,
	                                         const __m256 &e2,
	                                         const __m256 &e3,
	                                         const __m256 &x,
	                                         const __m256 &y,
	                                         const __m256 &z )
	{
		return _mm256_add_ps( _mm256_add_ps( _mm256_mul_ps( e0, x ), _mm256_mul_ps( e1, y ) ),
		                      _mm256_add_ps( _mm256_mul_ps( e2, z ), e3 ) );
	}

} // namespace NDetail

// Variant 5, AVX2: eight vertices per pass in SoA registers, each lane's matrix
// elements fetched with masked gathers straight from the SHMatrix array (twelve
// gathers per influence slot per eight vertices; lanes with a zero weight are masked
// off, so their bone index is never followed). Weights and bone indices are gathered
// out of the 24-byte weight records too. Remainder through RowsBlendFast.
BK2_SKIN_TARGET_AVX2 inline void AVX2Gather( const SSkinCall &c )
{
	const int nBlocks = c.nCount / 8;
	// Lane l of a block reads record l: 24 bytes apart, so six floats.
	const __m256i recordFloats = _mm256_setr_epi32( 0, 6, 12, 18, 24, 30, 36, 42 );
	const __m256 zero = _mm256_setzero_ps();
	for ( int blk = 0; blk < nBlocks; ++blk )
	{
		const int k = blk * 8;
		__m256 x, y, z;
		NDetail::Deinterleave8( c.pSrc + 3 * k, x, y, z );
		const float *pRec = reinterpret_cast<const float *>( c.pWeights + k );
		// The four bone index bytes sit at byte 20 of each record, float slot 5.
		const __m256i bones = _mm256_castps_si256( _mm256_i32gather_ps( pRec + 5, recordFloats, 4 ) );
		__m256 ax = zero, ay = zero, az = zero;
		for ( int i = 0; i < 4; ++i )
		{
			const __m256 w = _mm256_i32gather_ps( pRec + i, recordFloats, 4 );
			const __m256 live = _mm256_cmp_ps( w, zero, _CMP_NEQ_OQ );
			if ( _mm256_movemask_ps( live ) == 0 )
			{
				continue;
			}
			const __m256i bone = _mm256_and_si256( _mm256_srli_epi32( bones, 8 * i ), _mm256_set1_epi32( 0xFF ) );
			const __m256i base = _mm256_slli_epi32( bone, 4 );
			const float *m = c.pMatrices;
			__m256 e[12];
			for ( int el = 0; el < 12; ++el )
			{
				e[el] = _mm256_mask_i32gather_ps( zero, m + el, base, live, 4 );
			}
			const __m256 rx = NDetail::Row8( e[0], e[1], e[2], e[3], x, y, z );
			const __m256 ry = NDetail::Row8( e[4], e[5], e[6], e[7], x, y, z );
			const __m256 rz = NDetail::Row8( e[8], e[9], e[10], e[11], x, y, z );
			ax = _mm256_add_ps( ax, _mm256_mul_ps( rx, w ) );
			ay = _mm256_add_ps( ay, _mm256_mul_ps( ry, w ) );
			az = _mm256_add_ps( az, _mm256_mul_ps( rz, w ) );
		}
		NDetail::Interleave8( c.pDst + 3 * k, ax, ay, az );
	}
	RowsTail( c, nBlocks * 8 );
}

// Variant 6, AVX2: as RunsSSE, but single-bone runs go eight vertices at a time in
// SoA with the bone's twelve elements broadcast once per run.
BK2_SKIN_TARGET_AVX2 inline void RunsAVX2( const SSkinCall &c )
{
	NDetail::PrepareBoneColumns( c.pMatrices, c.nBones, c.pScratch );
	const int nLast = c.nCount - 1;
	for ( int r = 0; r < c.nRuns; ++r )
	{
		const SSkinRun &run = c.pRuns[r];
		const int nEnd = run.nStart + run.nCount;
		const int nFastEnd = nEnd > nLast ? nLast : nEnd;
		int k = run.nStart;
		if ( run.nMask == 1 )
		{
			const float *m = c.pMatrices + 16 * run.cBones[0];
			const __m256 e0 = _mm256_broadcast_ss( m + 0 ), e1 = _mm256_broadcast_ss( m + 1 );
			const __m256 e2 = _mm256_broadcast_ss( m + 2 ), e3 = _mm256_broadcast_ss( m + 3 );
			const __m256 e4 = _mm256_broadcast_ss( m + 4 ), e5 = _mm256_broadcast_ss( m + 5 );
			const __m256 e6 = _mm256_broadcast_ss( m + 6 ), e7 = _mm256_broadcast_ss( m + 7 );
			const __m256 e8 = _mm256_broadcast_ss( m + 8 ), e9 = _mm256_broadcast_ss( m + 9 );
			const __m256 e10 = _mm256_broadcast_ss( m + 10 ), e11 = _mm256_broadcast_ss( m + 11 );
			// Eight-vertex blocks read and write exactly 24 floats, so they may run up
			// to nEnd, not just nFastEnd.
			for ( ; k + 8 <= nEnd; k += 8 )
			{
				__m256 x, y, z;
				NDetail::Deinterleave8( c.pSrc + 3 * k, x, y, z );
				__m256 rx = NDetail::Row8( e0, e1, e2, e3, x, y, z );
				__m256 ry = NDetail::Row8( e4, e5, e6, e7, x, y, z );
				__m256 rz = NDetail::Row8( e8, e9, e10, e11, x, y, z );
				if ( !run.bUnitWeight )
				{
					const float *pRec = reinterpret_cast<const float *>( c.pWeights + k );
					const __m256 w = _mm256_i32gather_ps( pRec, _mm256_setr_epi32( 0, 6, 12, 18, 24, 30, 36, 42 ), 4 );
					rx = _mm256_mul_ps( rx, w );
					ry = _mm256_mul_ps( ry, w );
					rz = _mm256_mul_ps( rz, w );
				}
				NDetail::Interleave8( c.pDst + 3 * k, rx, ry, rz );
			}
			// Before the per-vertex helpers were forced inline, GCC called a legacy-SSE
			// out-of-line copy from here with the upper YMM halves dirty and no
			// vzeroupper: 130 ns per vertex instead of 5 on Alder Lake. Any AVX2 code
			// that falls back to SSE2 code in the engine needs the same care.
			_mm256_zeroupper();
		}
		for ( ; k < nFastEnd; ++k )
		{
			_mm_storeu_ps( c.pDst + 3 * k,
			               NDetail::VertexColsXformFast( _mm_loadu_ps( c.pSrc + 3 * k ), c.pWeights[k], c.pScratch ) );
		}
		if ( k == nLast && nEnd == c.nCount )
		{
			NDetail::StoreXYZSafe(
					c.pDst + 3 * nLast,
					NDetail::VertexColsXformFast( NDetail::LoadXYZSafe( c.pSrc + 3 * nLast ), c.pWeights[nLast], c.pScratch ) );
		}
	}
}

#endif // BK2_SKINNING_SSE2

// Rigid parts: one matrix for the whole call (see TransformPositions in GSkinning.h).
// pMin/pMax are only written by the Bounds variants.
struct SRigidCall
{
	const float *pSrc;
	float *pDst;
	int nCount;
	const float *pMatrix;
	float *pMin;
	float *pMax;
};

// SHMatrix::RotateHVector per vertex, which is the engine's non-SSE path.
inline void RigidScalar( const SRigidCall &c )
{
	const float *m = c.pMatrix;
	for ( int k = 0; k < c.nCount; ++k )
	{
		const float x = c.pSrc[3 * k + 0], y = c.pSrc[3 * k + 1], z = c.pSrc[3 * k + 2];
		c.pDst[3 * k + 0] = m[0] * x + m[1] * y + m[2] * z + m[3];
		c.pDst[3 * k + 1] = m[4] * x + m[5] * y + m[6] * z + m[7];
		c.pDst[3 * k + 2] = m[8] * x + m[9] * y + m[10] * z + m[11];
	}
}

// The bounding box loop XFormPosition runs after the transform, for the two-pass
// baseline the fused variants are compared with.
inline void ScalarBounds( const float *pPos, int nCount, float *pMin, float *pMax )
{
	for ( int k = 0; k < nCount; ++k )
	{
		for ( int j = 0; j < 3; ++j )
		{
			const float v = pPos[3 * k + j];
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

#if BK2_SKINNING_SSE2

inline void RigidSSE( const SRigidCall &c )
{
	NSkinning::TransformPositions( c.pSrc, c.pDst, c.nCount, c.pMatrix );
}

// Two passes, as the engine would do with TransformPositions and its existing loop.
inline void RigidSSEThenBounds( const SRigidCall &c )
{
	NSkinning::TransformPositions( c.pSrc, c.pDst, c.nCount, c.pMatrix );
	ScalarBounds( c.pDst, c.nCount, c.pMin, c.pMax );
}

inline void RigidSSEBounds( const SRigidCall &c )
{
	NSkinning::TransformPositionsBounds( c.pSrc, c.pDst, c.nCount, c.pMatrix, c.pMin, c.pMax );
}

// Four vertices per pass in SoA registers, the twelve matrix elements broadcast once
// per call; remainder through TransformPositions.
inline void RigidSoA4( const SRigidCall &c )
{
	const float *m = c.pMatrix;
	__m128 e[12];
	for ( int i = 0; i < 12; ++i )
	{
		e[i] = _mm_set1_ps( m[i] );
	}
	const int nBlocks = c.nCount / 4;
	for ( int blk = 0; blk < nBlocks; ++blk )
	{
		const float *s = c.pSrc + 12 * blk;
		__m128 x, y, z;
		NDetail::Deinterleave4( _mm_loadu_ps( s ), _mm_loadu_ps( s + 4 ), _mm_loadu_ps( s + 8 ), x, y, z );
		const __m128 rx = _mm_add_ps( _mm_add_ps( _mm_mul_ps( e[0], x ), _mm_mul_ps( e[1], y ) ),
		                              _mm_add_ps( _mm_mul_ps( e[2], z ), e[3] ) );
		const __m128 ry = _mm_add_ps( _mm_add_ps( _mm_mul_ps( e[4], x ), _mm_mul_ps( e[5], y ) ),
		                              _mm_add_ps( _mm_mul_ps( e[6], z ), e[7] ) );
		const __m128 rz = _mm_add_ps( _mm_add_ps( _mm_mul_ps( e[8], x ), _mm_mul_ps( e[9], y ) ),
		                              _mm_add_ps( _mm_mul_ps( e[10], z ), e[11] ) );
		__m128 a, b, cc;
		NDetail::Interleave4( rx, ry, rz, a, b, cc );
		float *d = c.pDst + 12 * blk;
		_mm_storeu_ps( d, a );
		_mm_storeu_ps( d + 4, b );
		_mm_storeu_ps( d + 8, cc );
	}
	const int nDone = nBlocks * 4;
	NSkinning::TransformPositions( c.pSrc + 3 * nDone, c.pDst + 3 * nDone, c.nCount - nDone, m );
}

// Eight vertices per pass in SoA registers; remainder through TransformPositions.
// With bBounds the box is kept per lane in SoA and reduced once at the end.
template <bool bBounds>
BK2_SKIN_TARGET_AVX2 inline void RigidAVX2T( const SRigidCall &c )
{
	const float *m = c.pMatrix;
	const __m256 e0 = _mm256_broadcast_ss( m + 0 ), e1 = _mm256_broadcast_ss( m + 1 );
	const __m256 e2 = _mm256_broadcast_ss( m + 2 ), e3 = _mm256_broadcast_ss( m + 3 );
	const __m256 e4 = _mm256_broadcast_ss( m + 4 ), e5 = _mm256_broadcast_ss( m + 5 );
	const __m256 e6 = _mm256_broadcast_ss( m + 6 ), e7 = _mm256_broadcast_ss( m + 7 );
	const __m256 e8 = _mm256_broadcast_ss( m + 8 ), e9 = _mm256_broadcast_ss( m + 9 );
	const __m256 e10 = _mm256_broadcast_ss( m + 10 ), e11 = _mm256_broadcast_ss( m + 11 );
	const __m256 big = _mm256_set1_ps( 3.402823466e+38f );
	__m256 mnx = big, mny = big, mnz = big;
	__m256 mxx = _mm256_sub_ps( _mm256_setzero_ps(), big ), mxy = mxx, mxz = mxx;
	const int nBlocks = c.nCount / 8;
	for ( int blk = 0; blk < nBlocks; ++blk )
	{
		__m256 x, y, z;
		NDetail::Deinterleave8( c.pSrc + 24 * blk, x, y, z );
		const __m256 rx = NDetail::Row8( e0, e1, e2, e3, x, y, z );
		const __m256 ry = NDetail::Row8( e4, e5, e6, e7, x, y, z );
		const __m256 rz = NDetail::Row8( e8, e9, e10, e11, x, y, z );
		NDetail::Interleave8( c.pDst + 24 * blk, rx, ry, rz );
		if ( bBounds )
		{
			mnx = _mm256_min_ps( mnx, rx );
			mny = _mm256_min_ps( mny, ry );
			mnz = _mm256_min_ps( mnz, rz );
			mxx = _mm256_max_ps( mxx, rx );
			mxy = _mm256_max_ps( mxy, ry );
			mxz = _mm256_max_ps( mxz, rz );
		}
	}
	const int nDone = nBlocks * 8;
	if ( !bBounds )
	{
		NSkinning::TransformPositions( c.pSrc + 3 * nDone, c.pDst + 3 * nDone, c.nCount - nDone, m );
		return;
	}
	if ( c.nCount <= 0 )
	{
		return;
	}
	float lanes[6][8];
	_mm256_storeu_ps( lanes[0], mnx );
	_mm256_storeu_ps( lanes[1], mny );
	_mm256_storeu_ps( lanes[2], mnz );
	_mm256_storeu_ps( lanes[3], mxx );
	_mm256_storeu_ps( lanes[4], mxy );
	_mm256_storeu_ps( lanes[5], mxz );
	float vMin[3] = { lanes[0][0], lanes[1][0], lanes[2][0] };
	float vMax[3] = { lanes[3][0], lanes[4][0], lanes[5][0] };
	for ( int l = 1; l < 8; ++l )
	{
		for ( int j = 0; j < 3; ++j )
		{
			vMin[j] = lanes[j][l] < vMin[j] ? lanes[j][l] : vMin[j];
			vMax[j] = lanes[3 + j][l] > vMax[j] ? lanes[3 + j][l] : vMax[j];
		}
	}
	if ( nDone < c.nCount )
	{
		float tMin[3], tMax[3];
		NSkinning::TransformPositionsBounds( c.pSrc + 3 * nDone, c.pDst + 3 * nDone, c.nCount - nDone, m, tMin, tMax );
		for ( int j = 0; j < 3; ++j )
		{
			vMin[j] = tMin[j] < vMin[j] ? tMin[j] : vMin[j];
			vMax[j] = tMax[j] > vMax[j] ? tMax[j] : vMax[j];
		}
	}
	for ( int j = 0; j < 3; ++j )
	{
		c.pMin[j] = vMin[j];
		c.pMax[j] = vMax[j];
	}
}

BK2_SKIN_TARGET_AVX2 inline void RigidAVX2( const SRigidCall &c )
{
	RigidAVX2T<false>( c );
}

BK2_SKIN_TARGET_AVX2 inline void RigidAVX2Bounds( const SRigidCall &c )
{
	RigidAVX2T<true>( c );
}

#endif // BK2_SKINNING_SSE2

} // namespace NSkinningVariants
