#include "stdafx.h"

#include "AIGeneralView.h"

#ifdef OBK2_WITH_WX

#include "AIGeneralData.h"
#include "AIGeneralTypes.h"
#include "CommandHandlerDefines.h"
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

// The AI general points palette, in wx. The fifth, and the first built out of
// two identical halves: a labelled list with Add and Delete over it, twice,
// separated by an etched line. The anchors say the same thing --
//
//     SetControlStyle( IDC_AIGEN_LIST_IDS, ... RESIZE_VER, 1, 1, 1, 0.5f );
//     SetControlStyle( IDC_AIGEN_LIST_PARCELS, ... RESIZE_VER, 1, 1, 1, 0.5f );
//
// those trailing 0.5s being each list's share of the vertical slack, which is a
// sizer proportion of 1 apiece.
//
// So the halves are built by a helper rather than written twice, which is worth
// doing here and not in the MFC version: a resource template cannot say "one of
// these again" and a sizer can.

namespace
{
	class CAIGeneralWxWindow : public CWxHostWindow, public CAIGeneralPointsCommands
	{
		wxChoice *pPlayers = nullptr;
		wxListCtrl *pIDs = nullptr;
		wxListCtrl *pParcels = nullptr;
		wxButton *pIDDelete = nullptr;
		wxButton *pParcelDelete = nullptr;

		bool bIsDataSetting = false;
		SAIGeneralPointsWindowData::EAIGenPointsLastAction eLastAction =
				SAIGeneralPointsWindowData::AIGP_NO_ACTIONS;

	public:
		CAIGeneralWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->Set( CHID_AIGEN_POINTS_WINDOW, this );
		}

		virtual ~CAIGeneralWxWindow()
		{
			Singleton<ICommandHandlerContainer>()->Remove( CHID_AIGEN_POINTS_WINDOW );
		}

