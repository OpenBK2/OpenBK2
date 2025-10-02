#include "StdAfx.h"
#include "backgroundtiledtexture.h"
#include "UIVisitor.h"
#include "UIComponents.h"

REGISTER_SAVELOAD_CLASS(0x11075B42,CBackgroundTiledTexture)


// CBackgroundTiledTexture


void CBackgroundTiledTexture::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	CBackground::InitByDesc( _pDesc );
//	pDesc = checked_cast<const NDb::SBackgroundTiledTexture*>( _pDesc );
	const NDb::SBackgroundTiledTexture *pTmpDesc ( checked_cast<const NDb::SBackgroundTiledTexture*>( _pDesc ) );
	pDesc = pTmpDesc->Duplicate();
#ifndef _FINALRELEASE
	if ( CHECK_UI_TEXTURES_INSTANT_LOAD != 0 )
	{
		if ( pDesc )
			CheckInstantLoadTexture( pDesc->pTexture );
	}
#endif
}

int CBackgroundTiledTexture::operator &( IBinSaver &saver )
{
	saver.Add( 1, &pDesc );
	saver.Add( 2, &layout );
	saver.Add( 11, static_cast<CBackground*>(this) );
	return 0;
}

void CBackgroundTiledTexture::DivideSubrects( const NDb::SSubRect &in, CRectLayout *pLayout )
{
	if ( in.ptSize.x == 0 || in.ptSize.y == 0 ||
		   in.ptSize.x == -4.3160208e+008 || in.ptSize.y == -4.3160208e+008 ) return;

	CRectLayout::SRect sub;
	sub.sColor = FadeColor( pDesc->nColor, GetFadeValue() );

	sub.sTex.rcTexRect.y1 = in.rcMaps.y1;
	for ( float y = in.rcRect.y1; y < in.rcRect.y2; y += in.ptSize.y )
	{
		sub.fY = y;
		if ( y + in.ptSize.y > in.rcRect.y2 )
		{
			sub.fSizeY = in.rcRect.y2 - sub.fY;
			const float k = sub.fSizeY / in.ptSize.y;
			sub.sTex.rcTexRect.y2 = ( in.rcMaps.y1 + k * in.rcMaps.Height() ) ;
		}
		else
		{
			sub.fSizeY = y + in.ptSize.y - sub.fY;
			sub.sTex.rcTexRect.y2 = in.rcMaps.y2;
		}

		sub.sTex.rcTexRect.x1 = ( in.rcMaps.x1 ) ;
		for ( float x = in.rcRect.x1; x < in.rcRect.x2; x += in.ptSize.x )
		{
			sub.fX = x;

			if ( x + in.ptSize.x > in.rcRect.x2 )
			{
				sub.fSizeX = in.rcRect.x2 - sub.fX;
				const float k = sub.fSizeX / in.ptSize.x;
				sub.sTex.rcTexRect.x2 = ( in.rcMaps.x1 + k * in.rcMaps.Width()  );// / in.ptSize.x;
			}
			else
			{
				sub.fSizeX = x + in.ptSize.x - sub.fX;
				sub.sTex.rcTexRect.x2 = (  in.rcMaps.x2 );
			}
			
			CTRect<float> sTexRect( sub.sTex.rcTexRect );
			//swap( sTexRect.y1, sTexRect.y2 );

			pLayout->AddRect( sub.fX, sub.fY, sub.fSizeX, sub.fSizeY, sTexRect, sub.sColor );
		}
	}
}

#define _X(v) CTPoint<float>(v.ptSize.x,0)
#define _Y(v) CTPoint<float>(0,v.ptSize.y)

void CBackgroundTiledTexture::InitBorderAndFill()
{
	// corner elements
	// LT
	pDesc->rLT.rcRect.Set( pos.GetLeftTop(), pos.GetLeftTop() + _X(pDesc->rL) + _Y(pDesc->rT) );
	//RT
	pDesc->rRT.rcRect.Set( pos.GetRightTop() - _X(pDesc->rR), pos.GetRightTop() + _Y(pDesc->rT) );
	//LB
	pDesc->rLB.rcRect.Set( pos.GetLeftBottom() - _Y(pDesc->rB), pos.GetLeftBottom() + _X(pDesc->rL) );
	//RB
	pDesc->rRB.rcRect.Set( pos.GetRightBottom() - _Y(pDesc->rB) - _X(pDesc->rR), pos.GetRightBottom() );
	// border elements
	//T
	pDesc->rT.rcRect.Set( pos.GetLeftTop() + _X(pDesc->rL), pos.GetRightTop() - _X(pDesc->rR) + _Y(pDesc->rT) );
	//B
	pDesc->rB.rcRect.Set( pos.GetLeftBottom() + _X(pDesc->rL) - _Y(pDesc->rB), pos.GetRightBottom() - _X(pDesc->rR) );
	//L
	pDesc->rL.rcRect.Set( pos.GetLeftTop() + _Y(pDesc->rT), pos.GetLeftBottom() - _Y(pDesc->rB) + _X(pDesc->rL) );
	//R
	pDesc->rR.rcRect.Set( pos.GetRightTop() + _Y(pDesc->rT) - _X(pDesc->rR), pos.GetRightBottom() - _Y(pDesc->rB) );
	// inner element
	pDesc->rF.rcRect.Set( pos.GetLeftTop() + _X(pDesc->rL) + _Y(pDesc->rT), pos.GetRightBottom() - _X(pDesc->rR) - _Y(pDesc->rB) );
}

void CBackgroundTiledTexture::SetPos( const CVec2 &vPos, const CVec2 &ptSize )
{
	CBackground::SetPos( vPos, ptSize );
	InitBorderAndFill();
	// recalc layout
	layout.rects.clear();
	// reserve needed size at the beginning.

	DivideSubrects( pDesc->rLT, &layout );
	DivideSubrects( pDesc->rT, &layout );
	DivideSubrects( pDesc->rRT, &layout );
	DivideSubrects( pDesc->rL, &layout );
	DivideSubrects( pDesc->rF, &layout );
	DivideSubrects( pDesc->rR, &layout );
	DivideSubrects( pDesc->rLB, &layout );
	DivideSubrects( pDesc->rB, &layout );
	DivideSubrects( pDesc->rRB, &layout );
}

void CBackgroundTiledTexture::Visit( struct IUIVisitor * pVisitor )
{
	if ( !layout.rects.empty() )
	{
		//CRAP{ IT IS OPTIMISATION RESERVE
		CRectLayout tmp ( layout );
		VirtualToScreen( &tmp );
		pVisitor->VisitUIRect( pDesc->pTexture, 3, tmp );
		//CRAP}
	}
}

CBackgroundTiledTexture::CBackgroundTiledTexture()
{
}


