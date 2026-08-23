// The pre-extraction __asm block, transcribed verbatim from the CalcDirectionalLighting
// loop in test/original.h, wrapped so it can be called with the same three arguments
// and packed return value as CalcDirectionalLighting.asm.
//
// Only the wrapping is new. Nothing inside the __asm statement was touched, which is
// what makes a byte-for-byte match against the .asm meaningful.

// Geom.h uses WORD and friends, so the prelude has to come first, as everywhere
// else in this codebase.
#include "3Dmotor/stdafx.h"
#include "SentinelRef.h"

uint64_t InlineCalcDirectionalLightingMMX(
    const void *pDirData,
    const NGfx::SMMXWord *pTranslucentShade,
    uint32_t dwNormal )
{
    uint32_t dwColor = 0, dwShadowColor = 0;
    __asm
    {
        mov esi, pDirData
        movd mm7, dwNormal
        punpcklbw mm7, mm7
        psubw mm7, [esi+5*8]//shift
        pmaddwd mm7, [esi+4*8]//dirLight
        movq mm6, mm7
        psrlq mm6, 32
        paddd mm7, mm6
        psrad mm7, 15
        punpcklwd mm7, mm7
        punpckldq mm7, mm7
        movq mm6, mm7
        movq mm5, mm7
        psraw mm6, 16
        pand mm7, mm6
        pandn mm6, mm5 // mm6 = f, range [0, 0x4000]
        pcmpeqw mm0, mm0
        pxor mm7, mm0  // mm7 - -f
        movq mm0, [esi]//ambient // vRes
        movq mm1, mm0     // vResShadow
        movq mm2, [esi + 1*8]//lightColor
        movq mm3, [esi + 2*8]//incidentShadowColor
        movq mm4, [esi + 3*8]//shadeColor
        mov esi, pTranslucentShade
        movq mm5, [esi]
        pmulhw mm2, mm6
        pmulhw mm3, mm6
        pmulhw mm5, mm7
        pmulhw mm4, mm7
        paddw mm0, mm2
        paddw mm1, mm3
        paddw mm0, mm5
        paddw mm1, mm4
        psraw mm0, 4
        psraw mm1, 4
        packuswb mm0, mm0
        packuswb mm1, mm1
        movd dwColor, mm0
        movd dwShadowColor, mm1
    }
    __asm emms

    return ( static_cast<uint64_t>( dwShadowColor ) << 32 ) | dwColor;
}
