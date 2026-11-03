// Compares the fused per-vertex lighting kernels of GLightPerVertexFused.h with what
// CalcPerVertexLight does today for a lit part on a directional cache miss (the case
// every moving part hits every frame):
//
//   Current      shipped pCalcDirectionalLighting (AVX2 set), MultiplyOnColor twice
//                and the SGeomVecFull output loop (frozen copies of the engine's,
//                compiled here at the engine's optimisation level), then the gfxData
//                to vertex buffer memcpy of STGfxCacheTransformer::Transform
//   Fused*       one pass: light, multiply by the vertex colour, write the vertex,
//                into gfxData (+ memcpy), gfxData plus a streamed vertex buffer copy,
//                or the vertex buffer alone with plain or streaming stores
//   Split*       DirectionalMultiply into the cache arrays, then AssembleVertices
//
// and the pieces on their own: the directional kernels on normal streams with 0%, 30%
// and 70% of vertices repeating the previous normal (the shipped kernels' reuse
// shortcut), and MultiplyOnColor.
//
// One iteration is one render part of nCount vertices. Sizes: the census median is
// about 80 vertices per part, p90 about 600.
//
// Hot  - the same part over and over: everything in L1/L2.
// Cold - consecutive iterations walk through 64 MB of distinct parts (inputs,
//        gfxData, vertex buffer and cache arrays), so every call starts cold.
// In the game the truth is in between: normals and positions were written just
// before by the transform, the SUVInfo and gfxData are a frame old.
//
// The "vertex buffer" here is ordinary cacheable memory. A real locked dynamic buffer
// may be write-combined, where plain stores of whole lines are also combined but
// reads are uncached; the streaming variants model the WC-friendly store pattern, not
// WC memory itself.
//
// These are microbenchmark numbers. One once promised 6.9x and delivered 1.5% in the
// game, so measure the game before believing any of them.

#include "3Dmotor/stdafx.h"

#include "3DLib/GGeometry.h"
#include "3Dmotor/GfxBuffers.h"
#include "3Dmotor/GLightPerVertex.h"
#include "3Dmotor/GLightPerVertexFused.h"
#include "3Dmotor/GLightPerVertexKernels.h"
// The cube root table the lighting kernel sources reference, as LightingKernels.cpp
// gets it.
#include "3Dmotor/GSSEtransform.cpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

