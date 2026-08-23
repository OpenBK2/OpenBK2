// Proves that the MMX pulled out into test/original/*.asm computes bit-identical
// results to the __asm block it was lifted from.
//
// This is buildable on x86 only, because x86 is the only architecture where MSVC
// still accepts the inline form, so it is the only place the two can be run against
// each other at all. What it establishes is that the extraction was faithful. The
// x64 half of each .asm uses a different calling convention and is not covered here;
// that is covered by running the bit-exactness tests on both x86 and x64, where a
// broken x64 reference would disagree with the C++ kernels on x64 while x86 passed.
//
// Deliberately small: no googletest, and a fixed seed, so it builds and runs in
// seconds and any failure reproduces exactly.

// Geom.h uses WORD and friends, so the prelude has to come first, as everywhere
// else in this codebase.
#include "3Dmotor/stdafx.h"
#include "SentinelRef.h"

#include <cstdio>
#include <random>

namespace {

// Fixed seed on purpose: a sentinel that fails differently on every run is not one
// you can bisect against.
std::mt19937 rng( 0x5EED1234u );

uint32_t NextU32()
{
    return std::uniform_int_distribution<uint32_t>()( rng );
}

short NextWord()
{
    return static_cast<short>( std::uniform_int_distribution<int>( -32768, 32767 )( rng ) );
}

void RandomMMXWord( NGfx::SMMXWord &w )
{
    w.nX = NextWord();
    w.nY = NextWord();
    w.nZ = NextWord();
    w.nW = NextWord();
}

}

int main()
{
    enum { iterations = 200000 };

    // pDirData points at six consecutive SMMXWords: ambient, lightColor,
    // incidentShadowColor, shadeColor, dirLight, shift. That layout is what both
    // implementations index as [pDirData + n * 8], and GLightPerVertex.h
    // static_asserts that SPerVertexLightState really does start with them.
    NGfx::SMMXWord dirData[6];
    NGfx::SMMXWord translucentShade;

    int nFailures = 0;
    for ( int i = 0; i < iterations; ++i )
    {
        for ( int k = 0; k < 6; ++k )
            RandomMMXWord( dirData[k] );
        RandomMMXWord( translucentShade );
        const uint32_t dwNormal = NextU32();

        const uint64_t nInline = InlineCalcDirectionalLightingMMX( dirData, &translucentShade, dwNormal );
        const uint64_t nAsm = CalcDirectionalLightingMMX( dirData, &translucentShade, dwNormal );

        if ( nInline == nAsm )
            continue;

        printf( "MISMATCH iteration %d: normal %08lX inline color %08lX shadow %08lX,"
                " asm color %08lX shadow %08lX\n",
            i, static_cast<unsigned long>( dwNormal ),
            static_cast<unsigned long>( nInline & 0xffffffffu ),
            static_cast<unsigned long>( nInline >> 32 ),
            static_cast<unsigned long>( nAsm & 0xffffffffu ),
            static_cast<unsigned long>( nAsm >> 32 ) );
        if ( ++nFailures > 10 )
        {
            printf( "too many mismatches, stopping\n" );
            break;
        }
    }

    if ( nFailures != 0 )
    {
        printf( "CalcDirectionalLightingMMX: FAILED, %d mismatching results\n", nFailures );
        return 1;
    }

    printf( "CalcDirectionalLightingMMX: %d random cases bit-identical\n",
        static_cast<int>( iterations ) );
    return 0;
}
