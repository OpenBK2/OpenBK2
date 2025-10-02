#include "StdAfx.h"

#include "WindowMiniMap.h"
#include "../3DMotor/RectLayout.h"
#include "../Misc/Bresenham.h"
#include "../Misc/GeomMisc.h"
#include "../System/FastMath.h"
#include "../UI/UIVisitor.h"
#include "../Misc/PlaneGeometry.h"
#include "../Main/GameTimer.h"
#include "../System/Commands.h"

REGISTER_SAVELOAD_CLASS(0x1508EAC0, CWindowMiniMap);
REGISTER_SAVELOAD_CLASS(0x15099C00, CMiniMapLayer);

const float F_ANGLE_DELTA = 0.001f;
const float F_BIG_UNIT_RADIUS = 5.0f;
static int s_nDarkWarFogAlpha = 128;
static int s_nLightWarFogAlpha = 0;

struct SMatrix2
{
	float m[2][2];
	
	SMatrix2( float fAngle )
	{
		float fSin = sin( fAngle );
		float fCos = cos( fAngle );

		m[0][0] = fCos; 
		m[0][1] = -fSin; 
		m[1][0] = fSin; 
		m[1][1] = fCos; 
	}
	
	CVec2 Rotate( const CVec2 &vPos ) const
	{
		return CVec2( m[0][0] * vPos.x + m[0][1] * vPos.y, m[1][0] * vPos.x + m[1][1] * vPos.y );
	}
};

bool IsSegmentsIntersected( const CVec2 &p11, const CVec2 &p12, const CVec2 &p21, const CVec2 &p22 )
{
	const float D = (p12.x - p11.x)*(p22.y - p21.y) - (p12.y - p11.y)*(p22.x - p21.x);
	if ( D == 0.0f )
		return false;
	else
	{
		const float d1 = (p22.y - p21.y)*(p21.x - p11.x) - (p22.x - p21.x)*(p21.y - p11.y);
		const float t1 = d1/D;
		if ( t1 > 0.0f && t1 < 1.0f )
		{
			const float d2 = (p12.y - p11.y)*(p21.x - p11.x) - (p12.x - p11.x)*(p21.y - p11.y);
			const float t2 = d2/D;
			return ( t2 > 0.0f && t2 < 1.0f );
		}
		return false;
	}
}

// ************************************************************************************************************************ //
// **
// ** CMiniMapLayer
// **
// **
// **
// ************************************************************************************************************************ //

bool CMiniMapLayer::NeedUpdate()
{ 
	const int nTime = Singleton<IGameTimer>()->GetAbsTime();
	if ( bNeedUpdate && ( ( nTime < nLastUpdateTime ) || ( nTime > nLastUpdateTime + 300 ) ) )	 // Ну а зачем минимап обновлять чаще?
	{
		nLastUpdateTime = nTime;
		return true; 
	}
	else
		return false;
}

void CMiniMapLayer::Recalc()
{
	if ( nWidth > 0 && nHeight > 0 )
	{
		if ( !IsValid( pValue ) )
			pValue = NGfx::MakeTexture( GetNextPow2( nWidth ), GetNextPow2( nHeight ), 1, NGfx::SPixel8888::ID, NGfx::DYNAMIC_TEXTURE, NGfx::CLAMP );

		NGfx::CTextureLock<NGfx::SPixel8888> lock( pValue, 0, NGfx::INPLACE );
		const int nSize = sizeof( NGfx::SPixel8888 ) * nWidth;
		for ( int y = 0; y < nHeight; ++y ) 
			memcpy( lock[y], &points[y][0], nSize );
	}

	bNeedUpdate = false;
}

void CMiniMapLayer::SetSize( const int _nWidth, const int _nHeight )
{
	nWidth = _nWidth;
	nHeight = _nHeight;
	points.SetSizes( nWidth, nHeight );
	pValue = 0;
	bNeedUpdate = true;
}

void CMiniMapLayer::PutPixel( const int x, const int y, const NGfx::SPixel8888 &color )
{
	if ( y < points.GetSizeY() && x < points.GetSizeX() )
		points[y][x] = color;
}

const NGfx::SPixel8888 CMiniMapLayer::GetPixel( const int x, const int y )
{
	if ( x >= 0 && y >= 0 && x < points.GetSizeX() && y < points.GetSizeY() )
		return points[y][x];
	else
    return NGfx::SPixel8888( 0, 0, 0, 0 );
}

void CMiniMapLayer::Clear()
{
	points.FillEvery( NGfx::SPixel8888( 0, 0, 0, 0 ) );
}

int CMiniMapLayer::operator&( IBinSaver &saver )
{
	if ( NGlobal::GetVar( "m1", 0 ).GetFloat() == 0 )
		saver.Add( 1, &points );

	saver.Add( 2, &nWidth );
	saver.Add( 3, &nHeight );
	saver.Add( 4, &bNeedUpdate );

	if ( saver.IsReading() )
	{
		nLastUpdateTime = 0;
		bNeedUpdate = true;
		if ( NGlobal::GetVar( "m1", 0 ).GetFloat() != 0 )
		{
			pValue = 0;
			points.SetSizes( nWidth, nHeight );
			Clear();
		}
	}

	return 0;
}

// ************************************************************************************************************************ //
// **
// ** CWindowMiniMap
// **
// **
// **
// ************************************************************************************************************************ //

CWindowMiniMap::CWindowMiniMap() : 
	bButtonDown( 4, false ), 
	bNeedScreenToMapRefresh( true ), 
	vOldDir( VNULL2 ), 
	fAspect( 1.0f ),
	vLastMousePos( VNULL2 )
{
	AddObserver( "mission_win_mouse_move_emit", &CWindowMiniMap::MsgMouseMoveEmit );
}

void CWindowMiniMap::LoadMap( const int nWidth, const int nHeight, const int _nWarFogLevel )
{
	bNeedScreenToMapRefresh = true;
	vOldDir = VNULL2;
	fMapWidth = nWidth;
	fMapHeight = nHeight;
	fAspect = fMapWidth/fMapHeight;
	const int nWarFogLevel = Max( 2, _nWarFogLevel );
	warFogColors.resize( nWarFogLevel );
	for ( int i = 0; i < nWarFogLevel; ++i )
		warFogColors[i] = NGfx::SPixel8888( 0, 0, 0, s_nDarkWarFogAlpha - i*(s_nDarkWarFogAlpha - s_nLightWarFogAlpha)/(nWarFogLevel - 1) );

	// matrix calculation
	if ( !bRotable )
	{
		MakeTransformParams( pShared->vPoint00, pShared->vPoint01, pShared->vPoint10, pShared->vPoint11 );
	}
	else
	{
		SetBaseRotableParams();
		MakeRotableTransformParams( CVec2( 1.0f, 0.0f ) );
	}

	MakeMaskMimimap();
}

