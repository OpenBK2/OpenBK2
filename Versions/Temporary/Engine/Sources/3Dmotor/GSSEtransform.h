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

static int FloatToMMXTransformScale(const float value) {
	// multiply by 2048 (2^11) to integer fixed point
	return Float2Int( value * 0x800 );
}

static void AssignTransposed( NGfx::SCompactTransformer *pRes, const SHMatrix &m )
{
	//   z  y  x  w
	// a 33 22 11 0
	// b 13 32 21 0
	// c 23 12 31 0
	//   z  y  x
	//   x  z  y
	//   y  x  z

	//   x  y  z  w
	// a 11 22 33 0
	// b 21 32 13 0
	// c 31 12 23 0
	//   x  y  z
	//   y  z  x
	//   z  x  y

	// transposed
	//   x  y  z  w
	// a 11 21 31 0 [x y z]
	// b 22 32 12 0 [y z x]
	// c 33 13 23 0 [z x y]
	pRes->a.nZ = FloatToMMXTransformScale( m._33 );  pRes->a.nY = FloatToMMXTransformScale( m._22 );  pRes->a.nX = FloatToMMXTransformScale( m._11 );  pRes->a.nW = 0;
	pRes->b.nZ = FloatToMMXTransformScale( m._13 );  pRes->b.nY = FloatToMMXTransformScale( m._32 );  pRes->b.nX = FloatToMMXTransformScale( m._21 );  pRes->b.nW = 0;
	pRes->c.nZ = FloatToMMXTransformScale( m._23 );  pRes->c.nY = FloatToMMXTransformScale( m._12 );  pRes->c.nX = FloatToMMXTransformScale( m._31 );  pRes->c.nW = 0;
}

static void Assign( NGfx::SCompactTransformer *pRes, const SHMatrix &m )
{
	//   z  y  x  w
	// a 33 22 11 0 [z y x]
	// b 31 23 12 0 [x z y]
	// c 32 31 13 0 [y x z]

	//   x  y  z  w
	// a 11 22 33 0 [x y z]
	// b 12 23 31 0 [y z x]
	// c 13 31 32 0 [z x y]
	pRes->a.nZ = FloatToMMXTransformScale( m._33 );  pRes->a.nY = FloatToMMXTransformScale( m._22 );  pRes->a.nX = FloatToMMXTransformScale( m._11 );  pRes->a.nW = 0;
	pRes->b.nZ = FloatToMMXTransformScale( m._31 );  pRes->b.nY = FloatToMMXTransformScale( m._23 );  pRes->b.nX = FloatToMMXTransformScale( m._12 );  pRes->b.nW = 0;
	pRes->c.nZ = FloatToMMXTransformScale( m._32 );  pRes->c.nY = FloatToMMXTransformScale( m._21 );  pRes->c.nX = FloatToMMXTransformScale( m._13 );  pRes->c.nW = 0;
}

struct ShortVector4 { short x, y, z, w; };

// Helper: apply a single transform with optional shuffle, returns mm64 result
static ShortVector4 ApplyTransform(
	const ShortVector4 & mm0,
	const ShortVector4 & a,
	const ShortVector4 & b,
	const ShortVector4 & c) {
	ShortVector4 va, vb, vc;

	auto dot = [](const ShortVector4 & v1, const ShortVector4 & v2){
		ShortVector4 r;
		r.x = (v1.x * v2.x) >> 16;
		r.y = (v1.y * v2.y) >> 16;
		r.z = (v1.z * v2.z) >> 16;
		r.w = 0;
		return r;
	};

	va = dot(mm0, a);
	vb = dot(ShortVector4{mm0.y, mm0.z, mm0.x}, b);
	vc = dot(ShortVector4{mm0.z, mm0.x, mm0.y}, c);

	ShortVector4 result;
	int x = va.x + vb.x + vc.x;
	int y = va.y + vb.y + vc.y;
	int z = va.z + vb.z + vc.z;
	result.x = std::clamp(x, -32768, 32767);
	result.y = std::clamp(y, -32768, 32767);
	result.z = std::clamp(z, -32768, 32767);
	result.w = 0;

	return result;
}

// Helper: apply weight to a transform result (weight is in range [0, 255])
static ShortVector4 ApplyWeight(const ShortVector4 & v, unsigned char wIdx)
{
	ShortVector4 a;
	const int shift1 = 4;
	a.x = (v.x << shift1);
	a.y = (v.y << shift1);
	a.z = (v.z << shift1);
	ShortVector4 result;
	auto w = wIdx;
	const int shift2 = 10;
	result.x = (a.x * w) >> shift2;
	result.y = (a.y * w) >> shift2;
	result.z = (a.z * w) >> shift2;
	result.w = 0;
	return result;
}

