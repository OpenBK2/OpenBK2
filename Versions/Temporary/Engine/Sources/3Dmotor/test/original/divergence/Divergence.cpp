// Measures how far the shipping MMXTransformVector implementations sit from the
// original MMX, and tests whether an integer emulation closes the gap.
//
// MMXTransformVector2 and 3 were never made bit-exact. This asks why, by comparing
// two candidate implementations against the original for the single-transform case
// first, where the answer is not confounded by weighting:
//
//   ship  whatever the engine actually compiles. HAS_SSE2 is 1 on both x86 and x64
//         with default flags, so for the single-transform case this is the SSE2
//         intrinsic MMXTransformVector, not the glm one in the #else branch, which
//         no supported configuration ever builds. MMXTransformVector2 and 3 have no
//         SSE2 variant, so those two really are glm.
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
#include <cmath>

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

// The glm implementation from the #else branch of GSSEtransform.h, reproduced here so
// it can be measured. No supported configuration compiles it - HAS_SSE2 is 1 on x86 and
// x64 alike - but it is kept as the fallback a future Linux or ARM port would use, and
// something nothing builds is something nothing tests.
void GlmTransformVector( NGfx::SCompactVector &res, const NGfx::SCompactVector &src,
    const SHMatrix &transform1 )
{
    glm::mat4 matrix1 = LoadMatrix( transform1 );
    glm::vec4 vec = LoadCompactVector( src );
    glm::vec4 result = matrix1 * vec;
    glm::vec3 normal = glm::normalize( glm::vec3{ result.x, result.y, result.z } );
    res = SaveCompactVector( normal );
    res.w = src.w;
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

    SStats shipStats{ "sse2" }, emuStats{ "mmx emu" }, glmStats{ "glm" };
    // The SSE2 version was written to reproduce glm's evaluation order deliberately.
    // Whether it succeeded is worth stating rather than inferring from equal summaries.
    int nSse2EqualsGlm = 0;

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

        NGfx::SCompactVector viaGlmBranch{};
        GlmTransformVector( viaGlmBranch, src, transform );

        if ( viaGlm.dw == viaGlmBranch.dw )
            ++nSse2EqualsGlm;
        Accumulate( shipStats, ref, viaGlm );
        Accumulate( emuStats, ref, viaEmu );
        Accumulate( glmStats, ref, viaGlmBranch );
    }

    printf( "MMXTransformVector, %d random normal/rotation pairs, vs the original MMX\n\n",
        static_cast<int>( iterations ) );
    Report( shipStats );
    Report( glmStats );
    Report( emuStats );
    printf( "\nsse2 and glm agree with each other on %s of cases\n",
        nSse2EqualsGlm == iterations ? "100%" : "NOT all" );

    // MMXTransformVector2 and 3, against the legacy inline __asm in original.h, with
    // the same weight distribution MMXTransformVector_test uses. This is the divergence
    // that test has been failing on; measuring it against the inline original shows the
    // failure has nothing to do with the move to MASM.
    SStats ship2{ "glm2" }, ship3{ "glm3" };
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
        Accumulate( ship2, ref, got );

        const uint8_t v1 = static_cast<uint8_t>( random_uint8() / 2 );
        const uint8_t v2 = static_cast<uint8_t>( random_uint8() / 2 );
        const uint8_t v3 = static_cast<uint8_t>( 255 - v1 - v2 );
        original::MMXTransformVector3( ref, src, m1, v1, m2, v2, m3, v3 );
        MMXTransformVector3( got, src, m1, v1, m2, v2, m3, v3 );
        Accumulate( ship3, ref, got );
    }

    printf( "\nweighted variants, weights as MMXTransformVector_test draws them\n\n" );
    Report( ship2 );
    Report( ship3 );

    // How far apart are the two blended transforms? Independent random rotations are
    // the worst case and not what skinning does: neighbouring bones differ by a small
    // angle, and a blend of two nearly aligned rotations is well conditioned. Bucket
    // the error by the relative rotation angle to see which regime the big errors are
    // in. Angle comes from trace(R1^T R2) = 1 + 2 cos(theta) on the 3x3 parts.
    struct SBucket { long long n = 0, exact = 0; int worst = 0; };
    SBucket buckets[9];
    for ( int i = 0; i < iterations; ++i )
    {
        NGfx::SCompactVector src{};
        SHMatrix m1{}, m2{};
        randomizeNormalVector( src );
        randomizeMatrix( m1 );
        randomizeMatrix( m2 );

        double fTrace = 0;
        for ( int r = 0; r < 3; ++r )
        {
            for ( int c = 0; c < 3; ++c )
                fTrace += (double)m1.m[r][c] * m2.m[r][c];
        }
        double fCos = ( fTrace - 1.0 ) * 0.5;
        fCos = fCos > 1.0 ? 1.0 : ( fCos < -1.0 ? -1.0 : fCos );
        const double fDeg = acos( fCos ) * 180.0 / 3.14159265358979;
        int b = (int)( fDeg / 20.0 );
        if ( b > 8 ) b = 8;

        NGfx::SCompactVector ref{}, got{};
        const uint8_t w1 = random_uint8();
        const uint8_t w2 = (uint8_t)( 255 - w1 );
        original::MMXTransformVector2( ref, src, m1, w1, m2, w2 );
        MMXTransformVector2( got, src, m1, w1, m2, w2 );

        const int d0 = abs( (int)ref.x - (int)got.x );
        const int d1 = abs( (int)ref.y - (int)got.y );
        const int d2 = abs( (int)ref.z - (int)got.z );
        const int dm = d0 > d1 ? ( d0 > d2 ? d0 : d2 ) : ( d1 > d2 ? d1 : d2 );
        SBucket &bk = buckets[b];
        ++bk.n;
        if ( dm == 0 ) ++bk.exact;
        if ( dm > bk.worst ) bk.worst = dm;
    }

    printf( "\n\nMMXTransformVector2 error vs angle between the two transforms\n" );
    printf( "  angle      samples   exact     worst |diff|\n" );
    for ( int b = 0; b < 9; ++b )
    {
        const SBucket &bk = buckets[b];
        if ( bk.n == 0 ) continue;
        printf( "  %3d-%3d  %10lld  %6.2f%%  %6d\n", b * 20, b * 20 + 20,
            bk.n, ( 100.0 * bk.exact ) / bk.n, bk.worst );
    }

    // Is the original stable where glm is not? Bucket the same two-bone comparison by
    // the length of the blended normal before normalization. A near-zero blend has no
    // meaningful direction, so if the large errors live there, both implementations are
    // producing an arbitrary normal and they merely disagree about which one.
    struct SLenBucket { long long n = 0, exact = 0; int worst = 0; };
    SLenBucket lens[6];
    for ( int i = 0; i < iterations; ++i )
    {
        NGfx::SCompactVector src{};
        SHMatrix m1{}, m2{};
        randomizeNormalVector( src );
        randomizeMatrix( m1 );
        randomizeMatrix( m2 );
        const uint8_t w1 = random_uint8();
        const uint8_t w2 = (uint8_t)( 255 - w1 );

        // The same blend MMXTransformVector2 forms, in float, before it normalizes.
        const glm::vec4 v = LoadCompactVector( src );
        const glm::vec4 r1 = LoadMatrix( m1 ) * v;
        const glm::vec4 r2 = LoadMatrix( m2 ) * v;
        const glm::vec4 blended = r1 * ( w1 / 255.f ) + r2 * ( w2 / 255.f );
        const float fLen = glm::length( glm::vec3( blended.x, blended.y, blended.z ) );

        int b;
        if ( fLen < 0.01f )      b = 0;
        else if ( fLen < 0.05f ) b = 1;
        else if ( fLen < 0.10f ) b = 2;
        else if ( fLen < 0.25f ) b = 3;
        else if ( fLen < 0.50f ) b = 4;
        else                     b = 5;

        NGfx::SCompactVector ref{}, got{};
        original::MMXTransformVector2( ref, src, m1, w1, m2, w2 );
        MMXTransformVector2( got, src, m1, w1, m2, w2 );
        const int d0 = abs( (int)ref.x - (int)got.x );
        const int d1 = abs( (int)ref.y - (int)got.y );
        const int d2 = abs( (int)ref.z - (int)got.z );
        const int dm = d0 > d1 ? ( d0 > d2 ? d0 : d2 ) : ( d1 > d2 ? d1 : d2 );
        SLenBucket &bk = lens[b];
        ++bk.n;
        if ( dm == 0 ) ++bk.exact;
        if ( dm > bk.worst ) bk.worst = dm;
    }

    static const char *pszLen[6] =
        { "     < 0.01", "0.01 - 0.05", "0.05 - 0.10", "0.10 - 0.25", "0.25 - 0.50", "     > 0.50" };
    printf( "\n\nMMXTransformVector2 error vs blended normal length before normalize\n" );
    printf( "  length         samples   exact     worst |diff|\n" );
    for ( int b = 0; b < 6; ++b )
    {
        if ( lens[b].n == 0 ) continue;
        printf( "  %s  %10lld  %6.2f%%  %6d\n", pszLen[b], lens[b].n,
            ( 100.0 * lens[b].exact ) / lens[b].n, lens[b].worst );
    }
    return 0;
}
