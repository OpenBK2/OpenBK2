#pragma once

// An MFC window whose inside is drawn by wx.
//
// This is the shape every piece of the editor takes on its way across: the
// thing around it -- a docking pane, a tab control, a splitter -- is still MFC
// and still expects a CWnd, while what is in it is wx. The Log Window did this
// by hand first; the palettes need the same thing fifteen more times, so the
// two parts that are easy to get wrong live here once.
//
// **Why there is an extra window at all.** wxNativeContainerWindow adopts an
// existing HWND so wx windows can be created inside it, and adopting means
// *subclassing*: wx puts its own window procedure on the handle it is given.
// Handing it a Stingray pane directly worked and cost the pane its caption bar
// and close box, because SECControlBar draws those itself and its procedure no
// longer ran. So this creates a plain child window of its own for wx to adopt.
// The surrounding MFC window keeps its procedure; wx owns a rectangle inside it.
//
// **Why the teardown order is not obvious.** wx's procedure sits on this
// window's handle, so the wx side has to come down before the handle does. The
// two ways this window dies arrive differently:
//
//   * DestroyWindow(), which CDefault3DTabWindow::RemoveAllTabs calls -- WM_DESTROY
//     reaches OnDestroy while the handle is still alive, which is the moment to
//     do it;
//   * the parent being destroyed, where Windows destroys the child handles
//     first and ~CDefault3DTabWindow then deletes the C++ objects -- by which
//     point OnDestroy has already run.
//
// Both end at TearDownWx, which is idempotent, and the destructor calls it once
// more for the case where an object is deleted having never been created.

#ifdef OBK2_WITH_WX

#include "Interface_Widget.h"
#include "MfcWidget.h"

#include <wx/nativewin.h>
#include <wx/window.h>

class CWxHostWindow : public CWnd, public IWidget
{
	// The adopted handle's wx side. Owns the wx children; does not own, and
	// does not destroy, the handle it was given.
	wxNativeContainerWindow *pContainer = nullptr;

public:
	DECLARE_CWND_WIDGET();

	virtual ~CWxHostWindow()
	{
		TearDownWx();
	}

	// Creates the host as a child of pParent and hands wx the inside of it.
	// False if either half failed, with nothing left half-built.
	bool CreateHost( CWnd *pParent )
	{
		if ( pParent == 0 || pParent->GetSafeHwnd() == 0 )
		{
			return false;
		}
		// A bare child window with no class behaviour of its own; everything
		// visible inside it will be wx's. WS_CLIPCHILDREN so the host does not
		// paint over what wx puts in it.
		if ( !CreateEx( 0, AfxRegisterWndClass( 0 ), 0,
										WS_CHILD | WS_CLIPCHILDREN, CRect( 0, 0, 0, 0 ), pParent, 0 ) )
		{
			return false;
		}
		pContainer = new wxNativeContainerWindow( GetSafeHwnd() );
		if ( pContainer->GetHandle() == 0 )
		{
			// The documented failure report: GetHandle() answers null when the
			// handle could not be used. Nothing else says so.
			delete pContainer;
			pContainer = 0;
			DestroyWindow();
			return false;
		}
		return true;
	}

	// The wx window to parent content on, or null before CreateHost succeeded.
	wxWindow* Root() const
	{
		return pContainer;
	}

protected:
	// Lays the wx side over the whole of the host. Call from the owner's resize.
	void FitContainer()
	{
		if ( pContainer == 0 || GetSafeHwnd() == 0 )
		{
			return;
		}
		CRect rect;
		GetClientRect( &rect );
		pContainer->SetSize( 0, 0, rect.Width(), rect.Height() );
		pContainer->Layout();
	}

	// Safe to call more than once, and safe to call having never created.
	void TearDownWx()
	{
		if ( pContainer != 0 )
		{
			// Destroys the wx children; leaves the adopted handle alone, which is
			// what wxNativeContainerWindow's destructor is documented to do.
			pContainer->Destroy();
			pContainer = 0;
		}
	}

	// WindowProc rather than a message map, because a message map has to be
	// defined in exactly one translation unit and this header has no .cpp --
	// MapEditorLib does not link wx, only the front ends that include this do.
	// CWnd::WindowProc is virtual for exactly this sort of thing.
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam )
	{
		if ( message == WM_DESTROY )
		{
			// Before the handle goes, because wx's window procedure is on it.
			TearDownWx();
		}
		const LRESULT nResult = CWnd::WindowProc( message, wParam, lParam );
		if ( message == WM_SIZE )
		{
			FitContainer();
		}
		return nResult;
	}
};

#endif // OBK2_WITH_WX
