#include "3Dmotor/stdafx.h"

#include "3DLib/GGeometryUtil.h"

#include <algorithm>
#include <map>
#include <random>
#include <set>
#include <vector>

#include <gtest/gtest.h>

// CTriVertexCacheOptimizer reorders a mesh's triangles so that the GPU's
// post-transform vertex cache hits more often, and renumbers the vertices to
// match. It is 24 per cent of the time a map spends loading when the map
// instantiates every model in the database, and its fallback search is
// quadratic, so it is going to be rewritten.
//
// These tests exist to make that rewrite safe. They pin two different kinds of
// thing, and the difference matters:
//
//   Validity. The output has to be a permutation of the input, the vertex
//   renumbering has to be a permutation too, and the reported vertex count has
//   to be the number actually referenced. A rewrite that breaks any of these
//   corrupts geometry, and nothing downstream checks.
//
//   Quality. The whole point is cache misses, so the tests measure them. An
//   optimisation that made the optimiser fast and worse at its job would
//   otherwise pass everything and be a silent regression in frame rate.
//
// There are also exact-output tests. Those pin today's choices, including how
// ties are broken, which is what lets a rewrite claim to be a pure speed change
// rather than a different algorithm that happens to look similar.
namespace
{
using NGScene::CTriVertexCacheOptimizer;
using NGScene::SVxCache;

// What the renderer uses unless the device reports otherwise: GGeometry.cpp has
// nVCacheSize = 10, replaced at startup by what D3D says the hardware has.
const int VCACHE = 10;

STriangle Tri( int a, int b, int c )
{
	STriangle t;
	t.i1 = static_cast<uint16_t>( a );
	t.i2 = static_cast<uint16_t>( b );
	t.i3 = static_cast<uint16_t>( c );
	return t;
}

bool SameTriangle( const STriangle &a, const STriangle &b )
{
	return a.i1 == b.i1 && a.i2 == b.i2 && a.i3 == b.i3;
}

//! A grid of quads, each split into two triangles. The friendly case: every
//! triangle shares an edge with its neighbour, so a good ordering is close to a
//! strip and the cache should hit most of the time.
std::vector<STriangle> MakeGrid( int nW, int nH )
{
	std::vector<STriangle> tris;
	const int nStride = nW + 1;
	for ( int y = 0; y < nH; ++y )
	{
		for ( int x = 0; x < nW; ++x )
		{
			tris.push_back( Tri( y * nStride + x, y * nStride + x + 1, ( y + 1 ) * nStride + x ) );
			tris.push_back( Tri( y * nStride + x + 1, ( y + 1 ) * nStride + x, ( y + 1 ) * nStride + x + 1 ) );
		}
	}
	return tris;
}

//! Many small pieces sharing no vertices. This is the shape that makes the
//! current fallback fire: once a piece is exhausted there is nothing in the
//! cache to continue from, so the search starts over across every remaining
//! triangle. Vehicle models are full of separate parts, which is why it matters.
std::vector<STriangle> MakeDisconnectedPieces( int nPieces, int nTrisEach )
{
	std::vector<STriangle> tris;
	int nBase = 0;
	for ( int p = 0; p < nPieces; ++p )
	{
		for ( int k = 0; k < nTrisEach; ++k )
		{
			tris.push_back( Tri( nBase, nBase + k + 1, nBase + k + 2 ) );
		}
		nBase += nTrisEach + 2;
	}
	return tris;
}

//! A fan: one vertex shared by everything. Drives the high-valence paths,
//! CountNotCachedFL in particular, which walks every triangle on a vertex.
std::vector<STriangle> MakeFan( int nTris )
{
	std::vector<STriangle> tris;
	for ( int k = 0; k < nTris; ++k )
	{
		tris.push_back( Tri( 0, k + 1, k + 2 ) );
	}
	return tris;
}

//! Cache misses for an ordering, counted the way the hardware would: a vertex
//! already in the post-transform cache is free, anything else is a fetch.
//! CTriVertexCacheOptimizer::MeasureEfficiency counts exactly this, with the
//! same SVxCache, but it only writes to the trace.
int CountMisses( const std::vector<STriangle> &tris )
{
	SVxCache<VCACHE> cache;
	int nMisses = 0;
	for ( size_t k = 0; k < tris.size(); ++k )
	{
		const uint16_t idx[3] = { tris[k].i1, tris[k].i2, tris[k].i3 };
		for ( int i = 0; i < 3; ++i )
		{
			if ( !cache.IsIn( idx[i] ) )
			{
				++nMisses;
			}
			cache.Push( idx[i] );
		}
	}
	return nMisses;
}

//! Average cache misses per triangle. 3.0 means nothing is ever reused, 0.5 is
//! about the best a closed mesh can do.
double ACMR( const std::vector<STriangle> &tris )
{
	return tris.empty() ? 0.0 : double( CountMisses( tris ) ) / double( tris.size() );
}

int DistinctVertices( const std::vector<STriangle> &tris )
{
	std::set<uint16_t> seen;
	for ( size_t k = 0; k < tris.size(); ++k )
	{
		seen.insert( tris[k].i1 );
		seen.insert( tris[k].i2 );
		seen.insert( tris[k].i3 );
	}
	return static_cast<int>( seen.size() );
}

//! The highest index any triangle names, plus one. CountVertices derives the
//! vertex count this way, so a mesh skipping indices still has them in its
//! renumbering.
int VertexSpan( const std::vector<STriangle> &tris )
{
	int nMax = -1;
	for ( size_t k = 0; k < tris.size(); ++k )
	{
		nMax = (std::max)( nMax, (int)(std::max)( tris[k].i1, (std::max)( tris[k].i2, tris[k].i3 ) ) );
	}
	return nMax + 1;
}

struct SResult
{
	std::vector<STriangle> tris;
	std::vector<uint16_t> reorder;
	int nResVerts = 0;
};

SResult Optimise( const std::vector<STriangle> &input, int nVCacheSize = VCACHE )
{
	SResult r;
	r.tris = input;
	CTriVertexCacheOptimizer opt;
	opt.Optimize( &r.tris, &r.reorder, &r.nResVerts, nVCacheSize );
	return r;
}

//! Everything that has to be true of any output, whatever the algorithm inside.
//! Called by every test rather than restated, so a rewrite that breaks one of
//! them fails everywhere rather than in one forgotten case.
void ExpectValid( const std::vector<STriangle> &input, const SResult &r )
{
	// The same triangles, reordered. Counted as a multiset, since a mesh may
	// legitimately contain the same triangle twice.
	ASSERT_EQ( r.tris.size(), input.size() );
	std::map<std::vector<uint16_t>, int> before, after;
	for ( size_t k = 0; k < input.size(); ++k )
	{
		++before[{ input[k].i1, input[k].i2, input[k].i3 }];
		++after[{ r.tris[k].i1, r.tris[k].i2, r.tris[k].i3 }];
	}
	EXPECT_EQ( before, after ) << "output is not a permutation of the input";

	// The renumbering covers every vertex exactly once. FilterTrinagles indexes
	// the vertex buffer through it straight after Optimize returns, so a hole or
	// a duplicate is a corrupt mesh rather than a slow one. 0xffff is the
	// "unassigned" marker and must not survive.
	const int nSpan = VertexSpan( input );
	ASSERT_EQ( static_cast<int>( r.reorder.size() ), nSpan );
	std::vector<int> seen( nSpan, 0 );
	for ( size_t k = 0; k < r.reorder.size(); ++k )
	{
		ASSERT_NE( r.reorder[k], 0xffff ) << "vertex " << k << " was left unassigned";
		ASSERT_LT( r.reorder[k], nSpan ) << "vertex " << k << " renumbered out of range";
		++seen[r.reorder[k]];
	}
	for ( int k = 0; k < nSpan; ++k )
	{
		EXPECT_EQ( seen[k], 1 ) << "new index " << k << " used " << seen[k] << " times";
	}

	// The reported count is what the caller sizes the vertex buffer from, and
	// GGeometry.cpp asserts it is no larger than the source mesh.
	EXPECT_EQ( r.nResVerts, DistinctVertices( input ) );
	EXPECT_LE( r.nResVerts, nSpan );
}
} // namespace

