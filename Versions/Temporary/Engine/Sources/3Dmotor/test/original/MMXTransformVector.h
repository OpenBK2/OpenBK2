#pragma once

// Reference implementations of MMXTransformVector, MMXTransformVector2 and
// MMXTransformVector3: the original MMX bodies now live in MMXTransformVector.asm
// and MMXTransformVectorWeighted.asm rather than __asm statements, so these build on
// x64 too.
//
// nNormalizeTable and mmxWeights are handed to the assembly rather than named inside
// it. The inline originals could name the C++ globals directly because the compiler
// resolved them; MASM would need the decorated names.

#include "3Dmotor/GSSETransform.h"

#include <cstddef>
#include <cstdint>

// Argument block for the weighted entry points. Eight and eleven inputs is past what
// either ABI passes in registers, so they take a pointer to this instead of reaching
// past the shadow space for stack arguments. Every member is pointer sized so one
// SLOT equate in the assembly covers x86 and x64 alike.
struct SMMXTransformArgs
{
    const NGfx::SCompactTransformer *pTrans0;
    const NGfx::SCompactTransformer *pTrans1;
    const NGfx::SCompactTransformer *pTrans2;
    const SMMXFixups *pFixups;
    const NGfx::SMMXWord *pWeightTable;
    const short *pNormalizeTable;
    size_t nWeight0;
    size_t nWeight1;
    size_t nWeight2;
};

// MMXTransformVectorWeighted.asm indexes these by slot number.
static_assert( sizeof( SMMXTransformArgs ) == 9 * sizeof( void * ) );
static_assert( offsetof( SMMXTransformArgs, pTrans0 ) == 0 * sizeof( void * ) );
static_assert( offsetof( SMMXTransformArgs, pTrans1 ) == 1 * sizeof( void * ) );
static_assert( offsetof( SMMXTransformArgs, pTrans2 ) == 2 * sizeof( void * ) );
static_assert( offsetof( SMMXTransformArgs, pFixups ) == 3 * sizeof( void * ) );
static_assert( offsetof( SMMXTransformArgs, pWeightTable ) == 4 * sizeof( void * ) );
static_assert( offsetof( SMMXTransformArgs, pNormalizeTable ) == 5 * sizeof( void * ) );
static_assert( offsetof( SMMXTransformArgs, nWeight0 ) == 6 * sizeof( void * ) );
static_assert( offsetof( SMMXTransformArgs, nWeight1 ) == 7 * sizeof( void * ) );
static_assert( offsetof( SMMXTransformArgs, nWeight2 ) == 8 * sizeof( void * ) );

extern "C" uint32_t MMXTransformVectorMMX(
    uint32_t nSrc,
    const SMMXFixups *pFixups,
    const NGfx::SCompactTransformer *pTrans,
    const short *pNormalizeTable );

extern "C" uint32_t MMXTransformVector2MMX( uint32_t nSrc, const SMMXTransformArgs *pArgs );
extern "C" uint32_t MMXTransformVector3MMX( uint32_t nSrc, const SMMXTransformArgs *pArgs );

namespace original
{

static void MMXTransformVector( NGfx::SCompactVector &res, const NGfx::SCompactVector &src,
    const SHMatrix &transform )
{
    NGfx::SCompactTransformer compactTransform;
    Assign( &compactTransform, transform );
    res.dw = MMXTransformVectorMMX( src.dw, &fixups, &compactTransform, nNormalizeTable );
}

static void MMXTransformVector2( NGfx::SCompactVector &res, const NGfx::SCompactVector &src,
    const SHMatrix &transform1, uint8_t weight1,
    const SHMatrix &transform2, uint8_t weight2 )
{
    NGfx::SCompactTransformer compactTransform1, compactTransform2;
    Assign( &compactTransform1, transform1 );
    Assign( &compactTransform2, transform2 );

    SMMXTransformArgs args{};
    args.pTrans0 = &compactTransform1;
    args.pTrans1 = &compactTransform2;
    args.pFixups = &fixups;
    args.pWeightTable = mmxWeights;
    args.pNormalizeTable = nNormalizeTable;
    args.nWeight0 = weight1;
    args.nWeight1 = weight2;

    res.dw = MMXTransformVector2MMX( src.dw, &args );
}

static void MMXTransformVector3( NGfx::SCompactVector &res, const NGfx::SCompactVector &src,
    const SHMatrix &transform1, uint8_t weight1,
    const SHMatrix &transform2, uint8_t weight2,
    const SHMatrix &transform3, uint8_t weight3 )
{
    NGfx::SCompactTransformer compactTransform1, compactTransform2, compactTransform3;
    Assign( &compactTransform1, transform1 );
    Assign( &compactTransform2, transform2 );
    Assign( &compactTransform3, transform3 );

    SMMXTransformArgs args{};
    args.pTrans0 = &compactTransform1;
    args.pTrans1 = &compactTransform2;
    args.pTrans2 = &compactTransform3;
    args.pFixups = &fixups;
    args.pWeightTable = mmxWeights;
    args.pNormalizeTable = nNormalizeTable;
    args.nWeight0 = weight1;
    args.nWeight1 = weight2;
    args.nWeight2 = weight3;

    res.dw = MMXTransformVector3MMX( src.dw, &args );
}

}
