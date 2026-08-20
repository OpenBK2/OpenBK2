#include "stdafx.h"
#include "GLightPerVertex.h"
#include "3DLib/GGeometry.h"
#include "GfxBuffers.h"
//#include "..\Misc\HPTimer.h"
#include <mmintrin.h>
#include "GSSETransform.h"
#include "3DLib/Bound.h"

#include "System/Arch.h"

#if HAS_SSE2
#include <emmintrin.h>
#endif

#include <algorithm>

template<class T>
inline bool operator==( const CArray2D<T> &a, const CArray2D<T> &b )
{
	if ( a.GetSizeX() != b.GetSizeX() || a.GetSizeY() != b.GetSizeY() )
		return false;
	int nTotal = a.GetSizeX() * a.GetSizeY();
	const T *pAData = &a[0][0];
	const T *pBData = &b[0][0];
	for ( int k = 0; k < nTotal; ++k )
	{
		if ( pAData[k] != pBData[k] )
			return false;
	}
	return true;
}

static int nWarFogID;
namespace NGScene
{

// SPerVertexLightState

SPerVertexLightState::SPerVertexLightState() :
	bWarFogUseOnlyNew(false), nWarFogNewID(0), nWarFogOldID(0)
{
}

static void ConvertColor( NGfx::SMMXWord *p, const CVec3 &v )
{
	p->nZ = Float2Int( v.z * 0x4000 );
	p->nY = Float2Int( v.y * 0x4000 );
	p->nX = Float2Int( v.x * 0x4000 );
	p->nW = 0;
}
void SPerVertexLightState::SetDirectional(
	const CVec3 &_vAmbient, const CVec3 &_vLightColor, const CVec3 &_vShadeColor, const CVec3 &_vIncidentShadowColor,
	const CVec3 &_vDir, int _nDirectionalID, const CVec3 &_vDymanicLightsModification )
{
	ConvertColor( &ambient, _vAmbient );
	ConvertColor( &lightColor, _vLightColor );
	ConvertColor( &incidentShadowColor, _vIncidentShadowColor );
	ConvertColor( &shadeColor, _vShadeColor );
	ConvertColor( &dirLight, _vDir );
	shift.nX = shift.nY = shift.nZ = (short)0x8000;
	ambient.nX = ( ambient.nX >> 2 ) + 8;
	ambient.nY = ( ambient.nY >> 2 ) + 8;
	ambient.nZ = ( ambient.nZ >> 2 ) + 8;
	nDirectionalID = _nDirectionalID;
	vAmbientColor = _vAmbient;
	vLightColor = _vLightColor;
	vSunDir = _vDir;
	vDymanicLightsModification = _vDymanicLightsModification;
}
static float Lin( float f ) { return f*f*f; }
static float Outp( float f ) { return exp( log(f) / 3 ); }
static CVec3 GetLinearColor( const CVec3 &a ) { return CVec3( Lin(a.x), Lin(a.y), Lin(a.z) ); }
static CVec4 GetLinearColor( const CVec4 &a ) { return CVec4( Lin(a.x), Lin(a.y), Lin(a.z), Lin(a.w) ); }
static CVec3 GetOutputColor( const CVec3 &a ) { return CVec3( Outp(a.x), Outp(a.y), Outp(a.z) ); }
static CVec4 GetOutputColor( const CVec4 &a ) { return CVec4( Outp(a.x), Outp(a.y), Outp(a.z), Outp(a.w) ); }
void SPerVertexLightState::AddPointLight( const CVec3 &_vCenter, float _fRadius, const CVec3 &_vColor )
{
	CVec3 vLightColor( _vColor.x * vDymanicLightsModification.x, _vColor.y * vDymanicLightsModification.y, _vColor.z * vDymanicLightsModification.z );
	dynamicPointLights.push_back( SPointLightInfo( _vCenter, _fRadius, GetLinearColor(vLightColor), 0 ) );
}
void SPerVertexLightState::AddPointLight( const CVec3 &_vCenter, float _fRadius, const CVec3 &_vColor, int nPointID )
{
	CVec3 vLightColor( _vColor.x * vDymanicLightsModification.x, _vColor.y * vDymanicLightsModification.y, _vColor.z * vDymanicLightsModification.z );
	staticPointLights.push_back( SPointLightInfo( _vCenter, _fRadius, GetLinearColor(vLightColor), nPointID ) );
}
static bool CmpPL( const SPerVertexLightState::SPointLightInfo &a, const SPerVertexLightState::SPointLightInfo &b )
{
	return a.nID < b.nID;
}
void SPerVertexLightState::SortPointLights()
{
	std::sort( staticPointLights.begin(), staticPointLights.end(), CmpPL );
}

void SPerVertexLightState::SetWarFogBlend( float _fBlend )
{
	fWarFogBlend = _fBlend;
}

float SPerVertexLightState::GetWarFogBlend() const 
{ 
	if ( warFogNew.GetSizeX() <= 1 ) 
		return 1; 
	return bWarFogUseOnlyNew ? 1 : fWarFogBlend; 
}

bool SPerVertexLightState::SetWarFog( const CArray2D<unsigned char> &_fog, float _fScale )
{
	bool bRes = false;
	if ( warFogNew == _fog )
		bWarFogUseOnlyNew = true;
	else
	{
		bRes = true;
		bWarFogUseOnlyNew = false;
		warFogOld = warFogNew;
		warFogNew = _fog;

		nWarFogOldID = nWarFogNewID;
		nWarFogNewID = ++nWarFogID;
	}
	if ( warFogOld.GetSizeX() != warFogNew.GetSizeX() )
	{
		bRes = true;
		warFogOld = warFogNew;
		nWarFogOldID = nWarFogNewID;
	}
	bRes |= fWarFogScale != _fScale;
	fWarFogScale = _fScale;
	return bRes;
}

static void MultiplyOnColor( std::vector<DWORD> *pRes, const std::vector<DWORD> &mult )
{
	if ( mult.empty() )
		return;
	DWORD *pDst = &(*pRes)[0], *pDstEnd = pDst + pRes->size();
	const DWORD *pSrc = &mult[0];
	for ( ; pDst < pDstEnd; ++pDst, ++pSrc )
	{
		NGfx::SPixel8888 src = *pSrc;
		NGfx::SPixel8888 dst = *pDst;

		dst.r = std::min<int>(255, (src.r * dst.r) >> 8);
		dst.g = std::min<int>(255, (src.g * dst.g) >> 8);
		dst.b = std::min<int>(255, (src.b * dst.b) >> 8);
		dst.a = std::min<int>(255, (src.a * dst.a) >> 8);

		*pDst = dst.dwColor;
	}
}

// calc colors

static CVec3 MulPerComp( const CVec3 &a, const CVec3 &b ) { return CVec3( a.x * b.x, a.y * b.y, a.z * b.z ); }

#if HAS_SSE2 //SSE2 way of calculating, two lighting infos per register, compared to only one in old MMX
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
	const std::vector<WORD> &posIndices,
	const std::vector<NGfx::SCompactVector> &_normals,
	const SPerVertexLightState &ls, bool bTranslucent, const CVec3 &vTranslucentColor,
	std::vector<DWORD> *pResColors, std::vector<DWORD> *pResShadow )
{
	pResColors->resize( posIndices.size() );
	pResShadow->resize( posIndices.size() );
	DWORD dwColor = 0, dwShadowColor = 0, dwPrevNormal = 0;
	const NGfx::SMMXWord *pTranslucentShade = &ls.shadeColor;
	NGfx::SMMXWord transHolder{};
	if ( bTranslucent )
	{
		ConvertColor( &transHolder, MulPerComp( ls.vLightColor, vTranslucentColor ) );
		pTranslucentShade = &transHolder;
	}

	SDirectionalLightingSSE2Data lightingData;
	lightingData.shift = LoadMMXWord128( ls.shift );
	lightingData.dirLight = LoadMMXWord128( ls.dirLight );
	lightingData.ambient = LoadMMXWord128( ls.ambient );
	lightingData.lightColor = LoadMMXWord128( ls.lightColor );
	lightingData.incidentShadowColor = LoadMMXWord128( ls.incidentShadowColor );
	lightingData.shadeColor = LoadMMXWord128( ls.shadeColor );
	lightingData.translucentShade = LoadMMXWord128( *pTranslucentShade );
	lightingData.allBits = _mm_cmpeq_epi16( lightingData.shift, lightingData.shift );

	const int nSize = static_cast<int>( posIndices.size() );
	for ( int k = 0; k < nSize; ++k )
	{
		DWORD dwNormal = _normals[k].dw;
		if ( dwNormal != dwPrevNormal )
		{
			if ( k + 1 < nSize )
			{
				DWORD dwNextNormal = _normals[k + 1].dw;
				if ( dwNextNormal != dwNormal )
				{
					DWORD dwNextColor = 0, dwNextShadowColor = 0;
					CalcDirectionalLightingSSE2x2(
						dwNormal, dwNextNormal, lightingData,
						&dwColor, &dwShadowColor, &dwNextColor, &dwNextShadowColor );

					(*pResColors)[k] = dwColor;
					(*pResShadow)[k] = dwShadowColor;
					(*pResColors)[k + 1] = dwNextColor;
					(*pResShadow)[k + 1] = dwNextShadowColor;

					dwColor = dwNextColor;
					dwShadowColor = dwNextShadowColor;
					dwPrevNormal = dwNextNormal;
					++k;
					continue;
				}
			}

			CalcDirectionalLightingSSE2x1( dwNormal, lightingData, &dwColor, &dwShadowColor );
		}

		(*pResColors)[k] = dwColor;
		(*pResShadow)[k] = dwShadowColor;
		dwPrevNormal = dwNormal;
	}
}
#else // Classic "scalar" (pray to the compiler to optimize it better) way of computing dir lighting
static void CalcDirectionalLighting(
	const std::vector<WORD> &posIndices,
	const std::vector<NGfx::SCompactVector> &_normals,
	const SPerVertexLightState &ls, bool bTranslucent, const CVec3 &vTranslucentColor,
	std::vector<DWORD> *pResColors, std::vector<DWORD> *pResShadow )
{
	pResColors->resize( posIndices.size() );
	pResShadow->resize( posIndices.size() );
	DWORD dwColor = 0, dwShadowColor = 0, dwPrevNormal = 0;
	const NGfx::SMMXWord *pTranslucentShade = &ls.shadeColor;
	NGfx::SMMXWord transHolder{};
	if ( bTranslucent )
	{
		ConvertColor( &transHolder, MulPerComp( ls.vLightColor, vTranslucentColor ) );
		pTranslucentShade = &transHolder;
	}
	for ( int k = 0; k < posIndices.size(); ++k )
	{
		DWORD dwNormal = _normals[k].dw;
		if ( dwNormal != dwPrevNormal )
		{
			uint64_t normal = mmx::punpcklbw(dwNormal, dwNormal);

			auto combine_mmx_word = [](auto w) { return mmx::combine64(w.nZ, w.nY, w.nX, w.nW); };

			uint64_t shift = combine_mmx_word(ls.shift);
			uint64_t shadeColor = combine_mmx_word(ls.shadeColor);
			uint64_t dirLight = combine_mmx_word(ls.dirLight);
			uint64_t ambient = combine_mmx_word(ls.ambient);
			uint64_t lightColor = combine_mmx_word(ls.lightColor);
			uint64_t incidentShadowColor = combine_mmx_word(ls.incidentShadowColor);
			uint64_t translucentShade = combine_mmx_word(*pTranslucentShade);
			uint64_t vResShadow = ambient;
			uint64_t vRes = ambient;

			uint64_t shifted_normal = mmx::psubw(normal, shift);
			shifted_normal = mmx::pmaddwd(shifted_normal, dirLight);
			uint64_t normal_high = mmx::psrlq(shifted_normal, 32);
			shifted_normal = mmx::paddd(shifted_normal, normal_high);
			shifted_normal = mmx::psrad(shifted_normal, 15);
			shifted_normal = mmx::punpcklwd(shifted_normal, shifted_normal);
			shifted_normal = mmx::punpckldq(shifted_normal, shifted_normal);
			uint64_t sign = mmx::psraw(shifted_normal, 16);
			uint64_t negative_f = mmx::pand(shifted_normal, sign);
			uint64_t f = mmx::pandn(sign, shifted_normal);
			const uint64_t mask = 0xFFFF'FFFF'FFFF'FFFFULL;
			negative_f = mmx::pxor(negative_f, mask);

			lightColor = mmx::pmulhw(lightColor, f);
			incidentShadowColor = mmx::pmulhw(incidentShadowColor, f);
			translucentShade = mmx::pmulhw(translucentShade, negative_f);
			shadeColor = mmx::pmulhw(shadeColor, negative_f);

			vRes = mmx::paddw(vRes, lightColor);
			vResShadow = mmx::paddw(vResShadow, incidentShadowColor);
			vRes = mmx::paddw(vRes, translucentShade);
			vResShadow = mmx::paddw(vResShadow, shadeColor);
			vRes = mmx::psraw(vRes, 4);
			vResShadow = mmx::psraw(vResShadow, 4);

			dwColor = mmx::packuswb(vRes, vRes);
			dwShadowColor = mmx::packuswb(vResShadow, vResShadow);
		}

		(*pResColors)[k] = dwColor;
		(*pResShadow)[k] = dwShadowColor;
		dwPrevNormal = dwNormal;
	}
}
#endif

