#include "stdafx.h"

#include "LogView.h"


#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/CommandHandlerDefines.h"

#include <wx/textctrl.h>
#include <wx/settings.h>

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
		struct SStyledRange
		{
			long from, to;
			ELogOutputType type;
		};
		std::vector<SStyledRange> styledRanges;

		wxTextAttr Style( ELogOutputType type ) const
		{
			const auto colour = NLogView::GetColour( type );
			return wxTextAttr( wxColour( colour.nRed, colour.nGreen, colour.nBlue ) );
		}

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
			pText->Bind( wxEVT_SYS_COLOUR_CHANGED, &CLogViewWx::OnThemeChanged, this );
		}

		virtual void Append( ELogOutputType eLogOutputType, const std::string &rszText )
		{
			if ( pText == nullptr )
			{
				return;
			}
			// Keep severity ranges so a theme change also recolours existing text.
			const long from = pText->GetLastPosition();
			pText->SetDefaultStyle( Style( eLogOutputType ) );
			// FromUTF8: every narrow string in this tree is UTF-8 and wxString is
			// wide. This is the boundary, and it is one line.
			pText->AppendText( wxString::FromUTF8( rszText.c_str() ) );
			const long to = pText->GetLastPosition();
			if ( !styledRanges.empty() && styledRanges.back().type == eLogOutputType )
				styledRanges.back().to = to;
			else if ( to > from )
				styledRanges.push_back( { from, to, eLogOutputType } );
		}

		virtual void Clear()
		{
			styledRanges.clear();
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
		void OnThemeChanged( wxSysColourChangedEvent &event )
		{
			event.Skip();
			if ( pText )
			{
				// Style in place: Freeze/Thaw detaches GTK's text buffer and can
				// reset the selection and scroll position during a theme change.
				for ( const auto &range : styledRanges )
					pText->SetStyle( range.from, range.to, Style( range.type ) );
			}
		}

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
	// Normal output follows the native text colour. Severity colours need a
	// lighter palette on dark backgrounds to remain as readable as normal text.
	SLogColour GetColour( ELogOutputType eLogOutputType )
	{
		const bool dark = wxSystemSettings::GetAppearance().IsDark();
		switch ( eLogOutputType )
		{
			case LT_IMPORTANT:
				return dark ? SLogColour{ 0x86, 0xd9, 0x93 } : SLogColour{ 0x22, 0x77, 0x22 };
			case LT_ERROR:
				return dark ? SLogColour{ 0xff, 0x8a, 0x80 } : SLogColour{ 0xb7, 0x1c, 0x1c };
			case LT_NORMAL:
			default:
			{
				const wxColour colour = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );
				return SLogColour{ colour.Red(), colour.Green(), colour.Blue() };
			}
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

