// Compares the three per-vertex lighting kernel sets against each other.
//
// This is the measurement PR #132 needed and did not have: whether the AVX2 kernels
// are worth their maintenance cost over the SSE2 ones, and what either buys over the
// scalar reference. Each set is timed on the same inputs, built once outside the
// timed loop.
//
// Counts are per call. The engine transforms one object's vertices at a time, and
// GCombiner.cpp notes these are "often very small in size.. (less than 128 elements)",
// so 128 is the realistic case and 1024 shows the asymptote.

#include <benchmark/benchmark.h>

#include "3Dmotor/stdafx.h"
#include "3Dmotor/GLightPerVertexKernels.h"
#include "3Dmotor/GSSEtransform.h"
#include "3Dmotor/GSSEtransform.cpp"
#include "3Dmotor/test/random.h"

#include <vector>

namespace {

enum { fogSize = 33 };

struct SInputs
{
    std::vector<NGfx::SCompactVector> normals;
    std::vector<NGfx::SCompactVector> transp;
    std::vector<WORD> posIndices;
    std::vector<NGfx::SMMXWord> attenuation;
    std::vector<NGfx::SMMXWord> colors;
    std::vector<DWORD> src;
    std::vector<DWORD> dst;
    std::vector<CVec3> pos;
    std::vector<int> coords;
    std::vector<unsigned char> fog;
    std::vector<unsigned char> scale;
    std::vector<unsigned char> fogRes;
    NGScene::SPerVertexLightState ls;
    NGfx::SMMXWord translucent, lightColor;

    explicit SInputs( int n )
    {
        normals.resize( n );
        transp.resize( n );
        posIndices.resize( n );
        attenuation.resize( n );
        colors.resize( n );
        src.resize( n );
        dst.resize( n );
        pos.resize( n );
        coords.resize( n * 2 );
        fogRes.resize( n );
        fog.resize( fogSize * fogSize );
        scale.resize( 256 );

        auto word = []( NGfx::SMMXWord &w )
        {
            w.nX = random_uint16(); w.nY = random_uint16();
            w.nZ = random_uint16(); w.nW = random_uint16();
        };
        for ( int k = 0; k < n; ++k )
        {
            normals[k].dw = random_uint32();
            transp[k].dw = random_uint32();
            posIndices[k] = static_cast<WORD>( random_uint32() % n );
            word( attenuation[k] );
            word( colors[k] );
            src[k] = random_uint32();
            pos[k] = CVec3( random_float(), random_float(), random_float() );
            coords[k * 2] = static_cast<int>( random_uint32() & 0x000fffff );
            coords[k * 2 + 1] = static_cast<int>( random_uint32() & 0x000fffff );
        }
        for ( size_t k = 0; k < fog.size(); ++k )
            fog[k] = random_uint8();
        for ( int k = 0; k < 256; ++k )
            scale[k] = random_uint8();

        word( ls.ambient ); word( ls.lightColor ); word( ls.incidentShadowColor );
        word( ls.shadeColor ); word( ls.dirLight ); word( ls.shift );
        word( translucent ); word( lightColor );
    }
};

const NGScene::SLightingKernels *SetFor( int nIndex )
{
    switch ( nIndex )
    {
    case 0: return &NGScene::refLightingKernels;
    case 1: return NGScene::IsSSE2Present() ? &NGScene::sse2LightingKernels : nullptr;
    default: return NGScene::IsAVX2Present() ? &NGScene::avx2LightingKernels : nullptr;
    }
}

// state.range(0) selects the set, state.range(1) the vertex count.
#define LIGHTING_BENCH( name, body )                                              \
    static void name( benchmark::State &state )                                   \
    {                                                                             \
        const NGScene::SLightingKernels *pK = SetFor( (int)state.range( 0 ) );    \
        if ( !pK ) { state.SkipWithError( "not supported on this CPU" ); return; } \
        const int n = (int)state.range( 1 );                                      \
        SInputs in( n );                                                          \
        std::vector<DWORD> outA( n ), outB( n );                                  \
        for ( auto _ : state ) { body; benchmark::ClobberMemory(); }              \
        state.SetLabel( pK->pszName );                                            \
    }                                                                             \
    BENCHMARK( name )->ArgsProduct( { { 0, 1, 2 }, { 128, 1024 } } )

LIGHTING_BENCH( BM_CalcDirectionalLighting,
    pK->pCalcDirectionalLighting( &in.normals[0], n, in.ls, in.translucent, &outA[0], &outB[0] ) );

LIGHTING_BENCH( BM_SampleWarFogCoords,
    pK->pSampleWarFogCoords( &in.pos[0], n, 0.25f, &in.coords[0] ) );

LIGHTING_BENCH( BM_SampleWarFogInt,
    pK->pSampleWarFogInt( &in.coords[0], &in.fog[0], fogSize, &in.fogRes[0], n ) );

LIGHTING_BENCH( BM_CalcPointLightAttenuation,
    pK->pCalcPointLightAttenuation( &in.attenuation[0], &in.pos[0], n,
        CVec3( 0.5f, 0.5f, 0.5f ), 4.0f ) );

LIGHTING_BENCH( BM_CalcPointLightColorsIndexed,
    pK->pCalcPointLightColorsIndexed( &in.colors[0], &in.attenuation[0], &in.posIndices[0],
        &in.normals[0], n, in.lightColor ) );

LIGHTING_BENCH( BM_CalcPointLightColorsUniform,
    pK->pCalcPointLightColorsUniform( &in.colors[0], in.attenuation[0], &in.normals[0],
        sizeof( NGfx::SCompactVector ), n, in.lightColor ) );

LIGHTING_BENCH( BM_AddColors,
    pK->pAddColors( &in.dst[0], &in.src[0], &in.colors[0], n ) );

LIGHTING_BENCH( BM_ScaleColors,
    pK->pScaleColors( &in.dst[0], &in.src[0], sizeof( DWORD ), &in.scale[0], 0xff,
        &in.posIndices[0], &in.transp[0], n, false ) );

}

BENCHMARK_MAIN();