namespace
{

using NLightFused::SDirectionalConsts;
using NLightFused::SFusedLightKernels;
using NLightFused::SVertexCall;
using NLightFused::SVertexFull;

const size_t N_COLD_BYTES = size_t( 64 ) << 20;

// GLightPerVertex.cpp's MultiplyOnColor loop, over raw pointers so no vector is
// built per call.
void FrozenMultiplyOnColor( uint32_t *pDst, uint32_t *pDstEnd, const uint32_t *pSrc )
{
	for ( ; pDst < pDstEnd; ++pDst, ++pSrc )
	{
		NGfx::SPixel8888 src = *pSrc;
		NGfx::SPixel8888 dst = *pDst;

		dst.r = std::min<int>( 255, ( src.r * dst.r ) >> 8 );
		dst.g = std::min<int>( 255, ( src.g * dst.g ) >> 8 );
		dst.b = std::min<int>( 255, ( src.b * dst.b ) >> 8 );
		dst.a = std::min<int>( 255, ( src.a * dst.a ) >> 8 );

		*pDst = dst.dwColor;
	}
}

// The lit branch of CalcPerVertexLight's SGeomVecFull output loop.
void FrozenOutputLoop( NGfx::SGeomVecFull *pRes,
                       const std::vector<CVec3> &srcPos,
                       const NGScene::SUVInfo *pSrc,
                       const std::vector<uint16_t> &posIndices,
                       const std::vector<NGfx::SCompactVector> &_normals,
                       const std::vector<uint32_t> &colors,
                       const std::vector<uint32_t> &shadowColors,
                       const unsigned char *pWarFogOld,
                       const unsigned char *pWarFogNew,
                       int nWarFogMask )
{
	int k = 0;
	for ( const NGScene::SUVInfo *pEnd = pSrc + posIndices.size(); pSrc < pEnd; ++pSrc, ++pRes, ++k )
	{
		NGfx::SGeomVecFull &res = *pRes;
		const CVec3 &pos = srcPos[posIndices[k]];
		res.pos = pos;
		res.normal = _normals[k];
		res.tex = pSrc->tex;
		res.texLM = pSrc->texLM;
		int nWarFogIndex = posIndices[k] & nWarFogMask;
		res.texU.dw = ( shadowColors[k] & 0xffffff ) | ( pWarFogOld[nWarFogIndex] << 24 );
		res.texV.dw = ( colors[k] & 0xffffff ) | ( pWarFogNew[nWarFogIndex] << 24 );
	}
}

// One render part with everything the lighting reads and writes.
struct SPart
{
	std::vector<CVec3> srcPos;
	std::vector<uint16_t> posIndices;
	std::vector<NGScene::SUVInfo> uv;
	std::vector<NGfx::SCompactVector> normals;
	std::vector<uint32_t> vertexColor;
	std::vector<uint8_t> fogOld, fogNew;
	// Outputs: the cache arrays, gfxData and the vertex buffer range.
	std::vector<uint32_t> colors, shadow;
	std::vector<SVertexFull> gfx;
	SVertexFull *pVB = nullptr;
};

struct SPartSet
{
	std::vector<SPart> parts;
	// One 64-byte aligned block standing in for the dynamic vertex buffer.
	std::vector<SVertexFull> vbStorage;
	NGScene::SPerVertexLightState ls;
	NGfx::SMMXWord translucent;
	SDirectionalConsts consts;
};

void MakeSet( SPartSet &set, int nCount, double fRepeat, bool bCold )
{
	std::mt19937 rng( 4242 );
	// Census: ~2.6 vertices per position.
	const int nPos = (std::max)( 1, static_cast<int>( nCount / 2.6 ) );
	const size_t nPartBytes = size_t( nCount ) * ( 2 + 20 + 4 + 4 + 8 + 32 + 32 + 2 ) + size_t( nPos ) * 12 + 256;
	const int nParts = bCold ? static_cast<int>( std::max<size_t>( 2, N_COLD_BYTES / nPartBytes ) ) : 1;
	set.parts.resize( nParts );
	set.vbStorage.resize( size_t( nParts ) * nCount + 2 );
	SVertexFull *pVB = set.vbStorage.data();
	// 32-byte aligned start, as a locked buffer would be.
	while ( reinterpret_cast<uintptr_t>( pVB ) % 32 != 0 )
	{
		pVB = reinterpret_cast<SVertexFull *>( reinterpret_cast<char *>( pVB ) + 4 );
	}
	std::normal_distribution<float> gauss;
	std::bernoulli_distribution repeat( fRepeat );
	for ( int p = 0; p < nParts; ++p )
	{
		SPart &part = set.parts[p];
		part.srcPos.resize( nPos );
		for ( CVec3 &v : part.srcPos )
		{
			v = CVec3( gauss( rng ) * 100, gauss( rng ) * 100, gauss( rng ) * 100 );
		}
		part.posIndices.resize( nCount );
		part.uv.resize( nCount );
		part.normals.resize( nCount );
		part.vertexColor.resize( nCount );
		for ( int k = 0; k < nCount; ++k )
		{
			// Neighbouring vertices mostly share nearby positions, as an
			// optimised mesh's do.
			part.posIndices[k] =
					static_cast<uint16_t>( (std::min)( nPos - 1, static_cast<int>( k / 2.6 ) + int( rng() % 3 ) ) );
			NGScene::SUVInfo &u = part.uv[k];
			std::memset( &u, 0, sizeof( u ) );
			u.tex.nU = static_cast<short>( rng() );
			u.tex.nV = static_cast<short>( rng() );
			u.texLM.nU = static_cast<short>( rng() );
			u.texLM.nV = static_cast<short>( rng() );
			if ( k > 0 && repeat( rng ) )
			{
				part.normals[k] = part.normals[k - 1];
			}
			else
			{
				// A unit normal encoded the engine's way, w 255.
				float v[3] = { gauss( rng ), gauss( rng ), gauss( rng ) };
				const float len = std::sqrt( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] ) + 1e-6f;
				uint32_t d = 0xff000000u;
				for ( int i = 0; i < 3; ++i )
				{
					d |= static_cast<uint32_t>( std::clamp( static_cast<int>( v[2 - i] / len * 127 ) + 128, 0, 255 ) )
					     << ( 8 * i );
				}
				part.normals[k].dw = d;
			}
			// Mostly white-ish vertex colours.
			part.vertexColor[k] = 0xff000000u | ( 0xc0c0c0u + ( rng() & 0x3f3f3f ) );
		}
		part.fogOld.resize( nPos );
		part.fogNew.resize( nPos );
		for ( int i = 0; i < nPos; ++i )
		{
			part.fogOld[i] = static_cast<uint8_t>( rng() );
			part.fogNew[i] = static_cast<uint8_t>( rng() );
		}
		part.colors.resize( nCount );
		part.shadow.resize( nCount );
		part.gfx.resize( nCount );
		part.pVB = pVB + size_t( p ) * nCount;
	}
	set.ls.SetDirectional( CVec3( 0.35f, 0.33f, 0.3f ),
	                       CVec3( 0.9f, 0.85f, 0.7f ),
	                       CVec3( 0.2f, 0.22f, 0.3f ),
	                       CVec3( 0.5f, 0.45f, 0.4f ),
	                       CVec3( 0.48f, -0.6f, -0.64f ),
	                       1,
	                       CVec3( 1, 1, 1 ) );
	set.translucent = set.ls.shadeColor;
	const NGfx::SMMXWord *src[7] = { &set.ls.shift,
	                                 &set.ls.dirLight,
	                                 &set.ls.ambient,
	                                 &set.ls.lightColor,
	                                 &set.ls.incidentShadowColor,
	                                 &set.ls.shadeColor,
	                                 &set.translucent };
	int16_t *dst[7] = { set.consts.shift,
	                    set.consts.dirLight,
	                    set.consts.ambient,
	                    set.consts.lightColor,
	                    set.consts.incidentShadowColor,
	                    set.consts.shadeColor,
	                    set.consts.translucentShade };
	for ( int i = 0; i < 7; ++i )
	{
		std::memcpy( dst[i], src[i], 8 );
	}
}

