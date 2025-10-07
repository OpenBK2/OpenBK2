#pragma once
#include "GPixelFormat.h"

#include <glm/glm.hpp>

extern bool bIsSSEPresent;

inline glm::mat4 LoadMatrix(const SHMatrix & m) {
	// SHMatrix is row-major
	// glm is column-major
	return glm::mat4{
		m._11, m._21, m._31, m._41,
		m._12, m._22, m._32, m._42,
		m._13, m._23, m._33, m._43,
		m._14, m._24, m._34, m._44,
	};
}

inline void MatrixMultiplyVec3(const glm::mat4 & M, const CVec3 & vin, CVec3 & vout) {
	glm::vec4 v{vin.x, vin.y, vin.z, 1.0f};
	glm::vec4 res = M * v;
	vout.x = res.x;
	vout.y = res.y;
	vout.z = res.z;
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
	auto matrix = LoadMatrix(m);
	for (int i = 0; i < len; ++i) {
		MatrixMultiplyVec3(matrix, vin[i], vout[i]);
	}

}

struct SSSEVertexWeight
{
	float fWeights[4];
	BYTE nWeights[4];
	BYTE cBoneIndices[4];
};

static void SSESkinning( const CVec3 *pSrc, CVec3 *pRes, const SSSEVertexWeight *pWeight, const std::vector<SHMatrix> &blends, int nCount )
{
	for (int k = 0; k < nCount; ++k, ++pSrc, ++pRes, ++pWeight) {
		glm::mat4 blended{0.0f};

		for (int i = 0; i < 4; ++i) {
			float w = pWeight->fWeights[i];
			if (w == 0.0f) {
				continue;
			}
			int boneIndex = pWeight->cBoneIndices[i];
			glm::mat4 m = LoadMatrix(blends[boneIndex]);

			blended += w * m;
		}

		glm::vec4 v(pSrc->x, pSrc->y, pSrc->z, 1.0f);
		glm::vec4 r = blended * v;

		pRes->x = r.x;
		pRes->y = r.y;
		pRes->z = r.z;
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

	glm::mat4 matrix = LoadMatrix(m);

	for (int i = 0; i < len; ++i) {

		glm::vec4 v{vin[i].x, vin[i].y, vin[i].z, 1.0f};

		// transform
		glm::vec4 t = matrix * v;

		// perspective divide
		float invw = 1.0f / t.w;

		CVec3 proj{t.x * invw, t.y * invw, t.z * invw};

		vout[i].vSrc = vin[i];
		vout[i].vRes = proj;
		vout[i].nPad1 = 0;
		vout[i].nPad2 = 0;
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

