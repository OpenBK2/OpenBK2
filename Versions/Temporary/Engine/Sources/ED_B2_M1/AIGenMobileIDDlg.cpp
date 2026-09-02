#include "stdafx.h"
#include <fmt/format.h>
#include "AIGenMobileIDDlg.h"

//
//
//		CAIGenMobileDlg dialog
//
//

IMPLEMENT_DYNAMIC( CAIGenMobileDlg, CResizeDialog )

CAIGenMobileDlg::CAIGenMobileDlg( CWnd *pParentWindow, int *_pMobileID )
	: CResizeDialog( CAIGenMobileDlg::IDD, pParentWindow ),
	pMobileID( _pMobileID )
{
}

void CAIGenMobileDlg::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_AIGEN_EDIT_MOBILE_ID, edMobileID );
}

BOOL CAIGenMobileDlg::OnInitDialog()
{
	CResizeDialog::OnInitDialog();

	edMobileID.SetWindowText( fmt::format("{}", *pMobileID) );

	return true;
}

void CAIGenMobileDlg::OnOK()
{
	CString szMobileID;
	edMobileID.GetWindowText( szMobileID );
	int nMobileID = 0;
	sscanf( szMobileID, "%d", &nMobileID );
	(*pMobileID) = nMobileID;

	CResizeDialog::OnOK();
}


