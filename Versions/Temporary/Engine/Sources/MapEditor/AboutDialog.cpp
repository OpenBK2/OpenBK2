#include "StdAfx.h"

#include "AboutDialog.h"
#include "../MapEditorLib/Interface_UserData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CAboutDialog::CAboutDialog( CWnd* pParent)
	: CDialog( CAboutDialog::IDD, pParent ) {}


BEGIN_MESSAGE_MAP(CAboutDialog, CDialog)
END_MESSAGE_MAP()


BOOL CAboutDialog::OnInitDialog()
{
   CDialog::OnInitDialog();

  //Set dialog title
  CString strDialogTitle;
  strDialogTitle.Format( "About %s", Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str() );
  SetWindowText( strDialogTitle );

  //Set program version
  SetDlgItemText( IDC_ABOUT_VERSION_LABEL_RIGHT, Singleton<IUserDataContainer>()->Get()->constUserData.szVersion.c_str() );

  //Set program title
  SetDlgItemText( IDC_ABOUT_PROGRAM_TITLE_LABEL, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str() );
  return true;
}

// basement storage  


