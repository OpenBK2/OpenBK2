#include "stdafx.h"
#include "GPostProcessors.h"
#include "GfxEffects.h"
#include "GfxRender.h"
#include "GRenderCore.h"
#include "Gfx.h"
namespace NGScene
{

static void DoRender( NGfx::CRenderContext *pRC, const std::vector<IPostProcess::SObject> &render )
{
	for ( int k = 0; k < render.size(); ++k )
	{
		const IPostProcess::SObject &obj = render[k];
		SRenderGeometryInfo &info = *obj.pInfo;
		info.pVertices.Refresh();
		info.pTriLists[TLT_GEOM].Refresh();
		pRC->AddPrimitive( info.pVertices->GetValue(), info.pTriLists[TLT_GEOM]->GetValue()[ obj.nIdx ] );
	}
	pRC->Flush();
}

void RenderPostProcess( NGfx::CRenderContext *pRC, const SPostProcessData &data )
{
	// works wrong with fog due to smart_alpha
	NGfx::SEffConstLight eff;
	// post colorer
	for ( SPostProcessData::CColorHash::const_iterator i = data.postColorer.begin(); i != data.postColorer.end(); ++i )
	{
		eff.color = i->first;
		pRC->SetEffect( &eff );
		pRC->SetAlphaCombine( NGfx::COMBINE_SMART_ALPHA );
		pRC->SetDepth( NGfx::DEPTH_NORMAL );
		pRC->SetStencil( NGfx::STENCIL_NONE );
		DoRender( pRC, i->second );
	}
	// occluded
	/*
	if ( NGfx::Is16BitMode() )
		return;
	*/
	pRC->SetEffect( &eff );
	for ( SPostProcessData::CColorHash::const_iterator i = data.occluded.begin(); i != data.occluded.end(); ++i )
	{
		pRC->SetAlphaCombine( NGfx::COMBINE_ZERO_ONE );
		pRC->SetDepth( NGfx::DEPTH_EQUAL );
		pRC->SetStencil( NGfx::STENCIL_WRITE, 1 );
		DoRender( pRC, i->second );
	}
	for ( SPostProcessData::CColorHash::const_iterator i = data.occluded.begin(); i != data.occluded.end(); ++i )
	{
		eff.color = i->first;
		pRC->SetEffect( &eff );
		pRC->SetAlphaCombine( NGfx::COMBINE_SMART_ALPHA );
		pRC->SetDepth( NGfx::DEPTH_INVERSETEST );
		pRC->SetStencil( NGfx::STENCIL_TEST, 0 );
		DoRender( pRC, i->second );
	}
}

// CPostColorer

void CPostColorer::Render( SPostProcessData *pDst, const std::vector<SObject> &render )
{
	pColor.Refresh();
	const CVec4 &vColor = pColor->GetValue();
	std::vector<IPostProcess::SObject> &dst = pDst->postColorer[ vColor ];
	dst.insert( dst.end(), render.begin(), render.end() );
}

// COccludedColorer

void COccludedColorer::Render( SPostProcessData *pDst, const std::vector<SObject> &render )
{
	pColor.Refresh();
	const CVec4 &vColor = pColor->GetValue();
	std::vector<IPostProcess::SObject> &dst = pDst->occluded[ vColor ];
	dst.insert( dst.end(), render.begin(), render.end() );
}

}
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( 0x022a2151, CPostColorer )
REGISTER_SAVELOAD_CLASS( 0x17221B00, COccludedColorer )

