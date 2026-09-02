#include "stdafx.h"
#include <fmt/format.h>
#include "Misc/2Darray.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "Stats_B2_M1/IconsSet.h"
#include "Misc/StrProc.h"
#include "SceneB2/Scene.h"
#include "MiniMapWindow.h"
#include "ResourceDefines.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "EditorScene.h"
#include "SceneB2/TerrainInfo.h"
#include "MapInfoEditor.h"
#include "Image/Image.h"
#include "ED_B2_M1Dll.h"
#include "EditorMethods.h"
#include "System/VFSOperations.h"

#include <cstdint>

#include <zconf.h>

BEGIN_MESSAGE_MAP(CMiniMapWindow, CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

inline NImage::SColor GetColor( const NDb::STerrain &terrainDesc, const STerrainInfo &terrainInfo, const int nX, const int nY )
{
	if ( terrainInfo.tileTerraMap[nY][nX] >= terrainDesc.pTerraSet->terraTypes.size() )
		return NImage::SColor( 0xFF, 0xFF, 0, 0 );
	else
		return NImage::SColor( terrainDesc.pTerraSet->terraTypes[terrainInfo.tileTerraMap[nY][nX]]->nColor );
}

void CMiniMapWindow::LoadMap( const NDb::STerrain *pTerrainDesc )
{
	if ( !IsWindow( m_hWnd ) )
	{
		return;
	}
	if ( pTerrainDesc == 0 ) 
	{
		if ( bMapLoaded )
		{		
			mapDC.DeleteDC();
			bMapLoaded = false;
		}
	}
	else
	{
		if ( bMapLoaded )
			LoadMap( 0 );
		mapSize.cx = pTerrainDesc->nNumPatchesX * VIS_TILES_IN_PATCH;
		mapSize.cy = pTerrainDesc->nNumPatchesY * VIS_TILES_IN_PATCH;

		mapAISize.cx = pTerrainDesc->nNumPatchesX * AI_TILES_IN_PATCH;
		mapAISize.cy = pTerrainDesc->nNumPatchesY * AI_TILES_IN_PATCH;
		//
		const STerrainInfo *pTerrainInfo = Scene()->GetTerraManager()->GetTerraInfo() ;
		//const std::string szTerrainBinFileName = GetTerrainBinFileName( pTerrainDesc );
		//CFileStream stream( NVFS::GetMainVFS(), szTerrainBinFileName );
		//if ( stream.IsOk() )
		//{
		//	CPtr<IBinSaver> pSaver = CreateBinSaver( &stream, SAVER_MODE_READ );
		//	if ( pSaver == 0 )
		//	{
		//		//SStreamStats stats;
		//		//pStream->GetStats( &stats );
		//		//NI_ASSERT( pSaver != 0, fmt::format("Can't open stream \"{}\" to read map", stats.pszName) );
		//		NI_ASSERT( pSaver != 0, fmt::format("Can't open stream \"\" to read map") );
		//	}
		//	pSaver->Add( 1, &terrainInfo );
		//}
		//
		CDC *dc = GetDC();
		mapDC.CreateCompatibleDC( dc );
		mapBitmap.DeleteObject();
		mapBitmap.CreateCompatibleBitmap( dc, mapSize.cx + mapSize.cy - 1, mapSize.cx + mapSize.cy - 1 );
		mapDC.SelectObject( mapBitmap );
		ReleaseDC( dc );
		RECT r;
		SetRect( &r, 0, 0, mapSize.cx + mapSize.cy - 1, mapSize.cx + mapSize.cy - 1 );
		CBrush btnFace( COLOR_BTNFACE + 1 );
		mapDC.FillRect( &r, &btnFace );

		for ( int x = 0; x < mapSize.cx && x < pTerrainInfo->heights.GetSizeX() && x < pTerrainInfo->tileTerraMap.GetSizeX(); ++x )
		{
			for ( int y = 0; y < mapSize.cy && y < pTerrainInfo->heights.GetSizeY() && y < pTerrainInfo->tileTerraMap.GetSizeY(); ++y )
			{
				POINT p;
				p.x = x + y;
				p.y = x + mapSize.cy - y - 1;
				const NImage::SColor color11 = GetColor( *pTerrainDesc, *pTerrainInfo, x, y );
				mapDC.SetPixelV( p, RGB( color11.r, color11.g, color11.b ) );
				if ( x > 0 && y > 0 )
				{
					const NImage::SColor color10 = GetColor( *pTerrainDesc, *pTerrainInfo, x, y - 1 );
					const NImage::SColor color01 = GetColor( *pTerrainDesc, *pTerrainInfo, x - 1, y );
					const NImage::SColor color00 = GetColor( *pTerrainDesc, *pTerrainInfo, x - 1, y - 1 );
					const uint32_t r = (color00.r + color01.r + color10.r + color11.r)/4;
					const uint32_t g = (color00.g + color01.g + color10.g + color11.g)/4;
					const uint32_t b = (color00.b + color01.b + color10.b + color11.b)/4;
					p.x --;
					mapDC.SetPixelV( p, RGB( r, g, b ) );
				}
			}
		}

		bMapLoaded = true;
	}
}

void CMiniMapWindow::SetMapInfoEditorSize( const int nSizeX, const int nSizeY )
{
	mapInfoEditorSize.cx = nSizeX;
	mapInfoEditorSize.cy = nSizeY;
	Invalidate();
};

bool CMiniMapWindow::Create( CWnd *parentWindow )
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_MINIMAP_WINDOW, this );
	Singleton<ICommandHandlerContainer>()->Register( CHID_MAPINFO_MINIMAP_WINDOW, ID_MIMCO_GENERATE_MINIMAP_IMAGE, ID_MIMCO_GENERATE_MINIMAP_IMAGE );

	fontMiniMap.CreateFont( 20, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, DEFAULT_PITCH, "" );
	rectWhitePen.CreatePen( PS_SOLID, 1, RGB( 0, 255, 0 ) );
	rectBlackPen.CreatePen( PS_SOLID, 2, RGB( 64, 128, 0 ) );

	bMapLoaded = false;

	RECT defaultRect;
	SetRect( &defaultRect, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT );
	return CWnd::Create( AfxRegisterWndClass( CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, ::LoadCursor( NULL, IDC_ARROW ), 0, 0 ), "MiniMap", 0, defaultRect, parentWindow, 666 );
}

