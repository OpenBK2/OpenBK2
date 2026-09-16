#pragma once

// A wx view's place in whatever it is put in: a panel of a wx parent's, or,
// where the parent is still MFC, an MFC window whose inside is drawn by wx.
//
// **A wx parent.** The wx frame's panes and document window are wx panels, and
// say so (WxWidget.h). A view made in one is a wx panel of it and nothing else:
// no MFC window between the frame and the view. So is a palette made in a page
// of the wx shortcut bar, which the bar names with a CPageScope, since the
// palettes' factories are given an MFC tab window.
//
// **An MFC parent.** A Stingray pane, an MFC tab window, the MFC frame's
// document window. This is the shape every piece of the editor took on its way
// across, and the one it still takes in the MFC frame: the thing around it
// expects a CWnd, and what is in it is wx.
//
// **Why there is an extra window then.** wxNativeContainerWindow adopts an
// existing HWND so wx windows can be created inside it, and adopting means
// *subclassing*: wx puts its own window procedure on the handle it is given.
// Handing it a Stingray pane directly worked and cost the pane its caption bar
// and close box, because SECControlBar draws those itself and its procedure no
// longer ran. So this creates a plain child window of its own for wx to adopt.
// The surrounding MFC window keeps its procedure; wx owns a rectangle inside it.
//
// **Why the teardown order is not obvious.** The wx side has to come down, and
// BeforeTearDown run, while what it calls back into is still there. The ways a
// host dies arrive differently:
//
//   * DestroyHost() or DestroyWindow() -- WM_DESTROY reaches WindowProc while the
//     handle is still alive, which is the moment to do it;
//   * an MFC parent being destroyed, where Windows destroys the child handles
//     first and ~CDefault3DTabWindow then deletes the C++ objects -- by which
//     point WM_DESTROY has already come;
//   * a wx parent being destroyed, which destroys the panel with it and sends it
//     wxEVT_DESTROY while its children still stand.
//
// All end at BeforeTearDown once, and the destructor tears down once more for
// the case where an object is deleted having never been created.

#ifdef OBK2_WITH_WX

#include "Interface_Widget.h"
#include "MfcWidget.h"
#include "WxOwnership.h"
#include "WxWidget.h"

#include <wx/nativewin.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/window.h>

class CWxHostWindow : public CWnd, public IWidget, public IWxWidget
{
	// The adopted handle's wx side. Owns the wx children; does not own, and
	// does not destroy, the handle it was given.
	wxNativeContainerWindow *pContainer = nullptr;
	// Or, in a wx parent, a panel of the parent's with the wx side in it and no
	// MFC window at all.
	wxWeakRef<wxWindow> pPanel;
	// Whether BeforeTearDown has run for what was made.
	bool bTornDown = false;

	static wxWindow*& PageOverride()
	{
		static wxWindow *s_pPage = nullptr;
		return s_pPage;
	}

	bool CreatePanel( wxWindow *pParent )
	{
		pPanel = NWx::Child<wxPanel>( pParent, wxID_ANY );
		bTornDown = false;
		// The parent's destruction destroys the panel with it, and this is the
		// word of that: sent while the panel's children still stand.
		pPanel->Bind( wxEVT_DESTROY, [this]( wxWindowDestroyEvent &rEvent )
		{
			rEvent.Skip();
			if ( ( rEvent.GetEventObject() == pPanel.get() ) && !bTornDown )
			{
				bTornDown = true;
				BeforeTearDown();
			}
		} );
		return true;
	}

public:
	// While one of these is alive, CreateHost puts the wx side in a panel of
	// pPage whatever it is given as the parent. It is how the wx shortcut bar
	// (ED_B2_M1/ShortcutBarViewWx.cpp) makes the palettes, which are written to
	// be made in an MFC tab window, straight in its notebook pages, unchanged.
	// Per module, since the variable is a function-local static in a header;
	// the palettes and the bar are in the same one.
	class CPageScope
	{
		wxWindow *pPrevious;

	public:
		explicit CPageScope( wxWindow *pPage ) : pPrevious( PageOverride() ) { PageOverride() = pPage; }
		~CPageScope() { PageOverride() = pPrevious; }
	};

	// IWidget. What a view opens its dialogs over. A host made in a wx parent
	// has no MFC window, so the main window answers for it.
	virtual void* GetNativeWidget()
	{
		return ( GetSafeHwnd() != 0 ) ? static_cast<CWnd*>( this ) : MainFrameWnd();
	}

	// IWxWidget. A host made in a wx parent is its panel, which is what that
	// parent lays out; one made in an MFC parent is an MFC window, and not wx's.
	virtual wxWindow* GetWxWindow()
	{
		return pPanel.get();
	}

	virtual ~CWxHostWindow()
	{
		TearDownWx();
	}