void CWindowMiniMap::SetBaseRotableParams()
{
	// base rectangle
	CVec2 vMin( Min( pShared->vPoint00.x, Min( pShared->vPoint01.x, Min( pShared->vPoint10.x, pShared->vPoint11.x ) ) ),
		Min( pShared->vPoint00.y, Min( pShared->vPoint01.y, Min( pShared->vPoint10.y, pShared->vPoint11.y ) ) ) );
	CVec2 vMax( Max( pShared->vPoint00.x, Max( pShared->vPoint01.x, Max( pShared->vPoint10.x, pShared->vPoint11.x ) ) ),
		Max( pShared->vPoint00.y, Max( pShared->vPoint01.y, Max( pShared->vPoint10.y, pShared->vPoint11.y ) ) ) );

	if ( NGlobal::GetVar( "m1", 0 ).GetFloat() == 0 )
	{
		vRotableCenter = (vMin + vMax) * 0.5f;
	}
	else
	{
		float a1 = ( vMax.y - vMin.y ) * 0.5f;
		float a2 = ( vMax.x - vMin.x ) * 0.5f;
		float maxsize = sqrt( 2.0f ) * Max( a1, a2 );

		vRotableCenter.x = sqrt( 2.0f ) * a2;
		vRotableCenter.y = sqrt( 2.0f ) * a1;
	}
	if ( vRotableCenter.x == 0.0f && vRotableCenter.y == 0.0f )	
	{
		vRotableCenter = vSize * 0.5f;
	}

	if ( pShared->vRotableSize.x > 0 && pShared->vRotableSize.y > 0 )
		vRotableSize = pShared->vRotableSize;
	else
	{
		vRotableSize = CVec2( vMax.x - vMin.x, vMax.y - vMin.y );
		if ( vRotableSize.x == 0.0f && vRotableSize.y == 0.0f )	
			vRotableSize = vSize / FP_SQRT_2;
	}
}

CVec2 CWindowMiniMap::GetCWindowSize() const
{
	return CVec2( CWindow::GetInstance()->placement.size.Get() );
}

bool CWindowMiniMap::CalculateScreenToMapSlow( const CVec2 &vPos, CVec2 *pvRes )
{
	bool bResult = vPos.x >= 0.0f && vPos.y >= 0.0f && vPos.x < vSize.x && vPos.y < vSize.y;

	if ( !bRotable )
	{
		pvRes->x = mScreenToMap1[0][0] * vPos.x + mScreenToMap1[0][1] * vPos.y + mScreenToMap1[0][2];
		pvRes->y = mScreenToMap1[1][0] * vPos.x + mScreenToMap1[1][1] * vPos.y + mScreenToMap1[1][2];
		if ( pvRes->x + pvRes->y >= 1.0f )
		{
			pvRes->x = mScreenToMap2[0][0] * vPos.x + mScreenToMap2[0][1] * vPos.y + mScreenToMap2[0][2];
			pvRes->y = mScreenToMap2[1][0] * vPos.x + mScreenToMap2[1][1] * vPos.y + mScreenToMap2[1][2];
		}
	}
	else
	{
		*pvRes = rotableTransform.TransformBack( vPos );
	}

	if ( fAspect < 1.0f )
	{
		const float fDx = ( 1.0f - fAspect ) * 0.5f;
		pvRes->x = ( pvRes->x - fDx * 1.0f )/fAspect;
	}
	else if ( fAspect > 1.0f ) 
	{
		const float fDy = ( 1.0f - 1.0f / fAspect ) * 0.5f;
		pvRes->y = ( pvRes->y - fDy * 1.0f )*fAspect;
	}

	if ( bResult )
		bResult = ( pvRes->x >= 0.0f && pvRes->x <= 1.0f && pvRes->y >= 0.0f && pvRes->y <= 1.0f );

	pvRes->x = Clamp( pvRes->x, 0.0f, 1.0f ) * fMapWidth;
	pvRes->y = Clamp( pvRes->y, 0.0f, 1.0f ) * fMapHeight;
	return bResult;
}

bool CWindowMiniMap::MapToScreen( const CVec2 &vMapPos, CVec2 *pvRes ) const
{
	bool bResult = ( vMapPos.x >= 0.0f && vMapPos.y >= 0.0f &&
		vMapPos.x <= 1.0f && vMapPos.y <= 1.0f );

	//	CVec2 vPos( Clamp( vMapPos.x/fMapWidth, 0.0f, 1.0f ), Clamp( vMapPos.y/fMapHeight, 0.0f, 1.0f ) );
	CVec2 vPos( vMapPos.x/fMapWidth, vMapPos.y/fMapHeight );

	if ( fAspect < 1.0f )
	{
		const float fDx = (1 - fAspect)/2;
		vPos.x = fDx + vPos.x*( 1 - 2*fDx );
	}
	else if ( fAspect > 1.0f )
	{
		const float fDy = (1 - 1/fAspect)/2;
		vPos.y = fDy + vPos.y*( 1 - 2*fDy );
	}

	if ( !bRotable )
	{
		if ( vPos.x + vPos.y < 1.0f )
		{
			pvRes->x = mMapToScreen1[0][0] * vPos.x + mMapToScreen1[0][1] * vPos.y + mMapToScreen1[0][2];
			pvRes->y = mMapToScreen1[1][0] * vPos.x + mMapToScreen1[1][1] * vPos.y + mMapToScreen1[1][2];
		}
		else
		{
			pvRes->x = mMapToScreen2[0][0] * vPos.x + mMapToScreen2[0][1] * vPos.y + mMapToScreen2[0][2];
			pvRes->y = mMapToScreen2[1][0] * vPos.x + mMapToScreen2[1][1] * vPos.y + mMapToScreen2[1][2];
		}
	}
	else
	{
		*pvRes = rotableTransform.Transform( vPos );
	}

	return bResult;
}

