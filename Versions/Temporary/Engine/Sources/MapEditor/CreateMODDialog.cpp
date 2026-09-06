#include "stdafx.h"

#include "CreateMODDialog.h"
#include "CreateModView.h"
#include "Main/MODs.h"
#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/Interface_UserData.h"

CCreateMODDialog::CCreateMODDialog( CWnd* pParent )
	: CResizeDialog( CCreateMODDialog::IDD, pParent )
{
	SetControlStyle( IDC_CM_FOLDER_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_CM_FOLDER_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_CM_NAME_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_CM_NAME_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_CM_DESC_LABEL, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_CM_DESC_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR_VER );
	SetControlStyle( IDOK, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDCANCEL, ANCHORE_RIGHT_BOTTOM );
}


void CCreateMODDialog::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Text( pDX, IDC_CM_FOLDER_EDIT, strFolder );
	DDX_Text( pDX, IDC_CM_NAME_EDIT, strName );
	DDX_Text( pDX, IDC_CM_DESC_EDIT, strDescription );
}


BEGIN_MESSAGE_MAP(CCreateMODDialog, CResizeDialog)
	ON_EN_CHANGE(IDC_CM_FOLDER_EDIT, OnEnChangeFolderEdit)
	ON_EN_CHANGE(IDC_CM_NAME_EDIT, OnEnChangeNameEdit)
END_MESSAGE_MAP()


BOOL CCreateMODDialog::OnInitDialog() 
{
	CResizeDialog::OnInitDialog();
	UpdateControls();
	return true;
}


void CCreateMODDialog::UpdateControls()
{
	if ( CWnd *pwndButton = GetDlgItem( IDOK ) )
	{
		// The rule -- a folder name that is one component and not already taken,
		// plus a title -- is in CreateModView.h now, because the wx dialog has to
		// apply the same one and two copies of it would drift.
		const bool bEnable = NCreateMod::IsFolderNameFree( std::string( strFolder ) ) &&
												 !strName.IsEmpty();
		pwndButton->EnableWindow( bEnable );
	}
}


const std::string CCreateMODDialog::GetFolder()
{
	// Shared with the wx dialog; see CreateModView.h.
	return NCreateMod::MakeFolderPath( std::string( strFolder ) );
}


void CCreateMODDialog::OnEnChangeFolderEdit()
{
	UpdateData( true );
	UpdateControls();
}


void CCreateMODDialog::OnEnChangeNameEdit()
{
	UpdateData( true );
	UpdateControls();
}

// basement storage  


