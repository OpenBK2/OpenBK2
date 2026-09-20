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
#include "SceneSurface.h"

#include "port/time.h"
#include "port/vkcodes.h"
#include "port/mousekeys.h"
#include "MapEditorLib/WxKeyState.h"

#include <cstdint>

const int CChildFrameWndBase::DEFAULT_REFRESH_RATE = 50;


CChildFrameWndBase::CChildFrameWndBase()
	: pSurface( 0 ),
		bUpdateSceneTimer( false ),
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
		bWasResized( true ),
		rectBorder1( 0, 0, 0, 0 ),
		rectBorder2( 0, 0, 0, 0 ),
		rectWindow( 0, 0, 0, 0 ),
		rectMain( 0, 0, 0, 0 )
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_SCENE, this );
}


CChildFrameWndBase::~CChildFrameWndBase()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_SCENE );
}


void CChildFrameWndBase::Redraw()
{
	if ( pSurface != 0 )
	{
		pSurface->Redraw();
	}
}


bool CChildFrameWndBase::OnCreate( ISceneSurface *_pSurface )
{
	pSurface = _pSurface;
	//
	if ( !OnCreateChildFrameWnd() )
	{
		return false;
	}
	//
	NMainLoop::SetInputEnabled( false );
	//
	return true;
}


void CChildFrameWndBase::OnDestroy()
{
	KillUpdateSceneTimer();
	nUpdateSceneTimerInterval = 0;
	//
	OnDestroyChildFrameWnd();
	// The window is going; nothing below may reach for it any more.
	pSurface = 0;
}


void CChildFrameWndBase::OnTimer()
{
	if ( pSurface == 0 )
	{
		return;
	}
	if ( pSurface->HasFocus() )
	{
		if ( bRunModeEnabled || bGameInputEnabled )
		{
			// The game's step, as the MFC viewport ran it: the application's, not
			// the window's. The game steps as the active application when the
			// active window is the one the viewport is in -- the main window,
			// whichever frame that is.
			//
			// AfxGetApp()->OnIdle( 0 ) used to come first. With no MFC window
			// left, CWinApp::OnIdle had nothing to do: no m_pMainWnd and no frame
			// windows whose command UI to update, and no temporary CWnd maps to
			// empty. It went with MFC.
			NMainLoop::StepApp( pSurface->IsInActiveWindow() );
		}
		else
		{
			Singleton<IGameTimer>()->Update( GetCurrentTimeMilliseconds() );
			pSurface->RedrawWithoutErase();
		}
	}
	else if ( pSurface->IsShown() && bRenderEnabled && !bRunModeEnabled )
	{
		// Resource previews must animate while the browser/property panel has
		// focus. Advance rendering only; game input still requires viewport focus.
		Singleton<IGameTimer>()->Update( GetCurrentTimeMilliseconds() );
		pSurface->RedrawWithoutErase();
	}
}

//DebugTrace( "CChildFrameWndBase::On...(), flags: %u, ( %d, %d )\n", nFlags, point.x, point.y );

void CChildFrameWndBase::OnSetFocus( IWidget *pOldWidget )
{
	DebugTrace( "CChildFrameWndBase::OnSetFocus()\n" );
	Redraw();
	if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
	{
		pActiveInputState->OnSetFocus( pOldWidget );
	}
	NMainLoop::SetInputEnabled( true );
	//
	//Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, this );
}


void CChildFrameWndBase::OnKillFocus( IWidget *pNewWidget )
{
	DebugTrace( "CChildFrameWndBase::OnKillFocus()\n" );
	Redraw();
	if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
	{
		pActiveInputState->OnKillFocus( pNewWidget );
	}
	bInputEnabled = true;
	NMainLoop::SetInputEnabled( false );
}


void CChildFrameWndBase::OnMouseMove( unsigned nFlags, const CTPoint<int> &rPoint )
{
	if ( bInputEnabled )
	{
		if ( ( nFlags & ( MK_LBUTTON | MK_RBUTTON | MK_MBUTTON ) ) && ( pSurface != 0 ) )
		{
			pSurface->Focus();
		}
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnMouseMove( nFlags, rPoint );
		}
	}
	if ( bGameInputEnabled || ( bRunModeEnabled && !bRenderEnabled ) )
	{
		NInput::PostEvent( "win_mouse_move", PackCoords( CVec2( rPoint.x, rPoint.y ) ), nFlags );
	}
}