bool CWindowMiniMap::ScreenToMap( const CVec2 &vPos, CVec2 *pvRes )
{
	if ( bNeedScreenToMapRefresh )
	{
		bNeedScreenToMapRefresh = false;
		
		const int nSizeX = GetInstance()->placement.size.Get().x;
		const int nSizeY = GetInstance()->placement.size.Get().y;
		screenToMap.SetSizes( nSizeX, nSizeY );

		for ( int y = 0; y < nSizeY; ++y )
			for ( int x = 0; x < nSizeX; ++x )
				screenToMap[y][x].second = CalculateScreenToMapSlow( CVec2( x, y ), &(screenToMap[y][x].first) );
	}

	if ( vPos.x < screenToMap.GetSizeX() && vPos.y < screenToMap.GetSizeY() && vPos.x >= 0 && vPos.y >= 0 )
	{
		const pair<CVec2, bool> & result = screenToMap[(int)vPos.y][(int)vPos.x];
		*pvRes = result.first;
		return result.second;
	}
	else
		return false;
}

void CWindowMiniMap::RadiusMapToScreen( const CVec2 &vMapPos, CVec2 *pvRes )
{
	CVec2 vPos( vMapPos.x/fMapWidth, vMapPos.y/fMapHeight );

	if ( fAspect < 1.0f )
	{
		const float fDx = (1 - fAspect)/2;
		vPos.x = fDx + vPos.x*( 1 - 2*fDx );
	}
	else if ( fAspect > 1.0f )
	{
		const float fDy = (1 - 1/fAspect)/2;
		vPos.y = fDy + vPos.y*( 1 - 2*fDy );
	}
	
	if ( !bRotable )
	{
		pvRes->x = mMapToScreen1[0][0] * vPos.x + mMapToScreen1[0][1] * vPos.y;
		pvRes->y = mMapToScreen1[1][0] * vPos.x + mMapToScreen1[1][1] * vPos.y;
	}
	else
	{
//		*pvRes = rotableTransform.Transform( vPos ) - rotableTransform.Transform( VNULL2 );
		float f = rotableTransform.TransformRadius( (vPos.x + vPos.y) * 0.5f );
		pvRes->x = f;
		pvRes->y = f;
	}
}

float CWindowMiniMap::RadiusMapToScreen( float fRadius )
{
	CVec2 v;
	RadiusMapToScreen( CVec2( fRadius, fRadius ), &v );
	return (v.x + v.y) * 0.5f;
}

void CWindowMiniMap::SetNortDirectionTexture( const NDb::STexture *pTexture )
{
	pRotableNorthPointTexture = pTexture;
}

void CWindowMiniMap::InitByDesc( const struct NDb::SUIDesc *pDesc )
{
	const NDb::SWindowMiniMap *pMiniMapDesc( checked_cast<const NDb::SWindowMiniMap*>( pDesc ) );
	pInstance = pMiniMapDesc->Duplicate();
	CWindow::InitByDesc( pDesc );

	bNeedScreenToMapRefresh = true;
	vOldDir = VNULL2;
	
	pShared = checked_cast_ptr<const NDb::SWindowMiniMapShared *>( pMiniMapDesc->pShared );
	bRotable = pShared->bRotable;
	fPrevRotableAngle = -1.0f;
	if ( bRotable )
	{
		pRotableBackgroundTexture = pShared->pRotableBackgroundTexture;
		vRotableBackgroundSize = pShared->vRotableBackgroundSize;
		if ( pShared->vRotableSize.x > 0 && pShared->vRotableSize.y > 0 )
			vRotableNorhtPointSize = vRotableSize = pShared->vRotableSize;
		pRotableForegroundTexture = pShared->pRotableForegroundTexture;
	}
	
	bButtonDown.clear();
	bButtonDown.resize( 4, false );

	const int nSizeX = pInstance->placement.size.Get().x;
	const int nSizeY = pInstance->placement.size.Get().y;

/*
	for ( int i = 0; i < pShared->playerColors.size(); ++i )
		playerColors.push_back( NGfx::SPixel8888( pShared->playerColors[i].r*255.0f, pShared->playerColors[i].g*255.0f, pShared->playerColors[i].b*255.0f, 255 ) );
*/
	frameColor = NGfx::SPixel8888( pShared->vViewportFrameColor.r*255.0f, pShared->vViewportFrameColor.g*255.0f, pShared->vViewportFrameColor.b*255.0f, 255 );

	pUnitsLayer = new CMiniMapLayer();
	pUnitsLayer->SetSize( nSizeX, nSizeY );
	pUnitsLayer->Clear();

	pViewPortLayer = new CMiniMapLayer();
	pViewPortLayer->SetSize( nSizeX, nSizeY );
	pViewPortLayer->Clear();
	
	pFiguresLayer = new CMiniMapLayer();
	pFiguresLayer->SetSize( nSizeX, nSizeY );
	pFiguresLayer->Clear();

	pWarFogLayer = new CMiniMapLayer();
	pWarFogLayer->SetSize( nSizeX, nSizeY );
	pWarFogLayer->Clear();
	
	pRangesLayer = new CMiniMapLayer();
	pRangesLayer->SetSize( nSizeX, nSizeY );
	pRangesLayer->Clear();
	bRangesPresent = false;
	vSize = GetCWindowSize();
	
	vRotableCenter = VNULL2;
	vRotableSize = VNULL2;
}

void CWindowMiniMap::MakeMaskMimimap()
{
	const int nSizeX = pInstance->placement.size.Get().x;
	const int nSizeY = pInstance->placement.size.Get().y;

	maskMiniMap.SetSizes( nSizeX, nSizeY );
	maskMiniMap.FillZero();
	// FIXIT{ may be here we need maskMiniMap.SetData( vPos.x, vPos.y ) ?
	for ( int x = 0; x < nSizeX; ++x )
	{
		for ( int y = 0; y < nSizeY; ++y )
		{
			CVec2 vPos;
			if ( ScreenToMap( CVec2( x, y ), &vPos ) )
			{
				maskMiniMap.SetData( x, y );
			}
		}
	}
	// FIXIT}
}

void CWindowMiniMap::SetAdditionalScale( float _fAdditionalScale )
{
	fAdditionalScale = _fAdditionalScale;
}

void CWindowMiniMap::UpdateWarFog()
{
	pWarFogLayer->Clear();
	if ( warFog.IsEmpty() )
		return;

	const int nSizeX = pInstance->placement.size.Get().x;
	const int nSizeY = pInstance->placement.size.Get().y;
	const float fX = ((float)warFog.GetSizeX() - 1.0f ) / fMapWidth;
	const float fY = ((float)warFog.GetSizeY() - 1.0f ) / fMapHeight;
	for ( int x = 0; x < nSizeX; ++x )
	{
		for ( int y = 0; y < nSizeY; ++y )
		{
			CVec2 vPos;
			if ( maskMiniMap.GetData( x, y ) && ScreenToMap( CVec2( x, y ), &vPos ) )
			{
				const int nFogX = vPos.x * fX;
				const int nFogY = vPos.y * fY;
				const BYTE color = warFog[nFogY][nFogX];
				pWarFogLayer->PutPixel( x, y, warFogColors[color] );
			}
		}
	}
	pWarFogLayer->SetNeedUpdate();
}