SVertexCall MakeCall( SPart &part, SVertexFull *pRes, SVertexFull *pStream )
{
	SVertexCall c;
	c.pRes = pRes;
	c.pStream = pStream;
	c.pPositions = &part.srcPos[0].x;
	c.nPositions = static_cast<int>( part.srcPos.size() );
	c.pPosIndices = part.posIndices.data();
	c.pUV = reinterpret_cast<const uint8_t *>( part.uv.data() );
	c.nUVStride = sizeof( NGScene::SUVInfo );
	c.pNormals = &part.normals[0].dw;
	c.pFogOld = part.fogOld.data();
	c.pFogNew = part.fogNew.data();
	c.nFogMask = ~0u;
	c.nCount = static_cast<int>( part.normals.size() );
	return c;
}

enum EVariant
{
	CURRENT,          // shipped directional + frozen multiply x2 + frozen loop + memcpy
	CURRENT_NO_COPY,  // the same without the memcpy
	FUSED_GFX,        // fused into gfxData (+ cache arrays), no copy
	FUSED_GFX_NOFOG,  // the same with nFogMask 0 (one fog byte): bounds the fog cost
	FUSED_GFX_COPY,   // fused into gfxData, then memcpy to the VB
	FUSED_GFX_STREAM, // fused into gfxData and streamed into the VB
	FUSED_VB,         // fused straight into the VB, plain stores, no gfxData
	FUSED_VB_STREAM,  // fused straight into the VB, streaming stores only
	SPLIT_COPY,       // DirectionalMultiply into the cache arrays, Assemble to gfxData, memcpy
	DIR_SHIPPED,      // the shipped directional kernel alone
	DIR_FUSED,        // the branch-free directional kernel alone
	MUL_FROZEN,       // MultiplyOnColor twice, engine loop
	MUL_FUSED,        // MultiplyOnColor twice, SIMD
};

