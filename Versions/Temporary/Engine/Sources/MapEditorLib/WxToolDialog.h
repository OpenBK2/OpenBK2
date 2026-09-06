#pragma once

// A wx dialog with the frame the editor's own dialogs have: a short caption bar
// and no taskbar button. On Win32 that is WS_EX_TOOLWINDOW, and it is not a
// detail anyone chose per dialog -- sixteen of MapEditor.rc's twenty templates
// carry EXSTYLE WS_EX_TOOLWINDOW, so it is the house style. A migrated dialog
// without it sits next to the ones not yet migrated looking visibly heavier,
// which is exactly the kind of difference that makes an A/B comparison useless.
//
// wx will not give a dialog that style through wxFRAME_TOOL_WINDOW. The flag is
// honoured in wxTopLevelWindowMSW::MSWGetStyle, but only inside
//
//     if ( !(GetExtraStyle() & wxTOPLEVEL_EX_DIALOG) )
//
// and wxDialog::Create sets wxTOPLEVEL_EX_DIALOG on its first line. Clearing it
// is not the answer either: on MSW that same flag is what decides the window is
// created as a real dialog, through CreateDialogIndirect with a template,
// rather than as a plain frame. We want the dialog behaviour and the tool
// frame, so the style is added where wx expects a derived class to add one.
//
// MSWGetStyle is the documented hook for this. From wx/msw/window.h:
//
//     // this is the function that should be overridden in the derived classes,
//     // but you will mostly use MSWGetCreateWindowFlags() below
//
// so this is an extension point, not a hole. It is the reason there is no
// ::SetWindowLong here -- the one place that legitimately needs raw Win32 is
// WxModal.h, and only until the frame is wx.

#ifdef OBK2_WITH_WX

#include <wx/dialog.h>

class CWxToolDialog : public wxDialog
{
public:
	// Two-phase construction, and it is load bearing. wxDialog's own
	// constructor calls Create, which calls MSWGetStyle -- and a virtual called
	// from a base class constructor dispatches to the base, because the vtable
	// is still wxDialog's at that point. The override below would never run.
	// Creating the window here, from a constructor body, is what makes the
	// object's dynamic type CWxToolDialog before the call happens.
	//
	// The same reasoning says a class derived from this one cannot override
	// MSWGetStyle again and be heard: by then the window already exists. No
	// dialog needs to, and one that did would create itself the same way.
	CWxToolDialog( wxWindow *pParent, wxWindowID id, const wxString &rTitle,
								 const wxPoint &rPos = wxDefaultPosition,
								 const wxSize &rSize = wxDefaultSize,
								 long nStyle = wxDEFAULT_DIALOG_STYLE )
	{
		Create( pParent, id, rTitle, rPos, rSize, nStyle );
	}

#ifdef __WXMSW__
	// Everything wx decided, plus the one extended style it will not set for a
	// dialog. Elsewhere this compiles to nothing and the dialog gets the
	// platform's ordinary frame, which is the right answer there: a small
	// caption is a Windows convention, and wx maps wxFRAME_TOOL_WINDOW to the
	// local equivalent -- _NET_WM_WINDOW_TYPE_UTILITY on GTK -- for the window
	// kinds that can carry it.
	virtual WXDWORD MSWGetStyle( long nFlags, WXDWORD *pExStyle ) const override
	{
		const WXDWORD nStyle = wxDialog::MSWGetStyle( nFlags, pExStyle );
		if ( pExStyle != nullptr )
		{
			( *pExStyle ) |= WS_EX_TOOLWINDOW;
		}
		return nStyle;
	}
#endif // __WXMSW__
};

#endif // OBK2_WITH_WX
