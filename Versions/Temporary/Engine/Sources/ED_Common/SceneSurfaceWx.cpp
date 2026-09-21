#include "stdafx.h"

#include "SceneSurface.h"
#include "MapEditorLib/Cursors.h"


#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxPaintContext.h"
#include "ChildFrameWndBase.h"
#include "NativeViewport.h"
// MK_ flags for the input states' nFlags, and VK_ codes for their nChar.
#include "port/mousekeys.h"
#include "port/vkcodes.h"

#include <wx/dcclient.h>
#include <wx/event.h>
#include <wx/sizer.h>
#include <wx/timer.h>
#include <wx/toplevel.h>
#include <wx/window.h>

#include <algorithm>
#include <functional>

// The viewport's window in wx: a wxWindow the renderer draws in, inside a
// CWxHostWindow made as the document window's AFX_IDW_PANE_FIRST, as the MFC
// view was, so either frame lays it out the same way.
//
// **The input.** The states this feeds were written against MFC's handlers and
// still take what those took: an MK_ mask, a virtual key code, a repeat count,
// and the context menu before the right button's release rather than after it.
// None of that needs a window message any more -- wxMouseEvent answers the
// mask, VirtualKeyFromWx below answers the code, wx raises one event per
// repeat, and the order of the last is simply written out in OnRightUp. The
// states could be asked to speak wx directly one day; until then this is the
// one place that translates, and port/mousekeys.h and port/vkcodes.h carry the
// constants off Windows.
//
// **The handle.** The renderer, DirectInput and DirectSound are all given
// GetHandle(), which is an HWND on Windows and an SDL_Window off it, because
// that is what DXVK Native means by HWND. CNativeViewport (NativeViewport.h)
// is the whole of that, and is the only place in the editor that names GTK,
// X11 or SDL.

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


	// The MK_ mask a Win32 mouse message carried in its wParam: which buttons
	// and which modifier keys were down at the moment of the event. wxMouseEvent
	// answers each of those questions, and answers them the same way -- the
	// button that has just gone down is in the mask, the one that has just come
	// up is not.
	inline unsigned MouseFlags( const wxMouseEvent &rEvent )
	{
		unsigned nFlags = 0;
		if ( rEvent.LeftIsDown() ) { nFlags |= MK_LBUTTON; }
		if ( rEvent.RightIsDown() ) { nFlags |= MK_RBUTTON; }
		if ( rEvent.MiddleIsDown() ) { nFlags |= MK_MBUTTON; }
		if ( rEvent.ShiftDown() ) { nFlags |= MK_SHIFT; }
		if ( rEvent.ControlDown() ) { nFlags |= MK_CONTROL; }
		return nFlags;
	}


	inline CTPoint<int> ClientPoint( const wxMouseEvent &rEvent )
	{
		const wxPoint at = rEvent.GetPosition();
		return CTPoint<int>( at.x, at.y );
	}


	// A wx key code as the virtual key code the input states compare against.
	// The opposite direction of MainFrameWx's ToWxKeyCode, and for the same
	// reason: the states were written against WM_KEYDOWN's wParam.
	//
	// Zero for a key with no virtual code, which is not passed on. Only twenty
	// virtual keys are tested anywhere in the editor and all of them are here;
	// the rest are carried because they cost nothing and a state may grow a use
	// for one.
	inline unsigned VirtualKeyFromWx( int nKeyCode )
	{
		// Letters and digits are their own code in both, and wx reports a letter
		// key as its upper case character, which is what the virtual code is.
		if ( ( ( nKeyCode >= 'A' ) && ( nKeyCode <= 'Z' ) ) || ( ( nKeyCode >= '0' ) && ( nKeyCode <= '9' ) ) )
		{
			return static_cast<unsigned>( nKeyCode );
		}
		if ( ( nKeyCode >= WXK_F1 ) && ( nKeyCode <= WXK_F12 ) )
		{
			return static_cast<unsigned>( VK_F1 + ( nKeyCode - WXK_F1 ) );
		}
		if ( ( nKeyCode >= WXK_NUMPAD0 ) && ( nKeyCode <= WXK_NUMPAD9 ) )
		{
			return static_cast<unsigned>( VK_NUMPAD0 + ( nKeyCode - WXK_NUMPAD0 ) );
		}
		switch ( nKeyCode )
		{
			case WXK_BACK:						return VK_BACK;
			case WXK_TAB:							return VK_TAB;
			case WXK_RETURN:
			case WXK_NUMPAD_ENTER:		return VK_RETURN;
			case WXK_ESCAPE:					return VK_ESCAPE;
			case WXK_SPACE:						return VK_SPACE;
			case WXK_DELETE:					return VK_DELETE;
			case WXK_INSERT:					return VK_INSERT;
			case WXK_HOME:						return VK_HOME;
			case WXK_END:							return VK_END;
			case WXK_PAGEUP:					return VK_PRIOR;
			case WXK_PAGEDOWN:				return VK_NEXT;
			case WXK_LEFT:						return VK_LEFT;
			case WXK_RIGHT:						return VK_RIGHT;
			case WXK_UP:							return VK_UP;
			case WXK_DOWN:						return VK_DOWN;
			case WXK_SHIFT:						return VK_SHIFT;
			case WXK_CONTROL:					return VK_CONTROL;
			case WXK_ALT:							return VK_MENU;
			case WXK_NUMPAD_ADD:			return VK_ADD;
			case WXK_NUMPAD_SUBTRACT:	return VK_SUBTRACT;
			case WXK_NUMPAD_MULTIPLY:	return VK_MULTIPLY;
			case WXK_NUMPAD_DIVIDE:		return VK_DIVIDE;
			case WXK_NUMPAD_DECIMAL:	return VK_DECIMAL;
			case WXK_NUMPAD_HOME:			return VK_HOME;
			case WXK_NUMPAD_END:			return VK_END;
			case WXK_NUMPAD_PAGEUP:		return VK_PRIOR;
			case WXK_NUMPAD_PAGEDOWN:	return VK_NEXT;
			case WXK_NUMPAD_LEFT:			return VK_LEFT;
			case WXK_NUMPAD_RIGHT:		return VK_RIGHT;
			case WXK_NUMPAD_UP:				return VK_UP;
			case WXK_NUMPAD_DOWN:			return VK_DOWN;
			case WXK_NUMPAD_INSERT:		return VK_INSERT;
			case WXK_NUMPAD_DELETE:		return VK_DELETE;
			default:									return 0;
		}
	}


	// The window the renderer draws in.
	class CSceneWxWindow : public wxWindow
	{
		CChildFrameWndBase *pCore;

		void OnPaint( wxPaintEvent & )
		{
			if ( pumpHook )
				pumpHook();
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
			const wxSize size = GetClientSize();
			if ( sizeHook )
			{
				sizeHook( size.x, size.y );
			}
			if ( pCore->IsSettingUp() )
			{
				return;
			}
			pCore->OnSize( size.x, size.y );
		}

		void OnSetFocus( wxFocusEvent &rEvent )
		{
			if ( focusHook )
				focusHook( true );
			rEvent.Skip();
			CWxWindowWidget oldWidget( rEvent.GetWindow() );
			pCore->OnSetFocus( &oldWidget );
		}

		void OnKillFocus( wxFocusEvent &rEvent )
		{
			if ( focusHook )
				focusHook( false );
			rEvent.Skip();
			CWxWindowWidget newWidget( rEvent.GetWindow() );
			pCore->OnKillFocus( &newWidget );
		}

		// None of the mouse and key handlers skips its event, which is what the
		// message map did by answering every one of these messages itself.

		void OnMouse( wxMouseEvent &rEvent )
		{
			if ( mouseHook )
				mouseHook( rEvent );
			const unsigned nFlags = MouseFlags( rEvent );
			const CTPoint<int> point = ClientPoint( rEvent );
			const wxEventType eType = rEvent.GetEventType();
			if ( eType == wxEVT_MOTION ) { pCore->OnMouseMove( nFlags, point ); }
			else if ( eType == wxEVT_LEFT_DOWN ) { pCore->OnLButtonDown( nFlags, point ); }
			else if ( eType == wxEVT_LEFT_UP ) { pCore->OnLButtonUp( nFlags, point ); }
			else if ( eType == wxEVT_LEFT_DCLICK ) { pCore->OnLButtonDblClk( nFlags, point ); }
			else if ( eType == wxEVT_RIGHT_DOWN ) { pCore->OnRButtonDown( nFlags, point ); }
			else if ( eType == wxEVT_RIGHT_DCLICK ) { pCore->OnRButtonDblClk( nFlags, point ); }
			else if ( eType == wxEVT_MIDDLE_DOWN ) { pCore->OnMButtonDown( nFlags, point ); }
			else if ( eType == wxEVT_MIDDLE_UP ) { pCore->OnMButtonUp( nFlags, point ); }
			else if ( eType == wxEVT_MIDDLE_DCLICK ) { pCore->OnMButtonDblClk( nFlags, point ); }
		}

		// The right button's release, and with it the context menu.
		//
		// The order is the one the message map produced and states depend on: the
		// default handling of WM_RBUTTONUP is what turned it into WM_CONTEXTMENU,
		// and it ran *before* the handler, so a state heard of the menu first and
		// of the release second. Saying both here keeps that, and keeps it the
		// same on both platforms rather than leaving it to each one's idea of
		// when a context menu happens.
		//
		// The menu key and Shift+F10 no longer raise it. They did through
		// WM_CONTEXTMENU; no input state distinguishes them from the mouse, and
		// every one of them puts the menu at the point it is given.
		void OnRightUp( wxMouseEvent &rEvent )
		{
			if ( mouseHook )
				mouseHook( rEvent );
			// Screen coordinates, as WM_CONTEXTMENU carried and the states expect.
			const wxPoint screenAt = ClientToScreen( rEvent.GetPosition() );
			pCore->OnContextMenu( CTPoint<int>( screenAt.x, screenAt.y ) );
			pCore->OnRButtonUp( MouseFlags( rEvent ), ClientPoint( rEvent ) );
		}

		void OnMouseWheel( wxMouseEvent &rEvent )
		{
			if ( mouseHook )
				mouseHook( rEvent );
			// Terrain picking expects viewport coordinates for every mouse event,
			// including the wheel; screen coordinates shift the tool by the dock's
			// offset even when the render size is already correct.
			// The last argument was DefWindowProc's answer, which for a child
			// window is whatever the parent chain made of the message. There is no
			// wx equivalent and no use for one: the event reaches this window only
			// because nothing above it took the wheel, which is what that answer
			// was standing in for. See the commit that made this change for what
			// it means if the old value was ever false.
			pCore->OnMouseWheel( MouseFlags( rEvent ), static_cast<short>( rEvent.GetWheelRotation() ),
													 ClientPoint( rEvent ), true );
		}

		// wx raises one key event per repeat where Windows counted them into a
		// single message, so the count is always one and the loop that reads it
		// runs once per event, which comes to the same thing.
		//
		// The key message's flags -- scan code, extended bit, previous state --
		// are zero. Nothing compares them: the only reader stores them in
		// CStoreInputState's record of the event and never looks again.
		void OnKeyDown( wxKeyEvent &rEvent )
		{
			if ( keyHook )
				keyHook( rEvent.GetRawKeyFlags(), true );
			const unsigned nChar = VirtualKeyFromWx( rEvent.GetKeyCode() );
			if ( nChar != 0 )
			{
				pCore->OnKeyDown( nChar, 1, 0 );
			}
		}

		void OnKeyUp( wxKeyEvent &rEvent )
		{
			if ( keyHook )
				keyHook( rEvent.GetRawKeyFlags(), false );
			const unsigned nChar = VirtualKeyFromWx( rEvent.GetKeyCode() );
			if ( nChar != 0 )
			{
				pCore->OnKeyUp( nChar, 1, 0 );
			}
		}

	public:
		// The surface is told the new size first and unconditionally: off
		// Windows the handle is an SDL window wrapping this one, and it does not
		// learn its own size from the toolkit. See NativeViewport::SetSize. The
		// core's own OnSize keeps the condition it always had.
		std::function<void( int, int )> sizeHook;
		// The native bridge feeds camera bindings as well as updating SDL's
		// window state. Tool selection still goes through pCore's input states.
		std::function<void()> pumpHook;
		std::function<void( unsigned, bool )> keyHook;
		std::function<void( bool )> focusHook;
		std::function<void( const wxMouseEvent & )> mouseHook;

		CSceneWxWindow( wxWindow *pParent, CChildFrameWndBase *_pCore )
			// wxBORDER_SUNKEN is WS_EX_CLIENTEDGE, the MFC view's edge, which
			// AlignWndAspect counts on: it sizes the window four pixels larger
			// than the client. wxWANTS_CHARS so no key is taken for navigation
			// before the viewport sees it. Made with no size, as the MFC view was,
			// so the renderer starts on the client the frame then gives it.
			: wxWindow( pParent, wxID_ANY, wxDefaultPosition, wxSize( 0, 0 ), wxBORDER_SUNKEN | wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE ),
				pCore( _pCore )
		{
			// Map-tool cursors apply only to this window. The cursor boundary
			// holds a weak wx reference, so closing the surface clears the target.
			CWxWindowWidget cursorWidget( this );
			NCursor::SetTarget( &cursorWidget );
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
			// The input, which the message map used to answer as messages.
			Bind( wxEVT_MOTION, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_ENTER_WINDOW, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_LEAVE_WINDOW, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_LEFT_DOWN, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_LEFT_UP, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_LEFT_DCLICK, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_RIGHT_DOWN, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_RIGHT_DCLICK, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_MIDDLE_DOWN, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_MIDDLE_UP, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_MIDDLE_DCLICK, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_AUX1_DOWN, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_AUX1_UP, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_AUX1_DCLICK, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_AUX2_DOWN, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_AUX2_UP, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_AUX2_DCLICK, &CSceneWxWindow::OnMouse, this );
			Bind( wxEVT_RIGHT_UP, &CSceneWxWindow::OnRightUp, this );
			Bind( wxEVT_MOUSEWHEEL, &CSceneWxWindow::OnMouseWheel, this );
			Bind( wxEVT_KEY_DOWN, &CSceneWxWindow::OnKeyDown, this );
			Bind( wxEVT_KEY_UP, &CSceneWxWindow::OnKeyUp, this );
		}
	};


	class CWxSceneSurface : public CWxHostWindow, public ISceneSurface
	{
		CChildFrameWndBase *pCore = nullptr;
		CSceneWxWindow *pWindow = nullptr;
		// The handle the renderer, DirectInput and DirectSound are given. Made
		// once the window exists and let go before it is destroyed.
		CNativeViewport nativeViewport;
		// Whether the core has the window: from its OnCreate to its OnDestroy.
		bool bCoreHasWindow = false;
		CUpdateTimer updateTimer;
		// The scroll bars, indexed horizontal then vertical. Win32 kept a minimum
		// and a maximum in the bar itself and this class read them back out of
		// it; wx counts from zero with a thumb size, so the three numbers the
		// interface deals in are kept here and wx is told what follows from them.
		bool bScrollShown = false;
		int scrollMin[2] = { 0, 0 };
		int scrollMax[2] = { 0, 0 };
		int scrollPos[2] = { 0, 0 };

		static int ScrollIndex( bool bVertical ) { return bVertical ? 1 : 0; }

		void ApplyScrollBar( bool bVertical )
		{
			if ( pWindow == nullptr )
			{
				return;
			}
			const int nIndex = ScrollIndex( bVertical );
			const int nOrientation = bVertical ? wxVERTICAL : wxHORIZONTAL;
			if ( !bScrollShown )
			{
				// No range is how wx is told there is no scroll bar, which is what
				// clearing WS_VSCROLL and WS_HSCROLL used to say.
				pWindow->SetScrollbar( nOrientation, 0, 0, 0 );
				return;
			}
			// SetScrollRange's minimum and maximum are both positions the thumb can
			// reach and the page size was never set, so the thumb is one unit and
			// the range is one more than the span.
			pWindow->SetScrollbar( nOrientation, scrollPos[nIndex] - scrollMin[nIndex], 1,
														 scrollMax[nIndex] - scrollMin[nIndex] + 1 );
		}

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
			// After the core has let go, since it is the core's renderer that was
			// given the handle, and while the window is still there.
			nativeViewport.Detach();
		}

	public:
		CWxSceneSurface()
			: updateTimer( [this]()
				{
					if ( bCoreHasWindow )
					{
						nativeViewport.PumpEvents();
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
			// Before the core's OnCreate, which is where it makes the renderer
			// and asks for the handle.
			if ( !nativeViewport.Attach( pWindow ) )
			{
				DestroyHost();
				pWindow = nullptr;
				return false;
			}
			// The window goes before this surface does, so the hook never
			// outlives it.
			pWindow->sizeHook = [this]( int nWidth, int nHeight )
			{
				nativeViewport.SetSize( nWidth, nHeight );
			};
			pWindow->pumpHook = [this]() { nativeViewport.PumpEvents(); };
			pWindow->keyHook = [this]( unsigned code, bool down ) { nativeViewport.OnKey( code, down ); };
			pWindow->focusHook = [this]( bool focused ) { nativeViewport.SetFocus( focused ); };
			pWindow->mouseHook = [this]( const wxMouseEvent &event ) { nativeViewport.OnMouse( event ); };
			pWindow->Bind( wxEVT_SCROLLWIN_TOP, &CWxSceneSurface::OnScroll, this );
			pWindow->Bind( wxEVT_SCROLLWIN_BOTTOM, &CWxSceneSurface::OnScroll, this );
			pWindow->Bind( wxEVT_SCROLLWIN_LINEUP, &CWxSceneSurface::OnScroll, this );
			pWindow->Bind( wxEVT_SCROLLWIN_LINEDOWN, &CWxSceneSurface::OnScroll, this );
			pWindow->Bind( wxEVT_SCROLLWIN_PAGEUP, &CWxSceneSurface::OnScroll, this );
			pWindow->Bind( wxEVT_SCROLLWIN_PAGEDOWN, &CWxSceneSurface::OnScroll, this );
			pWindow->Bind( wxEVT_SCROLLWIN_THUMBTRACK, &CWxSceneSurface::OnScroll, this );
			pWindow->Bind( wxEVT_SCROLLWIN_THUMBRELEASE, &CWxSceneSurface::OnScroll, this );
			if ( pCore->IsScrollEnabled() )
			{
				ShowScrollBars( true );
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
			// An HWND on Windows and an SDL_Window off it, which is what DXVK
			// means by HWND there. NativeViewport.h has the whole of it.
			return static_cast<HWND>( nativeViewport.GetHandle() );
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
			return ( pWindow != nullptr ) && pWindow->IsShownOnScreen();
		}

		virtual bool IsInActiveWindow() const
		{
			if ( pWindow == nullptr )
			{
				return false;
			}
			// GetActiveWindow against GetAncestor( GA_ROOT ): whether the window
			// the viewport sits in is the one being worked in.
			// Not const: wxTopLevelWindow::IsActive is not, since wxMSW asks the
			// system rather than answering from what it already holds.
			wxWindow *const pTopLevel = wxGetTopLevelParent( pWindow );
			return ( pTopLevel != nullptr ) && static_cast<wxTopLevelWindow*>( pTopLevel )->IsActive();
		}

		virtual void DiscardPendingInput()
		{
#if BOOST_OS_WINDOWS
			// The two PeekMessage calls this replaces, which threw away whatever
			// the user did to the viewport while a command was running.
			MSG msg;
			const HWND hWindow = GetHandle();
			::PeekMessage( &msg, hWindow, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE );
			::PeekMessage( &msg, hWindow, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE );
#endif
			// Nothing off Windows: wx has no per window queue to take events out
			// of, and GTK's is not reachable through it. The editors that ask for
			// this are guarding against a click landing after a long command, so
			// the cost of not doing it is that the click is acted on.
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

		// The scroll bars are the window's own, as WS_HSCROLL and WS_VSCROLL made
		// them, but wx puts them there now rather than the style bits.
		//
		// What has gone with them is the trick that followed: the window was
		// minimised and then maximised, twice redrawing, "необходимо для того
		// чобы показать изменения сразу" -- because changing a window's style
		// does not recompute its frame and this was how the view forced it. wx
		// adds and removes the bar itself, so there is nothing to force.
		virtual void ShowScrollBars( bool bShow )
		{
			bScrollShown = bShow;
			ApplyScrollBar( false );
			ApplyScrollBar( true );
			Redraw();
		}

		virtual void SetScrollBarRange( bool bVertical, int nMin, int nMax )
		{
			const int nIndex = ScrollIndex( bVertical );
			scrollMin[nIndex] = nMin;
			scrollMax[nIndex] = (std::max)( nMin, nMax );
			scrollPos[nIndex] = (std::min)( (std::max)( scrollPos[nIndex], nMin ), scrollMax[nIndex] );
			ApplyScrollBar( bVertical );
		}

		virtual void GetScrollBarState( bool bVertical, int *pnMin, int *pnMax, int *pnPos ) const
		{
			const int nIndex = ScrollIndex( bVertical );
			( *pnMin ) = scrollMin[nIndex];
			( *pnMax ) = scrollMax[nIndex];
			( *pnPos ) = scrollPos[nIndex];
		}

		virtual void SetScrollBarPos( bool bVertical, int nPos )
		{
			const int nIndex = ScrollIndex( bVertical );
			scrollPos[nIndex] = (std::min)( (std::max)( nPos, scrollMin[nIndex] ), scrollMax[nIndex] );
			ApplyScrollBar( bVertical );
		}

		// A scroll bar moved. wx names the same eight things WM_HSCROLL and
		// WM_VSCROLL named, and says which bar it was, so the core is told in the
		// terms it already used.
		void OnScroll( wxScrollWinEvent &rEvent )
		{
			if ( ( pCore == nullptr ) || !pCore->IsScrollEnabled() )
			{
				rEvent.Skip();
				return;
			}
			const bool bVertical = ( rEvent.GetOrientation() == wxVERTICAL );
			const wxEventType eType = rEvent.GetEventType();
			EScrollAction eAction = SCROLL_OTHER;
			if ( eType == wxEVT_SCROLLWIN_TOP ) { eAction = SCROLL_TO_START; }
			else if ( eType == wxEVT_SCROLLWIN_BOTTOM ) { eAction = SCROLL_TO_END; }
			else if ( eType == wxEVT_SCROLLWIN_LINEUP ) { eAction = SCROLL_LINE_BACK; }
			else if ( eType == wxEVT_SCROLLWIN_LINEDOWN ) { eAction = SCROLL_LINE_FORWARD; }
			else if ( eType == wxEVT_SCROLLWIN_PAGEUP ) { eAction = SCROLL_PAGE_BACK; }
			else if ( eType == wxEVT_SCROLLWIN_PAGEDOWN ) { eAction = SCROLL_PAGE_FORWARD; }
			else if ( ( eType == wxEVT_SCROLLWIN_THUMBTRACK ) || ( eType == wxEVT_SCROLLWIN_THUMBRELEASE ) )
			{
				eAction = SCROLL_THUMB;
			}
			// wx counts from zero; the core works in the range it set.
			const unsigned nPos = static_cast<unsigned>( rEvent.GetPosition() + scrollMin[ScrollIndex( bVertical )] );
			if ( bVertical )
			{
				pCore->OnVScroll( eAction, nPos );
			}
			else
			{
				pCore->OnHScroll( eAction, nPos );
			}
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

