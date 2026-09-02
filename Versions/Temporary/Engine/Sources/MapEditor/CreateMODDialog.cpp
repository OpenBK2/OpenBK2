#include "stdafx.h"

#include "CreateMODDialog.h"
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
		bool bEnable = ( !strFolder.IsEmpty() && !strName.IsEmpty() && ( strFolder.FindOneOf( "/\\" ) < 0 ) );
		if ( bEnable )
		{
			std::string szFolder = GetFolder();
			std::vector<NMOD::SMOD> modList;
			NMOD::GetAllMODs( &modList );
			for ( int nIndex = 0; nIndex < modList.size(); ++nIndex )
			{
				if ( CStringManager::Compare( modList[nIndex].szFullFolderPath, szFolder, true, true,false ) == 0 )
				{
					bEnable = false;
					break;
				}
			}
		}
		pwndButton->EnableWindow( bEnable );
	}
}


const std::string CCreateMODDialog::GetFolder()
{
	std::string szFolder;
	if ( SUserData *pUserData = Singleton<IUserDataContainer>()->Get() )
	{
		szFolder = pUserData->constUserData.szDataStorageFolder;
		if( !szFolder.empty() )
		{
			szFolder = szFolder.substr( 0, szFolder.size() - 1 );
		}
		int nPos = szFolder.rfind( '\\' );
		if( nPos > 0 )
		{
			szFolder = szFolder.substr( 0, nPos );
		}
		szFolder += std::string( "\\Mods\\" ) + std::string( strFolder ) + std::string( "\\" );
	}
	return szFolder;
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