#if HAS_SSE2
struct SWarFogSSE2Sample
{
	DWORD fogValues;
	int xWeights;
	int yWeights;
};

static inline SWarFogSSE2Sample PrepareWarFogSSE2Sample(
	int nX, int nY, int nMask, const CArray2D<unsigned char> &fog )
{
	const int nYU = ( nY >> 14 ) & nMask;
	const int nYfi = nY & 0x3fff;
	const int nXL = ( nX >> 14 ) & nMask;
	const int nXfi = nX & 0x3fff;
	const unsigned char *pUp = (&fog[nYU][0]) + nXL;
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

static void SampleWarFogInt( const std::vector<int> &intCoords, const CArray2D<unsigned char> &fog, std::vector<unsigned char> *_pRes, int nVertices )
{
	ASSERT( fog.GetSizeX() == fog.GetSizeY() );
	ASSERT( GetNextPow2( fog.GetSizeX() - 1 ) + 1 == fog.GetSizeX() );
	if ( nVertices <= 0 )
		return;

	unsigned char *pRes = &(*_pRes)[0];
	const int nMask = fog.GetSizeX() - 2;
	int k = 0;
	for ( ; k + 1 < nVertices; k += 2 )
	{
		const SWarFogSSE2Sample sample0 = PrepareWarFogSSE2Sample(
			intCoords[k * 2], intCoords[k * 2 + 1], nMask, fog );
		const SWarFogSSE2Sample sample1 = PrepareWarFogSSE2Sample(
			intCoords[k * 2 + 2], intCoords[k * 2 + 3], nMask, fog );
		const WORD result = SampleWarFogIntSSE2x2( sample0, sample1 );
		pRes[k] = static_cast<unsigned char>( result );
		pRes[k + 1] = static_cast<unsigned char>( result >> 8 );
	}

	if ( k < nVertices )
	{
		const SWarFogSSE2Sample sample = PrepareWarFogSSE2Sample(
			intCoords[k * 2], intCoords[k * 2 + 1], nMask, fog );
		const SWarFogSSE2Sample emptySample{};
		pRes[k] = static_cast<unsigned char>( SampleWarFogIntSSE2x2( sample, emptySample ) );
	}
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

	// Convert with double intermediates to retain the old x87 float-product precision.
	const float fpScale = fScale * 0x4000;
	const __m128d scale = _mm_set1_pd( static_cast<double>( fpScale ) );
	int k = 0;
	for ( ; k + 1 < nVertices; k += 2 )
	{
		const __m128 positions = _mm_set_ps(
			srcPos[k + 1].y, srcPos[k + 1].x, srcPos[k].y, srcPos[k].x );
		const __m128d positions0 = _mm_cvtps_pd( positions );
		const __m128d positions1 = _mm_cvtps_pd( _mm_movehl_ps( positions, positions ) );
		const __m128i coords0 = _mm_cvtpd_epi32( _mm_mul_pd( positions0, scale ) );
		const __m128i coords1 = _mm_cvtpd_epi32( _mm_mul_pd( positions1, scale ) );
		const __m128i coords = _mm_unpacklo_epi64( coords0, coords1 );
		_mm_storeu_si128( reinterpret_cast<__m128i*>( &tmp[k * 2] ), coords );
	}

	if ( k < nVertices )
	{
		const __m128 positions = _mm_set_ps( 0, 0, srcPos[k].y, srcPos[k].x );
		const __m128d positionsDouble = _mm_cvtps_pd( positions );
		const __m128i coords = _mm_cvtpd_epi32( _mm_mul_pd( positionsDouble, scale ) );
		_mm_storel_epi64( reinterpret_cast<__m128i*>( &tmp[k * 2] ), coords );
	}

	SampleWarFogInt( tmp, fog1, _pRes1, nVertices );
	if ( _pRes2 )
		SampleWarFogInt( tmp, fog2, _pRes2, nVertices );
}
#else
// sample fog of war array with bilinear filteration and store result to pRes
#pragma warning( disable : 4799 )
static void SampleWarFogInt( const std::vector<int> &intCoords, const CArray2D<unsigned char> &fog, std::vector<unsigned char> *_pRes, int nVertices )
{
	ASSERT( fog.GetSizeX() == fog.GetSizeY() );
	ASSERT( GetNextPow2( fog.GetSizeX() - 1 ) + 1 == fog.GetSizeX() );
	uint64_t zero = 0;
	unsigned char *pRes = &(*_pRes)[0];
	int nMask = fog.GetSizeX() - 2;
	for ( const int *pTmp = &intCoords[0], *pTmpEnd = pTmp + nVertices * 2; pTmp < pTmpEnd; pTmp += 2, ++pRes )
	{
		int nY = pTmp[1];
		int nYU = ( nY >> 14 ) & nMask;
		int nYfi = nY & 0x3fff;
		uint64_t nYf = ( 0x4000 - nYfi ) | (nYfi << 16);
		int nX = pTmp[0];
		int nXL = ( nX >> 14 ) & nMask;
		int nXfi = nX & 0x3fff;
		uint64_t nXf = ( 0x4000 - nXfi ) | (nXfi << 16) ;
		const unsigned char *pUp = (&fog[nYU][0]) + nXL;
		const unsigned char *pDown = pUp + nMask + 2;

		uint64_t nData = mmx::punpckldq(
			mmx::punpcklbw(  *(unsigned short*)pUp , zero ),
			mmx::punpcklbw(  *(unsigned short*)pDown , zero )
			);
		nXf = mmx::punpckldq( nXf, nXf );
		nData = mmx::pmaddwd( nData, nXf );
		nData = mmx::psrad( nData, 14 );
		nData = mmx::packssdw( nData, zero );
		nData = mmx::pmaddwd( nData, nYf );
		*pRes = nData >> 14;
	}
}
#pragma warning( default : 4799 )

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
	float fpScale = fScale * 0x4000;

	int *out = tmp.data();
	for (const CVec3 &v : srcPos) {
		*out++ = static_cast<int>(std::lrintf(v.x * fpScale));
		*out++ = static_cast<int>(std::lrintf(v.y * fpScale));
	}

	SampleWarFogInt( tmp, fog1, _pRes1, nVertices );
	if ( _pRes2 )
		SampleWarFogInt( tmp, fog2, _pRes2, nVertices );
}
#endif

