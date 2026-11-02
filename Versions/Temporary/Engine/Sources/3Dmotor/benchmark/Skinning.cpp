// Compares the position skinning kernels of GSkinning.h and test/SkinningVariants.h
// with the frozen copy of the engine's SSESkinning / SSEBatchTransform
// (test/SkinningReference.h). One benchmark iteration is one call, i.e. one render
// part, including whatever per-call preparation a kernel needs (bone columns), so
// the small sizes show the call overhead that dominates real parts.
//
// Distributions, from the census of the 32,333 shipped render parts:
//   RigidPart - every position one influence of weight 1.0 and the same bone. 98.8%
//               of parts, 94.4% of positions. Through the skinning kernels this is
//               what the engine does today; the Rigid/ group below is what a load-time
//               "rigid" flag per part would let it do instead.
//   Blend     - the truly blended 5% (infantry, flags): influence counts 1/2/3/4 at
//               39/17/22/22%, bones drawn at random from the skeleton, so the stored
//               order has short runs (the census measured a mean of 1.9).
//   BlendSorted - the same vertices grouped by bone set, which is what a load-time
//               reorder would give; this separates the reorder's gain (better
//               branch prediction for every kernel) from the Runs kernels' own gain.
// Sizes are positions per part: census median 31, mean 117, p90 243, p99 1614.
// Skeletons have 16 bones (the census median) unless BK2_SKIN_BONES says otherwise;
// BK2_SKIN_DIST="f1,f2,f3,f4" replaces the Blend mix.
//
// Hot  - one part, called over and over: everything in L1/L2.
// Cold - consecutive iterations walk through 64 MB of distinct parts (positions,
//        weights, results and matrices), so every call starts with cold caches; the
//        L2 here is 1.25 MB per core, the L3 24 MB.
//
// These are microbenchmark numbers. One once promised 6.9x and delivered 1.5% in the
// game, so measure the game before believing any of them.

#include "3Dmotor/stdafx.h"

#include "3Dmotor/GLightPerVertexKernels.h"
// IsAVX2Present() lives beside the lighting kernels, which need the initialised
// nCubicRoot table; the 3Dmotor DLL does not export it on Windows.
#include "3Dmotor/GSSEtransform.cpp"
#include "3Dmotor/test/SkinningReference.h"
#include "3Dmotor/test/SkinningVariants.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