void CMiniMapWindow::Destroy()
{
	Singleton<ICommandHandlerContainer>()->UnRegister( CHID_MAPINFO_MINIMAP_WINDOW );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_MINIMAP_WINDOW );

	fontMiniMap.DeleteObject();
	rectWhitePen.DeleteObject();
	rectBlackPen.DeleteObject();
	mapDC.DeleteDC();
	mapBitmap.DeleteObject();
	CWnd::DestroyWindow();
	LoadMap( 0 );
}

bool CMiniMapWindow::EditorToMiniMap( CVec2 *pvResult, const CVec2 &vEditorPos )
{
	CRect clientRect;
	GetClientRect( &clientRect );

	ICamera *pCamera = Camera();

	CVec3 vNear, vFar;
	pCamera->GetProjectiveRayPoints( &vNear, &vFar, vEditorPos );

	const float t = ( 0.0f - vNear.z)/( vFar.z - vNear.z );

  const float fBitmapX = ( (( vFar.x - vNear.x )*t + vNear.x)/VIS_TILE_SIZE );
	const float fBitmapY = ( (( vFar.y - vNear.y )*t + vNear.y)/VIS_TILE_SIZE );

	pvResult->x = ( fBitmapX + fBitmapY )                        *(float)clientRect.Width()/ (float)(mapSize.cx+mapSize.cy-1);
	pvResult->y = ( fBitmapX + (float)mapSize.cy - fBitmapY - 1 )*(float)clientRect.Height()/(float)(mapSize.cx+mapSize.cy-1);

	return true;
}

bool CMiniMapWindow::MiniMapToEditor( CVec2 *pvResult, const CVec2 &vMiniMapPos )
{
	CRect clientRect;
	GetClientRect( &clientRect );

	const float fBitmapX = vMiniMapPos.x*(float)(mapSize.cx+mapSize.cy-1)/(float)clientRect.Width();
	const float fBitmapY = vMiniMapPos.y*(float)(mapSize.cx+mapSize.cy-1)/(float)clientRect.Height();

	pvResult->x = Clamp( ( fBitmapX + fBitmapY - (float)mapSize.cy + 1.0f )/2.0f, 0.0f, (float)mapSize.cx ) * VIS_TILE_SIZE;
	pvResult->y = Clamp( ( fBitmapX - fBitmapY + (float)mapSize.cx - 1.0f )/2.0f, 0.0f, (float)mapSize.cy ) * VIS_TILE_SIZE;

	return true;
}

