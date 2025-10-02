#ifndef __GRenderModes_H_
#define __GRenderModes_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

namespace NGfx
{
	class CTexture;
}
namespace NGScene
{

enum ESceneRenderMode
{
	SRM_SHOWOVERDRAW,
	SRM_SHOWLIGHTMAP,
	SRM_BEST,
	SRM_SHOWPOLYCOUNT,
	SRM_LAST
};
enum ERenderPath
{
	RP_TNL,
//	RP_FASTEST,
	RP_GF3_FAST,
//	RP_SHOWOCCLUDERS,
//	RP_GF2,
//	RP_UPDATE_CL,
//	RP_SHOW_PL_OVERDRAW,
	RP_SHOWOVERDRAW,
	RP_SHOWLIGHTMAP,
//	RP_GF2_CL,
//	RP_GF3_CL,
//	RP_R300
	RP_SHOWPOLYCOUNT,
};
//inline bool IsUingRegisters( ERenderPath rp ) { return rp >= RP_GF2; }
//inline bool IsUsingCacheLighting( ERenderPath rp ) { return rp >= RP_UPDATE_CL; }
inline bool IsUsingShadows( ERenderPath rp ) { return rp == RP_GF3_FAST; }

enum EHSRMode
{
	HSR_NONE,
	HSR_FAST,
	HSR_DYNAMIC,
	HSR_LAST
};
enum ETransparentMode
{
	TRM_NONE,
	TRM_NORMAL,
	TRM_ONLY,
	TRM_LAST
};
enum EScenePartsSet
{
	SPS_STATIC = 1,
	SPS_DYNAMIC = 2,
	SPS_ALL = 3
};
enum ELightFlags
{
	LF_NEVER_STATIC = 1,
	LF_USE_DIRECTIONAL_APPROX = 2,
	LF_DO_NOT_CACHE_LIGHT = 4,
	LF_SKIP_LIGHTING = 8,
	LF_DO_NOT_MULTIPLY_ON_TRANSPARENCY = 16
};
enum EGeometryAttributes
{
	GATTR_VERTEX_COLOR = 13
};
enum EAlphaMode2D
{
	AM2D_NORMAL,
	AM2D_PREMUL,
};
enum ELightmapQuality
{
	LM_QUALITY_DRAFT,
	LM_QUALITY_RADIOSITY,
};

enum EShadowsQuality
{
	SQ_SIMPLE,
	SQ_NORMAL,
	SQ_PRE_BEST,
	SQ_BEST
};

struct SRTClearParams
{
	enum EClearType
	{
		CT_NONE,
		CT_ZBUFFER_ONLY,
		CT_FULL
	};
	EClearType ct;
	CVec4 vColor;
	
	SRTClearParams() : ct(CT_FULL), vColor(0,0,0,1) {}
};

inline SBound MakeLargeHintBound() { SBound b; b.SphereInit( CVec3(0,0,0), 1e6 ); return b; }
}
#endif
