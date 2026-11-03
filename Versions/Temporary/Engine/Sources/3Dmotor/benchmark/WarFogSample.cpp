// Compares the fused war fog kernels of GWarFogSample.h with the shipped path, which
// GLightPerVertex.cpp's SampleWarFog runs as pSampleWarFogCoords followed by one
// pSampleWarFogInt per grid ("Shipped/SSE2", "Shipped/AVX2"; their coordinate buffer
// is preallocated here, so the shipped numbers are slightly flattered). One benchmark
// iteration is one render part: nCount positions sampled from one grid (the fog
// changed on one side only) or from two (both changed, or the cache was cleared,
// which is what a moving part does every frame).
//
// Parts are realistic: positions within a box of 0.5 to 6 world units (a Tiger's hull
// is about 3.8 by 6.2, a road wheel 0.8) around a random point of a 256-cell map, in
// random order, sampled from 257 by 257 grids (a 16 by 16 patch map; 2.75 world
// units per cell). Sizes are positions per part: the census median is 31, p90 243.
//
// Hot  - one part, called over and over: positions, results and grids in L1/L2.
// Cold - consecutive iterations walk through 64 MB of distinct parts and through
//        eight grid pairs (1 MB), so positions arrive from memory and the grid
//        lines mostly from L2/L3, about what a frame of dozens of tanks sees.
//
// These are microbenchmark numbers. One once promised 6.9x and delivered 1.5% in the
// game, so measure the game before believing any of them.

#include "3Dmotor/stdafx.h"

// IsAVX2Present() and the shipped kernel sets come with the lighting kernel sources
// the CMake target adds; GSSEtransform.cpp supplies the nCubicRoot they reference.
#include "3Dmotor/GLightPerVertexKernels.h"
#include "3Dmotor/GSSEtransform.cpp"
#include "3Dmotor/GSSEtransform.h"
#include "3Dmotor/GWarFogSample.h"

#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

