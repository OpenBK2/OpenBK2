#pragma once
#include "GPixelFormat.h"

extern bool bIsSSEPresent;

inline void SSELoadMatrix( const SHMatrix &m )
{
	const float *row0 = &m._11;
	__asm 
	{
		// load columns of matrix into xmm4-7
		mov         eax, row0
		movups xmm7, [eax]       // a b c d
		movups xmm1, [eax + 16]  // e f g h
		movups xmm2, [eax + 32]  // i j k l
		movups xmm3, [eax + 48]  // m n o p
		movaps xmm5, xmm7        // a b c d
		movaps xmm0, xmm1        // e f g h

		unpckhps xmm7, xmm2      // a i b j
		unpckhps xmm1, xmm3      // e m f n
		movaps   xmm6, xmm7      // a i b j
		unpckhps xmm7, xmm1      // a e i m ---
		unpcklps xmm6, xmm1      // b f j n ---

		unpcklps xmm5, xmm2      // c k d l
		unpcklps xmm0, xmm3      // g o h p
		movaps   xmm4, xmm5      // c k d l
		unpckhps xmm5, xmm0      // c g k o ---
		unpcklps xmm4, xmm0      // d h l p ---
	}
}
// MatrixMultiply3 -- a C++/ASM version of MatrixMultiply2, which takes
// advantage of Intel's SSE instructions.  This version requires that
// M be in column-major order.
//
// Performance: 57 cycles/vector (for Vec4 output)
inline void SSEMatrixMultiply3( const CVec3 *vin, CVec3 *vout )
{
	__asm 
	{
		mov         esi, vin
		mov         edi, vout

		// we'll store the final result in xmm2
		// broadcast x into xmm1, multiply it by the first
		// column of the matrix (xmm4), and add it to the total
		movss   xmm0, [esi]
		movss   xmm1, [esi+4]
		movss   xmm2, [esi+8]
		shufps   xmm0, xmm0, 0
		shufps   xmm1, xmm1, 0
		shufps   xmm2, xmm2, 0
		mulps xmm0, xmm4
		mulps xmm1, xmm5
		mulps xmm2, xmm6
		addps	xmm0, xmm7
		addps xmm1, xmm2
		addps xmm0, xmm1
		// write the results to vout
		//movups   [edi], xmm2
		movss [edi], xmm0
		movaps xmm1, xmm0
		shufps xmm1, xmm1, 0x55
		shufps xmm0, xmm0, 0xaa
		movss [edi+4], xmm1
		movss [edi+8], xmm0
	}
}

