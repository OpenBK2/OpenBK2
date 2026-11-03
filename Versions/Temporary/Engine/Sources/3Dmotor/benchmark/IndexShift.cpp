// Compares the implementations of the 16-bit to 32-bit index shift that
// CDynamicTrisBase::AddPrimitiveGeometry (GfxBuffers.cpp) runs for every batched mesh.
// The variants live in test/IndexShiftVariants.h, the shipped one in GIndexShift.h.
//
// Sizes are indices per call. A mesh is usually a few hundred to a few thousand, and
// one call can never exceed 16380: the dynamic index buffer is 64 KB of 32-bit
// indices, and AddPrimitiveGeometry splits anything larger across buffer refills.
// 30000 is kept anyway to show the asymptote.
//
// Two destinations:
//   Hot   - one small buffer, rewritten every iteration, so it stays in L1/L2.
//   Cold  - the destination walks through 64 MB, so every write misses the cache,
//           which is what non-temporal stores are meant to help with.
// Neither is the real destination. That is memory from IDirect3DIndexBuffer9::Lock,
// which DXVK maps from host-visible Vulkan memory and which may be write-combined;
// user space cannot allocate write-combined memory, so no microbenchmark here can
// reproduce it. These numbers are microbenchmark numbers and say nothing direct about
// frame time.
//
// Both buffers are offset so that the source is only 2-byte aligned and the
// destination only 4-byte aligned, as in the engine.

#include "3Dmotor/stdafx.h"

#include "3Dmotor/GLightPerVertexKernels.h"
// IsAVX2Present() lives beside the lighting kernels, which need the initialised
// nCubicRoot table; the 3Dmotor DLL does not export it on Windows.
#include "3Dmotor/GSSEtransform.cpp"
#include "3Dmotor/test/IndexShiftVariants.h"

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

namespace
{

using TShiftFunc = void ( * )( const uint16_t *, int32_t *, int, int32_t );

enum class EIsa
{
	Base,
	SSE41,
	AVX2
};

struct SVariant
{
	const char *pszName;
	TShiftFunc pFunc;
	EIsa eIsa;
};

const SVariant variants[] = {
	{ "Original", &NIndexShift::Original, EIsa::Base },
	{ "Ref", &NGfx::ShiftIndicesRef, EIsa::Base },
	{ "RestrictIvdep", &NIndexShift::RestrictIvdep, EIsa::Base },
#if BK2_INDEX_SHIFT_AUTOVEC
	{ "AutoVec", &NIndexShift::AutoVec, EIsa::Base },
#endif
#if BK2_INDEX_SHIFT_SSE2
	{ "SSE2", &NGfx::ShiftIndicesSSE2, EIsa::Base },
	{ "SSE2x16", &NIndexShift::SSE2x16, EIsa::Base },
	{ "SSE2Unaligned", &NIndexShift::SSE2Unaligned, EIsa::Base },
	{ "SSE2Stream", &NIndexShift::SSE2Stream, EIsa::Base },
	{ "SSE41", &NIndexShift::SSE41, EIsa::SSE41 },
	{ "AVX2", &NIndexShift::AVX2, EIsa::AVX2 },
	{ "AVX2Aligned", &NIndexShift::AVX2Aligned, EIsa::AVX2 },
	{ "AVX2Stream", &NIndexShift::AVX2Stream, EIsa::AVX2 },
#endif
#if BK2_INDEX_SHIFT_STDX
	{ "Stdx", &NIndexShift::Stdx, EIsa::Base },
#endif
};

// SSE4.1 is implied by AVX2, and every CPU this benchmark is run on to compare the
// two has both, so the AVX2 check stands in for it rather than adding a second
// CPUID probe.
bool IsUsable( EIsa eIsa )
{
	return eIsa == EIsa::Base || NGScene::IsAVX2Present();
}

const int N_COLD_BYTES = 64 << 20;

void BM_IndexShift( benchmark::State &state, const SVariant &variant, bool bCold )
{
	if ( !IsUsable( variant.eIsa ) )
	{
		state.SkipWithError( "CPU lacks the instruction set" );
		return;
	}
	const int nSize = static_cast<int>( state.range( 0 ) );
	std::mt19937 rng( 12345 );
	std::uniform_int_distribution<int> dist( 0, 65535 );
	// One extra element in front of each buffer to knock the alignment down to the
	// engine's 2 and 4 bytes.
	std::vector<uint16_t> src( nSize + 1 );
	for ( uint16_t &n : src )
	{
		n = static_cast<uint16_t>( dist( rng ) );
	}
	const int nDstInts = bCold ? N_COLD_BYTES / 4 : nSize + 1;
	std::vector<int32_t> dst( nDstInts + 8 );
	// Fill once so that page faults are not part of the measurement.
	std::fill( dst.begin(), dst.end(), 0 );
	const uint16_t *pSrc = src.data() + 1;
	int nOffset = 1;
	const int32_t nShift = 1234567;
	for ( auto _ : state )
	{
		variant.pFunc( pSrc, dst.data() + nOffset, nSize, nShift );
		benchmark::ClobberMemory();
		if ( bCold )
		{
			// Next call writes fresh lines; wrap before running off the end.
			nOffset += nSize;
			if ( nOffset + nSize > nDstInts )
			{
				nOffset = 1;
			}
		}
	}
	state.SetItemsProcessed( state.iterations() * nSize );
	state.SetBytesProcessed( state.iterations() * nSize * 6 );
}

int RegisterAll()
{
	for ( const bool bCold : { false, true } )
	{
		for ( const SVariant &variant : variants )
		{
			std::string name = std::string( bCold ? "Cold/" : "Hot/" ) + variant.pszName;
			benchmark::RegisterBenchmark( name.c_str(), BM_IndexShift, variant, bCold )
					->Arg( 300 )
					->Arg( 3000 )
					->Arg( 30000 );
		}
	}
	return 0;
}

const int nRegistered = RegisterAll();

} // namespace

BENCHMARK_MAIN();
