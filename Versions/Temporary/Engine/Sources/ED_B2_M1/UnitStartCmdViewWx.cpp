#include "stdafx.h"

#include "UnitStartCmdView.h"

#ifdef OBK2_WITH_WX

#include "CommandHandlerDefines.h"
#include "ResourceDefines.h"
#include "StringResources.h"
#include "UnitStartCmdData.h"

#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"

#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>

// The unit start commands palette, in wx. The third palette, and the first with
// more than one column and with buttons that mean something -- Add, Delete, up
// and down all report the same UI event and let the state behind decide what to
// do, which is why they need no logic here beyond saying which one was pressed.

namespace
{
	class CUnitStartCmdWxWindow : public CWxHostWindow, public CUnitStartCmdCommands
	{
		wxListCtrl *pCommands = nullptr;

		// Filling the list raises the same selection events the user does.
		bool bIsDataBeginSet = false;
		// Which button was last pressed, read back by GetDialogData. Set for the
		// duration of one notification and cleared again, as the MFC palette does.
		SUnitStartCmdWindowData::EAction eLastAction = SUnitStartCmdWindowData::NO_CMD;

	public:
		CUnitStartCmdWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->Set( CHID_UNIT_START_CMD_WINDOW, this );
		}

		virtual ~CUnitStartCmdWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->Remove( CHID_UNIT_START_CMD_WINDOW );
		}

		bool Build( CWnd *pParent )
		{
			if ( !CreateHost( pParent ) )
			{
				return false;
			}
			wxWindow *const pRoot = Root();

			// The anchors from CUnitStartCmdWindow's constructor:
			//   IDC_BUTTON_UP, IDC_BUTTON_DOWN  ANCHORE_RIGHT     -> pushed right
			//   IDC_LIST_UNIT_CMD  ANCHORE_LEFT|RIGHT|BOTTOM|RESIZE_HOR_VER
			//                                                     -> proportion 1
			//   Add and Delete carry none, so they stay at the left.
			//
			// The template says the same thing with numbers: Add at x=0 and
			// Delete at x=30 in a 145-wide dialog, up at 94 and down at 120. A
			// stretch spacer between the pairs is what that gap is.
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			wxBoxSizer *pButtons = new wxBoxSizer( wxHORIZONTAL );
			pButtons->Add( MakeButton( pRoot, "Add", SUnitStartCmdWindowData::ADD_CMD ) );
			pButtons->Add( MakeButton( pRoot, "Delete", SUnitStartCmdWindowData::DEL_CMD ),
										 wxSizerFlags().Border( wxLEFT, 2 ) );
			pButtons->AddStretchSpacer( 1 );
			pButtons->Add( MakeButton( pRoot, "up", SUnitStartCmdWindowData::ORDER_UP_CMD ) );
			pButtons->Add( MakeButton( pRoot, "down", SUnitStartCmdWindowData::ORDER_DOWN_CMD ),
										 wxSizerFlags().Border( wxLEFT, 2 ) );
			pSizer->Add( pButtons, wxSizerFlags().Expand().Border( wxBOTTOM, 3 ) );

			// Gridlines are the only one of the three extended styles that has to
			// be asked for; see FormationViewWx.cpp.
			pCommands = NWx::Child<wxListCtrl>( pRoot, wxID_ANY,
																					wxDefaultPosition, wxDefaultSize,
																					wxLC_REPORT | wxLC_HRULES | wxLC_VRULES |
																					wxBORDER_SUNKEN );
			pCommands->InsertColumn( 0, wxString::FromUTF8( RCSTR( "Type" ) ), wxLIST_FORMAT_LEFT );
			pCommands->InsertColumn( 1, wxString::FromUTF8( RCSTR( "Target" ) ), wxLIST_FORMAT_LEFT );
			pCommands->Bind( wxEVT_SIZE, &CUnitStartCmdWxWindow::OnListResized, this );
			pCommands->Bind( wxEVT_LIST_ITEM_SELECTED,
											 &CUnitStartCmdWxWindow::OnSelectionChanged, this );
			pCommands->Bind( wxEVT_LIST_ITEM_ACTIVATED,
											 &CUnitStartCmdWxWindow::OnItemActivated, this );
			pSizer->Add( pCommands, wxSizerFlags( 1 ).Expand() );

			pRoot->SetSizer( pSizer );
			return true;
		}

		//	CPaletteCommands
		virtual void GetDialogData( SUnitStartCmdWindowData *pData )
		{
			pData->Clear();
			// commands is not filled on the way out; nothing reads it there.
			pData->eLastAction = eLastAction;
			for ( long nRow = 0; nRow < pCommands->GetItemCount(); ++nRow )
			{
				if ( pCommands->GetItemState( nRow, wxLIST_STATE_SELECTED ) & wxLIST_STATE_SELECTED )
				{
					pData->selectedCommands.push_back( (int)pCommands->GetItemData( nRow ) );
				}
			}
		}

		virtual void SetDialogData( const SUnitStartCmdWindowData *pData )
		{
			bIsDataBeginSet = true;
			eLastAction = SUnitStartCmdWindowData::NO_CMD;
			//
			pCommands->DeleteAllItems();
			for ( size_t nIndex = 0; nIndex < pData->commands.size(); ++nIndex )
			{
				const SUnitStartCmdWindowData::SCmd &rCmd = pData->commands[nIndex];
				const long nRow = pCommands->InsertItem( (long)nIndex, wxString() );
				pCommands->SetItem( nRow, 0, wxString::FromUTF8( rCmd.szType.c_str() ) );
				pCommands->SetItem( nRow, 1, wxString::FromUTF8( rCmd.szTarget.c_str() ) );
				pCommands->SetItemData( nRow, rCmd.nIndex );
			}
			//
			// selectedCommands holds what GetItemData returned -- the command's
			// index in MapInfo.startCommandList -- and the MFC palette then uses
			// it as a *row* number here. The two agree only while the list is in
			// command order, which it is. Reproduced rather than corrected: this
			// is the behaviour the editor has, and changing it is a separate
			// question from which toolkit draws the list.
			for ( size_t nIndex = 0; nIndex < pData->selectedCommands.size(); ++nIndex )
			{
				const int nRow = pData->selectedCommands[nIndex];
				if ( nRow >= 0 && nRow < pCommands->GetItemCount() )
				{
					pCommands->SetItemState( nRow, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
				}
			}
			bIsDataBeginSet = false;
		}

	private:
		wxButton* MakeButton( wxWindow *pRoot, const char *pszLabel,
													SUnitStartCmdWindowData::EAction eAction )
		{
			wxButton *pButton = NWx::Child<wxButton>( pRoot, wxID_ANY,
																								wxString::FromUTF8( pszLabel ),
																								wxDefaultPosition, wxDefaultSize,
																								wxBU_EXACTFIT );
			pButton->Bind( wxEVT_BUTTON,
										 [this, eAction] ( wxCommandEvent& ) { NotifyHandler( eAction ); } );
			return pButton;
		}

		void NotifyHandler( SUnitStartCmdWindowData::EAction eAction )
		{
			// The action is readable only for the duration of the notification,
			// which is what the MFC palette's set-notify-clear does.
			eLastAction = eAction;
			if ( !bIsDataBeginSet )
			{
				Singleton<ICommandHandlerContainer>()->HandleCommand(
						CHID_UNIT_START_CMD_STATE, ID_UNIT_START_CMD_WINDOW_UI_EVENT, 0 );
			}
			eLastAction = SUnitStartCmdWindowData::NO_CMD;
		}

		void OnSelectionChanged( wxListEvent& )
		{
			CWaitCursor wcur;
			NotifyHandler( SUnitStartCmdWindowData::SEL_CHANGE );
		}

		void OnItemActivated( wxListEvent& )
		{
			// wxEVT_LIST_ITEM_ACTIVATED is the double click, which is what
			// NM_DBLCLK was bound to.
			NotifyHandler( SUnitStartCmdWindowData::EDIT_CMD );
		}

		void OnListResized( wxSizeEvent &rEvent )
		{
			rEvent.Skip();
			// 60/40, as OnInitDialog splits the client width.
			const int nWidth = pCommands->GetClientSize().x;
			if ( pCommands->GetColumnCount() == 2 && nWidth > 0 )
			{
				pCommands->SetColumnWidth( 0, ( nWidth * 6 ) / 10 );
				pCommands->SetColumnWidth( 1, nWidth - ( nWidth * 6 ) / 10 );
			}
		}
	};
}


namespace NUnitStartCmdView
{
	CWnd* CreateWx( CDefault3DTabWindow *pTabWindow )
	{
		CUnitStartCmdWxWindow *pWindow = pTabWindow->AddNewTab( new CUnitStartCmdWxWindow() );
		if ( pWindow == 0 )
		{
			return 0;
		}
		if ( !pWindow->Build( pTabWindow ) )
		{
			// Left in the tab list deliberately: it is the list's to delete.
			return 0;
		}
		return pWindow;
	}
}

#endif // OBK2_WITH_WX