		bool Build( CWnd *pParent )
		{
			if ( !CreateHost( pParent ) )
			{
				return false;
			}
			wxWindow *const pRoot = Root();
			wxBoxSizer *pSizer = new wxBoxSizer( wxVERTICAL );

			wxBoxSizer *pPlayerRow = new wxBoxSizer( wxHORIZONTAL );
			pPlayerRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY, "Player:" ),
											 wxSizerFlags().CentreVertical().Border( wxRIGHT, 4 ) );
			pPlayers = NWx::Child<wxChoice>( pRoot, wxID_ANY );
			// IDC_COMBO_PLAYER is the only control on its row with RESIZE_HOR.
			pPlayerRow->Add( pPlayers, wxSizerFlags( 1 ).CentreVertical() );
			pSizer->Add( pPlayerRow, wxSizerFlags().Expand() );

			pSizer->Add( NWx::Child<wxStaticLine>( pRoot, wxID_ANY ),
									 wxSizerFlags().Expand().Border( wxTOP | wxBOTTOM, 4 ) );

			pIDs = AddSection( pRoot, pSizer, "mobileScriptIDs",
												 SAIGeneralPointsWindowData::AIGP_ID_ADD,
												 SAIGeneralPointsWindowData::AIGP_ID_DEL, &pIDDelete );
			pIDs->InsertColumn( 0, wxString::FromUTF8( RCSTR( "#" ) ), wxLIST_FORMAT_LEFT );
			pIDs->InsertColumn( 1, wxString::FromUTF8( RCSTR( "ID" ) ), wxLIST_FORMAT_LEFT );
			pIDs->Bind( wxEVT_SIZE, &CAIGeneralWxWindow::OnIDsResized, this );
			pIDs->Bind( wxEVT_LIST_ITEM_SELECTED,
									&CAIGeneralWxWindow::OnIDSelected, this );

			pSizer->Add( NWx::Child<wxStaticLine>( pRoot, wxID_ANY ),
									 wxSizerFlags().Expand().Border( wxTOP | wxBOTTOM, 4 ) );

			pParcels = AddSection( pRoot, pSizer, "Parcels",
														 SAIGeneralPointsWindowData::AIGP_PARCEL_ADD,
														 SAIGeneralPointsWindowData::AIGP_PARCEL_DEL,
														 &pParcelDelete );
			pParcels->InsertColumn( 0, wxString::FromUTF8( RCSTR( "#" ) ), wxLIST_FORMAT_LEFT );
			pParcels->InsertColumn( 1, wxString::FromUTF8( RCSTR( "Points" ) ), wxLIST_FORMAT_LEFT );
			pParcels->InsertColumn( 2, wxString::FromUTF8( RCSTR( "Position" ) ), wxLIST_FORMAT_LEFT );
			pParcels->InsertColumn( 3, wxString::FromUTF8( RCSTR( "Importance" ) ), wxLIST_FORMAT_LEFT );
			pParcels->InsertColumn( 4, wxString::FromUTF8( RCSTR( "Type" ) ), wxLIST_FORMAT_LEFT );
			pParcels->Bind( wxEVT_SIZE, &CAIGeneralWxWindow::OnParcelsResized, this );
			pParcels->Bind( wxEVT_LIST_ITEM_SELECTED,
											&CAIGeneralWxWindow::OnParcelSelected, this );
			pParcels->Bind( wxEVT_LIST_ITEM_ACTIVATED,
											&CAIGeneralWxWindow::OnParcelActivated, this );

			pPlayers->Bind( wxEVT_CHOICE, &CAIGeneralWxWindow::OnPlayerChanged, this );

			pRoot->SetSizer( pSizer );
			return true;
		}

		//	CPaletteCommands
		virtual void GetDialogData( SAIGeneralPointsWindowData *pData )
		{
			// Unusual among these palettes, and worth reading twice: this does not
			// report the whole of its state. The caller passes in the data it
			// already has and only the field the last action touched is written
			// back, because everything else is the state's to know.
			pData->eLastAction = eLastAction;
			switch ( pData->eLastAction )
			{
				case SAIGeneralPointsWindowData::AIGP_PLAYER_JUMP:
					pData->nCurrentPlayer = pPlayers->GetSelection();
					break;
				//
				case SAIGeneralPointsWindowData::AIGP_ID_JUMP:
					if ( SAIGeneralPointsWindowData::SAIPlayerInfo *pPlayer = CurrentPlayer( pData ) )
					{
						const long nRow = pIDs->GetNextItem( -1, wxLIST_NEXT_ALL,
																								 wxLIST_STATE_SELECTED );
						if ( nRow != -1 )
						{
							pPlayer->nCurrentID = (int)nRow;
						}
					}
					break;
				//
				case SAIGeneralPointsWindowData::AIGP_PARCEL_JUMP:
					if ( SAIGeneralPointsWindowData::SAIPlayerInfo *pPlayer = CurrentPlayer( pData ) )
					{
						const long nRow = pParcels->GetNextItem( -1, wxLIST_NEXT_ALL,
																										 wxLIST_STATE_SELECTED );
						if ( nRow != -1 )
						{
							pPlayer->nCurrentParcel = (int)nRow;
						}
					}
					break;
			}
		}

		virtual void SetDialogData( const SAIGeneralPointsWindowData *pData )
		{
			// No copy of the whole struct. The MFC version makes one only so it
			// can call CurrentPlayer(), CurrentID() and CurrentParcel(), which are
			// not const; the indices are read straight out of pData here instead.
			const int nPlayer = pData->nCurrentPlayer;
			if ( nPlayer < 0 || nPlayer >= (int)pData->players.size() )
			{
				return;
			}
			const SAIGeneralPointsWindowData::SAIPlayerInfo &rPlayer = pData->players[nPlayer];

			bIsDataSetting = true;

			pPlayers->Clear();
			for ( size_t nIndex = 0; nIndex < pData->players.size(); ++nIndex )
			{
				pPlayers->Append( wxString::FromUTF8( fmt::format( "{}", nIndex ).c_str() ) );
			}
			if ( nPlayer < (int)pPlayers->GetCount() )
			{
				pPlayers->SetSelection( nPlayer );
			}

			pIDs->DeleteAllItems();
			for ( size_t nIndex = 0; nIndex < rPlayer.mobileScriptIDs.size(); ++nIndex )
			{
				const long nRow = pIDs->InsertItem( (long)nIndex, wxString() );
				pIDs->SetItem( nRow, 0, wxString::FromUTF8( fmt::format( "{}", nIndex ).c_str() ) );
				pIDs->SetItem( nRow, 1, wxString::FromUTF8(
						fmt::format( "{}", rPlayer.mobileScriptIDs[nIndex] ).c_str() ) );
				pIDs->SetItemData( nRow, (long)nIndex );
			}
			Select( pIDs, rPlayer.nCurrentID );
			pIDDelete->Enable( !rPlayer.mobileScriptIDs.empty() && rPlayer.nCurrentID >= 0 );

			pParcels->DeleteAllItems();
			for ( size_t nIndex = 0; nIndex < rPlayer.parcels.size(); ++nIndex )
			{
				const SAIGeneralPointsWindowData::SAIPlayerInfo::SAIParcel &rParcel =
						rPlayer.parcels[nIndex];
				const long nRow = pParcels->InsertItem( (long)nIndex, wxString() );
				pParcels->SetItem( nRow, 0, wxString::FromUTF8( fmt::format( "{}", nIndex ).c_str() ) );
				pParcels->SetItem( nRow, 1, wxString::FromUTF8(
						fmt::format( "{}", rParcel.reinforcePoints.size() ).c_str() ) );
				pParcels->SetItem( nRow, 2, wxString::FromUTF8(
						fmt::format( "{:.0f}, {:.0f}", rParcel.vCenter.x, rParcel.vCenter.y ).c_str() ) );
				pParcels->SetItem( nRow, 3, wxString::FromUTF8(
						fmt::format( "{:.2f}", rParcel.fImportance ).c_str() ) );
				pParcels->SetItem( nRow, 4, wxString::FromUTF8(
						typeAIGeneralParcel.GetMnemonic( rParcel.eType ).c_str() ) );
				pParcels->SetItemData( nRow, (long)nIndex );
			}
			Select( pParcels, rPlayer.nCurrentParcel );
			pParcelDelete->Enable( !rPlayer.parcels.empty() && rPlayer.nCurrentParcel >= 0 );

			bIsDataSetting = false;
		}

	private:
		// One labelled list with Add and Delete over it, which this palette has
		// twice. Returns the list; hands back the Delete button through pDelete
		// because that is the one whose enabled state follows the data.
		wxListCtrl* AddSection( wxWindow *pRoot, wxBoxSizer *pSizer, const char *pszLabel,
														SAIGeneralPointsWindowData::EAIGenPointsLastAction eAdd,
														SAIGeneralPointsWindowData::EAIGenPointsLastAction eDelete,
														wxButton **ppDelete )
		{
			wxBoxSizer *pRow = new wxBoxSizer( wxHORIZONTAL );
			pRow->Add( NWx::Child<wxStaticText>( pRoot, wxID_ANY,
																					 wxString::FromUTF8( pszLabel ) ),
								 wxSizerFlags().CentreVertical() );
			pRow->AddStretchSpacer( 1 );
			pRow->Add( MakeButton( pRoot, "Add", eAdd ), wxSizerFlags().CentreVertical() );
			( *ppDelete ) = MakeButton( pRoot, "Delete", eDelete );
			pRow->Add( ( *ppDelete ), wxSizerFlags().CentreVertical().Border( wxLEFT, 2 ) );
			pSizer->Add( pRow, wxSizerFlags().Expand().Border( wxBOTTOM, 3 ) );

			// Gridlines are the only one of the three extended styles that has to
			// be asked for; see FormationViewWx.cpp.
			wxListCtrl *pList = NWx::Child<wxListCtrl>( pRoot, wxID_ANY,
																								 wxDefaultPosition, wxDefaultSize,
																								 wxLC_REPORT | wxLC_SINGLE_SEL |
																								 wxLC_HRULES | wxLC_VRULES | wxBORDER_SUNKEN );
			// Proportion 1 each, which is what the two 0.5 vertical factors on the
			// anchors add up to: the lists share the slack evenly.
			//
			// The minimum has to be lowered for that to hold. A sizer divides the
			// space *above* each item's minimum, and wxListCtrl's own best height
			// is bigger than half of what this palette has -- so the first list
			// took its minimum and the second was left with nothing at all. That
			// is what it looked like on screen: one list and an invisible one.
			pList->SetMinSize( wxSize( 20, 20 ) );
			pSizer->Add( pList, wxSizerFlags( 1 ).Expand() );
			// Insert adds and Delete removes, from either list, as the two
			// LVN_KEYDOWN handlers do.
			pList->Bind( wxEVT_KEY_DOWN,
									 [this, eAdd, eDelete] ( wxKeyEvent &rEvent )
									 {
										 switch ( rEvent.GetKeyCode() )
										 {
										 case WXK_INSERT: Notify( eAdd ); break;
										 case WXK_DELETE: Notify( eDelete ); break;
										 default: rEvent.Skip(); break;
										 }
									 } );
			return pList;
		}

		wxButton* MakeButton( wxWindow *pRoot, const char *pszLabel,
													SAIGeneralPointsWindowData::EAIGenPointsLastAction eAction )
		{
			wxButton *pButton = NWx::Child<wxButton>( pRoot, wxID_ANY,
																								wxString::FromUTF8( pszLabel ),
																								wxDefaultPosition, wxDefaultSize,
																								wxBU_EXACTFIT );
			pButton->Bind( wxEVT_BUTTON,
										 [this, eAction] ( wxCommandEvent& ) { Notify( eAction ); } );
			return pButton;
		}

		static SAIGeneralPointsWindowData::SAIPlayerInfo* CurrentPlayer(
				SAIGeneralPointsWindowData *pData )
		{
			// Guarded, where the MFC palette indexes players[] straight from
			// nCurrentPlayer. It has never been out of range in practice; a
			// palette that reads its own combo has no business trusting that.
			const int nPlayer = pData->nCurrentPlayer;
			if ( nPlayer < 0 || nPlayer >= (int)pData->players.size() )
			{
				return 0;
			}
			return &pData->players[nPlayer];
		}

		static void Select( wxListCtrl *pList, int nRow )
		{
			// Guarded, where the MFC call is not: CListCtrl::SetItemState takes -1
			// to mean every item; wxListCtrl would be asked about a row that does
			// not exist.
			if ( nRow >= 0 && nRow < pList->GetItemCount() )
			{
				pList->SetItemState( nRow, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
														 wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
			}
		}

		void Notify( SAIGeneralPointsWindowData::EAIGenPointsLastAction eAction )
		{
			eLastAction = eAction;
			if ( !bIsDataSetting )
			{
				CWaitCursor wcur;
				Singleton<ICommandHandlerContainer>()->HandleCommand(
						CHID_AIGEN_POINTS_STATE, ID_AIGEN_POINTS_WINDOW_UI_EVENT, 0 );
			}
			eLastAction = SAIGeneralPointsWindowData::AIGP_NO_ACTIONS;
		}

		void OnPlayerChanged( wxCommandEvent& )
		{
			Notify( SAIGeneralPointsWindowData::AIGP_PLAYER_JUMP );
		}

		void OnIDSelected( wxListEvent& )
		{
			if ( bIsDataSetting )
			{
				return;
			}
			Notify( SAIGeneralPointsWindowData::AIGP_ID_JUMP );
		}

		void OnParcelSelected( wxListEvent& )
		{
			// The MFC handler for the parcels list does not check bIsDataSetting
			// the way the IDs one does; Notify does it anyway, for both.
			Notify( SAIGeneralPointsWindowData::AIGP_PARCEL_JUMP );
		}

		void OnParcelActivated( wxListEvent& )
		{
			Notify( SAIGeneralPointsWindowData::AIGP_PARCEL_EDIT );
		}

		void OnIDsResized( wxSizeEvent &rEvent )
		{
			rEvent.Skip();
			// 10/90, as OnInitDialog splits the client width.
			const int nWidth = pIDs->GetClientSize().x;
			if ( pIDs->GetColumnCount() == 2 && nWidth > 0 )
			{
				pIDs->SetColumnWidth( 0, nWidth / 10 );
				pIDs->SetColumnWidth( 1, nWidth - nWidth / 10 );
			}
		}

		void OnParcelsResized( wxSizeEvent &rEvent )
		{
			rEvent.Skip();
			// 10/10/25/15/40, the same split OnInitDialog uses. The last column
			// takes the rounding so the five always add up to the width.
			const int nWidth = pParcels->GetClientSize().x;
			if ( pParcels->GetColumnCount() != 5 || nWidth <= 0 )
			{
				return;
			}
			const int nShares[4] = { nWidth / 10, nWidth / 10, ( nWidth * 25 ) / 100,
															 ( nWidth * 15 ) / 100 };
			int nUsed = 0;
			for ( int nColumn = 0; nColumn < 4; ++nColumn )
			{
				pParcels->SetColumnWidth( nColumn, nShares[nColumn] );
				nUsed += nShares[nColumn];
			}
			pParcels->SetColumnWidth( 4, nWidth - nUsed );
		}
	};
}


namespace NAIGeneralView
{
	CWnd* CreateWx( CDefault3DTabWindow *pTabWindow )
	{
		CAIGeneralWxWindow *pWindow = pTabWindow->AddNewTab( new CAIGeneralWxWindow() );
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
