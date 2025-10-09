#pragma once

#include "3Dmotor/GLightPerVertex.h"

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

	static void ConvertColor( NGfx::SMMXWord *p, const CVec3 &v )
    {
	    p->nZ = Float2Int( v.z * 0x4000 );
	    p->nY = Float2Int( v.y * 0x4000 );
	    p->nX = Float2Int( v.x * 0x4000 );
	    p->nW = 0;
    }

	static CVec3 MulPerComp( const CVec3 &a, const CVec3 &b ) {
	    return CVec3( a.x * b.x, a.y * b.y, a.z * b.z );
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
			ConvertColor( &transHolder, MulPerComp( ls.vLightColor, vTranslucentColor ) );
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

	static void MMXTransformVector(NGfx::SCompactVector* pRes, const NGfx::SCompactVector* pSrc, const SMMXFixups* pFixups,
	    const NGfx::SCompactTransformer* pTrans)
	{
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
	}

	static void MMXTransformVector2(NGfx::SCompactVector* pRes, const NGfx::SCompactVector* pSrc, const SMMXFixups* pFixups,
	    const NGfx::SCompactTransformer* pTrans, char w1,
	    const NGfx::SCompactTransformer* pTrans2, char w2)
	{
	    _asm
	    {
	        mov esi, pSrc
	        mov ecx, [esi]
	        mov edi, ecx
	        and edi, 0xffffff
	        and ecx, 0xff000000
	        movd mm7, edi
	        mov esi, pTrans
	        mov ebx, pTrans2
	        mov edi, pFixups
	        pxor mm0, mm0
	        punpcklbw mm0, mm7 // unpacked vector
	        psubw mm0, [edi]

	        movq mm1, mm0    // z y x
	        movq mm5, mm1
	        pmulhw mm1, [esi]
	        pmulhw mm5, [ebx]
	        movq mm2, mm0
	        movq mm3, mm0
	        psllq mm2, 16
	        psrlq mm3, 32
	        paddw mm2, mm3   // x z y
	        movq mm6, mm2
	        pmulhw mm2, [esi + 8]
	        pmulhw mm6, [ebx + 8]
	        movq mm3, mm0
	        movq mm4, mm0
	        paddsw mm1, mm2
	        paddsw mm5, mm6
	        psllq mm3, 32
	        psrlq mm4, 16
	        paddw mm3, mm4   // y x z
	        movq mm6, mm3
	        pmulhw mm3, [esi + 16]
	        pmulhw mm6, [ebx + 16]
	        paddsw mm1, mm3 // packed result
	        paddsw mm5, mm6
	        movzx esi, w1
	        movzx ebx, w2
	        psllw mm1, 4
	        psllw mm5, 4
	        pmulhw mm1, qword ptr[esi * 8 + mmxWeights]
	        pmulhw mm5, qword ptr[ebx * 8 + mmxWeights]
	        paddsw mm1, mm5
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
	        or ecx, edi
	        mov esi, pRes
	        mov[esi], ecx
	        emms
	    }
	}

	static void MMXTransformVector3(NGfx::SCompactVector* pRes, const NGfx::SCompactVector* pSrc, const SMMXFixups* pFixups,
	    const NGfx::SCompactTransformer* pTrans, char w1,
	    const NGfx::SCompactTransformer* pTrans2, char w2,
	    const NGfx::SCompactTransformer* pTrans3, char w3)
	{
	    _asm
	    {
	        mov esi, pSrc
	        mov ecx, [esi]
	        mov edi, ecx
	        and edi, 0xffffff
	        and ecx, 0xff000000
	        movd mm7, edi
	        mov esi, pTrans
	        mov ebx, pTrans2
	        mov edx, pTrans3
	        mov edi, pFixups
	        pxor mm0, mm0
	        punpcklbw mm0, mm7 // unpacked vector
	        psubw mm0, [edi]

	        movq mm1, mm0    // z y x
	        movq mm5, mm1
	        movq mm7, mm1
	        pmulhw mm1, [esi]
	        pmulhw mm5, [ebx]
	        pmulhw mm7, [edx]
	        movq mm2, mm0
	        movq mm3, mm0
	        psllq mm2, 16
	        psrlq mm3, 32
	        paddw mm2, mm3   // x z y
	        movq mm6, mm2
	        movq mm3, mm2
	        pmulhw mm2, [esi + 8]
	        pmulhw mm6, [ebx + 8]
	        pmulhw mm3, [edx + 8]
	        paddsw mm7, mm3
	        movq mm3, mm0
	        movq mm4, mm0
	        paddsw mm1, mm2
	        paddsw mm5, mm6
	        psllq mm3, 32
	        psrlq mm4, 16
	        paddw mm3, mm4   // y x z
	        movq mm6, mm3
	        movq mm4, mm3
	        pmulhw mm3, [esi + 16]
	        pmulhw mm6, [ebx + 16]
	        pmulhw mm4, [edx + 16]
	        paddsw mm1, mm3 // packed result
	        paddsw mm5, mm6
	        paddsw mm7, mm4
	        movzx esi, w1
	        movzx ebx, w2
	        movzx edx, w3
	        psllw mm1, 4
	        psllw mm5, 4
	        psllw mm7, 4
	        pmulhw mm1, qword ptr[esi * 8 + mmxWeights]
	        pmulhw mm5, qword ptr[ebx * 8 + mmxWeights]
	        pmulhw mm7, qword ptr[edx * 8 + mmxWeights]
	        paddsw mm1, mm5
	        paddsw mm1, mm7
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
	        or ecx, edi
	        mov esi, pRes
	        mov[esi], ecx
	        emms
	    }
	}

	static void SampleWarFogInt( const std::vector<int> &intCoords, const CArray2D<unsigned char> &fog, std::vector<unsigned char> *_pRes, int nVertices )
    {
		ASSERT( fog.GetSizeX() == fog.GetSizeY() );
		ASSERT( GetNextPow2( fog.GetSizeX() - 1 ) + 1 == fog.GetSizeX() );
		__m64 zero;
		zero = _m_from_int( 0 );
		unsigned char *pRes = &(*_pRes)[0];
		int nMask = fog.GetSizeX() - 2;
		for ( const int *pTmp = &intCoords[0], *pTmpEnd = pTmp + nVertices * 2; pTmp < pTmpEnd; pTmp += 2, ++pRes )
		{
		    int nY = pTmp[1];
		    int nYU = ( nY >> 14 ) & nMask;
		    int nYfi = nY & 0x3fff;
		    __m64 nYf = _m_from_int( ( 0x4000 - nYfi ) | (nYfi << 16) );
		    int nX = pTmp[0];
		    int nXL = ( nX >> 14 ) & nMask;
		    int nXfi = nX & 0x3fff;
		    __m64 nXf = _m_from_int( ( 0x4000 - nXfi ) | (nXfi << 16)  );
		    const unsigned char *pUp = (&fog[nYU][0]) + nXL;
		    const unsigned char *pDown = pUp + nMask + 2;

		    __m64 nData = _mm_unpacklo_pi32(
				_m_punpcklbw( _m_from_int( *(unsigned short*)pUp ), zero ),
				_m_punpcklbw( _m_from_int( *(unsigned short*)pDown ), zero )
				);
		    nXf = _mm_unpacklo_pi32( nXf, nXf );
		    nData = _m_pmaddwd( nData, nXf );
		    nData = _m_psradi( nData, 14 );
		    nData = _mm_packs_pi32( nData, zero );
		    nData = _m_pmaddwd( nData, nYf );
		    *pRes = _m_to_int( nData ) >> 14;
		}
        __asm emms;
    }
}
