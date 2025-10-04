#include "stdafx.h"
#include "GRects.h"
#include "GfxUtils.h"

namespace NGScene
{

void RenderRectLayout( NGfx::C2DQuadsRenderer *pRes, NGfx::CTexture *pTex, const CRectLayout &l, float fZ, ELayoutRenderMode lrm )
{
	for ( int i = 0; i < l.rects.size(); ++i )
	{
		const CRectLayout::SRect &r = l.rects[i];
		NGfx::SPixel8888 color = r.sColor;
		if ( lrm == LRM_CLEAR_RECT )
			color.a = 0;

		CTRect<float> rTarget( r.fX, r.fY, r.fX + r.fSizeX, r.fY + r.fSizeY );
		CTRect<float> rSrc( r.sTex.rcTexRect ); 
		pRes->AddRect( rTarget, pTex, rSrc, color, fZ );
	}
}

bool ClipRect( CTRect<float> *pClippedRect, CRectLayout::STextureCoord *pTex, 
	const CRectLayout::SRect &sRect, const CTPoint<float> &sPosition, const CTRect<float> &sWindow )
{
	CTRect<float> &sClippedRect = *pClippedRect;
	CRectLayout::STextureCoord &sTex = *pTex;

	CTRect<float> sSourceRect;
	sSourceRect.x1 = sPosition.x + Min( sRect.fX, sRect.fX + sRect.fSizeX );
	sSourceRect.y1 = sPosition.y + Min( sRect.fY, sRect.fY + sRect.fSizeY );
	sSourceRect.x2 = sPosition.x + Max( sRect.fX, sRect.fX + sRect.fSizeX );
	sSourceRect.y2 = sPosition.y + Max( sRect.fY, sRect.fY + sRect.fSizeY );

	sClippedRect = sSourceRect;
	sClippedRect.x1 = Clamp( sClippedRect.x1, sWindow.x1, sWindow.x2 );
	sClippedRect.x2 = Clamp( sClippedRect.x2, sWindow.x1, sWindow.x2 );
	sClippedRect.y1 = Clamp( sClippedRect.y1, sWindow.y1, sWindow.y2 );
	sClippedRect.y2 = Clamp( sClippedRect.y2, sWindow.y1, sWindow.y2 );
	if ( ( sClippedRect.Width() <= 0 ) || ( sClippedRect.Height() <= 0 ) )
		return false;

	sTex = sRect.sTex;
	float fCSWidth = float( sTex.rcTexRect.Width() ) / sSourceRect.Width();
	float fCSHeight = float( sTex.rcTexRect.Height() ) / sSourceRect.Height();
	sTex.rcTexRect.x1 += fCSWidth * ( sClippedRect.x1 - sSourceRect.x1  );
	sTex.rcTexRect.x2 += fCSWidth * ( sClippedRect.x2 - sSourceRect.x2  );
	sTex.rcTexRect.y1 += fCSHeight * ( sClippedRect.y1 - sSourceRect.y1  );
	sTex.rcTexRect.y2 += fCSHeight * ( sClippedRect.y2 - sSourceRect.y2  );
	return true;
}

void RenderRectLayoutClipped( NGfx::C2DQuadsRenderer *pRes, NGfx::CTexture *pTex, const CRectLayout &sLayout, const CTPoint<float> &sPosition, const CTRect<float> &sWindow, float fZ, ELayoutRenderMode lrm )
{
	for ( int nTemp = 0; nTemp < sLayout.rects.size(); nTemp++ )
	{
		const CRectLayout::SRect &sRect = sLayout.rects[nTemp];
		CTRect<float> sClippedRect;
		CRectLayout::STextureCoord sTex;
		
		if ( !ClipRect( &sClippedRect, &sTex, sRect, sPosition, sWindow ) )
			continue;

		NGfx::SPixel8888 color = sRect.sColor;
		if ( lrm == LRM_CLEAR_RECT )
			color.dwColor = 0;

		pRes->AddRect( sClippedRect, pTex, sTex.rcTexRect, color, fZ );
	}
}

void RenderRect( NGfx::C2DQuadsRenderer *pRes, NGfx::CTexture *pTex, 
	const CVec2 *pPos4, const NGfx::SPixel8888 *pColors4, const CTRect<float> &rSrc, float fZ )
{
	pRes->AddRect( pPos4, pColors4, pTex, rSrc, fZ );
}

} // namespace
using namespace NGScene;