void CWindowMiniMap::Visit( interface IUIVisitor *pVisitor )
{
	if ( !IsVisible() )
		return;

	CWindow::Visit( pVisitor );

	CTRect<float> rect = GetWindowRect();
	CVec2 vPos( rect.x1, rect.y1 );
	CVec2 vSize( rect.GetSizeX(), rect.GetSizeY() );
	CVec2 sBackgroundPos[4];
	NGfx::SPixel8888 sBackgroundColors[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
	CTRect<float> rectTexture;

	if ( const NDb::STexture *pTmpTexture = pMaterial ? pMaterial->pTexture : pTexture )
	{
		if ( !bRotable )
		{
			const CTRect<float> rectTexture = CTRect<float>( 0.0f, 0.0f, pTmpTexture->nWidth, pTmpTexture->nHeight );
			
			CRectLayout rects;
			rects.AddRect( vPos.x, vPos.y, vSize.x, vSize.y, rectTexture, 0xFFFFFFFF );
			VirtualToScreen( &rects );
			pVisitor->VisitUIRect( pTmpTexture, 3, rects );
		}
		else
		{
			CVec2 sPos[4];
			NGfx::SPixel8888 sColors[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
			CVec2 vCenter( vPos + vSize * 0.5f );
			
			rectTexture = CTRect<float>( 0.0f, 0.0f, vRotableBackgroundSize.x, vRotableBackgroundSize.y );

			// background
			if ( pRotableBackgroundTexture )
			{
				sBackgroundPos[1] = rotableTransform.RotateBackgroundPos( CVec2( -vRotableBackgroundSize.x / 2, -vRotableBackgroundSize.y / 2 ) ) + vCenter;
				sBackgroundPos[0] = rotableTransform.RotateBackgroundPos( CVec2( vRotableBackgroundSize.x / 2, -vRotableBackgroundSize.y / 2 ) ) + vCenter;
				sBackgroundPos[3] = rotableTransform.RotateBackgroundPos( CVec2( vRotableBackgroundSize.x / 2, vRotableBackgroundSize.y / 2 ) ) + vCenter;
				sBackgroundPos[2] = rotableTransform.RotateBackgroundPos( CVec2( -vRotableBackgroundSize.x / 2, vRotableBackgroundSize.y / 2 ) ) + vCenter;

				VirtualToScreen( &sBackgroundPos[0] );
				VirtualToScreen( &sBackgroundPos[1] );
				VirtualToScreen( &sBackgroundPos[2] );
				VirtualToScreen( &sBackgroundPos[3] );

				pVisitor->VisitUIRect( pRotableBackgroundTexture, 3, sBackgroundPos, sBackgroundColors, rectTexture );
			}
			
			float fDX = 0.0f;
			float fDY = 0.0f;
			if ( fAspect < 1.0f )
			{
				fDX = (1.0f - fAspect) / 2.0f;
			}
			else if ( fAspect > 1.0f )
			{
				fDY = (1.0f - 1.0f / fAspect) / 2.0f;
			}
			
			// border flicker correction
			float fDeltaX = vSize.x > 1.0f ? 2.0f / vSize.x : 0.0f;
			float fDeltaY = vSize.y > 1.0f ? 2.0f / vSize.y : 0.0f;

			sPos[1] = vPos + rotableTransform.Transform( CVec2( fDX + fDeltaX, fDY + fDeltaY ) );
			sPos[0] = vPos + rotableTransform.Transform( CVec2( fDX + fDeltaX, 1.0f - fDY - fDeltaY ) );
			sPos[3] = vPos + rotableTransform.Transform( CVec2( 1.0f - fDX - fDeltaX, 1.0f - fDY - fDeltaY ) );
			sPos[2] = vPos + rotableTransform.Transform( CVec2( 1.0f - fDX - fDeltaX, fDY + fDeltaY ) );
			VirtualToScreen( &sPos[0] );
			VirtualToScreen( &sPos[1] );
			VirtualToScreen( &sPos[2] );
			VirtualToScreen( &sPos[3] );

			{
				const CTRect<float> rectTexture = CTRect<float>( 1.0f, 1.0f, pTmpTexture->nWidth - 1.0f, pTmpTexture->nHeight - 1.0f );
				pVisitor->VisitUIRect( pTmpTexture, 3, sPos, sColors, rectTexture );
			}
			
			if ( pRotableForegroundTexture )
			{
				pVisitor->VisitUIRect( pRotableForegroundTexture, 3, sBackgroundPos, sBackgroundColors, rectTexture );
			}
		}
	}
	CRectLayout rects;
	{
		const CTRect<float> rectTexture = CTRect<float>( 0.0f, 0.0f, pInstance->placement.size.Get().x, pInstance->placement.size.Get().y );
		rects.AddRect( vPos.x, vPos.y, 
			pInstance->placement.size.Get().x, pInstance->placement.size.Get().y, rectTexture, 0xFFFFFFFF );
		VirtualToScreen( &rects );
		//{ update WarFog data
		//}
		pVisitor->VisitUITextureRect( pWarFogLayer, 3, rects );
		pVisitor->VisitUITextureRect( pUnitsLayer, 3, rects );
		pVisitor->VisitUITextureRect( pViewPortLayer, 3, rects );
	}
	if ( pRotableNorthPointTexture )
		pVisitor->VisitUIRect( pRotableNorthPointTexture, 3, sBackgroundPos, sBackgroundColors, rectTexture );

	for ( int i = 0; i < markers.size(); ++i )
	{
		const SMarker &marker = markers[i];
		
		CVec2 vMarkerPos;
		MapToScreen( marker.vPos, &vMarkerPos );
		const CTRect<float> rectTexture = CTRect<float>( 0.0f, 0.0f, marker.pTexture->nWidth, marker.pTexture->nHeight );
		CRectLayout rects;
		rects.AddRect( vPos.x + vMarkerPos.x - marker.vTexturePoint.x, 
			vPos.y + vMarkerPos.y - marker.vTexturePoint.y, 
			marker.pTexture->nWidth, marker.pTexture->nHeight, rectTexture, 0xFFFFFFFF );
		VirtualToScreen( &rects );
		pVisitor->VisitUIRect( marker.pTexture, 3, rects );
	}

	pVisitor->VisitUITextureRect( pFiguresLayer, 3, rects );
	pVisitor->VisitUITextureRect( pRangesLayer, 3, rects );
}

bool CWindowMiniMap::IsActiveArea( const CVec2 &vPos ) const
{
	if ( !CWindow::IsActiveArea( vPos ) )
		return false;

	CTRect<float> rect = GetWindowRect();
	CVec2 vScreenPos( rect.x1, rect.y1 );
	CVec2 vSize( rect.GetSizeX(), rect.GetSizeY() );
	CVec2 vCenter( vSize * 0.5f );

	vector<CVec2> poligon;
	poligon.resize( 4 );
	poligon[0] = rotableTransform.RotateBackgroundPos( CVec2( -vRotableBackgroundSize.x / 2, -vRotableBackgroundSize.y / 2 ) ) + vCenter;
	poligon[1] = rotableTransform.RotateBackgroundPos( CVec2( vRotableBackgroundSize.x / 2, -vRotableBackgroundSize.y / 2 ) ) + vCenter;
	poligon[2] = rotableTransform.RotateBackgroundPos( CVec2( vRotableBackgroundSize.x / 2, vRotableBackgroundSize.y / 2 ) ) + vCenter;
	poligon[3] = rotableTransform.RotateBackgroundPos( CVec2( -vRotableBackgroundSize.x / 2, vRotableBackgroundSize.y / 2 ) ) + vCenter;

	return CP_OUTSIDE != ClassifyPolygon( poligon, vPos - vScreenPos );
}

void CWindowMiniMap::MakeTransformParams( const CVec2 &vPoint00, const CVec2 &vPoint01, 
	const CVec2 &vPoint10, const CVec2 &vPoint11 )
{
	bNeedScreenToMapRefresh = true;
	// for top-left matrix m1:
	// 0 = p00.x * m11 + p00.y * m12 + m13 |
	// 0 = p01.x * m11 + p01.y * m12 + m13 |
	// 1 = p10.x * m11 + p10.y * m12 + m13 |
	// ------------------------------------+
	// 0 = p00.x * m21 + p00.y * m22 + m23 |
	// 1 = p01.x * m21 + p01.y * m22 + m23 |
	// 0 = p10.x * m21 + p10.y * m22 + m23 |
	float fDet = Det( vPoint00.x, vPoint00.y, 1,
                    vPoint01.x, vPoint01.y, 1,
                    vPoint10.x, vPoint10.y, 1 );

	mScreenToMap1[0][0] = Det( 0, vPoint00.y, 1,
	                           0, vPoint01.y, 1,
									           1, vPoint10.y, 1 )/fDet;

	mScreenToMap1[0][1] = Det( vPoint00.x, 0, 1,
	                           vPoint01.x, 0, 1,
									           vPoint10.x, 1, 1 )/fDet;

	mScreenToMap1[0][2] = Det( vPoint00.x, vPoint00.y, 0,
	                           vPoint01.x, vPoint01.y, 0,
		                         vPoint10.x, vPoint10.y, 1 )/fDet;

	mScreenToMap1[1][0] = Det( 0, vPoint00.y, 1,
		                         1, vPoint01.y, 1,
		                         0, vPoint10.y, 1 )/fDet;

	mScreenToMap1[1][1] = Det( vPoint00.x, 0, 1,
		                         vPoint01.x, 1, 1,
		                         vPoint10.x, 0, 1 )/fDet;

	mScreenToMap1[1][2] = Det( vPoint00.x, vPoint00.y, 0,
		                         vPoint01.x, vPoint01.y, 1,
		                         vPoint10.x, vPoint10.y, 0 )/fDet;

	// for bottom-right matrix m2:
	// 0 = p01.x * m11 + p01.y * m12 + m13 |
	// 1 = p10.x * m11 + p10.y * m12 + m13 |
	// 1 = p11.x * m11 + p11.y * m12 + m13 |
	// ------------------------------------+
	// 1 = p01.x * m21 + p01.y * m22 + m23 |
	// 0 = p10.x * m21 + p10.y * m22 + m23 |
	// 1 = p11.x * m11 + p11.y * m12 + m13 |
	fDet = Det( vPoint01.x, vPoint01.y, 1,
           vPoint10.x, vPoint10.y, 1,
           vPoint11.x, vPoint11.y, 1 );

	mScreenToMap2[0][0] = Det( 0, vPoint01.y, 1,
	                           1, vPoint10.y, 1,
									           1, vPoint11.y, 1 )/fDet;

	mScreenToMap2[0][1] = Det( vPoint01.x, 0, 1,
	                           vPoint10.x, 1, 1,
									           vPoint11.x, 1, 1 )/fDet;

	mScreenToMap2[0][2] = Det( vPoint01.x, vPoint01.y, 0,
	                           vPoint10.x, vPoint10.y, 1,
		                         vPoint11.x, vPoint11.y, 1 )/fDet;

	mScreenToMap2[1][0] = Det( 1, vPoint01.y, 1,
		                         0, vPoint10.y, 1,
		                         1, vPoint11.y, 1 )/fDet;

	mScreenToMap2[1][1] = Det( vPoint01.x, 1, 1,
		                         vPoint10.x, 0, 1,
		                         vPoint11.x, 1, 1 )/fDet;

	mScreenToMap2[1][2] = Det( vPoint01.x, vPoint01.y, 1,
		                         vPoint10.x, vPoint10.y, 0,
		                         vPoint11.x, vPoint11.y, 1 )/fDet;
	
	mMapToScreen1[0][0] = vPoint10.x - vPoint00.x;
	mMapToScreen1[0][1] = vPoint01.x - vPoint00.x;
	mMapToScreen1[0][2] = vPoint00.x;

	mMapToScreen1[1][0] = vPoint10.y - vPoint00.y;
	mMapToScreen1[1][1] = vPoint01.y - vPoint00.y;
	mMapToScreen1[1][2] = vPoint00.y;

	mMapToScreen2[0][0] = vPoint11.x - vPoint01.x;
	mMapToScreen2[0][1] = vPoint11.x - vPoint10.x;
	mMapToScreen2[0][2] = vPoint01.x + vPoint10.x - vPoint11.x;

	mMapToScreen2[1][0] = vPoint11.y - vPoint01.y;
	mMapToScreen2[1][1] = vPoint11.y - vPoint10.y;
	mMapToScreen2[1][2] = vPoint01.y + vPoint10.y - vPoint11.y;
}

void CWindowMiniMap::MakeRotableTransformParams( const CVec2 &vDirView )
{
	bNeedScreenToMapRefresh = true;
	float fAngleView = atan2( vDirView.y, vDirView.x ) - FP_PI2;
	float fDeltaAngle = fabsf( fAngleView - fPrevRotableAngle );
	if ( fDeltaAngle < F_ANGLE_DELTA )
		return;
	
	rotableTransform.Init( fAngleView, vRotableSize, vRotableCenter );
	MakeMaskMimimap();
	fPrevRotableAngle = fAngleView;
}

bool CWindowMiniMap::OnButtonDown( const CVec2 &vPos, const int nButton )
{
 	CWindow::OnButtonDown( vPos, nButton );

	if ( !IsVisible() || !IsEnabled() )
		return false;

	CVec2 vAIPos;
	CTRect<float> rect = GetWindowRect();
	if ( ScreenToMap( vPos - CVec2( rect.x1, rect.y1 ), &vAIPos ) )
	{
		bButtonDown[nButton] = true;
		NInput::PostEvent( "minimap_down", PackCoords( vAIPos ), nButton );
#ifndef _FINALRELEASE
		static int nCount = 0;
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 2, StrFmt( "minimap_down (%.0f,%.0f) from window pos (%.0f, %.0f), #%i", vAIPos.x, vAIPos.y, vPos.x, vPos.y, nCount++ ) );
#endif
		return true;
	}
	return false;
}

bool CWindowMiniMap::OnButtonUp( const CVec2 &vPos, const int nButton )
{
	CWindow::OnButtonUp( vPos, nButton );

	if ( !IsVisible() || !IsEnabled() )
		return false;

	CVec2 vAIPos( -1, -1 );
	/*if ( IsInside( vPos ) )
	{
		ScreenToMap( vPos - pInstance->placement.position.Get(), &vAIPos );
		NInput::PostEvent( "minimap_up", PackCoords( vAIPos ), nButton );
#ifndef _FINALRELEASE
		static int nCount = 0;
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 3, StrFmt( "minimap_up (%.0f,%.0f) from window pos (%.0f, %.0f), #%i", vAIPos.x, vAIPos.y, vPos.x, vPos.y, nCount++ ) );
#endif
		return true;
	}*/
	bool bRet = bButtonDown[nButton] == 1;
	bButtonDown[nButton] = false;
	NInput::PostEvent( "minimap_up", PackCoords( vAIPos ), nButton );
	return bRet;
}

bool CWindowMiniMap::OnMouseMove( const CVec2 &vPos, const int nButton )
{
	CWindow::OnMouseMove( vPos, nButton );

	if ( !IsVisible() || !IsEnabled() )
		return false;

	CVec2 vAIPos;
	
	bool bPressed = false;
	for ( int i = 0; i < bButtonDown.size(); ++i )
		bPressed |= bButtonDown[i] != 0;

	CTRect<float> rect = GetWindowRect();
	vLastMousePos = vPos;
	bool bResult = ScreenToMap( vPos - CVec2( rect.x1, rect.y1 ), &vAIPos );
	NInput::PostEvent( "reinf_update_minimap_pos", PackCoords( vAIPos ), bResult );
	if ( bResult )
	{
		if ( bPressed )
		{
			NInput::PostEvent( "minimap_move", PackCoords( vAIPos ), nButton );
#ifndef _FINALRELEASE
			static int nCount = 0;
			CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 4, StrFmt( "minimap_move (%.0f,%.0f) from window pos (%.0f, %.0f), #%i", vAIPos.x, vAIPos.y, vPos.x, vPos.y, nCount++ ) );
#endif
		}
		return true;
	}
	return false;
}

