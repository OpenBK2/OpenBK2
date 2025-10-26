#pragma once
#include "GPixelFormat.h"
#include "MMXhelpers.h"

#include <optional>

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

// no shuffle, store columns in a, b, c
static void AssignRegular( NGfx::SCompactTransformer *pRes, const SHMatrix &m )
{
	pRes->a.nX = FloatToMMXTransformScale( m.xx );  pRes->a.nY = FloatToMMXTransformScale( m.yx );  pRes->a.nZ = FloatToMMXTransformScale( m.zx );  pRes->a.nW = 0;
	pRes->b.nX = FloatToMMXTransformScale( m.xy );  pRes->b.nY = FloatToMMXTransformScale( m.yy );  pRes->b.nZ = FloatToMMXTransformScale( m.zy );  pRes->b.nW = 0;
	pRes->c.nX = FloatToMMXTransformScale( m.xz );  pRes->c.nY = FloatToMMXTransformScale( m.yz );  pRes->c.nZ = FloatToMMXTransformScale( m.zz );  pRes->c.nW = 0;
}

struct ShortVector4 { short x, y, z, w; };

// Helper: apply a single transform with optional shuffle, returns mm64 result
static ShortVector4 ApplyTransform(
	const ShortVector4 & mm0,
	const ShortVector4 & a,
	const ShortVector4 & b,
	const ShortVector4 & c) {

	ShortVector4 result;
	result.x = ((mm0.x * a.x) >> 16) + ((mm0.y * b.x) >> 16) + ((mm0.z * c.x) >> 16);
	result.y = ((mm0.x * a.y) >> 16) + ((mm0.y * b.y) >> 16) + ((mm0.z * c.y) >> 16);
	result.z = ((mm0.x * a.z) >> 16) + ((mm0.y * b.z) >> 16) + ((mm0.z * c.z) >> 16);
	result.w = 0;

	return result;
}

// Helper: apply weight to a transform result (weight is in range [0, 255])
static ShortVector4 ApplyWeight(const ShortVector4 & v, uint8_t weight)
{
	ShortVector4 a;
	const int shift1 = 4;
	a.x = (v.x << shift1);
	a.y = (v.y << shift1);
	a.z = (v.z << shift1);
	ShortVector4 result;
	const int shift2 = 10;
	result.x = (a.x * weight) >> shift2;
	result.y = (a.y * weight) >> shift2;
	result.z = (a.z * weight) >> shift2;
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

static glm::vec4 LoadCompactVector(const NGfx::SCompactVector & src) {

	auto convert = [](const int component) {
		return (component - 128.0) / 127.0f;
	};

	return {convert(src.x), convert(src.y), convert(src.z), 0.0f};
}

static NGfx::SCompactVector SaveCompactVector(const glm::vec3 & src) {

	auto convert = [](const float component) -> uint8_t {
		return std::clamp( static_cast<int>( component * 127 ) + 128, 0, 255 );
	};

	return {convert(src.z), convert(src.y), convert(src.x), 0};
}

static void MMXTransformVector(
	NGfx::SCompactVector & res,
	const NGfx::SCompactVector & src,
	const SHMatrix & transform1)
{
	glm::mat4 matrix1 = LoadMatrix(transform1);
	glm::vec4 vec = LoadCompactVector(src);

	glm::vec4 result = matrix1 * vec;

	glm::vec3 normal = glm::normalize(glm::vec3{ result.x, result.y, result.z });

	res = SaveCompactVector(normal);
	res.w = src.w;
}

static void MMXTransformVector2(
	NGfx::SCompactVector & res,
	const NGfx::SCompactVector & src,
	const SHMatrix & transform1,
	uint8_t weight1,
	const SHMatrix & transform2,
	uint8_t weight2)
{
	glm::mat4 matrix1 = LoadMatrix(transform1);
	glm::vec4 vec = LoadCompactVector(src);

	glm::vec4 result = matrix1 * vec;

	glm::mat4 matrix2 = LoadMatrix(transform2);
	glm::vec4 result2 = matrix2 * vec;

	result = result * (weight1 / 255.f) + result2 * (weight2 / 255.f);

	glm::vec3 normal = glm::normalize(glm::vec3{ result.x, result.y, result.z });

	res = SaveCompactVector(normal);
	res.w = src.w;
}

static void MMXTransformVector3(
	NGfx::SCompactVector & res,
	const NGfx::SCompactVector & src,
	const SHMatrix & transform1,
	uint8_t weight1,
	const SHMatrix & transform2,
	uint8_t weight2,
	const SHMatrix & transform3,
	uint8_t weight3)
{
	glm::mat4 matrix1 = LoadMatrix(transform1);
	glm::vec4 vec = LoadCompactVector(src);

	glm::vec4 result = matrix1 * vec;

	glm::mat4 matrix2 = LoadMatrix(transform2);
	glm::vec4 result2 = matrix2 * vec;

	glm::mat4 matrix3 = LoadMatrix(transform3);
	glm::vec4 result3 = matrix3 * vec;

	result = result * (weight1 / 255.f) + result2 * (weight2 / 255.f) + result3 * (weight3 / 255.f);
	glm::vec3 normal = glm::normalize(glm::vec3{ result.x, result.y, result.z });

	res = SaveCompactVector(normal);
	res.w = src.w;
}
