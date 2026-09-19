#include "stdafx.h"

#include "LogView.h"


#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/CommandHandlerDefines.h"

#include <wx/textctrl.h>

// The Log Window's contents, drawn by wx: a read-only text control that is
// appended to, cleared, copied from and selected, in the log's three colours
// (NLogView::GetColour). It was the first piece of the editor drawn by wx,
// and lived inside the MFC docking pane through an adopted child window; the
// pane is wx's now, and the control is made straight in it.
//
// No context menu: the MFC pane's IDM_LOG_CONTEXT_MENU was never carried
// across.

namespace
{
	class CLogViewWx : public ILogView
	{
		// Owned by the window it is made in, which may go first.
		wxWeakRef<wxTextCtrl> pText;
		// Registered as the selection command handler when the contents take
		// focus. Borrowed: the pane outlives this view.
		ICommandHandler *pSelectionHandler = nullptr;

	public:
		virtual ~CLogViewWx()
		{
			if ( pText )
			{
				// The window it is made in outlives this view: the text control is
				// bound to this view, so it goes with it.
				pText->Destroy();
			}
		}

		// NLogView::CreateWxLogViewIn: the text control straight in a wx window.
		bool CreateIn( wxWindow *pParent, ICommandHandler *_pSelectionHandler )
		{
			if ( pParent == nullptr )
			{
				return false;
			}
			pSelectionHandler = _pSelectionHandler;
			CreateText( pParent );
			return true;
		}

		wxWindow* GetWindow() const
		{
			return pText;
		}

		void CreateText( wxWindow *pParent )
		{
			// wxTE_RICH2 is what makes per-range colour possible at all: without
			// it a wxTextCtrl on MSW is a plain EDIT and SetDefaultStyle does
			// nothing.
			pText = NWx::Child<wxTextCtrl>( pParent, wxID_ANY, wxString(),
																			wxDefaultPosition, wxDefaultSize,
																			wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP | wxTE_RICH2 );

			// The same registration CLogWindow::OnSetFocus does on the MFC side:
			// focus here means selection commands belong to this pane.
			pText->Bind( wxEVT_SET_FOCUS, &CLogViewWx::OnSetFocus, this );
		}

		virtual void Append( ELogOutputType eLogOutputType, const std::string &rszText )
		{
			if ( pText == nullptr )
			{
				return;
			}
			// SetDefaultStyle applies to text appended after it, which is exactly
			// the shape of a log. The same three colours as the Scintilla view,
			// from the same place.
			const NLogView::SLogColour colour = NLogView::GetColour( eLogOutputType );
			pText->SetDefaultStyle( wxTextAttr( wxColour( colour.nRed, colour.nGreen, colour.nBlue ) ) );
			// FromUTF8: every narrow string in this tree is UTF-8 and wxString is
			// wide. This is the boundary, and it is one line.
			pText->AppendText( wxString::FromUTF8( rszText.c_str() ) );
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
	// The colours the editor has always used for its log, written as components.
	// Moved here from the Scintilla log view when that went with CMainFrame.
	SLogColour GetColour( ELogOutputType eLogOutputType )
	{
		switch ( eLogOutputType )
		{
			case LT_IMPORTANT:
				return SLogColour{ 0x22, 0x77, 0x22 };	// green
			case LT_ERROR:
				return SLogColour{ 0xff, 0x33, 0x33 };	// red, and it always was
			case LT_NORMAL:
			default:
				return SLogColour{ 0x00, 0x00, 0x00 };	// black
		}
	}


	ILogView* CreateWxLogViewIn( wxWindow *pParent, ICommandHandler *pSelectionHandler, wxWindow **ppWindow )
	{
		CLogViewWx *const pView = new CLogViewWx();
		if ( !pView->CreateIn( pParent, pSelectionHandler ) )
		{
			delete pView;
			return nullptr;
		}
		if ( ppWindow != nullptr )
		{
			( *ppWindow ) = pView->GetWindow();
		}
		return pView;
	}
}

