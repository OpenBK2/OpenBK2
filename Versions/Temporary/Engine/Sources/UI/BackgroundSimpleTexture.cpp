#include "StdAfx.h"
#include "backgroundsimpletexture.h"
#include "UIVisitor.h"
#include "Tools.h"
#include "../3dMotor/DBScene.h"
#include "../3DMotor/RectLayout.h"
#include "UIComponents.h"

REGISTER_SAVELOAD_CLASS( 0x11075B41, CBackgroundSimpleTexture )

void CBackgroundSimpleTexture::Visit( IUIVisitor * pVisitor )
{
	if ( pos.IsEmpty() || !pStats ) 
		return;

	CTPoint<int> size;
	CTRect<float> mem;
	if ( pIcon )
	{
		size = pIcon->GetSize();
		mem = pos;
		int w = pIcon->GetSize().x - pTexture->nWidth;
		int h = pIcon->GetSize().y - pTexture->nHeight;
		pos.Inflate( w/2, h/2 );
	}
	else if ( pTexture )
	{
		size.x = pTexture->nWidth;
		size.y = pTexture->nHeight;
	}

	CRectLayout::SRect rc;
	rc.fX = pos.x1;
	rc.fY = pos.y1;
	rc.sColor = FadeColor( pStats->nColor, GetFadeValue() );

	if ( pTexture )
	{
		rc.fSizeX = Min( float(pos.Width()), float(size.x) );
		rc.fSizeY = Min( float(pos.Height()), float(size.y) );

		NUITools::ApplyTextureAllign( pStats->eTextureX, pos.Width(), size.x, 
			&rc.sTex.rcTexRect.x1, &rc.sTex.rcTexRect.x2, &rc.sTex.rcTexRect.x1 );
		NUITools::ApplyTextureAllign( pStats->eTextureY, pos.Height(), size.y, 
			&rc.sTex.rcTexRect.y1, &rc.sTex.rcTexRect.y2, &rc.sTex.rcTexRect.y1 );
	}
	else
	{
		rc.fSizeX = pos.Width();
		rc.fSizeY = pos.Height();
	}

	CRectLayout rects;
	rects.AddRect( rc.fX, rc.fY, rc.fSizeX, rc.fSizeY, rc.sTex.rcTexRect, rc.sColor );
	VirtualToScreen( &rects );

	if( pIcon )
	{
		pVisitor->VisitUITextureRect( pIcon, 3, rects );			
		pos = mem;
	}
	else
		pVisitor->VisitUIRect( pTexture, 3, rects );
}

void CBackgroundSimpleTexture::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	CBackground::InitByDesc( _pDesc );
	pStats = checked_cast<const NDb::SBackgroundSimpleTexture*>( _pDesc );
	pTexture = pStats->pTexture;
#ifndef _FINALRELEASE
	if ( CHECK_UI_TEXTURES_INSTANT_LOAD != 0 )
	{
		CheckInstantLoadTexture( pTexture );
	}
#endif
	pIcon = 0;
}

void CBackgroundSimpleTexture::SetPos( const CVec2 &vPos, const CVec2 &vSize )
{
	CBackground::SetPos( vPos, vSize );
}

void CBackgroundSimpleTexture::SetTexture( const struct NDb::STexture *_pDesc )
{
	if ( pTexture != 0 )
		pTexture = 0;
	if ( _pDesc != 0 )
		pTexture = checked_cast<const NDb::STexture*>( _pDesc );
#ifndef _FINALRELEASE
	if ( CHECK_UI_TEXTURES_INSTANT_LOAD != 0 )
	{
		CheckInstantLoadTexture( pTexture );
	}
#endif
}

void CBackgroundSimpleTexture::SetOutline( const CDBID &nOutlineType )
{	
	if( !pTexture )
		return;

	if( !pIcon )
	{
		pIcon = new CIconOutliner( nOutlineType );
		pIcon->SetTexture( pTexture );
	}
};

int CBackgroundSimpleTexture::operator&( interface IBinSaver &saver )
{
	saver.Add( 1, static_cast<CBackground*>( this ) );
	saver.Add( 4, &pStats );
	saver.Add( 5, &pTexture );
	saver.Add( 6, &pIcon );
	return 0;
}


