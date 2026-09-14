#include "stdafx.h"
#include "MapEditorLib/MfcWidget.h"
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
		image = NMiniMapView::SImage();
		return;
	}
	if ( bMapLoaded )
	{
		LoadMap( 0 );
	}
	// NMiniMapView::BuildImage draws the picture this set a pixel at a time
	// with SetPixelV; the same pixels go into the bitmap at once.
	if ( !NMiniMapView::BuildImage( pTerrainDesc, &image ) )
	{
		return;
	}
	CDC *dc = GetDC();
	mapDC.CreateCompatibleDC( dc );
	mapBitmap.DeleteObject();
	mapBitmap.CreateCompatibleBitmap( dc, image.nSide, image.nSide );
	mapDC.SelectObject( mapBitmap );
	ReleaseDC( dc );
	BITMAPINFO info = {};
	info.bmiHeader.biSize = sizeof( info.bmiHeader );
	info.bmiHeader.biWidth = image.nSide;
	// Negative: rows top down, as the image holds them.
	info.bmiHeader.biHeight = -image.nSide;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;
	::SetDIBitsToDevice( mapDC.GetSafeHdc(), 0, 0, image.nSide, image.nSide, 0, 0, 0, image.nSide, &image.pixels[0], &info, DIB_RGB_COLORS );
	bMapLoaded = true;
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
	( *pvResult ) = NMiniMapView::EditorToMiniMap( image, clientRect.Width(), clientRect.Height(), vEditorPos );
	return true;
}

bool CMiniMapWindow::MiniMapToEditor( CVec2 *pvResult, const CVec2 &vMiniMapPos )
{
	CRect clientRect;
	GetClientRect( &clientRect );
	( *pvResult ) = NMiniMapView::MiniMapToEditor( image, clientRect.Width(), clientRect.Height(), vMiniMapPos );
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
		dc.StretchBlt( clientRect.left, clientRect.top, clientRect.Width(), clientRect.Height(), &mapDC, 0, 0, image.nSide, image.nSide, SRCCOPY );
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
		pMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, MainFrameWnd(), 0 );
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


