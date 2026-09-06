#include "stdafx.h"

#include "LogView.h"

#ifdef OBK2_WITH_WX

#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/MfcWidget.h"

#include <wx/nativewin.h>
#include <wx/textctrl.h>

// The Log Window's contents, drawn by wx, inside the MFC docking pane.
//
// This is the first piece of the editor drawn by wx rather than MFC, and it is
// deliberately a piece with no layout and almost no logic: a read-only text
// control that is appended to, cleared, copied from and selected.
//
// How it gets inside an MFC pane, and why there is an extra window in the way.
//
// wxNativeContainerWindow adopts an existing HWND and lets wx windows be created
// inside it; wxHAS_NATIVE_CONTAINER_WINDOW is defined on __WXMSW__ with HWND as
// the handle type, and its destructor deliberately does not destroy the adopted
// window. But adopting means *subclassing*: wx puts its own window procedure on
// the HWND it is given. Handing it the pane directly worked and cost the pane
// its caption bar and close box, because SECControlBar draws those itself and
// its procedure no longer ran.
//
// So a plain MFC child window is created inside the pane first, and wx adopts
// that. The pane keeps its own procedure, its gripper and its docking; wx owns
// a rectangle inside it and nothing else. That is the boundary this whole slice
// is about, and it needs to be a real window to hold.
//
// What this does NOT do, which the Scintilla one does, so that a comparison
// between them is honest rather than flattering:
//
//   * No context menu. The pane's IDM_LOG_CONTEXT_MENU is an MFC menu resource
//     tracked on the MFC frame; porting it is a separate question from whether
//     a wx control can live here at all.
//   * No per-line colour. wxTextCtrl can do it with wxTE_RICH2 and
//     SetDefaultStyle and that is the next thing to add. Until then the log type
//     is said in the text instead of shown, so nothing is silently lost.

namespace
{
	class CLogViewWx : public ILogView
	{
		// An MFC child of the pane, created here and owned here. wx adopts this
		// rather than the pane, so that the pane's own window procedure survives.
		CWnd wndHost;
		// The host, adopted. wx subclasses it; it does not destroy it.
		wxNativeContainerWindow *pContainer = nullptr;
		// Owned by pContainer, as any wx child is by its parent.
		wxTextCtrl *pText = nullptr;
		// Registered as the selection command handler when the contents take
		// focus. Borrowed: the pane outlives this view.
		ICommandHandler *pSelectionHandler = nullptr;

	public:
		virtual ~CLogViewWx()
		{
			if ( pContainer != nullptr )
			{
				// Destroys the wx children; leaves the adopted host alone, which is
				// what wxNativeContainerWindow's destructor is documented to do.
				pContainer->Destroy();
				pContainer = nullptr;
				pText = nullptr;
			}
			// And the host is ours, so it goes too. After wx, not before: wx's
			// window procedure is on it until the container is gone.
			if ( wndHost.GetSafeHwnd() != nullptr )
			{
				wndHost.DestroyWindow();
			}
		}

		virtual bool Create( IWidget *pParentPane, ICommandHandler *_pSelectionHandler )
		{
			CWnd *const pwndPane = ToCWnd( pParentPane );
			if ( pwndPane == nullptr || pwndPane->GetSafeHwnd() == nullptr )
			{
				return false;
			}
			pSelectionHandler = _pSelectionHandler;

			// A bare child window with no class behaviour of its own; everything
			// visible inside it will be wx's.
			if ( !wndHost.CreateEx( 0, AfxRegisterWndClass( 0 ), 0,
															WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
															CRect( 0, 0, 0, 0 ), pwndPane, 0 ) )
			{
				return false;
			}

			pContainer = new wxNativeContainerWindow( wndHost.GetSafeHwnd() );
			if ( pContainer->GetHandle() == nullptr )
			{
				// The documented failure report: GetHandle() answers null when the
				// handle could not be used. Nothing else says so.
				delete pContainer;
				pContainer = nullptr;
				return false;
			}

			pText = NWx::Child<wxTextCtrl>( pContainer, wxID_ANY, wxString(),
																			wxDefaultPosition, wxDefaultSize,
																			wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP );

			// The same registration CLogWindow::OnSetFocus does on the MFC side:
			// focus here means selection commands belong to this pane.
			pText->Bind( wxEVT_SET_FOCUS, &CLogViewWx::OnSetFocus, this );
			return true;
		}

		virtual bool IsCreated() const
		{
			return pText != nullptr;
		}

		virtual void SetBounds( const CTRect<int> &rBounds )
		{
			if ( pText == nullptr )
			{
				return;
			}
			// The MFC host takes the position inside the pane; the wx control
			// fills the host. Two steps because the boundary is between them.
			wndHost.SetWindowPos( 0, rBounds.left, rBounds.top,
														rBounds.Width(), rBounds.Height(),
														SWP_NOZORDER | SWP_NOACTIVATE );
			pText->SetSize( 0, 0, rBounds.Width(), rBounds.Height() );
		}

		virtual void Show( bool bShow )
		{
			if ( pText != nullptr )
			{
				pText->Show( bShow );
			}
		}

		virtual void Append( ELogOutputType eLogOutputType, const std::string &rszText )
		{
			if ( pText == nullptr )
			{
				return;
			}
			const char *pszPrefix = "";
			switch ( eLogOutputType )
			{
				case LT_IMPORTANT:
					pszPrefix = "[!] ";
					break;
				case LT_ERROR:
					pszPrefix = "[E] ";
					break;
				default:
					break;
			}
			// FromUTF8 both halves: every narrow string in this tree is UTF-8 and
			// wxString is wide. This is the boundary, and it is one line.
			pText->AppendText( wxString::FromUTF8( pszPrefix ) + wxString::FromUTF8( rszText.c_str() ) );
		}

		virtual void Clear()
		{
			if ( pText != nullptr )
			{
				pText->Clear();
			}
		}

		virtual void Redraw()
		{
			if ( pText != nullptr )
			{
				pText->Update();
			}
		}

		virtual void Copy()
		{
			if ( pText != nullptr )
			{
				pText->Copy();
			}
		}

		virtual void SelectAll()
		{
			if ( pText != nullptr )
			{
				pText->SelectAll();
			}
		}

		virtual bool HasSelection() const
		{
			if ( pText == nullptr )
			{
				return false;
			}
			long nFrom = 0;
			long nTo = 0;
			pText->GetSelection( &nFrom, &nTo );
			return nFrom != nTo;
		}

		virtual bool IsEmpty() const
		{
			return pText == nullptr || pText->IsEmpty();
		}

	private:
		void OnSetFocus( wxFocusEvent &rEvent )
		{
			rEvent.Skip();
			if ( pSelectionHandler != nullptr )
			{
				Singleton<ICommandHandlerContainer>()->Set( CHID_SELECTION, pSelectionHandler );
			}
		}
	};
}


namespace NLogView
{
	ILogView* CreateWxLogView()
	{
		return new CLogViewWx();
	}
}

#endif // OBK2_WITH_WX
