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
void RenderShowLightmap( CTransformStack *pTS, NGfx::CRenderContext *pRC,
	IRender *pRender, const CSceneFragments &scene );
}
