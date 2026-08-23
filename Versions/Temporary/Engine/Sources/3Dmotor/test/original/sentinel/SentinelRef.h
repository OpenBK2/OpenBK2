#pragma once

// The sentinel compares the extracted MMX primitive against the __asm block it was
// lifted from, not the C++ loop around it. The loop is shared and identical either
// way, so comparing at this level tests exactly the thing that moved, and keeps the
// target free of SPerVertexLightState and everything that drags in.

#include "Misc/Geom.h"
#include "3Dmotor/GPixelFormat.h"

#include <cstdint>

// The __asm form, transcribed verbatim from test/original.h. x86 only: MSVC has no
// inline assembler on x64, which is the whole reason the .asm form exists.
uint64_t InlineCalcDirectionalLightingMMX(
    const void *pDirData,
    const NGfx::SMMXWord *pTranslucentShade,
    uint32_t dwNormal );

// The extracted CalcDirectionalLighting.asm, assembled for this architecture.
extern "C" uint64_t CalcDirectionalLightingMMX(
    const void *pDirData,
    const NGfx::SMMXWord *pTranslucentShade,
    uint32_t dwNormal );