const float F_PL_RADIUS2 = 64;
const float F_PL_MIN_DISTANCE_NORMALIZED = 0.25f;
const int N_PL_ATTENUATION_SCALE = 8191;
static void CalcPointLightAttenuation( std::vector<NGfx::SMMXWord> *pRes, const std::vector<CVec3> &srcPos, const CVec3 &_vCenter, float _fRadius )
{
	int nSize = srcPos.size();
	pRes->resize( nSize );
	float fScale = F_PL_RADIUS2 / sqr( _fRadius );
	for ( int k = 0; k < nSize; ++k )
	{
		CVec3 v = _vCenter - srcPos[k];
		float f = fabs2( v );
		float fCut = (std::max)( 0.0f, sqr(_fRadius) - f ) * ( 1 / sqr(_fRadius ) );
		float fAttenuation = fCut / ( f * fScale + F_PL_MIN_DISTANCE_NORMALIZED ) / sqrt( f );
		NGfx::SMMXWord &n = (*pRes)[k];
		n.nX = Float2Int( v.x * fAttenuation * N_PL_ATTENUATION_SCALE );
		n.nY = Float2Int( v.y * fAttenuation * N_PL_ATTENUATION_SCALE );
		n.nZ = Float2Int( v.z * fAttenuation * N_PL_ATTENUATION_SCALE );
		n.nW = 0;
	}
}

