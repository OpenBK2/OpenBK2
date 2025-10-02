#include "stdafx.h"

#include "KeySettingsDlg.h"

CMovEditorKeySettingsDlg::CMovEditorKeySettingsDlg( CWnd *pParentWindow, NDb::SScriptMovieKeyPos *_pKey, 
																										string *_pszName )
	: CResizeDialog( CMovEditorKeySettingsDlg::IDD, pParentWindow ),
	pKey( _pKey ),
	pszName( _pszName )
{
	NI_ASSERT( pParentWindow, "CMovEditorKeySettingsDlg : parent window is NULL!" )
	NI_ASSERT( pKey, "Cannot edit NULL key settings!" )
}


void CMovEditorKeySettingsDlg::DoDataExchange( CDataExchange *pDX )
{
	CResizeDialog::DoDataExchange( pDX );

	DDX_Control( pDX, IDC_MOVEDKEY_NAME, wndEditName );
	DDX_Control( pDX, IDC_MOVEDKEY_RADIO_IN_FLAT, rbnInFlat );
	DDX_Control( pDX, IDC_MOVEDKEY_RADIO_IN_TANG, rbnInTangent );
	DDX_Control( pDX, IDC_MOVEDKEY_RADIO_OUT_FLAT, rbnOutFlat );
	DDX_Control( pDX, IDC_MOVEDKEY_RADIO_OUT_TANG, rbnOutTangent );
	DDX_Control( pDX, IDC_MOVEDKEY_PARAM_EDIT, wndEditParam );
}


void CMovEditorKeySettingsDlg::OnOK()
{
	GetDialogData();

	CResizeDialog::OnOK();
}


BOOL CMovEditorKeySettingsDlg::OnInitDialog()
{
	CResizeDialog::OnInitDialog();

	SetDialogData();
	return true;
}


void CMovEditorKeySettingsDlg::GetDialogData()
{
	pKey->bIsTangentIn = rbnInTangent.GetCheck();
	pKey->bIsTangentOut = rbnOutTangent.GetCheck();

	CString szParamTxt;
	GetDlgItemText( IDC_MOVEDKEY_PARAM_EDIT, szParamTxt );

	pKey->szKeyParam = szParamTxt;
}


void CMovEditorKeySettingsDlg::SetDialogData()
{
	rbnInFlat.SetCheck( !pKey->bIsTangentIn );
	rbnInTangent.SetCheck( pKey->bIsTangentIn );
	rbnOutFlat.SetCheck( !pKey->bIsTangentOut );
	rbnOutTangent.SetCheck( pKey->bIsTangentOut );
	SetDlgItemText( IDC_MOVEDKEY_NAME, pszName->c_str() );
	SetDlgItemText( IDC_MOVEDKEY_PARAM_EDIT, pKey->szKeyParam.c_str() );
}



