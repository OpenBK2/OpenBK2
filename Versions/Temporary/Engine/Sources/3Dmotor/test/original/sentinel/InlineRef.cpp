// The pre-extraction __asm blocks, transcribed from test/original.h and wrapped so
// each can be called with the same arguments and return value as its .asm
// counterpart. Only the wrapping is new; nothing inside an __asm statement was
// touched, which is what makes a byte-for-byte match meaningful.

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

// Deliberately keeps the two things the .asm could not: it clobbers ebx, and it names
// the C++ global nNormalizeTable inside the assembly. The .asm form saves ebx itself
// and takes the table as an argument, so agreeing with this proves the extraction and
// the parameterisation at once.
uint32_t InlineMMXTransformVectorMMX(
    uint32_t nSrc,
    const SMMXFixups *pFixups,
    const NGfx::SCompactTransformer *pTrans )
{
    // The original read *pSrc through a pointer and wrote *pRes. Here the value goes
    // in and the packed result comes back, matching the .asm entry point; everything
    // between the first pxor and the final movd is untouched.
    NGfx::SCompactVector src;
    src.dw = nSrc;
    NGfx::SCompactVector res;
    res.dw = 0;
    const NGfx::SCompactVector *pSrc = &src;
    NGfx::SCompactVector *pRes = &res;

    _asm
    {
        mov esi, pSrc // esi = pSrc
        mov ecx, [esi] // ecx = *pSrc
        mov edi, ecx // edi = *pSrc
        and edi, 0xffffff // edi = *pSrc & 0xFFFFFF (clear w?)
        and ecx, 0xff000000 // ecx = *pSrc & 0xFF000000 (keep W only)
        movd mm7, edi       // mm7 = *pSrc & 0xFFFFFF
        mov esi, pTrans // esi = pTrans
        mov edi, pFixups // edi = pFixups (normalFixup)
        pxor mm0, mm0    // mm0 = 0
        punpcklbw mm0, mm7 // unpacked vector
        psubw mm0, [edi] // mm0 -= normalFixup

        movq mm1, mm0    // z y x // mm1 = mm1 * mm0
        pmulhw mm1, [esi]
        movq mm2, mm0
        movq mm3, mm0
        psllq mm2, 16
        psrlq mm3, 32
        paddw mm2, mm3   // x z y
        pmulhw mm2, [esi + 8]
        movq mm3, mm0
        movq mm4, mm0
        paddsw mm1, mm2
        psllq mm3, 32
        psrlq mm4, 16
        paddw mm3, mm4   // y x z
        pmulhw mm3, [esi + 16]
        paddsw mm1, mm3 // packed result
        // normalize
        psllw mm1, 3
        movq mm2, mm1
        pmaddwd mm2, mm2
        movq mm3, mm2
        psrlq mm3, 32
        paddd mm2, mm3
        movd ebx, mm2
        shr ebx, 18
        xor eax, eax
        mov ax, [nNormalizeTable + ebx * 2]
        movd mm2, eax
        punpcklwd mm2, mm2
        punpckldq mm2, mm2
        pmulhw mm1, mm2
        psllw mm1, 5
        // pack and output result
        paddw mm1, [edi + 8]
        psrlw mm1, 8
        packuswb mm1, mm1
        movd edi, mm1
        or ecx, edi // edi |= ecx
        mov esi, pRes
        mov[esi], ecx
        emms
    }

    return res.dw;
}
