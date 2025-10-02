#include "StdAfx.h"
#include "grendermodes.h"
#include "GRenderPathTnl.h"
#include "GRenderLight.h"
#include "GTransparent.h"

namespace NGScene
{

void RenderTnL( CTransformStack *pTS, CTransformStack *pClipTS, NGfx::CRenderContext *pRC, 
	IRender *pRender, CSceneFragments &scene, CTransparentRenderer *pTransp, ETransparentMode trMode, NGfx::CCubeTexture *_pSky )
{
	SRenderPathContext rpc( true, 0, 0 );
	SLightInfo lightInfo;
	if ( trMode != TRM_ONLY )
	{
		CRenderCmdList lightOps;
		const vector<SRenderFragmentInfo*> &fragments = scene.GetFragments();
		for ( int i = 1; i < fragments.size(); ++i )
		{
			if ( scene.IsFilteredFragment( i ) )
				continue;
			const SRenderFragmentInfo &frag = *fragments[i];
			COpGenContext op( &lightOps.ops, &frag );
			frag.pMaterial->AddOperations( &op, &rpc );
		}
		Execute( pRender, pRC, *pTS, lightOps, scene, lightInfo );
	}

	pRender->RenderPostProcess( pTS, pRC );

	// draw transparent stuff
	if ( trMode != TRM_NONE )
	{
		STransparentRenderContext trc( rpc.bTnL, pRC, 0, _pSky, RP_TNL, &rpc, lightInfo );
		pTransp->Render( trc );
	}
}
}
