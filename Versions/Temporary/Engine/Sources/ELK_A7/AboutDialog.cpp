#include "StdAfx.h"

#include "AboutDialog.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CAboutDialog::CAboutDialog( CWnd* pParent)
	: CDialog( CAboutDialog::IDD, pParent )
{
}

void CAboutDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAboutDialog, CDialog)
END_MESSAGE_MAP()


BOOL CAboutDialog::OnInitDialog()
{
   CDialog::OnInitDialog();

  //Set dialog title
  CString strProgramTitle;
  strProgramTitle.LoadString( AFX_IDS_APP_TITLE );

  CString strDialogTitle;
  strDialogTitle.Format( _T( "About %s" ), LPCTSTR( strProgramTitle ) );
  SetWindowText( strDialogTitle );

  //Set program version
  CString strProgramVersion;
  strProgramVersion.LoadString( IDS_PROGRAM_VERSION );
  SetDlgItemText( IDC_ABOUT_VERSION, strProgramVersion );

  //Set program title
  SetDlgItemText( IDC_ABOUT_PROGRAM_TITLE, strProgramTitle );
  return true;
}