bool CWindowMiniMap::MsgMouseMoveEmit( const SGameMessage &msg )
{
	CVec2 vAIPos;
	CTRect<float> rect = GetWindowRect();
	bool bResult = IsInside( vLastMousePos ) && ScreenToMap( vLastMousePos - CVec2( rect.x1, rect.y1 ), &vAIPos );
	NInput::PostEvent( "reinf_update_minimap_pos", PackCoords( vAIPos ), bResult );

	return false;
}

void CWindowMiniMap::SetUnits( const vector< SMiniMapUnitInfo > &vUnits )
{
	pUnitsLayer->Clear();
	if ( !playerColors.empty() )
	{
		for ( int i = 0; i < vUnits.size(); ++i )
		{
			const SMiniMapUnitInfo &unit = vUnits[i];
			//NI_ASSERT( vUnits[i].player >=0 && vUnits[i].player < playerColors.size(), StrFmt( "MiniMap's color not define for player %d", vUnits[i].player ) );
			if ( unit.player >=0 && unit.player < playerColors.size() )
			{
				CVec2 vPos;
				MapToScreen( CVec2( unit.x, unit.y ), &vPos );
				float fRadius = RadiusMapToScreen( unit.radius * AI_TILE_SIZE );
				if ( fRadius < F_BIG_UNIT_RADIUS || NGlobal::GetVar( "m1", 0 ).GetFloat() != 0 )
				{
					//if ( vUnits[i].player == 0 || pWarFogLayer->GetPixel( vPos.x, vPos.y ).a < 64 )
					{
						CDrawMiniMapPixelFunctional funcDrawPixel( pUnitsLayer, playerColors[unit.player], &maskMiniMap );
						funcDrawPixel( vPos.x, vPos.y );
						funcDrawPixel( vPos.x + 1, vPos.y );
						funcDrawPixel( vPos.x - 1, vPos.y );
						funcDrawPixel( vPos.x, vPos.y + 1 );
						funcDrawPixel( vPos.x, vPos.y - 1 );
					}
				}
				else
				{
					// only large units

					int nRadius = fRadius * 0.666f; // empiric size correction
					int nRadius2 = nRadius * nRadius;
					int nCenterX = vPos.x;
					int nCenterY = vPos.y;
					
					CDrawMiniMapPixelFunctional funcDrawPixel( pUnitsLayer, playerColors[unit.player], &maskMiniMap );
					
					for ( int x = -nRadius; x <= nRadius; ++x )
					{
						for ( int y = -nRadius; y <= nRadius; ++y )
						{
							if ( (x * x + y * y) < nRadius2 )
							{
								funcDrawPixel( nCenterX + x, nCenterY + y );
							}
						}
					}
				}
			}
		}
	}
	pUnitsLayer->SetNeedUpdate();
}