	// Creates the host in pParent: a panel of it if it is wx's, an MFC window
	// the wx side adopts if it is MFC's. False if that failed, with nothing left
	// half-built. nControlID is the MFC window's, for an MFC parent that finds
	// its child by id -- the MDI child lays out the one that is
	// AFX_IDW_PANE_FIRST; a wx parent is told its contents instead.
	bool CreateHost( IWidget *pParent, UINT nControlID = 0 )
	{
		if ( PageOverride() == nullptr )
		{
			if ( wxWindow *const pWxParent = ToWxWindow( pParent ) )
			{
				return CreatePanel( pWxParent );
			}
		}
		return CreateHost( ToCWnd( pParent ), nControlID );
	}

	// The same for a parent that is an MFC window, as a palette's tab window is.
	bool CreateHost( CWnd *pParent, UINT nControlID = 0 )
	{
		if ( wxWindow *const pPage = PageOverride() )
		{
			return CreatePanel( pPage );
		}
		if ( pParent == 0 || pParent->GetSafeHwnd() == 0 )
		{
			return false;
		}
		// A bare child window with no class behaviour of its own; everything
		// visible inside it will be wx's. WS_CLIPCHILDREN so the host does not
		// paint over what wx puts in it.
		if ( !CreateEx( 0, AfxRegisterWndClass( 0 ), 0,
										WS_CHILD | WS_CLIPCHILDREN, CRect( 0, 0, 0, 0 ), pParent, nControlID ) )
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
		bTornDown = false;
		return true;
	}

	// The wx window to parent content on, or null before CreateHost succeeded.
	wxWindow* Root() const
	{
		return ( pContainer != nullptr ) ? pContainer : pPanel.get();
	}

	// Shows or hides the host, whichever it is.
	void ShowHost( bool bShow )
	{
		if ( GetSafeHwnd() != 0 )
		{
			ShowWindow( bShow ? SW_SHOW : SW_HIDE );
		}
		else if ( pPanel )
		{
			pPanel->Show( bShow );
			// A sizer leaves a hidden window out.
			if ( wxWindow *const pParent = pPanel->GetParent() )
			{
				pParent->Layout();
			}
		}
	}

	// Takes the host down, whichever it is: the wx side first, then the MFC
	// window if there is one.
	void DestroyHost()
	{
		TearDownWx();
		if ( GetSafeHwnd() != 0 )
		{
			DestroyWindow();
		}
	}

	// Takes the wx side down now, while whatever it calls back into is still
	// there, and leaves any MFC window standing.
	void DestroyContents()
	{
		TearDownWx();
	}

	// A surface filling the host that scrolls vertically and never
	// horizontally, for a palette to put its controls on. Null before
	// CreateHost succeeded.
	//
	// Palettes are a column of rows in a tab that is sometimes shorter than
	// the column, so they scroll: every row keeps the size it asked for, and a
	// tab too short for them all scrolls to the rest rather than squashing it.
	// See FieldViewWx.cpp for the palette that made this necessary.
	//
	// **ShowScrollbars( wxSHOW_SB_NEVER, ... ) is the half that matters, and
	// SetScrollRate( 0, 8 ) on its own does not do it.** A zero horizontal
	// rate stops horizontal scrolling, but the layout does not look at the
	// rate. wxScrollHelperBase::ScrollLayout lays the sizer out at the virtual
	// size on every axis where IsScrollbarShown() is true, and that is
	// `visibility != wxSHOW_SB_NEVER` -- true by default, whatever the rate. So
	// a palette laid itself out at the wider of the tab and its own minimum
	// width, and anything past the tab's edge was simply cut off. Nothing
	// showed it until the script camera palette: a five column report list's
	// best width is the sum of its columns, five 80 pixel defaults, and the
	// palette came out 400 pixels wide in a 218 pixel tab, its columns sized
	// to a width nobody could see.
	//
	// With the horizontal bar ruled out the width is always the tab's, and a
	// row that stretches stretches to that.
	wxScrolledWindow* CreateScrolledRoot()
	{
		wxWindow *const pHostRoot = Root();
		if ( pHostRoot == 0 )
		{
			return nullptr;
		}
		wxScrolledWindow *const pScrolled = NWx::Child<wxScrolledWindow>( pHostRoot, wxID_ANY );
		pScrolled->SetScrollRate( 0, 8 );
		pScrolled->ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_DEFAULT );
		wxBoxSizer *pHostSizer = new wxBoxSizer( wxVERTICAL );
		pHostSizer->Add( pScrolled, wxSizerFlags( 1 ).Expand() );
		pHostRoot->SetSizer( pHostSizer );
		return pScrolled;
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

	// What a derived host stops before its wx side goes -- a timer that would
	// otherwise read controls that are no longer there. Called once, while the
	// derived object still exists, except when the host is deleted without ever
	// being torn down, when there is nothing to stop.
	virtual void BeforeTearDown()
	{
	}

	// Safe to call more than once, and safe to call having never created.
	void TearDownWx()
	{
		if ( ( pContainer == 0 ) && !pPanel )
		{
			return;
		}
		if ( !bTornDown )
		{
			bTornDown = true;
			BeforeTearDown();
		}
		if ( pContainer != 0 )
		{
			// Destroys the wx children; leaves the adopted handle alone, which is
			// what wxNativeContainerWindow's destructor is documented to do.
			pContainer->Destroy();
			pContainer = 0;
		}
		if ( pPanel )
		{
			pPanel->Destroy();
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