// Validity, across every shape that reaches a different path inside.

TEST( TriVertexCacheOptimizer, EmptyMesh )
{
	const std::vector<STriangle> input;
	SResult r = Optimise( input );
	EXPECT_TRUE( r.tris.empty() );
	EXPECT_EQ( r.nResVerts, 0 );
}

TEST( TriVertexCacheOptimizer, SingleTriangleTakesTheShortPath )
{
	// Below nVCacheSize / 3 the whole search is skipped and only the renumbering
	// runs, which is a path of its own.
	const std::vector<STriangle> input = { Tri( 7, 8, 9 ) };
	SResult r = Optimise( input );
	ExpectValid( input, r );
	EXPECT_TRUE( SameTriangle( r.tris[0], input[0] ) ) << "the short path must not reorder";
}

TEST( TriVertexCacheOptimizer, ExactlyAtTheShortPathBoundary )
{
	// size() <= nVCacheSize / 3 is the test, so with a cache of 10 that is 3
	// triangles taking the short path and 4 taking the long one.
	const std::vector<STriangle> three = { Tri( 0, 1, 2 ), Tri( 1, 2, 3 ), Tri( 2, 3, 4 ) };
	SResult r3 = Optimise( three );
	ExpectValid( three, r3 );
	for ( size_t k = 0; k < three.size(); ++k )
	{
		EXPECT_TRUE( SameTriangle( r3.tris[k], three[k] ) ) << "at " << k;
	}

	std::vector<STriangle> four = three;
	four.push_back( Tri( 3, 4, 5 ) );
	SResult r4 = Optimise( four );
	ExpectValid( four, r4 );
}