bool CChildFrameWndBase::OnMouseWheel( unsigned nFlags, short zDelta, const CTPoint<int> &rScreenPoint, bool bDefaultResult )
{
	if ( bDefaultResult )
	{
		if ( bInputEnabled )
		{
			if ( ( nFlags & ( MK_LBUTTON | MK_RBUTTON | MK_MBUTTON ) ) && ( pSurface != 0 ) )
			{
				pSurface->Focus();
			}
			if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
			{
				// IInputState takes the point by non-const reference.
				CTPoint<int> point = rScreenPoint;
				return pActiveInputState->OnMouseWheel( nFlags, zDelta, point );
			}
		}
	}
	return bDefaultResult;
}


void CChildFrameWndBase::OnLButtonDown( unsigned nFlags, const CTPoint<int> &rPoint )
{
	if ( bInputEnabled )
	{
		if ( pSurface != 0 )
		{
			pSurface->Focus();
		}
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnLButtonDown( nFlags, rPoint );
		}
	}
	if ( bGameInputEnabled || ( bRunModeEnabled && !bRenderEnabled ) )
	{
		NInput::PostEvent( "win_left_button_down", PackCoords( CVec2( rPoint.x, rPoint.y ) ), nFlags );
	}
}


void CChildFrameWndBase::OnLButtonUp( unsigned nFlags, const CTPoint<int> &rPoint )
{
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnLButtonUp( nFlags, rPoint );
		}
	}
	if ( bGameInputEnabled || ( bRunModeEnabled && !bRenderEnabled ) )
	{
		NInput::PostEvent( "win_left_button_up", PackCoords( CVec2( rPoint.x, rPoint.y ) ), nFlags );
	}
}


void CChildFrameWndBase::OnLButtonDblClk( unsigned nFlags, const CTPoint<int> &rPoint )
{
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnLButtonDblClk( nFlags, rPoint );
		}
	}
	if ( bGameInputEnabled || ( bRunModeEnabled && !bRenderEnabled ) )
	{
		NInput::PostEvent( "win_left_button_dblclk", PackCoords( CVec2( rPoint.x, rPoint.y ) ), nFlags );
	}
}


void CChildFrameWndBase::OnRButtonDown( unsigned nFlags, const CTPoint<int> &rPoint )
{
	if ( bInputEnabled )
	{
		if ( pSurface != 0 )
		{
			pSurface->Focus();
		}
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnRButtonDown( nFlags, rPoint );
		}
	}
	if ( bGameInputEnabled || ( bRunModeEnabled && !bRenderEnabled ) )
	{
		NInput::PostEvent( "win_right_button_down", PackCoords( CVec2( rPoint.x, rPoint.y ) ), nFlags );
	}
}


void CChildFrameWndBase::OnRButtonUp( unsigned nFlags, const CTPoint<int> &rPoint )
{
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnRButtonUp( nFlags, rPoint );
		}
	}
	if ( bGameInputEnabled || ( bRunModeEnabled && !bRenderEnabled ) )
	{
		NInput::PostEvent( "win_right_button_up", PackCoords( CVec2( rPoint.x, rPoint.y ) ), nFlags );
	}
}


void CChildFrameWndBase::OnRButtonDblClk( unsigned nFlags, const CTPoint<int> &rPoint )
{
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnRButtonDblClk( nFlags, rPoint );
		}
	}
	if ( bGameInputEnabled || ( bRunModeEnabled && !bRenderEnabled ) )
	{
		NInput::PostEvent( "win_right_button_dblclk", PackCoords( CVec2( rPoint.x, rPoint.y ) ), nFlags );
	}
}


void CChildFrameWndBase::OnMButtonDown( unsigned nFlags, const CTPoint<int> &rPoint )
{
	if ( bInputEnabled )
	{
		if ( pSurface != 0 )
		{
			pSurface->Focus();
		}
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnMButtonDown( nFlags, rPoint );
		}
	}
}


