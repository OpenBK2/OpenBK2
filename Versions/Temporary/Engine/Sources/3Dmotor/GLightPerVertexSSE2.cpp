// SSE2 implementation of the per-vertex lighting kernels.
//
// Each 128-bit register holds two of the original 64-bit MMX operands, so most
// kernels here process two vertices per iteration and the odd tail falls back to
// the low half. SSE2 intrinsics compile regardless of /arch:, so this translation
// unit needs no special compile options; GetLightingKernels() decides at runtime
// whether the CPU can actually run it.

#include "stdafx.h"
#include "GLightPerVertexKernels.h"

#include <emmintrin.h>

#include <algorithm>

namespace NGScene
{
namespace
{

static inline __m128i LoadMMXWord64( const NGfx::SMMXWord &w )
{
	// SMMXWord is laid out exactly like the old 64-bit MMX operand: z, y, x, w.
	return _mm_loadl_epi64( reinterpret_cast<const __m128i*>( &w ) );
}

static inline __m128i LoadMMXWord128( const NGfx::SMMXWord &w )
{
	const __m128i value = LoadMMXWord64( w );
	return _mm_unpacklo_epi64( value, value );
}

struct SDirectionalLightingSSE2Data
{
	__m128i shift;
	__m128i dirLight;
	__m128i ambient;
	__m128i lightColor;
	__m128i incidentShadowColor;
	__m128i shadeColor;
	__m128i translucentShade;
	__m128i allBits;
};

static inline void CalcDirectionalLightingSSE2x1(
	DWORD dwNormal, const SDirectionalLightingSSE2Data &data, DWORD *pColor, DWORD *pShadowColor )
{
	__m128i normal = _mm_cvtsi32_si128( static_cast<int>( dwNormal ) );
	normal = _mm_unpacklo_epi8( normal, normal );

	__m128i shiftedNormal = _mm_sub_epi16( normal, data.shift );
	shiftedNormal = _mm_madd_epi16( shiftedNormal, data.dirLight );
	__m128i normalHigh = _mm_srli_epi64( shiftedNormal, 32 );
	shiftedNormal = _mm_add_epi32( shiftedNormal, normalHigh );
	shiftedNormal = _mm_srai_epi32( shiftedNormal, 15 );
	shiftedNormal = _mm_unpacklo_epi16( shiftedNormal, shiftedNormal );
	shiftedNormal = _mm_unpacklo_epi32( shiftedNormal, shiftedNormal );

	__m128i sign = _mm_srai_epi16( shiftedNormal, 16 );
	__m128i negativeF = _mm_and_si128( shiftedNormal, sign );
	__m128i f = _mm_andnot_si128( sign, shiftedNormal );
	negativeF = _mm_xor_si128( negativeF, data.allBits );

	__m128i vRes = data.ambient;
	__m128i vResShadow = vRes;
	__m128i lightColor = _mm_mulhi_epi16( data.lightColor, f );
	__m128i incidentShadowColor = _mm_mulhi_epi16( data.incidentShadowColor, f );
	__m128i translucentShade = _mm_mulhi_epi16( data.translucentShade, negativeF );
	__m128i shadeColor = _mm_mulhi_epi16( data.shadeColor, negativeF );

	vRes = _mm_add_epi16( vRes, lightColor );
	vResShadow = _mm_add_epi16( vResShadow, incidentShadowColor );
	vRes = _mm_add_epi16( vRes, translucentShade );
	vResShadow = _mm_add_epi16( vResShadow, shadeColor );
	vRes = _mm_srai_epi16( vRes, 4 );
	vResShadow = _mm_srai_epi16( vResShadow, 4 );

	*pColor = static_cast<DWORD>( _mm_cvtsi128_si32( _mm_packus_epi16( vRes, vRes ) ) );
	*pShadowColor = static_cast<DWORD>( _mm_cvtsi128_si32( _mm_packus_epi16( vResShadow, vResShadow ) ) );
}

static inline void CalcDirectionalLightingSSE2x2(
	DWORD dwNormal0, DWORD dwNormal1, const SDirectionalLightingSSE2Data &data,
	DWORD *pColor0, DWORD *pShadowColor0, DWORD *pColor1, DWORD *pShadowColor1 )
{
	// Each 64-bit half follows the original MMX code for one normal.
	__m128i normal = _mm_or_si128(
		_mm_cvtsi32_si128( static_cast<int>( dwNormal0 ) ),
		_mm_slli_si128( _mm_cvtsi32_si128( static_cast<int>( dwNormal1 ) ), 4 ) );
	normal = _mm_unpacklo_epi8( normal, normal );

	__m128i shiftedNormal = _mm_sub_epi16( normal, data.shift );
	shiftedNormal = _mm_madd_epi16( shiftedNormal, data.dirLight );
	__m128i normalHigh = _mm_srli_epi64( shiftedNormal, 32 );
	shiftedNormal = _mm_add_epi32( shiftedNormal, normalHigh );
	shiftedNormal = _mm_srai_epi32( shiftedNormal, 15 );
	shiftedNormal = _mm_shufflelo_epi16( shiftedNormal, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	shiftedNormal = _mm_shufflehi_epi16( shiftedNormal, _MM_SHUFFLE( 0, 0, 0, 0 ) );

	__m128i sign = _mm_srai_epi16( shiftedNormal, 16 );
	__m128i negativeF = _mm_and_si128( shiftedNormal, sign );
	__m128i f = _mm_andnot_si128( sign, shiftedNormal );
	negativeF = _mm_xor_si128( negativeF, data.allBits );

	__m128i vRes = data.ambient;
	__m128i vResShadow = vRes;
	__m128i lightColor = _mm_mulhi_epi16( data.lightColor, f );
	__m128i incidentShadowColor = _mm_mulhi_epi16( data.incidentShadowColor, f );
	__m128i translucentShade = _mm_mulhi_epi16( data.translucentShade, negativeF );
	__m128i shadeColor = _mm_mulhi_epi16( data.shadeColor, negativeF );

	vRes = _mm_add_epi16( vRes, lightColor );
	vResShadow = _mm_add_epi16( vResShadow, incidentShadowColor );
	vRes = _mm_add_epi16( vRes, translucentShade );
	vResShadow = _mm_add_epi16( vResShadow, shadeColor );
	vRes = _mm_srai_epi16( vRes, 4 );
	vResShadow = _mm_srai_epi16( vResShadow, 4 );

	__m128i packedColor = _mm_packus_epi16( vRes, vRes );
	__m128i packedShadowColor = _mm_packus_epi16( vResShadow, vResShadow );
	*pColor0 = static_cast<DWORD>( _mm_cvtsi128_si32( packedColor ) );
	*pShadowColor0 = static_cast<DWORD>( _mm_cvtsi128_si32( packedShadowColor ) );
	*pColor1 = static_cast<DWORD>( _mm_cvtsi128_si32( _mm_srli_si128( packedColor, 4 ) ) );
	*pShadowColor1 = static_cast<DWORD>( _mm_cvtsi128_si32( _mm_srli_si128( packedShadowColor, 4 ) ) );
}

static void CalcDirectionalLighting(
	const NGfx::SCompactVector *pNormals, int nCount,
	const SPerVertexLightState &ls, const NGfx::SMMXWord &translucentShade,
	DWORD *pResColors, DWORD *pResShadow )
{
	DWORD dwColor = 0, dwShadowColor = 0, dwPrevNormal = 0;

	SDirectionalLightingSSE2Data lightingData;
	lightingData.shift = LoadMMXWord128( ls.shift );
	lightingData.dirLight = LoadMMXWord128( ls.dirLight );
	lightingData.ambient = LoadMMXWord128( ls.ambient );
	lightingData.lightColor = LoadMMXWord128( ls.lightColor );
	lightingData.incidentShadowColor = LoadMMXWord128( ls.incidentShadowColor );
	lightingData.shadeColor = LoadMMXWord128( ls.shadeColor );
	lightingData.translucentShade = LoadMMXWord128( translucentShade );
	lightingData.allBits = _mm_cmpeq_epi16( lightingData.shift, lightingData.shift );

	for ( int k = 0; k < nCount; ++k )
	{
		DWORD dwNormal = pNormals[k].dw;
		if ( dwNormal != dwPrevNormal )
		{
			if ( k + 1 < nCount )
			{
				DWORD dwNextNormal = pNormals[k + 1].dw;
				if ( dwNextNormal != dwNormal )
				{
					DWORD dwNextColor = 0, dwNextShadowColor = 0;
					CalcDirectionalLightingSSE2x2(
						dwNormal, dwNextNormal, lightingData,
						&dwColor, &dwShadowColor, &dwNextColor, &dwNextShadowColor );

					pResColors[k] = dwColor;
					pResShadow[k] = dwShadowColor;
					pResColors[k + 1] = dwNextColor;
					pResShadow[k + 1] = dwNextShadowColor;

					dwColor = dwNextColor;
					dwShadowColor = dwNextShadowColor;
					dwPrevNormal = dwNextNormal;
					++k;
					continue;
				}
			}

			CalcDirectionalLightingSSE2x1( dwNormal, lightingData, &dwColor, &dwShadowColor );
		}

		pResColors[k] = dwColor;
		pResShadow[k] = dwShadowColor;
		dwPrevNormal = dwNormal;
	}
}

// NOTE: this forms the scaled coordinate in double precision, while the reference
// kernel forms it in float and rounds with lrintf. The two can land on different
// integers at a ULP boundary. Preserved verbatim from the pre-split code so this
// commit stays a pure move; see the bit-exactness test before changing it.
static void SampleWarFogCoords( const CVec3 *pSrcPos, int nVertices, float fScale, int *pIntCoords )
{
	if ( nVertices <= 0 )
		return;

	const float fpScale = fScale * 0x4000;
	const __m128d scale = _mm_set1_pd( static_cast<double>( fpScale ) );
	int k = 0;
	for ( ; k + 1 < nVertices; k += 2 )
	{
		const __m128 positions = _mm_set_ps(
			pSrcPos[k + 1].y, pSrcPos[k + 1].x, pSrcPos[k].y, pSrcPos[k].x );
		const __m128d positions0 = _mm_cvtps_pd( positions );
		const __m128d positions1 = _mm_cvtps_pd( _mm_movehl_ps( positions, positions ) );
		const __m128i coords0 = _mm_cvtpd_epi32( _mm_mul_pd( positions0, scale ) );
		const __m128i coords1 = _mm_cvtpd_epi32( _mm_mul_pd( positions1, scale ) );
		const __m128i coords = _mm_unpacklo_epi64( coords0, coords1 );
		_mm_storeu_si128( reinterpret_cast<__m128i*>( &pIntCoords[k * 2] ), coords );
	}

	if ( k < nVertices )
	{
		const __m128 positions = _mm_set_ps( 0, 0, pSrcPos[k].y, pSrcPos[k].x );
		const __m128d positionsDouble = _mm_cvtps_pd( positions );
		const __m128i coords = _mm_cvtpd_epi32( _mm_mul_pd( positionsDouble, scale ) );
		_mm_storel_epi64( reinterpret_cast<__m128i*>( &pIntCoords[k * 2] ), coords );
	}
}

struct SWarFogSSE2Sample
{
	DWORD fogValues;
	int xWeights;
	int yWeights;
};

static inline SWarFogSSE2Sample PrepareWarFogSSE2Sample(
	int nX, int nY, int nMask, const unsigned char *pFog, int nFogSizeX )
{
	const int nYU = ( nY >> 14 ) & nMask;
	const int nYfi = nY & 0x3fff;
	const int nXL = ( nX >> 14 ) & nMask;
	const int nXfi = nX & 0x3fff;
	const unsigned char *pUp = pFog + nYU * nFogSizeX + nXL;
	const unsigned char *pDown = pUp + nMask + 2;

	SWarFogSSE2Sample sample;
	sample.fogValues = static_cast<DWORD>( pUp[0] ) |
		( static_cast<DWORD>( pUp[1] ) << 8 ) |
		( static_cast<DWORD>( pDown[0] ) << 16 ) |
		( static_cast<DWORD>( pDown[1] ) << 24 );
	sample.xWeights = ( 0x4000 - nXfi ) | ( nXfi << 16 );
	sample.yWeights = ( 0x4000 - nYfi ) | ( nYfi << 16 );
	return sample;
}

static inline WORD SampleWarFogIntSSE2x2( const SWarFogSSE2Sample &sample0, const SWarFogSSE2Sample &sample1 )
{
	const __m128i zero = _mm_setzero_si128();
	__m128i fogValues = _mm_set_epi32(
		0, 0, static_cast<int>( sample1.fogValues ), static_cast<int>( sample0.fogValues ) );
	fogValues = _mm_unpacklo_epi8( fogValues, zero );

	const __m128i xWeights = _mm_set_epi32(
		sample1.xWeights, sample1.xWeights, sample0.xWeights, sample0.xWeights );
	__m128i horizontal = _mm_madd_epi16( fogValues, xWeights );
	horizontal = _mm_srai_epi32( horizontal, 14 );
	horizontal = _mm_packs_epi32( horizontal, horizontal );

	const __m128i yWeights = _mm_set_epi32(
		sample1.yWeights, sample0.yWeights, sample1.yWeights, sample0.yWeights );
	__m128i vertical = _mm_madd_epi16( horizontal, yWeights );
	vertical = _mm_srai_epi32( vertical, 14 );
	vertical = _mm_packs_epi32( vertical, vertical );
	vertical = _mm_packus_epi16( vertical, vertical );
	return static_cast<WORD>( _mm_extract_epi16( vertical, 0 ) );
}

static void SampleWarFogInt(
	const int *pIntCoords, const unsigned char *pFog, int nFogSizeX,
	unsigned char *pRes, int nVertices )
{
	ASSERT( GetNextPow2( nFogSizeX - 1 ) + 1 == nFogSizeX );
	if ( nVertices <= 0 )
		return;

	const int nMask = nFogSizeX - 2;
	int k = 0;
	for ( ; k + 1 < nVertices; k += 2 )
	{
		const SWarFogSSE2Sample sample0 = PrepareWarFogSSE2Sample(
			pIntCoords[k * 2], pIntCoords[k * 2 + 1], nMask, pFog, nFogSizeX );
		const SWarFogSSE2Sample sample1 = PrepareWarFogSSE2Sample(
			pIntCoords[k * 2 + 2], pIntCoords[k * 2 + 3], nMask, pFog, nFogSizeX );
		const WORD result = SampleWarFogIntSSE2x2( sample0, sample1 );
		pRes[k] = static_cast<unsigned char>( result );
		pRes[k + 1] = static_cast<unsigned char>( result >> 8 );
	}

	if ( k < nVertices )
	{
		const SWarFogSSE2Sample sample = PrepareWarFogSSE2Sample(
			pIntCoords[k * 2], pIntCoords[k * 2 + 1], nMask, pFog, nFogSizeX );
		const SWarFogSSE2Sample emptySample{};
		pRes[k] = static_cast<unsigned char>( SampleWarFogIntSSE2x2( sample, emptySample ) );
	}
}

// NOTE: rcp/rsqrt are 12-bit approximations where the reference kernel divides and
// takes an exact square root, and cvtps2dq rounds to nearest where Float2Int
// truncates. Both are deviations from the reference, preserved verbatim from the
// pre-split code; see the bit-exactness test before changing them.
static void CalcPointLightAttenuation(
	NGfx::SMMXWord *pRes, const CVec3 *pSrcPos, int nCount,
	const CVec3 &vCenter, float fRadius )
{
	if ( nCount == 0 )
		return;

	const float fAttScale = F_PL_RADIUS2 / sqr( fRadius );
	const float fRadius2 = sqr( fRadius );
	const float fCutMult = N_PL_ATTENUATION_SCALE / fRadius2;
	const float fAttAdd = F_PL_MIN_DISTANCE_NORMALIZED;

	// Both vectors use the SMMXWord lane order: z, y, x, w.
	const __m128 center = _mm_set_ps( 0.0f, vCenter.x, vCenter.y, vCenter.z );
	const __m128 radiusSquared = _mm_set_ss( fRadius2 );
	const __m128 attenuationScale = _mm_set_ss( fAttScale );
	const __m128 cutMultiplier = _mm_set_ss( fCutMult );
	const __m128 attenuationAdd = _mm_set_ss( fAttAdd );
	const __m128 zero = _mm_setzero_ps();

	for ( int k = 0; k < nCount; ++k )
	{
		const CVec3 &position = pSrcPos[k];
		const __m128 packedPosition = _mm_set_ps( 0.0f, position.x, position.y, position.z );
		__m128 delta = _mm_sub_ps( center, packedPosition );

		// Sum in the same (z*z + y*y) + x*x order as the original assembly.
		__m128 distanceSquared = _mm_mul_ps( delta, delta );
		__m128 shuffledSquares = _mm_shuffle_ps( distanceSquared, distanceSquared, 0xe1 );
		distanceSquared = _mm_add_ss( distanceSquared, shuffledSquares );
		shuffledSquares = _mm_shuffle_ps( shuffledSquares, shuffledSquares, 0xe2 );
		distanceSquared = _mm_add_ss( distanceSquared, shuffledSquares );

		__m128 cut = _mm_sub_ss( radiusSquared, distanceSquared );
		__m128 denominator = _mm_mul_ss( distanceSquared, attenuationScale );
		const __m128 inverseDistance = _mm_rsqrt_ss( distanceSquared );
		cut = _mm_mul_ss( cut, cutMultiplier );
		denominator = _mm_add_ss( denominator, attenuationAdd );
		cut = _mm_max_ss( cut, zero );
		denominator = _mm_rcp_ss( denominator );
		__m128 attenuation = _mm_mul_ss( cut, inverseDistance );
		attenuation = _mm_mul_ss( attenuation, denominator );
		attenuation = _mm_shuffle_ps( attenuation, attenuation, _MM_SHUFFLE( 0, 0, 0, 0 ) );
		delta = _mm_mul_ps( delta, attenuation );

		const __m128i converted = _mm_cvtps_epi32( delta );
		const __m128i packed = _mm_packs_epi32( converted, converted );
		_mm_storel_epi64( reinterpret_cast<__m128i*>( &pRes[k] ), packed );
	}
}

struct SPointLightColorsSSE2Data
{
	__m128i shift;
	__m128i attenuation;
	__m128i lightColor;
	__m128i rounding;
};

static inline __m128i CalcPointLightColorsSSE2x2(
	__m128i packedNormals, __m128i resColors, const SPointLightColorsSSE2Data &data )
{
	// Each 64-bit half follows the original MMX calculation for one vertex.
	__m128i normals = _mm_unpacklo_epi8( packedNormals, packedNormals );
	normals = _mm_sub_epi16( normals, data.shift );
	normals = _mm_madd_epi16( normals, data.attenuation );
	normals = _mm_add_epi32( normals, _mm_srli_epi64( normals, 32 ) );
	normals = _mm_srai_epi32( normals, 15 );

	// Saturate and broadcast the dot product separately within both 64-bit halves.
	normals = _mm_packs_epi32( normals, normals );
	normals = _mm_shufflelo_epi16( normals, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	normals = _mm_shufflehi_epi16( normals, _MM_SHUFFLE( 2, 2, 2, 2 ) );
	normals = _mm_and_si128( normals, _mm_cmpgt_epi16( normals, _mm_setzero_si128() ) );

	const __m128i productsLow = _mm_mullo_epi16( normals, data.lightColor );
	const __m128i productsHigh = _mm_mulhi_epi16( normals, data.lightColor );
	__m128i products0 = _mm_unpacklo_epi16( productsLow, productsHigh );
	__m128i products1 = _mm_unpackhi_epi16( productsLow, productsHigh );
	products0 = _mm_srai_epi32( _mm_add_epi32( products0, data.rounding ), 13 );
	products1 = _mm_srai_epi32( _mm_add_epi32( products1, data.rounding ), 13 );

	const __m128i lightContribution = _mm_packs_epi32( products0, products1 );
	return _mm_adds_epi16( resColors, lightContribution );
}

static void CalcPointLightColorsIndexed(
	NGfx::SMMXWord *pRes, const NGfx::SMMXWord *pAttenuation,
	const WORD *pPosIndices, const NGfx::SCompactVector *pNormals, int nCount,
	const NGfx::SMMXWord &lightColorWord )
{
	NGfx::SMMXWord shift{};
	shift.nX = shift.nY = shift.nZ = (short)0x8000;

	SPointLightColorsSSE2Data data;
	data.shift = LoadMMXWord128( shift );
	data.lightColor = LoadMMXWord128( lightColorWord );
	data.rounding = _mm_set1_epi32( 1 << 12 );

	int k = 0;
	for ( ; k + 1 < nCount; k += 2 )
	{
		const __m128i packedNormals = _mm_loadl_epi64( reinterpret_cast<const __m128i*>( &pNormals[k] ) );
		data.attenuation = _mm_unpacklo_epi64(
			LoadMMXWord64( pAttenuation[ pPosIndices[k] ] ),
			LoadMMXWord64( pAttenuation[ pPosIndices[k + 1] ] ) );
		const __m128i resColors = _mm_loadu_si128( reinterpret_cast<const __m128i*>( &pRes[k] ) );
		const __m128i result = CalcPointLightColorsSSE2x2( packedNormals, resColors, data );
		_mm_storeu_si128( reinterpret_cast<__m128i*>( &pRes[k] ), result );
	}

	if ( k < nCount )
	{
		const __m128i packedNormal = _mm_cvtsi32_si128( static_cast<int>( pNormals[k].dw ) );
		data.attenuation = LoadMMXWord64( pAttenuation[ pPosIndices[k] ] );
		const __m128i resColor = LoadMMXWord64( pRes[k] );
		const __m128i result = CalcPointLightColorsSSE2x2( packedNormal, resColor, data );
		_mm_storel_epi64( reinterpret_cast<__m128i*>( &pRes[k] ), result );
	}
}

static void CalcPointLightColorsUniform(
	NGfx::SMMXWord *pRes, const NGfx::SMMXWord &attenuation,
	const NGfx::SCompactVector *pNormals, int nNormalStride, int nCount,
	const NGfx::SMMXWord &lightColorWord )
{
	NGfx::SMMXWord shift{};
	shift.nX = shift.nY = shift.nZ = (short)0x8000;

	SPointLightColorsSSE2Data data;
	data.shift = LoadMMXWord128( shift );
	data.attenuation = LoadMMXWord128( attenuation );
	data.lightColor = LoadMMXWord128( lightColorWord );
	data.rounding = _mm_set1_epi32( 1 << 12 );

	auto normalAt = [pNormals, nNormalStride]( int k ) -> DWORD
	{
		const unsigned char *p = reinterpret_cast<const unsigned char *>( pNormals ) + k * nNormalStride;
		return reinterpret_cast<const NGfx::SCompactVector *>( p )->dw;
	};

	int k = 0;
	for ( ; k + 1 < nCount; k += 2 )
	{
		const __m128i packedNormals = _mm_or_si128(
			_mm_cvtsi32_si128( static_cast<int>( normalAt( k ) ) ),
			_mm_slli_si128( _mm_cvtsi32_si128( static_cast<int>( normalAt( k + 1 ) ) ), 4 ) );
		const __m128i resColors = _mm_loadu_si128( reinterpret_cast<const __m128i*>( &pRes[k] ) );
		const __m128i result = CalcPointLightColorsSSE2x2( packedNormals, resColors, data );
		_mm_storeu_si128( reinterpret_cast<__m128i*>( &pRes[k] ), result );
	}

	if ( k < nCount )
	{
		const __m128i packedNormal = _mm_cvtsi32_si128( static_cast<int>( normalAt( k ) ) );
		const __m128i resColor = LoadMMXWord64( pRes[k] );
		const __m128i result = CalcPointLightColorsSSE2x2( packedNormal, resColor, data );
		_mm_storel_epi64( reinterpret_cast<__m128i*>( &pRes[k] ), result );
	}
}

static inline __m128i CalcAddColorIndicesSSE2( __m128i packedColors, __m128i addColors )
{
	// Each 64-bit half reproduces the four word lanes of one old MMX calculation.
	__m128i colors = _mm_unpacklo_epi8( packedColors, packedColors );
	colors = _mm_srli_epi16( colors, 1 );

	__m128i colorSquares = _mm_mulhi_epi16( colors, colors );
	colorSquares = _mm_slli_epi16( colorSquares, 1 );

	__m128i colorCubesHigh = _mm_mulhi_epi16( colorSquares, colors );
	__m128i colorCubesLow = _mm_mullo_epi16( colorSquares, colors );
	colorCubesHigh = _mm_slli_epi16( colorCubesHigh, 1 );
	colorCubesHigh = _mm_adds_epi16( colorCubesHigh, addColors );
	colorCubesHigh = _mm_srli_epi16( colorCubesHigh, 1 );

	colorCubesLow = _mm_srli_epi16( colorCubesLow, 2 );
	colorCubesLow = _mm_or_si128( colorCubesLow, _mm_set1_epi16( 0x4000 ) );
	const __m128i highIsZero = _mm_cmpeq_epi16( colorCubesHigh, _mm_setzero_si128() );
	return _mm_or_si128(
		_mm_and_si128( colorCubesLow, highIsZero ),
		_mm_andnot_si128( highIsZero, colorCubesHigh ) );
}

static inline DWORD LookupCubicRootColorSSE2( __m128i indices )
{
	const int nZ = _mm_extract_epi16( indices, 0 ) & 0x7fff;
	const int nY = _mm_extract_epi16( indices, 1 ) & 0x7fff;
	const int nX = _mm_extract_epi16( indices, 2 ) & 0x7fff;
	return static_cast<DWORD>( nCubicRoot[nZ] ) |
		( static_cast<DWORD>( nCubicRoot[nY] ) << 8 ) |
		( static_cast<DWORD>( nCubicRoot[nX] ) << 16 );
}

static void AddColors( DWORD *pRes, const DWORD *pSrc, const NGfx::SMMXWord *pAdd, int nCount )
{
	if ( nCount == 0 )
		return;

	int k = 0;
	for ( ; k + 1 < nCount; k += 2 )
	{
		const __m128i packedColors = _mm_loadl_epi64( reinterpret_cast<const __m128i*>( pSrc + k ) );
		const __m128i addColors = _mm_unpacklo_epi64( LoadMMXWord64( pAdd[k] ), LoadMMXWord64( pAdd[k + 1] ) );
		const __m128i indices = CalcAddColorIndicesSSE2( packedColors, addColors );
		pRes[k] = LookupCubicRootColorSSE2( indices );
		pRes[k + 1] = LookupCubicRootColorSSE2( _mm_srli_si128( indices, 8 ) );
	}

	if ( k < nCount )
	{
		const __m128i packedColor = _mm_cvtsi32_si128( static_cast<int>( pSrc[k] ) );
		const __m128i indices = CalcAddColorIndicesSSE2( packedColor, LoadMMXWord64( pAdd[k] ) );
		pRes[k] = LookupCubicRootColorSSE2( indices );
	}
}

struct SScaleColorsSSE2Data
{
	__m128i alpha;
	__m128i transparencyMask;
};

static inline __m128i ScaleColorsSSE2x2(
	DWORD color0, DWORD color1, int scale0, int scale1,
	int transparencyScale0, int transparencyScale1,
	const SScaleColorsSSE2Data &data )
{
	// Each 64-bit half reproduces the original four-word MMX pipeline.
	__m128i colors = _mm_or_si128(
		_mm_cvtsi32_si128( static_cast<int>( color0 ) ),
		_mm_slli_si128( _mm_cvtsi32_si128( static_cast<int>( color1 ) ), 4 ) );
	colors = _mm_unpacklo_epi8( colors, colors );
	colors = _mm_srli_epi16( colors, 1 );

	const __m128i scales = _mm_set_epi16(
		scale1, scale1, scale1, scale1,
		scale0, scale0, scale0, scale0 );
	colors = _mm_mulhi_epi16( colors, scales );
	colors = _mm_or_si128( colors, data.alpha );

	__m128i transparencyScales = _mm_set_epi16(
		transparencyScale1, transparencyScale1, transparencyScale1, transparencyScale1,
		transparencyScale0, transparencyScale0, transparencyScale0, transparencyScale0 );
	transparencyScales = _mm_or_si128( transparencyScales, data.transparencyMask );
	colors = _mm_mulhi_epi16( colors, transparencyScales );
	return _mm_packus_epi16( colors, colors );
}

static void ScaleColors(
	DWORD *pRes, const DWORD *pSrc, int nSrcStride,
	const unsigned char *pScale, int nScaleMask,
	const WORD *pPosIndices, const NGfx::SCompactVector *pTransp, int nCount,
	bool bMultiplyOnTransparency )
{
	if ( nCount == 0 )
		return;

	NGfx::SMMXWord alpha{};
	alpha.nW = 0x1ff;
	NGfx::SMMXWord transparencyMask{};
	if ( !bMultiplyOnTransparency )
	{
		transparencyMask.nX = transparencyMask.nY = transparencyMask.nZ = 0x7fff;
	}

	SScaleColorsSSE2Data data;
	data.alpha = LoadMMXWord128( alpha );
	data.transparencyMask = LoadMMXWord128( transparencyMask );

	// A stride of 0 is a real caller: it broadcasts one source colour over every vertex.
	const int nSourceStep = nSrcStride / 4;
	int k = 0;
	for ( ; k + 1 < nCount; k += 2, pSrc += nSourceStep * 2 )
	{
		const int scale0 = static_cast<int>( pScale[ pPosIndices[k] & nScaleMask ] ) << 2;
		const int scale1 = static_cast<int>( pScale[ pPosIndices[k + 1] & nScaleMask ] ) << 2;
		const int transparencyScale0 = static_cast<int>( pTransp[k].w ) << 7;
		const int transparencyScale1 = static_cast<int>( pTransp[k + 1].w ) << 7;
		const __m128i result = ScaleColorsSSE2x2(
			pSrc[0], pSrc[nSourceStep], scale0, scale1,
			transparencyScale0, transparencyScale1, data );
		_mm_storel_epi64( reinterpret_cast<__m128i*>( &pRes[k] ), result );
	}

	if ( k < nCount )
	{
		const int scale = static_cast<int>( pScale[ pPosIndices[k] & nScaleMask ] ) << 2;
		const int transparencyScale = static_cast<int>( pTransp[k].w ) << 7;
		const __m128i result = ScaleColorsSSE2x2(
			*pSrc, 0, scale, 0, transparencyScale, 0, data );
		pRes[k] = static_cast<DWORD>( _mm_cvtsi128_si32( result ) );
	}
}

}

const SLightingKernels sse2LightingKernels =
{
	"sse2",
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