TEST( TriVertexCacheOptimizer, Grid )
{
	const std::vector<STriangle> input = MakeGrid( 8, 8 );
	SResult r = Optimise( input );
	ExpectValid( input, r );
}

TEST( TriVertexCacheOptimizer, DisconnectedPieces )
{
	// The shape that makes the fallback search fire over and over.
	const std::vector<STriangle> input = MakeDisconnectedPieces( 12, 4 );
	SResult r = Optimise( input );
	ExpectValid( input, r );
}

TEST( TriVertexCacheOptimizer, Fan )
{
	const std::vector<STriangle> input = MakeFan( 40 );
	SResult r = Optimise( input );
	ExpectValid( input, r );
}

TEST( TriVertexCacheOptimizer, DegenerateTrianglesSurvive )
{
	// Repeated indices inside a triangle. These exist in shipped meshes and the
	// optimiser must pass them through rather than drop or rewrite them.
	std::vector<STriangle> input = MakeGrid( 3, 3 );
	input.push_back( Tri( 0, 0, 0 ) );
	input.push_back( Tri( 5, 5, 6 ) );
	SResult r = Optimise( input );
	ExpectValid( input, r );
}

TEST( TriVertexCacheOptimizer, UnreferencedVerticesStillGetAnIndex )
{
	// Indices with gaps: vertex 1, 2 and 4 are never named, but the renumbering
	// is sized by the highest index and OptimizeVertexOrder fills the holes at
	// the end. The caller indexes it by the original vertex number, so a hole
	// would read 0xffff out of the vertex buffer.
	const std::vector<STriangle> input = {
		Tri( 0, 3, 5 ), Tri( 3, 5, 6 ), Tri( 5, 6, 7 ), Tri( 6, 7, 8 ), Tri( 0, 7, 8 ) };
	SResult r = Optimise( input );
	ExpectValid( input, r );
	// 0, 3, 5, 6, 7 and 8 are referenced; 1, 2 and 4 are not, and are pushed to
	// the end of the renumbering rather than left as 0xffff.
	EXPECT_EQ( r.nResVerts, 6 );
	EXPECT_EQ( r.reorder.size(), 9u );
}

TEST( TriVertexCacheOptimizer, RandomMeshes )
{
	// Shapes nobody thought to write down, with a fixed seed so a failure can be
	// reproduced.
	std::mt19937 rng( 20261027 );
	for ( int nCase = 0; nCase < 20; ++nCase )
	{
		const int nVerts = 6 + (int)( rng() % 60 );
		const int nTris = 4 + (int)( rng() % 120 );
		std::vector<STriangle> input;
		for ( int k = 0; k < nTris; ++k )
		{
			input.push_back( Tri( rng() % nVerts, rng() % nVerts, rng() % nVerts ) );
		}
		SResult r = Optimise( input );
		ASSERT_NO_FATAL_FAILURE( ExpectValid( input, r ) ) << "case " << nCase;
	}
}

TEST( TriVertexCacheOptimizer, VariousCacheSizes )
{
	// nVCacheSize comes from the device at startup, so it is not always 10.
	const std::vector<STriangle> input = MakeGrid( 6, 6 );
	for ( int nCache : { 4, 8, 10, 12, 16, 24, 32 } )
	{
		SResult r = Optimise( input, nCache );
		ASSERT_NO_FATAL_FAILURE( ExpectValid( input, r ) ) << "cache size " << nCache;
	}
}

