#include "stdafx.h"

#include "ReinfPointsView.h"

#ifdef OBK2_WITH_WX

#include "CommandHandlerDefines.h"
#include "ReinfPointsData.h"
#include "ResourceDefines.h"
#include "StringResources.h"

#include "MapEditorLib/WxHostWindow.h"
#include "MapEditorLib/WxOwnership.h"

#include <fmt/format.h>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>

#include <string>

// The reinforcement points palette, in wx. The fourth palette, and the biggest
// so far: three columns, a player combo, four buttons and a separator, with
// three of the buttons enabled only while a point is selected.
//
// Every action reports the same command and leaves the state behind to decide
// what it means; which action it was travels in eLastAction, set for the
// duration of the notification and cleared again. That is the MFC palette's
// set-notify-clear, and the reason it cannot simply be a member the caller
// reads whenever it likes.

namespace
{
	class CReinfPointsWxWindow : public CWxHostWindow, public CReinfPointsCommands
	{
		wxListCtrl *pPoints = nullptr;
		wxChoice *pPlayers = nullptr;
		wxButton *pDelete = nullptr;
		wxButton *pDeploy = nullptr;
		wxButton *pTyped = nullptr;

		bool bIsDataSetting = false;
		int nSelectedIndex = -1;
		SReinfPointsWindowData::EReinfWndLastAction eLastAction =
				SReinfPointsWindowData::RWA_NO_ACTIONS;

	public:
		CReinfPointsWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->Set( CHID_REINF_POINTS_WINDOW, this );
		}

