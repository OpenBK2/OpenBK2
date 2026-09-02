#include "stdafx.h"
#include <fmt/format.h>

#include "MovEditorSettingsWindow.h"

CMovEditorSettingsDlg::CMovEditorSettingsDlg( CWnd *pParentWindow, float *_pLength )
	: CResizeDialog( CMovEditorSettingsDlg::IDD, pParentWindow ),
	pLength( _pLength )
{
	NI_ASSERT( pParentWindow, "CMovEditorSettingsDlg : parent window is NULL!" );
}


void CMovEditorSettingsDlg::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
}


void CMovEditorSettingsDlg::OnOK()
{
	CString szLen;
	GetDlgItemText( IDC_MOVEDS_LEN_EDIT, szLen );
	float fLength = 0.0f;
	sscanf( szLen, "%g", &fLength );

	(*pLength) = fLength;

	CResizeDialog::OnOK();
}


BOOL CMovEditorSettingsDlg::OnInitDialog()
{
	SetDlgItemText( IDC_MOVEDS_LEN_EDIT, fmt::format("{:g}", (*pLength)).c_str() );
	return true;
}



