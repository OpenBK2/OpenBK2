#include "stdafx.h"
#include "GRects.h"
#include "2DScene.h"
#include "GfxUtils.h"
#include "GRenderModes.h"

namespace NGScene
{

class C2DScene: public I2DScene
{
	OBJECT_NOCOPY_METHODS(C2DScene);

	NGfx::C2DQuadsRenderer quadRender, quadRenderTest;
	NGfx::C2DQuadsRenderer *pLastQR;

	NGfx::C2DQuadsRenderer *GetQR( bool bDepthTest )
	{
		NGfx::C2DQuadsRenderer *pQR = bDepthTest ? &quadRenderTest : &quadRender;
		if ( pLastQR != pQR && pLastQR != 0 )
			pLastQR->Flush();
		pLastQR = pQR;
		return pQR;
	}
public:
	C2DScene() : pLastQR(0) {}
	void CreateDynamicRects( CPtrFuncBase<NGfx::CTexture> *pTexture, const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sClipWindow );
	void CreateDynamicRects( CPtrFuncBase<NGfx::CTexture> *pTexture, 
		const CVec2 *pPos4, const NGfx::SPixel8888 *pColors4, const CTRect<float> &rectTexture );
	void CreateDynamicClearRects( const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sClipWindow );

	void StartNewFrame( NGfx::CTexture *pTarget, const CVec2 &vSize, EAlphaMode2D _AlphaMode2D );
	void Flush();
};

void C2DScene::CreateDynamicRects( CPtrFuncBase<NGfx::CTexture> *pTexture, const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sWindow )
{
	float fZ = sLayout.fZ;
	NGfx::C2DQuadsRenderer *pQR = GetQR( fZ != 0 );
	if ( pTexture )
	{
		CDGPtr<CPtrFuncBase<NGfx::CTexture> > pTex( pTexture );
		pTex.Refresh();
		RenderRectLayoutClipped( pQR, pTex->GetValue(), sLayout, sPosition, sWindow, fZ, LRM_NORMAL );
	}
	else
		RenderRectLayoutClipped( pQR, 0, sLayout, sPosition, sWindow, fZ, LRM_NORMAL );
}

void C2DScene::CreateDynamicRects( CPtrFuncBase<NGfx::CTexture> *pTexture, 
	const CVec2 *pPos4, const NGfx::SPixel8888 *pColors4, const CTRect<float> &rectTexture )
{
	NGfx::C2DQuadsRenderer *pQR = GetQR( false );
	if ( pTexture )
	{
		CDGPtr<CPtrFuncBase<NGfx::CTexture> > pTex( pTexture );
		pTex.Refresh();
		RenderRect( pQR, pTex->GetValue(), pPos4, pColors4, rectTexture, 0.0f );
	}
	else
		RenderRect( pQR, 0, pPos4, pColors4, rectTexture, 0.0f );
}

void C2DScene::CreateDynamicClearRects( const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sWindow )
{
	NGfx::C2DQuadsRenderer *pQR = GetQR( false );
	RenderRectLayoutClipped( pQR, 0, sLayout, sPosition, sWindow, sLayout.fZ, LRM_CLEAR_RECT );
}

void C2DScene::StartNewFrame( NGfx::CTexture *pTarget, const CVec2 &vSize, EAlphaMode2D _AlphaMode2D )
{
	NGfx::CRenderContext rc;
	if ( pTarget ) 
		rc.SetTextureRT( pTarget, 0 );
	if ( _AlphaMode2D == AM2D_NORMAL )
		rc.SetAlphaCombine( NGfx::COMBINE_ALPHA );
	else
	if ( _AlphaMode2D == AM2D_PREMUL )
		rc.SetAlphaCombine( NGfx::COMBINE_SMART_ALPHA );
	rc.SetStencil( NGfx::STENCIL_NONE );
	quadRender.SetTarget( rc, vSize, NGfx::QRM_OVERWRITE );
	quadRenderTest.SetTarget( rc, vSize, NGfx::QRM_DEPTH_NORMAL );
}

void C2DScene::Flush()
{
	quadRender.Flush();
}

// Make scene

I2DScene* Make2DScene()
{
	return new C2DScene;
}

} // NAMESOACE
using namespace NGScene;
REGISTER_SAVELOAD_CLASS( _3DMOTOR, 0xF2005171, C2DScene );

