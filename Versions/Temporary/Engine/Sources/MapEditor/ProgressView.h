#pragma once

#include "MapEditorLib/Interface_Widget.h"

#include <string>

// The editor's progress dialog, behind a boundary that names no toolkit.
//
// The one on screen whenever the editor is busy: "Open resource ...", a label
// over a bar. CMainFrame owns it and drives it through the seven
// IMainFrame::*ProgressDialog* calls, which were already free of any toolkit;
// this is the same seven, one layer down, where the window actually is.
//
// Two things about it are worth knowing before reading either implementation.
//
// **It does not appear for half a second.** The window is created hidden and a
// 500 ms timer shows it, so an operation that finishes quickly never flashes a
// dialog at the user. Both implementations keep that, and it is why a probe
// that looks for the window immediately after CreateProgressDialog finds
// nothing.
//
// **Every setter repaints, by hand.** Whoever is driving this is in the middle
// of a long operation and is not going back to a message loop until it ends, so
// nothing would paint on its own. That is what CProgressDialog::UpdateControls
// was for -- the dialog and the frame behind it, both told to paint now -- and
// each implementation does the same after every change.
//
// There used to be a second progress display beside this one -- CProgressDlg on
// a CWinThread, reached through an IProgressHook singleton -- and it never ran:
// the hook was registered and nothing ever called its Create. It has been
// removed, so this is the only one.
namespace NProgressView
{
	class IView
	{
	public:
		virtual ~IView() {}

		// Creates the window, hidden, owned by pParent. It shows itself half a
		// second later if it is still up.
		virtual bool Create( IWidget *pParent ) = 0;
		virtual bool IsCreated() const = 0;
		// What CMainFrame::CreateProgressDialog does when the window is already
		// there: show it again and repaint.
		virtual void Show() = 0;
		virtual void Destroy() = 0;

		virtual void SetTitle( const std::string &rszTitle ) = 0;
		virtual void SetMessage( const std::string &rszMessage ) = 0;
		virtual void SetRange( int nStart, int nFinish ) = 0;
		virtual void SetPosition( int nPosition ) = 0;
		// One step on, back to the start when it runs off the end: the editor
		// uses this where it has no count to report.
		virtual void IteratePosition() = 0;
	};


	// Which implementation the frame gets: wx, unless OBK2_WX_DIALOGS=0. The caller owns
	// the result and destroys it with delete.
	IView* Create();

	// Named so the dispatcher can reach them; not for anything else to call.
	IView* CreateMfc();
#ifdef OBK2_WITH_WX
	IView* CreateWx();
#endif
}
