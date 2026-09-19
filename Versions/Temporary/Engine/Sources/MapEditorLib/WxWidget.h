#pragma once

// The wx front-end's side of the neutral handles in Interface_Widget.h.
//
// An IWidget is a token only the front-end that made it may look inside. A wx
// widget is an IWxWidget as well, and ToWxWindow asks; a cast rather than a
// method on IWidget, so the neutral interface still names no toolkit. Its
// GetNativeWidget answers with the window's native handle, an HWND, which is
// what MainWindow.h reads without including wx. (MFC's windows were the
// other kind, with ToCWnd for the question; they are gone.)
//
// Header-only because MapEditorLib does not link wx.


#include "Interface_Widget.h"

#include <wx/window.h>

struct IWxWidget
{
	virtual ~IWxWidget() {}
	// The wx window the widget stands for, to parent wx windows on or to lay
	// out; null while it has none.
	virtual wxWindow* GetWxWindow() = 0;
};


// The wx window behind a widget, or null for one that is not wx's, and for
// none. Null in, null out.
inline wxWindow* ToWxWindow( IWidget *pWidget )
{
	if ( IWxWidget *const pWxWidget = dynamic_cast<IWxWidget*>( pWidget ) )
	{
		return pWxWidget->GetWxWindow();
	}
	return nullptr;
}



// A wx window handed about as a widget: a dialog as the owner of what it opens,
// a notebook page as a palette's parent. Borrows the window; owns nothing.
class CWxWindowWidget : public IWidget, public IWxWidget
{
	wxWindow *pWindow;

public:
	explicit CWxWindowWidget( wxWindow *_pWindow ) : pWindow( _pWindow ) {}
	virtual void* GetNativeWidget() { return ( pWindow != nullptr ) ? pWindow->GetHandle() : nullptr; }
	virtual wxWindow* GetWxWindow() { return pWindow; }
};