struct SVariantInfo
{
	EVariant e;
	const char *pszName;
};

void RunOnce(
		EVariant e, SPartSet &set, SPart &part, const SFusedLightKernels &fused, const NGScene::SLightingKernels &shipped )
{
	const int n = static_cast<int>( part.normals.size() );
	switch ( e )
	{
		case CURRENT:
		case CURRENT_NO_COPY:
			shipped.pCalcDirectionalLighting(
					part.normals.data(), n, set.ls, set.translucent, part.colors.data(), part.shadow.data() );
			FrozenMultiplyOnColor( part.shadow.data(), part.shadow.data() + n, part.vertexColor.data() );
			FrozenMultiplyOnColor( part.colors.data(), part.colors.data() + n, part.vertexColor.data() );
			FrozenOutputLoop( reinterpret_cast<NGfx::SGeomVecFull *>( part.gfx.data() ),
			                  part.srcPos,
			                  part.uv.data(),
			                  part.posIndices,
			                  part.normals,
			                  part.colors,
			                  part.shadow,
			                  part.fogOld.data(),
			                  part.fogNew.data(),
			                  -1 );
			if ( e == CURRENT )
			{
				std::memcpy( part.pVB, part.gfx.data(), sizeof( SVertexFull ) * n );
			}
			break;
		case FUSED_GFX:
		case FUSED_GFX_COPY:
			fused.pLightAndAssemble( MakeCall( part, part.gfx.data(), nullptr ),
			                         set.consts,
			                         part.vertexColor.data(),
			                         part.colors.data(),
			                         part.shadow.data() );
			if ( e == FUSED_GFX_COPY )
			{
				std::memcpy( part.pVB, part.gfx.data(), sizeof( SVertexFull ) * n );
			}
			break;
		case FUSED_GFX_NOFOG:
		{
			SVertexCall call = MakeCall( part, part.gfx.data(), nullptr );
			call.nFogMask = 0;
			fused.pLightAndAssemble( call, set.consts, part.vertexColor.data(), part.colors.data(), part.shadow.data() );
			break;
		}
		case FUSED_GFX_STREAM:
			fused.pLightAndAssemble( MakeCall( part, part.gfx.data(), part.pVB ),
			                         set.consts,
			                         part.vertexColor.data(),
			                         part.colors.data(),
			                         part.shadow.data() );
			break;
		case FUSED_VB:
			fused.pLightAndAssemble( MakeCall( part, part.pVB, nullptr ),
			                         set.consts,
			                         part.vertexColor.data(),
			                         part.colors.data(),
			                         part.shadow.data() );
			break;
		case FUSED_VB_STREAM:
			fused.pLightAndAssemble( MakeCall( part, nullptr, part.pVB ),
			                         set.consts,
			                         part.vertexColor.data(),
			                         part.colors.data(),
			                         part.shadow.data() );
			break;
		case SPLIT_COPY:
			fused.pDirectionalMultiply(
					&part.normals[0].dw, n, set.consts, part.vertexColor.data(), part.colors.data(), part.shadow.data() );
			fused.pAssembleVertices(
					MakeCall( part, part.gfx.data(), nullptr ), nullptr, part.colors.data(), part.shadow.data() );
			std::memcpy( part.pVB, part.gfx.data(), sizeof( SVertexFull ) * n );
			break;
		case DIR_SHIPPED:
			shipped.pCalcDirectionalLighting(
					part.normals.data(), n, set.ls, set.translucent, part.colors.data(), part.shadow.data() );
			break;
		case DIR_FUSED:
			fused.pDirectionalLighting( &part.normals[0].dw, n, set.consts, part.colors.data(), part.shadow.data() );
			break;
		case MUL_FROZEN:
			FrozenMultiplyOnColor( part.shadow.data(), part.shadow.data() + n, part.vertexColor.data() );
			FrozenMultiplyOnColor( part.colors.data(), part.colors.data() + n, part.vertexColor.data() );
			break;
		case MUL_FUSED:
			fused.pMultiplyOnColor( part.shadow.data(), part.vertexColor.data(), n );
			fused.pMultiplyOnColor( part.colors.data(), part.vertexColor.data(), n );
			break;
	}
}