// Quality. What the optimiser is for.

TEST( TriVertexCacheOptimizer, GridGetsFewerCacheMisses )
{
	const std::vector<STriangle> input = MakeGrid( 10, 10 );
	SResult r = Optimise( input );
	ExpectValid( input, r );

	const double fBefore = ACMR( input );
	const double fAfter = ACMR( r.tris );
	EXPECT_LT( fAfter, fBefore ) << "before " << fBefore << ", after " << fAfter;
	// Recorded from today's output. A closed grid should reuse nearly every
	// vertex; this is the bound a rewrite must not drift past, and it is loose
	// enough that a different but equally good tie-break still passes.
	EXPECT_LT( fAfter, 1.0 ) << "cache misses per triangle regressed to " << fAfter;
}

TEST( TriVertexCacheOptimizer, FanGetsFewerCacheMisses )
{
	const std::vector<STriangle> input = MakeFan( 60 );
	SResult r = Optimise( input );
	ExpectValid( input, r );
	EXPECT_LE( ACMR( r.tris ), ACMR( input ) );
}

TEST( TriVertexCacheOptimizer, DisconnectedPiecesAreNotMadeWorse )
{
	// Separate pieces cannot share vertices, so there is little to win; the test
	// is that the optimiser does not shuffle them into something worse than the
	// order it was given.
	const std::vector<STriangle> input = MakeDisconnectedPieces( 10, 5 );
	SResult r = Optimise( input );
	ExpectValid( input, r );
	EXPECT_LE( ACMR( r.tris ), ACMR( input ) );
}

// Determinism and exact output. These pin the choices, tie-breaks included, so
// that a rewrite can show it is the same algorithm run faster.

TEST( TriVertexCacheOptimizer, IsDeterministic )
{
	const std::vector<STriangle> input = MakeGrid( 7, 5 );
	SResult a = Optimise( input );
	SResult b = Optimise( input );
	ASSERT_EQ( a.tris.size(), b.tris.size() );
	for ( size_t k = 0; k < a.tris.size(); ++k )
	{
		EXPECT_TRUE( SameTriangle( a.tris[k], b.tris[k] ) ) << "triangle " << k;
	}
	EXPECT_EQ( a.reorder, b.reorder );
	EXPECT_EQ( a.nResVerts, b.nResVerts );
}

TEST( TriVertexCacheOptimizer, ExactOutputForASmallGrid )
{
	// Golden. Not a statement that this order is the only good one, but that it
	// is the order today's code produces: the fallback picks the unused triangle
	// with the fewest free links and the lowest index on a tie, and a rewrite
	// that keeps that rule reproduces this exactly.
	const std::vector<STriangle> input = MakeGrid( 2, 2 );
	SResult r = Optimise( input );
	ExpectValid( input, r );

	std::vector<std::vector<int>> got;
	for ( size_t k = 0; k < r.tris.size(); ++k )
	{
		got.push_back( { r.tris[k].i1, r.tris[k].i2, r.tris[k].i3 } );
	}
	const std::vector<std::vector<int>> expected = {
		{ 0, 1, 3 }, { 1, 2, 4 }, { 1, 3, 4 }, { 3, 4, 6 }, { 2, 4, 5 }, { 4, 6, 7 }, { 4, 5, 7 }, { 5, 7, 8 } };
	EXPECT_EQ( got, expected );
}

TEST( TriVertexCacheOptimizer, ExactOutputForDisconnectedPieces )
{
	// Golden for the fallback path specifically, which is the one being
	// rewritten.
	const std::vector<STriangle> input = MakeDisconnectedPieces( 3, 3 );
	SResult r = Optimise( input );
	ExpectValid( input, r );

	std::vector<std::vector<int>> got;
	for ( size_t k = 0; k < r.tris.size(); ++k )
	{
		got.push_back( { r.tris[k].i1, r.tris[k].i2, r.tris[k].i3 } );
	}
	const std::vector<std::vector<int>> expected = { { 0, 1, 2 },
	                                                 { 0, 3, 4 },
	                                                 { 0, 2, 3 },
	                                                 { 5, 6, 7 },
	                                                 { 5, 8, 9 },
	                                                 { 5, 7, 8 },
	                                                 { 10, 11, 12 },
	                                                 { 10, 13, 14 },
	                                                 { 10, 12, 13 } };
	EXPECT_EQ( got, expected );
}
