#include "stdafx.h"

#include "Misc/2Darray.h"
#include "commandhandlerdefines.h"
#include "stringresources.h"
#include "aigeneraltypes.h"
#include "AIGeneralWindow.h"

#include <cstdint>

#include <zconf.h>

//
//
//		AI GENERAL POINTS WINDOW
//
//

BEGIN_MESSAGE_MAP( CAIGeneralPointsWindow, CResizeDialog )
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_COMBO_PLAYER, OnChangePlayerCombo)

	ON_NOTIFY(LVN_ITEMCHANGED, IDC_AIGEN_LIST_PARCELS, OnLvnItemchangedListParcels)
	ON_NOTIFY(LVN_KEYDOWN, IDC_AIGEN_LIST_PARCELS, OnLvnKeydownAigenListParcels)
	ON_BN_CLICKED(IDC_AIGEN_BUTTON_ADD_PARCEL, OnBnClickedAddParcel)
	ON_BN_CLICKED(IDC_AIGEN_BUTTON_DEL_PARCEL, OnBnClickedDeleteParcel)

	ON_NOTIFY(LVN_ITEMCHANGED, IDC_AIGEN_LIST_IDS, OnLvnItemchangedListIDs)
	ON_NOTIFY(LVN_KEYDOWN, IDC_AIGEN_LIST_IDS, OnLvnKeydownAigenListIDs)
	ON_BN_CLICKED(IDC_AIGEN_BUTTON_ADD_ID, OnBnClickedAddID)
	ON_BN_CLICKED(IDC_AIGEN_BUTTON_DEL_ID, OnBnClickedDeleteID)

	ON_NOTIFY(NM_DBLCLK, IDC_AIGEN_LIST_PARCELS, OnNMDblclkAigenListParcels)
END_MESSAGE_MAP()


CAIGeneralPointsWindow::CAIGeneralPointsWindow( CWnd* pParentWindow )
	: CResizeDialog( CAIGeneralPointsWindow::IDD, pParentWindow ),
	bIsDataSetting( false )
{
	SetControlStyle( IDC_COMBO_PLAYER, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_AIGEN_COMBO_PTYPE, ANCHORE_LEFT_TOP | RESIZE_HOR );

	SetControlStyle( IDC_AIGEN_DELIMITER_0, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_AIGEN_LABEL_IDS, ANCHORE_LEFT_TOP | ANCHORE_LEFT );
	SetControlStyle( IDC_AIGEN_BUTTON_ADD_ID, ANCHORE_RIGHT_TOP );
	SetControlStyle( IDC_AIGEN_BUTTON_DEL_ID, ANCHORE_RIGHT_TOP );
	SetControlStyle( IDC_AIGEN_LIST_IDS, ANCHORE_LEFT_TOP | RESIZE_HOR | RESIZE_VER, 1.0f, 1.0f, 1.0f, 0.5f );

	SetControlStyle( IDC_AIGEN_DELIMITER_1, ANCHORE_VER_CENTER | ANCHORE_LEFT | RESIZE_HOR );
	SetControlStyle( IDC_AIGEN_LABEL_PARCELS, ANCHORE_VER_CENTER | ANCHORE_LEFT, 1.0f, 0.5f, 1.0f, 1.0f );
	SetControlStyle( IDC_AIGEN_BUTTON_ADD_PARCEL, ANCHORE_VER_CENTER | ANCHORE_RIGHT );
	SetControlStyle( IDC_AIGEN_BUTTON_DEL_PARCEL, ANCHORE_VER_CENTER | ANCHORE_RIGHT );

	SetControlStyle( IDC_AIGEN_LIST_PARCELS, ANCHORE_LEFT_BOTTOM | RESIZE_HOR | RESIZE_VER, 1.0f, 1.0f, 1.0f, 0.5f );

	Singleton<ICommandHandlerContainer>()->Set( CHID_AIGEN_POINTS_WINDOW, this );
}


