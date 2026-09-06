#include "stdafx.h"

#include "FormationView.h"

#ifdef OBK2_WITH_WX

#include "CommandHandlerDefines.h"
#include "DialogData.h"
#include "FormationMnemonics.h"
#include "PaletteCommands.h"
#include "ResourceDefines.h"
#include "StringResources.h"

#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"

#include <wx/checkbox.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

// The squad formations palette, in wx. The second palette, and the first with a
// list control in it -- which is the only reason it is worth doing next, since
// a report-mode list is what most of the remaining thirteen are built around.
//
// wxListCtrl in wxLC_REPORT is the same Win32 SysListView32 underneath, so the
// extended styles the MFC one asks for are asked for the same way, and item
// data still carries the enum rather than the row number.

namespace
{
	class CFormationWxWindow : public CWxHostWindow,
														 public CPaletteCommands<SFormationWindowDialogData>
	{
		wxListCtrl *pFormations = nullptr;
		wxCheckBox *pPropMask = nullptr;

		// Remembered across SetDialogData, exactly as the MFC palette does: the
		// editor refills the list and the previous row has to come back selected.
		int nSelectedIndex = -1;
		// Filling the list raises the same selection events the user does.
		bool bIsDataSetting = false;

	public:
		CFormationWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->Set( CHID_FORMATION_LIST_DIALOG, this );
		}

		virtual ~CFormationWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->Remove( CHID_FORMATION_LIST_DIALOG );
		}

		bool Build( CWnd *pParent )
		{
			if ( !CreateHost( pParent ) )
			{
				return false;
			}
			wxWindow *const pRoot = Root();

			// Only IDC_LIST_FORMATIONS carries an anchor in the MFC constructor --
			// ANCHORE_LEFT_TOP | ANCHORE_HOR_CENTER | RESIZE_HOR_VER -- so it is
			// the one thing that grows and the other two are fixed rows above it.
			// The template's order top to bottom is checkbox, label, list.
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			pPropMask = NWx::Child<wxCheckBox>( pRoot, wxID_ANY, "Property mask" );
			pSizer->Add( pPropMask, wxSizerFlags().Border( wxBOTTOM, 4 ) );

			pSizer->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Formation" ),
									 wxSizerFlags().Border( wxBOTTOM, 2 ) );

			// The MFC palette asks for LVS_EX_GRIDLINES, LVS_EX_FULLROWSELECT and
			// LVS_EX_INFOTIP. Two of the three are wx's defaults on MSW --
			// wxListCtrl::MSWSetExListStyles always sets FULLROWSELECT, and sets
			// LABELTIP unless the control has a custom tooltip -- so only the
			// gridlines have to be asked for, as wxLC_HRULES|wxLC_VRULES.
			pFormations = NWx::Child<wxListCtrl>( pRoot, wxID_ANY,
																						wxDefaultPosition, wxDefaultSize,
																						wxLC_REPORT | wxLC_SINGLE_SEL |
																						wxLC_HRULES | wxLC_VRULES | wxBORDER_SUNKEN );
			pFormations->InsertColumn( 0, wxString::FromUTF8( RCSTR( "Formation types" ) ),
																 wxLIST_FORMAT_CENTER );
			// One column filling the control, which is what OnInitDialog means by
			// giving it the client width -- but kept true on every resize rather
			// than only at creation, when the palette has not been laid out yet
			// and the client width is still nothing.
			pFormations->Bind( wxEVT_SIZE, &CFormationWxWindow::OnListResized, this );
			pSizer->Add( pFormations, wxSizerFlags( 1 ).Expand() );

			pRoot->SetSizer( pSizer );

			pFormations->Bind( wxEVT_LIST_ITEM_SELECTED,
												 &CFormationWxWindow::OnFormationSelected, this );
			pPropMask->Bind( wxEVT_CHECKBOX, &CFormationWxWindow::OnPropMask, this );
			return true;
		}

		//	CPaletteCommands
		virtual void GetDialogData( SFormationWindowDialogData *pData )
		{
			if ( pData == 0 )
			{
				return;
			}
			// The MFC palette scans for the selected row and leaves nSelectedIndex
			// alone when there is none, rather than clearing it. Same here: the
			// remembered row is what SetDialogData restores.
			const long nSelected = pFormations->GetNextItem( -1, wxLIST_NEXT_ALL,
																											 wxLIST_STATE_SELECTED );
			if ( nSelected != -1 )
			{
				nSelectedIndex = (int)nSelected;
			}
			//
			pData->eSelectedFormation = NDb::SSquadRPGStats::SFormation::EFormationMoveType( -1 );
			if ( nSelectedIndex != -1 )
			{
				pData->eSelectedFormation =
						static_cast<NDb::SSquadRPGStats::SFormation::EFormationMoveType>(
								pFormations->GetItemData( nSelectedIndex ) );
			}
			//
			pData->bChkPropmask = pPropMask->GetValue();
		}

		virtual void SetDialogData( const SFormationWindowDialogData *pData )
		{
			if ( pData == 0 )
			{
				return;
			}
			bIsDataSetting = true;
			pFormations->DeleteAllItems();
			for ( size_t nIndex = 0; nIndex < pData->squadFormations.size(); ++nIndex )
			{
				const long nRow = pFormations->InsertItem(
						(long)nIndex,
						wxString::FromUTF8( typeFormationMnemonics.GetMnemonic(
								pData->squadFormations[nIndex] ).c_str() ) );
				// The enum, not the row: the two stop matching as soon as anything
				// filters or reorders the list.
				pFormations->SetItemData( nRow, pData->squadFormations[nIndex] );
			}
			//
			if ( pFormations->GetItemCount() > 0 && nSelectedIndex < 0 )
			{
				nSelectedIndex = 0;
			}
			// Guarded, where the MFC call is not: CListCtrl::SetItemState takes -1
			// to mean every item, so it is harmless there on an empty list;
			// wxListCtrl would be asked about a row that does not exist.
			if ( nSelectedIndex >= 0 && nSelectedIndex < pFormations->GetItemCount() )
			{
				pFormations->SetItemState( nSelectedIndex, wxLIST_STATE_SELECTED,
																	 wxLIST_STATE_SELECTED );
			}
			//
			pPropMask->SetValue( pData->bChkPropmask );
			bIsDataSetting = false;
		}

	private:
		void NotifyHandler()
		{
			if ( bIsDataSetting )
			{
				return;
			}
			CWaitCursor wcur;
			Singleton<ICommandHandlerContainer>()->HandleCommand(
					CHID_SQUAD_FORMATIONS_STATE, ID_FORMATION_WINDOW_CHANGE_STATE,
					static_cast<uint32_t>( nSelectedIndex ) );
		}

		void OnFormationSelected( wxListEvent &rEvent )
		{
			nSelectedIndex = rEvent.GetIndex();
			NotifyHandler();
		}

		void OnPropMask( wxCommandEvent& )
		{
			NotifyHandler();
		}

		void OnListResized( wxSizeEvent &rEvent )
		{
			rEvent.Skip();
			if ( pFormations->GetColumnCount() > 0 )
			{
				pFormations->SetColumnWidth( 0, pFormations->GetClientSize().x );
			}
		}
	};
}


namespace NFormationView
{
	CWnd* CreateWx( CDefault3DTabWindow *pTabWindow )
	{
		CFormationWxWindow *pWindow = pTabWindow->AddNewTab( new CFormationWxWindow() );
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
