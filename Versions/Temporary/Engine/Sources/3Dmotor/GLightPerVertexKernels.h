#pragma once
// GPixelFormat.h and GLightPerVertex.h both use CVec3/CVec4 without including
// Geom.h themselves, so pull it in first and keep this header self-sufficient.
#include "Misc/Geom.h"
#include "GPixelFormat.h"
#include "GLightPerVertex.h"

// Cube root lookup table, built once in GSSEtransform.cpp. AddColors indexes it,
// so every kernel set needs it; declared here to keep those translation units
// from pulling in GSSEtransform.h and its glm dependency.
extern unsigned char nCubicRoot[32768];

namespace NGScene
{

// Point light attenuation constants. Shared by the kernels and by AddPointLight,
// which folds a whole object into a single attenuation value with the same formula.
constexpr float F_PL_RADIUS2 = 64;
constexpr float F_PL_MIN_DISTANCE_NORMALIZED = 0.25f;
constexpr int N_PL_ATTENUATION_SCALE = 8191;

// One complete set of per-vertex lighting kernels. There are three: a scalar
// reference (GLightPerVertexRef.cpp), SSE2 (GLightPerVertexSSE2.cpp) and AVX2
// (GLightPerVertexAVX2.cpp); GetLightingKernels() picks one at startup from CPUID.
//
// Every entry is a whole-array transform over raw pointers, and the caller owns all
// allocation. That is deliberate: GLightPerVertexAVX2.cpp is compiled with /arch:AVX2,
// so any inline function it emits - std::vector<T>::resize included - would be an
// AVX2-compiled COMDAT that the linker may pick over the baseline copy and then run
// on a CPU without AVX2. Keeping std::vector and CArray2D out of those translation
// units keeps the AVX2 code confined to the kernels themselves.
//
// Values derived from floats (translucentShade, lightColor) are converted to the
// fixed point SMMXWord form by the caller, so the kernel TUs stay integer only.
struct SLightingKernels
{
	// Identifies the set in test and benchmark output.
	const char *pszName;

	// pNormals and both outputs hold nCount entries.
	void ( *pCalcDirectionalLighting )(
		const NGfx::SCompactVector *pNormals, int nCount,
		const SPerVertexLightState &ls, const NGfx::SMMXWord &translucentShade,
		DWORD *pResColors, DWORD *pResShadow );

	// Scales nVertices positions into 2 * nVertices 16.14 fixed point x/y pairs.
	void ( *pSampleWarFogCoords )(
		const CVec3 *pSrcPos, int nVertices, float fScale, int *pIntCoords );

	// pFog is a square nFogSizeX by nFogSizeX array with contiguous rows, as laid
	// out by CArray2D. pRes holds nVertices bytes.
	void ( *pSampleWarFogInt )(
		const int *pIntCoords, const unsigned char *pFog, int nFogSizeX,
		unsigned char *pRes, int nVertices );

	void ( *pCalcPointLightAttenuation )(
		NGfx::SMMXWord *pRes, const CVec3 *pSrcPos, int nCount,
		const CVec3 &vCenter, float fRadius );

	// Per vertex attenuation, looked up as pAttenuation[pPosIndices[k]].
	void ( *pCalcPointLightColorsIndexed )(
		NGfx::SMMXWord *pRes, const NGfx::SMMXWord *pAttenuation,
		const WORD *pPosIndices, const NGfx::SCompactVector *pNormals, int nCount,
		const NGfx::SMMXWord &lightColor );

	// One attenuation for every vertex, normals strided through a larger struct.
	void ( *pCalcPointLightColorsUniform )(
		NGfx::SMMXWord *pRes, const NGfx::SMMXWord &attenuation,
		const NGfx::SCompactVector *pNormals, int nNormalStride, int nCount,
		const NGfx::SMMXWord &lightColor );

	void ( *pAddColors )(
		DWORD *pRes, const DWORD *pSrc, const NGfx::SMMXWord *pAdd, int nCount );

	void ( *pScaleColors )(
		DWORD *pRes, const DWORD *pSrc, int nSrcStride,
		const unsigned char *pScale, int nScaleMask,
		const WORD *pPosIndices, const NGfx::SCompactVector *pTransp, int nCount,
		bool bMultiplyOnTransparency );
};

// The three implementations. All three are always linked, so tests and benchmarks
// can reach any of them directly; check IsSSE2Present()/IsAVX2Present() before
// calling one the running CPU does not support.
extern const SLightingKernels refLightingKernels;
extern const SLightingKernels sse2LightingKernels;
extern const SLightingKernels avx2LightingKernels;

bool IsSSE2Present();
bool IsAVX2Present();

// Best set the running CPU supports. Resolved once, on first call.
const SLightingKernels &GetLightingKernels();

}
