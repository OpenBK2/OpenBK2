#include "stdafx.h"

#include "SceneSurface.h"


#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxPaintContext.h"
#include "ChildFrameWndBase.h"

#include <wx/dcclient.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <wx/window.h>

#include <functional>

// The viewport's window in wx: a wxWindow the renderer draws in, inside a
// CWxHostWindow made as the document window's AFX_IDW_PANE_FIRST, as the MFC
// view was, so either frame lays it out the same way.
//
// **Why the input is taken as Win32 messages.** The input states' contract is
// MFC's handlers: MK_ flags, virtual key codes, a key message's repeat count,
// and the window's default handling done before the handler runs -- which is
// when a right button's release becomes WM_CONTEXTMENU, so a state hears of
// the context menu before the release. wx keeps none of those apart: a skipped
// event gets its default handling after the handler, not before. The viewport
// is a Direct3D 9 window and exists only on Windows, so its input is taken
// here as the messages themselves, in MSWHandleMessage, each given its default
// handling first and then passed on as MFC's message map passed it. Painting,
// sizing, focus and the timer are wx's own events.
//
// Two of the reasons this used to give are gone, and what is left is worth
// knowing when the viewport does move: no state ever distinguished
// WM_SYSKEYDOWN from WM_KEYDOWN, or read WM_CHAR, so those handlers have been
// deleted. What a portable surface would still have to reproduce is the MK_
// flags, the repeat count, and the context menu arriving before the button
// release -- and wx has all three, in wxMouseEvent's modifiers, one event per
// repeat, and wxEVT_CONTEXT_MENU.

namespace
{
	// A wx window, as the focus notifications carry the other window, is a
	// CWxWindowWidget (WxWidget.h). The input states only pass it on.


	class CUpdateTimer : public wxTimer
	{
		std::function<void()> tick;

	public:
		explicit CUpdateTimer( std::function<void()> _tick ) : tick( std::move( _tick ) ) {}
		virtual void Notify() { tick(); }
	};


	inline CTPoint<int> PointFromLParam( WXLPARAM lParam )
	{
		// GET_X_LPARAM and GET_Y_LPARAM, without <windowsx.h>, whose macros
		// rename wx methods.
		return CTPoint<int>( static_cast<short>( LOWORD( lParam ) ), static_cast<short>( HIWORD( lParam ) ) );
	}


	// The window the renderer draws in.
	class CSceneWxWindow : public wxWindow
	{
		CChildFrameWndBase *pCore;

		void OnPaint( wxPaintEvent & )
		{
			if ( !pCore->BeginPaint() )
			{
				// A nested paint while the window moves itself, or one after the
				// viewport let go: validated, and nothing more.
				wxPaintDC dc( this );
				return;
			}
			wxPaintDC dc( this );
			CWxPaintContext paintContext( dc, this );
			pCore->Paint( &paintContext );
		}

		void OnSize( wxSizeEvent & )
		{
			if ( pCore->IsSettingUp() )
			{
				return;
			}
			const wxSize size = GetClientSize();
			pCore->OnSize( size.x, size.y );
		}

		void OnSetFocus( wxFocusEvent &rEvent )
		{
			rEvent.Skip();
			CWxWindowWidget oldWidget( rEvent.GetWindow() );
			pCore->OnSetFocus( &oldWidget );
		}

		void OnKillFocus( wxFocusEvent &rEvent )
		{
			rEvent.Skip();
			CWxWindowWidget newWidget( rEvent.GetWindow() );
			pCore->OnKillFocus( &newWidget );
		}