		virtual ~CReinfPointsWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->Remove( CHID_REINF_POINTS_WINDOW );
		}

		bool Build( CWnd *pParent )
		{
			if ( !CreateHost( pParent ) )
			{
				return false;
			}
			wxWindow *const pRoot = Root();

			// The anchors from CReinfPointsWindow's constructor:
			//   IDC_COMBO_PLAYER      ANCHORE_LEFT_TOP | RESIZE_HOR  -> takes the row's slack
			//   IDC_RP_DELIMITER_0    ANCHORE_LEFT_TOP | RESIZE_HOR  -> full width
			//   IDC_LIST_REINF_POINTS ANCHORE_LEFT|RIGHT|BOTTOM|RESIZE_HOR_VER
			//                                                        -> proportion 1
			// The four buttons carry none and stay where the template puts them.
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			wxBoxSizer *pTopRow = new wxBoxSizer( wxHORIZONTAL );
			pTopRow->Add( MakeButton( pRoot, "Add", SReinfPointsWindowData::RWA_POINT_ADD ),
										wxSizerFlags().CentreVertical() );
			pDelete = MakeButton( pRoot, "Delete", SReinfPointsWindowData::RWA_POINT_DEL );
			pTopRow->Add( pDelete, wxSizerFlags().CentreVertical().Border( wxLEFT, 2 ) );
			pTopRow->AddStretchSpacer( 1 );
			pTopRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Player:" ),
										wxSizerFlags().CentreVertical().Border( wxRIGHT, 4 ) );
			pPlayers = NWx::Child<wxChoice>( pRoot, wxID_ANY );
			pTopRow->Add( pPlayers, wxSizerFlags().CentreVertical() );
			pSizer->Add( pTopRow, wxSizerFlags().Expand() );

			// IDC_RP_DELIMITER_0 is an SS_ETCHEDHORZ static, which is what a
			// wxStaticLine draws.
			pSizer->Add( NWx::Child<wxStaticLine>( pRoot, wxID_ANY ),
									 wxSizerFlags().Expand().Border( wxTOP | wxBOTTOM, 4 ) );

			wxBoxSizer *pTemplateRow = new wxBoxSizer( wxHORIZONTAL );
			// No proportion: the template gives these two 60 dialog units each in
			// a 148-wide palette, so they sit side by side at the left rather
			// than sharing the whole width. Stretching them was the first thing
			// tried and it does not look like the palette it replaces.
			pDeploy = MakeButton( pRoot, "Deploy Template",
														SReinfPointsWindowData::RWA_POINT_EDIT_DEPLOY );
			pTemplateRow->Add( pDeploy );
			pTyped = MakeButton( pRoot, "Typed Templates",
													 SReinfPointsWindowData::RWA_POINT_EDIT_TYPED );
			pTemplateRow->Add( pTyped, wxSizerFlags().Border( wxLEFT, 2 ) );
			pSizer->Add( pTemplateRow, wxSizerFlags().Expand().Border( wxBOTTOM, 3 ) );

			// Gridlines are the only one of the three extended styles that has to
			// be asked for; see FormationViewWx.cpp.
			pPoints = NWx::Child<wxListCtrl>( pRoot, wxID_ANY,
																				wxDefaultPosition, wxDefaultSize,
																				wxLC_REPORT | wxLC_SINGLE_SEL |
																				wxLC_HRULES | wxLC_VRULES | wxBORDER_SUNKEN );
			pPoints->InsertColumn( 0, wxString::FromUTF8( RCSTR( "ID" ) ), wxLIST_FORMAT_LEFT, 30 );
			pPoints->InsertColumn( 1, wxString::FromUTF8( RCSTR( "Deploy template" ) ),
														 wxLIST_FORMAT_LEFT );
			pPoints->InsertColumn( 2, wxString::FromUTF8( RCSTR( "Typed templates" ) ),
														 wxLIST_FORMAT_LEFT );
			pPoints->Bind( wxEVT_SIZE, &CReinfPointsWxWindow::OnListResized, this );
			pPoints->Bind( wxEVT_LIST_ITEM_SELECTED,
										 &CReinfPointsWxWindow::OnPointSelected, this );
			pPoints->Bind( wxEVT_LIST_ITEM_ACTIVATED,
										 &CReinfPointsWxWindow::OnPointActivated, this );
			pSizer->Add( pPoints, wxSizerFlags( 1 ).Expand() );

			pPlayers->Bind( wxEVT_CHOICE, &CReinfPointsWxWindow::OnPlayerChanged, this );
			// Insert adds a point and Delete removes one, from the list, exactly
			// as CReinfPointsWindow::OnKeyDown does.
			pPoints->Bind( wxEVT_KEY_DOWN, &CReinfPointsWxWindow::OnKeyDown, this );

			pRoot->SetSizer( pSizer );
			// Deliberately not disabled here. The MFC palette decides these three
			// only in SetDialogData, so between creation and the editor's first
			// fill they are enabled -- and Delete notifies even with nothing
			// selected. Greying them at build looked tidier and was a behaviour
			// change for no reason anyone asked for.
			return true;
		}

		//	CPaletteCommands
		virtual void GetDialogData( SReinfPointsWindowData *pData )
		{
			pData->Clear();
			pData->nPlayerCount = pPlayers->GetCount();
			const int nSelection = pPlayers->GetSelection();
			pData->nPlayerIndex = ( nSelection == wxNOT_FOUND ) ? -1 : PlayerAt( nSelection );
			//
			pData->nSelectedPoint = -1;
			for ( long nRow = 0; nRow < pPoints->GetItemCount(); ++nRow )
			{
				if ( pPoints->GetItemState( nRow, wxLIST_STATE_SELECTED ) & wxLIST_STATE_SELECTED )
				{
					pData->nSelectedPoint = (int)nRow;
				}
			}
			pData->eLastAction = eLastAction;
		}

		virtual void SetDialogData( const SReinfPointsWindowData *pData )
		{
			nSelectedIndex = pData->nSelectedPoint;
			if ( pData->bAviationPointSelected )
			{
				return;
			}
			bIsDataSetting = true;

			pPlayers->Clear();
			for ( int nPlayer = 0; nPlayer < pData->nPlayerCount; ++nPlayer )
			{
				pPlayers->Append( wxString::FromUTF8( std::to_string( nPlayer ).c_str() ),
													reinterpret_cast<void*>( static_cast<uintptr_t>( nPlayer ) ) );
			}
			for ( unsigned nPos = 0; nPos < pPlayers->GetCount(); ++nPos )
			{
				if ( PlayerAt( nPos ) == pData->nPlayerIndex )
				{
					pPlayers->SetSelection( nPos );
					break;
				}
			}
			//
			pPoints->DeleteAllItems();
			for ( size_t nIndex = 0; nIndex < pData->reinfPoints.size(); ++nIndex )
			{
				const SReinfPointsWindowData::SReinfPoint &rPoint = pData->reinfPoints[nIndex];
				const long nRow = pPoints->InsertItem( (long)nIndex, wxString() );
				pPoints->SetItem( nRow, 0, wxString::FromUTF8( fmt::format( "{}", nIndex ).c_str() ) );
				pPoints->SetItem( nRow, 1, wxString::FromUTF8( rPoint.szDeployTemplate.c_str() ) );
				pPoints->SetItem( nRow, 2, wxString::FromUTF8(
						std::to_string( rPoint.typedTemplates.size() ).c_str() ) );
				pPoints->SetItemData( nRow, (long)nIndex );
			}
			// Guarded, where the MFC call is not: CListCtrl::SetItemState takes -1
			// to mean every item, so it is harmless there on an empty list;
			// wxListCtrl would be asked about a row that does not exist.
			if ( pData->nSelectedPoint >= 0 && pData->nSelectedPoint < pPoints->GetItemCount() )
			{
				pPoints->SetItemState( pData->nSelectedPoint,
															 wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
															 wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
			}
			EnablePointButtons( pData->nSelectedPoint != -1 );
			//
			eLastAction = SReinfPointsWindowData::RWA_NO_ACTIONS;
			bIsDataSetting = false;
		}

	private:
		int PlayerAt( unsigned nPos ) const
		{
			return (int)reinterpret_cast<uintptr_t>( pPlayers->GetClientData( nPos ) );
		}

		void EnablePointButtons( bool bEnable )
		{
			pDelete->Enable( bEnable );
			pDeploy->Enable( bEnable );
			pTyped->Enable( bEnable );
		}

		wxButton* MakeButton( wxWindow *pRoot, const char *pszLabel,
													SReinfPointsWindowData::EReinfWndLastAction eAction )
		{
			wxButton *pButton = NWx::Child<wxButton>( pRoot, wxID_ANY,
																								wxString::FromUTF8( pszLabel ),
																								wxDefaultPosition, wxDefaultSize,
																								wxBU_EXACTFIT );
			pButton->Bind( wxEVT_BUTTON,
										 [this, eAction] ( wxCommandEvent& ) { Notify( eAction ); } );
			return pButton;
		}

		void Notify( SReinfPointsWindowData::EReinfWndLastAction eAction )
		{
			// Deploy and Typed do nothing without a point, which the MFC palette
			// checks as well as greying the buttons.
			if ( nSelectedIndex == -1 &&
					 ( eAction == SReinfPointsWindowData::RWA_POINT_EDIT_DEPLOY ||
						 eAction == SReinfPointsWindowData::RWA_POINT_EDIT_TYPED ) )
			{
				return;
			}
			CWaitCursor wcur;
			eLastAction = eAction;
			if ( !bIsDataSetting )
			{
				Singleton<ICommandHandlerContainer>()->HandleCommand(
						CHID_REINF_POINTS_STATE, ID_REINF_POINTS_WINDOW_CHANGE_STATE, 0 );
			}
			eLastAction = SReinfPointsWindowData::RWA_NO_ACTIONS;
		}

		void OnPlayerChanged( wxCommandEvent& )
		{
			Notify( SReinfPointsWindowData::RWA_PLAYER_CHANGE );
		}

		void OnPointSelected( wxListEvent &rEvent )
		{
			if ( bIsDataSetting )
			{
				return;
			}
			nSelectedIndex = rEvent.GetIndex();
			EnablePointButtons( nSelectedIndex != -1 );
			Notify( SReinfPointsWindowData::RWA_POINT_SEL_CHANGE );
		}

		void OnPointActivated( wxListEvent& )
		{
			Notify( SReinfPointsWindowData::RWA_POINT_JUMP );
		}

		void OnKeyDown( wxKeyEvent &rEvent )
		{
			switch ( rEvent.GetKeyCode() )
			{
			case WXK_INSERT:
				Notify( SReinfPointsWindowData::RWA_POINT_ADD );
				break;
			case WXK_DELETE:
				Notify( SReinfPointsWindowData::RWA_POINT_DEL );
				break;
			default:
				rEvent.Skip();
				break;
			}
		}

		void OnListResized( wxSizeEvent &rEvent )
		{
			rEvent.Skip();
			// The ID column is 30 wide and the other two split what is left, as
			// OnInitDialog divides the client width.
			const int nWidth = pPoints->GetClientSize().x;
			if ( pPoints->GetColumnCount() == 3 && nWidth > 30 )
			{
				pPoints->SetColumnWidth( 1, ( nWidth - 30 ) / 2 );
				pPoints->SetColumnWidth( 2, ( nWidth - 30 ) - ( nWidth - 30 ) / 2 );
			}
		}
	};
}


namespace NReinfPointsView
{
	CWnd* CreateWx( CDefault3DTabWindow *pTabWindow )
	{
		CReinfPointsWxWindow *pWindow = pTabWindow->AddNewTab( new CReinfPointsWxWindow() );
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