void CChildFrameWndBase::OnMButtonUp( unsigned nFlags, const CTPoint<int> &rPoint )
{
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnMButtonUp( nFlags, rPoint );
		}
	}
}


void CChildFrameWndBase::OnMButtonDblClk( unsigned nFlags, const CTPoint<int> &rPoint )
{
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnMButtonDblClk( nFlags, rPoint );
		}
	}
}


void CChildFrameWndBase::OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags )
{
	if ( nChar == 'Q' )
	{
		if ( NWxKey::IsDown( VK_SHIFT ) &&
				 NWxKey::IsDown( VK_CONTROL )	)
		{
			bShowStatistic = !bShowStatistic;
			Redraw();
		}
	}
	else if ( nChar == 'W' )
	{
		if ( NWxKey::IsDown( VK_SHIFT ) &&
				 NWxKey::IsDown( VK_CONTROL )	)
		{
			bShowMovieBorders = !bShowMovieBorders;
			bWasResized = true;
			Redraw();
		}
	}
	if ( bInputEnabled )
	{
		if ( pSurface != 0 )
		{
			pSurface->Focus();
		}
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
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnKeyUp( nChar, nRepCnt, nFlags );
		}
	}
}


void CChildFrameWndBase::OnContextMenu( const CTPoint<int> &rScreenPoint )
{
	if ( bInputEnabled )
	{
		if ( IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState() )
		{
			pActiveInputState->OnContextMenu( rScreenPoint );
		}
	}
}


bool CChildFrameWndBase::BeginPaint()
{
	// Place in AlignWndAspect can synchronously send another paint. The surface
	// validates that nested paint without resizing or entering the renderer
	// again while the outer paint is still arranging the preview window.
	if ( bIsSettingUp || ( pSurface == 0 ) )
	{
		return false;
	}
	if ( bWasResized )
	{
		bWasResized = false;
		AlignWndAspect();
	}

	OnPreDrawChildFrameWnd();
	return true;
}


void CChildFrameWndBase::Paint( IPaintContext *pPaintContext )
{
	// The boundary: everything below IInputState draws through the context and
	// names no toolkit. DrawFocus and DrawStatistic are the front-end's own
	// marks, drawn on the same context after the state's.
	IInputState *pActiveInputState = Singleton<IEditorContainer>()->GetActiveInputState();
	if ( pActiveInputState )
	{
		pActiveInputState->Draw( pPaintContext );
	}
	//
	if ( bRenderEnabled )
	{
		OnDrawChildFrameWnd();
	}
	//
	if ( pActiveInputState )
	{
		pActiveInputState->PostDraw( pPaintContext );
	}
	if ( pSurface == 0 )
	{
		return;
	}
	if ( pSurface->HasFocus() )
	{
		DrawFocus( pPaintContext );
	}
	if ( bShowStatistic )
	{
		DrawStatistic( pPaintContext );
	}
	const CTPoint<int> size = pSurface->GetClientSize();
	//
	OnResizeChildFrameWnd( size.x, size.y );
}


void CChildFrameWndBase::OnSize( int cx, int cy )
{
	if ( bIsSettingUp || ( pSurface == 0 ) )
	{
		return;
	}

	const CTPoint<int> size = pSurface->GetClientSize();
	rectMain = CTRect<int>( 0, 0, cx + 4, cy + 4 );
	//
	OnResizeChildFrameWnd( size.x, size.y );
	if( bEnableScroll )
	{
		const int nGap = 16;
		pSurface->SetScrollBarRange( false, -nGap, 1024 - cx + 1 + nGap );
		pSurface->SetScrollBarRange( true, -nGap, 768 - cy + 1 + nGap );

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
			rectBorder1 = CTRect<int>( rectMain.left, rectMain.top, rectMain.left + nBorderWidth, rectMain.bottom );
			rectBorder2 = CTRect<int>( rectMain.right - nBorderWidth, rectMain.top, rectMain.right, rectMain.bottom );
			rectWindow = CTRect<int>( rectBorder1.right, rectBorder1.top, rectBorder2.left, rectBorder2.bottom );
		}
		else	// vertical layout
		{
			const int nMainHeight = rectMain.Width() * fDefaultAspect;
			const int nBorderHeight = ( rectMain.Height() - nMainHeight ) / 2;
			rectBorder1 = CTRect<int>( rectMain.left, rectMain.top, rectMain.right, rectMain.top + nBorderHeight );
			rectBorder2 = CTRect<int>( rectMain.left, rectMain.bottom - nBorderHeight, rectMain.right, rectMain.bottom );
			rectWindow = CTRect<int>( rectBorder1.left, rectBorder1.bottom, rectBorder2.right, rectBorder2.top );
		}

		if ( bShowMovieBorders )
		{
			pSurface->Place( rectMain );

			// 0x00BBGGRR: RGB( 127, 127, 127 ).
			pSurface->Fill( rectMain, 0x007F7F7F );
			pSurface->Place( rectWindow );
		}
		else
		{
			pSurface->Place( rectMain );
		}
	}

	bIsSettingUp = false;
}


