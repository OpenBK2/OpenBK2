#include "stdafx.h"

#include "System/Time.h"
#include "UI/CommandParam.h"
#include "UI/DBUserInterface.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/Interface_Editor.h"
#include "Main/GameTimer.h"
#include "Main/MainLoop.h"
#include "Input/GameMessage.h"
#include "UI/UI.h"
#include "ChildFrameWndBase.h"

#include "port/time.h"

#include <cstdint>

const int CChildFrameWndBase::DEFAULT_REFRESH_RATE = 50;


BEGIN_MESSAGE_MAP(CChildFrameWndBase, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_CHAR()
	ON_WM_SYSKEYDOWN()
	ON_WM_SYSKEYUP()
	ON_WM_SYSCHAR()
	ON_WM_CONTEXTMENU()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()


CChildFrameWndBase::CChildFrameWndBase() 
	: nUpdateSceneTimer( 0 ), 
		nUpdateSceneTimerInterval( 0 ), 
		bRunModeEnabled( false ),
		bGameInputEnabled( false ),
		bInputEnabled( true ),
		bRenderEnabled( true ),
		bShowStatistic( false ),
		bShowMovieBorders( false ),
		bEnableSceneUpdate( true ),
		bEnableScroll( false ),
		bIsSettingUp( false ),
		bWasResized( true )
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_SCENE, this );
}


CChildFrameWndBase::~CChildFrameWndBase()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SCENE );
}


BOOL CChildFrameWndBase::PreCreateWindow( CREATESTRUCT &rCreateStruct ) 
{
	if ( !CWnd::PreCreateWindow( rCreateStruct ) )
	{
		return FALSE;
	}

	rCreateStruct.dwExStyle |= WS_EX_CLIENTEDGE;
	rCreateStruct.style |= bEnableScroll ? ( WS_VSCROLL | WS_HSCROLL ) : 0;
	rCreateStruct.style &= ~WS_BORDER;
	rCreateStruct.lpszClass = AfxRegisterWndClass( CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
																								 ::LoadCursor( NULL, IDC_ARROW ),
																								 HBRUSH( COLOR_WINDOW + 1 ),
																								 NULL );

	return TRUE;
}


int CChildFrameWndBase::OnCreate( LPCREATESTRUCT pCreateStruct ) 
{
	if ( CWnd::OnCreate( pCreateStruct ) == -1 )
	{
		return -1;
	}
	//
	if ( !OnCreateChildFrameWnd() )
	{
		return -1;
	}
	//
	NMainLoop::SetInputEnabled( false );
	//
	return 0;
}


void CChildFrameWndBase::OnDestroy() 
{
	KillUpdateSceneTimer();
	nUpdateSceneTimerInterval = 0;
	//
	OnDestroyChildFrameWnd();
	//
	CWnd::OnDestroy();
}


void CChildFrameWndBase::OnTimer( unsigned nIDEvent ) 
{
	if ( nIDEvent == GetUpdateSceneTimerID() )
	{
		OnUpdateSceneTimer();
	}
	CWnd::OnTimer( nIDEvent );
}

//DebugTrace( "CChildFrameWndBase::On...(), flags: %u, ( %d, %d )\n", nFlags, point.x, point.y );

void CChildFrameWndBase::OnSetFocus( CWnd* pOldWnd )
{
	CWnd::OnSetFocus( pOldWnd );
	DebugTrace( "CChildFrameWndBase::OnSetFocus()\n" );
	RedrawWindow();
	if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
	{
		pActiveInputState->OnSetFocus( pOldWnd );
	}
	NMainLoop::SetInputEnabled( true );
	//
	//Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
}


void CChildFrameWndBase::OnKillFocus( CWnd* pNewWnd )
{
	CWnd::OnKillFocus( pNewWnd );
	DebugTrace( "CChildFrameWndBase::OnKillFocus()\n" );
	RedrawWindow();
	if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
	{
		pActiveInputState->OnKillFocus( pNewWnd );
	}
	bInputEnabled = true;
	NMainLoop::SetInputEnabled( false );
}