#if !HAS_SSE2
static void CalculateLightColor(uint32_t dwNormal, uint64_t shift, uint64_t shift1, uint64_t lightColor, uint64_t att, NGfx::SMMXWord *pResColor) {

	uint64_t resColor = mmx::combine64( pResColor->nZ, pResColor->nY, pResColor->nX, pResColor->nW );

	uint64_t normal = mmx::punpcklbw(dwNormal, dwNormal);
	normal = mmx::psubw(normal, shift);
	normal = mmx::pmaddwd(normal, att);
	uint64_t normal_high = mmx::psrlq(normal, 32);
	normal = mmx::paddd(normal, normal_high);
	normal = mmx::psrad(normal, 15);
	normal = mmx::packssdw(normal, normal);
	normal = mmx::punpcklwd(normal, normal);
	normal = mmx::punpckldq(normal, normal);
	uint64_t mask = mmx::pcmpgtw(normal, 0);
	normal = mmx::pand(normal, mask);
	uint64_t low = mmx::pmullw(normal, lightColor);
	uint64_t high = mmx::pmulhw(normal, lightColor);
	uint64_t unpacked_low = mmx::punpcklwd(low, high);
	uint64_t unpacked_high = mmx::punpckhwd(low, high);
	unpacked_low = mmx::paddd(unpacked_low, shift1);
	unpacked_high = mmx::paddd(unpacked_high, shift1);
	unpacked_low = mmx::psrad(unpacked_low, 13);
	unpacked_high = mmx::psrad(unpacked_high, 13);
	uint64_t result = mmx::packssdw(unpacked_low, unpacked_high);
	result = mmx::paddsw(result, resColor);
	mmx::split64(result, pResColor->nZ, pResColor->nY, pResColor->nX, pResColor->nW);
}
#endif

#if HAS_SSE2
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

static void CalcPointLightColors( std::vector<NGfx::SMMXWord> *pRes,
	const std::vector<NGfx::SMMXWord> &attenuation, const std::vector<WORD> &posIndices,
	const std::vector<NGfx::SCompactVector> &_normals,
	const CVec3 &_vColor )
{
	NGfx::SMMXWord shift{};
	shift.nX = shift.nY = shift.nZ = (short)0x8000;
	NGfx::SMMXWord lightColor{};
	lightColor.nX = Float2Int( _vColor.x * 32767 );
	lightColor.nY = Float2Int( _vColor.y * 32767 );
	lightColor.nZ = Float2Int( _vColor.z * 32767 );

	SPointLightColorsSSE2Data data;
	data.shift = LoadMMXWord128( shift );
	data.lightColor = LoadMMXWord128( lightColor );
	data.rounding = _mm_set1_epi32( 1 << 12 );

	const int nSize = static_cast<int>( posIndices.size() );
	int k = 0;
	for ( ; k + 1 < nSize; k += 2 )
	{
		const __m128i packedNormals = _mm_loadl_epi64( reinterpret_cast<const __m128i*>( &_normals[k] ) );
		data.attenuation = _mm_unpacklo_epi64(
			LoadMMXWord64( attenuation[ posIndices[k] ] ),
			LoadMMXWord64( attenuation[ posIndices[k + 1] ] ) );
		const __m128i resColors = _mm_loadu_si128( reinterpret_cast<const __m128i*>( &(*pRes)[k] ) );
		const __m128i result = CalcPointLightColorsSSE2x2( packedNormals, resColors, data );
		_mm_storeu_si128( reinterpret_cast<__m128i*>( &(*pRes)[k] ), result );
	}

	if ( k < nSize )
	{
		const __m128i packedNormal = _mm_cvtsi32_si128( static_cast<int>( _normals[k].dw ) );
		data.attenuation = LoadMMXWord64( attenuation[ posIndices[k] ] );
		const __m128i resColor = LoadMMXWord64( (*pRes)[k] );
		const __m128i result = CalcPointLightColorsSSE2x2( packedNormal, resColor, data );
		_mm_storel_epi64( reinterpret_cast<__m128i*>( &(*pRes)[k] ), result );
	}
}
#else
static void CalcPointLightColors( std::vector<NGfx::SMMXWord> *pRes,
	const std::vector<NGfx::SMMXWord> &attenuation, const std::vector<WORD> &posIndices,
	const std::vector<NGfx::SCompactVector> &_normals,
	const CVec3 &_vColor )
{
	uint64_t shift = mmx::combine64(static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), 0);
	uint64_t shift1 = mmx::combine64(1 << 12, 0, 1 << 12, 0);
	uint64_t lightColor = mmx::combine64( Float2Int( _vColor.z * 32767 ), Float2Int( _vColor.y * 32767 ), Float2Int( _vColor.x * 32767 ), 0 );

	for ( int k = 0; k < posIndices.size(); ++k )
	{
		DWORD dwNormal = _normals[k].dw;
		const NGfx::SMMXWord *pAtt = &attenuation[ posIndices[k] ];
		uint64_t att = mmx::combine64( pAtt->nZ, pAtt->nY, pAtt->nX, pAtt->nW );

		NGfx::SMMXWord *pResColor = &(*pRes)[k];
		CalculateLightColor(dwNormal, shift, shift1, lightColor, att, pResColor);
	}
}
#endif

