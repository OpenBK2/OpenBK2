// Compares the rigid normal transform kernels of GNormalTransform.h and
// test/NormalTransformVariants.h with the frozen copy of the engine's SSE2
// MMXTransformVector, called once per normal as SimpleTransform and
// SingleSkinTransform do today ("Ref"). One benchmark iteration is one call, i.e.
// one render part, including the per-call matrix preparation, so the small sizes show
// the call overhead that dominates real parts.
//
// Normals are read the way the engine would pass them, the normal field of an
// SUVInfo array (stride 20), and written to a packed array. Sizes are normals per
// part: the census median is 31 positions per part and normals are per vertex,
// about 2.6 per position, so a typical part has around 80.
//
// Hot  - one part, called over and over: everything in L1.
// Cold - consecutive iterations walk through 64 MB of distinct parts (sources,
//        results and matrices), so every call starts with cold caches.
//
// These are microbenchmark numbers. One once promised 6.9x and delivered 1.5% in the
// game, so measure the game before believing any of them.

#include "3Dmotor/stdafx.h"

// IsAVX2Present() comes with the lighting kernel sources the CMake target adds;
// GSSEtransform.cpp supplies the tables GSSEtransform.h's code expects, whose live
// MMXTransformVector2/3 are the baseline of the Blend group.
#include "3Dmotor/GLightPerVertexKernels.h"
#include "3Dmotor/GSSEtransform.cpp"
#include "3Dmotor/GSSEtransform.h"
#include "3Dmotor/test/NormalTransformVariants.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

namespace
{

using NNormalVariants::SNormalCall;
using NNormalVariants::SNormalVariant;

const size_t N_COLD_BYTES = size_t( 64 ) << 20;
const int N_STRIDE = 20; // sizeof( SUVInfo )

// A rotation with a scale near 1 and a large translation.
void RandomMatrix( std::mt19937 &rng, float *m )
{
	std::uniform_real_distribution<float> angle( -3.14159f, 3.14159f );
	std::uniform_real_distribution<float> scale( 0.8f, 1.2f );
	std::uniform_real_distribution<float> trans( -1000.0f, 1000.0f );
	const float a = angle( rng ), b = angle( rng ), s = scale( rng );
	const float ca = std::cos( a ), sa = std::sin( a ), cb = std::cos( b ), sb = std::sin( b );
	const float r[3][3] = {
		{ ca * s, -sa * cb * s, sa * sb * s }, { sa * s, ca * cb * s, -ca * sb * s }, { 0, sb * s, cb * s } };
	for ( int row = 0; row < 3; ++row )
	{
		for ( int col = 0; col < 3; ++col )
		{
			m[4 * row + col] = r[row][col];
		}
		m[4 * row + 3] = trans( rng );
	}
	m[12] = m[13] = m[14] = 0;
	m[15] = 1;
}

// nParts parts of nCount normals each, back to back.
struct SPartSet
{
	int nCount = 0;
	int nParts = 0;
	std::vector<uint8_t> src;
	std::vector<uint32_t> dst;
	std::vector<float> matrices;
};

void MakePartSet( SPartSet &set, int nCount, bool bCold )
{
	set.nCount = nCount;
	const size_t nPartBytes = size_t( nCount ) * ( N_STRIDE + 4 ) + 64;
	set.nParts = bCold ? static_cast<int>( std::max<size_t>( 2, N_COLD_BYTES / nPartBytes ) ) : 1;
	std::mt19937 rng( 12345 );
	set.src.resize( size_t( set.nParts ) * nCount * N_STRIDE );
	// Unit-ish normals, as the data holds: random direction, encoded the engine's way.
	std::normal_distribution<float> gauss;
	for ( size_t k = 0; k < size_t( set.nParts ) * nCount; ++k )
	{
		uint8_t *p = &set.src[k * N_STRIDE];
		for ( int b = 0; b < N_STRIDE; ++b )
		{
			p[b] = static_cast<uint8_t>( rng() );
		}
		float v[3] = { gauss( rng ), gauss( rng ), gauss( rng ) };
		const float len = std::sqrt( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] ) + 1e-6f;
		// Bytes z, y, x, w at offset 8, where SUVInfo keeps the normal.
		p[8] = static_cast<uint8_t>( std::clamp( static_cast<int>( v[2] / len * 127 ) + 128, 0, 255 ) );
		p[9] = static_cast<uint8_t>( std::clamp( static_cast<int>( v[1] / len * 127 ) + 128, 0, 255 ) );
		p[10] = static_cast<uint8_t>( std::clamp( static_cast<int>( v[0] / len * 127 ) + 128, 0, 255 ) );
		p[11] = 255;
	}
	set.dst.assign( size_t( set.nParts ) * nCount, 0 );
	set.matrices.resize( size_t( set.nParts ) * 16 );
	for ( int p = 0; p < set.nParts; ++p )
	{
		RandomMatrix( rng, &set.matrices[16 * size_t( p )] );
	}
}