namespace
{

const size_t N_COLD_BYTES = size_t( 64 ) << 20;
const int N_GRID = 257;
const int N_COLD_GRID_PAIRS = 8;
const float F_SCALE = 1.0f / 2.75f;
const float F_CELL = 2.75f;

enum EVariant
{
	V_SHIPPED_SSE2,
	V_SHIPPED_AVX2,
	V_SCALAR,
	V_SSE2,
	V_AVX2_GATHER,
	V_AVX2_INSERT,
	V_AVX2_BLOCK_CHECK,
	V_AVX2_UNIFORM, // FindUniformFog first, AVX2 gather kernel when it fails
	V_COUNT
};

// What the grids hold, for the uniform shortcut.
enum EGridKind
{
	G_RANDOM,  // random bytes: the check fails on its second byte, its cheapest miss
	G_UNIFORM, // all 255: the check always hits
	G_SPARSE,  // 255 with one byte in 64 at 254: hits for most parts, late misses for some
};

const char *VariantName( int nVariant )
{
	static const char *names[V_COUNT] = { "Shipped/SSE2",
	                                      "Shipped/AVX2",
	                                      "Fused/Scalar",
	                                      "Fused/SSE2",
	                                      "Fused/AVX2Gather",
	                                      "Fused/AVX2Insert",
	                                      "Fused/AVX2BlockCheck",
	                                      "Fused/AVX2Uniform" };
	return names[nVariant];
}

bool NeedsAVX2( int nVariant )
{
	return nVariant == V_SHIPPED_AVX2 || nVariant >= V_AVX2_GATHER;
}

// Positions of a part with the given extent, in random order, around a random
// point of the map.
void MakePart( std::mt19937 &rng, float *pPos, int nCount, float fExtent )
{
	const float fMap = ( N_GRID - 1 ) * F_CELL;
	const float fCX = std::uniform_real_distribution<float>( 4.0f, fMap - 4.0f )( rng );
	const float fCY = std::uniform_real_distribution<float>( 4.0f, fMap - 4.0f )( rng );
	std::uniform_real_distribution<float> offset( -0.5f * fExtent, 0.5f * fExtent );
	for ( int k = 0; k < nCount; ++k )
	{
		pPos[3 * k] = fCX + offset( rng );
		pPos[3 * k + 1] = fCY + offset( rng );
		pPos[3 * k + 2] = offset( rng );
	}
}

struct SData
{
	int nCount = 0;
	int nParts = 0;
	std::vector<float> pos;
	std::vector<uint8_t> resA, resB;
	std::vector<int> coords;
	std::vector<std::vector<uint8_t>> grids; // pairs: 2i is A, 2i + 1 is B
};

void MakeData( SData &d, int nCount, bool bCold, int nGridKind )
{
	std::mt19937 rng( 0x5eed + nCount );
	d.nCount = nCount;
	const size_t nPartBytes = size_t( nCount ) * ( 12 + 2 );
	d.nParts = bCold ? static_cast<int>( N_COLD_BYTES / nPartBytes ) + 1 : 1;
	d.pos.resize( size_t( d.nParts ) * nCount * 3 );
	d.resA.resize( size_t( d.nParts ) * nCount );
	d.resB.resize( size_t( d.nParts ) * nCount );
	d.coords.resize( 2 * size_t( nCount ) );
	std::uniform_real_distribution<float> extent( 0.5f, 6.0f );
	for ( int p = 0; p < d.nParts; ++p )
	{
		MakePart( rng, &d.pos[size_t( p ) * nCount * 3], nCount, extent( rng ) );
	}
	const int nGrids = bCold ? 2 * N_COLD_GRID_PAIRS : 2;
	d.grids.resize( nGrids );
	for ( std::vector<uint8_t> &g : d.grids )
	{
		g.resize( N_GRID * N_GRID );
		for ( uint8_t &b : g )
		{
			if ( nGridKind == G_RANDOM )
			{
				b = static_cast<uint8_t>( rng() );
			}
			else if ( nGridKind == G_UNIFORM )
			{
				b = 255;
			}
			else
			{
				b = rng() % 64 == 0 ? 254 : 255;
			}
		}
	}
}

void Run( int nVariant, SData &d, int nPart, int nGridPair, bool bTwo )
{
	const float *pPos = &d.pos[size_t( nPart ) * d.nCount * 3];
	uint8_t *pResA = &d.resA[size_t( nPart ) * d.nCount];
	uint8_t *pResB = bTwo ? &d.resB[size_t( nPart ) * d.nCount] : nullptr;
	const uint8_t *pFogA = &d.grids[2 * nGridPair][0];
	const uint8_t *pFogB = bTwo ? &d.grids[2 * nGridPair + 1][0] : nullptr;
	switch ( nVariant )
	{
		case V_SHIPPED_SSE2:
		case V_SHIPPED_AVX2:
		{
			const NGScene::SLightingKernels &k =
					nVariant == V_SHIPPED_SSE2 ? NGScene::sse2LightingKernels : NGScene::avx2LightingKernels;
			k.pSampleWarFogCoords( reinterpret_cast<const CVec3 *>( pPos ), d.nCount, F_SCALE, &d.coords[0] );
			k.pSampleWarFogInt( &d.coords[0], pFogA, N_GRID, pResA, d.nCount );
			if ( bTwo )
			{
				k.pSampleWarFogInt( &d.coords[0], pFogB, N_GRID, pResB, d.nCount );
			}
			break;
		}
		case V_SCALAR:
			NWarFog::SampleWarFogScalar( pPos, d.nCount, F_SCALE, N_GRID, pFogA, pResA, pFogB, pResB );
			break;
		case V_SSE2:
			NWarFog::SampleWarFogSSE2( pPos, d.nCount, F_SCALE, N_GRID, pFogA, pResA, pFogB, pResB );
			break;
		case V_AVX2_GATHER:
			NWarFog::SampleWarFogAVX2( pPos, d.nCount, F_SCALE, N_GRID, pFogA, pResA, pFogB, pResB );
			break;
		case V_AVX2_INSERT:
			NWarFog::SampleWarFogAVX2T<NWarFog::EAVX2Load::Insert>(
					pPos, d.nCount, F_SCALE, N_GRID, pFogA, pResA, pFogB, pResB );
			break;
		case V_AVX2_BLOCK_CHECK:
			NWarFog::SampleWarFogAVX2T<NWarFog::EAVX2Load::GatherBlockCheck>(
					pPos, d.nCount, F_SCALE, N_GRID, pFogA, pResA, pFogB, pResB );
			break;
		default:
			NWarFog::SampleWarFogAVX2Uniform( pPos, d.nCount, F_SCALE, N_GRID, pFogA, pResA, pFogB, pResB );
			break;
	}
}

// Arguments: variant, positions per part, grids (1 or 2), cold (0 or 1), grid kind.
void BM_WarFog( benchmark::State &state )
{
	const int nVariant = static_cast<int>( state.range( 0 ) );
	const int nCount = static_cast<int>( state.range( 1 ) );
	const bool bTwo = state.range( 2 ) == 2;
	const bool bCold = state.range( 3 ) != 0;
	const int nGridKind = static_cast<int>( state.range( 4 ) );
	if ( NeedsAVX2( nVariant ) && !NGScene::IsAVX2Present() )
	{
		state.SkipWithError( "no AVX2" );
		return;
	}
	SData d;
	MakeData( d, nCount, bCold, nGridKind );
	int nPart = 0, nGridPair = 0;
	for ( auto _ : state )
	{
		Run( nVariant, d, nPart, nGridPair, bTwo );
		benchmark::ClobberMemory();
		if ( ++nPart == d.nParts )
		{
			nPart = 0;
		}
		if ( bCold && ++nGridPair == N_COLD_GRID_PAIRS )
		{
			nGridPair = 0;
		}
	}
	state.SetLabel( VariantName( nVariant ) );
	// Seconds per position, shown by the console reporter as e.g. "1.2n".
	state.counters["per_pos"] =
			benchmark::Counter( nCount, benchmark::Counter::kIsIterationInvariantRate | benchmark::Counter::kInvert );
}

BENCHMARK( BM_WarFog )
		->ArgsProduct( { { V_SHIPPED_SSE2,
                       V_SHIPPED_AVX2,
                       V_SCALAR,
                       V_SSE2,
                       V_AVX2_GATHER,
                       V_AVX2_INSERT,
                       V_AVX2_BLOCK_CHECK,
                       V_AVX2_UNIFORM },
                     { 16, 31, 64, 243, 1024 },
                     { 1, 2 },
                     { 0, 1 },
                     { G_RANDOM } } );

// The uniform shortcut where it hits (G_UNIFORM) and where it mostly hits but some
// parts scan their block and then miss (G_SPARSE), against the plain AVX2 kernel on
// the same data. G_RANDOM in the matrix above is its cheapest miss.
BENCHMARK( BM_WarFog )
		->ArgsProduct(
				{ { V_AVX2_GATHER, V_AVX2_UNIFORM }, { 16, 31, 64, 243, 1024 }, { 1, 2 }, { 0, 1 }, { G_UNIFORM, G_SPARSE } } );

// How often the part-level fast paths would apply: the share of parts whose every
// position falls in one fog cell (all four corners shared), and the share of
// eight-position blocks that do (what EAVX2Load::GatherBlockCheck tests), by part
// extent, positions in random order. Printed once before the benchmarks run.
void PrintFastPathCensus()
{
	std::mt19937 rng( 1234 );
	const float extents[] = { 0.25f, 0.5f, 0.8f, 1.0f, 1.5f, 2.0f, 2.5f, 3.8f, 6.2f };
	const int nCount = 31;
	std::printf( "fast path census, %d positions per part, 2.75 unit cells:\n", nCount );
	for ( float fExtent : extents )
	{
		int nPartsInOneCell = 0, nBlocksInOneCell = 0, nBlocks = 0;
		const int nParts = 20000;
		std::vector<float> pos( 3 * nCount );
		std::vector<int32_t> coords( 2 * nCount );
		for ( int p = 0; p < nParts; ++p )
		{
			MakePart( rng, &pos[0], nCount, fExtent );
			NWarFog::SampleWarFogCoordsSSE2( &pos[0], nCount, F_SCALE, &coords[0] );
			auto cell = [&]( int k ) { return ( ( coords[2 * k + 1] >> 14 ) << 16 ) ^ ( coords[2 * k] >> 14 ); };
			bool bAll = true;
			for ( int k = 1; k < nCount; ++k )
			{
				bAll &= cell( k ) == cell( 0 );
			}
			nPartsInOneCell += bAll;
			for ( int b = 0; b + 8 <= nCount; b += 8 )
			{
				bool bBlock = true;
				for ( int k = b + 1; k < b + 8; ++k )
				{
					bBlock &= cell( k ) == cell( b );
				}
				nBlocksInOneCell += bBlock;
				++nBlocks;
			}
		}
		std::printf( "  extent %4.2f: parts in one cell %5.1f%%, 8-blocks in one cell %5.1f%%\n",
		             fExtent,
		             100.0 * nPartsInOneCell / nParts,
		             100.0 * nBlocksInOneCell / nBlocks );
	}
}

} // namespace

int main( int argc, char **argv )
{
	PrintFastPathCensus();
	benchmark::Initialize( &argc, argv );
	if ( benchmark::ReportUnrecognizedArguments( argc, argv ) )
	{
		return 1;
	}
	benchmark::RunSpecifiedBenchmarks();
	benchmark::Shutdown();
	return 0;
}
