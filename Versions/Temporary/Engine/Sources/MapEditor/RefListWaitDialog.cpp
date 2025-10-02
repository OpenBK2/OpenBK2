#include "stdafx.h"

#include "RefListWaitDialog.h"
#include "libdb/ResourceManager.h"

BEGIN_MESSAGE_MAP(CRefListWaitDialog, CResizeDialog)
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CRefListWaitDialog::CRefListWaitDialog( CWnd* pParent )
: CResizeDialog( CRefListWaitDialog::IDD, pParent ), bComplete( false ) 
{
	SetControlStyle( IDOK, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER );
}

void CRefListWaitDialog::OnTimer( UINT nIDEvent ) 
{
	if ( !bComplete )
	{
		pResourceManager->FillReferencingObjects( &bComplete, szTargetTypeName, szTargetName, *pResultList );
		if ( bComplete )
		{
			KillTimer( nTimer );
			CResizeDialog::OnOK();
		}
	}
}

BOOL CRefListWaitDialog::OnInitDialog()
{
	CResizeDialog::OnInitDialog();
	pResourceManager->FillReferencingObjects( &bComplete, szTargetTypeName, szTargetName, *pResultList );
	if ( bComplete )
		CResizeDialog::OnOK();
	else
		nTimer = SetTimer( 1, 300, 0 );
	return true;
}

void CRefListWaitDialog::OnOK()
{
	CResizeDialog::OnOK();
}

void CRefListWaitDialog::OnCancel()
{
	CResizeDialog::OnCancel();
}

void CRefListWaitDialog::OnDestroy() 
{
	SaveResizeDialogOptions();
	CResizeDialog::OnDestroy();
}


