#include "stdafx.h"
#include "GRenderPathLightmap.h"
#include "GRenderExecute.h"

namespace NGScene
{
void RenderShowLightmap( CTransformStack *pTS, NGfx::CRenderContext *pRC,
	IRender *pRender, const CSceneFragments &scene )
{
	SLightInfo lightInfo;
	CRenderCmdList lightOps;
	const std::vector<SRenderFragmentInfo*> &fragments = scene.GetFragments();
	for ( int i = 1; i < fragments.size(); ++i )
	{
		if ( scene.IsFilteredFragment( i ) )
			continue;
		const SRenderFragmentInfo &frag = *fragments[i];
		COpGenContext op( &lightOps.ops, &frag );
		if ( !frag.pMaterial->DoesSupportLightmaps() )
			continue;
		if ( !frag.vars.pLM )
			continue;
		CDGPtr<CPtrFuncBase<NGfx::CTexture> > pTex = frag.vars.pLM.GetPtr();
		pTex.Refresh();
		op.AddOperation( RO_G3_SHOW_LM, 10, 0, 0, pTex->GetValue() );
	}
	Execute( pRender, pRC, *pTS, lightOps, scene, lightInfo );
}
}

