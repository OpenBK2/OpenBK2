#pragma once

// A wx dialog that what it opens can belong to.
//
// A button in a wx dialog's property grid, or a command in its trees, opens
// another dialog, and that has to be owned by this one, not by the frame, or
// closing it would enable the frame again under this dialog. GetOwnerWidget
// is this dialog as the IWidget such a dialog is given as its owner.
//
// This was CWxMfcOwnerDialog, which attached the wx dialog's handle to a CWnd
// so that MFC dialogs could belong to it, and answered WM_DISABLEMODAL so that
// the MFC frame's BeginModalState would not disable it. There are no MFC
// dialogs or MFC frame left, and both went.

#include "WxToolDialog.h"
#include "WxWidget.h"

class CWxOwnerDialog : public CWxToolDialog
{
	CWxWindowWidget owner { this };

public:
	CWxOwnerDialog( wxWindow *pParent, wxWindowID id, const wxString &rTitle,
									const wxPoint &rPos = wxDefaultPosition,
									const wxSize &rSize = wxDefaultSize,
									long nStyle = wxDEFAULT_DIALOG_STYLE )
		: CWxToolDialog( pParent, id, rTitle, rPos, rSize, nStyle )
	{
	}

	// What a dialog opened from this one is given as its owner.
	IWidget* GetOwnerWidget()
	{
		return &owner;
	}
};
