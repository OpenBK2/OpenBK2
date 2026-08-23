#pragma once

// Reference implementation of MMXTransformVector: the original MMX body now lives in
// MMXTransformVector.asm rather than an __asm statement, so this builds on x64 too.
//
// nNormalizeTable is handed to the assembly as an argument rather than named inside
// it. The inline original could name the C++ global directly because the compiler
// resolved it; MASM would need the decorated name.

#include "3Dmotor/GSSETransform.h"

#include <cstdint>

extern "C" uint32_t MMXTransformVectorMMX(
    uint32_t nSrc,
    const SMMXFixups *pFixups,
    const NGfx::SCompactTransformer *pTrans,
    const short *pNormalizeTable );

namespace original
{

static void MMXTransformVector( NGfx::SCompactVector &res, const NGfx::SCompactVector &src,
    const SHMatrix &transform )
{
    NGfx::SCompactTransformer compactTransform;
    Assign( &compactTransform, transform );
    res.dw = MMXTransformVectorMMX( src.dw, &fixups, &compactTransform, nNormalizeTable );
}

}