// nIsa: 0 SSE2, 1 AVX2, 2 AVX2 without the fog gather.
void BM_Light( benchmark::State &state, EVariant e, int nIsa, double fRepeat, bool bCold )
{
	const bool bAVX2 = nIsa != 0;
	if ( bAVX2 && !NGScene::IsAVX2Present() )
	{
		state.SkipWithError( "CPU lacks AVX2" );
		return;
	}
	const SFusedLightKernels &fused = nIsa == 0   ? NLightFused::sse2FusedLightKernels
	                                  : nIsa == 1 ? NLightFused::avx2FusedLightKernels
	                                              : NLightFused::avx2NoGatherFusedLightKernels;
	const NGScene::SLightingKernels &shipped = bAVX2 ? NGScene::avx2LightingKernels : NGScene::sse2LightingKernels;
	const int nCount = static_cast<int>( state.range( 0 ) );
	SPartSet set;
	MakeSet( set, nCount, fRepeat, bCold );
	size_t p = 0;
	for ( auto _ : state )
	{
		RunOnce( e, set, set.parts[p], fused, shipped );
		benchmark::ClobberMemory();
		if ( ++p == set.parts.size() )
		{
			p = 0;
		}
	}
	NLightFused::StreamFence();
	state.SetItemsProcessed( state.iterations() * nCount );
	state.counters["per_vertex"] = benchmark::Counter(
			static_cast<double>( nCount ), benchmark::Counter::kIsIterationInvariantRate | benchmark::Counter::kInvert );
}

int RegisterAll()
{
	const SVariantInfo pipeline[] = { { CURRENT, "Current" },
	                                  { CURRENT_NO_COPY, "CurrentNoCopy" },
	                                  { FUSED_GFX, "FusedGfx" },
	                                  { FUSED_GFX_NOFOG, "FusedGfxFogConst" },
	                                  { FUSED_GFX_COPY, "FusedGfxCopy" },
	                                  { FUSED_GFX_STREAM, "FusedGfxStreamVB" },
	                                  { FUSED_VB, "FusedVB" },
	                                  { FUSED_VB_STREAM, "FusedVBStream" },
	                                  { SPLIT_COPY, "SplitCopy" } };
	const SVariantInfo pieces[] = {
		{ DIR_SHIPPED, "DirShipped" }, { DIR_FUSED, "DirFused" }, { MUL_FROZEN, "MulFrozen" }, { MUL_FUSED, "MulFused" } };
	for ( const bool bCold : { false, true } )
	{
		const std::string temp = bCold ? "Cold/" : "Hot/";
		for ( const int nIsa : { 1, 2, 0 } )
		{
			const std::string isa = nIsa == 0 ? "sse2/" : ( nIsa == 1 ? "avx2/" : "avx2ng/" );
			// The whole pass at 30% repeated normals, a flat-shaded-ish mix.
			for ( const SVariantInfo &v : pipeline )
			{
				const std::string name = temp + isa + v.pszName;
				benchmark::RegisterBenchmark( name.c_str(), BM_Light, v.e, nIsa, 0.3, bCold )->Arg( 80 )->Arg( 600 );
			}
			for ( const SVariantInfo &v : pieces )
			{
				// The pieces never touch the fog, so the no-gather set adds nothing.
				if ( nIsa == 2 )
				{
					continue;
				}
				for ( const double fRepeat : { 0.0, 0.3, 0.7 } )
				{
					if ( ( v.e == MUL_FROZEN || v.e == MUL_FUSED ) && fRepeat != 0.3 )
					{
						continue;
					}
					const std::string name = temp + isa + v.pszName + "/repeat" + std::to_string( int( fRepeat * 100 + 0.5 ) );
					benchmark::RegisterBenchmark( name.c_str(), BM_Light, v.e, nIsa, fRepeat, bCold )->Arg( 80 )->Arg( 600 );
				}
			}
		}
	}
	return 0;
}

const int nRegistered = RegisterAll();

} // namespace

BENCHMARK_MAIN();
