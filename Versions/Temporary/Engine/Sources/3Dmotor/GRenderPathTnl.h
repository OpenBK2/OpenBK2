#pragma once

namespace NGfx
{
	class CRenderContext;
	class CCubeTexture;
}
class CTransformStack;
namespace NGScene
{
class IRender;
class CSceneFragments;
struct SParticleLMRenderTargetInfo;
class CDirectionalLight;
class CTransparentRenderer;
enum ETransparentMode : int;

void RenderTnL( CTransformStack *pTS, CTransformStack *pClipTS, NGfx::CRenderContext *pRC, 
	IRender *pRender, CSceneFragments &scene, CTransparentRenderer *pTransp, ETransparentMode trMode, NGfx::CCubeTexture *_pSky );
}