void CChildFrameWndBase::OnMouseMove( unsigned nFlags, CPoint point ) 
{
	CWnd::OnMouseMove( nFlags, point );
	if ( bInputEnabled )
	{
		if ( nFlags & ( MK_LBUTTON | MK_RBUTTON | MK_MBUTTON ) )
		{
			SetFocus();
		}
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnMouseMove( nFlags, CTPoint<int>( point.x, point.y ) );
		}
	}
	if ( bGameInputEnabled || ( bRunModeEnabled && !bRenderEnabled ) )
	{
		NInput::PostEvent( "win_mouse_move", PackCoords( CVec2( point.x, point.y) ), nFlags );
	}
}


BOOL CChildFrameWndBase::OnMouseWheel( unsigned nFlags, short zDelta, CPoint point )
{
	const BOOL bResult = CWnd::OnMouseWheel( nFlags, zDelta, point );
	if ( bResult )
	{
		if ( bInputEnabled )
		{
			if ( nFlags & ( MK_LBUTTON | MK_RBUTTON | MK_MBUTTON ) )
			{
				SetFocus();
			}
			if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
			{
				return pActiveInputState->OnMouseWheel( nFlags, zDelta, CTPoint<int>( point.x, point.y ) );
			}
		}
	}
	return bResult;
}


void CChildFrameWndBase::OnLButtonDown( unsigned nFlags, CPoint point ) 
{
	CWnd::OnLButtonDown( nFlags, point );
	if ( bInputEnabled )
	{
		SetFocus();
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnLButtonDown( nFlags, CTPoint<int>( point.x, point.y ) );
		}
	}
	if ( bGameInputEnabled || ( bRunModeEnabled && !bRenderEnabled ) )
	{
		NInput::PostEvent( "win_left_button_down", PackCoords( CVec2( point.x, point.y) ), nFlags );
	}
}


void CChildFrameWndBase::OnLButtonUp( unsigned nFlags, CPoint point ) 
{
	CWnd::OnLButtonUp( nFlags, point );
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnLButtonUp( nFlags, CTPoint<int>( point.x, point.y ) );
		}
	}
	if ( bGameInputEnabled || ( bRunModeEnabled && !bRenderEnabled ) )
	{
		NInput::PostEvent( "win_left_button_up", PackCoords( CVec2( point.x, point.y) ), nFlags );
	}
}


void CChildFrameWndBase::OnLButtonDblClk( unsigned nFlags, CPoint point ) 
{
	CWnd::OnLButtonDblClk( nFlags, point );
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnLButtonDblClk( nFlags, CTPoint<int>( point.x, point.y ) );
		}
	}
	if ( bGameInputEnabled || ( bRunModeEnabled && !bRenderEnabled ) )
	{
		NInput::PostEvent( "win_left_button_dblclk", PackCoords( CVec2( point.x, point.y) ), nFlags );
	}
}


void CChildFrameWndBase::OnRButtonDown( unsigned nFlags, CPoint point ) 
{
	CWnd::OnRButtonDown( nFlags, point );
	if ( bInputEnabled )
	{
		SetFocus();
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnRButtonDown( nFlags, CTPoint<int>( point.x, point.y ) );
		}
	}
	if ( bGameInputEnabled || ( bRunModeEnabled && !bRenderEnabled ) )
	{
		NInput::PostEvent( "win_right_button_down", PackCoords( CVec2( point.x, point.y) ), nFlags );
	}
}


void CChildFrameWndBase::OnRButtonUp( unsigned nFlags, CPoint point ) 
{
	CWnd::OnRButtonUp( nFlags, point );
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnRButtonUp( nFlags, CTPoint<int>( point.x, point.y ) );
		}
	}
	if ( bGameInputEnabled || ( bRunModeEnabled && !bRenderEnabled ) )
	{
		NInput::PostEvent( "win_right_button_up", PackCoords( CVec2( point.x, point.y) ), nFlags );
	}
}


