#include "StdAfx.h"
#include "UI.h"
#include "../3dMotor/RectLayout.h"

CVec2 vScreenRect( 1024, 768 );
CVec2 vScreenOrg( VNULL2 );	// origin of virtual screen


int VirtualToScreenX( float fX )
{
	const float fXScale = vScreenRect.x / 1024.0f;
	return (fX - vScreenOrg.x) * fXScale;
}

int VirtualToScreenY( float fY )
{
	const float fYScale = vScreenRect.y / 768.0f;
	return (fY - vScreenOrg.y) * fYScale;
}

int ScreenToVirtualX( float fX )
{
	const float fXScale = 1024.0f / vScreenRect.x ;
	return fXScale * fX + vScreenOrg.x;
}

int ScreenToVirtualY( float fY )
{
	const float fYScale = 768.0f / vScreenRect.y ;
	return fYScale * fY + vScreenOrg.y;
}

void ScreenToVirtual( const CTPoint<int> &vPos, CTPoint<int> *pScreenPos )
{
	const float fXScale = 1024.0f / vScreenRect.x ;
	const float fYScale = 768.0f / vScreenRect.y;
	pScreenPos->x = fXScale * vPos.x + vScreenOrg.x;
	pScreenPos->y = fYScale * vPos.y + vScreenOrg.y;
}

CVec2 ScreenToVirtual( const CVec2 &vPos )
{
	/*
	const float fXScale = 1024.0f / vScreenRect.x ;
	const float fYScale = 768.0f / vScreenRect.y;
	vPos.x = fXScale * vPos.x + vScreenOrg.x;
	vPos.y = fYScale * vPos.y + vScreenOrg.y;
	return vPos;
	*/
	return CVec2( 1024.0f*vPos.x/vScreenRect.x + vScreenOrg.x, 768.0f*vPos.y/vScreenRect.y + vScreenOrg.y );
}

void ScreenToVirtual( const CVec2 &vPos, CVec2 *pScreenPos )
{
	const float fXScale = 1024.0f / vScreenRect.x ;
	const float fYScale = 768.0f / vScreenRect.y;
	pScreenPos->x = fXScale * vPos.x + vScreenOrg.x;
	pScreenPos->y = fYScale * vPos.y + vScreenOrg.y;
}


void VirtualToScreenX( const float fX, float *pX )
{
	const float fXScale = vScreenRect.x / 1024.0f;
	*pX = (fX - vScreenOrg.x) * fXScale;
}

void VirtualToScreenY( const float fY, float *pY )
{
	const float fYScale = vScreenRect.y / 768.0f;
	*pY = (fY - vScreenOrg.y) * fYScale;
}

void VirtualToScreenX( float *pX )
{
	VirtualToScreenX( *pX, pX );
}

void VirtualToScreenY( float *pY )
{
	VirtualToScreenY( *pY, pY );
}

CRectLayout &VirtualToScreen( class CRectLayout *pRects )
{
	for ( int i = 0; i < pRects->rects.size(); ++i )
	{
		CRectLayout::SRect & rc = pRects->rects[i];

		VirtualToScreenX( rc.fX + rc.fSizeX, &rc.fSizeX );
		VirtualToScreenX( rc.fX, &rc.fX );
		rc.fSizeX -= rc.fX;
		
		VirtualToScreenY( rc.fY + rc.fSizeY, &rc.fSizeY );
		VirtualToScreenY( rc.fY, &rc.fY );
		rc.fSizeY -= rc.fY;
	}
	return *pRects;
}

void VirtualToScreen( const CTRect<float> &src, CTRect<float> *pRes )
{
	VirtualToScreenX( src.x1, &pRes->x1 );
	VirtualToScreenX( src.x2, &pRes->x2 );

	VirtualToScreenY( src.y1, &pRes->y1 );
	VirtualToScreenY( src.y2, &pRes->y2 );
}

CTRect<float> &VirtualToScreen( CTRect<float> *src )
{
	VirtualToScreenX( src->x1, &src->x1 );
	VirtualToScreenX( src->x2, &src->x2 );

	VirtualToScreenY( src->y1, &src->y1 );
	VirtualToScreenY( src->y2, &src->y2 );
	return *src;
}

CTPoint<float> &VirtualToScreen( CTPoint<float> *src )
{
	VirtualToScreenX( src->x, &src->x );
	VirtualToScreenY( src->y, &src->y );
	return *src;
}

CVec2 &VirtualToScreen( CVec2 *src )
{
	VirtualToScreenX( src->x, &src->x );
	VirtualToScreenY( src->y, &src->y );
	return *src;
}

void VirtualToScreen( const CTPoint<float> &src, CTPoint<float> *pRes )
{
	VirtualToScreenX( src.x, &pRes->x );
	VirtualToScreenY( src.y, &pRes->y );
}

BASIC_REGISTER_CLASS( IWindow )
