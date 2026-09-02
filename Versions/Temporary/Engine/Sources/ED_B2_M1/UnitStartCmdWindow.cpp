#include "stdafx.h"

#include "CommandHandlerDefines.h"
#include "UnitStartCmdWindow.h"

#include <cstdint>

//
//
//		UNITS START COMMANDS WINDOW
//
//

BEGIN_MESSAGE_MAP(CUnitStartCmdWindow, CResizeDialog )
	
	ON_WM_DESTROY()
	
	ON_BN_CLICKED( IDC_BUTTON_ADD_CMD, OnButtonAdd )
	ON_BN_CLICKED( IDC_BUTTON_DEL_CMD, OnButtonDel )
	
	ON_BN_CLICKED(IDC_BUTTON_UP, OnButtonUp)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, OnButtonDown)

	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_UNIT_CMD, OnLvnItemchangedListUnitCmd)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_UNIT_CMD, OnNMDblclkListUnitCmd)

END_MESSAGE_MAP()


CUnitStartCmdWindow::CUnitStartCmdWindow( CWnd* pParentWindow )
	: CResizeDialog( CUnitStartCmdWindow::IDD, pParentWindow ),
	bIsDataBeginSet( false ),
	eLastAction( SUnitStartCmdWindowData::NO_CMD )
{
	SetControlStyle( IDC_BUTTON_UP, ANCHORE_RIGHT );
	SetControlStyle( IDC_BUTTON_DOWN, ANCHORE_RIGHT );
	SetControlStyle( IDC_LIST_UNIT_CMD, ANCHORE_LEFT | ANCHORE_RIGHT | ANCHORE_BOTTOM | RESIZE_HOR_VER );

	Singleton<ICommandHandlerContainer>()->Set( CHID_UNIT_START_CMD_WINDOW, this );
}


CUnitStartCmdWindow::~CUnitStartCmdWindow()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_UNIT_START_CMD_WINDOW );
}


void CUnitStartCmdWindow::DoDataExchange( CDataExchange *pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_LIST_UNIT_CMD, lcCommands );
	DDX_Control( pDX, IDC_BUTTON_ADD_CMD, btnAdd );
	DDX_Control( pDX, IDC_BUTTON_DEL_CMD, btnDel );
	DDX_Control( pDX, IDC_BUTTON_UP, btnUp );
	DDX_Control( pDX, IDC_BUTTON_DOWN, btnDown );
}


BOOL CUnitStartCmdWindow::OnInitDialog()
{
	CResizeDialog::OnInitDialog();
	//
	lcCommands.ModifyStyle( 0, LVS_ALIGNLEFT | LVS_REPORT, 0 ); 
	lcCommands.SetExtendedStyle( LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP ); 
	lcCommands.EnableToolTips( TRUE );
	CRect r;
	lcCommands.GetClientRect( &r );
	lcCommands.InsertColumn( 0, RCSTR("Type"), LVCFMT_LEFT, r.Width() * 0.6 );
	lcCommands.InsertColumn( 1, RCSTR("Target"), LVCFMT_LEFT, r.Width() * 0.4 );
	//
	return TRUE;
}


bool CUnitStartCmdWindow::HandleCommand( unsigned nCommandID, uintptr_t dwData )
{
	SUnitStartCmdWindowData *pData = reinterpret_cast<SUnitStartCmdWindowData*>( dwData );
	//
	NI_ASSERT( pData, "CUnitStartCmdWindow::HandleCommand(): dwData == 0" );
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
		//
		case ID_WINDOW_SET_DIALOG_DATA:
		{
			SetDialogData( pData );
			return true;
		}
	}

	return false;
}


bool CUnitStartCmdWindow::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CUnitStartCmdWindow::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CUnitStartCmdWindow::UpdateCommand(), pbCheck == 0" );
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


void CUnitStartCmdWindow::NotifyHandler()
{
	if ( bIsDataBeginSet )
		return;	// чтобы сообщения не шли в момент установки свойств контролов

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_UNIT_START_CMD_STATE, ID_UNIT_START_CMD_WINDOW_UI_EVENT, 0 );
}


// useful NotifyHandler
void CUnitStartCmdWindow::NotifyHandler( SUnitStartCmdWindowData::EAction eAction )
{
	SetLastAction( eAction );
	NotifyHandler();
	SetLastAction( SUnitStartCmdWindowData::NO_CMD );
}


void CUnitStartCmdWindow::GetDialogData( SUnitStartCmdWindowData *pData )
{
	if ( !pData )
		return;

	pData->Clear();
	// pData->commands -- не возвращаются, т.к. никому не нужны
	pData->eLastAction = eLastAction;
	for ( int i = 0; i < lcCommands.GetItemCount(); ++i )
	{
		if ( lcCommands.GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED )
		{
			pData->selectedCommands.push_back( lcCommands.GetItemData(i) );
		}
	}
}


void CUnitStartCmdWindow::SetDialogData( const SUnitStartCmdWindowData *pData )
{
	if ( !pData )
		return;

	bIsDataBeginSet = true;
	//
	eLastAction = SUnitStartCmdWindowData::NO_CMD;
	//
  lcCommands.DeleteAllItems();
	for ( int i = 0; i < pData->commands.size(); ++i )
	{
		const SUnitStartCmdWindowData::SCmd &cmd = pData->commands[i];
		int nItem = lcCommands.InsertItem( i, "" );
		lcCommands.SetItemText( nItem, 0, cmd.szType.c_str() );
		lcCommands.SetItemText( i, 1,  (const char*)cmd.szTarget.c_str() );
		lcCommands.SetItemData( i, cmd.nIndex );
	}
	//
	for ( int i = 0 ; i < pData->selectedCommands.size(); ++i )
	{
		lcCommands.SetItemState( pData->selectedCommands[i], LVIS_SELECTED, LVIS_SELECTED );
	}

	bIsDataBeginSet = false;
}


void CUnitStartCmdWindow::OnDestroy()
{
	SaveResizeDialogOptions();
	CResizeDialog::OnDestroy();
}


void CUnitStartCmdWindow::OnButtonAdd()
{
	NotifyHandler( SUnitStartCmdWindowData::ADD_CMD );
}


void CUnitStartCmdWindow::OnButtonDel()
{
	NotifyHandler( SUnitStartCmdWindowData::DEL_CMD );
}


void CUnitStartCmdWindow::OnButtonUp()
{
	NotifyHandler( SUnitStartCmdWindowData::ORDER_UP_CMD );
}


void CUnitStartCmdWindow::OnButtonDown()
{
	NotifyHandler( SUnitStartCmdWindowData::ORDER_DOWN_CMD );
}


void CUnitStartCmdWindow::OnNMDblclkListUnitCmd( NMHDR *pNMHDR, LRESULT *pResult )
{
	(*pResult) = 0;
	NotifyHandler( SUnitStartCmdWindowData::EDIT_CMD );
}


void CUnitStartCmdWindow::OnLvnItemchangedListUnitCmd( NMHDR *pNMHDR, LRESULT *pResult )
{
	CWaitCursor wcur;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	(*pResult) = 0;

	if ( (pNMLV->uChanged & LVIF_STATE) && 
			 (pNMLV->uNewState & LVIS_SELECTED) &&
			 !(pNMLV->uOldState & LVIS_SELECTED) )
	{
		NotifyHandler( SUnitStartCmdWindowData::SEL_CHANGE );
	}
}