void CChildFrameWndBase::OnRButtonDblClk( unsigned nFlags, CPoint point ) 
{
	CWnd::OnRButtonDblClk( nFlags, point );
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnRButtonDblClk( nFlags, CTPoint<int>( point.x, point.y ) );
		}
	}
	if ( bGameInputEnabled || ( bRunModeEnabled && !bRenderEnabled ) )
	{
		NInput::PostEvent( "win_right_button_dblclk", PackCoords( CVec2( point.x, point.y) ), nFlags );
	}
}


void CChildFrameWndBase::OnMButtonDown( unsigned nFlags, CPoint point ) 
{
	CWnd::OnMButtonDown( nFlags, point );
	if ( bInputEnabled )
	{
		SetFocus();
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnMButtonDown( nFlags, CTPoint<int>( point.x, point.y ) );
		}
	}
}


void CChildFrameWndBase::OnMButtonUp( unsigned nFlags, CPoint point ) 
{
	CWnd::OnMButtonUp( nFlags, point );
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnMButtonUp( nFlags, CTPoint<int>( point.x, point.y ) );
		}
	}
}


void CChildFrameWndBase::OnMButtonDblClk( unsigned nFlags, CPoint point ) 
{
	CWnd::OnMButtonDblClk( nFlags, point );
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnMButtonDblClk( nFlags, CTPoint<int>( point.x, point.y ) );
		}
	}
}


void CChildFrameWndBase::OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	CWnd::OnKeyDown( nChar, nRepCnt, nFlags );
	if ( nChar == 'Q' )
	{
		if ( ( ( GetAsyncKeyState( VK_SHIFT ) & 0x8000 ) > 0 ) &&
				 ( ( GetAsyncKeyState( VK_CONTROL ) & 0x8000 ) > 0 )	)
		{
			bShowStatistic = !bShowStatistic;
			RedrawWindow();
		}
	}
	else if ( nChar == 'W' )
	{
		if ( ((GetAsyncKeyState(VK_SHIFT) & 0x8000) > 0) &&
				 ((GetAsyncKeyState(VK_CONTROL) & 0x8000) > 0)	)
		{
			bShowMovieBorders = !bShowMovieBorders;
			bWasResized = true;
			RedrawWindow();
		}
	}
	if ( bInputEnabled )
	{
		SetFocus();
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnKeyDown( nChar, nRepCnt, nFlags );
		}
	}
	if ( bGameInputEnabled || ( bRunModeEnabled && !bRenderEnabled ) )
	{
		for ( int nIndex = 0; nIndex < nRepCnt; ++nIndex )
		{
			NInput::PostEvent( "win_key", nChar, 0 );
		}
	}
}


void CChildFrameWndBase::OnKeyUp( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	CWnd::OnKeyUp( nChar, nRepCnt, nFlags );
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnKeyUp( nChar, nRepCnt, nFlags );
		}
	}
}


void CChildFrameWndBase::OnChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	CWnd::OnChar( nChar, nRepCnt, nFlags );
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnChar( nChar, nRepCnt, nFlags );
		}
	}
	if ( bGameInputEnabled || ( bRunModeEnabled && !bRenderEnabled ) )
	{
		for ( int nIndex = 0; nIndex < nRepCnt; ++nIndex )
		{
			NInput::PostEvent( "win_char", nChar, 0 );
		}
	}
}


void CChildFrameWndBase::OnSysKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	CWnd::OnSysKeyDown( nChar, nRepCnt, nFlags );
	if ( bInputEnabled )
	{
		SetFocus();
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnSysKeyDown( nChar, nRepCnt, nFlags );
		}
	}
}


void CChildFrameWndBase::OnSysKeyUp( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	CWnd::OnSysKeyUp( nChar, nRepCnt, nFlags );
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnSysKeyUp( nChar, nRepCnt, nFlags );
		}
	}
}


void CChildFrameWndBase::OnSysChar( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	CWnd::OnSysChar( nChar, nRepCnt, nFlags );
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnSysChar( nChar, nRepCnt, nFlags );
		}
	}
}