#if HAS_SSE2
static void CalcPointLightColors( std::vector<NGfx::SMMXWord> *pRes,
	const NGfx::SMMXWord &attenuation, const SUVInfo *pSrc, int _nSize, const CVec3 &_vColor )
{
	NGfx::SMMXWord shift{};
	shift.nX = shift.nY = shift.nZ = (short)0x8000;
	NGfx::SMMXWord lightColor{};
	lightColor.nX = Float2Int( _vColor.x * 32767 );
	lightColor.nY = Float2Int( _vColor.y * 32767 );
	lightColor.nZ = Float2Int( _vColor.z * 32767 );

	SPointLightColorsSSE2Data data;
	data.shift = LoadMMXWord128( shift );
	data.attenuation = LoadMMXWord128( attenuation );
	data.lightColor = LoadMMXWord128( lightColor );
	data.rounding = _mm_set1_epi32( 1 << 12 );

	int k = 0;
	for ( ; k + 1 < _nSize; k += 2 )
	{
		const __m128i packedNormals = _mm_or_si128(
			_mm_cvtsi32_si128( static_cast<int>( pSrc[k].normal.dw ) ),
			_mm_slli_si128( _mm_cvtsi32_si128( static_cast<int>( pSrc[k + 1].normal.dw ) ), 4 ) );
		const __m128i resColors = _mm_loadu_si128( reinterpret_cast<const __m128i*>( &(*pRes)[k] ) );
		const __m128i result = CalcPointLightColorsSSE2x2( packedNormals, resColors, data );
		_mm_storeu_si128( reinterpret_cast<__m128i*>( &(*pRes)[k] ), result );
	}

	if ( k < _nSize )
	{
		const __m128i packedNormal = _mm_cvtsi32_si128( static_cast<int>( pSrc[k].normal.dw ) );
		const __m128i resColor = LoadMMXWord64( (*pRes)[k] );
		const __m128i result = CalcPointLightColorsSSE2x2( packedNormal, resColor, data );
		_mm_storel_epi64( reinterpret_cast<__m128i*>( &(*pRes)[k] ), result );
	}
}
#else
static void CalcPointLightColors( std::vector<NGfx::SMMXWord> *pRes,
	const NGfx::SMMXWord &attenuation, const SUVInfo *pSrc, int _nSize, const CVec3 &_vColor )
{
	uint64_t shift = mmx::combine64(static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), static_cast<int16_t>(0x8000), 0);
	uint64_t shift1 = mmx::combine64(1 << 12, 0, 1 << 12, 0);
	uint64_t lightColor = mmx::combine64( Float2Int( _vColor.z * 32767 ), Float2Int( _vColor.y * 32767 ), Float2Int( _vColor.x * 32767 ), 0 );

	DWORD dwPrevNormal = 0;
	NGfx::SMMXWord prevColor{};
	const NGfx::SMMXWord *pAtt = &attenuation;
	for ( int k = 0; k < _nSize; ++k )
	{
		DWORD dwNormal = pSrc[k].normal.dw;
		NGfx::SMMXWord *pResColor = &(*pRes)[k];
		if ( dwNormal != dwPrevNormal )
		{
			uint64_t att = mmx::combine64( pAtt->nZ, pAtt->nY, pAtt->nX, pAtt->nW );
			NGfx::SMMXWord *pResColor = &(*pRes)[k];

			CalculateLightColor(dwNormal, shift, shift1, lightColor, att, pResColor);
			dwPrevNormal = dwNormal;
		}
		else
		{
			uint64_t resColor = mmx::combine64( pResColor->nZ, pResColor->nY, pResColor->nX, pResColor->nW );
			uint64_t prev = mmx::combine64( prevColor.nZ, prevColor.nY, prevColor.nX, prevColor.nW );
			resColor = mmx::paddsw(resColor, prev);
			mmx::split64(resColor, pResColor->nZ, pResColor->nY, pResColor->nX, pResColor->nW);
		}
	}
}
#endif

#if HAS_SSE2
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

static void AddColors( std::vector<DWORD> *pRes, const std::vector<DWORD> &src, const std::vector<NGfx::SMMXWord> &add )
{
	ASSERT( pRes->size() >= add.size() );
	ASSERT( src.size() >= add.size() );
	int nSize = add.size();
	if ( nSize == 0 )
		return;

	DWORD *pResPtr = &(*pRes)[0];
	const DWORD *pSrcPtr = &src[0];
	const NGfx::SMMXWord *pAdd = &add[0];
	int k = 0;
	for ( ; k + 1 < nSize; k += 2 )
	{
		const __m128i packedColors = _mm_loadl_epi64( reinterpret_cast<const __m128i*>( pSrcPtr + k ) );
		const __m128i addColors = _mm_unpacklo_epi64( LoadMMXWord64( pAdd[k] ), LoadMMXWord64( pAdd[k + 1] ) );
		const __m128i indices = CalcAddColorIndicesSSE2( packedColors, addColors );
		pResPtr[k] = LookupCubicRootColorSSE2( indices );
		pResPtr[k + 1] = LookupCubicRootColorSSE2( _mm_srli_si128( indices, 8 ) );
	}

	if ( k < nSize )
	{
		const __m128i packedColor = _mm_cvtsi32_si128( static_cast<int>( pSrcPtr[k] ) );
		const __m128i indices = CalcAddColorIndicesSSE2( packedColor, LoadMMXWord64( pAdd[k] ) );
		pResPtr[k] = LookupCubicRootColorSSE2( indices );
	}
}
#else
static void AddColors( std::vector<DWORD> *pRes, const std::vector<DWORD> &src, const std::vector<NGfx::SMMXWord> &add )
{
	ASSERT( pRes->size() >= add.size() );
	ASSERT( src.size() >= add.size() );
	int nSize = add.size();
	DWORD *pResPtr = &(*pRes)[0];
	const DWORD *pSrcPtr = &src[0];
	const NGfx::SMMXWord *pAdd = &add[0];

	uint64_t mask = 0x4000'4000'4000'4000ULL;

	for ( DWORD *pResEnd = pResPtr + nSize; pResPtr < pResEnd; ++pResPtr, ++pSrcPtr, ++pAdd )
	{
		DWORD dwColor = *pSrcPtr;
		uint64_t addColor = mmx::combine64(pAdd->nZ, pAdd->nY, pAdd->nX, pAdd->nW);

		uint64_t color = mmx::punpcklbw(dwColor, dwColor);
		color = mmx::psrlw(color, 1);
		uint64_t color_square = mmx::pmulhw(color, color);
		color_square = mmx::psllw(color_square, 1);
		uint64_t color_cube_high = mmx::pmulhw(color_square, color);
		uint64_t color_cube_low = mmx::pmullw(color_square, color);
		color_cube_high = mmx::psllw(color_cube_high, 1);
		color_cube_high = mmx::paddsw(color_cube_high, addColor);
		color_cube_high = mmx::psrlw(color_cube_high, 1);
		color_cube_low = mmx::psrlw(color_cube_low, 2);
		color_cube_low = mmx::por(color_cube_low, mask);
		uint64_t eq = mmx::pcmpeqw(color_cube_high, 0);
		color_cube_low = mmx::pand(color_cube_low, eq);
		color_cube_high = mmx::pandn(eq, color_cube_high);
		uint64_t color_cube = mmx::por(color_cube_high, color_cube_low);
		color_cube_high = mmx::psrlq(color_cube, 32);
		uint32_t index1 = color_cube & 0x7FFF;
		uint32_t index2 = (color_cube >> 16) & 0x7FFF;
		uint32_t index3 = color_cube_high & 0x7FFF;
		uint8_t c1 = nCubicRoot[index1];
		uint8_t c2 = nCubicRoot[index2];
		uint8_t c3 = nCubicRoot[index3];
		dwColor = c1 | (c2 << 8) | (c3 << 16);
		*pResPtr = dwColor;
	}
}
#endif

