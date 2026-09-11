#include "stdafx.h"

#include "EnterNameDialog.h"

//
//
//		ENTER NAME DIALOG
//
//

BEGIN_MESSAGE_MAP(CEnterNameDialog,CDialog)
END_MESSAGE_MAP()

CEnterNameDialog::CEnterNameDialog( CWnd* pParentWindow, const std::string &_szDlgCaption,  const std::string &_szLabelText,
																		const std::string &rszInitialName )
	:	CResizeDialog( CEnterNameDialog::IDD, pParentWindow ),
	szName( rszInitialName ),
	szDlgCaption( _szDlgCaption ),
	szLabelText( _szLabelText )
{
}


void CEnterNameDialog::DoDataExchange( CDataExchange *pDX )
{
	CDialog::DoDataExchange( pDX );
}


BOOL CEnterNameDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText( szDlgCaption.c_str() );
  
	CEdit *pEdit = static_cast<CEdit*>( GetDlgItem( IDC_EDIT_NAME ) );
	if ( pEdit )
		pEdit->SetWindowText( szName.c_str() );

	CStatic *pLabel = static_cast<CStatic*>( GetDlgItem( IDC_STATIC_LABEL ) );
	if ( pLabel )
		pLabel->SetWindowText( szLabelText.c_str() );
	
	return TRUE;
}


void CEnterNameDialog::OnOK()
{
	CEdit *pEdit = static_cast<CEdit*>( GetDlgItem( IDC_EDIT_NAME ) );
	if ( pEdit )
	{
		CString szStr;
		pEdit->GetWindowText( szStr );
		szName = (LPCSTR)szStr;
		CDialog::OnOK();
	}
}


void CEnterNameDialog::OnCancel()
{
	szName.clear();
	CDialog::OnCancel();
}


void CEnterNameDialog::GetName( std::string *pName )
{
	(*pName) = szName;
}


INT_PTR CEnterNameDialog::DoModal()
{
	extern HINSTANCE theEDB2M1Instance;
	AfxSetResourceHandle( theEDB2M1Instance );
	INT_PTR res = CDialog::DoModal();
	AfxSetResourceHandle( AfxGetInstanceHandle() );
	return res;
}



