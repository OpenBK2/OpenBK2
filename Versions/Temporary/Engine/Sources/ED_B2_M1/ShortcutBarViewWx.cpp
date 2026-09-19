#include "stdafx.h"

#include "ShortcutBarView.h"


#include "MapEditorLib/DefaultShortcutBar.h"
#include "MapEditorLib/DefaultTabWindow.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"

#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/tglbtn.h>

#include <cstdint>
#include <memory>
#include <vector>

// The shortcut bar in wx: a column with a button per bar, the open bar's button
// pressed and its tabs, a notebook, filling the space under it, the buttons of
// the bars after it at the bottom -- how SECShortcutBar lays its bars out.
//
// The palettes are made by their own Create functions, unchanged: each bar
// keeps a CDefault3DTabWindow that never becomes a window as the list those
// functions register in, and so as the owner that deletes them, and a
// CWxHostWindow::CPageScope points the palette at the notebook page it goes in.
//
// What is reported is what CDefaultShortcutBar and CDefault3DTabWindow report:
// opening a bar sends MAKELONG( its shown tab, bar ) to the bar's handler, and a
// tab changed sends MAKELONG( tab, INVALID_SHORTCUT_INDEX ) to the bar's own tab
// handler, when it has one, and then MAKELONG( tab, bar ) to the bar's handler
// when the bar is the open one. Nothing is reported while a bar is being built.