// Shared final normalization
static ShortVector4 NormalizeAndShift(const ShortVector4 & v)
{
	ShortVector4 w;
	w.x = v.x << 3;
	w.y = v.y << 3;
	w.z = v.z << 3;
	w.w = 0;

	uint32_t sumSquares = w.x * w.x + w.y * w.y + w.z * w.z;

	uint32_t idx = sumSquares >> 18;

	short normalize = (std::min)( 0x7fff, Float2Int( (64 * (127 * 16)) / sqrt( idx + 0.99f ) ) );

	// divide back by 2048 (2^11)
	const size_t shift = 11;
	w.x = (w.x * normalize) >> shift;
	w.y = (w.y * normalize) >> shift;
	w.z = (w.z * normalize) >> shift;
	w.w = 0;

	return w;
}

// General template function for N transforms (1..3)
static void MMXTransformVectorGeneral(
	NGfx::SCompactVector* pRes,
	const NGfx::SCompactVector* pSrc,
	const NGfx::SCompactTransformer* pTrans1, uint8_t w1 = 0,
	const NGfx::SCompactTransformer* pTrans2 = nullptr, uint8_t w2 = 0,
	const NGfx::SCompactTransformer* pTrans3 = nullptr, uint8_t w3 = 0)
{
	ShortVector4 mm0;
	// mm0 *= 256, SCompactVector scaled from range [0, 255] into [0, 65535]
	mm0.z = static_cast<short>(pSrc->z) << 8;
	mm0.y = static_cast<short>(pSrc->y) << 8;
	mm0.x = static_cast<short>(pSrc->x) << 8;
	mm0.w = 0;

	// subtract 0x8000 = 32768, range [-32768, 32767]
	mm0.z -= fixups.normalFixup.nZ;
	mm0.y -= fixups.normalFixup.nY;
	mm0.x -= fixups.normalFixup.nX;

	// Apply first transform
	const ShortVector4 a1 = {pTrans1->a.nX, pTrans1->a.nY, pTrans1->a.nZ, pTrans1->a.nW };
	const ShortVector4 b1 = {pTrans1->b.nX, pTrans1->b.nY, pTrans1->b.nZ, pTrans1->b.nW };
	const ShortVector4 c1 = {pTrans1->c.nX, pTrans1->c.nY, pTrans1->c.nZ, pTrans1->c.nW };
	ShortVector4 mm = ApplyTransform(mm0, a1, b1, c1);

	// Second transform
	if (pTrans2) {

		mm = ApplyWeight(mm, w1);

		const ShortVector4 a2 = {pTrans2->a.nX, pTrans2->a.nY, pTrans2->a.nZ, pTrans2->a.nW };
		const ShortVector4 b2 = {pTrans2->b.nX, pTrans2->b.nY, pTrans2->b.nZ, pTrans2->b.nW };
		const ShortVector4 c2 = {pTrans2->c.nX, pTrans2->c.nY, pTrans2->c.nZ, pTrans2->c.nW };
		ShortVector4 mm2 = ApplyTransform(mm0, a2, b2, c2);
		mm2 = ApplyWeight(mm2, w2);

		mm.x += mm2.x;
		mm.y += mm2.y;
		mm.z += mm2.z;
	}

	// Third transform
	if (pTrans3) {
		const ShortVector4 a3 = {pTrans3->a.nX, pTrans3->a.nY, pTrans3->a.nZ, pTrans3->a.nW };
		const ShortVector4 b3 = {pTrans3->b.nX, pTrans3->b.nY, pTrans3->b.nZ, pTrans3->b.nW };
		const ShortVector4 c3 = {pTrans3->c.nX, pTrans3->c.nY, pTrans3->c.nZ, pTrans3->c.nW };
		ShortVector4 mm3 = ApplyTransform(mm0, a3, b3, c3);
		mm3 = ApplyWeight(mm3, w3);

		mm.x += mm3.x;
		mm.y += mm3.y;
		mm.z += mm3.z;
	}

	ShortVector4 w = NormalizeAndShift(mm);

	w.z += fixups.shiftedFixup.nZ;
	w.y += fixups.shiftedFixup.nY;
	w.x += fixups.shiftedFixup.nX;

	w.z = static_cast<short>(std::clamp(0xFF & (w.z >> 8), 0, 255));
	w.y = static_cast<short>(std::clamp(0xFF & (w.y >> 8), 0, 255));
	w.x = static_cast<short>(std::clamp(0xFF & (w.x >> 8), 0, 255));

	pRes->z = static_cast<unsigned char>(w.z);
	pRes->y = static_cast<unsigned char>(w.y);
	pRes->x = static_cast<unsigned char>(w.x);
	pRes->w = pSrc->w;
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
	uint8_t w1,
	const NGfx::SCompactTransformer* pTrans2,
	uint8_t w2)
{
	MMXTransformVectorGeneral(pRes, pSrc, pTrans, w1, pTrans2, w2);
}

static void MMXTransformVector3(
	NGfx::SCompactVector* pRes,
	const NGfx::SCompactVector* pSrc,
	const NGfx::SCompactTransformer* pTrans,
	uint8_t w1,
	const NGfx::SCompactTransformer* pTrans2,
	uint8_t w2,
	const NGfx::SCompactTransformer* pTrans3,
	uint8_t w3)
{
	MMXTransformVectorGeneral(pRes, pSrc, pTrans, w1, pTrans2, w2, pTrans3, w3);
}
