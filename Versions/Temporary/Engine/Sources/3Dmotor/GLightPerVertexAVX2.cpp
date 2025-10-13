// AVX2 implementation of the per-vertex lighting kernels.
//
// Each 256-bit register holds four of the original 64-bit MMX operands, so most
// kernels here process four vertices per iteration.
//
// This translation unit is compiled with /arch:AVX2 (see 3Dmotor/CMakeLists.txt) and
// must only ever be entered through avx2LightingKernels, which GetLightingKernels()
// hands out only when CPUID and XGETBV both agree AVX2 is usable. Keep the include
// list minimal and everything but the kernel table in the anonymous namespace: any
// inline function with external linkage emitted here is an AVX2-compiled COMDAT that
// the linker may pick over the baseline copy, which would then run AVX2 instructions
// on a CPU that has none. That is also why the kernels take raw pointers and never
// touch std::vector or CArray2D.

#include "stdafx.h"
#include "GLightPerVertexKernels.h"

#include <immintrin.h>

#include <cstdint>

namespace NGScene
{
namespace
{

static inline __m128i LoadMMXWord64AVX2( const NGfx::SMMXWord &word )
{
	return _mm_loadl_epi64( reinterpret_cast<const __m128i*>( &word ) );
}

static inline __m256i LoadMMXWord256( const NGfx::SMMXWord &word )
{
	const __m128i value = LoadMMXWord64AVX2( word );
	const __m128i pair = _mm_unpacklo_epi64( value, value );
	return _mm256_broadcastsi128_si256( pair );
}

static inline __m256i LoadMMXWordsAVX2(
	const NGfx::SMMXWord &word0, const NGfx::SMMXWord &word1,
	const NGfx::SMMXWord &word2, const NGfx::SMMXWord &word3 )
{
	const __m128i low = _mm_unpacklo_epi64(
		LoadMMXWord64AVX2( word0 ), LoadMMXWord64AVX2( word1 ) );
	const __m128i high = _mm_unpacklo_epi64(
		LoadMMXWord64AVX2( word2 ), LoadMMXWord64AVX2( word3 ) );
	return _mm256_set_m128i( high, low );
}

static inline __m256i ExpandPackedBytesAVX2( __m128i packedBytes )
{
	// punpcklbw(value, value) duplicated each byte into both halves of a word.
	__m256i words = _mm256_cvtepu8_epi16( packedBytes );
	return _mm256_or_si256( words, _mm256_slli_epi16( words, 8 ) );
}

static inline __m128i PackLow64HalvesAVX2( __m256i values )
{
	return _mm_unpacklo_epi64(
		_mm256_castsi256_si128( values ),
		_mm256_extracti128_si256( values, 1 ) );
}

static inline void StoreMMXWordsAVX2( __m256i values, NGfx::SMMXWord *destination, int count )
{
	if ( count == 4 )
	{
		_mm256_storeu_si256( reinterpret_cast<__m256i*>( destination ), values );
		return;
	}

	const __m128i low = _mm256_castsi256_si128( values );
	if ( count >= 2 )
		_mm_storeu_si128( reinterpret_cast<__m128i*>( destination ), low );
	else
		_mm_storel_epi64( reinterpret_cast<__m128i*>( destination ), low );

	if ( count == 3 )
	{
		const __m128i high = _mm256_extracti128_si256( values, 1 );
		_mm_storel_epi64( reinterpret_cast<__m128i*>( destination + 2 ), high );
	}
}

struct SDirectionalLightingAVX2Data
{
	__m256i shift;
	__m256i dirLight;
	__m256i ambient;
	__m256i lightColor;
	__m256i incidentShadowColor;
	__m256i shadeColor;
	__m256i translucentShade;
	__m256i allBits;
};

static inline void CalcDirectionalLightingAVX2x4(
	__m128i packedNormals, const SDirectionalLightingAVX2Data &data,
	__m128i *packedColors, __m128i *packedShadowColors )
{
	// Four independent old MMX calculations occupy the four 64-bit quarters.
	const __m256i normals = ExpandPackedBytesAVX2( packedNormals );
	__m256i shiftedNormals = _mm256_sub_epi16( normals, data.shift );
	shiftedNormals = _mm256_madd_epi16( shiftedNormals, data.dirLight );
	shiftedNormals = _mm256_add_epi32(
		shiftedNormals, _mm256_srli_epi64( shiftedNormals, 32 ) );
	shiftedNormals = _mm256_srai_epi32( shiftedNormals, 15 );
	shiftedNormals = _mm256_shufflelo_epi16(
		shiftedNormals, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	shiftedNormals = _mm256_shufflehi_epi16(
		shiftedNormals, _MM_SHUFFLE( 0, 0, 0, 0 ) );

	const __m256i sign = _mm256_srai_epi16( shiftedNormals, 16 );
	__m256i negativeF = _mm256_and_si256( shiftedNormals, sign );
	const __m256i f = _mm256_andnot_si256( sign, shiftedNormals );
	negativeF = _mm256_xor_si256( negativeF, data.allBits );

	__m256i colors = data.ambient;
	__m256i shadowColors = colors;
	const __m256i lightColor = _mm256_mulhi_epi16( data.lightColor, f );
	const __m256i incidentShadowColor = _mm256_mulhi_epi16( data.incidentShadowColor, f );
	const __m256i translucentShade = _mm256_mulhi_epi16( data.translucentShade, negativeF );
	const __m256i shadeColor = _mm256_mulhi_epi16( data.shadeColor, negativeF );

	colors = _mm256_add_epi16( colors, lightColor );
	shadowColors = _mm256_add_epi16( shadowColors, incidentShadowColor );
	colors = _mm256_add_epi16( colors, translucentShade );
	shadowColors = _mm256_add_epi16( shadowColors, shadeColor );
	colors = _mm256_srai_epi16( colors, 4 );
	shadowColors = _mm256_srai_epi16( shadowColors, 4 );

	*packedColors = PackLow64HalvesAVX2( _mm256_packus_epi16( colors, colors ) );
	*packedShadowColors = PackLow64HalvesAVX2(
		_mm256_packus_epi16( shadowColors, shadowColors ) );
}

static void CalcDirectionalLighting(
	const NGfx::SCompactVector *pNormals, int nCount,
	const SPerVertexLightState &ls, const NGfx::SMMXWord &translucentShade,
	uint32_t *pResColors, uint32_t *pResShadow )
{
	uint32_t dwColor = 0, dwShadowColor = 0, dwPrevNormal = 0;

	SDirectionalLightingAVX2Data data;
	data.shift = LoadMMXWord256( ls.shift );
	data.dirLight = LoadMMXWord256( ls.dirLight );
	data.ambient = LoadMMXWord256( ls.ambient );
	data.lightColor = LoadMMXWord256( ls.lightColor );
	data.incidentShadowColor = LoadMMXWord256( ls.incidentShadowColor );
	data.shadeColor = LoadMMXWord256( ls.shadeColor );
	data.translucentShade = LoadMMXWord256( translucentShade );
	data.allBits = _mm256_cmpeq_epi16( data.shift, data.shift );

	for ( int k = 0; k < nCount; ++k )
	{
		const uint32_t normal0 = pNormals[k].dw;
		if ( normal0 != dwPrevNormal )
		{
			if ( k + 3 < nCount )
			{
				const uint32_t normal1 = pNormals[k + 1].dw;
				const uint32_t normal2 = pNormals[k + 2].dw;
				const uint32_t normal3 = pNormals[k + 3].dw;
				if ( normal1 != normal0 && normal2 != normal1 && normal3 != normal2 )
				{
					const __m128i packedNormals = _mm_set_epi32(
						static_cast<int>( normal3 ), static_cast<int>( normal2 ),
						static_cast<int>( normal1 ), static_cast<int>( normal0 ) );
					__m128i colors, shadowColors;
					CalcDirectionalLightingAVX2x4(
						packedNormals, data, &colors, &shadowColors );
					_mm_storeu_si128(
						reinterpret_cast<__m128i*>( &pResColors[k] ), colors );
					_mm_storeu_si128(
						reinterpret_cast<__m128i*>( &pResShadow[k] ), shadowColors );

					dwColor = static_cast<uint32_t>( _mm_extract_epi32( colors, 3 ) );
					dwShadowColor = static_cast<uint32_t>( _mm_extract_epi32( shadowColors, 3 ) );
					dwPrevNormal = normal3;
					k += 3;
					continue;
				}
			}

			__m128i colors, shadowColors;
			CalcDirectionalLightingAVX2x4(
				_mm_cvtsi32_si128( static_cast<int>( normal0 ) ),
				data, &colors, &shadowColors );
			dwColor = static_cast<uint32_t>( _mm_cvtsi128_si32( colors ) );
			dwShadowColor = static_cast<uint32_t>( _mm_cvtsi128_si32( shadowColors ) );
		}

		pResColors[k] = dwColor;
		pResShadow[k] = dwShadowColor;
		dwPrevNormal = normal0;
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
	const __m256d scale = _mm256_set1_pd( static_cast<double>( fpScale ) );
	int k = 0;
	for ( ; k + 3 < nVertices; k += 4 )
	{
		const __m128 positions01 = _mm_set_ps(
			pSrcPos[k + 1].y, pSrcPos[k + 1].x, pSrcPos[k].y, pSrcPos[k].x );
		const __m128 positions23 = _mm_set_ps(
			pSrcPos[k + 3].y, pSrcPos[k + 3].x, pSrcPos[k + 2].y, pSrcPos[k + 2].x );
		const __m128i coords01 = _mm256_cvtpd_epi32(
			_mm256_mul_pd( _mm256_cvtps_pd( positions01 ), scale ) );
		const __m128i coords23 = _mm256_cvtpd_epi32(
			_mm256_mul_pd( _mm256_cvtps_pd( positions23 ), scale ) );
		_mm_storeu_si128( reinterpret_cast<__m128i*>( &pIntCoords[k * 2] ), coords01 );
		_mm_storeu_si128( reinterpret_cast<__m128i*>( &pIntCoords[k * 2 + 4] ), coords23 );
	}

	if ( k < nVertices )
	{
		const int remaining = nVertices - k;
		const CVec3 &position0 = pSrcPos[k];
		const __m128 positions01 = _mm_set_ps(
			remaining > 1 ? pSrcPos[k + 1].y : 0.0f,
			remaining > 1 ? pSrcPos[k + 1].x : 0.0f,
			position0.y, position0.x );
		const __m128 positions23 = _mm_set_ps(
			0.0f, 0.0f,
			remaining > 2 ? pSrcPos[k + 2].y : 0.0f,
			remaining > 2 ? pSrcPos[k + 2].x : 0.0f );
		const __m128i coords01 = _mm256_cvtpd_epi32(
			_mm256_mul_pd( _mm256_cvtps_pd( positions01 ), scale ) );
		const __m128i coords23 = _mm256_cvtpd_epi32(
			_mm256_mul_pd( _mm256_cvtps_pd( positions23 ), scale ) );
		if ( remaining >= 2 )
			_mm_storeu_si128( reinterpret_cast<__m128i*>( &pIntCoords[k * 2] ), coords01 );
		else
			_mm_storel_epi64( reinterpret_cast<__m128i*>( &pIntCoords[k * 2] ), coords01 );
		if ( remaining == 3 )
			_mm_storel_epi64( reinterpret_cast<__m128i*>( &pIntCoords[k * 2 + 4] ), coords23 );
	}
}

struct SWarFogAVX2Sample
{
	uint32_t fogValues;
	int xWeights;
	int yWeights;
};

static inline SWarFogAVX2Sample PrepareWarFogAVX2Sample(
	int nX, int nY, int nMask, const unsigned char *pFog, int nFogSizeX )
{
	const int nYU = ( nY >> 14 ) & nMask;
	const int nYfi = nY & 0x3fff;
	const int nXL = ( nX >> 14 ) & nMask;
	const int nXfi = nX & 0x3fff;
	const unsigned char *pUp = pFog + nYU * nFogSizeX + nXL;
	const unsigned char *pDown = pUp + nMask + 2;

	SWarFogAVX2Sample sample;
	sample.fogValues = static_cast<uint32_t>( pUp[0] ) |
		( static_cast<uint32_t>( pUp[1] ) << 8 ) |
		( static_cast<uint32_t>( pDown[0] ) << 16 ) |
		( static_cast<uint32_t>( pDown[1] ) << 24 );
	sample.xWeights = ( 0x4000 - nXfi ) | ( nXfi << 16 );
	sample.yWeights = ( 0x4000 - nYfi ) | ( nYfi << 16 );
	return sample;
}

static inline uint32_t SampleWarFogIntAVX2x4(
	const SWarFogAVX2Sample &sample0, const SWarFogAVX2Sample &sample1,
	const SWarFogAVX2Sample &sample2, const SWarFogAVX2Sample &sample3 )
{
	const __m128i packedFog = _mm_set_epi32(
		static_cast<int>( sample3.fogValues ), static_cast<int>( sample2.fogValues ),
		static_cast<int>( sample1.fogValues ), static_cast<int>( sample0.fogValues ) );
	const __m256i fogValues = _mm256_cvtepu8_epi16( packedFog );

	const __m256i xWeights = _mm256_set_epi32(
		sample3.xWeights, sample3.xWeights, sample2.xWeights, sample2.xWeights,
		sample1.xWeights, sample1.xWeights, sample0.xWeights, sample0.xWeights );
	__m256i horizontal = _mm256_madd_epi16( fogValues, xWeights );
	horizontal = _mm256_srai_epi32( horizontal, 14 );
	horizontal = _mm256_packs_epi32( horizontal, horizontal );

	const __m256i yWeights = _mm256_set_epi32(
		sample3.yWeights, sample2.yWeights, sample3.yWeights, sample2.yWeights,
		sample1.yWeights, sample0.yWeights, sample1.yWeights, sample0.yWeights );
	__m256i vertical = _mm256_madd_epi16( horizontal, yWeights );
	vertical = _mm256_srai_epi32( vertical, 14 );
	vertical = _mm256_packs_epi32( vertical, vertical );
	vertical = _mm256_packus_epi16( vertical, vertical );

	const __m128i low = _mm256_castsi256_si128( vertical );
	const __m128i high = _mm256_extracti128_si256( vertical, 1 );
	return static_cast<uint32_t>( _mm_extract_epi16( low, 0 ) ) |
		( static_cast<uint32_t>( _mm_extract_epi16( high, 0 ) ) << 16 );
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
	for ( ; k + 3 < nVertices; k += 4 )
	{
		const SWarFogAVX2Sample sample0 = PrepareWarFogAVX2Sample(
			pIntCoords[k * 2], pIntCoords[k * 2 + 1], nMask, pFog, nFogSizeX );
		const SWarFogAVX2Sample sample1 = PrepareWarFogAVX2Sample(
			pIntCoords[k * 2 + 2], pIntCoords[k * 2 + 3], nMask, pFog, nFogSizeX );
		const SWarFogAVX2Sample sample2 = PrepareWarFogAVX2Sample(
			pIntCoords[k * 2 + 4], pIntCoords[k * 2 + 5], nMask, pFog, nFogSizeX );
		const SWarFogAVX2Sample sample3 = PrepareWarFogAVX2Sample(
			pIntCoords[k * 2 + 6], pIntCoords[k * 2 + 7], nMask, pFog, nFogSizeX );
		const uint32_t result = SampleWarFogIntAVX2x4( sample0, sample1, sample2, sample3 );
		pRes[k] = static_cast<unsigned char>( result );
		pRes[k + 1] = static_cast<unsigned char>( result >> 8 );
		pRes[k + 2] = static_cast<unsigned char>( result >> 16 );
		pRes[k + 3] = static_cast<unsigned char>( result >> 24 );
	}

	if ( k < nVertices )
	{
		SWarFogAVX2Sample samples[4]{};
		const int remaining = nVertices - k;
		for ( int i = 0; i < remaining; ++i )
		{
			samples[i] = PrepareWarFogAVX2Sample(
				pIntCoords[(k + i) * 2], pIntCoords[(k + i) * 2 + 1], nMask, pFog, nFogSizeX );
		}
		const uint32_t result = SampleWarFogIntAVX2x4(
			samples[0], samples[1], samples[2], samples[3] );
		for ( int i = 0; i < remaining; ++i )
			pRes[k + i] = static_cast<unsigned char>( result >> (i * 8) );
	}
}

struct SPointLightAttenuationAVX2Data
{
	__m256 center;
	__m128 radiusSquared;
	__m128 attenuationScale;
	__m128 cutMultiplier;
	__m128 attenuationAdd;
	__m128 zero;
};

static inline __m128 CalcPointLightAttenuationFactorAVX2(
	__m128 distanceSquared, const SPointLightAttenuationAVX2Data &data )
{
	// Keep rcp/rsqrt scalar so their approximate bits match the SSE2 kernel exactly.
	__m128 cut = _mm_sub_ss( data.radiusSquared, distanceSquared );
	__m128 denominator = _mm_mul_ss( distanceSquared, data.attenuationScale );
	const __m128 inverseDistance = _mm_rsqrt_ss( distanceSquared );
	cut = _mm_mul_ss( cut, data.cutMultiplier );
	denominator = _mm_add_ss( denominator, data.attenuationAdd );
	cut = _mm_max_ss( cut, data.zero );
	denominator = _mm_rcp_ss( denominator );
	__m128 attenuation = _mm_mul_ss( cut, inverseDistance );
	attenuation = _mm_mul_ss( attenuation, denominator );
	return _mm_shuffle_ps( attenuation, attenuation, _MM_SHUFFLE( 0, 0, 0, 0 ) );
}

static inline __m128i CalcPointLightAttenuationAVX2x2(
	const CVec3 &position0, const CVec3 &position1,
	const SPointLightAttenuationAVX2Data &data )
{
	const __m128 packedPosition0 = _mm_set_ps(
		0.0f, position0.x, position0.y, position0.z );
	const __m128 packedPosition1 = _mm_set_ps(
		0.0f, position1.x, position1.y, position1.z );
	const __m256 packedPositions = _mm256_set_m128( packedPosition1, packedPosition0 );
	__m256 deltas = _mm256_sub_ps( data.center, packedPositions );

	// Each 128-bit lane keeps the SSE2 (z*z + y*y) + x*x accumulation order.
	__m256 distanceSquared = _mm256_mul_ps( deltas, deltas );
	__m256 shuffledSquares = _mm256_shuffle_ps( distanceSquared, distanceSquared, 0xe1 );
	distanceSquared = _mm256_add_ps( distanceSquared, shuffledSquares );
	shuffledSquares = _mm256_shuffle_ps( shuffledSquares, shuffledSquares, 0xe2 );
	distanceSquared = _mm256_add_ps( distanceSquared, shuffledSquares );

	const __m128 attenuation0 = CalcPointLightAttenuationFactorAVX2(
		_mm256_castps256_ps128( distanceSquared ), data );
	const __m128 attenuation1 = CalcPointLightAttenuationFactorAVX2(
		_mm256_extractf128_ps( distanceSquared, 1 ), data );
	const __m256 attenuation = _mm256_set_m128( attenuation1, attenuation0 );
	deltas = _mm256_mul_ps( deltas, attenuation );

	const __m256i converted = _mm256_cvtps_epi32( deltas );
	return PackLow64HalvesAVX2( _mm256_packs_epi32( converted, converted ) );
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

	const __m128 center = _mm_set_ps( 0.0f, vCenter.x, vCenter.y, vCenter.z );
	SPointLightAttenuationAVX2Data data;
	data.center = _mm256_set_m128( center, center );
	data.radiusSquared = _mm_set_ss( fRadius2 );
	data.attenuationScale = _mm_set_ss( fAttScale );
	data.cutMultiplier = _mm_set_ss( fCutMult );
	data.attenuationAdd = _mm_set_ss( fAttAdd );
	data.zero = _mm_setzero_ps();

	int k = 0;
	for ( ; k + 1 < nCount; k += 2 )
	{
		const __m128i result = CalcPointLightAttenuationAVX2x2(
			pSrcPos[k], pSrcPos[k + 1], data );
		_mm_storeu_si128( reinterpret_cast<__m128i*>( &pRes[k] ), result );
	}
	if ( k < nCount )
	{
		// The second lane is discarded; zero keeps it out of the denormal path.
		const CVec3 emptyPosition( 0.0f, 0.0f, 0.0f );
		const __m128i result = CalcPointLightAttenuationAVX2x2(
			pSrcPos[k], emptyPosition, data );
		_mm_storel_epi64( reinterpret_cast<__m128i*>( &pRes[k] ), result );
	}
}

struct SPointLightColorsAVX2Data
{
	__m256i shift;
	__m256i attenuation;
	__m256i lightColor;
	__m256i rounding;
};

static inline __m256i CalcPointLightColorsAVX2x4(
	__m128i packedNormals, __m256i resColors, const SPointLightColorsAVX2Data &data )
{
	// Four independent old MMX calculations occupy the four 64-bit quarters.
	__m256i normals = ExpandPackedBytesAVX2( packedNormals );
	normals = _mm256_sub_epi16( normals, data.shift );
	normals = _mm256_madd_epi16( normals, data.attenuation );
	normals = _mm256_add_epi32( normals, _mm256_srli_epi64( normals, 32 ) );
	normals = _mm256_srai_epi32( normals, 15 );

	// Pack, saturate, and broadcast one dot product within each 64-bit quarter.
	normals = _mm256_packs_epi32( normals, normals );
	normals = _mm256_shufflelo_epi16( normals, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	normals = _mm256_shufflehi_epi16( normals, _MM_SHUFFLE( 2, 2, 2, 2 ) );
	normals = _mm256_and_si256(
		normals, _mm256_cmpgt_epi16( normals, _mm256_setzero_si256() ) );

	const __m256i productsLow = _mm256_mullo_epi16( normals, data.lightColor );
	const __m256i productsHigh = _mm256_mulhi_epi16( normals, data.lightColor );
	__m256i products02 = _mm256_unpacklo_epi16( productsLow, productsHigh );
	__m256i products13 = _mm256_unpackhi_epi16( productsLow, productsHigh );
	products02 = _mm256_srai_epi32(
		_mm256_add_epi32( products02, data.rounding ), 13 );
	products13 = _mm256_srai_epi32(
		_mm256_add_epi32( products13, data.rounding ), 13 );

	const __m256i lightContribution = _mm256_packs_epi32( products02, products13 );
	return _mm256_adds_epi16( resColors, lightContribution );
}

static void CalcPointLightColorsIndexed(
	NGfx::SMMXWord *pRes, const NGfx::SMMXWord *pAttenuation,
	const uint16_t *pPosIndices, const NGfx::SCompactVector *pNormals, int nCount,
	const NGfx::SMMXWord &lightColorWord )
{
	NGfx::SMMXWord shift{};
	shift.nX = shift.nY = shift.nZ = (short)0x8000;

	SPointLightColorsAVX2Data data;
	data.shift = LoadMMXWord256( shift );
	data.lightColor = LoadMMXWord256( lightColorWord );
	data.rounding = _mm256_set1_epi32( 1 << 12 );

	int k = 0;
	for ( ; k + 3 < nCount; k += 4 )
	{
		const __m128i packedNormals = _mm_loadu_si128(
			reinterpret_cast<const __m128i*>( &pNormals[k] ) );
		data.attenuation = LoadMMXWordsAVX2(
			pAttenuation[pPosIndices[k]], pAttenuation[pPosIndices[k + 1]],
			pAttenuation[pPosIndices[k + 2]], pAttenuation[pPosIndices[k + 3]] );
		const __m256i resColors = _mm256_loadu_si256(
			reinterpret_cast<const __m256i*>( &pRes[k] ) );
		const __m256i result = CalcPointLightColorsAVX2x4(
			packedNormals, resColors, data );
		_mm256_storeu_si256( reinterpret_cast<__m256i*>( &pRes[k] ), result );
	}

	if ( k < nCount )
	{
		const int remaining = nCount - k;
		const NGfx::SMMXWord zero{};
		const NGfx::SMMXWord &attenuation1 = remaining > 1 ? pAttenuation[pPosIndices[k + 1]] : zero;
		const NGfx::SMMXWord &attenuation2 = remaining > 2 ? pAttenuation[pPosIndices[k + 2]] : zero;
		data.attenuation = LoadMMXWordsAVX2(
			pAttenuation[pPosIndices[k]], attenuation1, attenuation2, zero );
		const __m128i packedNormals = _mm_set_epi32(
			0,
			remaining > 2 ? static_cast<int>( pNormals[k + 2].dw ) : 0,
			remaining > 1 ? static_cast<int>( pNormals[k + 1].dw ) : 0,
			static_cast<int>( pNormals[k].dw ) );

		const NGfx::SMMXWord &resColor1 = remaining > 1 ? pRes[k + 1] : zero;
		const NGfx::SMMXWord &resColor2 = remaining > 2 ? pRes[k + 2] : zero;
		const __m256i resColors = LoadMMXWordsAVX2(
			pRes[k], resColor1, resColor2, zero );
		const __m256i result = CalcPointLightColorsAVX2x4(
			packedNormals, resColors, data );
		StoreMMXWordsAVX2( result, &pRes[k], remaining );
	}
}

static void CalcPointLightColorsUniform(
	NGfx::SMMXWord *pRes, const NGfx::SMMXWord &attenuation,
	const NGfx::SCompactVector *pNormals, int nNormalStride, int nCount,
	const NGfx::SMMXWord &lightColorWord )
{
	NGfx::SMMXWord shift{};
	shift.nX = shift.nY = shift.nZ = (short)0x8000;

	SPointLightColorsAVX2Data data;
	data.shift = LoadMMXWord256( shift );
	data.attenuation = LoadMMXWord256( attenuation );
	data.lightColor = LoadMMXWord256( lightColorWord );
	data.rounding = _mm256_set1_epi32( 1 << 12 );

	auto normalAt = [pNormals, nNormalStride]( int k ) -> uint32_t
	{
		const unsigned char *p = reinterpret_cast<const unsigned char *>( pNormals ) + k * nNormalStride;
		return reinterpret_cast<const NGfx::SCompactVector *>( p )->dw;
	};

	int k = 0;
	for ( ; k + 3 < nCount; k += 4 )
	{
		const __m128i packedNormals = _mm_set_epi32(
			static_cast<int>( normalAt( k + 3 ) ),
			static_cast<int>( normalAt( k + 2 ) ),
			static_cast<int>( normalAt( k + 1 ) ),
			static_cast<int>( normalAt( k ) ) );
		const __m256i resColors = _mm256_loadu_si256(
			reinterpret_cast<const __m256i*>( &pRes[k] ) );
		const __m256i result = CalcPointLightColorsAVX2x4(
			packedNormals, resColors, data );
		_mm256_storeu_si256( reinterpret_cast<__m256i*>( &pRes[k] ), result );
	}

	if ( k < nCount )
	{
		const int remaining = nCount - k;
		const NGfx::SMMXWord zero{};
		const __m128i packedNormals = _mm_set_epi32(
			0,
			remaining > 2 ? static_cast<int>( normalAt( k + 2 ) ) : 0,
			remaining > 1 ? static_cast<int>( normalAt( k + 1 ) ) : 0,
			static_cast<int>( normalAt( k ) ) );
		const NGfx::SMMXWord &resColor1 = remaining > 1 ? pRes[k + 1] : zero;
		const NGfx::SMMXWord &resColor2 = remaining > 2 ? pRes[k + 2] : zero;
		const __m256i resColors = LoadMMXWordsAVX2(
			pRes[k], resColor1, resColor2, zero );
		const __m256i result = CalcPointLightColorsAVX2x4(
			packedNormals, resColors, data );
		StoreMMXWordsAVX2( result, &pRes[k], remaining );
	}
}

static inline __m256i CalcAddColorIndicesAVX2(
	__m128i packedColors, __m256i addColors )
{
	// Four independent old MMX calculations occupy the four 64-bit quarters.
	__m256i colors = ExpandPackedBytesAVX2( packedColors );
	colors = _mm256_srli_epi16( colors, 1 );

	__m256i colorSquares = _mm256_mulhi_epi16( colors, colors );
	colorSquares = _mm256_slli_epi16( colorSquares, 1 );

	__m256i colorCubesHigh = _mm256_mulhi_epi16( colorSquares, colors );
	__m256i colorCubesLow = _mm256_mullo_epi16( colorSquares, colors );
	colorCubesHigh = _mm256_slli_epi16( colorCubesHigh, 1 );
	colorCubesHigh = _mm256_adds_epi16( colorCubesHigh, addColors );
	colorCubesHigh = _mm256_srli_epi16( colorCubesHigh, 1 );

	colorCubesLow = _mm256_srli_epi16( colorCubesLow, 2 );
	colorCubesLow = _mm256_or_si256(
		colorCubesLow, _mm256_set1_epi16( 0x4000 ) );
	const __m256i highIsZero = _mm256_cmpeq_epi16(
		colorCubesHigh, _mm256_setzero_si256() );
	return _mm256_or_si256(
		_mm256_and_si256( colorCubesLow, highIsZero ),
		_mm256_andnot_si256( highIsZero, colorCubesHigh ) );
}

static inline uint32_t LookupCubicRootColorAVX2( __m128i indices )
{
	const int nZ = _mm_extract_epi16( indices, 0 ) & 0x7fff;
	const int nY = _mm_extract_epi16( indices, 1 ) & 0x7fff;
	const int nX = _mm_extract_epi16( indices, 2 ) & 0x7fff;
	return static_cast<uint32_t>( nCubicRoot[nZ] ) |
		( static_cast<uint32_t>( nCubicRoot[nY] ) << 8 ) |
		( static_cast<uint32_t>( nCubicRoot[nX] ) << 16 );
}

static void AddColors( uint32_t *pRes, const uint32_t *pSrc, const NGfx::SMMXWord *pAdd, int nCount )
{
	if ( nCount == 0 )
		return;

	int k = 0;
	for ( ; k + 3 < nCount; k += 4 )
	{
		const __m128i packedColors = _mm_loadu_si128(
			reinterpret_cast<const __m128i*>( pSrc + k ) );
		const __m256i addColors = _mm256_loadu_si256(
			reinterpret_cast<const __m256i*>( pAdd + k ) );
		const __m256i indices = CalcAddColorIndicesAVX2( packedColors, addColors );
		const __m128i low = _mm256_castsi256_si128( indices );
		const __m128i high = _mm256_extracti128_si256( indices, 1 );
		pRes[k] = LookupCubicRootColorAVX2( low );
		pRes[k + 1] = LookupCubicRootColorAVX2( _mm_srli_si128( low, 8 ) );
		pRes[k + 2] = LookupCubicRootColorAVX2( high );
		pRes[k + 3] = LookupCubicRootColorAVX2( _mm_srli_si128( high, 8 ) );
	}

	if ( k < nCount )
	{
		const int remaining = nCount - k;
		const NGfx::SMMXWord zero{};
		const __m128i packedColors = _mm_set_epi32(
			0,
			remaining > 2 ? static_cast<int>( pSrc[k + 2] ) : 0,
			remaining > 1 ? static_cast<int>( pSrc[k + 1] ) : 0,
			static_cast<int>( pSrc[k] ) );
		const NGfx::SMMXWord &add1 = remaining > 1 ? pAdd[k + 1] : zero;
		const NGfx::SMMXWord &add2 = remaining > 2 ? pAdd[k + 2] : zero;
		const __m256i addColors = LoadMMXWordsAVX2(
			pAdd[k], add1, add2, zero );
		const __m256i indices = CalcAddColorIndicesAVX2( packedColors, addColors );
		const __m128i low = _mm256_castsi256_si128( indices );
		const __m128i high = _mm256_extracti128_si256( indices, 1 );
		pRes[k] = LookupCubicRootColorAVX2( low );
		if ( remaining > 1 )
			pRes[k + 1] = LookupCubicRootColorAVX2( _mm_srli_si128( low, 8 ) );
		if ( remaining > 2 )
			pRes[k + 2] = LookupCubicRootColorAVX2( high );
	}
}

struct SScaleColorsAVX2Data
{
	__m256i alpha;
	__m256i transparencyMask;
};

static inline __m128i ScaleColorsAVX2x4(
	__m128i packedColors,
	int scale0, int scale1, int scale2, int scale3,
	int transparencyScale0, int transparencyScale1,
	int transparencyScale2, int transparencyScale3,
	const SScaleColorsAVX2Data &data )
{
	// Four independent old MMX calculations occupy the four 64-bit quarters.
	__m256i colors = ExpandPackedBytesAVX2( packedColors );
	colors = _mm256_srli_epi16( colors, 1 );

	const __m256i scales = _mm256_set_epi16(
		scale3, scale3, scale3, scale3,
		scale2, scale2, scale2, scale2,
		scale1, scale1, scale1, scale1,
		scale0, scale0, scale0, scale0 );
	colors = _mm256_mulhi_epi16( colors, scales );
	colors = _mm256_or_si256( colors, data.alpha );

	__m256i transparencyScales = _mm256_set_epi16(
		transparencyScale3, transparencyScale3, transparencyScale3, transparencyScale3,
		transparencyScale2, transparencyScale2, transparencyScale2, transparencyScale2,
		transparencyScale1, transparencyScale1, transparencyScale1, transparencyScale1,
		transparencyScale0, transparencyScale0, transparencyScale0, transparencyScale0 );
	transparencyScales = _mm256_or_si256(
		transparencyScales, data.transparencyMask );
	colors = _mm256_mulhi_epi16( colors, transparencyScales );
	return PackLow64HalvesAVX2( _mm256_packus_epi16( colors, colors ) );
}

static void ScaleColors(
	uint32_t *pRes, const uint32_t *pSrc, int nSrcStride,
	const unsigned char *pScale, int nScaleMask,
	const uint16_t *pPosIndices, const NGfx::SCompactVector *pTransp, int nCount,
	bool bMultiplyOnTransparency )
{
	if ( nCount == 0 )
		return;

	NGfx::SMMXWord alpha{};
	alpha.nW = 0x1ff;
	NGfx::SMMXWord transparencyMask{};
	if ( !bMultiplyOnTransparency )
		transparencyMask.nX = transparencyMask.nY = transparencyMask.nZ = 0x7fff;

	SScaleColorsAVX2Data data;
	data.alpha = LoadMMXWord256( alpha );
	data.transparencyMask = LoadMMXWord256( transparencyMask );

	// A stride of 0 is a real caller: it broadcasts one source colour over every vertex.
	const int nSourceStep = nSrcStride / 4;
	int k = 0;
	for ( ; k + 3 < nCount; k += 4, pSrc += nSourceStep * 4 )
	{
		const __m128i packedColors = _mm_set_epi32(
			static_cast<int>( pSrc[nSourceStep * 3] ),
			static_cast<int>( pSrc[nSourceStep * 2] ),
			static_cast<int>( pSrc[nSourceStep] ),
			static_cast<int>( pSrc[0] ) );
		const int scale0 = static_cast<int>( pScale[pPosIndices[k] & nScaleMask] ) << 2;
		const int scale1 = static_cast<int>( pScale[pPosIndices[k + 1] & nScaleMask] ) << 2;
		const int scale2 = static_cast<int>( pScale[pPosIndices[k + 2] & nScaleMask] ) << 2;
		const int scale3 = static_cast<int>( pScale[pPosIndices[k + 3] & nScaleMask] ) << 2;
		const __m128i result = ScaleColorsAVX2x4(
			packedColors, scale0, scale1, scale2, scale3,
			static_cast<int>( pTransp[k].w ) << 7,
			static_cast<int>( pTransp[k + 1].w ) << 7,
			static_cast<int>( pTransp[k + 2].w ) << 7,
			static_cast<int>( pTransp[k + 3].w ) << 7, data );
		_mm_storeu_si128( reinterpret_cast<__m128i*>( &pRes[k] ), result );
	}

	if ( k < nCount )
	{
		const int remaining = nCount - k;
		const __m128i packedColors = _mm_set_epi32(
			0,
			remaining > 2 ? static_cast<int>( pSrc[nSourceStep * 2] ) : 0,
			remaining > 1 ? static_cast<int>( pSrc[nSourceStep] ) : 0,
			static_cast<int>( pSrc[0] ) );
		const int scale0 = static_cast<int>( pScale[pPosIndices[k] & nScaleMask] ) << 2;
		const int scale1 = remaining > 1
			? static_cast<int>( pScale[pPosIndices[k + 1] & nScaleMask] ) << 2 : 0;
		const int scale2 = remaining > 2
			? static_cast<int>( pScale[pPosIndices[k + 2] & nScaleMask] ) << 2 : 0;
		const __m128i result = ScaleColorsAVX2x4(
			packedColors, scale0, scale1, scale2, 0,
			static_cast<int>( pTransp[k].w ) << 7,
			remaining > 1 ? static_cast<int>( pTransp[k + 1].w ) << 7 : 0,
			remaining > 2 ? static_cast<int>( pTransp[k + 2].w ) << 7 : 0,
			0, data );
		pRes[k] = static_cast<uint32_t>( _mm_cvtsi128_si32( result ) );
		if ( remaining > 1 )
			pRes[k + 1] = static_cast<uint32_t>( _mm_extract_epi32( result, 1 ) );
		if ( remaining > 2 )
			pRes[k + 2] = static_cast<uint32_t>( _mm_extract_epi32( result, 2 ) );
	}
}

}

const SLightingKernels avx2LightingKernels =
{
	"avx2",
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
