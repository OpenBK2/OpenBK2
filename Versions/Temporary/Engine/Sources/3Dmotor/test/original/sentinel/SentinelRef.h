#pragma once

// The sentinel compares each extracted MMX primitive against the __asm block it was
// lifted from, not the C++ loop around it. The loops are shared and identical either
// way, so comparing at this level tests exactly the thing that moved, and keeps the
// target free of SPerVertexLightState and everything that drags in.

#include "Misc/Geom.h"
#include "3Dmotor/GPixelFormat.h"
#include "3Dmotor/GSSETransform.h"

#include <cstdint>

// The __asm forms, transcribed verbatim from test/original.h. x86 only: MSVC has no
// inline assembler on x64, which is the whole reason the .asm forms exist.
uint64_t InlineCalcDirectionalLightingMMX(
    const void *pDirData,
    const NGfx::SMMXWord *pTranslucentShade,
    uint32_t dwNormal );

uint32_t InlineMMXTransformVectorMMX(
    uint32_t nSrc,
    const SMMXFixups *pFixups,
    const NGfx::SCompactTransformer *pTrans );

// The extracted .asm bodies, assembled for this architecture.
extern "C" uint64_t CalcDirectionalLightingMMX(
    const void *pDirData,
    const NGfx::SMMXWord *pTranslucentShade,
    uint32_t dwNormal );

extern "C" uint32_t MMXTransformVectorMMX(
    uint32_t nSrc,
    const SMMXFixups *pFixups,
    const NGfx::SCompactTransformer *pTrans,
    const short *pNormalizeTable );

// MMXTransformVector2 and 3 are compared through wrappers instead of a transcription:
// each side is compiled in its own translation unit (LegacyTransformRef.cpp and
// AsmTransformRef.cpp) because both define these in namespace original.
void LegacyTransform2( NGfx::SCompactVector &res, const NGfx::SCompactVector &src,
    const SHMatrix &m1, uint8_t w1, const SHMatrix &m2, uint8_t w2 );
void LegacyTransform3( NGfx::SCompactVector &res, const NGfx::SCompactVector &src,
    const SHMatrix &m1, uint8_t w1, const SHMatrix &m2, uint8_t w2,
    const SHMatrix &m3, uint8_t w3 );

void AsmTransform2( NGfx::SCompactVector &res, const NGfx::SCompactVector &src,
    const SHMatrix &m1, uint8_t w1, const SHMatrix &m2, uint8_t w2 );
void AsmTransform3( NGfx::SCompactVector &res, const NGfx::SCompactVector &src,
    const SHMatrix &m1, uint8_t w1, const SHMatrix &m2, uint8_t w2,
    const SHMatrix &m3, uint8_t w3 );