	protected:
		virtual bool MSWHandleMessage( WXLRESULT *pResult, WXUINT nMessage, WXWPARAM wParam, WXLPARAM lParam )
		{
			const unsigned nFlags = static_cast<unsigned>( wParam );
			switch ( nMessage )
			{
				case WM_MOUSEMOVE:
				case WM_LBUTTONDOWN:
				case WM_LBUTTONUP:
				case WM_LBUTTONDBLCLK:
				case WM_RBUTTONDOWN:
				case WM_RBUTTONUP:
				case WM_RBUTTONDBLCLK:
				case WM_MBUTTONDOWN:
				case WM_MBUTTONUP:
				case WM_MBUTTONDBLCLK:
				{
					( *pResult ) = MSWDefWindowProc( nMessage, wParam, lParam );
					const CTPoint<int> point = PointFromLParam( lParam );
					switch ( nMessage )
					{
						case WM_MOUSEMOVE:			pCore->OnMouseMove( nFlags, point ); break;
						case WM_LBUTTONDOWN:		pCore->OnLButtonDown( nFlags, point ); break;
						case WM_LBUTTONUP:			pCore->OnLButtonUp( nFlags, point ); break;
						case WM_LBUTTONDBLCLK:	pCore->OnLButtonDblClk( nFlags, point ); break;
						case WM_RBUTTONDOWN:		pCore->OnRButtonDown( nFlags, point ); break;
						case WM_RBUTTONUP:			pCore->OnRButtonUp( nFlags, point ); break;
						case WM_RBUTTONDBLCLK:	pCore->OnRButtonDblClk( nFlags, point ); break;
						case WM_MBUTTONDOWN:		pCore->OnMButtonDown( nFlags, point ); break;
						case WM_MBUTTONUP:			pCore->OnMButtonUp( nFlags, point ); break;
						case WM_MBUTTONDBLCLK:	pCore->OnMButtonDblClk( nFlags, point ); break;
					}
					return true;
				}
				case WM_MOUSEWHEEL:
				{
					// What the default handling answers is what decides whether the
					// input state hears of the wheel at all, as CWnd::OnMouseWheel's
					// Default() did.
					const WXLRESULT nDefault = MSWDefWindowProc( nMessage, wParam, lParam );
					const bool bResult = pCore->OnMouseWheel( LOWORD( wParam ), static_cast<short>( HIWORD( wParam ) ), PointFromLParam( lParam ), nDefault != 0 );
					( *pResult ) = bResult ? TRUE : FALSE;
					return true;
				}
				case WM_KEYDOWN:
				case WM_KEYUP:
				case WM_CHAR:
				case WM_SYSKEYDOWN:
				case WM_SYSKEYUP:
				case WM_SYSCHAR:
				{
					( *pResult ) = MSWDefWindowProc( nMessage, wParam, lParam );
					const unsigned nChar = static_cast<unsigned>( wParam );
					const unsigned nRepCnt = LOWORD( lParam );
					const unsigned nKeyFlags = HIWORD( lParam );
					// The character and system-key messages are still taken and given
					// their default handling, because that is what decides whether wx
					// sees them, and taking them is what the MFC message map did. No
					// input state ever listened to them: OnChar, OnSysKeyDown,
					// OnSysKeyUp and OnSysChar were carried the whole length of the
					// chain and overridden by nothing.
					switch ( nMessage )
					{
						case WM_KEYDOWN:		pCore->OnKeyDown( nChar, nRepCnt, nKeyFlags ); break;
						case WM_KEYUP:			pCore->OnKeyUp( nChar, nRepCnt, nKeyFlags ); break;
					}
					return true;
				}
				case WM_CONTEXTMENU:
				{
					( *pResult ) = MSWDefWindowProc( nMessage, wParam, lParam );
					pCore->OnContextMenu( PointFromLParam( lParam ) );
					return true;
				}
				case WM_HSCROLL:
				case WM_VSCROLL:
				{
					// Taken whether or not the viewport scrolls, as the MFC handlers
					// took them, and given the default handling only when it does.
					( *pResult ) = 0;
					if ( pCore->IsScrollEnabled() )
					{
						( *pResult ) = MSWDefWindowProc( nMessage, wParam, lParam );
						if ( nMessage == WM_HSCROLL )
						{
							pCore->OnHScroll( LOWORD( wParam ), HIWORD( wParam ) );
						}
						else
						{
							pCore->OnVScroll( LOWORD( wParam ), HIWORD( wParam ) );
						}
					}
					return true;
				}
			}
			return wxWindow::MSWHandleMessage( pResult, nMessage, wParam, lParam );
		}

