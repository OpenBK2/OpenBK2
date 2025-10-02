#include "StdAfx.h"
#include "UISpecificB2.h"
#include "ui/commandparam.h"
#include "ui/dbuserinterface.h"
#include "ui/ui.h"
#include "system/dg.h"
#include "Components.h"
#include "UI/UIVisitor.h"
#include "3Dmotor/DBScene.h"

IWindow* AddWindowCopy( IWindow *pParent, const struct NDb::SUIDesc *pDesc )
{
	IWindow *pWnd = Singleton<IUIInitialization>()->CreateWindowFromDesc( pDesc );
	if ( !pWnd )
		return 0;
	if ( pParent )
		pParent->AddChild( pWnd, true );
	pWnd->Init();
	return pWnd;
}

IWindow* AddWindowCopy( IWindow *pParent, const IWindow *pSample )
{
	if ( !pSample )
		return 0;
	return AddWindowCopy( pParent, pSample->GetDesc() );
}

// CTextureRoundSegmentVisitor

CTextureRoundSegmentVisitor::CTextureRoundSegmentVisitor() :
	fStartAngle( 0 ),
	fFinishAngle( FP_2PI ),
	colors( 4, 0xFFFFFFFF )
{
}

void CTextureRoundSegmentVisitor::SetTexture( const NDb::STexture *_pTexture )
{
	pTexture = _pTexture;
}

void CTextureRoundSegmentVisitor::SetColor( DWORD dwColor )
{
	fill( colors.begin(), colors.end(), NGfx::SPixel8888( dwColor ) );
}

void CTextureRoundSegmentVisitor::SetPlacement( const CTRect<float> &_rect )
{
	rect = _rect;
}

void CTextureRoundSegmentVisitor::SetAngles( float _fStartAngle, float _fFinishAngle )
{
	fStartAngle = NormalizeAngleInRadian( _fStartAngle + FP_PI4 ) - FP_PI4;
	fFinishAngle = NormalizeAngleInRadian( _fFinishAngle + FP_PI4 ) - FP_PI4;
}