void CMiniMapWindow::OnPaint()
{
	CPaintDC paintDC( this );
	//
	CRect clientRect;
	GetClientRect( &clientRect );
	//
	CDC dc;
	int nRes = dc.CreateCompatibleDC( &paintDC );
	CBitmap bmp;
	nRes = bmp.CreateCompatibleBitmap( &paintDC, clientRect.Width(), clientRect.Height() );
	CBitmap *pOldBitmap = dc.SelectObject( &bmp );
	dc.FillSolidRect( 0, 0, clientRect.Width(), clientRect.Height(), RGB( 0, 0, 0 ) );
	//
	if ( bMapLoaded )
	{
// draw terrain
		dc.StretchBlt( clientRect.left, clientRect.top, clientRect.Width(), clientRect.Height(), &mapDC, 0, 0, mapSize.cx+mapSize.cy - 1, mapSize.cx + mapSize.cy - 1, SRCCOPY );
// draw current view rect
		POINT rect[5];
		CVec2 vCoord;
		EditorToMiniMap( &vCoord, CVec2( 0, 0 ) );
		rect[0].x = vCoord.x; rect[0].y = vCoord.y;
		EditorToMiniMap( &vCoord, CVec2( mapInfoEditorSize.cx, 0 ) );
		rect[1].x = vCoord.x; rect[1].y = vCoord.y;
		EditorToMiniMap( &vCoord, CVec2( mapInfoEditorSize.cx, mapInfoEditorSize.cy ) );
		rect[2].x = vCoord.x; rect[2].y = vCoord.y;
		EditorToMiniMap( &vCoord, CVec2( 0, mapInfoEditorSize.cy ) );
		rect[3].x = vCoord.x; rect[3].y = vCoord.y;
		rect[4] = rect[0];
		dc.SelectObject( rectBlackPen );
		dc.Polyline( rect, 5 );
		for ( int i = 0; i < 5; ++i )
		{
			rect[i].x -= 1;
			rect[i].y -= 2;
		}
		dc.SelectObject( rectWhitePen );
		dc.Polyline( rect, 5 );
	}
	else
	{
		CBrush brush( COLOR_BTNFACE );
		dc.FillRect( &clientRect, &brush );
		dc.SetBkMode( TRANSPARENT );
		dc.SetTextColor( COLOR_BTNTEXT );
		dc.SelectObject( fontMiniMap );
		dc.SetTextAlign( TA_CENTER );
		dc.TextOut( ( clientRect.left+clientRect.right ) / 2, ( clientRect.top+clientRect.bottom ) / 2, "No map loaded" );
	}
	//
	paintDC.BitBlt( clientRect.left, clientRect.top, clientRect.Width(), clientRect.Height(), &dc, 0, 0, SRCCOPY );
	dc.SelectObject( pOldBitmap );
}

void CMiniMapWindow::OnLButtonDown( unsigned nFlags, CPoint point )
{
	CWnd::OnLButtonDown( nFlags, point );
	CVec2 vAIPosition;
	MiniMapToEditor( &vAIPosition, CVec2( point.x, point.y ) );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_CAMERA_POSITION, PackCoords( vAIPosition ) );
}

void CMiniMapWindow::OnMouseMove( unsigned nFlags, CPoint point )
{
	CWnd::OnMouseMove( nFlags, point );
	if ( nFlags & MK_LBUTTON )
	{
		CVec2 vAIPosition;
		MiniMapToEditor( &vAIPosition, CVec2( point.x, point.y ) );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_CAMERA_POSITION, PackCoords( vAIPosition ) );
		OnPaint();	
	}
}

void CMiniMapWindow::OnLButtonUp( unsigned nFlags, CPoint point )
{
	CWnd::OnLButtonUp( nFlags, point );
}

void CMiniMapWindow::OnContextMenu( CWnd* pWnd, CPoint point )
{
	CWnd::OnContextMenu( pWnd, point );
	//
	CMenu mainPopupMenu;
	AfxSetResourceHandle( theEDB2M1Instance );
	mainPopupMenu.LoadMenu( IDM_MAPINFO_CONTEXT_MENU );
	AfxSetResourceHandle( AfxGetInstanceHandle() );
	CMenu *pMenu = mainPopupMenu.GetSubMenu( MICM_MINIMAP );
	if ( pMenu )
	{
		pMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, Singleton<IMainFrameContainer>()->GetSECWorkbook(), 0 );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	}
	mainPopupMenu.DestroyMenu();
}


bool CMiniMapWindow::HandleCommand( unsigned nCommandID, uintptr_t dwData )
{
	switch( nCommandID )
	{
		case ID_MIMCO_GENERATE_MINIMAP_IMAGE:
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_MAP_INFO_EDITOR, ID_MIMCO_GENERATE_MINIMAP_IMAGE, 0 );
			return true;
		}
		default:
			return false;
	}
	return false;
}


bool CMiniMapWindow::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CMiniMapWindow::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CMiniMapWindow::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_MIMCO_GENERATE_MINIMAP_IMAGE:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		default:
			return false;
	}
	return false;
}


