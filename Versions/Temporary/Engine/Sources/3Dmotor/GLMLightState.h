#pragma once

namespace NGScene
{

const float F_SKY_SINGLE_STRENGTH_MUL = 2.0f;
interface IGScene;
struct SPerVertexLightState;
struct SLightStateCalcSeed
{
	int nSeed;

	SLightStateCalcSeed() : nSeed(0) {}
};

class CLightState
{
	void AddRay( const CVec3 &vFrom, const CVec3 &vDir, const CVec3 &vColor );
	void AddParallel( const SSphere &_bound, const CVec3 &vDir, const CVec3 &vColor );
	void AddPoint( const CVec3 &vCenter, float fRadius, const CVec3 &_vColor );
	CPtr<IGScene> pVis;
public:
	struct SSemiPointLight
	{
		// if vNormal is zero then its not semi point light, its full circle light
		CVec3 vColor, vCenter, vNormal;
		float fRadius;

		SSemiPointLight() {}
		SSemiPointLight( const CVec3 &_vColor, const CVec3 &_vCenter, const CVec3 &_vNormal, float _fRadius )
			: vColor(_vColor), vCenter(_vCenter), vNormal(_vNormal), fRadius(_fRadius) {}
	};
	struct SPointLight
	{
		CVec3 vColor, vCenter;
		float fRadius;
		//bool bCastShadow;
		
		SPointLight() {}
		SPointLight( const CVec3 &_vColor, const CVec3 &_vCenter, float _fR )//, bool _bCastShadow )
			: vColor(_vColor), vCenter(_vCenter), fRadius(_fR) {}//, bCastShadow(_bCastShadow) {}
	};
	ZDATA
	vector<SSemiPointLight> semiPoints;
	vector<SPointLight> points;
	vector<CVec3> skyDirections;
	CVec3 vAmbientColor;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&semiPoints); f.Add(3,&points); f.Add(4,&skyDirections); f.Add(5,&vAmbientColor); return 0; }

	void Clear() { semiPoints.clear(); points.clear(); skyDirections.clear(); }
	CVec3 GenerateSkyDir( SLightStateCalcSeed *pSeed, int nSkyDirs );
	void CreateScattered( SLightStateCalcSeed *pSeed, const SSphere &_bounds, const SPerVertexLightState &l, IGScene *pVis, int nSkyDirs );
	void CreateSimple( SLightStateCalcSeed *pSeed, const SSphere &_bounds, const SPerVertexLightState &l, int nSkyDirs );
};

}

