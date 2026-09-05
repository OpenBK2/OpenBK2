#include "stdafx.h"

#include "libdb/Manipulator.h"
#include "SearchObjectDialog.h"
#include "MapEditorLib/ShellFont.h"

CSearchObjectDialog::CSearchObjectDialog( CWnd* pParent ) : CDialog( CSearchObjectDialog::IDD, pParent )
{
}


void CSearchObjectDialog::DoDataExchange( CDataExchange* pDX )
{
	CDialog::DoDataExchange( pDX );
	DDX_Text( pDX, IDC_SO_TEXT_EDIT, strText );
}


BEGIN_MESSAGE_MAP(CSearchObjectDialog, CDialog)
END_MESSAGE_MAP()


BOOL CSearchObjectDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	// Not a CResizeDialog, so it does not get the shell font from there.
	NEditorFont::ApplyShellFont( this );
	GotoDlgCtrl( GetDlgItem( IDC_SO_TEXT_EDIT ) );
	return true;
}


void CSearchObjectDialog::SetText( const std::string &rszText )
{
	strText = rszText.c_str();
}


std::string CSearchObjectDialog::GetText()
{
	return std::string( strText );
}

// basement storage  