// BatchTransform1 -- A modified version of BatchMultiply4 which makes
// an additional assumption about the vectors in vin: if each vector's
// 4th element (the homogenous coordinate w) is assumed to be 1.0 (as is
// the case for 3D vertices), we can eliminate a move, a shuffle and a
// multiply instruction.
//
// Performance: 17 cycles/vector (for Vec4 output)
inline void SSEBatchTransform( const SHMatrix &m, const CVec3 *vin, CVec3 *vout, int len )
{
	SSELoadMatrix( m );

	// if there are an odd number of vectors, process the first one
	// separately and advance the pointers
	if ( len & 0x1 )
	{
		SSEMatrixMultiply3( vin, vout );
		++vin;
		++vout;
	}
	len >>= 1; // we process two vectors at a time

	__asm 
	{
		mov      esi, vin
		mov      edi, vout
		mov      ecx, len

BT2_START:
		// process x (hiding the prefetches in the delays)
		movss      xmm1, [esi+0x00]
		movss      xmm3, [esi+0x00 + 12]
		shufps   xmm1, xmm1, 0x00
		prefetchnta [edi+0x30]
		shufps   xmm3, xmm3, 0x00
		mulps      xmm1, xmm4
		prefetchnta   [esi+0x30]
		mulps      xmm3, xmm4

					// process y
		movss      xmm0, [esi+0x04]
		movss      xmm2, [esi+0x04 + 12]
		shufps   xmm0, xmm0, 0x00
		shufps   xmm2, xmm2, 0x00
		mulps      xmm0, xmm5
		mulps      xmm2, xmm5
		addps      xmm1, xmm0
		addps      xmm3, xmm2

		// process z (hiding some pointer arithmetic between
		// the multiplies)
		movss      xmm0, [esi+0x08]
		movss      xmm2, [esi+0x08 + 12]
		shufps   xmm0, xmm0, 0x00
		shufps   xmm2, xmm2, 0x00
		mulps      xmm0, xmm6
		add         esi, 12 * 2
		mulps      xmm2, xmm6
		add         edi, 12 * 2 // 0x20
		addps      xmm1, xmm0
		addps      xmm3, xmm2

		// process w
		addps      xmm1, xmm7
		addps      xmm3, xmm7

		// write output vectors to memory and loop
		movss [edi-24], xmm1
		movaps xmm0, xmm1
		shufps xmm0, xmm0, 0x55
		shufps xmm1, xmm1, 0xaa
		movss [edi-20], xmm0
		movss [edi-16], xmm1

		movss [edi-12], xmm3
		movaps xmm2, xmm3
		shufps xmm2, xmm2, 0x55
		shufps xmm3, xmm3, 0xaa
		movss [edi-8], xmm2
		movss [edi-4], xmm3
		//movaps   [edi-0x20], xmm1
		//movaps   [edi-0x10], xmm3

		dec         ecx
		jnz         BT2_START
	}
}

struct SSSEVertexWeight
{
	float fWeights[4];
	BYTE nWeights[4];
	BYTE cBoneIndices[4];
};

// xmm7, xmm1, xmm2, xmm3 - transformation matrix, xmm3 is not used since it should always be (0,0,0,1)
static void __forceinline SSEOneVertex( const CVec3 *pSrc, CVec3 *pRes )//__m128 *pM )
{
	__asm
	{
		// transpose transformation matrix
		movaps xmm5, xmm7        // a b c d
		movaps xmm0, xmm1        // e f g h

		unpckhps xmm7, xmm2      // a i b j
		unpckhps xmm1, xmm3      // e m f n
		movaps   xmm6, xmm7      // a i b j
		unpckhps xmm7, xmm1      // a e i m ---
		unpcklps xmm6, xmm1      // b f j n ---

		unpcklps xmm5, xmm2      // c k d l
		unpcklps xmm0, xmm3      // g o h p
		movaps   xmm4, xmm5      // c k d l
		unpckhps xmm5, xmm0      // c g k o ---
		unpcklps xmm4, xmm0      // d h l p ---
//		mov eax, pM
		mov esi, pSrc
		mov edi, pRes

		movss   xmm0, [esi]
		movss   xmm1, [esi+4]
		movss   xmm2, [esi+8]
		shufps   xmm0, xmm0, 0
		shufps   xmm1, xmm1, 0
		shufps   xmm2, xmm2, 0
		mulps xmm0, xmm4
		mulps xmm1, xmm5
		mulps xmm2, xmm6
		addps	xmm0, xmm7
		addps xmm1, xmm2
		addps xmm0, xmm1

		// load v into xmm0.
		//movups   xmm0, [esi]
		//movups   xmm2, xmm0
		//shufps   xmm2, xmm2, 0x00
		//mulps      xmm2, xmm4//[eax]
		//movups   xmm1, xmm0
		//shufps   xmm1, xmm1, 0x55
		//mulps      xmm1, xmm5//[eax+16]
		//addps      xmm2, xmm1
		//movups   xmm1, xmm0
		//shufps   xmm1, xmm1, 0xAA
		//mulps      xmm1, xmm6//[eax+32]
		//addps      xmm2, xmm1
		//addps      xmm2, xmm7//[eax+48]

		movss [edi], xmm0
		movaps xmm1, xmm0
		shufps xmm1, xmm1, 0x55
		shufps xmm0, xmm0, 0xaa
		movss [edi+4], xmm1
		movss [edi+8], xmm0
	}
}