void CWindowMiniMap::SetWarFog( const CArray2D<BYTE> *pWarFogInfo )
{
	if ( NGlobal::GetVar( "m1", 0 ).GetFloat() == 1 )
	{
		pWarFog = pWarFogInfo;
		UpdateWarFog();
	}
	else
	{
		warFog = *pWarFogInfo;
		UpdateWarFog();
	}
}

void CWindowMiniMap::SetMarkers( const vector<SMarker> &_markers )
{
	markers.clear();
	markers.resize( _markers.size() );
	for ( int i = 0; i < markers.size(); ++i )
	{
		const SMarker &src = _markers[i];
		SMarker &dst = markers[i];
		
		dst.pTexture = src.pTexture;
		dst.vPos = src.vPos;
		dst.vTexturePoint = src.vTexturePoint;
	}
}

void CWindowMiniMap::SetFigures( const vector<SFigure> &figures )
{
	pFiguresLayer->Clear();
	for ( int i = 0; i < figures.size(); ++i )
	{
		const SFigure &figure = figures[i];
		CDrawMiniMapPixelFunctional funcDrawPixel( pFiguresLayer, figure.color, &maskMiniMap );
		float fHalfSize = figure.fSize * 0.5f;
		SMatrix2 matrix( figure.fAngle );
		switch ( figure.eType )
		{
			case NDb::MFT_TRIANGLE:
			{
				CVec2 vPos1;
				CVec2 vPos2;
				CVec2 vPos3;
				MapToScreen( figure.vPos + matrix.Rotate( CVec2( 0.0f, fHalfSize ) ), &vPos1 );
				MapToScreen( figure.vPos + matrix.Rotate( CVec2( figure.fSize / -FP_SQRT_3, -fHalfSize ) ), &vPos2 );
				MapToScreen( figure.vPos + matrix.Rotate( CVec2( figure.fSize / FP_SQRT_3, -fHalfSize ) ), &vPos3 );

				MakeLine2( vPos1.x, vPos1.y, vPos2.x, vPos2.y, funcDrawPixel );
				MakeLine2( vPos2.x, vPos2.y, vPos3.x, vPos3.y, funcDrawPixel );
				MakeLine2( vPos3.x, vPos3.y, vPos1.x, vPos1.y, funcDrawPixel );
			}
			break;

			case NDb::MFT_SQUARE:
			{
				CVec2 vPos1;
				CVec2 vPos2;
				CVec2 vPos3;
				CVec2 vPos4;
				MapToScreen( figure.vPos + matrix.Rotate( CVec2( -fHalfSize, -fHalfSize ) ), &vPos1 );
				MapToScreen( figure.vPos + matrix.Rotate( CVec2( fHalfSize, -fHalfSize ) ), &vPos2 );
				MapToScreen( figure.vPos + matrix.Rotate( CVec2( fHalfSize, fHalfSize ) ), &vPos3 );
				MapToScreen( figure.vPos + matrix.Rotate( CVec2( -fHalfSize, fHalfSize ) ), &vPos4 );

				MakeLine2( vPos1.x, vPos1.y, vPos2.x, vPos2.y, funcDrawPixel );
				MakeLine2( vPos2.x, vPos2.y, vPos3.x, vPos3.y, funcDrawPixel );
				MakeLine2( vPos3.x, vPos3.y, vPos4.x, vPos4.y, funcDrawPixel );
				MakeLine2( vPos4.x, vPos4.y, vPos1.x, vPos1.y, funcDrawPixel );
			}
			break;

			case NDb::MFT_CIRCLE:
			{
				CVec2 vPos;
				MapToScreen( figure.vPos, &vPos );
				float fRadius = RadiusMapToScreen( fHalfSize );

				BresenhamCircle( vPos.x, vPos.y, fRadius, funcDrawPixel );
			}
			break;
		};
	}

	pFiguresLayer->SetNeedUpdate();
}