CAIGeneralPointsWindow::~CAIGeneralPointsWindow()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_AIGEN_POINTS_WINDOW );
}


void CAIGeneralPointsWindow::DoDataExchange( CDataExchange *pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_AIGEN_LIST_IDS, lcIDs );
	DDX_Control( pDX, IDC_AIGEN_LIST_PARCELS, lcParcels );
	DDX_Control( pDX, IDC_AIGEN_BUTTON_ADD_ID, btnIDAdd );
	DDX_Control( pDX, IDC_AIGEN_BUTTON_DEL_ID, btnIDDel );
	DDX_Control( pDX, IDC_AIGEN_BUTTON_ADD_PARCEL, btnParcelAdd );
	DDX_Control( pDX, IDC_AIGEN_BUTTON_DEL_PARCEL, btnParcelDel );
	DDX_Control( pDX, IDC_COMBO_PLAYER, comboPlayer );
}


BOOL CAIGeneralPointsWindow::OnInitDialog()
{
	bIsDataSetting = true;
	CResizeDialog::OnInitDialog();
	//
	CRect rect;
	lcIDs.ModifyStyle( 0, LVS_ALIGNLEFT | LVS_REPORT, 0 ); 
	lcIDs.SetExtendedStyle( LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP ); 
	lcIDs.EnableToolTips( TRUE );
	lcIDs.GetClientRect( &rect );
	lcIDs.InsertColumn( 0, RCSTR("#"), LVCFMT_LEFT, rect.Width()*0.1f );
	lcIDs.InsertColumn( 1, RCSTR("ID"), LVCFMT_LEFT, rect.Width()*0.9f );
	//
	lcParcels.ModifyStyle( 0, LVS_ALIGNLEFT | LVS_REPORT, 0 ); 
	lcParcels.SetExtendedStyle( LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP );
	lcParcels.EnableToolTips( TRUE );
	lcParcels.GetClientRect( &rect );
	lcParcels.InsertColumn( 0, RCSTR("#"), LVCFMT_LEFT, rect.Width()*0.1f );
	lcParcels.InsertColumn( 1, RCSTR("Points"), LVCFMT_LEFT, rect.Width()*0.1f );
	lcParcels.InsertColumn( 2, RCSTR("Position"), LVCFMT_LEFT, rect.Width()*0.25f );
	lcParcels.InsertColumn( 3, RCSTR("Importance"), LVCFMT_LEFT, rect.Width()*0.15f );
	lcParcels.InsertColumn( 4, RCSTR("Type"), LVCFMT_LEFT, rect.Width()*0.4f );
	//
	bIsDataSetting = false;

	return TRUE;
}


bool CAIGeneralPointsWindow::HandleCommand( UINT nCommandID, uint32_t dwData )
{
	SAIGeneralPointsWindowData *pData = reinterpret_cast<SAIGeneralPointsWindowData*>( dwData );
	//
	NI_ASSERT( pData, "CAIGeneralPointsWindow::HandleCommand(): dwData == 0" );
	if ( !pData )
		return false;
	//
	switch ( nCommandID )
	{
	case ID_WINDOW_GET_DIALOG_DATA:
		{
			GetDialogData( pData );
			return true;
		}
		break;
	case ID_WINDOW_SET_DIALOG_DATA:
		{
			SetDialogData( pData );
			return true;
		}
		break;
	}

	return false;
}


bool CAIGeneralPointsWindow::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CAIGeneralPointsWindow::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CAIGeneralPointsWindow::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID ) 
	{
	case ID_WINDOW_GET_DIALOG_DATA:
	case ID_WINDOW_SET_DIALOG_DATA:
		( *pbEnable ) = true;
		( *pbCheck ) = false;
		return true;
	default:
		return false;
	}

	return false;
}


