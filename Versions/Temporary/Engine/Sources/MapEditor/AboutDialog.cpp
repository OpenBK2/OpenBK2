#include "stdafx.h"

#include "AboutDialog.h"
#include "MapEditorLib/ShellFont.h"
#include "MapEditorLib/Interface_UserData.h"

CAboutDialog::CAboutDialog( CWnd* pParent)
	: CDialog( CAboutDialog::IDD, pParent ) {}


BEGIN_MESSAGE_MAP(CAboutDialog, CDialog)
END_MESSAGE_MAP()


BOOL CAboutDialog::OnInitDialog()
{
   CDialog::OnInitDialog();
   // Not a CResizeDialog, so it does not get the shell font from there.
   NEditorFont::ApplyShellFont( this );

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


