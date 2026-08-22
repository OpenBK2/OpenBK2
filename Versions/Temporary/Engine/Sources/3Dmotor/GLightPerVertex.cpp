#include "stdafx.h"
#include "GLightPerVertex.h"
#include "3DLib/GGeometry.h"
#include "GfxBuffers.h"
//#include "..\Misc\HPTimer.h"
#include <mmintrin.h>
#include "GSSEtransform.h"
#include "3DLib/Bound.h"

#include "GLightPerVertexKernels.h"

#include <algorithm>
#include <cstdint>

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

static void MultiplyOnColor( std::vector<uint32_t> *pRes, const std::vector<uint32_t> &mult )
{
	if ( mult.empty() )
		return;
	uint32_t *pDst = &(*pRes)[0], *pDstEnd = pDst + pRes->size();
	const uint32_t *pSrc = &mult[0];
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

// Adapters between the engine's std::vector based call sites and the kernel table.
// They own every allocation and every float-to-fixed conversion, so the kernel
// translation units stay free of std::vector and CArray2D - see the note in
// GLightPerVertexKernels.h for why that matters to the AVX2 build.

static void CalcDirectionalLighting(
	const std::vector<uint16_t> &posIndices,
	const std::vector<NGfx::SCompactVector> &_normals,
	const SPerVertexLightState &ls, bool bTranslucent, const CVec3 &vTranslucentColor,
	std::vector<uint32_t> *pResColors, std::vector<uint32_t> *pResShadow )
{
	pResColors->resize( posIndices.size() );
	pResShadow->resize( posIndices.size() );
	if ( posIndices.empty() )
		return;
	NGfx::SMMXWord translucentShade = ls.shadeColor;
	if ( bTranslucent )
		ConvertColor( &translucentShade, MulPerComp( ls.vLightColor, vTranslucentColor ) );
	GetLightingKernels().pCalcDirectionalLighting(
		&_normals[0], posIndices.size(), ls, translucentShade,
		&(*pResColors)[0], &(*pResShadow)[0] );
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

	const SLightingKernels &kernels = GetLightingKernels();
	kernels.pSampleWarFogCoords( &srcPos[0], nVertices, fScale, &tmp[0] );

	ASSERT( fog1.GetSizeX() == fog1.GetSizeY() );
	kernels.pSampleWarFogInt( &tmp[0], &fog1[0][0], fog1.GetSizeX(), &(*_pRes1)[0], nVertices );
	if ( _pRes2 )
	{
		ASSERT( fog2.GetSizeX() == fog2.GetSizeY() );
		kernels.pSampleWarFogInt( &tmp[0], &fog2[0][0], fog2.GetSizeX(), &(*_pRes2)[0], nVertices );
	}
}

static void CalcPointLightAttenuation( std::vector<NGfx::SMMXWord> *pRes, const std::vector<CVec3> &srcPos, const CVec3 &_vCenter, float _fRadius )
{
	int nSize = srcPos.size();
	pRes->resize( nSize );
	if ( nSize == 0 )
		return;
	GetLightingKernels().pCalcPointLightAttenuation(
		&(*pRes)[0], &srcPos[0], nSize, _vCenter, _fRadius );
}

static NGfx::SMMXWord MakePointLightColor( const CVec3 &vColor )
{
	NGfx::SMMXWord lightColor{};
	lightColor.nX = Float2Int( vColor.x * 32767 );
	lightColor.nY = Float2Int( vColor.y * 32767 );
	lightColor.nZ = Float2Int( vColor.z * 32767 );
	lightColor.nW = 0;
	return lightColor;
}

static void CalcPointLightColors( std::vector<NGfx::SMMXWord> *pRes,
	const std::vector<NGfx::SMMXWord> &attenuation, const std::vector<uint16_t> &posIndices,
	const std::vector<NGfx::SCompactVector> &_normals,
	const CVec3 &_vColor )
{
	if ( posIndices.empty() )
		return;
	GetLightingKernels().pCalcPointLightColorsIndexed(
		&(*pRes)[0], &attenuation[0], &posIndices[0], &_normals[0],
		posIndices.size(), MakePointLightColor( _vColor ) );
}

static void CalcPointLightColors( std::vector<NGfx::SMMXWord> *pRes,
	const NGfx::SMMXWord &attenuation, const SUVInfo *pSrc, int _nSize, const CVec3 &_vColor )
{
	if ( _nSize <= 0 )
		return;
	// The kernel walks normals with a stride, so it never needs to know SUVInfo.
	GetLightingKernels().pCalcPointLightColorsUniform(
		&(*pRes)[0], attenuation, &pSrc[0].normal, sizeof( SUVInfo ), _nSize,
		MakePointLightColor( _vColor ) );
}

static void AddColors( std::vector<uint32_t> *pRes, const std::vector<uint32_t> &src, const std::vector<NGfx::SMMXWord> &add )
{
	ASSERT( pRes->size() >= add.size() );
	ASSERT( src.size() >= add.size() );
	if ( add.empty() )
		return;
	// pRes and src alias at two call sites; every kernel reads a block before it
	// writes that same block, so in-place accumulation is safe.
	GetLightingKernels().pAddColors( &(*pRes)[0], &src[0], &add[0], add.size() );
}

static void ScaleColors( std::vector<uint32_t> *pRes, const uint32_t *_pSrc, int nSrcStride,
	unsigned char *pScale, int nScaleMask, const std::vector<uint16_t> &posIndices, const std::vector<NGfx::SCompactVector> &transp,
	bool bMultiplyOnTransparency )
{
	int nSize = posIndices.size();
	if ( pRes->size() < nSize )
		pRes->resize( nSize );
	if ( nSize == 0 )
		return;
	ASSERT( sizeof(uint32_t) == sizeof(transp[0]) );
	GetLightingKernels().pScaleColors(
		&(*pRes)[0], _pSrc, nSrcStride, pScale, nScaleMask,
		&posIndices[0], &transp[0], nSize, bMultiplyOnTransparency );
}

static void AddPointLight( const SPerVertexLightState::SPointLightInfo &p,
	const std::vector<CVec3> &srcPos, const SUVInfo *pSrc, const std::vector<uint16_t> &posIndices,
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
	std::vector<uint32_t> *pColors, *pShadowColors;
	static std::vector<uint32_t> colorsHold, shadowColorsHold;
	static std::vector<NGfx::SMMXWord> pointColors;
	static std::vector<unsigned char> warFogNewHold, warFogOldHold;

	SPVLightCalcer() : pWarFogNew(0), pWarFogOld(0), pColors(0), pShadowColors(0) {}

	void CalcLight(
		const std::vector<CVec3> &srcPos, const SUVInfo *pSrc, const std::vector<uint16_t> &posIndices,
		const std::vector<NGfx::SCompactVector> &_normals, const std::vector<uint32_t> &vertexColor,
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

std::vector<uint32_t> SPVLightCalcer::colorsHold, SPVLightCalcer::shadowColorsHold;
std::vector<NGfx::SMMXWord> SPVLightCalcer::pointColors;
std::vector<unsigned char> SPVLightCalcer::warFogNewHold, SPVLightCalcer::warFogOldHold;

void CalcPerVertexLight( NGfx::SGeomVecFull *pRes,
	const std::vector<CVec3> &srcPos, const SUVInfo *pSrc, const std::vector<uint16_t> &posIndices,
	const std::vector<NGfx::SCompactVector> &_normals, const std::vector<uint32_t> &vertexColor,
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

void CalcPerVertexLight( NGfx::SGeomVecT2C1 *pRes,
	const std::vector<CVec3> &srcPos, const SUVInfo *pSrc, const std::vector<uint16_t> &posIndices,
	const std::vector<NGfx::SCompactVector> &_normals, const std::vector<uint32_t> &vertexColor,
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
			uint32_t dwWhite = 0x404040;
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

