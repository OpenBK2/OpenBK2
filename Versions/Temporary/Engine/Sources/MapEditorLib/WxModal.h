#pragma once

// Running a wx dialog modally over an MFC owner.
//
// This is the only place in the tree that should contain raw Win32 for that
// purpose, and the reason it contains any is worth writing down, because the
// obvious reading -- "the point of wx is not to write Win32" -- is right, and
// this is the exception that proves where the boundary currently is.
//
// A wx dialog is made modal by giving it a wx parent: ShowModal then disables
// the parent, keeps itself above it, and restores activation on close. There is
// no wx window for this editor's main frame, and there will not be until the
// frame itself is wx. The tempting shortcut, wrapping the frame in a
// wxNativeContainerWindow, is the mistake the Log Window already made on a
// smaller target: adopting a window means subclassing it, and subclassing the
// main frame would break its own message handling.
//
// So the two things modality consists of are done by hand:
//
//   * GWLP_HWNDPARENT is set to the owner, which is what makes Windows keep the
//     dialog above it and hand activation back when it closes;
//   * the owner is disabled for the duration, which is what stops it taking
//     input, and is what MFC's own CDialog::DoModal does.
//
// **This file is scaffolding with a known end date.** When the main frame is a
// wxFrame, every caller passes it as a wx parent, ShowModal does all of the
// above itself, and this header is deleted. Nothing else should grow Win32 in
// the meantime: a migrated dialog is pure wx plus one call to ShowModalOver.

#ifdef OBK2_WITH_WX

#include "Interface_Widget.h"
#include "MfcWidget.h"

#include <wx/dialog.h>

namespace NWxModal
{
	namespace NDetail
	{
		// Disables a window and enables it back. RAII rather than a pair of
		// calls, because the cancel path has to undo it too.
		class CDisabledOwner
		{
			HWND hwndOwner;
			bool bWasEnabled;

		public:
			explicit CDisabledOwner( HWND _hwndOwner )
				: hwndOwner( _hwndOwner ), bWasEnabled( false )
			{
				if ( hwndOwner != 0 )
				{
					bWasEnabled = ::IsWindowEnabled( hwndOwner ) != FALSE;
					::EnableWindow( hwndOwner, FALSE );
				}
			}

			~CDisabledOwner()
			{
				if ( hwndOwner != 0 && bWasEnabled )
				{
					::EnableWindow( hwndOwner, TRUE );
				}
			}
		};
	}


	// Shows pDialog modally over pOwner and returns what ShowModal returned,
	// so callers compare against wxID_OK as they would anywhere else.
	//
	// pOwner may be any widget in the frame -- a docking pane, a control. The
	// walk to the top-level window is the point: disabling the window handed in
	// would leave the rest of the frame live, which looks modal and is not. MFC
	// does the same walk in CWnd::GetSafeOwner before DoModal disables anything,
	// so matching it is what keeps a migrated dialog behaving like the one it
	// replaced.
	//
	// The check worth running on every dialog that moves: while it is up, the
	// frame's IsWindowEnabled must be false.
	inline int ShowModalOver( wxDialog *pDialog, IWidget *pOwner )
	{
		if ( pDialog == 0 )
		{
			return wxID_CANCEL;
		}

		CWnd *const pwndOwner = ToCWnd( pOwner );
		HWND hwndOwner = pwndOwner != 0 ? pwndOwner->GetSafeHwnd() : 0;
		if ( hwndOwner != 0 )
		{
			hwndOwner = ::GetAncestor( hwndOwner, GA_ROOT );
		}

		if ( hwndOwner != 0 )
		{
			// Before the disable, so the dialog is never ownerless while visible.
			::SetWindowLongPtr( (HWND)pDialog->GetHandle(), GWLP_HWNDPARENT, (LONG_PTR)hwndOwner );
		}

		NDetail::CDisabledOwner disabledOwner( hwndOwner );
		return pDialog->ShowModal();
	}
}

#endif // OBK2_WITH_WX
