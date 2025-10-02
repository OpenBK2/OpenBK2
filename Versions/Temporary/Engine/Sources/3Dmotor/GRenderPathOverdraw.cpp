#include "StdAfx.h"
#include "GRenderPathOverdraw.h"
#include "GRenderExecute.h"
#include "GfxUtils.h"


namespace NGScene
{
void RenderOverdraw( CTransformStack *pTS, NGfx::CRenderContext *pRC, 
	IRender *pRender, CSceneFragments &scene )
{
	CRenderCmdList lightOps;
	const vector<SRenderFragmentInfo*> &fragments = scene.GetFragments();
	for ( int k = 0; k < fragments.size(); ++k )
	{
		if ( scene.IsFilteredFragment( k ) )
			continue;
		const SRenderFragmentInfo &f = *fragments[k];
		COpGenContext fi( &lightOps.ops, &f );
		fi.AddOperation( RO_SOLID_COLOR, 100, DPM_NONE|STM_INCREMENT, 0, &VNULL4, 0.0f, 0.0f );
	}
	Execute( pRender, pRC, *pTS, lightOps, scene, SLightInfo() );
}

void ColorOverdraw( NGfx::CRenderContext *pRC )
{
	const int N_COLORS = 20;
	const float colors[N_COLORS][3] = 
	{
		{   0.0f/255.0f,   0.0f/255.0f,  85.0f/255.0f },
		{   0.0f/255.0f,   0.0f/255.0f, 170.0f/255.0f },
		{   0.0f/255.0f,   0.0f/255.0f, 255.0f/255.0f },
		{   0.0f/255.0f,  85.0f/255.0f, 255.0f/255.0f },
		{   0.0f/255.0f, 170.0f/255.0f, 255.0f/255.0f },
		{   0.0f/255.0f, 255.0f/255.0f, 255.0f/255.0f },
		{   0.0f/255.0f, 255.0f/255.0f, 170.0f/255.0f },
		{   0.0f/255.0f, 255.0f/255.0f,  85.0f/255.0f },
		{   0.0f/255.0f, 255.0f/255.0f,   0.0f/255.0f },
		{  85.0f/255.0f, 255.0f/255.0f,   0.0f/255.0f },
		{ 170.0f/255.0f, 255.0f/255.0f,   0.0f/255.0f },
		{ 255.0f/255.0f, 255.0f/255.0f,   0.0f/255.0f },
		{ 255.0f/255.0f, 170.0f/255.0f,   0.0f/255.0f },
		{ 255.0f/255.0f, 127.0f/255.0f,   0.0f/255.0f },
		{ 255.0f/255.0f,  85.0f/255.0f,   0.0f/255.0f },
		{ 255.0f/255.0f,   0.0f/255.0f,   0.0f/255.0f },
		{ 255.0f/255.0f,   0.0f/255.0f,  42.0f/255.0f },
		{ 255.0f/255.0f,   0.0f/255.0f,  85.0f/255.0f },
		{ 255.0f/255.0f,   0.0f/255.0f, 170.0f/255.0f },
		{ 255.0f/255.0f,   0.0f/255.0f, 255.0f/255.0f }
	};
	pRC->SetAlphaCombine( NGfx::COMBINE_NONE );
	for ( int k = 0; k < N_COLORS; ++k )
	{
		if ( k == N_COLORS - 1 )
			pRC->SetStencil( NGfx::STENCIL_GREATER, k - 1 );
		else
			pRC->SetStencil( NGfx::STENCIL_TEST, k );
		NGfx::C2DQuadsRenderer qr;
		qr.SetTarget( *pRC, CVec2(1000,1000), NGfx::QRM_SIMPLE );
		CTRect<float> rect( 0, 0, 1000, 1000 );
		NGfx::SPixel8888 color;
		color.dwColor = NGfx::GetDWORDColor( CVec4(colors[k][0], colors[k][1], colors[k][2], 1 ) );
		qr.AddRect( rect, 0, rect, color );
	}
	pRC->SetStencil( NGfx::STENCIL_NONE );
}
}
