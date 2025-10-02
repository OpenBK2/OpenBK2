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

void RenderOverdraw( CTransformStack *pTS, NGfx::CRenderContext *pRC, 
	IRender *pRender, CSceneFragments &scene );
void ColorOverdraw( NGfx::CRenderContext *pRC );
}

