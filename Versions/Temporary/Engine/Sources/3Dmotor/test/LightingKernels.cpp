// Checks every per-vertex lighting kernel set against the scalar reference.
//
// The other tests in this directory drive the kernels through the adapters in
// GLightPerVertex.cpp, which call GetLightingKernels() and therefore only ever
// exercise the best set the running CPU supports. On an AVX2 machine that leaves the
// SSE2 path completely untested. This drives refLightingKernels, sse2LightingKernels
// and avx2LightingKernels directly and compares them element by element.
//
// refLightingKernels is the mmx:: transcription of the original MMX, checked against
// the assembly itself elsewhere, so it is the right thing to hold the others to.
//
// Counts run from 1 upward so the remainder paths get covered: SSE2 handles two
// vertices per iteration and AVX2 four, and the tail is where indexing mistakes live.

#include "3Dmotor/stdafx.h"
#include "3Dmotor/GLightPerVertexKernels.h"
#include "3Dmotor/GSSEtransform.h"
// Brings the initialised nCubicRoot that AddColors indexes. A zeroed stand-in would
// make every AddColors result zero and the comparison vacuous.
#include "3Dmotor/GSSEtransform.cpp"

#include <gtest/gtest.h>

#include <random>
#include <vector>

namespace {

// random.h seeds from std::random_device, so a failure there cannot be reproduced.
// These comparisons are exactly the kind that fail on one input in a million, so the
// generator is local and fixed-seeded: the same run every time, and a failure can be
// bisected.
std::mt19937 rng( 0xA57C0DE1u );

uint32_t random_uint32() { return std::uniform_int_distribution<uint32_t>()( rng ); }
uint16_t random_uint16() { return (uint16_t)std::uniform_int_distribution<int>( -32768, 32767 )( rng ); }
unsigned char random_uint8() { return (unsigned char)std::uniform_int_distribution<int>( 0, 255 )( rng ); }
float random_float() { return std::uniform_real_distribution<float>( -1.0f, 1.0f )( rng ); }

struct SKernelSet
{
    const char *pszName;
    const NGScene::SLightingKernels *pKernels;
};

// Only the sets this CPU can actually execute. Entering avx2LightingKernels without
// AVX2 would fault.
std::vector<SKernelSet> Candidates()
{
    std::vector<SKernelSet> sets;
    if ( NGScene::IsSSE2Present() )
        sets.push_back( { "sse2", &NGScene::sse2LightingKernels } );
    if ( NGScene::IsAVX2Present() )
        sets.push_back( { "avx2", &NGScene::avx2LightingKernels } );
    return sets;
}

const NGScene::SLightingKernels &Ref()
{
    return NGScene::refLightingKernels;
}

// 1..17 covers every remainder case for widths of 2 and 4 several times over; 64 and
// 257 exercise the steady state and an awkward tail after it.
const std::vector<int> &Counts()
{
    static const std::vector<int> counts =
        { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 64, 257 };
    return counts;
}

NGfx::SMMXWord RandomWord()
{
    NGfx::SMMXWord w;
    w.nX = random_uint16();
    w.nY = random_uint16();
    w.nZ = random_uint16();
    w.nW = random_uint16();
    return w;
}

std::vector<NGfx::SCompactVector> RandomNormals( int n )
{
    std::vector<NGfx::SCompactVector> v( n );
    for ( int k = 0; k < n; ++k )
        v[k].dw = random_uint32();
    return v;
}

}

TEST(LightingKernels, CalcDirectionalLighting) {
    for ( const SKernelSet &set : Candidates() ) {
        for ( int n : Counts() ) {
            const std::vector<NGfx::SCompactVector> normals = RandomNormals( n );
            NGScene::SPerVertexLightState ls;
            ls.ambient = RandomWord();
            ls.lightColor = RandomWord();
            ls.incidentShadowColor = RandomWord();
            ls.shadeColor = RandomWord();
            ls.dirLight = RandomWord();
            ls.shift = RandomWord();
            const NGfx::SMMXWord translucent = RandomWord();

            std::vector<DWORD> refColors( n ), refShadow( n ), gotColors( n ), gotShadow( n );
            Ref().pCalcDirectionalLighting( &normals[0], n, ls, translucent, &refColors[0], &refShadow[0] );
            set.pKernels->pCalcDirectionalLighting( &normals[0], n, ls, translucent, &gotColors[0], &gotShadow[0] );

            EXPECT_EQ( refColors, gotColors ) << set.pszName << " n=" << n;
            EXPECT_EQ( refShadow, gotShadow ) << set.pszName << " n=" << n;
        }
    }
}