void CChildFrameWndBase::OnContextMenu( CWnd *pWnd, CPoint point )
{
	CWnd::OnContextMenu( pWnd, point );
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnContextMenu( CTPoint<int>( point.x, point.y ) );
		}
	}
}


BOOL CChildFrameWndBase::OnEraseBkgnd( CDC* pDC )
{
	return FALSE;
}


void CChildFrameWndBase::OnPaint() 
{
	if ( bWasResized )
	{
		AlignWndAspect();
		bWasResized = false;
	}

	OnPreDrawChildFrameWnd();
	//
	CPaintDC dc( this );

	IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState();
	if ( pActiveInputState )
	{
		pActiveInputState->Draw( &dc );
	}
	//
	if ( bRenderEnabled )
	{
		OnDrawChildFrameWnd();
	}
	//
	if ( pActiveInputState )
	{
		pActiveInputState->PostDraw( &dc );
	}
	if ( GetFocus() == this )
	{
		DrawFocus( &dc );
	}
	if ( bShowStatistic )
	{
		DrawStatistic( &dc );
	}
	CRect size;
	GetClientRect( size );
	//
	OnResizeChildFrameWnd( size.Width(), size.Height() );
}


void CChildFrameWndBase::OnSize( unsigned nType, int cx, int cy )
{
	if ( bIsSettingUp )
		return;

	CWnd::OnSize( nType, cx, cy );

	CRect size;
	GetClientRect( size );
	rectMain = CRect( 0, 0, cx + 4, cy + 4 );
	//
	OnResizeChildFrameWnd( size.Width(), size.Height() );
	if( bEnableScroll )
	{
		const int nGap = 16;
		SetScrollRange( SB_HORZ, -nGap, 1024 - cx + 1 + nGap, TRUE );
		SetScrollRange( SB_VERT, -nGap, 768 - cy + 1 + nGap, TRUE );

		rectMain.bottom += nGap;
		rectMain.right += nGap;
	}

	bWasResized = true;
}


void CChildFrameWndBase::AlignWndAspect()
{
	bIsSettingUp = true;

	const float fDefaultAspect = 0.75f;
	//const int nFrameWidth = 10;

	if ( rectMain.Width() > 0 )
	{
		const float fRectAspect = (float)rectMain.Height() / rectMain.Width();
		if ( fRectAspect < fDefaultAspect )	//	horizontal layout
		{
			const int nMainWidth = rectMain.Height() / fDefaultAspect;
			const int nBorderWidth = ( rectMain.Width() - nMainWidth ) / 2;
			rectBorder1 = CRect( rectMain.left, rectMain.top, rectMain.left + nBorderWidth, rectMain.bottom );
			rectBorder2 = CRect( rectMain.right - nBorderWidth, rectMain.top, rectMain.right, rectMain.bottom );
			rectWindow = CRect( rectBorder1.right, rectBorder1.top, rectBorder2.left, rectBorder2.bottom );
		}
		else	// vertical layout
		{
			const int nMainHeight = rectMain.Width() * fDefaultAspect;
			const int nBorderHeight = ( rectMain.Height() - nMainHeight ) / 2;
			rectBorder1 = CRect( rectMain.left, rectMain.top, rectMain.right, rectMain.top + nBorderHeight );
			rectBorder2 = CRect( rectMain.left, rectMain.bottom - nBorderHeight, rectMain.right, rectMain.bottom );
			rectWindow = CRect( rectBorder1.left, rectBorder1.bottom, rectBorder2.right, rectBorder2.top );
		}

		if ( bShowMovieBorders )
		{
			MoveWindow( rectMain );

			CPaintDC dc( this );
			dc.FillSolidRect( rectMain, RGB(127, 127, 127) );
			MoveWindow( rectWindow );
		}
		else
		{
			MoveWindow( rectMain );
		}
	}

	bIsSettingUp = false;
}


