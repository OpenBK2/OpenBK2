#include "StdAfx.h"
#include "GRenderPathPolycount.h"
#include "GRenderExecute.h"

namespace NGScene
{
void RenderPolycount( CTransformStack *pTS, NGfx::CRenderContext *pRC, IRender *pRender, CSceneFragments &scene )
{
	CRenderCmdList lightOps;
	const vector<SRenderFragmentInfo*> &fragments = scene.GetFragments();
	vector<CVec4> colors;
	colors.resize( fragments.size(), CVec4(0,0,0,0) );
	for ( int k = 0; k < fragments.size(); ++k )
	{
		if ( scene.IsFilteredFragment( k ) )
			continue;
		const SRenderFragmentInfo &f = *fragments[k];

		float fFade = f.vars.fFade;
		CVec4 &vColor = colors[k];
		fFade = sqrt( fFade );
		vColor = CVec4(1,0,0,1) * ( 1 - fFade ) + CVec4(0,0,1,1) * fFade;

		COpGenContext fi( &lightOps.ops, &f );
		fi.AddOperation( RO_SOLID_COLOR, 100, 0, 0, &vColor, 0.0f, 0.0f );
	}
	Execute( pRender, pRC, *pTS, lightOps, scene, SLightInfo() );
}
}