void BM_Normals( benchmark::State &state, SNormalVariant v, bool bCold )
{
	if ( v.bNeedsAVX2 && !NGScene::IsAVX2Present() )
	{
		state.SkipWithError( "CPU lacks AVX2" );
		return;
	}
	const int nCount = static_cast<int>( state.range( 0 ) );
	SPartSet set;
	MakePartSet( set, nCount, bCold );
	int p = 0;
	for ( auto _ : state )
	{
		const size_t nOff = size_t( p ) * nCount;
		v.pFunc( SNormalCall{ set.src.data() + nOff * N_STRIDE + 8,
		                      N_STRIDE,
		                      set.dst.data() + nOff,
		                      nCount,
		                      set.matrices.data() + 16 * size_t( p ) } );
		benchmark::ClobberMemory();
		if ( ++p == set.nParts )
		{
			p = 0;
		}
	}
	state.SetItemsProcessed( state.iterations() * nCount );
	// Seconds per normal; the console shows it as ns/us per normal.
	state.counters["per_normal"] = benchmark::Counter(
			static_cast<double>( nCount ), benchmark::Counter::kIsIterationInvariantRate | benchmark::Counter::kInvert );
}

// The blended vertices of non-rigid parts: every vertex has two or three influences
// on bones drawn from a 16-bone skeleton, weights summing to 255, and goes through
// either the live glm MMXTransformVector2/3 or TransformNormalBlend2/3, one call per
// normal as SingleSkinTransform makes them. One iteration is nCount normals.
struct SBlendVertex
{
	uint8_t nBones[3];
	uint8_t nWeights[3];
};

void BM_Blend( benchmark::State &state, bool bNew, int nInfluences )
{
	const int nCount = static_cast<int>( state.range( 0 ) );
	const int N_BONES = 16;
	std::mt19937 rng( 777 );
	std::vector<float> matrices( 16 * N_BONES );
	for ( int b = 0; b < N_BONES; ++b )
	{
		RandomMatrix( rng, &matrices[16 * b] );
	}
	std::vector<NGfx::SCompactVector> src( nCount ), dst( nCount );
	std::vector<SBlendVertex> verts( nCount );
	std::uniform_int_distribution<int> bone( 0, N_BONES - 1 ), share( 40, 200 );
	for ( int k = 0; k < nCount; ++k )
	{
		src[k].dw = static_cast<uint32_t>( rng() );
		SBlendVertex &v = verts[k];
		const int w1 = share( rng );
		const int w2 = nInfluences == 3 ? ( 255 - w1 ) / 2 : 255 - w1;
		v = SBlendVertex{
			{ static_cast<uint8_t>( bone( rng ) ), static_cast<uint8_t>( bone( rng ) ), static_cast<uint8_t>( bone( rng ) ) },
			{ static_cast<uint8_t>( w1 ), static_cast<uint8_t>( w2 ), static_cast<uint8_t>( 255 - w1 - w2 ) } };
	}
	const SHMatrix *pBlends = reinterpret_cast<const SHMatrix *>( matrices.data() );
	for ( auto _ : state )
	{
		for ( int k = 0; k < nCount; ++k )
		{
			const SBlendVertex &v = verts[k];
			if ( bNew )
			{
				const float *m1 = &matrices[16 * v.nBones[0]], *m2 = &matrices[16 * v.nBones[1]];
				dst[k].dw =
						nInfluences == 3
								? NNormals::TransformNormalBlend3(
											src[k].dw, m1, v.nWeights[0], m2, v.nWeights[1], &matrices[16 * v.nBones[2]], v.nWeights[2] )
								: NNormals::TransformNormalBlend2( src[k].dw, m1, v.nWeights[0], m2, v.nWeights[1] );
			}
			else if ( nInfluences == 3 )
			{
				MMXTransformVector3( dst[k],
				                     src[k],
				                     pBlends[v.nBones[0]],
				                     v.nWeights[0],
				                     pBlends[v.nBones[1]],
				                     v.nWeights[1],
				                     pBlends[v.nBones[2]],
				                     v.nWeights[2] );
			}
			else
			{
				MMXTransformVector2( dst[k], src[k], pBlends[v.nBones[0]], v.nWeights[0], pBlends[v.nBones[1]], v.nWeights[1] );
			}
		}
		benchmark::ClobberMemory();
	}
	state.SetItemsProcessed( state.iterations() * nCount );
	state.counters["per_normal"] = benchmark::Counter(
			static_cast<double>( nCount ), benchmark::Counter::kIsIterationInvariantRate | benchmark::Counter::kInvert );
}

int RegisterAll()
{
	for ( const int nInfluences : { 2, 3 } )
	{
		for ( const bool bNew : { false, true } )
		{
			const std::string name =
					std::string( "Hot/" ) + ( bNew ? "BlendNew" : "BlendGlm" ) + std::to_string( nInfluences );
			benchmark::RegisterBenchmark( name.c_str(), BM_Blend, bNew, nInfluences )->Arg( 256 );
		}
	}
	int nVariants = 0;
	const SNormalVariant *pVariants = NNormalVariants::GetNormalVariants( &nVariants );
	for ( const bool bCold : { false, true } )
	{
		for ( int v = 0; v < nVariants; ++v )
		{
			const std::string name = std::string( bCold ? "Cold/" : "Hot/" ) + pVariants[v].pszName;
			auto *pBench = benchmark::RegisterBenchmark( name.c_str(), BM_Normals, pVariants[v], bCold );
			for ( const int64_t n : { 16, 32, 64, 256, 1024 } )
			{
				pBench->Arg( n );
			}
		}
	}
	return 0;
}

const int nRegistered = RegisterAll();

} // namespace

BENCHMARK_MAIN();
