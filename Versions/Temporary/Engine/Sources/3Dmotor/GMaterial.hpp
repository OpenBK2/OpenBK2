#pragma once

#include "3Dmotor_export.h"


#include "System/Dg.h"
namespace NGfx
{
	class CTexture;
	class CCubeTexture;
}

namespace NGScene
{

enum EDynamicType
{
	DT_DONT_CARE,
	DT_FORCE_STATIC,
	DT_FORCE_DYNAMIC,
};
enum EMaterialFlags
{
	MF_GENERIC = 0,
	MF_WATER = 1,
	MF_TRACKS = 2,
	MF_TERRAIN = 3,
	MF_CLOUDS_H5 = 4,
	MF_ANIM_WATER = 5,
	MF_SURF = 6,
	MF_SIMPLE_SKY = 7,
	MF_REFLECT_WATER = 8,
	MF_EFFECT_MASK = 15,

	MF_OPAQUE = 0,
	MF_OVERLAY = 16,
	MF_OVERLAY_ZWRITE = 32,
	MF_TRANSPARENT = 48,
	MF_ALPHA_TEST = 64,
	MF_DECAL = 80,
	MF_ALPHA_MASK = 112,

	//MA_OPAQUE = 0,
	//MA_TRANSPARENT = 1,
	//MA_OVERLAY = 2,
	//MA_WATER = 3,
	//MA_TRACKS = 4,
	//MA_OVERLAY_ZWRITE = 5,
	//MA_TERRAIN = 6,
	//MA_CLOUDS_H5 = 8,
	//MA_ANIM_WATER = 9,
	//MA_SURF = 10,
	//MA_TYPE_MASK = 15,
	//MA_ALPHA_TEST = 16,
	//MA_2SIDED = 32,
	//MA_SELF_ILLUM = 64
};

class IMaterial;
struct SMaterialCreateInfo
{
	CPtr<CFuncBase<CVec4> > pColor;
	CObj<CPtrFuncBase<NGfx::CTexture> > pTexture, pBump, pMirrorTexture, pSpecular, pDetail;
	CObj<CPtrFuncBase<NGfx::CTexture> > pMask;
	CVec4 vAvrgTexColor;
	CObj<CPtrFuncBase<NGfx::CCubeTexture> > pSky;
	float fMetalMirror, fDielMirror;
	int alphaMode;
	int nPriority;
	bool bDoesCastShadow;
	float fSpecPower;
	CVec3 vTranslucentColor;
	float fFloatParam;
	float fDetailScale;
	bool bProjectOnTerrain;
	bool bSelfIllum, b2Sided;
	bool bApplyFog;
	bool bAddPlaced;
	bool bIgnoreZ;
	bool bBackFaceCastShadow;
	bool bReceiveShadow;
	EDynamicType eDynamicType;

	SMaterialCreateInfo() : pColor(0), vAvrgTexColor(1,1,1,1), vTranslucentColor(0,0,0),
		fMetalMirror(0), fDielMirror(0), alphaMode(MF_GENERIC|MF_OPAQUE), bDoesCastShadow(true), nPriority(0),
		fSpecPower(32), fFloatParam(0), fDetailScale(1), bProjectOnTerrain(false), bSelfIllum(false), b2Sided(false), bApplyFog(true), bAddPlaced(false), bIgnoreZ(false), bBackFaceCastShadow(false), bReceiveShadow(true), eDynamicType(DT_DONT_CARE) {}
};
_3DMOTOR_EXPORT IMaterial* CreateMaterial( const SMaterialCreateInfo &m_ );
// make exact decal from decal material for alpha test geometry
IMaterial* GetExactDecal( IMaterial *p );
IMaterial *AttachColor( IMaterial *pSrc, CFuncBase<CVec4> *pColor );

}

