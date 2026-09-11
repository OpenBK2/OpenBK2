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
// the meantime: a migrated dialog is pure wx plus one call to ShowModalOver,
// and one to CentreOver for where it opens.

#ifdef OBK2_WITH_WX

#include "Interface_Widget.h"
#include "MfcWidget.h"

#include <wx/dialog.h>

#include <algorithm>

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


	// The top-level window pOwner belongs to: the frame, for any widget in it
	// -- a docking pane, a control. Null for a null widget.
	//
	// The walk to the top is the point. Disabling the window handed in would
	// leave the rest of the frame live, which looks modal and is not. MFC does
	// the same walk in CWnd::GetSafeOwner before DoModal disables anything, so
	// matching it is what keeps a migrated dialog behaving like the one it
	// replaced.
	inline HWND FindOwnerFrame( IWidget *pOwner )
	{
		CWnd *const pwndOwner = ToCWnd( pOwner );
		HWND hwndOwner = pwndOwner != 0 ? pwndOwner->GetSafeHwnd() : 0;
		if ( hwndOwner != 0 )
		{
			hwndOwner = ::GetAncestor( hwndOwner, GA_ROOT );
		}
		return hwndOwner;
	}


	// Centres pDialog over the frame pOwner belongs to, kept on that frame's
	// monitor's work area -- where an MFC dialog opens. A template with no
	// position is centred by MFC on its owner (CWnd::CenterWindow, from
	// _AfxPostInitDialog), measured on the area name dialog: its centre and
	// the frame's were the same pixel. wxWindow::Centre cannot do it here: the
	// dialog has no wx parent, so Centre centres on the screen, which is the
	// same place only while the frame is maximised. Call it after the dialog
	// has its size. With no owner, falls back to Centre.
	inline void CentreOver( wxDialog *pDialog, IWidget *pOwner )
	{
		if ( pDialog == 0 )
		{
			return;
		}
		const HWND hwndOwner = FindOwnerFrame( pOwner );
		RECT rcOwner;
		if ( hwndOwner == 0 || !::IsWindowVisible( hwndOwner ) || ::IsIconic( hwndOwner ) ||
				 !::GetWindowRect( hwndOwner, &rcOwner ) )
		{
			pDialog->Centre();
			return;
		}
		const wxSize size = pDialog->GetSize();
		int x = ( rcOwner.left + rcOwner.right - size.x ) / 2;
		int y = ( rcOwner.top + rcOwner.bottom - size.y ) / 2;
		MONITORINFO monitor = {};
		monitor.cbSize = sizeof( monitor );
		if ( ::GetMonitorInfo( ::MonitorFromWindow( hwndOwner, MONITOR_DEFAULTTONEAREST ), &monitor ) )
		{
			const RECT &rcWork = monitor.rcWork;
			x = ( std::max )( static_cast<int>( rcWork.left ), ( std::min )( x, static_cast<int>( rcWork.right ) - size.x ) );
			y = ( std::max )( static_cast<int>( rcWork.top ), ( std::min )( y, static_cast<int>( rcWork.bottom ) - size.y ) );
		}
		pDialog->Move( x, y );
	}


	// Shows pDialog modally over pOwner and returns what ShowModal returned,
	// so callers compare against wxID_OK as they would anywhere else.
	//
	// The check worth running on every dialog that moves: while it is up, the
	// frame's IsWindowEnabled must be false, and the frame must be its owner.
	inline int ShowModalOver( wxDialog *pDialog, IWidget *pOwner )
	{
		if ( pDialog == 0 )
		{
			return wxID_CANCEL;
		}

		const HWND hwndOwner = FindOwnerFrame( pOwner );

		// The common dialogs -- colour, file, font -- have no window yet: on MSW
		// they are the system's own, created inside ShowModal by ChooseColor and
		// its siblings, so there is nothing here to set an owner on. They take
		// their owner from wx's choice of parent, and wx refuses a parent that
		// is not shown on screen, in which case they get none at all -- measured
		// on the model palette's colour picker with its pane hidden. So a common
		// dialog that has to be owned by the frame sets that itself, from its
		// WM_INITDIALOG hook, before it is shown: see WxColourDialog.h. What is
		// left to do here is the disabling, which is the half no wx dialog can
		// do for itself, because the frame is not a wx window and
		// wxWindowDisabler only knows about wx ones.
		if ( hwndOwner != 0 && pDialog->GetHandle() != 0 )
		{
			// Before the disable, so the dialog is never ownerless while visible.
			::SetWindowLongPtr( (HWND)pDialog->GetHandle(), GWLP_HWNDPARENT, (LONG_PTR)hwndOwner );
		}

		NDetail::CDisabledOwner disabledOwner( hwndOwner );
		return pDialog->ShowModal();
	}
}

#endif // OBK2_WITH_WX
