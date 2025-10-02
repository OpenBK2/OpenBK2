#pragma once

namespace NGfx
{
	class CRenderContext;
}
class CTransformStack;
namespace NGScene
{
struct SNLProjectionInfo
{
	// u = dp4( pos, vTexU ), v = dp4( pos, vTexV )
	// TexU = vShift.z * u * log( 7 + sqrt(u) ) / sqrt( 7 + sqr(u) ) + vShift.x
	// TexV = vShift.w * v * log( 7 + sqrt(v) ) / sqrt( 7 + sqr(v) ) + vShift.y
	CVec4 vTexU, vTexV;
	CVec4 vShift;
	CVec4 vMinMax;// ( uMin, uMax, vMin, vMax )
	
	/////////////////////////////////////////////////////linear infos from this point
	CVec4 vDirX; 
	CVec4 vDirY; 
	float fMinX, fMaxX, fMinY, fMaxY;
	CVec4 lvShift; 
	bool bNeedUpdate;

	SNLProjectionInfo() :
	    lvShift(0, 0, 0, 0),
	    vTexU(1,0,0,0), 
		vTexV(0,1,0,0), 
		vShift(0,0,0,0), 
		vMinMax(0,0,0,0), 
		bNeedUpdate(true),
		fMinX(+1e38f),
		fMinY(+1e38f),
		fMaxX(-1e38f),
		fMaxY(-1e38f)
		{
		}
};

struct SShadowMatrixAlign
{
	float fRotation;

	SShadowMatrixAlign() : fRotation(0) {}
};

// fMinimalElement - value influencing non linearity of projection in meters, the more this value the more linear projection is
void MakeShadowMatrix( SNLProjectionInfo *pRes, CTransformStack *pShadowGeomTS, float fMinimalSize, const CTransformStack &ts, 
	const CVec3 &ptDir, float fMaxHeight, const SBound &sceneBound, float fSceneHeight, SShadowMatrixAlign *pAlign );

}

