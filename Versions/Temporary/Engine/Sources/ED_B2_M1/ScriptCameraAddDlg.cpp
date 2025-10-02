#include "StdAfx.h"

#include "ScriptCameraAddDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CScriptCameraAddDlg::CScriptCameraAddDlg( CWnd *pParentWindow, string *_pszNewCameraName, const vector<NCamera::CCameraPlacement> &rvExistingCameras )
	: CResizeDialog( CScriptCameraAddDlg::IDD, pParentWindow ),
	pszNewCameraName( _pszNewCameraName ),
	vExistingCameras( rvExistingCameras )
{
}


BOOL CScriptCameraAddDlg::OnInitDialog()
{
	if ( CWnd *pWnd = GetDlgItem(IDOK) )
	{
		pWnd->EnableWindow( 0 );
	}

	return TRUE;
}

void CScriptCameraAddDlg::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
}


BEGIN_MESSAGE_MAP(CScriptCameraAddDlg, CResizeDialog)
	ON_EN_CHANGE(IDC_SCAD_EDIT_NAME, OnEnChangeScriptCameraNameEdit)
END_MESSAGE_MAP()


void CScriptCameraAddDlg::OnEnChangeScriptCameraNameEdit()
{
	if ( CWnd *pWnd = GetDlgItem(IDOK) )
	{
		pWnd->EnableWindow( 1 );
	}

	CString szNewCameraInput;
	GetDlgItemText( IDC_SCAD_EDIT_NAME, szNewCameraInput );
	for ( vector<NCamera::CCameraPlacement>::const_iterator it = vExistingCameras.begin(); it < vExistingCameras.end(); ++it )
	{
		if ( szNewCameraInput == it->szName.c_str() )
		{
			if ( CWnd *pWnd = GetDlgItem(IDOK) )
			{
				pWnd->EnableWindow( 0 );
			}
			break;
		}
	}
	(*pszNewCameraName) = szNewCameraInput;
}