void CChildFrameWndBase::SetUpdateSceneTimer()
{
	KillUpdateSceneTimer();
	if ( nUpdateSceneTimerInterval != 0 )
	{
		nUpdateSceneTimer = SetTimer( GetUpdateSceneTimerID(), nUpdateSceneTimerInterval, 0 );
		if ( nUpdateSceneTimer == 0 )
		{
			NI_ASSERT( 0, "CChildFrameWndBase::SetUpdateSceneTimer() Can't create timer" );
		}
	}
}


void CChildFrameWndBase::KillUpdateSceneTimer()
{
	if ( nUpdateSceneTimer != 0 )
	{
		KillTimer( nUpdateSceneTimer );
	}
	nUpdateSceneTimer = 0;
}


void CChildFrameWndBase::OnUpdateSceneTimer()
{
	if ( GetFocus() == this )
	{
		if ( bRunModeEnabled || bGameInputEnabled )
		{
			AfxGetApp()->OnIdle( 0 );
			NMainLoop::StepApp( ::GetActiveWindow() == AfxGetMainWnd()->GetSafeHwnd() );
		}
		else
		{
			Singleton<IGameTimer>()->Update( GetCurrentTimeMilliseconds() );
			RedrawWindow( 0, 0, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE );
		}
	}
}


void CChildFrameWndBase::RemoveInput()
{
	MSG msg;
	PeekMessage( &msg, GetSafeHwnd(), WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE );
	PeekMessage( &msg, GetSafeHwnd(), WM_KEYFIRST, WM_KEYLAST, PM_REMOVE );
}


void CChildFrameWndBase::EnableInput( uint32_t dwData )
{
	DebugTrace( "CChildFrameWndBase::EnableInput( %d )", dwData );
	bInputEnabled = ( dwData != 0 );
}


void CChildFrameWndBase::EnableAutoUpdate( uint32_t dwData )
{
	if ( dwData != 0 )
	{
		nUpdateSceneTimerInterval = dwData;
		SetUpdateSceneTimer();
	}
	else
	{
		KillUpdateSceneTimer();
	}
}


void CChildFrameWndBase::EnableRunMode( uint32_t dwData )
{
	bRunModeEnabled = ( dwData != 0 );
	if ( bRunModeEnabled )
	{
		NInput::InitInput( m_hWnd, false, true );
		NInput::SetSection( "editor_mapeditor" );
	}
	else
	{
		NInput::DoneInput();
	}
	//
	EnableAutoUpdate( dwData );
}


void CChildFrameWndBase::EnableGameInput( IInterfaceCommand *pInterfaceCommand )
{
	NInput::InitInput( m_hWnd, false, true );
	NInput::SetSection( "editor_mapeditor" );
	if ( pInterfaceCommand != 0 )
	{
		NMainLoop::Command( pInterfaceCommand );
	}
	bGameInputEnabled = true;
	EnableAutoUpdate( DEFAULT_REFRESH_RATE );
}


void CChildFrameWndBase::DisableGameInput()
{
	NInput::DoneInput();
	NMainLoop::ResetStack();
	bGameInputEnabled = false;
	EnableAutoUpdate( 0 );
}


void CChildFrameWndBase::EnableRender( uint32_t dwData )
{
	bRenderEnabled = ( dwData != 0 );
}


void CChildFrameWndBase::EnableScrollbars( uint32_t dwData )
{
	if ( bEnableScroll != ( dwData > 0 ) )
	{
		bEnableScroll = ( dwData > 0 );
		if ( bEnableScroll ) 
		{
			ModifyStyle( 0, WS_VSCROLL | WS_HSCROLL, 1 );
			ShowScrollBar( SB_BOTH, true );
		}
		else
		{
			ModifyStyle( WS_VSCROLL | WS_HSCROLL, 0, 1 );
			ShowScrollBar( SB_BOTH, false );
		}
		// необходимо для того чобы показать изменения сразу
		WINDOWPLACEMENT windowPlacement;
		windowPlacement.length = sizeof( WINDOWPLACEMENT );
		//
		GetWindowPlacement( &windowPlacement );
		windowPlacement.showCmd = SW_SHOWMINIMIZED;
		SetWindowPlacement( &windowPlacement );
		RedrawWindow();
		//
		windowPlacement.showCmd = SW_SHOWMAXIMIZED;
		SetWindowPlacement( &windowPlacement );
		RedrawWindow();
	}
}