namespace
{
	void Report( unsigned nCommandHandlerID, unsigned nCommandID, int nTab, int nBar )
	{
		if ( ( nCommandHandlerID == INVALID_COMMAND_HANDLER_ID ) || ( nCommandID == INVALID_COMMAND_ID ) )
		{
			return;
		}
		if ( ( nTab < 0 ) || ( nTab > INVALID_TAB_INDEX ) )
		{
			nTab = INVALID_TAB_INDEX;
		}
		if ( ( nBar < 0 ) || ( nBar > INVALID_SHORTCUT_INDEX ) )
		{
			nBar = INVALID_SHORTCUT_INDEX;
		}
		const uint32_t dwParam = MAKELONG( nTab, nBar );
		bool bEnable = false;
		bool bChecked = false;
		if ( Singleton<ICommandHandlerContainer>()->UpdateCommand( nCommandHandlerID, nCommandID, &bEnable, &bChecked ) && bEnable )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( nCommandHandlerID, nCommandID, dwParam );
		}
	}


	class CWxShortcutBarView : public CWxHostWindow, public NShortcutBar::IView
	{
		struct SBar
		{
			wxToggleButton *pButton = nullptr;
			wxNotebook *pTabs = nullptr;
			// The palettes' list and owner; never a window.
			CDefault3DTabWindow owner;
			// The same palettes, to take their wx side down before they go.
			std::vector<CWxHostWindow*> palettes;
			unsigned nTabCommandHandlerID = INVALID_COMMAND_HANDLER_ID;
			unsigned nTabCommandID = INVALID_COMMAND_ID;
			bool bBuilt = false;
		};

		std::vector<std::unique_ptr<SBar>> bars;
		wxBoxSizer *pSizer = nullptr;
		int nOpenBar = -1;
		unsigned nCommandHandlerID = INVALID_COMMAND_HANDLER_ID;
		unsigned nCommandID = INVALID_COMMAND_ID;

		SBar* Bar( int nBar ) const
		{
			return ( ( nBar >= 0 ) && ( nBar < static_cast<int>( bars.size() ) ) ) ? bars[nBar].get() : nullptr;
		}

		// Shows the open bar's tabs, presses its button, and nothing else.
		void Open( int nBar )
		{
			nOpenBar = nBar;
			for ( int nIndex = 0; nIndex < static_cast<int>( bars.size() ); ++nIndex )
			{
				SBar *const pBar = bars[nIndex].get();
				if ( !pBar->bBuilt )
				{
					continue;
				}
				pBar->pButton->SetValue( nIndex == nBar );
				pBar->pTabs->Show( nIndex == nBar );
			}
			if ( Root() != nullptr )
			{
				Root()->Layout();
			}
		}

		void OnButton( int nBar )
		{
			SBar *const pBar = Bar( nBar );
			if ( pBar == nullptr )
			{
				return;
			}
			Open( nBar );
			// CDefaultShortcutBar::OnChangeBar: the bar's shown tab goes with it.
			Report( nCommandHandlerID, nCommandID, pBar->pTabs->GetSelection(), nBar );
		}

		void OnTabChanged( int nBar, wxBookCtrlEvent &rEvent )
		{
			rEvent.Skip();
			SBar *const pBar = Bar( nBar );
			if ( ( pBar == nullptr ) || !pBar->bBuilt || ( rEvent.GetEventObject() != pBar->pTabs ) )
			{
				return;
			}
			const int nTab = rEvent.GetSelection();
			// CDefault3DTabWindow's handler first, then the bar's for the open bar,
			// in the order the MFC bar's windows get TCM_TABSEL.
			Report( pBar->nTabCommandHandlerID, pBar->nTabCommandID, nTab, INVALID_SHORTCUT_INDEX );
			if ( nBar == nOpenBar )
			{
				Report( nCommandHandlerID, nCommandID, nTab, nBar );
			}
		}

	protected:
		// The palettes' wx side goes before the bar's does: their timers still
		// run, and they are not the bar's windows' to destroy in any order.
		virtual void BeforeTearDown()
		{
			for ( std::unique_ptr<SBar> &rpBar : bars )
			{
				for ( CWxHostWindow *pPalette : rpBar->palettes )
				{
					pPalette->DestroyContents();
				}
			}
		}

	public:
		virtual ~CWxShortcutBarView()
		{
			Destroy();
		}

		// IView
		virtual bool Create( IWidget *pPane, unsigned nControlID )
		{
			if ( !CreateHost( pPane ) || ( Root() == nullptr ) )
			{
				return false;
			}
			pSizer = new wxBoxSizer( wxVERTICAL );
			Root()->SetSizer( pSizer );
			ShowHost( true );
			return true;
		}

		virtual void Destroy()
		{
			DestroyHost();
			// The owners delete the palettes, whose wx side is already gone.
			bars.clear();
			nOpenBar = -1;
		}

		virtual IWidget* GetWidget()
		{
			return this;
		}

		virtual void Show( bool bShow )
		{
			ShowHost( bShow );
		}

		virtual int BeginBar( unsigned nTabCommandHandlerID, unsigned nTabCommandID )
		{
			if ( Root() == nullptr )
			{
				return -1;
			}
			std::unique_ptr<SBar> pBar( new SBar() );
			pBar->nTabCommandHandlerID = nTabCommandHandlerID;
			pBar->nTabCommandID = nTabCommandID;
			// Made now so the palettes have pages to go in; laid out and shown in
			// EndBar, when the bar goes in the column.
			pBar->pTabs = NWx::Child<wxNotebook>( Root(), wxID_ANY );
			pBar->pTabs->Hide();
			const int nBar = static_cast<int>( bars.size() );
			pBar->pTabs->Bind( wxEVT_NOTEBOOK_PAGE_CHANGED, [this, nBar]( wxBookCtrlEvent &rEvent ) { OnTabChanged( nBar, rEvent ); } );
			bars.push_back( std::move( pBar ) );
			return nBar;
		}

		virtual bool AddTab( int nBar, const std::string &rszLabel, const NShortcutBar::TPaletteFactory &rFactory )
		{
			SBar *const pBar = Bar( nBar );
			if ( pBar == nullptr )
			{
				return false;
			}
			wxPanel *const pPage = NWx::Child<wxPanel>( pBar->pTabs, wxID_ANY );
			pPage->SetSizer( new wxBoxSizer( wxVERTICAL ) );
			CWnd *pPalette = 0;
			{
				CWxHostWindow::CPageScope pageScope( pPage );
				pPalette = rFactory( &pBar->owner );
			}
			CWxHostWindow *const pHost = dynamic_cast<CWxHostWindow*>( pPalette );
			if ( ( pHost == nullptr ) || ( pHost->Root() == nullptr ) )
			{
				// No palette, or an MFC one, which a wx page cannot hold. The
				// owner still deletes whatever was registered.
				pPage->Destroy();
				return false;
			}
			pBar->palettes.push_back( pHost );
			pPage->GetSizer()->Add( pHost->Root(), wxSizerFlags( 1 ).Expand() );
			pBar->pTabs->AddPage( pPage, wxString::FromUTF8( rszLabel.c_str() ) );
			return true;
		}

		virtual void ActivateTab( int nBar, int nTab )
		{
			SBar *const pBar = Bar( nBar );
			if ( ( pBar != nullptr ) && ( nTab >= 0 ) && ( nTab < static_cast<int>( pBar->pTabs->GetPageCount() ) ) )
			{
				pBar->pTabs->ChangeSelection( nTab );
			}
		}

		virtual void EndBar( int nBar, const std::string &rszLabel )
		{
			SBar *const pBar = Bar( nBar );
			if ( ( pBar == nullptr ) || pBar->bBuilt )
			{
				return;
			}
			pBar->pButton = NWx::Child<wxToggleButton>( Root(), wxID_ANY, wxString::FromUTF8( rszLabel.c_str() ) );
			pBar->pButton->Bind( wxEVT_TOGGLEBUTTON, [this, nBar]( wxCommandEvent & ) { OnButton( nBar ); } );
			pSizer->Add( pBar->pButton, wxSizerFlags().Expand() );
			pSizer->Add( pBar->pTabs, wxSizerFlags( 1 ).Expand() );
			pBar->bBuilt = true;
			Open( ( nOpenBar >= 0 ) ? nOpenBar : nBar );
		}

		virtual void SelectBar( int nBar )
		{
			if ( Bar( nBar ) != nullptr )
			{
				Open( nBar );
			}
		}

		virtual void SetCommandHandlerID( unsigned _nCommandHandlerID, unsigned _nCommandID )
		{
			nCommandHandlerID = _nCommandHandlerID;
			nCommandID = _nCommandID;
		}
	};
}


namespace NShortcutBar
{
	IView* Create()
	{
		return new CWxShortcutBarView();
	}
}