	public:
		CSceneWxWindow( wxWindow *pParent, CChildFrameWndBase *_pCore )
			// wxBORDER_SUNKEN is WS_EX_CLIENTEDGE, the MFC view's edge, which
			// AlignWndAspect counts on: it sizes the window four pixels larger
			// than the client. wxWANTS_CHARS so no key is taken for navigation
			// before the viewport sees it. Made with no size, as the MFC view was,
			// so the renderer starts on the client the frame then gives it.
			: wxWindow( pParent, wxID_ANY, wxDefaultPosition, wxSize( 0, 0 ), wxBORDER_SUNKEN | wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE ),
				pCore( _pCore )
		{
			// Nothing erases: the renderer covers the client on every paint, as the
			// MFC view's OnEraseBkgnd said.
			SetBackgroundStyle( wxBG_STYLE_PAINT );
			Bind( wxEVT_PAINT, &CSceneWxWindow::OnPaint, this );
			Bind( wxEVT_ERASE_BACKGROUND, []( wxEraseEvent & ) {} );
			Bind( wxEVT_SIZE, &CSceneWxWindow::OnSize, this );
			Bind( wxEVT_SET_FOCUS, &CSceneWxWindow::OnSetFocus, this );
			Bind( wxEVT_KILL_FOCUS, &CSceneWxWindow::OnKillFocus, this );
			// wx asks that a window which captures the mouse hear of losing it.
			Bind( wxEVT_MOUSE_CAPTURE_LOST, []( wxMouseCaptureLostEvent & ) {} );
		}
	};


	class CWxSceneSurface : public CWxHostWindow, public ISceneSurface
	{
		CChildFrameWndBase *pCore = nullptr;
		CSceneWxWindow *pWindow = nullptr;
		// Whether the core has the window: from its OnCreate to its OnDestroy.
		bool bCoreHasWindow = false;
		CUpdateTimer updateTimer;

	protected:
		// The window's destruction, however it comes: the timer stops and the
		// core lets go while the window is still there, as the MFC view's
		// OnDestroy did before its handle went.
		virtual void BeforeTearDown()
		{
			updateTimer.Stop();
			if ( bCoreHasWindow )
			{
				bCoreHasWindow = false;
				pCore->OnDestroy();
			}
		}

	public:
		CWxSceneSurface()
			: updateTimer( [this]()
				{
					if ( bCoreHasWindow )
					{
						pCore->OnTimer();
					}
				} )
		{
		}

		virtual ~CWxSceneSurface()
		{
			// Here and not only in the base destructor, where BeforeTearDown is
			// no longer this class's.
			TearDownWx();
		}

		// ISceneSurface
		virtual bool Create( IFrameWindow *pFrame, CChildFrameWndBase *_pCore )
		{
			pCore = _pCore;
			if ( !CreateHost( pFrame ) || ( Root() == nullptr ) )
			{
				return false;
			}
			pWindow = NWx::Child<CSceneWxWindow>( Root(), pCore );
			wxBoxSizer *const pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( pWindow, wxSizerFlags( 1 ).Expand() );
			Root()->SetSizer( pSizer );
			if ( pCore->IsScrollEnabled() )
			{
				::ShowScrollBar( GetHandle(), SB_BOTH, TRUE );
			}
			bCoreHasWindow = true;
			if ( !pCore->OnCreate( this ) )
			{
				// As a window whose WM_CREATE refused: destroyed, with the core
				// told as its WM_DESTROY told it.
				DestroyHost();
				pWindow = nullptr;
				return false;
			}
			ShowHost( true );
			return true;
		}

		virtual IWidget* GetWidget()
		{
			return this;
		}

		virtual HWND GetHandle() const
		{
			return ( pWindow != nullptr ) ? static_cast<HWND>( pWindow->GetHWND() ) : 0;
		}

		virtual void Redraw()
		{
			if ( pWindow != nullptr )
			{
				pWindow->Refresh( true );
				pWindow->Update();
			}
		}

		virtual void RedrawWithoutErase()
		{
			if ( pWindow != nullptr )
			{
				pWindow->Refresh( false );
				pWindow->Update();
			}
		}

		virtual bool HasFocus() const
		{
			return ( pWindow != nullptr ) && pWindow->HasFocus();
		}

		virtual void Focus()
		{
			if ( pWindow != nullptr )
			{
				pWindow->SetFocus();
			}
		}

		virtual bool IsShown() const
		{
			// Visible on screen, every parent shown -- IsWindowVisible, which the
			// renderer asks the same window before resizing its back buffer.
			return ( pWindow != nullptr ) && ( ::IsWindowVisible( GetHandle() ) != FALSE );
		}

