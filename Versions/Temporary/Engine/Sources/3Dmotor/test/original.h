#pragma once

namespace original {
    static void MultiplyOnColor( std::vector<DWORD> *pRes, const std::vector<DWORD> &mult )
    {
        if ( mult.empty() )
            return;
        DWORD *pDst = &(*pRes)[0], *pDstEnd = pDst + pRes->size();
        const DWORD *pSrc = &mult[0];
        for ( ; pDst < pDstEnd; ++pDst, ++pSrc )
        {
            DWORD dwB = *pSrc;
            __asm
            {
                mov esi, pDst
                pxor mm7, mm7
                movd mm0, [esi]
                movd mm1, dwB
                punpcklbw mm0, mm7
                punpcklbw mm1, mm7
                pmullw mm0, mm1
                psrlw mm0, 8
                packuswb mm0, mm0
                movd [esi], mm0
            }
        }
        __asm emms
    }
}