void CChildFrameWndBase::EnableMouseCapture( uint32_t dwData )
{
	if ( dwData > 0 )
	{
		SetCapture();
	}
	else
	{
		ReleaseCapture();
	}
}


void CChildFrameWndBase::GetDimensions( uint32_t dwData )
{
	if ( dwData != 0 )
	{
		CTPoint<int> *pDimensions = reinterpret_cast<CTPoint<int>*>( dwData );
		CRect clientRect;
		GetClientRect( &clientRect );
		pDimensions->x = clientRect.Width();
		pDimensions->y = clientRect.Height();
	}
}


bool CChildFrameWndBase::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	switch( nCommandID )
	{
	case ID_SCENE_UPDATE:
		RedrawWindow();
		return true;
	case ID_SCENE_REMOVE_INPUT:
		RemoveInput();
		return true;
	case ID_SCENE_ENABLE_INPUT:
		EnableInput( dwData );
		return true;
	case ID_SCENE_GET_DIMENSIONS:
		GetDimensions( dwData );
		return true;
	case ID_SCENE_ENABLE_RUN_MODE:
		EnableRunMode( dwData );
		return true;
	case ID_SCENE_ENABLE_GAME_INPUT:
		EnableGameInput( reinterpret_cast<IInterfaceCommand*>( dwData ) );
		return true;
	case ID_SCENE_DISABLE_GAME_INPUT:
		DisableGameInput();
		return true;
	case ID_SCENE_ENABLE_RENDER:
		EnableRender( dwData );
		return true;
	case ID_SCENE_ENABLE_UPDATE:
		bEnableSceneUpdate = ( dwData > 0 );
		return true;
	case ID_SCENE_ENABLE_SCROLLBARS:
		EnableScrollbars( dwData );
		return true;
	case ID_SCENE_ENABLE_MOUSE_CAPTURE:
		EnableMouseCapture( dwData );
		return true;
	case ID_SCENE_SET_FOCUS:
		if ( GetFocus() != this )
		{
			SetFocus();
		}
		return true;
	case ID_SCENE_GET_FOCUS:
		return ( GetFocus() == this );
	case ID_SCENE_SHOW_STATISTIC:
		bShowStatistic = !bShowStatistic;
		RedrawWindow();
		if ( dwData != 0 )
		{
			( *( reinterpret_cast<bool*>( dwData ) ) ) = bShowStatistic;
		}
		return true;
	case ID_SCENE_SHOW_MOVIE_BORDERS:
		bShowMovieBorders = !bShowMovieBorders;
		RedrawWindow();
		if ( dwData != 0 )
		{
			( *( reinterpret_cast<bool*>( dwData ) ) ) = bShowMovieBorders;
		}
		return true;
	case ID_SCENE_RESIZE_TO_GAME:
		{
			bShowMovieBorders = (bool)(dwData);
			bWasResized = true;
			RedrawWindow();
			return true;
		}
	case ID_SELECTION_NEW:
		OnKeyDown( VK_INSERT, 0, 0 );
		return true;
	case ID_SELECTION_CLEAR:
		OnKeyDown( VK_DELETE, 0, 0 );
		return true;
	default:
		return false;
	}
	return false;
}