static void SSESkinning( const CVec3 *pSrc, CVec3 *pRes, const SSSEVertexWeight *pWeight, const vector<SHMatrix> &blends, int nCount )
{
	const SHMatrix *pMatrices = &blends[0];
	ASSERT( (((int)pMatrices) & 0xf ) == 0 );
	for ( int k = 0; k < nCount; ++k, ++pSrc, ++pRes, ++pWeight )
	{
		__asm
		{
			mov esi, pMatrices
			mov edi, pWeight
			movzx ebx, byte ptr[edi+20]
			shl ebx, 6
			movaps xmm7, [ebx + esi ]
			movaps xmm1, [ebx + esi + 16]
			movaps xmm2, [ebx + esi + 32]
			movaps xmm3, [ebx + esi + 48]
			cmp byte ptr [edi+ 17], 0
			jz ex
			movss xmm0, [edi]
			shufps xmm0, xmm0, 0
			mulps xmm7, xmm0
			mulps xmm1, xmm0
			mulps xmm2, xmm0
			
			movzx ebx, byte ptr[edi+21]
			shl ebx, 6
			movss xmm0, [edi+4]
			shufps xmm0, xmm0, 0
			movaps xmm4, [ebx + esi]
			movaps xmm5, [ebx + esi + 16]
			movaps xmm6, [ebx + esi + 32]
			mulps xmm4, xmm0
			mulps xmm5, xmm0
			mulps xmm6, xmm0
			addps xmm7, xmm4
			addps xmm1, xmm5
			addps xmm2, xmm6

			cmp byte ptr [edi+ 18], 0
			jz ex
			movzx ebx, byte ptr[edi+22]
			shl ebx, 6
			movss xmm0, [edi+8]
			shufps xmm0, xmm0, 0
			movaps xmm4, [ebx + esi]
			movaps xmm5, [ebx + esi + 16]
			movaps xmm6, [ebx + esi + 32]
			mulps xmm4, xmm0
			mulps xmm5, xmm0
			mulps xmm6, xmm0
			addps xmm7, xmm4
			addps xmm1, xmm5
			addps xmm2, xmm6

			cmp byte ptr [edi+ 19], 0
			jz ex
			movzx ebx, byte ptr[edi+23]
			shl ebx, 6
			movss xmm0, [edi+12]
			shufps xmm0, xmm0, 0
			movaps xmm4, [ebx + esi]
			movaps xmm5, [ebx + esi + 16]
			movaps xmm6, [ebx + esi + 32]
			mulps xmm4, xmm0
			mulps xmm5,xmm0
			mulps xmm6, xmm0
			addps xmm7, xmm4
			addps xmm1, xmm5
			addps xmm2, xmm6
ex:;
		}
		SSEOneVertex( pSrc, pRes );
	}
}

// xform by matrix, perform perspective divide and projection
struct SSSEResultVertex
{
	CVec3 vRes, vSrc;
	int nPad1, nPad2;
};
inline void SSETransformAndProject( const SHMatrix &m, const CVec3 *vin, SSSEResultVertex *vout, int len )
{
	if ( len <= 0 )
		return;
	SSELoadMatrix( m );
	__asm 
	{
		mov         esi, vin
		mov         edi, vout
		mov ecx, len

		// we'll store the final result in xmm2
		// broadcast x into xmm1, multiply it by the first
		// column of the matrix (xmm4), and add it to the total
lp:
		movss   xmm0, [esi]
		movss   xmm1, [esi+4]
		movss   xmm2, [esi+8]
		shufps   xmm0, xmm0, 0
		shufps   xmm1, xmm1, 0
		shufps   xmm2, xmm2, 0
		mulps xmm0, xmm4
		mulps xmm1, xmm5
		mulps xmm2, xmm6
		addps	xmm0, xmm7
		addps xmm1, xmm2
		addps xmm0, xmm1
		// project
		movaps xmm3, xmm0
		shufps xmm3, xmm3, 0xaa
		rcpss xmm3, xmm3
		movaps xmm1, xmm0
		movaps xmm2, xmm0
		shufps xmm2, xmm2, 0x55
		mulss xmm1, xmm3
		mulss xmm2, xmm3
		// write result to vOut
		movss [edi], xmm1
		movss [edi+4], xmm2
		movss [edi+8], xmm3
		movss [edi+12], xmm0
		movaps xmm1, xmm0
		shufps xmm1, xmm1, 0x55
		shufps xmm0, xmm0, 0xaa
		movss [edi+12+4], xmm1
		movss [edi+12+8], xmm0
		// to write out whole line
		mov [edi+24], ecx
		mov [edi+28], ecx
		add esi, 12
		add edi, 32
		dec ecx
		jnz lp
	}
}