void CAIGeneralPointsWindow::NotifyHandler()
{
	if ( bIsDataSetting )
		return;	// to disable messages during controls setup

  CWaitCursor wcur;
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_AIGEN_POINTS_STATE,
																												ID_AIGEN_POINTS_WINDOW_UI_EVENT, 0 );
}


// useful NotifyHandler
void CAIGeneralPointsWindow::NotifyHandler( SAIGeneralPointsWindowData::EAIGenPointsLastAction eAction )
{
	SetLastAction( eAction );
	NotifyHandler();
	SetLastAction( SAIGeneralPointsWindowData::AIGP_NO_ACTIONS );
}


void CAIGeneralPointsWindow::OnDestroy()
{
	SaveResizeDialogOptions();
	CResizeDialog::OnDestroy();
}


void CAIGeneralPointsWindow::GetDialogData( SAIGeneralPointsWindowData *pData )
{
	pData->eLastAction = eLastAction;
	switch ( pData->eLastAction )
	{
		case SAIGeneralPointsWindowData::AIGP_PLAYER_JUMP:
		{
			pData->nCurrentPlayer = comboPlayer.GetCurSel();
			break;
		}
		//
		case SAIGeneralPointsWindowData::AIGP_ID_JUMP:
		{
			for ( int i = 0; i < lcIDs.GetItemCount(); ++i )
			{
				if ( lcIDs.GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED )
				{
					pData->players[pData->CurrentPlayer()].nCurrentID = i;
					break;
				}
			}
			break;
		}
		//
		case SAIGeneralPointsWindowData::AIGP_PARCEL_JUMP:
		{
			for ( int i = 0; i < lcParcels.GetItemCount(); ++i )
			{
				if ( lcParcels.GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED )
				{
					pData->players[pData->CurrentPlayer()].nCurrentParcel = i;
					break;
				}
			}
			break;
		}
	}
}


void CAIGeneralPointsWindow::SetDialogData( const SAIGeneralPointsWindowData *pData )
{
	if ( !pData )
		return;

	bIsDataSetting = true;
  SAIGeneralPointsWindowData dialogData = *pData;
	SAIGeneralPointsWindowData::SAIPlayerInfo &player = dialogData.players[dialogData.CurrentPlayer()];
	//
	comboPlayer.ResetContent();
	for ( int nPlayer = 0; nPlayer < dialogData.players.size(); ++nPlayer )
	{
		const int nStringNumber = comboPlayer.AddString( StrFmt("%d", nPlayer) );
		comboPlayer.SetItemData( nStringNumber, nPlayer );
	}
	comboPlayer.SelectString( 0, StrFmt("%d", dialogData.CurrentPlayer()) );
	//
	lcIDs.DeleteAllItems();	
	for ( int nID = 0; nID < player.mobileScriptIDs.size(); ++nID )
	{
		const int nItem = lcIDs.InsertItem( nID, "" );
		lcIDs.SetItemText( nItem, 0, StrFmt("%d", nID) );
		lcIDs.SetItemText( nItem, 1, StrFmt("%d", player.mobileScriptIDs[nID]) );
		lcIDs.SetItemData( nItem, nID );
	}
	lcIDs.SetItemState( dialogData.CurrentID(), LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED );
	btnIDDel.EnableWindow( (player.mobileScriptIDs.size() > 0) && (dialogData.CurrentID() >= 0) );
	//
	lcParcels.DeleteAllItems();
	for ( int nParcel = 0; nParcel < player.parcels.size(); ++nParcel )
	{
		const int nItem = lcParcels.InsertItem( nParcel, "" );
		lcParcels.SetItemText( nItem, 0, StrFmt("%d", nParcel) );
		lcParcels.SetItemText( nItem, 1, StrFmt("%d", player.parcels[nParcel].reinforcePoints.size()) );
		lcParcels.SetItemText( nItem, 2, StrFmt("%.0f, %.0f", player.parcels[nParcel].vCenter.x, player.parcels[nParcel].vCenter.y) );
		lcParcels.SetItemText( nItem, 3, StrFmt("%.2f", player.parcels[nParcel].fImportance) );
		lcParcels.SetItemText( nItem, 4, typeAIGeneralParcel.GetMnemonic(player.parcels[nParcel].eType).c_str() );
		lcParcels.SetItemData( nItem, nParcel );
	}
	lcParcels.SetItemState( dialogData.CurrentParcel(), LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED );
	btnParcelDel.EnableWindow( (player.parcels.size() > 0) && (dialogData.CurrentParcel() >= 0) );
	dialogData.eLastAction = SAIGeneralPointsWindowData::AIGP_NO_ACTIONS;
	//
	bIsDataSetting = false;
}


