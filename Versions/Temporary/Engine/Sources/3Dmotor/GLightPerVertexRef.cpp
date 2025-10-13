// Scalar reference implementation of the per-vertex lighting kernels.
//
// This is the mmx:: emulation of the original 2005 MMX assembly, kept as the
// baseline the SSE2 and AVX2 sets are checked against and as the fallback for a
// CPU without SSE2. Clarity beats speed here: nothing in this file is meant to
// be fast, only to be obviously the same arithmetic as the original.

#include "stdafx.h"
#include "GLightPerVertexKernels.h"
#include "MMXhelpers.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace NGScene
{
namespace
{

static void CalcDirectionalLighting(
	const NGfx::SCompactVector *pNormals, int nCount,
	const SPerVertexLightState &ls, const NGfx::SMMXWord &translucentShade,
	uint32_t *pResColors, uint32_t *pResShadow )
{
	uint32_t dwColor = 0, dwShadowColor = 0, dwPrevNormal = 0;
	for ( int k = 0; k < nCount; ++k )
	{
		uint32_t dwNormal = pNormals[k].dw;
		if ( dwNormal != dwPrevNormal )
		{
			uint64_t normal = mmx::punpcklbw( dwNormal, dwNormal );

			auto combine_mmx_word = []( auto w ) { return mmx::combine64( w.nZ, w.nY, w.nX, w.nW ); };

			uint64_t shift = combine_mmx_word( ls.shift );
			uint64_t shadeColor = combine_mmx_word( ls.shadeColor );
			uint64_t dirLight = combine_mmx_word( ls.dirLight );
			uint64_t ambient = combine_mmx_word( ls.ambient );
			uint64_t lightColor = combine_mmx_word( ls.lightColor );
			uint64_t incidentShadowColor = combine_mmx_word( ls.incidentShadowColor );
			uint64_t translucent = combine_mmx_word( translucentShade );
			uint64_t vResShadow = ambient;
			uint64_t vRes = ambient;

			uint64_t shifted_normal = mmx::psubw( normal, shift );
			shifted_normal = mmx::pmaddwd( shifted_normal, dirLight );
			uint64_t normal_high = mmx::psrlq( shifted_normal, 32 );
			shifted_normal = mmx::paddd( shifted_normal, normal_high );
			shifted_normal = mmx::psrad( shifted_normal, 15 );
			shifted_normal = mmx::punpcklwd( shifted_normal, shifted_normal );
			shifted_normal = mmx::punpckldq( shifted_normal, shifted_normal );
			uint64_t sign = mmx::psraw( shifted_normal, 16 );
			uint64_t negative_f = mmx::pand( shifted_normal, sign );
			uint64_t f = mmx::pandn( sign, shifted_normal );
			const uint64_t mask = 0xFFFFFFFFFFFFFFFFULL;
			negative_f = mmx::pxor( negative_f, mask );

			lightColor = mmx::pmulhw( lightColor, f );
			incidentShadowColor = mmx::pmulhw( incidentShadowColor, f );
			translucent = mmx::pmulhw( translucent, negative_f );
			shadeColor = mmx::pmulhw( shadeColor, negative_f );

			vRes = mmx::paddw( vRes, lightColor );
			vResShadow = mmx::paddw( vResShadow, incidentShadowColor );
			vRes = mmx::paddw( vRes, translucent );
			vResShadow = mmx::paddw( vResShadow, shadeColor );
			vRes = mmx::psraw( vRes, 4 );
			vResShadow = mmx::psraw( vResShadow, 4 );

			dwColor = mmx::packuswb( vRes, vRes );
			dwShadowColor = mmx::packuswb( vResShadow, vResShadow );
		}

		pResColors[k] = dwColor;
		pResShadow[k] = dwShadowColor;
		dwPrevNormal = dwNormal;
	}
}

static void SampleWarFogCoords( const CVec3 *pSrcPos, int nVertices, float fScale, int *pIntCoords )
{
	// 16.14 fixed point, matching the shifts SampleWarFogInt applies.
	const float fpScale = fScale * 0x4000;
	int *pOut = pIntCoords;
	for ( int k = 0; k < nVertices; ++k )
	{
		const CVec3 &v = pSrcPos[k];
		*pOut++ = static_cast<int>( std::lrintf( v.x * fpScale ) );
		*pOut++ = static_cast<int>( std::lrintf( v.y * fpScale ) );
	}
}

// No MMX intrinsics here, only the mmx:: integer emulation, so the C4799
// "no EMMS instruction" suppression the original needed no longer applies.
static void SampleWarFogInt(
	const int *pIntCoords, const unsigned char *pFog, int nFogSizeX,
	unsigned char *pRes, int nVertices )
{
	ASSERT( GetNextPow2( nFogSizeX - 1 ) + 1 == nFogSizeX );
	uint64_t zero = 0;
	int nMask = nFogSizeX - 2;
	for ( const int *pTmp = pIntCoords, *pTmpEnd = pTmp + nVertices * 2; pTmp < pTmpEnd; pTmp += 2, ++pRes )
	{
		int nY = pTmp[1];
		int nYU = ( nY >> 14 ) & nMask;
		int nYfi = nY & 0x3fff;
		uint64_t nYf = ( 0x4000 - nYfi ) | ( nYfi << 16 );
		int nX = pTmp[0];
		int nXL = ( nX >> 14 ) & nMask;
		int nXfi = nX & 0x3fff;
		uint64_t nXf = ( 0x4000 - nXfi ) | ( nXfi << 16 );
		// CArray2D rows are contiguous, so row nYU starts at nYU * nFogSizeX.
		const unsigned char *pUp = pFog + nYU * nFogSizeX + nXL;
		const unsigned char *pDown = pUp + nMask + 2;

		uint64_t nData = mmx::punpckldq(
			mmx::punpcklbw( *(unsigned short*)pUp, zero ),
			mmx::punpcklbw( *(unsigned short*)pDown, zero )
			);
		nXf = mmx::punpckldq( nXf, nXf );
		nData = mmx::pmaddwd( nData, nXf );
		nData = mmx::psrad( nData, 14 );
		nData = mmx::packssdw( nData, zero );
		nData = mmx::pmaddwd( nData, nYf );
		*pRes = nData >> 14;
	}
}

static void CalcPointLightAttenuation(
	NGfx::SMMXWord *pRes, const CVec3 *pSrcPos, int nCount,
	const CVec3 &vCenter, float fRadius )
{
	float fScale = F_PL_RADIUS2 / sqr( fRadius );
	for ( int k = 0; k < nCount; ++k )
	{
		CVec3 v = vCenter - pSrcPos[k];
		float f = fabs2( v );
		float fCut = (std::max)( 0.0f, sqr( fRadius ) - f ) * ( 1 / sqr( fRadius ) );
		float fAttenuation = fCut / ( f * fScale + F_PL_MIN_DISTANCE_NORMALIZED ) / sqrt( f );
		NGfx::SMMXWord &n = pRes[k];
		n.nX = Float2Int( v.x * fAttenuation * N_PL_ATTENUATION_SCALE );
		n.nY = Float2Int( v.y * fAttenuation * N_PL_ATTENUATION_SCALE );
		n.nZ = Float2Int( v.z * fAttenuation * N_PL_ATTENUATION_SCALE );
		n.nW = 0;
	}
}

static void CalculateLightColor(
	uint32_t dwNormal, uint64_t shift, uint64_t shift1, uint64_t lightColor,
	uint64_t att, NGfx::SMMXWord *pResColor )
{
	uint64_t resColor = mmx::combine64( pResColor->nZ, pResColor->nY, pResColor->nX, pResColor->nW );

	uint64_t normal = mmx::punpcklbw( dwNormal, dwNormal );
	normal = mmx::psubw( normal, shift );
	normal = mmx::pmaddwd( normal, att );
	uint64_t normal_high = mmx::psrlq( normal, 32 );
	normal = mmx::paddd( normal, normal_high );
	normal = mmx::psrad( normal, 15 );
	normal = mmx::packssdw( normal, normal );
	normal = mmx::punpcklwd( normal, normal );
	normal = mmx::punpckldq( normal, normal );
	uint64_t mask = mmx::pcmpgtw( normal, 0 );
	normal = mmx::pand( normal, mask );
	uint64_t low = mmx::pmullw( normal, lightColor );
	uint64_t high = mmx::pmulhw( normal, lightColor );
	uint64_t unpacked_low = mmx::punpcklwd( low, high );
	uint64_t unpacked_high = mmx::punpckhwd( low, high );
	unpacked_low = mmx::paddd( unpacked_low, shift1 );
	unpacked_high = mmx::paddd( unpacked_high, shift1 );
	unpacked_low = mmx::psrad( unpacked_low, 13 );
	unpacked_high = mmx::psrad( unpacked_high, 13 );
	uint64_t result = mmx::packssdw( unpacked_low, unpacked_high );
	result = mmx::paddsw( result, resColor );
	mmx::split64( result, pResColor->nZ, pResColor->nY, pResColor->nX, pResColor->nW );
}

static uint64_t PointLightShift()
{
	return mmx::combine64(
		static_cast<int16_t>( 0x8000 ), static_cast<int16_t>( 0x8000 ),
		static_cast<int16_t>( 0x8000 ), 0 );
}

static void CalcPointLightColorsIndexed(
	NGfx::SMMXWord *pRes, const NGfx::SMMXWord *pAttenuation,
	const uint16_t *pPosIndices, const NGfx::SCompactVector *pNormals, int nCount,
	const NGfx::SMMXWord &lightColorWord )
{
	uint64_t shift = PointLightShift();
	uint64_t shift1 = mmx::combine64( 1 << 12, 0, 1 << 12, 0 );
	uint64_t lightColor = mmx::combine64(
		lightColorWord.nZ, lightColorWord.nY, lightColorWord.nX, lightColorWord.nW );

	for ( int k = 0; k < nCount; ++k )
	{
		uint32_t dwNormal = pNormals[k].dw;
		const NGfx::SMMXWord *pAtt = &pAttenuation[ pPosIndices[k] ];
		uint64_t att = mmx::combine64( pAtt->nZ, pAtt->nY, pAtt->nX, pAtt->nW );
		CalculateLightColor( dwNormal, shift, shift1, lightColor, att, &pRes[k] );
	}
}

static void CalcPointLightColorsUniform(
	NGfx::SMMXWord *pRes, const NGfx::SMMXWord &attenuation,
	const NGfx::SCompactVector *pNormals, int nNormalStride, int nCount,
	const NGfx::SMMXWord &lightColorWord )
{
	uint64_t shift = PointLightShift();
	uint64_t shift1 = mmx::combine64( 1 << 12, 0, 1 << 12, 0 );
	uint64_t lightColor = mmx::combine64(
		lightColorWord.nZ, lightColorWord.nY, lightColorWord.nX, lightColorWord.nW );
	uint64_t att = mmx::combine64( attenuation.nZ, attenuation.nY, attenuation.nX, attenuation.nW );

	// The original assembly cached the light contribution for a repeated normal and
	// re-added it; that contribution is identical for identical normals under a fixed
	// attenuation, so recomputing per vertex gives the same result with less state.
	const unsigned char *pNormalBytes = reinterpret_cast<const unsigned char *>( pNormals );
	for ( int k = 0; k < nCount; ++k, pNormalBytes += nNormalStride )
	{
		uint32_t dwNormal = reinterpret_cast<const NGfx::SCompactVector *>( pNormalBytes )->dw;
		CalculateLightColor( dwNormal, shift, shift1, lightColor, att, &pRes[k] );
	}
}

static void AddColors( uint32_t *pRes, const uint32_t *pSrc, const NGfx::SMMXWord *pAdd, int nCount )
{
	uint64_t mask = 0x4000400040004000ULL;

	for ( int k = 0; k < nCount; ++k )
	{
		uint32_t dwColor = pSrc[k];
		uint64_t addColor = mmx::combine64( pAdd[k].nZ, pAdd[k].nY, pAdd[k].nX, pAdd[k].nW );

		uint64_t color = mmx::punpcklbw( dwColor, dwColor );
		color = mmx::psrlw( color, 1 );
		uint64_t color_square = mmx::pmulhw( color, color );
		color_square = mmx::psllw( color_square, 1 );
		uint64_t color_cube_high = mmx::pmulhw( color_square, color );
		uint64_t color_cube_low = mmx::pmullw( color_square, color );
		color_cube_high = mmx::psllw( color_cube_high, 1 );
		color_cube_high = mmx::paddsw( color_cube_high, addColor );
		color_cube_high = mmx::psrlw( color_cube_high, 1 );
		color_cube_low = mmx::psrlw( color_cube_low, 2 );
		color_cube_low = mmx::por( color_cube_low, mask );
		uint64_t eq = mmx::pcmpeqw( color_cube_high, 0 );
		color_cube_low = mmx::pand( color_cube_low, eq );
		color_cube_high = mmx::pandn( eq, color_cube_high );
		uint64_t color_cube = mmx::por( color_cube_high, color_cube_low );
		uint64_t color_cube_hi32 = mmx::psrlq( color_cube, 32 );
		uint32_t index1 = color_cube & 0x7FFF;
		uint32_t index2 = ( color_cube >> 16 ) & 0x7FFF;
		uint32_t index3 = color_cube_hi32 & 0x7FFF;
		uint8_t c1 = nCubicRoot[index1];
		uint8_t c2 = nCubicRoot[index2];
		uint8_t c3 = nCubicRoot[index3];
		pRes[k] = c1 | ( c2 << 8 ) | ( c3 << 16 );
	}
}

static void ScaleColors(
	uint32_t *pRes, const uint32_t *pSrc, int nSrcStride,
	const unsigned char *pScale, int nScaleMask,
	const uint16_t *pPosIndices, const NGfx::SCompactVector *pTransp, int nCount,
	bool bMultiplyOnTransparency )
{
	NGfx::SMMXWord mTransp;
	mTransp.nX = mTransp.nY = mTransp.nZ = 0; mTransp.nW = 0x1ff;
	uint64_t transparency = mmx::combine64( mTransp.nZ, mTransp.nY, mTransp.nX, mTransp.nW );

	if ( bMultiplyOnTransparency )
	{
		mTransp.nX = mTransp.nY = mTransp.nZ = 0; mTransp.nW = 0;
	}
	else
	{
		mTransp.nX = mTransp.nY = mTransp.nZ = 0x7fff; mTransp.nW = 0;
	}
	uint64_t multiplyTransparency = mmx::combine64( mTransp.nZ, mTransp.nY, mTransp.nX, mTransp.nW );

	// A stride of 0 is a real caller: it broadcasts one source colour over every vertex.
	const int nSourceStep = nSrcStride / 4;
	for ( int k = 0; k < nCount; ++k, pSrc += nSourceStep )
	{
		int nScaleIndex = pPosIndices[k] & nScaleMask;
		uint64_t n = ( (int)( pScale[ nScaleIndex ] ) ) << 2;
		uint64_t nScale = pTransp[k].w << 7;

		uint64_t src = mmx::punpcklbw( *pSrc, *pSrc );
		src = mmx::psrlw( src, 1 );
		n = mmx::punpcklwd( n, n );
		n = mmx::punpckldq( n, n );
		src = mmx::pmulhw( src, n );
		src = mmx::por( src, transparency );
		nScale = mmx::punpcklwd( nScale, nScale );
		nScale = mmx::punpckldq( nScale, nScale );
		nScale = mmx::por( nScale, multiplyTransparency );
		src = mmx::pmulhw( src, nScale );
		src = mmx::packuswb( src, src );
		pRes[k] = src & 0xFFFFFFFFUL;
	}
}

}

const SLightingKernels refLightingKernels =
{
	"ref",
	&CalcDirectionalLighting,
	&SampleWarFogCoords,
	&SampleWarFogInt,
	&CalcPointLightAttenuation,
	&CalcPointLightColorsIndexed,
	&CalcPointLightColorsUniform,
	&AddColors,
	&ScaleColors,
};

}
