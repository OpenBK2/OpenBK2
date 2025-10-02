#include "stdafx.h"

#include "ED_B2_M1Dll.h"
#include "UnitStartCmdState.h"
#include "EdUnitStartCmd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//
//		UNIT START COMMAND EDITOR
//
//

BEGIN_MESSAGE_MAP(CEdUnitStartCmd,CResizeDialog)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, OnBnClickedButtonClear)
	ON_CBN_SELCHANGE(IDC_COMBO_CMD_TYPE, OnCbnSelchangeComboCmdType)
END_MESSAGE_MAP()

CEdUnitStartCmd::CEdUnitStartCmd( CUnitStartCmdState *_pCommandState )
	:	CResizeDialog( CEdUnitStartCmd::IDD ),
	pCommandState( _pCommandState ),
	bEditMode( true ),
	nCommandIndex( -1 )
{
	if ( !pCommandState )
	{
		NI_ASSERT( pCommandState, "CEdUnitStartCmd::CEdUnitStartCmd(): pCommandState == 0" );
	}
	SetControlStyle( IDC_EDIT_TARGET_UNIT, ANCHORE_LEFT | RESIZE_HOR );
	SetControlStyle( IDC_BUTTON_CLEAR, ANCHORE_RIGHT	);
	SetControlStyle( IDC_EDIT_CMD_PARAM, ANCHORE_LEFT | RESIZE_HOR );
	SetControlStyle( IDC_COMBO_CMD_TYPE, ANCHORE_LEFT | RESIZE_HOR );
}


void CEdUnitStartCmd::DoDataExchange( CDataExchange *pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_COMBO_CMD_TYPE, cbCmdTypes );
	DDX_Control( pDX, IDC_EDIT_CMD_PARAM, edData );
	DDX_Control( pDX, IDC_EDIT_TARGET_UNIT, edTgtUnit );
	DDX_Control( pDX, IDC_BUTTON_CLEAR, btnClear );
}

BOOL CEdUnitStartCmd::OnInitDialog()
{
	CResizeDialog::OnInitDialog();

	LoadUnitCommandTypesFromXML( &cmdTypes );

	SDlgData dd;
	SetDialogData( &dd );

	return TRUE;
}

INT_PTR CEdUnitStartCmd::DoModal()
{
	AfxSetResourceHandle( theEDB2M1Instance );
	INT_PTR res = CResizeDialog::DoModal();
	AfxSetResourceHandle( AfxGetInstanceHandle() );
	return res;
}

void CEdUnitStartCmd::OnOK()
{
	if ( pCommandState )
		pCommandState->OnEdUnitStartCmdDialogEvent( CEdUnitStartCmd::EV_OK );
	ShowWindow( SW_HIDE );
}

void CEdUnitStartCmd::OnCancel()
{
	if ( pCommandState )
		pCommandState->OnEdUnitStartCmdDialogEvent( CEdUnitStartCmd::EV_CANCEL );
	ShowWindow( SW_HIDE );
}

void CEdUnitStartCmd::SetDialogData( const SDlgData *pData )
{
	if ( !pData )
		return;

	bEditMode = pData->bEditMode;
	nCommandIndex = pData->nCommandIndex;
	SetWindowText( bEditMode ? RCSTR("New command") : RCSTR("Edit command") );

	cbCmdTypes.ResetContent();
	CString strUnknownCommand = "<UNKNOWN>";
	CString strSelItem = strUnknownCommand;
	bool bFound = false;
	for ( int i = 0; i < cmdTypes.size(); ++i )
	{
		SUnitCommandTypeInfo &ct = cmdTypes[i];
		//
		int nItem = cbCmdTypes.AddString( ct.szName.c_str() );
		cbCmdTypes.SetItemData( nItem, ct.nValue );
		if ( ct.nValue == pData->nSelectedCmdType )
		{
			strSelItem = ct.szName.c_str();
			bFound = true;
		}
	}
	int nUnknItem = cbCmdTypes.AddString( strUnknownCommand );
	cbCmdTypes.SetItemData( nUnknItem, -1 );	
	//
	cbCmdTypes.SelectString( -1, strSelItem );
	edData.EnableWindow( bFound );
	btnClear.EnableWindow( bFound );
	GetDlgItem( IDOK )->EnableWindow( bFound );
	//
	edTgtUnit.SetWindowText( pData->szTarget.c_str() );
	if ( pData->szTarget.empty() )
	{
		btnClear.EnableWindow( FALSE );
	}

	edData.SetWindowText( StrFmt("%d", pData->nData) );
}

void CEdUnitStartCmd::GetDialogData( SDlgData *pData )
{
	if ( !pData )
		return;

	pData->Clear();

	int nSelItem = cbCmdTypes.GetCurSel();
	if ( nSelItem != -1 )
	{
		pData->nSelectedCmdType = cbCmdTypes.GetItemData( nSelItem );
	}
	else
	{
		pData->nSelectedCmdType = -1;
	}

	if ( nSelItem >= 0 && nSelItem < cmdTypes.size() )
	{
		pData->bSelectedCmdNeedTargetUnit = cmdTypes[nSelItem].nNeedTargetUnit;
	}
	else
	{
		pData->bSelectedCmdNeedTargetUnit = false;
	}

	CString szTmp;
	edData.GetWindowText( szTmp );
	pData->nData = atoi( szTmp );

	szTmp.Empty();
	edTgtUnit.GetWindowText( szTmp );
	pData->szTarget = (const char*)szTmp;

	pData->bEditMode = bEditMode;
	pData->nCommandIndex = nCommandIndex;
}

void CEdUnitStartCmd::OnBnClickedButtonClear()
{
	if ( pCommandState )
		pCommandState->OnEdUnitStartCmdDialogEvent( CEdUnitStartCmd::EV_CLEAR );
}

void CEdUnitStartCmd::OnCbnSelchangeComboCmdType()
{
	if ( pCommandState )
	{
		pCommandState->OnEdUnitStartCmdDialogEvent( CEdUnitStartCmd::EV_TYPE_CHANGE );
	}
	int nSelection = cbCmdTypes.GetCurSel();
	if ( nSelection != -1 )
	{
		int nItemData = cbCmdTypes.GetItemData( nSelection );
		if ( nItemData == -1 )
		{
			edData.EnableWindow( FALSE );
			btnClear.EnableWindow( FALSE );
			GetDlgItem( IDOK )->EnableWindow( FALSE );
		}
		else
		{
			edData.EnableWindow( TRUE );
			btnClear.EnableWindow( TRUE );
			GetDlgItem( IDOK )->EnableWindow( TRUE );
		}
	}
}

int CEdUnitStartCmd::GetSelectedCommandType()
{
	int nSel = cbCmdTypes.GetCurSel();
	if ( nSel == -1 )
		return 0;
	return cbCmdTypes.GetItemData( nSel );
}

void CEdUnitStartCmd::UpdateTarget( const string &szNewTarget )
{
	edTgtUnit.SetWindowText( szNewTarget.c_str() );
}