bool CChildFrameWndBase::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CChildFrameWndBase::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CChildFrameWndBase::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
	case ID_SCENE_UPDATE:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	case ID_SCENE_REMOVE_INPUT:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	case ID_SCENE_ENABLE_INPUT:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	case ID_SCENE_GET_DIMENSIONS:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	case ID_SCENE_ENABLE_RUN_MODE:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	case ID_SCENE_ENABLE_GAME_INPUT:
	case ID_SCENE_DISABLE_GAME_INPUT:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	case ID_SCENE_ENABLE_RENDER:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	case ID_SCENE_ENABLE_UPDATE:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	case ID_SCENE_ENABLE_SCROLLBARS:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	case ID_SCENE_ENABLE_MOUSE_CAPTURE:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	case ID_SCENE_SET_FOCUS:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	case ID_SCENE_GET_FOCUS:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	case ID_SCENE_SHOW_STATISTIC:
		( *pbEnable ) = true;
		( *pbCheck ) = bShowStatistic;
		return true;
	case ID_SCENE_SHOW_MOVIE_BORDERS:
		( *pbEnable ) = true;
		( *pbCheck ) = bShowMovieBorders;
		return true;
	case ID_SELECTION_NEW:
	case ID_SELECTION_CLEAR:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	default:
		return false;
	}
	return false;
}


void CChildFrameWndBase::OnHScroll( unsigned nSBCode, unsigned nPos, CScrollBar* pScrollBar )
{
	if ( bEnableScroll )
	{
		CWnd::OnHScroll( nSBCode, nPos, pScrollBar );

		SCROLLINFO si;
		si.cbSize = sizeof(si);
		GetScrollInfo( SB_HORZ, &si );

		int nStep = (si.nMax-si.nMin)/20;
		if ( nStep == 0 )
			nStep = 1;

		switch( nSBCode )
		{
		case SB_RIGHT:
			nPos = si.nMax;
			break;
		case SB_LEFT:
			nPos = si.nMin;
			break;
		case SB_PAGERIGHT:
		case SB_LINERIGHT:
			nPos = si.nPos + nStep;
			break;
		case SB_PAGELEFT:
		case SB_LINELEFT:
			nPos = si.nPos - nStep;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			break;
		default:
			return;
		}
		if ( (int)nPos < (int)si.nMin )
			nPos = si.nMin;
		else
			if ( (int)nPos > (int)si.nMax )
				nPos = si.nMax;

		if ( nPos != si.nPos )
		{
			SetScrollPos( SB_HORZ, nPos, TRUE );

			int x, y;
			Singleton<IUIInitialization>()->GetVirtualScreenController()->GetOrigin( &x, &y );
			if ( x != nPos )
			{
				Singleton<IUIInitialization>()->GetVirtualScreenController()->SetOrigin( nPos, y );
				RedrawWindow();
			}
		}
	}
}


void CChildFrameWndBase::OnVScroll( unsigned nSBCode, unsigned nPos, CScrollBar* pScrollBar )
{
	if ( bEnableScroll )
	{
		CWnd::OnVScroll( nSBCode, nPos, pScrollBar );

		SCROLLINFO si;
		si.cbSize = sizeof(si);
		GetScrollInfo( SB_VERT, &si );

		int nStep = (si.nMax-si.nMin)/20;
		if ( nStep == 0 )
			nStep = 1;

		switch( nSBCode )
		{
		case SB_BOTTOM:
			nPos = si.nMax;
			break;
		case SB_TOP:
			nPos = si.nMin;
			break;
		case SB_PAGEDOWN:
		case SB_LINEDOWN:
			nPos = si.nPos + nStep;
			break;
		case SB_PAGEUP:
		case SB_LINEUP:
			nPos = si.nPos - nStep;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			break;
		default:
			return;
		}
		if ( (int)nPos < (int)si.nMin )
			nPos = si.nMin;
		else
			if ( (int)nPos > (int)si.nMax )
				nPos = si.nMax;

		if ( nPos != si.nPos )
		{
			SetScrollPos( SB_VERT, nPos, TRUE );

			int x, y;
			Singleton<IUIInitialization>()->GetVirtualScreenController()->GetOrigin( &x, &y );
			if ( y != nPos )
			{
				Singleton<IUIInitialization>()->GetVirtualScreenController()->SetOrigin( x, nPos );
				RedrawWindow();
			}
		}
	}
}

// basement storage  


