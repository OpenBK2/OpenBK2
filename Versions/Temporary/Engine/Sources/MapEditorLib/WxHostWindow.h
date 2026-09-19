#pragma once

// A wx view's place in what it is put in: a panel of a wx parent's -- a pane
// of the frame, the document window, or a page of the wx shortcut bar.
//
// It used to be a CWnd as well, and able to sit in an MFC parent, as a plain
// MFC child window that wx adopted. Every MFC parent is gone, and the palettes
// are handed about as IWidget* now, so that went with them.
//
// **Why the teardown order is not obvious.** The wx side has to come down, and
// BeforeTearDown run, while what it calls back into is still there. A host
// dies either by DestroyHost, or by its wx parent being destroyed, which
// destroys the panel with it and sends it wxEVT_DESTROY while its children
// still stand. Both end at BeforeTearDown once, and the destructor tears down
// once more for the case where an object is deleted having never been created.


#include "Interface_Widget.h"
#include "MfcWidget.h"
#include "WxOwnership.h"
#include "WxWidget.h"

#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/window.h>

class CWxHostWindow : public IWidget, public IWxWidget
{
	// A panel of the parent's with the wx side in it.
	wxWeakRef<wxWindow> pPanel;
	// Whether BeforeTearDown has run for what was made.
	bool bTornDown = false;

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
	// IWidget. What a view opens its dialogs over. The host has no MFC window,
	// so the main window answers for it.
	virtual void* GetNativeWidget()
	{
		return MainFrameWnd();
	}

	// IWxWidget. The panel, which is what the parent lays out.
	virtual wxWindow* GetWxWindow()
	{
		return pPanel.get();
	}

	virtual ~CWxHostWindow()
	{
		TearDownWx();
	}

	// Creates the host: a panel of pParent, which has to be wx's -- a pane, the
	// document window, or a shortcut bar page. False, with nothing made, for
	// one that is not.
	bool CreateHost( IWidget *pParent )
	{
		if ( wxWindow *const pWxParent = ToWxWindow( pParent ) )
		{
			return CreatePanel( pWxParent );
		}
		return false;
	}

	// The wx window to parent content on, or null before CreateHost succeeded.
	wxWindow* Root() const
	{
		return pPanel.get();
	}

	// Shows or hides the host.
	void ShowHost( bool bShow )
	{
		if ( pPanel )
		{
			pPanel->Show( bShow );
			// A sizer leaves a hidden window out.
			if ( wxWindow *const pParent = pPanel->GetParent() )
			{
				pParent->Layout();
			}
		}
	}

	// Takes the host down.
	void DestroyHost()
	{
		TearDownWx();
	}

	// Takes the wx side down now, while whatever it calls back into is still
	// there. The same as DestroyHost, since there is no MFC window to leave.
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
		if ( !pPanel )
		{
			return;
		}
		if ( !bTornDown )
		{
			bTornDown = true;
			BeforeTearDown();
		}
		if ( pPanel )
		{
			pPanel->Destroy();
		}
	}
};