void CAIGeneralPointsWindow::OnChangePlayerCombo()
{
	NotifyHandler( SAIGeneralPointsWindowData::AIGP_PLAYER_JUMP );
}


void CAIGeneralPointsWindow::OnLvnItemchangedListIDs( NMHDR *pNMHDR, LRESULT *pResult )
{
	(*pResult) = 0;
	if ( bIsDataSetting )
		return;

	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>( pNMHDR );

	if ( (pNMLV->uChanged & LVIF_STATE) && 
			 (pNMLV->uNewState & LVIS_SELECTED) &&
			 !(pNMLV->uOldState & LVIS_SELECTED) )
	{
		NotifyHandler( SAIGeneralPointsWindowData::AIGP_ID_JUMP );
	}
}


void CAIGeneralPointsWindow::OnLvnItemchangedListParcels( NMHDR *pNMHDR, LRESULT *pResult )
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>( pNMHDR );
	(*pResult) = 0;

	if ( (pNMLV->uChanged & LVIF_STATE) && 
			 (pNMLV->uNewState & LVIS_SELECTED) &&
			 !(pNMLV->uOldState & LVIS_SELECTED) )
	{
		NotifyHandler( SAIGeneralPointsWindowData::AIGP_PARCEL_JUMP );
	}
}


void CAIGeneralPointsWindow::OnBnClickedAddID()
{
	NotifyHandler( SAIGeneralPointsWindowData::AIGP_ID_ADD );
}


void CAIGeneralPointsWindow::OnBnClickedDeleteID()
{
	NotifyHandler( SAIGeneralPointsWindowData::AIGP_ID_DEL );
}


void CAIGeneralPointsWindow::OnBnClickedAddParcel()
{
	NotifyHandler( SAIGeneralPointsWindowData::AIGP_PARCEL_ADD );
}


void CAIGeneralPointsWindow::OnBnClickedDeleteParcel()
{
	NotifyHandler( SAIGeneralPointsWindowData::AIGP_PARCEL_DEL );
}


void CAIGeneralPointsWindow::OnLvnKeydownAigenListParcels( NMHDR *pNMHDR, LRESULT *pResult )
{
  LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	(*pResult) = 0;

	switch ( pLVKeyDown->wVKey )
	{
	case VK_DELETE:
		OnBnClickedDeleteParcel();
		break;
		//
	case VK_INSERT:
		OnBnClickedAddParcel();
		break;
	}
}


void CAIGeneralPointsWindow::OnLvnKeydownAigenListIDs( NMHDR *pNMHDR, LRESULT *pResult )
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	(*pResult) = 0;

	switch ( pLVKeyDown->wVKey )
	{
	case VK_DELETE:
		OnBnClickedDeleteID();
		break;
		//
	case VK_INSERT:
		OnBnClickedAddID();
		break;
	}
}


void CAIGeneralPointsWindow::OnNMDblclkAigenListParcels(NMHDR *pNMHDR, LRESULT *pResult)
{
	(*pResult) = 0;
	NotifyHandler( SAIGeneralPointsWindowData::AIGP_PARCEL_EDIT );
}



