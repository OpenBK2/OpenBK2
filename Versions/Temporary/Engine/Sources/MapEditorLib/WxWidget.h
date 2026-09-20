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
// Header-only, like the other Wx*.h here: the front ends that include them are
// where wx belongs. MapEditorLib links wx itself now, but only for the one
// translation unit behind MessageBoxes.h, and nothing else here includes a wx
// header -- keeping it that way is what stops windows.h's A/W macros being
// undefined in a TU that then fails to link against NDb::GetObjectA.


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


// The top-level window a widget belongs to: the main frame for anything inside
// it, a dialog for a control in that dialog. Null in, null out.
//
// This is what a dialog takes as its wx parent, and the walk to the top is the
// point. wx owns a dialog to its parent's top-level window and centres it on
// its parent, so handing it the pane a command came from would centre on the
// pane. MFC made the same walk in CWnd::GetSafeOwner before DoModal, so this is
// also what keeps a migrated dialog opening where the one it replaced did.
inline wxWindow* ToWxOwnerWindow( IWidget *pOwner )
{
	wxWindow *const pWindow = ToWxWindow( pOwner );
	return ( pWindow != nullptr ) ? wxGetTopLevelParent( pWindow ) : nullptr;
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
