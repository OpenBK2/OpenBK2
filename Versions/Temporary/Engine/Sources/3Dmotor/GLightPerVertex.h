#pragma once
#include "GPixelFormat.h"
#include "../Misc/2Darray.h"

namespace NGfx
{
struct SGeomVecFull;
struct SGeomVecT2C1;
}
namespace NGScene
{

// SPerVertexLightState

struct SPerVertexLightState
{
	struct SPointLightInfo
	{
		CVec3 vCenter;
		float fRadius;
		CVec3 vColor;
		int nID;

		SPointLightInfo() {}
		SPointLightInfo( const CVec3 &_vCenter, float _fRadius, const CVec3 &_vColor, int _nID ) 
			: vCenter(_vCenter), fRadius(_fRadius), vColor(_vColor), nID(_nID) {}
	};
	ZDATA
	NGfx::SMMXWord ambient, lightColor, incidentShadowColor, shadeColor, dirLight, shift;
	int nDirectionalID;
	// fog of war
	CArray2D<unsigned char> warFogNew, warFogOld;
	int nWarFogNewID, nWarFogOldID;
	// fWarFogBlend == 1 - use new, == 0 - use old
	float fWarFogScale, fWarFogBlend;
	bool bWarFogUseOnlyNew;
	vector<SPointLightInfo> dynamicPointLights, staticPointLights;
	CVec3 vAmbientColor, vLightColor, vSunDir;
	ZSKIP
	CVec3 vDymanicLightsModification;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&ambient); f.Add(3,&lightColor); f.Add(4,&incidentShadowColor); f.Add(5,&shadeColor); f.Add(6,&dirLight); f.Add(7,&shift); f.Add(8,&nDirectionalID); f.Add(9,&warFogNew); f.Add(10,&warFogOld); f.Add(11,&nWarFogNewID); f.Add(12,&nWarFogOldID); f.Add(13,&fWarFogScale); f.Add(14,&fWarFogBlend); f.Add(15,&bWarFogUseOnlyNew); f.Add(16,&dynamicPointLights); f.Add(17,&staticPointLights); f.Add(18,&vAmbientColor); f.Add(19,&vLightColor); f.Add(20,&vSunDir); f.Add(22,&vDymanicLightsModification); return 0; }

	SPerVertexLightState();
	void SetDirectional(
		const CVec3 &_vAmbient, const CVec3 &_vLightColor, const CVec3 &_vShadeColor, const CVec3 &_vIncidentShadowColor,
		const CVec3 &_vDir, int _nDirectionalID, const CVec3 &_vDymanicLightsModification );
	void AddPointLight( const CVec3 &_vCenter, float _fRadius, const CVec3 &_vColor, int nPointID );
	void AddPointLight( const CVec3 &_vCenter, float _fRadius, const CVec3 &_vColor );
	void SortPointLights();
	void SetWarFogBlend( float fBlend );
	// returns true if need resample
	bool SetWarFog( const CArray2D<unsigned char> &fog, float fScale );
	void ResetStaticLights() { staticPointLights.resize(0); }
	void ResetDynamicLights() { dynamicPointLights.resize(0); }
	float GetWarFogBlend() const;
};

struct SCacheLightingInfo
{
	// can not be changed on the fly
	ZDATA
	bool bReplaceWithDirectional, bDoNotCacheLighting, bSkipLighting, bSelfIllum;
	bool bMultiplyOnTransparency, bSkipStaticPointLights, bTranslucent;
	CVec3 vTranslucentColor;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&bReplaceWithDirectional); f.Add(3,&bDoNotCacheLighting); f.Add(4,&bSkipLighting); f.Add(5,&bSelfIllum); f.Add(6,&bMultiplyOnTransparency); f.Add(7,&bSkipStaticPointLights); f.Add(8,&bTranslucent); f.Add(9,&vTranslucentColor); return 0; }
	vector<NGfx::SMMXWord> pointLight;
	vector<int> pointLightIDs;
	vector<DWORD> colors, shadowColors;
	int nDirectionalLightID;
	vector<unsigned char> warFogNew, warFogOld;
	int nWarFogNewID, nWarFogOldID;

	SCacheLightingInfo() : bReplaceWithDirectional( false ), bDoNotCacheLighting(false), bSkipLighting(false), bSelfIllum(false), bMultiplyOnTransparency(true), bSkipStaticPointLights(false), bTranslucent(false), vTranslucentColor(0,0,0) { Clear(); }
	void Clear() { nDirectionalLightID = 0; pointLightIDs.clear(); nWarFogNewID = 0; nWarFogOldID = 0; }
};

struct SUVInfo;
void CalcPerVertexLight( NGfx::SGeomVecFull *pRes, 
	const vector<CVec3> &srcPos, const SUVInfo *pSrc, const vector<WORD> &posIndices, 
	const vector<NGfx::SCompactVector> &_normals, const vector<DWORD> &vertexColor,
	const SPerVertexLightState &ls, SCacheLightingInfo *pCache, const SBound &bv );
void CalcPerVertexLight( NGfx::SGeomVecT2C1 *pRes, 
	const vector<CVec3> &srcPos, const SUVInfo *pSrc, const vector<WORD> &posIndices, 
	const vector<NGfx::SCompactVector> &_normals, const vector<DWORD> &vertexColor, 
	const SPerVertexLightState &ls, SCacheLightingInfo *pCache, const SBound &bv );
void SampleWarFog( const vector<CVec3> &vPos, const SPerVertexLightState &ls, vector<float> *pRes );

}