static void AddPointLight( const SPerVertexLightState::SPointLightInfo &p,
	const std::vector<CVec3> &srcPos, const SUVInfo *pSrc, const std::vector<WORD> &posIndices,
	const std::vector<NGfx::SCompactVector> &_normals,
	std::vector<NGfx::SMMXWord> *pColors,
	SCacheLightingInfo *pCache, const SBound &bv )
{
	static std::vector<NGfx::SMMXWord> attenuation;
	if ( pCache->bReplaceWithDirectional )
	{
		float fScale = F_PL_RADIUS2 / sqr( p.fRadius );
		CVec3 vCenter = bv.s.ptCenter;
		NGfx::SMMXWord att;
		CVec3 v = p.vCenter - vCenter;
		float f = fabs2( v );
		// take into account size of object
		float fCorrected = f + sqr( bv.s.fRadius ) * 0.25f;
		float fCut = (std::max)( 0.0f, sqr(p.fRadius) - fCorrected ) * ( 1 / sqr(p.fRadius ) );
		float fAttenuation = fCut / ( fCorrected * fScale + F_PL_MIN_DISTANCE_NORMALIZED ) / sqrt( f );
		att.nX = Float2Int( v.x * fAttenuation * N_PL_ATTENUATION_SCALE );
		att.nY = Float2Int( v.y * fAttenuation * N_PL_ATTENUATION_SCALE );
		att.nZ = Float2Int( v.z * fAttenuation * N_PL_ATTENUATION_SCALE );
		att.nW = 0;
		CalcPointLightColors( pColors, att, pSrc, posIndices.size(), p.vColor );
	}
	else
	{
		CalcPointLightAttenuation( &attenuation, srcPos, p.vCenter, p.fRadius );
		CalcPointLightColors( pColors, attenuation, posIndices, _normals, p.vColor );
	}
}

struct SPVLightCalcer
{
	int nWarFogMask;
	unsigned char cFogHold;
	unsigned char *pWarFogNew, *pWarFogOld;
	std::vector<DWORD> *pColors, *pShadowColors;
	static std::vector<DWORD> colorsHold, shadowColorsHold;
	static std::vector<NGfx::SMMXWord> pointColors;
	static std::vector<unsigned char> warFogNewHold, warFogOldHold;

	SPVLightCalcer() : pWarFogNew(0), pWarFogOld(0), pColors(0), pShadowColors(0) {}