// MMX helpers

extern short nNormalizeTable[16384];
extern NGfx::SMMXWord mmxWeights[512];
extern unsigned char nCubicRoot[32768];

struct SMMXFixups
{
	NGfx::SMMXWord normalFixup, shiftedFixup;
};

inline void CreateFixups( SMMXFixups *pRes )
{
	short nShift = (short)0x8000;
	NGfx::SMMXWord &a = pRes->normalFixup;
	a.nZ = nShift; a.nY = nShift; a.nX = nShift; a.nW = 0;
	NGfx::SMMXWord &b = pRes->shiftedFixup;
	short nFixShift = (short)0x8080;
	b.nX = nFixShift; b.nY = nFixShift; b.nZ = nFixShift; b.nW = 0; 
}

static void AssignTransposed( NGfx::SCompactTransformer *pRes, const SHMatrix &m )
{
	pRes->a.nZ = Float2Int( m._33 * 0x800 );  pRes->a.nY = Float2Int( m._22 * 0x800 );  pRes->a.nX = Float2Int( m._11 * 0x800 );  pRes->a.nW = 0;
	pRes->b.nZ = Float2Int( m._13 * 0x800 );  pRes->b.nY = Float2Int( m._32 * 0x800 );  pRes->b.nX = Float2Int( m._21 * 0x800 );  pRes->b.nW = 0;
	pRes->c.nZ = Float2Int( m._23 * 0x800 );  pRes->c.nY = Float2Int( m._12 * 0x800 );  pRes->c.nX = Float2Int( m._31 * 0x800 );  pRes->c.nW = 0;
}

static void Assign( NGfx::SCompactTransformer *pRes, const SHMatrix &m )
{
	pRes->a.nZ = Float2Int( m._33 * 0x800 );  pRes->a.nY = Float2Int( m._22 * 0x800 );  pRes->a.nX = Float2Int( m._11 * 0x800 );  pRes->a.nW = 0;
	pRes->b.nZ = Float2Int( m._31 * 0x800 );  pRes->b.nY = Float2Int( m._23 * 0x800 );  pRes->b.nX = Float2Int( m._12 * 0x800 );  pRes->b.nW = 0;
	pRes->c.nZ = Float2Int( m._32 * 0x800 );  pRes->c.nY = Float2Int( m._21 * 0x800 );  pRes->c.nX = Float2Int( m._13 * 0x800 );  pRes->c.nW = 0;
}

