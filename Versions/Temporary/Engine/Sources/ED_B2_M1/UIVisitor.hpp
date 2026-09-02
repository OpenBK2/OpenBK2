#pragma once


#include "3Dmotor/G2DView.h"
#include "UI/UIVisitor.h"
#include "UI/UIML.h"
#include "UI/UI.h"

class CUIVisitor : public IUIVisitor
{
	CObj<NGScene::I2DGameView> p2DView;

	CTRect<float> rcClip;
	std::list< CTRect<float> > rcStoredClip;
	bool bClipApplied;

	virtual void ClipRestore()
	{
		NI_ASSERT( !rcStoredClip.empty(), "restore empty clip" );
		rcClip = rcStoredClip.back();
		rcStoredClip.pop_back();
		bClipApplied = !rcStoredClip.empty();
	}

	virtual void ClipSet( const CTRect<float> &_rClip )
	{
		rcStoredClip.push_back( rcClip );
		rcClip.Intersect(_rClip);
		bClipApplied = true;
	}

public:
	void SetG2DView( NGScene::I2DGameView *_p2DView, CVec2 vScreen ) 
	{
		Singleton<IUIInitialization>()->GetVirtualScreenController()->SetResolution( vScreen.x, vScreen.y );

		const CVec2 vScreenSize( _p2DView->GetViewportSize() );
		p2DView = _p2DView;
		rcClip.x1 = 0;
		rcClip.x2 = vScreenSize.x;
		rcClip.y1 = 0;
		rcClip.y2 = vScreenSize.y;
		bClipApplied = false;
	}
	void VisitZClearRect( const CRectLayout &_sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sClipWindow, float fZ=1.0f )
	{
		CRectLayout sLayout = _sLayout;
		sLayout.fZ = fZ;
		p2DView->CreateDynamicClearRects( sLayout, sPosition, sClipWindow );
	}

	void VisitUIRect( const NDb::STexture *pTexture, const int nShadingEffect, const class CRectLayout &rects )
	{
		// recalc to new size
		p2DView->CreateDynamicRects( pTexture, rects, CTPoint<float>( 0,0 ), rcClip );
	}
	void VisitUITextureRect( CPtrFuncBase<NGfx::CTexture> *pTexture, const int nShadingEffect, const class CRectLayout &rects )
	{
		p2DView->CreateDynamicRects( pTexture, rects, CTPoint<float>( 0,0 ), rcClip );
	}
	void VisitUIRect( const NDb::STexture *pTexture, const int nShadingEffect, 
		const CVec2 *pPos4, const NGfx::SPixel8888 *pColors4, const CTRect<float> &rectTexture )
	{
		ASSERT( 0 ); // not implemented
	}
	void VisitUIText( IML *pML, const CTPoint<float> &sPosition, const CTRect<float> &sWindow )
	{
		if ( bClipApplied )
		{
			CTRect<float> clipRect = sWindow;
			clipRect.Intersect( rcClip );
			pML->Render( p2DView, sPosition, clipRect );
		}
		else
		{
			pML->Render( p2DView, sPosition, sWindow );
		}
	}
};



