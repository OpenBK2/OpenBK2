// Measures how far the shipping MMXTransformVector implementations sit from the
// original MMX, and tests whether an integer emulation closes the gap.
//
// MMXTransformVector2 and 3 were never made bit-exact. This asks why, by comparing
// two candidate implementations against the original for the single-transform case
// first, where the answer is not confounded by weighting:
//
//   glm   the shipping implementation: full float matrix, float normalize
//   emu   the same pipeline written with the mmx:: integer helpers
//
// The original quantises the matrix to 16-bit fixed point in Assign() before it
// multiplies anything, then works in saturating 16-bit and normalizes through a
// lookup table. If that is what the divergence is made of, no float implementation
// can close it and the integer one should match exactly.
//
// x86 only: it compares against the inline __asm in test/original.h.

#include "3Dmotor/stdafx.h"
#include "3Dmotor/GSSETransform.h"

#include "../../original.h"
#include "../../random.h"

#include <cstdio>
#include <cstdlib>

namespace {

uint64_t Combine( const NGfx::SMMXWord &w )
{
    return mmx::combine64( w.nZ, w.nY, w.nX, w.nW );
}

// The MMXTransformVectorImpl pipeline, instruction for instruction, in mmx::.
uint32_t EmulateTransform1( uint32_t nSrc, const SMMXFixups &fx,
    const NGfx::SCompactTransformer &t, const short *pNormalizeTable )
{
    const uint32_t nW = nSrc & 0xff000000u;
    const uint64_t mm7 = nSrc & 0x00ffffffu;

    uint64_t mm0 = mmx::punpcklbw( 0, mm7 );
    mm0 = mmx::psubw( mm0, Combine( fx.normalFixup ) );

    const uint64_t a = Combine( t.a ), b = Combine( t.b ), c = Combine( t.c );

    uint64_t mm1 = mmx::pmulhw( mm0, a );          // z y x
    uint64_t mm2 = mmx::psllq( mm0, 16 );
    uint64_t mm3 = mmx::psrlq( mm0, 32 );
    mm2 = mmx::paddw( mm2, mm3 );                  // x z y
    mm2 = mmx::pmulhw( mm2, b );
    mm3 = mmx::psllq( mm0, 32 );
    uint64_t mm4 = mmx::psrlq( mm0, 16 );
    mm1 = mmx::paddsw( mm1, mm2 );
    mm3 = mmx::paddw( mm3, mm4 );                  // y x z
    mm3 = mmx::pmulhw( mm3, c );
    mm1 = mmx::paddsw( mm1, mm3 );

    // normalize
    mm1 = mmx::psllw( mm1, 3 );
    mm2 = mmx::pmaddwd( mm1, mm1 );
    mm3 = mmx::psrlq( mm2, 32 );
    mm2 = mmx::paddd( mm2, mm3 );
    const uint32_t nIndex = static_cast<uint32_t>( mm2 & 0xffffffffu ) >> 18;
    const uint64_t nScale = static_cast<uint16_t>( pNormalizeTable[nIndex] );
    uint64_t mm2b = mmx::punpcklwd( nScale, nScale );
    mm2b = mmx::punpckldq( mm2b, mm2b );
    mm1 = mmx::pmulhw( mm1, mm2b );
    mm1 = mmx::psllw( mm1, 5 );

    // pack and merge the original w back in
    mm1 = mmx::paddw( mm1, Combine( fx.shiftedFixup ) );
    mm1 = mmx::psrlw( mm1, 8 );
    const uint64_t nPacked = mmx::packuswb( mm1, mm1 );
    return static_cast<uint32_t>( nPacked & 0xffffffffu ) | nW;
}

struct SStats
{
    const char *pszName;
    long long nCases = 0;
    long long nExact = 0;
    int nMaxDiff = 0;
    long long hist[6] = {};   // |diff| of 0, 1, 2, 3, 4, >=5
};

void Accumulate( SStats &s, const NGfx::SCompactVector &ref, const NGfx::SCompactVector &got )
{
    const int d[3] =
    {
        abs( static_cast<int>( ref.x ) - static_cast<int>( got.x ) ),
        abs( static_cast<int>( ref.y ) - static_cast<int>( got.y ) ),
        abs( static_cast<int>( ref.z ) - static_cast<int>( got.z ) ),
    };
    ++s.nCases;
    if ( d[0] == 0 && d[1] == 0 && d[2] == 0 )
        ++s.nExact;
    for ( int k = 0; k < 3; ++k )
    {
        if ( d[k] > s.nMaxDiff )
            s.nMaxDiff = d[k];
        s.hist[d[k] < 5 ? d[k] : 5] += 1;
    }
}

void Report( const SStats &s )
{
    const double fExact = s.nCases ? ( 100.0 * s.nExact ) / s.nCases : 0.0;
    const long long nComponents = s.nCases * 3;
    printf( "%-8s exact %8lld / %8lld (%6.2f%%)  max |diff| %d\n",
        s.pszName, s.nExact, s.nCases, fExact, s.nMaxDiff );
    printf( "         per component |diff|:" );
    for ( int k = 0; k < 6; ++k )
    {
        const double f = nComponents ? ( 100.0 * s.hist[k] ) / nComponents : 0.0;
        if ( k < 5 )
            printf( "  %d:%.2f%%", k, f );
        else
            printf( "  >=5:%.2f%%", f );
    }
    printf( "\n" );
}

}

