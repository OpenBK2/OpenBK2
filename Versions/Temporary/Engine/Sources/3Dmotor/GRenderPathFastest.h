#pragma once

namespace NGfx
{
	class CRenderContext;
	class CTexture;
	class CCubeTexture;
}

class CTransformStack;

namespace NGScene
{

class IRender;
class CSceneFragments;
struct SParticleLMRenderTargetInfo;
class CDirectionalLight;
struct SRTClearParams;
class CTransparentRenderer;
enum ETransparentMode : int;

void RenderGf3Fast( CTransformStack *pTS, CTransformStack *pClipTS, NGfx::CRenderContext *pRC, 
	IRender *pRender, CSceneFragments &scene, const SParticleLMRenderTargetInfo &particleLM,
	const SRTClearParams &rtClear, CDirectionalLight *pLight, float fWarFogBlend, int nLightingOptions,
	CTransparentRenderer *pTransp, ETransparentMode trMode,
	NGfx::CTexture *pParticleLight, NGfx::CCubeTexture *_pSky );

}