void CTextureRoundSegmentVisitor::Visit( struct IUIVisitor *pVisitor )
{
	CTRect<float> rectScreen;
	VirtualToScreen( rect, &rectScreen );
	if ( rectScreen.GetSizeX() < 0.5f || rectScreen.GetSizeY() <= 0.5f )
		return;
	float fAspect = rectScreen.GetSizeY() / rectScreen.GetSizeX();
	CVec2 vScreenCenter( rectScreen.GetCenter().x, rectScreen.GetCenter().y );
	CVec2 vScreenHalf( rectScreen.GetSizeX() * 0.5f, rectScreen.GetSizeY() * 0.5f );
	CVec2 vTextureSize( pTexture ? pTexture->nWidth : 0, pTexture ? pTexture->nHeight : 0 );
	CVec2 vTextureCenter( vTextureSize.x / 2.0f, vTextureSize.y / 2.0f );
	CVec2 &vTextureHalf = vTextureCenter;
	CTRect<float> rectTexture( 0.0f, 0.0f, vTextureSize.x, vTextureSize.y );

	float fMinAngle = -FP_PI4;
	float fMaxAngle = FP_PI4;
/*	if ( fStartAngle > fFinishAngle )
	{
		fStartAngle -= FP_2PI;
		fMinAngle -= FP_2PI;
		fMaxAngle -= FP_2PI;
	}*/
	for ( int i = 0; i < 4; ++i )
	{
		float fMinAngle2 = fMinAngle;
		float fMaxAngle2 = fMaxAngle;
		bool bResult = ClampAngles( &fMinAngle2, &fMaxAngle2, fStartAngle, fFinishAngle );
/*		if ( fStartAngle <= fFinishAngle )
		{
			fMinAngle2 = Max( fMinAngle, fStartAngle );
			fMaxAngle2 = Min( fMaxAngle, fFinishAngle );
		}
		else
		{
			fMinAngle2 = Max( fMinAngle, fStartAngle );
			fMaxAngle2 = Min( fMaxAngle, fFinishAngle );
		}*/

//		if ( fStartAngle <= fFinishAngle ? fMinAngle2 < fMaxAngle2 : fMinAngle2 > fMaxAngle2 )
		if ( bResult )
		{
			float fMinCos = cosf( fMinAngle2 );
			float fMinSin = sinf( fMinAngle2 );
			float fMaxCos = cosf( fMaxAngle2 );
			float fMaxSin = sinf( fMaxAngle2 );
			
			CVec2 vScr2;
			CVec2 vScr3;
			CVec2 vTex2;
			CVec2 vTex3;
			
			switch ( i )
			{
				case 0:
				{
					float fMinK = fMinSin / fMinCos;
					float fMaxK = fMaxSin / fMaxCos;
					vScr2 = CVec2( rectScreen.x2, vScreenCenter.y - fMinK * vScreenHalf.x * fAspect );
					vScr3 = CVec2( rectScreen.x2, vScreenCenter.y - fMaxK * vScreenHalf.x * fAspect );
					vTex2 = CVec2( vTextureSize.x, vTextureCenter.y - fMinK * vTextureHalf.x );
					vTex3 = CVec2( vTextureSize.x, vTextureCenter.y - fMaxK * vTextureHalf.x );
				}
				break;
				
				case 1:
				{
					float fMinK = fMinCos / fMinSin;
					float fMaxK = fMaxCos / fMaxSin;
					vScr2 = CVec2( vScreenCenter.x + fMinK * vScreenHalf.y / fAspect, rectScreen.y1 );
					vScr3 = CVec2( vScreenCenter.x + fMaxK * vScreenHalf.y / fAspect, rectScreen.y1 );
					vTex2 = CVec2( vTextureCenter.x + fMinK * vTextureHalf.y, 0 );
					vTex3 = CVec2( vTextureCenter.x + fMaxK * vTextureHalf.y, 0 );
				}
				break;
				
				case 2:
				{
					float fMinK = fMinSin / fMinCos;
					float fMaxK = fMaxSin / fMaxCos;
					vScr2 = CVec2( rectScreen.x1, vScreenCenter.y + fMinK * vScreenHalf.x * fAspect );
					vScr3 = CVec2( rectScreen.x1, vScreenCenter.y + fMaxK * vScreenHalf.x * fAspect );
					vTex2 = CVec2( 0, vTextureCenter.y + fMinK * vTextureHalf.x );
					vTex3 = CVec2( 0, vTextureCenter.y + fMaxK * vTextureHalf.x );
				}
				break;
				
				case 3:
				{
					float fMinK = fMinCos / fMinSin;
					float fMaxK = fMaxCos / fMaxSin;
					vScr2 = CVec2( vScreenCenter.x - fMinK * vScreenHalf.y / fAspect, rectScreen.y2 );
					vScr3 = CVec2( vScreenCenter.x - fMaxK * vScreenHalf.y / fAspect, rectScreen.y2 );
					vTex2 = CVec2( vTextureCenter.x - fMinK * vTextureHalf.y, vTextureSize.y );
					vTex3 = CVec2( vTextureCenter.x - fMaxK * vTextureHalf.y, vTextureSize.y );
				}
				break;
			}
			
			DrawTriangle( pVisitor, vScreenCenter, vScr2, vScr3, vTextureCenter, vTex2, vTex3 );
		}
		fMinAngle += FP_PI2;
		fMaxAngle += FP_PI2;
	}
}

void CTextureRoundSegmentVisitor::DrawTriangle( struct IUIVisitor *pVisitor, const CVec2 &v1, const CVec2 &v2, const CVec2 &v3,
	const CVec2 &vTex1, const CVec2 &vTex2, const CVec2 &vTex3 )
{
	// CRAP - выводится просто цвет без текстуры, сделать вывод с текстурой нетривиально,
	// сейчас такой возможности нет (и нельзя будет пользоваться 2D-текстурой, если совсем не заморочиться)
	// vTex2, vTex3 - для CRAP не используются
	CTRect<float> rectTexture( 1, 1, pTexture ? pTexture->nWidth - 1 : 2, pTexture ? pTexture->nHeight - 1 : 2 );

	CVec2 sPos[4];
	sPos[0] = v1;
	sPos[1] = v2;
	sPos[2] = v3;
	sPos[3] = v3;
	pVisitor->VisitUIRect( pTexture, 3, sPos, &colors[0], rectTexture );
}

bool CTextureRoundSegmentVisitor::ClampAngles( float *pStart, float *pFinish, float fMin, float fMax )
{
	if ( fMin > fMax )
		fMin -= FP_2PI;
	while ( *pStart > fMax )
	{
		*pStart -= FP_2PI;
		*pFinish -= FP_2PI;
		if ( *pFinish < fMin )
			return false;
	}
	while ( *pFinish < fMin )
	{
		*pStart += FP_2PI;
		*pFinish += FP_2PI;
		if ( *pStart > fMax )
			return false;
	}
	*pStart = Max( *pStart, fMin );
	*pFinish = Min( *pFinish, fMax );
	return *pStart < *pFinish;
}