TEST(LightingKernels, SampleWarFogCoords) {
    for ( const SKernelSet &set : Candidates() ) {
        for ( int n : Counts() ) {
            std::vector<CVec3> pos( n );
            for ( int k = 0; k < n; ++k )
                pos[k] = CVec3( random_float(), random_float(), random_float() );
            const float fScale = random_float();

            std::vector<int> ref( n * 2, 0 ), got( n * 2, 0 );
            Ref().pSampleWarFogCoords( &pos[0], n, fScale, &ref[0] );
            set.pKernels->pSampleWarFogCoords( &pos[0], n, fScale, &got[0] );

            EXPECT_EQ( ref, got ) << set.pszName << " n=" << n;
        }
    }
}

TEST(LightingKernels, SampleWarFogInt) {
    enum { fogSize = 33 };   // GetNextPow2(n-1)+1, as the kernels assert
    for ( const SKernelSet &set : Candidates() ) {
        for ( int n : Counts() ) {
            std::vector<unsigned char> fog( fogSize * fogSize );
            for ( size_t k = 0; k < fog.size(); ++k )
                fog[k] = random_uint8();
            std::vector<int> coords( n * 2 );
            for ( int k = 0; k < n * 2; ++k )
                coords[k] = static_cast<int>( random_uint32() & 0x000fffff );

            std::vector<unsigned char> ref( n, 0 ), got( n, 0 );
            Ref().pSampleWarFogInt( &coords[0], &fog[0], fogSize, &ref[0], n );
            set.pKernels->pSampleWarFogInt( &coords[0], &fog[0], fogSize, &got[0], n );

            EXPECT_EQ( ref, got ) << set.pszName << " n=" << n;
        }
    }
}

// The only kernel that is not bit-exact, and deliberately so: the SIMD versions use
// _mm_rcp_ss and _mm_rsqrt_ss where the reference divides and takes an exact square
// root, and round to nearest where Float2Int truncates. Both are approximations of a
// value that is then scaled by N_PL_ATTENUATION_SCALE and packed into a short, so the
// result is expected to differ by a small amount rather than not at all. The bound is
// asserted rather than the equality, and the observed worst case is reported so a
// regression that widens it shows up as a number rather than as a pass.
TEST(LightingKernels, CalcPointLightAttenuation) {
    int nWorst = 0;
    for ( const SKernelSet &set : Candidates() ) {
        for ( int n : Counts() ) {
            std::vector<CVec3> pos( n );
            for ( int k = 0; k < n; ++k )
                pos[k] = CVec3( random_float(), random_float(), random_float() );
            const CVec3 vCenter( random_float(), random_float(), random_float() );
            const float fRadius = 1.0f + fabsf( random_float() ) * 8.0f;

            std::vector<NGfx::SMMXWord> ref( n ), got( n );
            Ref().pCalcPointLightAttenuation( &ref[0], &pos[0], n, vCenter, fRadius );
            set.pKernels->pCalcPointLightAttenuation( &got[0], &pos[0], n, vCenter, fRadius );

            for ( int k = 0; k < n; ++k ) {
                const int v[3] = { ref[k].nX, ref[k].nY, ref[k].nZ };
                const int g[3] = { got[k].nX, got[k].nY, got[k].nZ };
                for ( int c = 0; c < 3; ++c ) {
                    const int d = abs( v[c] - g[c] );
                    if ( d > nWorst )
                        nWorst = d;
                    // rcp and rsqrt are each accurate to about 1.5 * 2^-12 relative, and
                    // the result is scaled by N_PL_ATTENUATION_SCALE before it is packed
                    // into a short, so the absolute error grows with the value. Allow
                    // roughly a tenth of a percent plus one for the rounding mode, which
                    // is loose enough for the approximation and tight enough that a real
                    // regression, or a component going wrong outright, still fails.
                    const int nAllowed = 1 + ( abs( v[c] ) >> 10 );
                    EXPECT_LE( d, nAllowed )
                        << set.pszName << " n=" << n << " k=" << k << " component=" << c
                        << " reference=" << v[c];
                }
            }
        }
    }
    RecordProperty( "worst_abs_difference", nWorst );
    printf( "[   INFO   ] worst component difference from the reference: %d\n", nWorst );
}