	void CalcLight(
		const std::vector<CVec3> &srcPos, const SUVInfo *pSrc, const std::vector<WORD> &posIndices,
		const std::vector<NGfx::SCompactVector> &_normals, const std::vector<DWORD> &vertexColor,
		const SPerVertexLightState &ls, SCacheLightingInfo *pCache, const SBound &bv )
	{
		//NHPTimer::STime tStart, tFinish;
		//NHPTimer::GetTime( &tStart );
		int nVertices = posIndices.size();

		if ( !pCache->bSkipLighting )
		{
			// determine affecting point lights
			std::vector<int> pl, plIdx, dynplIdx;
			if ( !pCache->bSkipStaticPointLights )
			{
				for ( int k = 0; k < ls.staticPointLights.size(); ++k )
				{
					const SPerVertexLightState::SPointLightInfo &p = ls.staticPointLights[k];
					if ( !DoesIntersect( SSphere( p.vCenter, p.fRadius ), bv ) )
						continue;
					pl.push_back( p.nID );
					plIdx.push_back( k );
				}
			}

			// decide if cache succeeded
			if (
				pCache->nDirectionalLightID == ls.nDirectionalID &&
				pCache->pointLightIDs == pl )
			{
				pColors = &pCache->colors;
				pShadowColors = &pCache->shadowColors;
			}
			else
			{
				if ( pCache->bDoNotCacheLighting )
				{
					pColors = &colorsHold;
					pShadowColors = &shadowColorsHold;
				}
				else
				{
					pColors = &pCache->colors;
					pShadowColors = &pCache->shadowColors;
				}

				// calc directional
				// ~60 clocks / vertex without same normal short cut
				CalcDirectionalLighting( posIndices, _normals, ls, pCache->bTranslucent, pCache->vTranslucentColor, pColors, pShadowColors );
				if ( !pCache->bDoNotCacheLighting )
					pCache->nDirectionalLightID = ls.nDirectionalID;

				MultiplyOnColor( pShadowColors, vertexColor );
				MultiplyOnColor( pColors, vertexColor );

				// calc point lights
				if ( !pl.empty() )
				{
					std::vector<NGfx::SMMXWord> *pPointColors = &pCache->pointLight;
					if ( pl != pCache->pointLightIDs )
					{
						if ( pCache->bDoNotCacheLighting )
							pPointColors = &pointColors;

						pPointColors->resize( 0 );
						NGfx::SMMXWord zero;
						Zero( zero );
						pPointColors->resize( nVertices, zero );
						for ( int k = 0; k < plIdx.size(); ++k )
						{
							const SPerVertexLightState::SPointLightInfo &p = ls.staticPointLights[ plIdx[k] ];
							AddPointLight( p, srcPos, pSrc, posIndices, _normals, pPointColors, pCache, bv );
						}

						if ( !pCache->bDoNotCacheLighting )
							pCache->pointLightIDs = pl;
					}
					else
						ASSERT( !pCache->bDoNotCacheLighting );
					AddColors( pColors, *pColors, *pPointColors );
					AddColors( pShadowColors, *pShadowColors, *pPointColors );
				}
			}
			// calc dynamic point lights
			if ( !ls.dynamicPointLights.empty() )
			{
				pointColors.resize( 0 );
				for ( int k = 0; k < ls.dynamicPointLights.size(); ++k )
				{
					// ~150 clocks / vertex / point light if replace with directional is not used
					const SPerVertexLightState::SPointLightInfo &p = ls.dynamicPointLights[k];
					if ( !DoesIntersect( SSphere( p.vCenter, p.fRadius ), bv ) )
						continue;
					if ( pointColors.empty() )
					{
						NGfx::SMMXWord zero;
						Zero( zero );
						pointColors.resize( nVertices, zero );
					}
					AddPointLight( p, srcPos, pSrc, posIndices, _normals, &pointColors, pCache, bv );
				}
				if ( !pointColors.empty() )
				{
					if ( colorsHold.size() < nVertices )
						colorsHold.resize( nVertices );
					if ( shadowColorsHold.size() < nVertices )
						shadowColorsHold.resize( nVertices );
					AddColors( &colorsHold, *pColors, pointColors );
					AddColors( &shadowColorsHold, *pShadowColors, pointColors );
					pColors = &colorsHold;
					pShadowColors = &shadowColorsHold;
				}
			}
		}

		// fetch warfog
		if ( ls.warFogNew.GetSizeX() > 1 )
		{
			std::vector<unsigned char> *pNew, *pOld;
			if ( pCache->bDoNotCacheLighting || pCache->bReplaceWithDirectional )
			{
				pNew = &warFogNewHold;
				pOld = &warFogOldHold;
			}
			else
			{
				pNew = &pCache->warFogNew;
				pOld = &pCache->warFogOld;
				if ( ls.nWarFogOldID != pCache->nWarFogOldID && ls.nWarFogOldID == pCache->nWarFogNewID )
				{
					pCache->warFogOld.swap( pCache->warFogNew );
					std::swap( pCache->nWarFogNewID, pCache->nWarFogOldID );
				}
			}

			if ( pCache->bReplaceWithDirectional )
			{
				std::vector<CVec3> src(1);
				src[0] = bv.s.ptCenter;
				SampleWarFog( src, ls.fWarFogScale, pNew, ls.warFogNew, pOld, ls.warFogOld  );
				nWarFogMask = 0;
			}
			else
			{
				if ( ls.nWarFogNewID != pCache->nWarFogNewID && ls.nWarFogOldID != pCache->nWarFogOldID )
				{
					SampleWarFog( srcPos, ls.fWarFogScale, pNew, ls.warFogNew, pOld, ls.warFogOld );
					if ( !pCache->bDoNotCacheLighting )
					{
						pCache->nWarFogNewID = ls.nWarFogNewID;
						pCache->nWarFogOldID = ls.nWarFogOldID;
					}
				}
				else
				{
					if ( ls.nWarFogNewID != pCache->nWarFogNewID )
					{
						// ~50 clocks / vertex
						SampleWarFog( srcPos, ls.fWarFogScale, pNew, ls.warFogNew, 0, ls.warFogNew );
						if ( !pCache->bDoNotCacheLighting )
							pCache->nWarFogNewID = ls.nWarFogNewID;
					}
					if ( ls.nWarFogOldID != pCache->nWarFogOldID )
					{
						// ~50 clocks / vertex
						SampleWarFog( srcPos, ls.fWarFogScale, pOld, ls.warFogOld, 0, ls.warFogNew );
						if ( !pCache->bDoNotCacheLighting )
							pCache->nWarFogOldID = ls.nWarFogOldID;
					}
				}
				nWarFogMask = 0xffffffff;
			}
			pWarFogOld = &(*pOld)[0];
			pWarFogNew = &(*pNew)[0];
		}
		else
		{
			cFogHold = 0xff;
			pWarFogOld = &cFogHold;
			pWarFogNew = &cFogHold;
			nWarFogMask = 0;
		}
		//NHPTimer::GetTime( &tFinish );
		//DbgTrc( "%g clocks per vertex, %d vertices", ((double)(tFinish - tStart)) / nVertices, nVertices );
	}
};

void SampleWarFog( const std::vector<CVec3> &vPos, const SPerVertexLightState &ls, std::vector<float> *pRes )
{
	if ( ls.warFogNew.GetSizeX() <= 1 )
	{
		pRes->resize( 0 );
		return;
	}
	if ( pRes->size() < vPos.size() )
		pRes->resize( vPos.size() );
	std::vector<unsigned char> newFog, oldFog;
	SampleWarFog( vPos, ls.fWarFogScale, &newFog, ls.warFogNew, &oldFog, ls.warFogOld  );
	float fBlend = ls.GetWarFogBlend();
	float fN = fBlend * ( 1 / 255.0f ), fO = ( 1 - fBlend ) * ( 1 / 255.0f );
	for ( int k = 0; k < vPos.size(); ++k )
	{
		float fRes = newFog[0] * fN + oldFog[0] * fO;
		(*pRes)[k] = fRes;
	}
}

std::vector<DWORD> SPVLightCalcer::colorsHold, SPVLightCalcer::shadowColorsHold;
std::vector<NGfx::SMMXWord> SPVLightCalcer::pointColors;
std::vector<unsigned char> SPVLightCalcer::warFogNewHold, SPVLightCalcer::warFogOldHold;