// disable no emms warning, emms is placed after all mmx calcs
#pragma warning( disable : 4799 )
static void MMXTransformVector( NGfx::SCompactVector *pRes, const NGfx::SCompactVector *pSrc, const SMMXFixups *pFixups,
	const NGfx::SCompactTransformer *pTrans )
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
		mov edi, pFixups
		pxor mm0, mm0
		punpcklbw mm0, mm7 // unpacked vector
		psubw mm0, [edi]
		
		movq mm1, mm0    // z y x
		pmulhw mm1, [esi] 
		movq mm2, mm0
		movq mm3, mm0
		psllq mm2, 16
		psrlq mm3, 32
		paddw mm2, mm3   // x z y
		pmulhw mm2, [esi+8]
		movq mm3, mm0
		movq mm4, mm0
		paddsw mm1, mm2
		psllq mm3, 32
		psrlq mm4, 16
		paddw mm3, mm4   // y x z
		pmulhw mm3, [esi+16]
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
		mov ax, [nNormalizeTable + ebx*2]
		movd mm2, eax
		punpcklwd mm2, mm2
		punpckldq mm2, mm2
		pmulhw mm1, mm2
		psllw mm1, 5
		// pack and output result
		paddw mm1, [edi+8]
		psrlw mm1, 8
		packuswb mm1, mm1
		movd edi, mm1
		or ecx, edi
		mov esi, pRes
		mov [esi], ecx
	}
}

static void MMXTransformVector2( NGfx::SCompactVector *pRes, const NGfx::SCompactVector *pSrc, const SMMXFixups *pFixups,
	const NGfx::SCompactTransformer *pTrans, char w1,
	const NGfx::SCompactTransformer *pTrans2, char w2 )
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
		pmulhw mm2, [esi+8]
		pmulhw mm6, [ebx+8]
		movq mm3, mm0
		movq mm4, mm0
		paddsw mm1, mm2
		paddsw mm5, mm6
		psllq mm3, 32
		psrlq mm4, 16
		paddw mm3, mm4   // y x z
		movq mm6, mm3
		pmulhw mm3, [esi+16]
		pmulhw mm6, [ebx+16]
		paddsw mm1, mm3 // packed result
		paddsw mm5, mm6
		movzx esi, w1
		movzx ebx, w2
		psllw mm1, 4
		psllw mm5, 4
		pmulhw mm1, qword ptr[esi*8 + mmxWeights]
		pmulhw mm5, qword ptr[ebx*8 + mmxWeights]
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
		mov ax, [nNormalizeTable + ebx*2]
		movd mm2, eax
		punpcklwd mm2, mm2
		punpckldq mm2, mm2
		pmulhw mm1, mm2
		psllw mm1, 5
		// pack and output result
		paddw mm1, [edi+8]
		psrlw mm1, 8
		packuswb mm1, mm1
		movd edi, mm1
		or ecx, edi
		mov esi, pRes
		mov [esi], ecx
	}
}

static void MMXTransformVector3( NGfx::SCompactVector *pRes, const NGfx::SCompactVector *pSrc, const SMMXFixups *pFixups,
	const NGfx::SCompactTransformer *pTrans, char w1,
	const NGfx::SCompactTransformer *pTrans2, char w2,
	const NGfx::SCompactTransformer *pTrans3, char w3 )
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
		pmulhw mm2, [esi+8]
		pmulhw mm6, [ebx+8]
		pmulhw mm3, [edx+8]
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
		pmulhw mm3, [esi+16]
		pmulhw mm6, [ebx+16]
		pmulhw mm4, [edx+16]
		paddsw mm1, mm3 // packed result
		paddsw mm5, mm6
		paddsw mm7, mm4
		movzx esi, w1
		movzx ebx, w2
		movzx edx, w3
		psllw mm1, 4
		psllw mm5, 4
		psllw mm7, 4
		pmulhw mm1, qword ptr[esi*8 + mmxWeights]
		pmulhw mm5, qword ptr[ebx*8 + mmxWeights]
		pmulhw mm7, qword ptr[edx*8 + mmxWeights]
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
		mov ax, [nNormalizeTable + ebx*2]
		movd mm2, eax
		punpcklwd mm2, mm2
		punpckldq mm2, mm2
		pmulhw mm1, mm2
		psllw mm1, 5
		// pack and output result
		paddw mm1, [edi+8]
		psrlw mm1, 8
		packuswb mm1, mm1
		movd edi, mm1
		or ecx, edi
		mov esi, pRes
		mov [esi], ecx
	}
}
#pragma warning( default : 4799 )