void CWindowMiniMap::SetRangeInfo( const int nUnitID, const SShootAreas &rangeInfo )
{
	// Calculate AI radius to X ratio
	CVec2 vT;
	RadiusMapToScreen( CVec2( FP_SQRT_2 / 2, FP_SQRT_2 / 2 ), &vT );
	float fR2XRatio = vT.x;
	// Calculate minimap squash ratio
	float fY2XRatio = float( GetPlacement().GetSizeY() ) / GetPlacement().GetSizeX();
	// Draw ranges
	for ( list<SShootArea>::const_iterator it = rangeInfo.areas.begin(); it != rangeInfo.areas.end(); ++it )
	{
		const SShootArea &area = *it;
		CDrawMiniMapPixelFunctional funcDrawPixel( pRangesLayer, 0xFF000000 | area.GetColor(), &maskMiniMap );
		CVec2 vCenter;
		MapToScreen( CVec2( area.vCenter3D.x, area.vCenter3D.y ), &vCenter );
		const float fStartAngle = AI2VisRad(area.wStartAngle) + FP_PI2;
		const float fEndAngle = AI2VisRad(area.wFinishAngle) + FP_PI2;

		if ( bRotable )
		{
			float fMin = RadiusMapToScreen( area.fMinR );
			float fMax = RadiusMapToScreen( area.fMaxR );
			// Min range
			BresenhamCircle( vCenter.x, vCenter.y, fMin, funcDrawPixel );
			// Max range
			BresenhamCircle( vCenter.x, vCenter.y, fMax, funcDrawPixel );
		}
		else
		{
			// Min range
			BresenhamEllipse( vCenter.x, vCenter.y, area.fMinR * fR2XRatio, fY2XRatio, funcDrawPixel );
			// Max range
			BresenhamEllipse( vCenter.x, vCenter.y, area.fMaxR * fR2XRatio, fY2XRatio, funcDrawPixel );
		}

		// Sector lines
		if ( area.wStartAngle != area.wFinishAngle )
		{
			CVec2 vPos;

			vPos.x = area.vCenter3D.x + area.fMaxR * NMath::Cos( fStartAngle );
			vPos.y = area.vCenter3D.y + area.fMaxR * NMath::Sin( fStartAngle );
			MapToScreen( vPos, &vPos );
			MakeLine2( vCenter.x, vCenter.y, vPos.x, vPos.y, funcDrawPixel );
			vPos.x = area.vCenter3D.x + area.fMaxR * NMath::Cos( fEndAngle );
			vPos.y = area.vCenter3D.y + area.fMaxR * NMath::Sin( fEndAngle );
			MapToScreen( vPos, &vPos );
			MakeLine2( vCenter.x, vCenter.y, vPos.x, vPos.y, funcDrawPixel );
		}
	}
	pRangesLayer->SetNeedUpdate();
	bRangesPresent = true;
}

