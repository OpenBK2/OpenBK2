#pragma once

#include "3Dmotor/GLightPerVertex.h"
// MMXTransformVector*Impl below use SMMXFixups, fixups, Assign and the lookup
// tables from here. That only ever worked because every includer happened to pull
// in GLightPerVertex.cpp first.
#include "3Dmotor/GSSETransform.h"
#include "3DLib/GGeometry.h"

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

    static void MMXTransformVectorImpl(NGfx::SCompactVector* pRes, const NGfx::SCompactVector* pSrc, const SMMXFixups* pFixups,
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

	static void MMXTransformVector(NGfx::SCompactVector & res, const NGfx::SCompactVector& src,
		const SHMatrix & transform) {

        NGfx::SCompactTransformer compact_transform;
        Assign(&compact_transform, transform);
        MMXTransformVectorImpl(&res, &src, &fixups, &compact_transform);
    }

	static void MMXTransformVector2Impl(NGfx::SCompactVector* pRes, const NGfx::SCompactVector* pSrc, const SMMXFixups* pFixups,
	    const NGfx::SCompactTransformer* pTrans, uint8_t w1,
	    const NGfx::SCompactTransformer* pTrans2, uint8_t w2)
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

	static void MMXTransformVector2(NGfx::SCompactVector & res, const NGfx::SCompactVector & src,
		const SHMatrix & transform1, uint8_t weight1,
		const SHMatrix & transform2, uint8_t weight2) {

        NGfx::SCompactTransformer compact_transform1, compact_transform2;
        Assign(&compact_transform1, transform1);
        Assign(&compact_transform2, transform2);
        MMXTransformVector2Impl(&res, &src, &fixups, &compact_transform1, weight1, &compact_transform2, weight2);
    }

	static void MMXTransformVector3Impl(NGfx::SCompactVector* pRes, const NGfx::SCompactVector* pSrc, const SMMXFixups* pFixups,
	const NGfx::SCompactTransformer* pTrans, uint8_t w1,
	const NGfx::SCompactTransformer* pTrans2, uint8_t w2,
	const NGfx::SCompactTransformer* pTrans3, uint8_t w3)
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

	static void MMXTransformVector3(NGfx::SCompactVector & res, const NGfx::SCompactVector & src,
		const SHMatrix & transform1, uint8_t weight1,
		const SHMatrix & transform2, uint8_t weight2,
		const SHMatrix & transform3, uint8_t weight3) {

        NGfx::SCompactTransformer compact_transform1, compact_transform2, compact_transform3;
        Assign(&compact_transform1, transform1);
        Assign(&compact_transform2, transform2);
        Assign(&compact_transform3, transform3);
        MMXTransformVector3Impl(&res, &src, &fixups, &compact_transform1, weight1, &compact_transform2, weight2, &compact_transform3, weight3);
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


	static void SampleWarFog( const std::vector<CVec3> &srcPos, float fScale, std::vector<unsigned char> *_pRes1, const CArray2D<unsigned char> &fog1,
		std::vector<unsigned char> *_pRes2, const CArray2D<unsigned char> &fog2 )
    {
		if ( srcPos.empty() )
		    return;
		int nVertices = srcPos.size();
		if ( _pRes1->size() < nVertices )
		    _pRes1->resize( nVertices );
		if ( _pRes2 && _pRes2->size() < nVertices )
		    _pRes2->resize( nVertices );

		static std::vector<int> tmp;
		if ( tmp.size() < nVertices * 2 )
		    tmp.resize( nVertices * 2 );

		// calc integer x & y
		{
		    float fpScale = fScale * 0x4000;
		    const CVec3 *pSrc = &srcPos[0], *pEnd = pSrc + nVertices;
		    int *pTmp = &tmp[0];
		    __asm
			{
				mov esi, pSrc
				mov edi, pTmp
				mov eax, pEnd
		lp:
				fld dword ptr [esi]
				fmul fpScale
				fistp dword ptr[edi]
				fld dword ptr [esi+4]
				fmul fpScale
				fistp dword ptr[edi+4]
				add esi, 12
				add edi, 8
				cmp esi, eax
				jnz lp
			}
		}

		SampleWarFogInt( tmp, fog1, _pRes1, nVertices );
		if ( _pRes2 )
		    SampleWarFogInt( tmp, fog2, _pRes2, nVertices );
		_m_empty();
		}


	static void CalcPointLightColors( std::vector<NGfx::SMMXWord> *pRes,
		const std::vector<NGfx::SMMXWord> &attenuation, const std::vector<WORD> &posIndices,
		const std::vector<NGfx::SCompactVector> &_normals,
		const CVec3 &_vColor )
	{
		NGfx::SMMXWord shift{}, lightColor{}, shift1{};
	    shift.nX = shift.nY = shift.nZ = (short)0x8000;
	    shift1.nX = shift1.nZ = 1 << 12; shift1.nY = shift1.nW = 0;
	    lightColor.nX = Float2Int( _vColor.x * 32767 );
	    lightColor.nY = Float2Int( _vColor.y * 32767 );
	    lightColor.nZ = Float2Int( _vColor.z * 32767 );
	    __asm
		{
			movq mm7, lightColor
			movq mm5, shift1
		}
	    for ( int k = 0; k < posIndices.size(); ++k )
	    {
            DWORD dwNormal = _normals[k].dw;
            const NGfx::SMMXWord *pAtt = &attenuation[ posIndices[k] ];
            NGfx::SMMXWord *pResColor = &(*pRes)[k];
            __asm
			{
				mov esi, pResColor
				mov edi, pAtt
				movq mm6, [esi]
				movd mm0, dwNormal
				punpcklbw mm0, mm0
				psubw mm0, shift
				pmaddwd mm0, [edi]
				movq mm1, mm0
				psrlq mm1, 32
				paddd mm0, mm1
				psrad mm0, 15
				packssdw mm0, mm0
				punpcklwd mm0, mm0
				pxor mm2, mm2
				punpckldq mm0, mm0
				movq mm1, mm0
				pcmpgtw mm1, mm2
				pand mm0, mm1
				movq mm1, mm0
				pmulhw mm0, mm7
				pmullw mm1, mm7
				movq mm2, mm1
				movq mm3, mm1
				punpcklwd mm2, mm0
				punpckhwd mm3, mm0
				paddd mm2, mm5
				paddd mm3, mm5
				psrad mm2, 13
				psrad mm3, 13
				packssdw mm2, mm3
				paddsw mm2, mm6
				movq [esi], mm2
			}
	    }
	    __asm emms
	}

	static void CalcPointLightColors( std::vector<NGfx::SMMXWord> *pRes,
	const NGfx::SMMXWord &attenuation, const NGScene::SUVInfo *pSrc, int _nSize, const CVec3 &_vColor )
	{
	    NGfx::SMMXWord shift{}, lightColor{}, shift1{};
	    shift.nX = shift.nY = shift.nZ = (short)0x8000;
	    shift1.nX = shift1.nZ = 1 << 12; shift1.nY = shift1.nW = 0;
	    lightColor.nX = Float2Int( _vColor.x * 32767 );
	    lightColor.nY = Float2Int( _vColor.y * 32767 );
	    lightColor.nZ = Float2Int( _vColor.z * 32767 );
	    __asm
		{
			movq mm7, lightColor
			movq mm5, shift1
		}
	    DWORD dwPrevNormal = 0;
	    __declspec(align(8)) NGfx::SMMXWord prevColor;
	    const NGfx::SMMXWord *pAtt = &attenuation;
	    for ( int k = 0; k < _nSize; ++k )
	    {
            DWORD dwNormal = pSrc[k].normal.dw;
            NGfx::SMMXWord *pResColor = &(*pRes)[k];
            if ( dwNormal != dwPrevNormal )
            {
                __asm
				{
					mov esi, pResColor
					mov edi, pAtt
					movq mm6, [esi]
					movd mm0, dwNormal
					punpcklbw mm0, mm0
					psubw mm0, shift
					pmaddwd mm0, [edi]
					movq mm1, mm0
					psrlq mm1, 32
					paddd mm0, mm1
					psrad mm0, 15
					packssdw mm0, mm0
					punpcklwd mm0, mm0
					pxor mm2, mm2
					punpckldq mm0, mm0
					movq mm1, mm0
					pcmpgtw mm1, mm2
					pand mm0, mm1
					movq mm1, mm0
					pmulhw mm0, mm7
					pmullw mm1, mm7
					movq mm2, mm1
					movq mm3, mm1
					punpcklwd mm2, mm0
					punpckhwd mm3, mm0
					paddd mm2, mm5
					paddd mm3, mm5
					psrad mm2, 13
					psrad mm3, 13
					packssdw mm2, mm3
					movq prevColor, mm2
					paddsw mm2, mm6
					movq [esi], mm2
				}
                dwPrevNormal = dwNormal;
            }
            else
            {
               __asm
				{
					mov esi, pResColor
					movq mm0, [esi]
					paddsw mm0, prevColor
					movq [esi], mm0
                }
            }
	    }
	    __asm emms
	}

	static void AddColors( std::vector<DWORD> *pRes, const std::vector<DWORD> &src, const std::vector<NGfx::SMMXWord> &add )
    {
		ASSERT( pRes->size() >= add.size() );
		ASSERT( src.size() >= add.size() );
		int nSize = add.size();
		DWORD *pResPtr = &(*pRes)[0];
		const DWORD *pSrcPtr = &src[0];
		const NGfx::SMMXWord *pAdd = &add[0];
		__asm
		{
			pxor mm7, mm7
			pcmpeqw mm6, mm6
			psllw mm6, 15
			psrlw mm6, 1
		}
		for ( DWORD *pResEnd = pResPtr + nSize; pResPtr < pResEnd; ++pResPtr, ++pSrcPtr, ++pAdd )
		{
			DWORD dwColor = *pSrcPtr;//(*pRes)[k];
			//NGfx::SMMXWord addColor = add[k];
			//addColor.nX = Clamp( Float2Int( add[k].x * 32767 ), 0, 32767 );
			//addColor.nY = Clamp( Float2Int( add[k].y * 32767 ), 0, 32767 );
			//addColor.nZ = Clamp( Float2Int( add[k].z * 32767 ), 0, 32767 );
			__asm
			{
				mov esi, pAdd
				movd mm0, dwColor
				punpcklbw mm0, mm0
				psrlw mm0, 1
				movq mm1, mm0
				pmulhw mm0, mm0
				psllw mm0, 1
				movq mm2, mm0
				pmulhw mm0, mm1
				pmullw mm2, mm1
				psllw mm0, 1
				paddsw mm0, [esi]//addColor
				psrlw mm0, 1
				// combine low part into lookup index if higher part is zero
				psrlw mm2, 2
				por mm2, mm6
				movq mm3, mm0
				pcmpeqw mm3, mm7
				pand mm2, mm3
				pandn mm3, mm0
				por mm3, mm2
				// calc cubic root from result
				movd ebx, mm3
				psrlq mm3, 32
				mov esi, ebx
				shr ebx, 16
				and esi, 0x7fff
				movzx eax, byte ptr[nCubicRoot + esi]
				and ebx, 0x7fff
				xor ecx, ecx
				mov ch, byte ptr[nCubicRoot + ebx]
				or eax, ecx
				movd ebx, mm3
				and ebx, 0x7fff
				movzx ecx, byte ptr[nCubicRoot + ebx]
				shl ecx, 16
				or eax, ecx
				mov dwColor, eax
				//emms
			}
			//DWORD dwTest = NGfx::GetDWORDColor( GetOutputColor(
			//	GetLinearColor( NGfx::GetCVec4Color( (*pRes)[k] ) ) +
			//	CVec4( add[k], 0 )
			//	) );
			*pResPtr = dwColor;
		}
		__asm emms
	}

	static void ScaleColors( std::vector<DWORD> *pRes, const DWORD *_pSrc, int nSrcStride,
		unsigned char *pScale, int nScaleMask, const std::vector<WORD> &posIndices, const std::vector<NGfx::SCompactVector> &transp,
		bool bMultiplyOnTransparency )
	{
	    int nSize = posIndices.size();
	    if ( pRes->size() < nSize )
            pRes->resize( nSize );
	    DWORD *p = &(*pRes)[0], *pEnd = p + nSize;
	    const DWORD *pSrc = _pSrc;
	    ASSERT( sizeof(DWORD) == sizeof(transp[0]) );
	    const NGfx::SCompactVector *pTransp = &transp[0];
	    const WORD *pPosIndices = &posIndices[0];
	    NGfx::SMMXWord mTransp;
	    mTransp.nX = mTransp.nY = mTransp.nZ = 0; mTransp.nW = 0x1ff;
	    __asm movq mm7, mTransp
		if ( bMultiplyOnTransparency )
		{
			mTransp.nX = mTransp.nY = mTransp.nZ = 0; mTransp.nW = 0;
		}
		else
		{
			mTransp.nX = mTransp.nY = mTransp.nZ = 0x7fff; mTransp.nW = 0;
		}
	    __asm movq mm6, mTransp
		for ( ; p < pEnd; ++p, pSrc += nSrcStride / 4, ++pPosIndices, ++pTransp )
		{
			int nScaleIndex = (*pPosIndices) & nScaleMask;
			int n = ((int) (pScale[ nScaleIndex ]) ) << 2;
			int nScale = pTransp->w << 7;
			//ASSERT( ((*pSrc) & 0xff000000 ) == 0 );
			__asm
			{
				mov esi, pSrc
				movd mm0, [esi]
				movd mm1, n
				punpcklbw mm0, mm0
				mov esi, p
				psrlw mm0, 1
				punpcklwd mm1, mm1
				punpckldq mm1, mm1
				pmulhw mm0, mm1
				por mm0, mm7
				movd mm2, nScale
				punpcklwd mm2, mm2
				punpckldq mm2, mm2
				por mm2, mm6
				pmulhw mm0, mm2
				packuswb mm0, mm0
				movd [esi], mm0
			}
		}
	    __asm emms
	}

}
