#include "stdafx.h"
#include "3Dmotor/RectLayout.h"
#include "BackgroundSimpleScallingTexture.h"
#include "UIVisitor.h"
#include "3Dmotor/DBScene.h"
#include "UIComponents.h"

REGISTER_SAVELOAD_CLASS(UI, 0x11075B40, CBackgroundSimpleScallingTexture)


//CBackgroundSimpleScallingTexture


void CBackgroundSimpleScallingTexture::InitByDesc( const struct NDb::SUIDesc *_pDesc )
{
	CBackground::InitByDesc( _pDesc );
	pStats = checked_cast<const NDb::SBackgroundSimpleScallingTexture*>( _pDesc );
	pTexture = pStats->pTexture;
	vSize = pStats->vSize;
#ifndef _FINALRELEASE
	if ( CHECK_UI_TEXTURES_INSTANT_LOAD != 0 )
	{
		CheckInstantLoadTexture( pTexture );
	}
#endif
}

void CBackgroundSimpleScallingTexture::SetTexture( const struct NDb::STexture *_pDesc )
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

void CBackgroundSimpleScallingTexture::Visit( struct IUIVisitor* pVisitor )
{
	if ( pos.IsEmpty() || !pStats ) 
		return;

	CRectLayout rects;
	const CTRect<float> rectTexture = ( pTexture == 0 )?CTRect<float>( 0.0f, 0.0f, 0.0f, 0.0f ) :
		CTRect<float>( 0.0f, 0.0f, vSize.x == 0.0f ? pTexture->nWidth : vSize.x, 
		vSize.y == 0.0f ? pTexture->nHeight : vSize.y );
	rects.AddRect( pos.x1, pos.y1, pos.Width(), pos.Height(), rectTexture, FadeColor( pStats->nColor, GetFadeValue() ) );

	VirtualToScreen( &rects );
	pVisitor->VisitUIRect( pTexture, 3, rects );
}

int CBackgroundSimpleScallingTexture::operator&( struct IBinSaver &saver )
{
	saver.Add( 1, static_cast<CBackground*>( this ) );
	saver.Add( 2, &pStats );
	saver.Add( 3 ,&pTexture );
	saver.Add( 4, &vSize );
	return 0;
}