namespace
{

using NSkinningVariants::SBoneColumns;
using NSkinning::SSkinWeights;
using NSkinningVariants::SRigidCall;
using NSkinningVariants::SSkinCall;
using NSkinningVariants::SSkinRun;

const size_t N_COLD_BYTES = size_t( 64 ) << 20;

void RefSkin( const SSkinCall &c )
{
	NSkinningReference::SSESkinning( reinterpret_cast<const CVec3 *>( c.pSrc ),
	                                 reinterpret_cast<CVec3 *>( c.pDst ),
	                                 c.pWeights,
	                                 reinterpret_cast<const SHMatrix *>( c.pMatrices ),
	                                 c.nCount );
}

void RefRigid( const SRigidCall &c )
{
	NSkinningReference::SSEBatchTransform( *reinterpret_cast<const SHMatrix *>( c.pMatrix ),
	                                       reinterpret_cast<const CVec3 *>( c.pSrc ),
	                                       reinterpret_cast<CVec3 *>( c.pDst ),
	                                       c.nCount );
}

// What the engine would run with a rigid flag and its existing bounds loop, but the
// transform still the glm one: separates the flag's gain from the kernel's.
void RefRigidThenBounds( const SRigidCall &c )
{
	RefRigid( c );
	NSkinningVariants::ScalarBounds( c.pDst, c.nCount, c.pMin, c.pMax );
}

struct SSkinVariant
{
	const char *pszName;
	void ( *pFunc )( const SSkinCall & );
	bool bNeedsAVX2;
};

const SSkinVariant skinVariants[] = {
	{ "Ref", &RefSkin, false },
	{ "Scalar", &NSkinningVariants::Scalar, false },
#if BK2_SKINNING_SSE2
	{ "ColsBlend", &NSkinningVariants::ColsBlend, false },
	{ "ColsXform", &NSkinningVariants::ColsXform, false },
	{ "ColsXformFast", &NSkinningVariants::ColsXformFast, false },
	{ "RowsBlend", &NSkinningVariants::RowsBlend, false },
	{ "RowsBlendFast", &NSkinningVariants::RowsBlendFast, false },
	{ "SoA4", &NSkinningVariants::SoA4, false },
	{ "RunsSSE", &NSkinningVariants::RunsSSE, false },
	{ "AVX2Gather", &NSkinningVariants::AVX2Gather, true },
	{ "RunsAVX2", &NSkinningVariants::RunsAVX2, true },
#endif
};

struct SRigidVariant
{
	const char *pszName;
	void ( *pFunc )( const SRigidCall & );
	bool bNeedsAVX2;
};

const SRigidVariant rigidVariants[] = {
	{ "RigidRef", &RefRigid, false },
	{ "RigidRefThenBounds", &RefRigidThenBounds, false },
	{ "RigidScalar", &NSkinningVariants::RigidScalar, false },
#if BK2_SKINNING_SSE2
	{ "RigidSSE", &NSkinningVariants::RigidSSE, false },
	{ "RigidSSEThenBounds", &NSkinningVariants::RigidSSEThenBounds, false },
	{ "RigidSSEBounds", &NSkinningVariants::RigidSSEBounds, false },
	{ "RigidSoA4", &NSkinningVariants::RigidSoA4, false },
	{ "RigidAVX2", &NSkinningVariants::RigidAVX2, true },
	{ "RigidAVX2Bounds", &NSkinningVariants::RigidAVX2Bounds, true },
#endif
};

enum class EDist
{
	RigidPart,
	Blend,
	BlendSorted
};

int BonesPerSkeleton()
{
	const char *psz = std::getenv( "BK2_SKIN_BONES" );
	const int n = psz ? std::atoi( psz ) : 16;
	return std::clamp( n, 1, 255 );
}

void BlendFractions( double *pFrac )
{
	const double defaults[4] = { 0.39, 0.17, 0.22, 0.22 };
	std::copy( defaults, defaults + 4, pFrac );
	if ( const char *psz = std::getenv( "BK2_SKIN_DIST" ) )
	{
		double v[4] = {};
		if ( std::sscanf( psz, "%lf,%lf,%lf,%lf", &v[0], &v[1], &v[2], &v[3] ) == 4 )
		{
			std::copy( v, v + 4, pFrac );
		}
	}
}

// An affine bone with a random rotation, scale near 1 and translation up to 1000.
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

// nParts parts of nCount positions each, laid out back to back in arenas, with
// their run tables built at "load time".
struct SPartSet
{
	int nCount = 0;
	int nBones = 0;
	int nParts = 0;
	std::vector<float> src, dst, matrices;
	std::vector<SSkinWeights> weights;
	std::vector<std::vector<SSkinRun>> runs;
	std::vector<SBoneColumns> scratch;
	float vMin[3] = {}, vMax[3] = {};
};

void MakeWeights( std::mt19937 &rng, EDist eDist, int nBones, SSkinWeights *pW, int nCount )
{
	std::uniform_int_distribution<int> bone( 0, nBones - 1 );
	std::uniform_real_distribution<float> raw( 0.05f, 1.0f );
	std::uniform_real_distribution<double> pick( 0.0, 1.0 );
	double frac[4];
	BlendFractions( frac );
	const int nPartBone = bone( rng );
	for ( int k = 0; k < nCount; ++k )
	{
		SSkinWeights &w = pW[k];
		w = SSkinWeights{};
		if ( eDist == EDist::RigidPart )
		{
			w.fWeights[0] = 1.0f;
			w.nWeights[0] = 255;
			w.cBoneIndices[0] = static_cast<uint8_t>( nPartBone );
			continue;
		}
		double r = pick( rng ) * ( frac[0] + frac[1] + frac[2] + frac[3] );
		int nInfl = 1;
		while ( nInfl < 4 && r >= frac[nInfl - 1] )
		{
			r -= frac[nInfl - 1];
			++nInfl;
		}
		nInfl = std::min( nInfl, nBones );
		float fSum = 0;
		for ( int i = 0; i < nInfl; ++i )
		{
			// Distinct bones per vertex, as an exporter writes them.
			int b;
			bool bDup;
			do
			{
				b = bone( rng );
				bDup = false;
				for ( int j = 0; j < i; ++j )
				{
					bDup = bDup || w.cBoneIndices[j] == b;
				}
			} while ( bDup );
			w.cBoneIndices[i] = static_cast<uint8_t>( b );
			w.fWeights[i] = raw( rng );
			fSum += w.fWeights[i];
		}
		for ( int i = 0; i < nInfl; ++i )
		{
			w.fWeights[i] = nInfl == 1 ? 1.0f : w.fWeights[i] / fSum;
			w.nWeights[i] = static_cast<uint8_t>( std::lround( w.fWeights[i] * 255.0f ) );
		}
	}
	if ( eDist == EDist::BlendSorted )
	{
		std::stable_sort( pW,
		                  pW + nCount,
		                  []( const SSkinWeights &a, const SSkinWeights &b )
		                  {
												const int ma = NSkinningVariants::WeightMask( a ), mb = NSkinningVariants::WeightMask( b );
												if ( ma != mb )
												{
													return ma < mb;
												}
												return std::lexicographical_compare(
														a.cBoneIndices, a.cBoneIndices + 4, b.cBoneIndices, b.cBoneIndices + 4 );
											} );
	}
}

void MakePartSet( SPartSet &set, EDist eDist, int nCount, int nBones, bool bCold )
{
	set.nCount = nCount;
	set.nBones = nBones;
	const size_t nPartBytes = size_t( nCount ) * ( 12 + 12 + 24 ) + size_t( nBones ) * 64;
	set.nParts = bCold ? static_cast<int>( std::max<size_t>( 2, N_COLD_BYTES / nPartBytes ) ) : 1;
	std::mt19937 rng( 12345 );
	std::uniform_real_distribution<float> pos( -5.0f, 5.0f );
	set.src.resize( size_t( set.nParts ) * nCount * 3 );
	for ( float &v : set.src )
	{
		v = pos( rng );
	}
	set.dst.assign( set.src.size(), 0.0f );
	set.matrices.resize( size_t( set.nParts ) * nBones * 16 );
	for ( size_t b = 0; b < size_t( set.nParts ) * nBones; ++b )
	{
		RandomMatrix( rng, &set.matrices[16 * b] );
	}
	set.weights.resize( size_t( set.nParts ) * nCount );
	set.runs.resize( set.nParts );
	for ( int p = 0; p < set.nParts; ++p )
	{
		SSkinWeights *pW = set.weights.data() + size_t( p ) * nCount;
		MakeWeights( rng, eDist, nBones, pW, nCount );
		NSkinningVariants::BuildRuns( pW, nCount, set.runs[p] );
	}
	set.scratch.resize( 256 );
}

SSkinCall SkinCall( SPartSet &set, int p )
{
	const size_t nOff = size_t( p ) * set.nCount;
	return SSkinCall{ set.src.data() + 3 * nOff,
	                  set.dst.data() + 3 * nOff,
	                  set.weights.data() + nOff,
	                  set.nCount,
	                  set.matrices.data() + size_t( p ) * set.nBones * 16,
	                  set.nBones,
	                  set.scratch.data(),
	                  set.runs[p].data(),
	                  static_cast<int>( set.runs[p].size() ) };
}

// Seconds per position, which the report script turns into ns.
void SetCounters( benchmark::State &state, int nCount )
{
	state.SetItemsProcessed( state.iterations() * nCount );
	state.counters["s_per_vtx"] = benchmark::Counter(
			static_cast<double>( nCount ), benchmark::Counter::kIsIterationInvariantRate | benchmark::Counter::kInvert );
}

void BM_Skin( benchmark::State &state, const SSkinVariant &v, EDist eDist, bool bCold )
{
	if ( v.bNeedsAVX2 && !NGScene::IsAVX2Present() )
	{
		state.SkipWithError( "CPU lacks AVX2" );
		return;
	}
	const int nCount = static_cast<int>( state.range( 0 ) );
	SPartSet set;
	MakePartSet( set, eDist, nCount, BonesPerSkeleton(), bCold );
	int p = 0;
	for ( auto _ : state )
	{
		v.pFunc( SkinCall( set, p ) );
		benchmark::ClobberMemory();
		if ( ++p == set.nParts )
		{
			p = 0;
		}
	}
	SetCounters( state, nCount );
}

void BM_Rigid( benchmark::State &state, const SRigidVariant &v, bool bCold )
{
	if ( v.bNeedsAVX2 && !NGScene::IsAVX2Present() )
	{
		state.SkipWithError( "CPU lacks AVX2" );
		return;
	}
	const int nCount = static_cast<int>( state.range( 0 ) );
	SPartSet set;
	// One matrix per part: the part's own bone.
	MakePartSet( set, EDist::RigidPart, nCount, 1, bCold );
	int p = 0;
	for ( auto _ : state )
	{
		const size_t nOff = size_t( p ) * nCount;
		v.pFunc( SRigidCall{ set.src.data() + 3 * nOff,
		                     set.dst.data() + 3 * nOff,
		                     nCount,
		                     set.matrices.data() + 16 * size_t( p ),
		                     set.vMin,
		                     set.vMax } );
		benchmark::ClobberMemory();
		if ( ++p == set.nParts )
		{
			p = 0;
		}
	}
	SetCounters( state, nCount );
}

int RegisterAll()
{
	const struct
	{
		const char *pszName;
		EDist eDist;
	} dists[] = { { "RigidPart", EDist::RigidPart }, { "Blend", EDist::Blend }, { "BlendSorted", EDist::BlendSorted } };

	for ( const bool bCold : { false, true } )
	{
		const std::vector<int64_t> sizes =
				bCold ? std::vector<int64_t>{ 32, 256 } : std::vector<int64_t>{ 16, 32, 64, 256, 1024, 5000 };
		for ( const auto &d : dists )
		{
			for ( const SSkinVariant &v : skinVariants )
			{
				const std::string name = std::string( bCold ? "Cold/" : "Hot/" ) + d.pszName + "/" + v.pszName;
				auto *pBench = benchmark::RegisterBenchmark( name.c_str(), BM_Skin, v, d.eDist, bCold );
				for ( const int64_t n : sizes )
				{
					pBench->Arg( n );
				}
			}
		}
		for ( const SRigidVariant &v : rigidVariants )
		{
			const std::string name = std::string( bCold ? "Cold/" : "Hot/" ) + "Rigid/" + v.pszName;
			auto *pBench = benchmark::RegisterBenchmark( name.c_str(), BM_Rigid, v, bCold );
			for ( const int64_t n : sizes )
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
