#pragma once
namespace NGfx
{
	class CRenderContext;
}
class CTransformStack;
namespace NGScene
{
class IRender;
class CSceneFragments;

void RenderPolycount( CTransformStack *pTS, NGfx::CRenderContext *pRC, IRender *pRender, CSceneFragments &scene );
}