void CWindowMiniMap::RemoveAllRangeInfo( )
{
	if ( bRangesPresent ) 
	{
		pRangesLayer->Clear();
		pRangesLayer->SetNeedUpdate();
		bRangesPresent = false;
	}
}

void CWindowMiniMap::SetMaterial( CDBPtr< NDb::SMaterial > _pMaterial )
{
	pMaterial = _pMaterial;
}

void CWindowMiniMap::SetTexture( const NDb::STexture *_pTexture )
{
	pTexture = _pTexture;
}

void CWindowMiniMap::SetLoadingMapParams( int nWidth, int nHeight )
{
	fMapWidth = nWidth;
	fMapHeight = nHeight;
	fAspect = fMapWidth/fMapHeight;

	SetBaseRotableParams();
	CVec2 vDir( -1.0f, 1.0f );
	Normalize( &vDir );
	MakeRotableTransformParams( vDir );
}

CVec2 CWindowMiniMap::GetAIToScreen( const CVec2 &vPos ) const
{
	CVec2 vScreenPos;
	MapToScreen( vPos, &vScreenPos );
	return vScreenPos;
}

void CWindowMiniMap::SetViewport( const vector< CVec2 > &vPoints )
{
	NI_VERIFY( vPoints.size() >= 4, "Wrong call", return );
	if ( bRotable )
	{
		CVec2 vDir = (vPoints[0] + vPoints[1]) - (vPoints[2] + vPoints[3]);
		Normalize( &vDir );
		if ( fabs2( vDir - vOldDir ) > 0.0001 )
		{
			vOldDir = vDir;
			MakeRotableTransformParams( vDir );
			UpdateWarFog();
			pUnitsLayer->SetNeedUpdateNow();
			pWarFogLayer->SetNeedUpdateNow();
			pViewPortLayer->SetNeedUpdateNow();
			pFiguresLayer->SetNeedUpdateNow();
			pRangesLayer->SetNeedUpdateNow();
		}
	}

	//DEBUG{
	/*
  vector<CVec2> vPos;
	vPos.resize( 4, VNULL2 );
	for ( int i = 0; i < vPoints.size(); ++i )
		MapToScreen( vPoints[i], &vPos[i] );
  
	if ( IsSegmentsIntersected( vPoints[0], vPoints[3], vPoints[1], vPoints[2] ) ) 
		DebugTrace( "input segments intersected" );
	else
		DebugTrace( "input segments not intersected" );

	if ( IsSegmentsIntersected( vPos[0], vPos[3], vPos[1], vPos[2] ) ) 
		DebugTrace( "output segments intersected" );
	else
		DebugTrace( "output segments not intersected" );

	DebugTrace( "-------------------------------" );
	*/
	//DEBUG}
	
	pViewPortLayer->Clear();
	CDrawMiniMapPixelFunctional funcDrawPixel( pViewPortLayer, frameColor, &maskMiniMap );
	for ( int i = 0; i < vPoints.size(); ++i )
	{
		CVec2 vPos1, vPos2;
		MapToScreen( vPoints[i], &vPos1 );
		if ( i < vPoints.size()-1 )
			MapToScreen( vPoints[i+1], &vPos2 );
		else
			MapToScreen( vPoints[0], &vPos2 );
		MakeLine2( vPos1.x, vPos1.y, vPos2.x, vPos2.y, funcDrawPixel );
	}
	pViewPortLayer->SetNeedUpdate();
}

void CWindowMiniMap::SetPlayerColor( const int nPlayer, const NGfx::SPixel8888 &color )
{
	if ( playerColors.size() <= nPlayer )
		playerColors.resize( nPlayer+1 );

	playerColors[nPlayer] = color;
}

void CWindowMiniMap::AfterLoad()
{
	if ( NGlobal::GetVar( "m1", 0 ).GetFloat() != 0 )
		MakeMaskMimimap();
}

int CWindowMiniMap::operator&( IBinSaver &saver )
{
	saver.Add( 1, static_cast<CWindow*>( this ) );
	saver.Add( 2, &mScreenToMap1 );
	saver.Add( 3, &mScreenToMap2 );
	saver.Add( 4, &mMapToScreen1 );
	saver.Add( 5, &mMapToScreen2 );
	saver.Add( 6, &fAspect );
	saver.Add( 7, &fMapWidth );
	saver.Add( 8, &fMapHeight );
	saver.Add( 9, &pUnitsLayer );
	saver.Add( 10, &pViewPortLayer );
	saver.Add( 11, &pWarFogLayer );
	saver.Add( 12, &warFogColors );

	if ( NGlobal::GetVar( "m1", 0 ).GetFloat() == 0 )
		saver.Add( 13, &maskMiniMap );

	saver.Add( 14, &pShared );
	saver.Add( 15, &pInstance );

	saver.Add( 16, &playerColors );
	saver.Add( 17, &frameColor );
	saver.Add( 18, &pMaterial );
	saver.Add( 19, &markers );
	saver.Add( 20, &pFiguresLayer );
	saver.Add( 21, &pRangesLayer );
	saver.Add( 22, &bRangesPresent );
	
	saver.Add( 23, &bRotable );
	saver.Add( 24, &vRotableCenter );
	saver.Add( 25, &vRotableSize );
	saver.Add( 26, &rotableTransform );

	if ( NGlobal::GetVar( "m1", 0 ).GetFloat() == 0 )
		saver.Add( 27, &warFog );
	else
		saver.Add( 27, &fAdditionalScale ) ;

	saver.Add( 29, &fPrevRotableAngle );
	saver.Add( 30, &pTexture );
	saver.Add( 31, &vLastMousePos );

	if ( saver.IsReading() )
	{
		bButtonDown.clear();
		bButtonDown.resize( 4, false );
		bNeedScreenToMapRefresh = true;
		vOldDir = VNULL2;
	}
	saver.Add( 32, &vSize	 );

	saver.Add( 33, &pRotableBackgroundTexture );
	saver.Add( 34, &vRotableBackgroundSize );

	saver.Add( 35, &pRotableNorthPointTexture );
	saver.Add( 36, &vRotableNorhtPointSize );
	
	saver.Add( 37, &pRotableForegroundTexture );

	return 0;
}

START_REGISTER(MiniMapCommands)

REGISTER_VAR_EX( "minimap_dark_warfog_alpha", NGlobal::VarIntHandler, &s_nDarkWarFogAlpha, 128, STORAGE_NONE );
REGISTER_VAR_EX( "minimap_light_warfog_alpha", NGlobal::VarIntHandler, &s_nLightWarFogAlpha, 0, STORAGE_NONE );

FINISH_REGISTER

