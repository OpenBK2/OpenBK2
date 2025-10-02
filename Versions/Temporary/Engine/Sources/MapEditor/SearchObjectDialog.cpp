#include "stdafx.h"

#include "../libdb/Manipulator.h"
#include "SearchObjectDialog.h"

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
	GotoDlgCtrl( GetDlgItem( IDC_SO_TEXT_EDIT ) );
	return true;
}


void CSearchObjectDialog::SetText( const string &rszText )
{
	strText = rszText.c_str();
}


string CSearchObjectDialog::GetText()
{
	return string( strText );
}

// basement storage  


