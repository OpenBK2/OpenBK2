#include "stdafx.h"

#include "CommandHandlerDefines.h"
#include "StringResources.h"
#include "ReinfPointsWindow.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//		REINFPOINTS WINDOW
//
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CReinfPointsWindow, CResizeDialog)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_CBN_SELCHANGE(IDC_COMBO_PLAYER, OnCbnSelchangeComboPlayer)
	ON_BN_CLICKED(IDC_BUTTON_REINF_POINTS_ADD, OnBnClickedButtonReinfPointsAdd)
	ON_BN_CLICKED(IDC_BUTTON_REINF_POINTS_DEL, OnBnClickedButtonReinfPointsDel)
	ON_BN_CLICKED(IDC_BUTTON_REINF_POINTS_DEPLOY, OnBnClickedButtonReinfPointsDeploy)
	ON_BN_CLICKED(IDC_BUTTON_REINF_POINTS_TYPED, OnBnClickedButtonReinfPointsTyped)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_REINF_POINTS, OnLvnItemchangedListReinfPoints)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_REINF_POINTS, OnLvnDblclkListReinfPoints)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CReinfPointsWindow::CReinfPointsWindow( CWnd* pParentWindow )
	:	CResizeDialog( CReinfPointsWindow::IDD, pParentWindow ),
	bIsDataSetting( false ),
	eLastAction( SReinfPointsWindowData::RWA_NO_ACTIONS )
{
	SetControlStyle( IDC_COMBO_PLAYER, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_RP_DELIMITER_0, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_LIST_REINF_POINTS, ANCHORE_LEFT | ANCHORE_RIGHT | ANCHORE_BOTTOM | RESIZE_HOR_VER );
	Singleton<ICommandHandlerContainer>()->Set( CHID_REINF_POINTS_WINDOW, this );
	//
	nSelectedIndex = -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CReinfPointsWindow::~CReinfPointsWindow()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_REINF_POINTS_WINDOW );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CReinfPointsWindow::OnInitDialog()
{
	CResizeDialog::OnInitDialog();
	//
	CRect rect;
	lcReinfPoints.ModifyStyle( 0, LVS_ALIGNLEFT | LVS_REPORT, 0 ); 
	lcReinfPoints.SetExtendedStyle( LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP ); 
	lcReinfPoints.EnableToolTips( TRUE );

	lcReinfPoints.GetClientRect( &rect );
	lcReinfPoints.InsertColumn( 0, RCSTR("ID"), LVCFMT_LEFT, 30 );
	lcReinfPoints.InsertColumn( 1, RCSTR("Deploy template"), LVCFMT_LEFT, (rect.Width()-30)*0.5f );
	lcReinfPoints.InsertColumn( 2, RCSTR("Typed templates"), LVCFMT_LEFT, (rect.Width()-30)*0.5f );
	//
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CReinfPointsWindow::OnDestroy() 
{
	SaveResizeDialogOptions();
	CResizeDialog::OnDestroy();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CReinfPointsWindow::DoDataExchange( CDataExchange *pDX )
{
	CResizeDialog::DoDataExchange( pDX ); 
	DDX_Control( pDX, IDC_LIST_REINF_POINTS, lcReinfPoints );
	DDX_Control( pDX, IDC_COMBO_PLAYER, wndPlayerComboBox );
	DDX_Control( pDX, IDC_BUTTON_REINF_POINTS_DEL, btnDel );
	DDX_Control( pDX, IDC_BUTTON_REINF_POINTS_DEPLOY, btnDeploy );
	DDX_Control( pDX, IDC_BUTTON_REINF_POINTS_TYPED, btnTyped );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CReinfPointsWindow::GetDialogData( SReinfPointsWindowData *pData )
{
	pData->Clear();

	pData->nPlayerCount = wndPlayerComboBox.GetCount();

	const int nStringNumber = wndPlayerComboBox.GetCurSel();
	if ( nStringNumber >= 0 )
		pData->nPlayerIndex = wndPlayerComboBox.GetItemData( nStringNumber );
	else
		pData->nPlayerIndex = -1;

	pData->nSelectedPoint = -1; // = nSelectedIndex
	for ( int i = 0; i < lcReinfPoints.GetItemCount(); ++i )
	{
		if ( lcReinfPoints.GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED )
		{
			pData->nSelectedPoint = i;
		}
	}

	pData->eLastAction = eLastAction;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CReinfPointsWindow::SetDialogData( const SReinfPointsWindowData *pData )
{
	nSelectedIndex = pData->nSelectedPoint;

	if ( !pData )
		return;
	if ( pData->bAviationPointSelected )
		return;

	bIsDataSetting = true; // флаг установки свойств контролов

	wndPlayerComboBox.ResetContent();
	for ( int nPlayerIndex = 0; nPlayerIndex < pData->nPlayerCount; ++nPlayerIndex )
	{
		const int nStringNumber = wndPlayerComboBox.AddString( StrFmt( "%d", nPlayerIndex ) );
		wndPlayerComboBox.SetItemData( nStringNumber, nPlayerIndex );
	}
	wndPlayerComboBox.SelectString( 0, StrFmt( "%d", pData->nPlayerIndex ) );
	//
	lcReinfPoints.DeleteAllItems();
	for ( int i = 0; i < pData->reinfPoints.size(); ++i )
	{
		const SReinfPointsWindowData::SReinfPoint &rp = pData->reinfPoints[i];
		int nItem = lcReinfPoints.InsertItem( i, "" );
		lcReinfPoints.SetItemText( nItem, 0, StrFmt("%d", i) );
		lcReinfPoints.SetItemText( nItem, 1, StrFmt("%s", rp.szDeployTemplate) );
		lcReinfPoints.SetItemText( nItem, 2, StrFmt( "%d", rp.typedTemplates.size()) );
		lcReinfPoints.SetItemData( nItem, i );
	}
	lcReinfPoints.SetItemState( pData->nSelectedPoint, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED );

	btnDel.EnableWindow( pData->nSelectedPoint != -1 );
	btnDeploy.EnableWindow( pData->nSelectedPoint != -1 );
	btnTyped.EnableWindow( pData->nSelectedPoint != -1 );

	eLastAction = SReinfPointsWindowData::RWA_NO_ACTIONS;
	//
	bIsDataSetting = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CReinfPointsWindow::HandleCommand( UINT nCommandID, DWORD dwData )
{
	SReinfPointsWindowData *pData = reinterpret_cast<SReinfPointsWindowData*>( dwData );

	switch( nCommandID ) 
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CReinfPointsWindow::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CReinfPointsWindow::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CReinfPointsWindow::UpdateCommand(), pbCheck == 0" );
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
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CReinfPointsWindow::NotifyHandler()
{
	if ( bIsDataSetting )
		return;	// чтобы сообщения не шли в момент установки свойств контролов

	CWaitCursor wcur;

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_REINF_POINTS_STATE, 
																												ID_REINF_POINTS_WINDOW_CHANGE_STATE, 
																												0 );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CReinfPointsWindow::OnCbnSelchangeComboPlayer()
{
	CWaitCursor wcur;
	eLastAction = SReinfPointsWindowData::RWA_PLAYER_CHANGE;
	NotifyHandler();
	eLastAction = SReinfPointsWindowData::RWA_NO_ACTIONS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CReinfPointsWindow::OnBnClickedButtonReinfPointsAdd()
{
	CWaitCursor wcur;
	eLastAction = SReinfPointsWindowData::RWA_POINT_ADD;
	NotifyHandler();
	eLastAction = SReinfPointsWindowData::RWA_NO_ACTIONS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CReinfPointsWindow::OnBnClickedButtonReinfPointsDel()
{
	CWaitCursor wcur;
	eLastAction = SReinfPointsWindowData::RWA_POINT_DEL;
	NotifyHandler();
	eLastAction = SReinfPointsWindowData::RWA_NO_ACTIONS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CReinfPointsWindow::OnBnClickedButtonReinfPointsDeploy()
{
	if ( nSelectedIndex == -1 )
		return;

	CWaitCursor wcur;
	eLastAction = SReinfPointsWindowData::RWA_POINT_EDIT_DEPLOY;
	NotifyHandler();
	eLastAction = SReinfPointsWindowData::RWA_NO_ACTIONS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CReinfPointsWindow::OnBnClickedButtonReinfPointsTyped()
{
	if ( nSelectedIndex == -1 )
		return;

	CWaitCursor wcur;
	eLastAction = SReinfPointsWindowData::RWA_POINT_EDIT_TYPED;
	NotifyHandler();
	eLastAction = SReinfPointsWindowData::RWA_NO_ACTIONS;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CReinfPointsWindow::OnLvnItemchangedListReinfPoints( NMHDR *pNMHDR, LRESULT *pResult )
{
	if ( bIsDataSetting )
		return;

	CWaitCursor wcur;
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>( pNMHDR );

	nSelectedIndex = pNMLV->iItem;

	if (	(pNMLV->uChanged & LVIF_STATE) && 
				(pNMLV->uNewState & LVIS_SELECTED) &&
				!(pNMLV->uOldState & LVIS_SELECTED) )
	{
		btnDel.EnableWindow( nSelectedIndex != -1 );
		btnDeploy.EnableWindow( nSelectedIndex != -1 );
		btnTyped.EnableWindow( nSelectedIndex != -1 );

		eLastAction = SReinfPointsWindowData::RWA_POINT_SEL_CHANGE;
		NotifyHandler();
		eLastAction = SReinfPointsWindowData::RWA_NO_ACTIONS;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CReinfPointsWindow::OnLvnDblclkListReinfPoints( NMHDR *pNMHDR, LRESULT *pResult )
{
	CWaitCursor wcur;
	eLastAction = SReinfPointsWindowData::RWA_POINT_JUMP;
	NotifyHandler();
	eLastAction = SReinfPointsWindowData::RWA_NO_ACTIONS;

	*pResult = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CReinfPointsWindow::OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags )
{
	switch ( nChar )
	{
	case VK_INSERT:
		OnBnClickedButtonReinfPointsAdd();
		break;
	case VK_DELETE:
		OnBnClickedButtonReinfPointsDel();
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
