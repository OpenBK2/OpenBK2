
#pragma once
#include "GRenderCore.h"

class CTransformStack;

namespace NGScene
{

struct SLightInfo
{
	bool bNeedSet;
	CVec3 vGlossColor, vAmbientColor, vLightColor, vShadeColor, vIncidentShadeColor;
	CVec4 vLightPos, vRadius;
	float fWarFogBlend;

	SLightInfo(): bNeedSet(false), vLightColor(VNULL3), vGlossColor(VNULL3), 
		vLightPos(VNULL4), vRadius(1,1,1,1), vAmbientColor(0,0,0), fWarFogBlend(0),
		vShadeColor(VNULL3), vIncidentShadeColor(VNULL3)
	{}
};

void Execute( IRender *pRender, NGfx::CRenderContext *pRC, const CTransformStack &ts, const CRenderCmdList &cl,
	const CSceneFragments &scene, const SLightInfo &lightInfo );

void StartRenderExecute( NGfx::CRenderContext *pRC, const SLightInfo &light );

void SetupNLShadowsProjection( NGfx::CRenderContext *pRC, const SPerspDirectionalDepthInfo &depthInfo );

}

