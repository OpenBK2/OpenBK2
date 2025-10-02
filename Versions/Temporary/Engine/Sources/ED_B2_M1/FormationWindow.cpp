#include "stdafx.h"

//#include "..\misc\2darray.h"
#include "CommandHandlerDefines.h"
#include "FormationMnemonics.h"
#include "StringResources.h"
#include "FormationWindow.h"

#include <zconf.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//
//						FORMATION WINDOW
//
//

BEGIN_MESSAGE_MAP(CFormationWindow, CResizeDialog)
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_FORMATIONS, OnLvnItemchangedPointsList)
	ON_BN_CLICKED(IDC_CHECK_PROPMASK, OnBnClickedCheckPropmask)
END_MESSAGE_MAP()


CFormationWindow::CFormationWindow( CWnd *pParentWindow )
	:	CResizeDialog( CFormationWindow::IDD, pParentWindow ),
	nSelectedIndex( -1 ),
	bIsDataSetting( false )
{
	SetControlStyle( IDC_LIST_FORMATIONS, ANCHORE_LEFT_TOP | ANCHORE_HOR_CENTER | RESIZE_HOR_VER );

	Singleton<ICommandHandlerContainer>()->Set( CHID_FORMATION_LIST_DIALOG, this );
}


CFormationWindow::~CFormationWindow()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_FORMATION_LIST_DIALOG );
}


BOOL CFormationWindow::OnInitDialog()
{
	CResizeDialog::OnInitDialog();

	CRect rect;
	formationsList.ModifyStyle( 0, LVS_ALIGNLEFT | LVS_REPORT, 0 ); 
	formationsList.SetExtendedStyle( LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP ); 
	formationsList.EnableToolTips( TRUE );
	formationsList.GetClientRect( &rect );
	formationsList.InsertColumn( 0, RCSTR("Formation types"), LVCFMT_CENTER, rect.Width() );

	return TRUE;
}


void CFormationWindow::OnDestroy() 
{
	SaveResizeDialogOptions();
	CResizeDialog::OnDestroy();
}


void CFormationWindow::DoDataExchange( CDataExchange *pDX )
{
	CResizeDialog::DoDataExchange( pDX ); 
	DDX_Control( pDX, IDC_LIST_FORMATIONS, formationsList );
	DDX_Control( pDX, IDC_CHECK_PROPMASK, chkPropMask );
}


void CFormationWindow::OnLvnItemchangedPointsList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	(*pResult) = 0;

	if (	(pNMLV->uChanged & LVIF_STATE) && 
				(pNMLV->uNewState & LVIS_SELECTED) &&
				!(pNMLV->uOldState & LVIS_SELECTED) )
	{
		nSelectedIndex = pNMLV->iItem;
		NotifyHandler();
	}
}


void CFormationWindow::NotifyHandler()
{
	if ( bIsDataSetting )
		return;

	CWaitCursor wcur;
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SQUAD_FORMATIONS_STATE, 
																												ID_FORMATION_WINDOW_CHANGE_STATE, 
																												static_cast<DWORD>(nSelectedIndex) );
}


void CFormationWindow::GetDialogData( SFormationWindowDialogData *pData )
{
	for ( int i = 0; i < formationsList.GetItemCount(); ++i )
	{
		if ( formationsList.GetItemState(i, LVIS_SELECTED) & LVIS_SELECTED )
		{
			nSelectedIndex = i;
			break;
		}
	}
	//
	pData->eSelectedFormation = NDb::SSquadRPGStats::SFormation::EFormationMoveType(-1); 
	if ( nSelectedIndex != -1 )
	{
		pData->eSelectedFormation = static_cast<NDb::SSquadRPGStats::SFormation::EFormationMoveType>(formationsList.GetItemData(nSelectedIndex) );
	}
	//
	pData->bChkPropmask = (chkPropMask.GetCheck() == BST_CHECKED);
}


void CFormationWindow::SetDialogData( const SFormationWindowDialogData *pData )
{
	bIsDataSetting = true;

	formationsList.DeleteAllItems();
	for ( int i = 0; i < pData->squadFormations.size(); ++i )
	{
		int nIdx = formationsList.InsertItem( i, typeFormationMnemonics.GetMnemonic(pData->squadFormations[i]).c_str(), -1 );
		formationsList.SetItemData( nIdx, pData->squadFormations[i] );
	}
	//
	if ( (formationsList.GetItemCount() > 0) && (nSelectedIndex < 0) )
		nSelectedIndex = 0;
	// 
	formationsList.SetItemState( nSelectedIndex, LVIS_SELECTED, LVIS_SELECTED );
	//
	chkPropMask.SetCheck( pData->bChkPropmask ? BST_CHECKED : BST_UNCHECKED );

	bIsDataSetting = false;
}


bool CFormationWindow::HandleCommand( UINT nCommandID, DWORD dwData )
{
	SFormationWindowDialogData *pData = reinterpret_cast<SFormationWindowDialogData*>( dwData );
	
	switch( nCommandID ) 
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


bool CFormationWindow::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CFormationWindow::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CFormationWindow::UpdateCommand(), pbCheck == 0" );
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


void CFormationWindow::OnBnClickedCheckPropmask()
{
	NotifyHandler();
}