TEST(LightingKernels, CalcPointLightColorsIndexed) {
    for ( const SKernelSet &set : Candidates() ) {
        for ( int n : Counts() ) {
            const std::vector<NGfx::SCompactVector> normals = RandomNormals( n );
            std::vector<WORD> posIndices( n );
            std::vector<NGfx::SMMXWord> attenuation( n );
            for ( int k = 0; k < n; ++k ) {
                posIndices[k] = static_cast<WORD>( random_uint32() % n );
                attenuation[k] = RandomWord();
            }
            const NGfx::SMMXWord lightColor = RandomWord();

            std::vector<NGfx::SMMXWord> ref( n ), got( n );
            for ( int k = 0; k < n; ++k )
                ref[k] = got[k] = RandomWord();   // the kernels accumulate into pRes

            Ref().pCalcPointLightColorsIndexed(
                &ref[0], &attenuation[0], &posIndices[0], &normals[0], n, lightColor );
            set.pKernels->pCalcPointLightColorsIndexed(
                &got[0], &attenuation[0], &posIndices[0], &normals[0], n, lightColor );

            for ( int k = 0; k < n; ++k ) {
                EXPECT_EQ( ref[k].nX, got[k].nX ) << set.pszName << " n=" << n << " k=" << k;
                EXPECT_EQ( ref[k].nY, got[k].nY ) << set.pszName << " n=" << n << " k=" << k;
                EXPECT_EQ( ref[k].nZ, got[k].nZ ) << set.pszName << " n=" << n << " k=" << k;
            }
        }
    }
}

TEST(LightingKernels, CalcPointLightColorsUniform) {
    for ( const SKernelSet &set : Candidates() ) {
        for ( int n : Counts() ) {
            const std::vector<NGfx::SCompactVector> normals = RandomNormals( n );
            const NGfx::SMMXWord attenuation = RandomWord();
            const NGfx::SMMXWord lightColor = RandomWord();

            std::vector<NGfx::SMMXWord> ref( n ), got( n );
            for ( int k = 0; k < n; ++k )
                ref[k] = got[k] = RandomWord();

            const int nStride = sizeof( NGfx::SCompactVector );
            Ref().pCalcPointLightColorsUniform(
                &ref[0], attenuation, &normals[0], nStride, n, lightColor );
            set.pKernels->pCalcPointLightColorsUniform(
                &got[0], attenuation, &normals[0], nStride, n, lightColor );

            for ( int k = 0; k < n; ++k ) {
                EXPECT_EQ( ref[k].nX, got[k].nX ) << set.pszName << " n=" << n << " k=" << k;
                EXPECT_EQ( ref[k].nY, got[k].nY ) << set.pszName << " n=" << n << " k=" << k;
                EXPECT_EQ( ref[k].nZ, got[k].nZ ) << set.pszName << " n=" << n << " k=" << k;
            }
        }
    }
}

TEST(LightingKernels, AddColors) {
    for ( const SKernelSet &set : Candidates() ) {
        for ( int n : Counts() ) {
            std::vector<DWORD> src( n );
            std::vector<NGfx::SMMXWord> add( n );
            for ( int k = 0; k < n; ++k ) {
                src[k] = random_uint32();
                add[k] = RandomWord();
            }

            std::vector<DWORD> ref( n, 0 ), got( n, 0 );
            Ref().pAddColors( &ref[0], &src[0], &add[0], n );
            set.pKernels->pAddColors( &got[0], &src[0], &add[0], n );

            EXPECT_EQ( ref, got ) << set.pszName << " n=" << n;
        }
    }
}

TEST(LightingKernels, ScaleColors) {
    for ( const SKernelSet &set : Candidates() ) {
        for ( int n : Counts() ) {
            for ( int nPass = 0; nPass < 2; ++nPass ) {
                const bool bMultiplyOnTransparency = nPass != 0;
                std::vector<DWORD> src( n );
                std::vector<NGfx::SCompactVector> transp( n );
                std::vector<WORD> posIndices( n );
                std::vector<unsigned char> scale( 256 );
                for ( int k = 0; k < n; ++k ) {
                    src[k] = random_uint32();
                    transp[k].dw = random_uint32();
                    posIndices[k] = static_cast<WORD>( random_uint32() );
                }
                for ( int k = 0; k < 256; ++k )
                    scale[k] = random_uint8();

                std::vector<DWORD> ref( n, 0 ), got( n, 0 );
                const int nStride = sizeof( DWORD );
                Ref().pScaleColors( &ref[0], &src[0], nStride, &scale[0], 0xff,
                    &posIndices[0], &transp[0], n, bMultiplyOnTransparency );
                set.pKernels->pScaleColors( &got[0], &src[0], nStride, &scale[0], 0xff,
                    &posIndices[0], &transp[0], n, bMultiplyOnTransparency );

                EXPECT_EQ( ref, got ) << set.pszName << " n=" << n
                    << " multiplyOnTransparency=" << bMultiplyOnTransparency;
            }
        }
    }
}
