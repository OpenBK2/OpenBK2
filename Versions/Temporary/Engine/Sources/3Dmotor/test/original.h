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

    static void CalcDirectionalLighting(
	const std::vector<WORD> &posIndices,
	const std::vector<NGfx::SCompactVector> &_normals,
	const NGScene::SPerVertexLightState &ls, bool bTranslucent, const CVec3 &vTranslucentColor,
	std::vector<DWORD> *pResColors, std::vector<DWORD> *pResShadow )
	{
		pResColors->resize( posIndices.size() );
		pResShadow->resize( posIndices.size() );
		DWORD dwColor = 0, dwShadowColor = 0, dwPrevNormal = 0;
		const void *pDirData = &ls.ambient;
		const NGfx::SMMXWord *pTranslucentShade = &ls.shadeColor;
		if ( bTranslucent )
		{
			NGfx::SMMXWord transHolder;
			NGScene::ConvertColor( &transHolder, NGScene::MulPerComp( ls.vLightColor, vTranslucentColor ) );
			pTranslucentShade = &transHolder;
		}
		for ( int k = 0; k < posIndices.size(); ++k )
		{
			DWORD dwNormal = _normals[k].dw;
			if ( dwNormal != dwPrevNormal )
			{
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
			}
			(*pResColors)[k] = dwColor;
			(*pResShadow)[k] = dwShadowColor;
			dwPrevNormal = dwNormal;
		}
		__asm emms
	}
}