void CalcPerVertexLight( NGfx::SGeomVecFull *pRes,
	const std::vector<CVec3> &srcPos, const SUVInfo *pSrc, const std::vector<WORD> &posIndices,
	const std::vector<NGfx::SCompactVector> &_normals, const std::vector<DWORD> &vertexColor,
	const SPerVertexLightState &ls, SCacheLightingInfo *pCache, const SBound &bv )
{
	if ( posIndices.empty() )
		return;
	SPVLightCalcer l;
	l.CalcLight( srcPos, pSrc, posIndices, _normals, vertexColor, ls, pCache, bv );
	//NHPTimer::STime tStart, tFinish;
	//NHPTimer::GetTime( &tStart );
	if ( pCache->bSkipLighting )
	{
		if ( pCache->bSelfIllum )
		{
			// ~? clocks / vertex
			int k = 0;
			for ( const SUVInfo *pEnd = pSrc + posIndices.size(); pSrc < pEnd; ++pSrc, ++pRes, ++k )
			{
				NGfx::SGeomVecFull &res = *pRes;
				const CVec3 &pos = srcPos[ posIndices[k] ];
				res.pos = pos;
				res.normal = _normals[k];
				res.tex = pSrc->tex;
				res.texLM = pSrc->texLM;
				int nWarFogIndex = posIndices[k] & l.nWarFogMask;
				res.texU.dw = 0x404040 | ( l.pWarFogOld[ nWarFogIndex ] << 24 );
				res.texV.dw = 0x404040 | ( l.pWarFogNew[ nWarFogIndex ] << 24 );
			}
		}
		else
		{
			// ~? clocks / vertex
			int k = 0;
			for ( const SUVInfo *pEnd = pSrc + posIndices.size(); pSrc < pEnd; ++pSrc, ++pRes, ++k )
			{
				NGfx::SGeomVecFull &res = *pRes;
				const CVec3 &pos = srcPos[ posIndices[k] ];
				res.pos = pos;
				res.normal = _normals[k];
				res.tex = pSrc->tex;
				res.texLM = pSrc->texLM;
				int nWarFogIndex = posIndices[k] & l.nWarFogMask;
				res.texU.dw = (pSrc->texU.dw & 0xffffff) | ( l.pWarFogOld[ nWarFogIndex ] << 24 );
				res.texV.dw = (pSrc->texV.dw & 0xffffff) | ( l.pWarFogNew[ nWarFogIndex ] << 24 );
			}
		}
	}
	else
	{
		// ~? clocks / vertex
		int k = 0;
		for ( const SUVInfo *pEnd = pSrc + posIndices.size(); pSrc < pEnd; ++pSrc, ++pRes, ++k )
		{
			NGfx::SGeomVecFull &res = *pRes;
			const CVec3 &pos = srcPos[ posIndices[k] ];
			res.pos = pos;
			res.normal = _normals[k];
			res.tex = pSrc->tex;
			res.texLM = pSrc->texLM;
			int nWarFogIndex = posIndices[k] & l.nWarFogMask;
			res.texU.dw = ((*l.pShadowColors)[k] & 0xffffff) | ( l.pWarFogOld[ nWarFogIndex ] << 24 );
			res.texV.dw = ((*l.pColors)[k] & 0xffffff) | ( l.pWarFogNew[ nWarFogIndex ] << 24 );
		}
	}
	//NHPTimer::GetTime( &tFinish );
	//DbgTrc( "%g clocks per vertex, %d vertices", ((double)(tFinish - tStart)) / nVertices, nVertices );
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
	uint64_t transparency = mmx::combine64(mTransp.nZ, mTransp.nY, mTransp.nX, mTransp.nW);

	if ( bMultiplyOnTransparency )
	{
		mTransp.nX = mTransp.nY = mTransp.nZ = 0; mTransp.nW = 0;
	}
	else
	{
		mTransp.nX = mTransp.nY = mTransp.nZ = 0x7fff; mTransp.nW = 0;
	}
	uint64_t multiplyTransparency = mmx::combine64(mTransp.nZ, mTransp.nY, mTransp.nX, mTransp.nW);

	for ( ; p < pEnd; ++p, pSrc += nSrcStride / 4, ++pPosIndices, ++pTransp )
	{
		int nScaleIndex = (*pPosIndices) & nScaleMask;
		uint64_t n = ((int) (pScale[ nScaleIndex ]) ) << 2;
		uint64_t nScale = pTransp->w << 7;

		uint64_t src = mmx::punpcklbw(*pSrc, *pSrc);
		src = mmx::psrlw(src, 1);
		n = mmx::punpcklwd(n, n);
		n = mmx::punpckldq(n, n);
		src = mmx::pmulhw(src, n);
		src = mmx::por(src, transparency);
		nScale = mmx::punpcklwd(nScale, nScale);
		nScale = mmx::punpckldq(nScale, nScale);
		nScale = mmx::por(nScale, multiplyTransparency);
		src = mmx::pmulhw(src, nScale);
		src = mmx::packuswb(src, src);
		*p = src & 0xFFFFFFFFUL;
	}
}

void CalcPerVertexLight( NGfx::SGeomVecT2C1 *pRes,
	const std::vector<CVec3> &srcPos, const SUVInfo *pSrc, const std::vector<WORD> &posIndices,
	const std::vector<NGfx::SCompactVector> &_normals, const std::vector<DWORD> &vertexColor,
	const SPerVertexLightState &ls, SCacheLightingInfo *pCache, const SBound &bv )
{
	if ( posIndices.empty() )
		return;
	SPVLightCalcer l;
	l.CalcLight( srcPos, pSrc, posIndices, _normals, vertexColor, ls, pCache, bv );

	if ( pCache->bSkipLighting )
	{
		if ( pCache->bSelfIllum )
		{
			DWORD dwWhite = 0x404040;
			ScaleColors( &l.colorsHold, &dwWhite, 0, l.pWarFogNew, l.nWarFogMask, posIndices, _normals, pCache->bMultiplyOnTransparency );
		}
		else
			ScaleColors( &l.colorsHold, &pSrc->texU.dw, sizeof(*pSrc), l.pWarFogNew, l.nWarFogMask, posIndices, _normals, pCache->bMultiplyOnTransparency );
	}
	else
	{
		ScaleColors( &l.colorsHold, &(*l.pColors)[0], sizeof((*l.pColors)[0]), l.pWarFogNew, l.nWarFogMask, posIndices, _normals, pCache->bMultiplyOnTransparency );
	}
	int k = 0;
	for ( const SUVInfo *pEnd = pSrc + posIndices.size(); pSrc < pEnd; ++pSrc, ++pRes, ++k )
	{
		NGfx::SGeomVecT2C1 &res = *pRes;
		const CVec3 &pos = srcPos[ posIndices[k] ];
		res.pos = pos;
		res.color.dwColor = l.colorsHold[k];//( l.colorsHold[k] & 0xffffff ) | ( _normals[ k ].dw & 0xff000000 );
		res.tex1 = NGfx::GetTexCoords( pSrc->tex );
		res.tex2 = NGfx::GetTexCoords( pSrc->texLM );
	}
}
}

