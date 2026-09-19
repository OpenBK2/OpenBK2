#pragma once

// The wx front-end's side of the neutral handles in Interface_Widget.h, beside
// MfcWidget.h's for MFC.
//
// An IWidget is a token only the front-end that made it may look inside, and
// two front-ends make them in one session: the wx frame's panes and the wx
// views, and MFC's windows. A wx widget says so by being an IWxWidget as well,
// and ToWxWindow asks; a widget that is not one is MFC's, and ToCWnd is the
// question for that. A cast rather than a method on IWidget, so the neutral
// interface still names no toolkit.
//
// Header-only because MapEditorLib does not link wx.


#include "Interface_Widget.h"

class wxWindow;

struct IWxWidget
{
	virtual ~IWxWidget() {}
	// The wx window the widget stands for, to parent wx windows on or to lay
	// out; null while it has none.
	virtual wxWindow* GetWxWindow() = 0;
};


// The wx window behind a widget, or null for one that is not wx's, and for
// none. Null in, null out, as ToCWnd.
inline wxWindow* ToWxWindow( IWidget *pWidget )
{
	if ( IWxWidget *const pWxWidget = dynamic_cast<IWxWidget*>( pWidget ) )
	{
		return pWxWidget->GetWxWindow();
	}
	return nullptr;
}

