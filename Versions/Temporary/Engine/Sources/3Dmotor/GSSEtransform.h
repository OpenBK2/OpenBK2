#pragma once
#include "GPixelFormat.h"
#include "MMXhelpers.h"

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

static constexpr short normalFixupValue = static_cast<short>(0x8000);
static constexpr short shiftedFixupValue = static_cast<short>(0x8080);

static constexpr SMMXFixups fixups = {
	{normalFixupValue, normalFixupValue, normalFixupValue, 0},
	{shiftedFixupValue, shiftedFixupValue, shiftedFixupValue, 0}
};

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

// Helper: apply a single transform with optional shuffle, returns mm64 result
static uint64_t ApplyTransform(
	uint64_t mm0,
	const short a[4],
	const short b[4],
	const short c[4])
{
	uint64_t mmA = mmx::pmulhw(mm0, mmx::combine64(a));
	uint64_t mmB = mmx::pmulhw(mmx::shuffleTransform(mm0, 16, 32), mmx::combine64(b));
	uint64_t mmC = mmx::pmulhw(mmx::shuffleTransform(mm0, 32, 16), mmx::combine64(c));
	mmA = mmx::paddsw(mmA, mmB);
	mmA = mmx::paddsw(mmA, mmC);
	return mmA;
}

// Helper: apply weight to a transform result
static uint64_t ApplyWeight(uint64_t mm, unsigned char wIdx)
{
	mm = mmx::psllw(mm, 4); // shift before weight
	short wArr[4] = { mmxWeights[wIdx].nZ, mmxWeights[wIdx].nY, mmxWeights[wIdx].nX, mmxWeights[wIdx].nW };
	return mmx::pmulhw(mm, mmx::combine64(wArr));
}

// Shared final normalization
static uint64_t NormalizeAndShift(uint64_t mm)
{
	mm = mmx::psllw(mm, 3);
	uint64_t mmSq = mmx::pmaddwd(mm, mm);
	uint32_t sumSquares = static_cast<uint32_t>(mmSq & 0xFFFFFFFFULL) +
		static_cast<uint32_t>((mmSq >> 32) & 0xFFFFFFFFULL);
	uint32_t idx = std::min(sumSquares >> 18, 16383U);
	short normalize = nNormalizeTable[idx];
	short normWords[4] = { normalize, normalize, normalize, normalize };
	mm = mmx::pmulhw(mm, mmx::combine64(normWords));
	mm = mmx::psllw(mm, 5);
	return mm;
}

// General template function for N transforms (1..3)
static void MMXTransformVectorGeneral(
	NGfx::SCompactVector* pRes,
	const NGfx::SCompactVector* pSrc,
	const NGfx::SCompactTransformer* pTrans1, char w1 = 0,
	const NGfx::SCompactTransformer* pTrans2 = nullptr, char w2 = 0,
	const NGfx::SCompactTransformer* pTrans3 = nullptr, char w3 = 0)
{
	uint32_t src32 = *(const uint32_t*)pSrc;
	uint32_t low = src32 & 0x00FFFFFF;
	uint32_t high = src32 & 0xFF000000;

	uint64_t mm0 = mmx::punpcklbw(0, low);

	short fix[4] = {
		fixups.normalFixup.nZ,
		fixups.normalFixup.nY,
		fixups.normalFixup.nX,
		fixups.normalFixup.nW
	};

	short mm0_words[4];
	mmx::split64(mm0, mm0_words);
	for (int i = 0; i < 4; ++i) mm0_words[i] -= fix[i];
	mm0 = mmx::combine64(mm0_words);

	// Apply first transform
	short a1[4] = { pTrans1->a.nZ, pTrans1->a.nY, pTrans1->a.nX, pTrans1->a.nW };
	short b1[4] = { pTrans1->b.nZ, pTrans1->b.nY, pTrans1->b.nX, pTrans1->b.nW };
	short c1[4] = { pTrans1->c.nZ, pTrans1->c.nY, pTrans1->c.nX, pTrans1->c.nW };
	uint64_t mm = ApplyTransform(mm0, a1, b1, c1);

	// Second transform
	if (pTrans2) {

		mm = ApplyWeight(mm, static_cast<unsigned char>(w1));

		short a2[4] = { pTrans2->a.nZ, pTrans2->a.nY, pTrans2->a.nX, pTrans2->a.nW };
		short b2[4] = { pTrans2->b.nZ, pTrans2->b.nY, pTrans2->b.nX, pTrans2->b.nW };
		short c2[4] = { pTrans2->c.nZ, pTrans2->c.nY, pTrans2->c.nX, pTrans2->c.nW };
		uint64_t mm2 = ApplyTransform(mm0, a2, b2, c2);
		mm2 = ApplyWeight(mm2, static_cast<unsigned char>(w2));
		mm = mmx::paddsw(mm, mm2);
	}

	// Third transform
	if (pTrans3) {
		short a3[4] = { pTrans3->a.nZ, pTrans3->a.nY, pTrans3->a.nX, pTrans3->a.nW };
		short b3[4] = { pTrans3->b.nZ, pTrans3->b.nY, pTrans3->b.nX, pTrans3->b.nW };
		short c3[4] = { pTrans3->c.nZ, pTrans3->c.nY, pTrans3->c.nX, pTrans3->c.nW };
		uint64_t mm3 = ApplyTransform(mm0, a3, b3, c3);
		mm3 = ApplyWeight(mm3, static_cast<unsigned char>(w3));
		mm = mmx::paddsw(mm, mm3);
	}

	mm = NormalizeAndShift(mm);

	short w[4];
	mmx::split64(mm, w);

	w[0] += fixups.shiftedFixup.nZ;
	w[1] += fixups.shiftedFixup.nY;
	w[2] += fixups.shiftedFixup.nX;
	w[3] += fixups.shiftedFixup.nW;

	for (int i = 0; i < 4; ++i) {
		int val = 0xFF & (w[i] >> 8);
		w[i] = static_cast<short>(std::clamp(val, 0, 255));
	}

	pRes->z = static_cast<unsigned char>(w[0]);
	pRes->y = static_cast<unsigned char>(w[1]);
	pRes->x = static_cast<unsigned char>(w[2]);
	pRes->w = static_cast<unsigned char>(w[3]) | pSrc->w;
}

static void MMXTransformVector(
	NGfx::SCompactVector* pRes,
	const NGfx::SCompactVector* pSrc,
	const NGfx::SCompactTransformer* pTrans)
{
	MMXTransformVectorGeneral(pRes, pSrc, pTrans);
}

static void MMXTransformVector2(
	NGfx::SCompactVector* pRes,
	const NGfx::SCompactVector* pSrc,
	const NGfx::SCompactTransformer* pTrans,
	char w1,
	const NGfx::SCompactTransformer* pTrans2,
	char w2)
{
	MMXTransformVectorGeneral(pRes, pSrc, pTrans, w1, pTrans2, w2);
}

static void MMXTransformVector3(
	NGfx::SCompactVector* pRes,
	const NGfx::SCompactVector* pSrc,
	const NGfx::SCompactTransformer* pTrans,
	char w1,
	const NGfx::SCompactTransformer* pTrans2,
	char w2,
	const NGfx::SCompactTransformer* pTrans3,
	char w3)
{
	MMXTransformVectorGeneral(pRes, pSrc, pTrans, w1, pTrans2, w2, pTrans3, w3);
}