		virtual CTPoint<int> GetClientSize() const
		{
			if ( pWindow == nullptr )
			{
				return CTPoint<int>( 0, 0 );
			}
			const wxSize size = pWindow->GetClientSize();
			return CTPoint<int>( size.x, size.y );
		}

		virtual void Place( const CTRect<int> &rRect )
		{
			if ( pWindow != nullptr )
			{
				pWindow->SetSize( rRect.left, rRect.top, rRect.Width(), rRect.Height() );
			}
		}

		virtual void Fill( const CTRect<int> &rRect, TWidgetColor color )
		{
			if ( pWindow != nullptr )
			{
				wxClientDC dc( pWindow );
				dc.SetPen( *wxTRANSPARENT_PEN );
				dc.SetBrush( wxBrush( CWxPaintContext::ToColour( color ) ) );
				dc.DrawRectangle( rRect.left, rRect.top, rRect.Width(), rRect.Height() );
			}
		}

		virtual void SetMouseCapture( bool bCapture )
		{
			if ( pWindow == nullptr )
			{
				return;
			}
			// wx keeps its own count of captures and asserts on an unbalanced one;
			// the states release a capture they may never have taken.
			if ( bCapture )
			{
				if ( !pWindow->HasCapture() )
				{
					pWindow->CaptureMouse();
				}
			}
			else if ( pWindow->HasCapture() )
			{
				pWindow->ReleaseMouse();
			}
		}

		virtual bool StartUpdateTimer( int nInterval )
		{
			return updateTimer.Start( nInterval );
		}

		virtual void StopUpdateTimer()
		{
			updateTimer.Stop();
		}

		// The scroll bars are the window's own, WS_HSCROLL and WS_VSCROLL, driven
		// as the MFC view drove them; wx manages none of them.
		virtual void ShowScrollBars( bool bShow )
		{
			const HWND hWindow = GetHandle();
			if ( hWindow == 0 )
			{
				return;
			}
			const LONG nStyle = ::GetWindowLong( hWindow, GWL_STYLE );
			::SetWindowLong( hWindow, GWL_STYLE, bShow ? ( nStyle | WS_VSCROLL | WS_HSCROLL ) : ( nStyle & ~( WS_VSCROLL | WS_HSCROLL ) ) );
			// CWnd::ModifyStyle( ..., 1 ): SWP_NOSIZE and the rest.
			::SetWindowPos( hWindow, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
			::ShowScrollBar( hWindow, SB_BOTH, bShow ? TRUE : FALSE );
			// необходимо для того чобы показать изменения сразу
			WINDOWPLACEMENT windowPlacement;
			windowPlacement.length = sizeof( WINDOWPLACEMENT );
			//
			::GetWindowPlacement( hWindow, &windowPlacement );
			windowPlacement.showCmd = SW_SHOWMINIMIZED;
			::SetWindowPlacement( hWindow, &windowPlacement );
			Redraw();
			//
			windowPlacement.showCmd = SW_SHOWMAXIMIZED;
			::SetWindowPlacement( hWindow, &windowPlacement );
			Redraw();
		}

		virtual void SetScrollBarRange( bool bVertical, int nMin, int nMax )
		{
			::SetScrollRange( GetHandle(), bVertical ? SB_VERT : SB_HORZ, nMin, nMax, TRUE );
		}

		virtual void GetScrollBarState( bool bVertical, int *pnMin, int *pnMax, int *pnPos ) const
		{
			SCROLLINFO si;
			si.cbSize = sizeof( si );
			si.fMask = SIF_ALL;
			::GetScrollInfo( GetHandle(), bVertical ? SB_VERT : SB_HORZ, &si );
			( *pnMin ) = si.nMin;
			( *pnMax ) = si.nMax;
			( *pnPos ) = si.nPos;
		}

		virtual void SetScrollBarPos( bool bVertical, int nPos )
		{
			::SetScrollPos( GetHandle(), bVertical ? SB_VERT : SB_HORZ, nPos, TRUE );
		}
	};
}


namespace NSceneSurface
{
	ISceneSurface* Create()
	{
		return new CWxSceneSurface();
	}
}

