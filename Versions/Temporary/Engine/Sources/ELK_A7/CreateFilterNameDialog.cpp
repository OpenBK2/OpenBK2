#include "stdafx.h"

#include "CreateFilterNameDialog.h"

const int CCreateFilterNameDialog::vID[] = 
{
	IDC_CF_FILTER_NAME_LABEL,	//0
	IDC_CF_FILTER_NAME_EDIT,	//1
	IDOK,											//2
	IDCANCEL,									//3
};


CCreateFilterNameDialog::CCreateFilterNameDialog( CWnd* pParent )
	: CResizeDialog( CCreateFilterNameDialog::IDD, pParent )
{
	m_Name = _T( "" );
	SetControlStyle( IDC_CF_FILTER_NAME_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_CF_FILTER_NAME_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR );

	SetControlStyle( IDOK, ANCHORE_HOR_CENTER | ANCHORE_BOTTOM );
	SetControlStyle( IDCANCEL, ANCHORE_HOR_CENTER | ANCHORE_BOTTOM );
}


void CCreateFilterNameDialog::GetRegistryKey( string *pszRegistryKey )
{
	if ( pszRegistryKey )
	{
		CString strPath;
		CString strProgramKey;
		CString strKey;
		strPath.LoadString( IDS_REGISTRY_PATH );
		strProgramKey.LoadString( AFX_IDS_APP_TITLE );
		strKey.LoadString( IDS_CFN_REGISTRY_KEY );
		( *pszRegistryKey ) = StrFmt( _T( "Software\\%s\\%s\\%s" ), LPCTSTR( strPath ), LPCTSTR( strProgramKey ), LPCTSTR( strKey ) );
	}
}


void CCreateFilterNameDialog::DoDataExchange(CDataExchange* pDX)
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Text(pDX, IDC_CF_FILTER_NAME_EDIT, m_Name);
}


BEGIN_MESSAGE_MAP(CCreateFilterNameDialog, CResizeDialog)
	ON_EN_CHANGE(IDC_CF_FILTER_NAME_EDIT, OnChangeFilterNameEdit)
END_MESSAGE_MAP()


void CCreateFilterNameDialog::OnChangeFilterNameEdit() 
{
	UpdateControls();
}


BOOL CCreateFilterNameDialog::OnInitDialog() 
{
	CResizeDialog::OnInitDialog();

	UpdateControls();
	return TRUE;
}


void CCreateFilterNameDialog::UpdateControls()
{
	if ( CWnd *pWnd = GetDlgItem( IDC_CF_FILTER_NAME_EDIT ) )
	{
		CString strText;
		pWnd->GetWindowText( strText );
		
		if ( pWnd = GetDlgItem( IDOK ) )
		{
			pWnd->EnableWindow( !strText.IsEmpty() );
		}
	}
}

// basement storage  



