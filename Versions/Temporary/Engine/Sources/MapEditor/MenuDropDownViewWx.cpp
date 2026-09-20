#include "stdafx.h"

#include "MenuDropDownView.h"


#include "MapEditorLib/WxWidget.h"
#include "MapEditorLib/WxOwnership.h"
#include "MapEditorLib/WxToolDialog.h"

#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/weakref.h>

// The undo and redo drop-down in wx: a borderless window holding one list,
// shown under the toolbar button and hidden again by a choice, by Escape or by
// the list losing the focus -- CMDDLDialog's three ways out.

namespace
{
	class CMenuDropDownWxWindow : public CWxToolDialog
	{
		wxListBox *pList = nullptr;
		unsigned nCommandID = 0;

	public:
		// IDD_MENU_DROP_DOWN_LIST is a WS_POPUP with DS_MODALFRAME and no
		// caption, which is the raised dialog frame and nothing else.
		explicit CMenuDropDownWxWindow( wxWindow *pParent )
			: CWxToolDialog( pParent, wxID_ANY, wxString(), wxDefaultPosition, wxDefaultSize, wxBORDER_RAISED )
		{
			// IDC_MDDL_OPERATIONS_LIST: single selection, client edge, 1 dlu in
			// from the frame. In pixels as MFC lays the template out, measured:
			// its MS Sans Serif 8 makes a dialog unit 2 pixels, so the list is
			// 198x164 at 2,2 in a 204x168 client -- 1 dlu in from the left, top
			// and bottom, and 2 from the right, as the template has it.
			pList = NWx::Child<wxListBox>( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
																		 0, nullptr, wxLB_SINGLE | wxBORDER_SUNKEN );
			wxBoxSizer *pRow = new wxBoxSizer( wxHORIZONTAL );
			pRow->Add( pList, wxSizerFlags( 1 ).Expand().Border( wxLEFT, FromDIP( 2 ) ) );
			pRow->AddSpacer( FromDIP( 4 ) );
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );
			pSizer->Add( pRow, wxSizerFlags( 1 ).Expand().Border( wxTOP | wxBOTTOM, FromDIP( 2 ) ) );
			SetSizer( pSizer );
			// 102x84 dlu.
			SetClientSize( FromDIP( wxSize( 204, 168 ) ) );

			pList->Bind( wxEVT_LISTBOX, &CMenuDropDownWxWindow::OnChoose, this );
			pList->Bind( wxEVT_KILL_FOCUS, &CMenuDropDownWxWindow::OnListKillFocus, this );
			// Escape on the list's own key-down. A wxEVT_CHAR_HOOK handler on this
			// window was tried first and an Escape posted to the list, the way the
			// probe tooling sends keys, never reached it while the window sat in
			// MFC's message loop.
			pList->Bind( wxEVT_KEY_DOWN, &CMenuDropDownWxWindow::OnListKeyDown, this );
			// Any close hides: the controller container shows this same window
			// again next time, and a destroyed one would leave it nothing to show.
			Bind( wxEVT_CLOSE_WINDOW, &CMenuDropDownWxWindow::OnClose, this );
		}

		// CMDDLDialog::SetParams and what CControllerContainer did after it.
		void ShowAt( int nX, int nY, unsigned _nCommandID, const std::list<std::string> &rEntries )
		{
			nCommandID = _nCommandID;
			pList->Clear();
			for ( std::list<std::string>::const_iterator itEntry = rEntries.begin(); itEntry != rEntries.end(); ++itEntry )
			{
				pList->Append( wxString::FromUTF8( itEntry->c_str() ) );
			}
			if ( pList->GetCount() > 0 )
			{
				pList->SetSelection( 0 );
			}
			Move( nX, nY );
			Show();
			Raise();
			pList->SetFocus();
		}

	private:
		// LBN_SELCHANGE: hidden first, then the choice, as CMDDLDialog does it.
		// An entry's index is its depth, which is the data the MFC list stored.
		void OnChoose( wxCommandEvent & )
		{
			const int nIndex = pList->GetSelection();
			Hide();
			NMenuDropDown::Choose( nCommandID, nIndex );
		}

		void OnListKillFocus( wxFocusEvent &rEvent )
		{
			Hide();
			rEvent.Skip();
		}

		void OnClose( wxCloseEvent & )
		{
			Hide();
		}

		// Escape is CMDDLDialog::OnCancel, which hides. Enter is its OnOK, which
		// does nothing, and nothing here claims it.
		void OnListKeyDown( wxKeyEvent &rEvent )
		{
			if ( rEvent.GetKeyCode() == WXK_ESCAPE )
			{
				Hide();
				return;
			}
			rEvent.Skip();
		}
	};


	class CWxMenuDropDown : public NMenuDropDown::IView
	{
		wxWeakRef<CMenuDropDownWxWindow> window;

	public:
		explicit CWxMenuDropDown( IWidget *pParent )
		{
			// Parented on the frame, so it stays above it; not modal.
			window = NWx::TopLevel<CMenuDropDownWxWindow>( ToWxOwnerWindow( pParent ) );
		}

		virtual ~CWxMenuDropDown()
		{
			if ( window )
			{
				window->Destroy();
			}
		}

		virtual void Show( int nX, int nY, unsigned nCommandID, const std::list<std::string> &rEntries )
		{
			if ( window )
			{
				window->ShowAt( nX, nY, nCommandID, rEntries );
			}
		}
	};
}


namespace NMenuDropDown
{
	IView* Create( IWidget *pParent )
	{
		return new CWxMenuDropDown( pParent );
	}
}