void CChildFrameWndBase::SetUpdateSceneTimer()
{
	KillUpdateSceneTimer();
	if ( ( nUpdateSceneTimerInterval != 0 ) && ( pSurface != 0 ) )
	{
		bUpdateSceneTimer = pSurface->StartUpdateTimer( nUpdateSceneTimerInterval );
		if ( !bUpdateSceneTimer )
		{
			NI_ASSERT( 0, "CChildFrameWndBase::SetUpdateSceneTimer() Can't create timer" );
		}
	}
}


void CChildFrameWndBase::KillUpdateSceneTimer()
{
	if ( bUpdateSceneTimer && ( pSurface != 0 ) )
	{
		pSurface->StopUpdateTimer();
	}
	bUpdateSceneTimer = false;
}


void CChildFrameWndBase::RemoveInput()
{
	if ( pSurface == 0 )
	{
		return;
	}
	pSurface->DiscardPendingInput();
}


void CChildFrameWndBase::EnableInput( uintptr_t dwData )
{
	DebugTrace( "CChildFrameWndBase::EnableInput( %d )", dwData );
	bInputEnabled = ( dwData != 0 );
}


void CChildFrameWndBase::EnableAutoUpdate( uintptr_t dwData )
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


void CChildFrameWndBase::EnableRunMode( uintptr_t dwData )
{
	bRunModeEnabled = ( dwData != 0 );
	if ( bRunModeEnabled )
	{
		NInput::InitInput( ( pSurface != 0 ) ? pSurface->GetHandle() : 0, true );
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
	NInput::InitInput( ( pSurface != 0 ) ? pSurface->GetHandle() : 0, true );
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
	// Camera control can suppress editor clicks until its key-up event. Closing
	// a map discards that input session, so do not carry the lock into the next map.
	bInputEnabled = true;
	EnableAutoUpdate( 0 );
}


void CChildFrameWndBase::EnableRender( uintptr_t dwData )
{
	bRenderEnabled = ( dwData != 0 );
}


void CChildFrameWndBase::EnableScrollbars( uintptr_t dwData )
{
	if ( bEnableScroll != ( dwData > 0 ) )
	{
		bEnableScroll = ( dwData > 0 );
		if ( pSurface != 0 )
		{
			pSurface->ShowScrollBars( bEnableScroll );
		}
	}
}


void CChildFrameWndBase::EnableMouseCapture( uintptr_t dwData )
{
	if ( pSurface != 0 )
	{
		pSurface->SetMouseCapture( dwData > 0 );
	}
}


void CChildFrameWndBase::GetDimensions( uintptr_t dwData )
{
	if ( ( dwData != 0 ) && ( pSurface != 0 ) )
	{
		CTPoint<int> *pDimensions = reinterpret_cast<CTPoint<int>*>( dwData );
		const CTPoint<int> size = pSurface->GetClientSize();
		pDimensions->x = size.x;
		pDimensions->y = size.y;
	}
}


bool CChildFrameWndBase::HandleCommand( unsigned nCommandID, uintptr_t dwData )
{
	switch( nCommandID )
	{
	case ID_SCENE_UPDATE:
		Redraw();
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
		if ( ( pSurface != 0 ) && !pSurface->HasFocus() )
		{
			pSurface->Focus();
		}
		return true;
	case ID_SCENE_GET_FOCUS:
		return ( pSurface != 0 ) && pSurface->HasFocus();
	case ID_SCENE_SHOW_STATISTIC:
		bShowStatistic = !bShowStatistic;
		Redraw();
		if ( dwData != 0 )
		{
			( *( reinterpret_cast<bool*>( dwData ) ) ) = bShowStatistic;
		}
		return true;
	case ID_SCENE_SHOW_MOVIE_BORDERS:
		bShowMovieBorders = !bShowMovieBorders;
		Redraw();
		if ( dwData != 0 )
		{
			( *( reinterpret_cast<bool*>( dwData ) ) ) = bShowMovieBorders;
		}
		return true;
	case ID_SCENE_RESIZE_TO_GAME:
		{
			bShowMovieBorders = (bool)(dwData);
			bWasResized = true;
			Redraw();
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


void CChildFrameWndBase::OnHScroll( EScrollAction eAction, unsigned nPos )
{
	if ( bEnableScroll && ( pSurface != 0 ) )
	{
		int nMin = 0;
		int nMax = 0;
		int nCurrentPos = 0;
		pSurface->GetScrollBarState( false, &nMin, &nMax, &nCurrentPos );

		int nStep = (nMax-nMin)/20;
		if ( nStep == 0 )
			nStep = 1;

		switch( eAction )
		{
		case SCROLL_TO_END:
			nPos = nMax;
			break;
		case SCROLL_TO_START:
			nPos = nMin;
			break;
		case SCROLL_PAGE_FORWARD:
		case SCROLL_LINE_FORWARD:
			nPos = nCurrentPos + nStep;
			break;
		case SCROLL_PAGE_BACK:
		case SCROLL_LINE_BACK:
			nPos = nCurrentPos - nStep;
			break;
		case SCROLL_THUMB:
			break;
		default:
			return;
		}
		if ( (int)nPos < nMin )
			nPos = nMin;
		else
			if ( (int)nPos > nMax )
				nPos = nMax;

		if ( nPos != nCurrentPos )
		{
			pSurface->SetScrollBarPos( false, nPos );

			int x, y;
			Singleton<IUIInitialization>()->GetVirtualScreenController()->GetOrigin( &x, &y );
			if ( x != nPos )
			{
				Singleton<IUIInitialization>()->GetVirtualScreenController()->SetOrigin( nPos, y );
				Redraw();
			}
		}
	}
}


void CChildFrameWndBase::OnVScroll( EScrollAction eAction, unsigned nPos )
{
	if ( bEnableScroll && ( pSurface != 0 ) )
	{
		int nMin = 0;
		int nMax = 0;
		int nCurrentPos = 0;
		pSurface->GetScrollBarState( true, &nMin, &nMax, &nCurrentPos );

		int nStep = (nMax-nMin)/20;
		if ( nStep == 0 )
			nStep = 1;

		switch( eAction )
		{
		case SCROLL_TO_END:
			nPos = nMax;
			break;
		case SCROLL_TO_START:
			nPos = nMin;
			break;
		case SCROLL_PAGE_FORWARD:
		case SCROLL_LINE_FORWARD:
			nPos = nCurrentPos + nStep;
			break;
		case SCROLL_PAGE_BACK:
		case SCROLL_LINE_BACK:
			nPos = nCurrentPos - nStep;
			break;
		case SCROLL_THUMB:
			break;
		default:
			return;
		}
		if ( (int)nPos < nMin )
			nPos = nMin;
		else
			if ( (int)nPos > nMax )
				nPos = nMax;

		if ( nPos != nCurrentPos )
		{
			pSurface->SetScrollBarPos( true, nPos );

			int x, y;
			Singleton<IUIInitialization>()->GetVirtualScreenController()->GetOrigin( &x, &y );
			if ( y != nPos )
			{
				Singleton<IUIInitialization>()->GetVirtualScreenController()->SetOrigin( x, nPos );
				Redraw();
			}
		}
	}
}

// basement storage