int main()
{
    enum { iterations = 200000 };

    SStats glmStats{ "glm" }, emuStats{ "emu" };

    for ( int i = 0; i < iterations; ++i )
    {
        NGfx::SCompactVector src{};
        SHMatrix transform{};
        randomizeNormalVector( src );
        randomizeMatrix( transform );

        NGfx::SCompactVector ref{}, viaGlm{}, viaEmu{};

        original::MMXTransformVector( ref, src, transform );
        MMXTransformVector( viaGlm, src, transform );

        NGfx::SCompactTransformer compactTransform;
        Assign( &compactTransform, transform );
        viaEmu.dw = EmulateTransform1( src.dw, fixups, compactTransform, nNormalizeTable );
        viaEmu.w = src.w;

        Accumulate( glmStats, ref, viaGlm );
        Accumulate( emuStats, ref, viaEmu );
    }

    printf( "MMXTransformVector, %d random normal/rotation pairs, vs the original MMX\n\n",
        static_cast<int>( iterations ) );
    Report( glmStats );
    Report( emuStats );

    // MMXTransformVector2 and 3, against the legacy inline __asm in original.h, with
    // the same weight distribution MMXTransformVector_test uses. This is the divergence
    // that test has been failing on; measuring it against the inline original shows the
    // failure has nothing to do with the move to MASM.
    SStats glm2{ "glm2" }, glm3{ "glm3" };
    for ( int i = 0; i < iterations; ++i )
    {
        NGfx::SCompactVector src{};
        SHMatrix m1{}, m2{}, m3{};
        randomizeNormalVector( src );
        randomizeMatrix( m1 );
        randomizeMatrix( m2 );
        randomizeMatrix( m3 );

        NGfx::SCompactVector ref{}, got{};

        const uint8_t w1 = random_uint8();
        const uint8_t w2 = static_cast<uint8_t>( 255 - w1 );
        original::MMXTransformVector2( ref, src, m1, w1, m2, w2 );
        MMXTransformVector2( got, src, m1, w1, m2, w2 );
        Accumulate( glm2, ref, got );

        const uint8_t v1 = static_cast<uint8_t>( random_uint8() / 2 );
        const uint8_t v2 = static_cast<uint8_t>( random_uint8() / 2 );
        const uint8_t v3 = static_cast<uint8_t>( 255 - v1 - v2 );
        original::MMXTransformVector3( ref, src, m1, v1, m2, v2, m3, v3 );
        MMXTransformVector3( got, src, m1, v1, m2, v2, m3, v3 );
        Accumulate( glm3, ref, got );
    }

    printf( "\nweighted variants, weights as MMXTransformVector_test draws them\n\n" );
    Report( glm2 );
    Report( glm3 );
    return 0;
}
