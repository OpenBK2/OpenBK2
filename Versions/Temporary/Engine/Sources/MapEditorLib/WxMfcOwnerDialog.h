#pragma once

// A wx dialog that MFC dialogs can belong to.
//
// A button in a wx dialog's property grid, or a command in its trees, can open
// an MFC dialog, and that wants a CWnd to belong to. Given the frame instead,
// CDialog::DoModal would enable the frame again when it closed, under the wx
// dialog. So the wx dialog's own handle is attached to a CWnd: attaching only
// puts it in MFC's handle map, it does not subclass, and MFC's owner search
// stops at a window that is not a child.
//
// Being in that map has a second effect, which this class undoes. When
// NWxModal::ShowModalOver disables the frame, CFrameWnd::OnEnable calls
// BeginModalState, and that disables every enabled window the frame owns that
// MFC knows by a permanent CWnd -- which now includes the wx dialog, owned by
// the frame and just shown over it. The dialog then takes no mouse input at
// all. BeginModalState skips a window that answers WM_DISABLEMODAL with
// nonzero, which is how MFC's own modeless windows stay live, so the dialog
// answers that. Measured on the link picker: its window style read DISABLED
// until it did.

#ifdef OBK2_WITH_WX

#include "MfcWidget.h"
#include "WxToolDialog.h"

#include <afxpriv.h>

class CWxMfcOwnerDialog : public CWxToolDialog
{
	// A CWnd over a window MFC did not make, for as long as that window lives.
	class COwner : public CWnd, public IWidget
	{
	public:
		DECLARE_CWND_WIDGET();

		virtual ~COwner()
		{
			// Detached before CWnd's destructor, which would destroy the window.
			Detach();
		}
	};

	// A base member, so a derived dialog's own members -- a grid, a browser
	// that hands it to what it opens -- go before it, while the handle stands.
	COwner owner;

public:
	CWxMfcOwnerDialog( wxWindow *pParent, wxWindowID id, const wxString &rTitle,
										 const wxPoint &rPos = wxDefaultPosition,
										 const wxSize &rSize = wxDefaultSize,
										 long nStyle = wxDEFAULT_DIALOG_STYLE )
		: CWxToolDialog( pParent, id, rTitle, rPos, rSize, nStyle )
	{
		// CWxToolDialog made the window in its constructor body.
		owner.Attach( static_cast<HWND>( GetHandle() ) );
	}

	// What an MFC dialog opened from this one is given as its owner.
	IWidget* GetMfcOwner()
	{
		return &owner;
	}

	virtual WXLRESULT MSWWindowProc( WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam ) override
	{
		if ( nMsg == WM_DISABLEMODAL )
		{
			return TRUE;
		}
		return CWxToolDialog::MSWWindowProc( nMsg, wParam, lParam );
	}
};

#endif // OBK2_WITH_WX
