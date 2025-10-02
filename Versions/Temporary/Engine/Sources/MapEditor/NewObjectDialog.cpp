#include "stdafx.h"

#include "../libdb/Manipulator.h"
#include "../Misc/StrProc.h"
#include "NewObjectDialog.h"
#include "../MapEditorLib/Interface_FolderCallback.h"
#include "../MapEditorLib/StringManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CNewObjectDialog::CNewObjectDialog( CWnd* pParent )
	: CResizeDialog( CNewObjectDialog::IDD, pParent ),
		bCreateControls( false ),
		bEnableType( true ),
		nNeedExport( 0 ),
		nObjectTypeNameIndex( -1 ),
		pBuildDataParams( 0 )
{
	SetControlStyle( IDC_NO_TYPE_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_NO_TYPE_COMBO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_NO_NAME_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_NO_NAME_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_NO_NEED_EXPORT_CHEKBOX, ANCHORE_LEFT_BOTTOM | RESIZE_HOR );
	SetControlStyle( IDC_NO_ADD_TYPE_CHECKBOX, ANCHORE_RIGHT_TOP );
	SetControlStyle( IDOK, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDCANCEL, ANCHORE_RIGHT_BOTTOM );
}


void CNewObjectDialog::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Text( pDX, IDC_NO_NAME_EDIT, strName );
	DDX_Check( pDX, IDC_NO_NEED_EXPORT_CHEKBOX, nNeedExport );
	DDX_Control( pDX, IDC_NO_TYPE_COMBO, wndObjectTypeComboBox );
}


BEGIN_MESSAGE_MAP(CNewObjectDialog, CResizeDialog)
	ON_EN_CHANGE(IDC_NO_NAME_EDIT, OnNameEditChange)
	ON_CBN_SELCHANGE(IDC_NO_TYPE_COMBO, OnSelchangeObjectTypeComboBox)
	ON_BN_CLICKED( IDC_NO_NEED_EXPORT_CHEKBOX, OnExportCheckBox )
	ON_BN_CLICKED( IDC_NO_ADD_TYPE_CHECKBOX, OnAddTypeCheckBox )
END_MESSAGE_MAP()


void CNewObjectDialog::UpdateOKButton()
{
	bCreateControls = true;
	if ( pBuildDataParams != 0 )
	{
		if ( CWnd *pwndOKButton = GetDlgItem( IDOK ) )
		{
			UpdateData( true );
			//
			szObjectTypeNamePostrfix = string( "_" ) + pBuildDataParams->szObjectTypeName;
			string szObjectNameToCompare = pBuildDataParams->szObjectName;
			string szObjectTypeNamePostrfixToCompare = szObjectTypeNamePostrfix;
			NStr::ToLower( &szObjectNameToCompare );
			NStr::ToLower( &szObjectTypeNamePostrfixToCompare );
			//
			string szObjectName;
			pBuildDataParams->GetObjectName( &szObjectName );
			//
			pwndOKButton->EnableWindow( ( !strName.IsEmpty() ) &&
																	( szObjectNameToCompare != szObjectTypeNamePostrfixToCompare ) &&
																	( Singleton<IFolderCallback>()->IsUniqueName( pBuildDataParams->szObjectTypeName, szObjectName ) ) );
		}
	}
	else
	{
		if ( CWnd *pwndOKButton = GetDlgItem( IDOK ) )
		{
			pwndOKButton->EnableWindow( false );
		}
	}
	bCreateControls = false;
}


void CNewObjectDialog::UpdateTitle()
{
	bCreateControls = true;
	if ( pBuildDataParams != 0 )
	{
		CString strFormatString;
		strFormatString.LoadString( IDS_PC_BD_DIALOG_TITLE );
		SetWindowText( StrFmt( strFormatString, pBuildDataParams->szObjectTypeName.c_str() ) );
	}
	bCreateControls = false;
}


void CNewObjectDialog::CreateTypeList()
{
	bCreateControls = true;
	if ( ( nObjectTypeNameIndex >= 0 ) && ( nObjectTypeNameIndex < objectTypeNameList.size() ) )
	{
		wndObjectTypeComboBox.ResetContent();
		int nLocalObjectTypeNameIndex = 0;
		for ( vector<string>::iterator itObjectTypeName = objectTypeNameList.begin(); itObjectTypeName != objectTypeNameList.end(); ++itObjectTypeName )
		{
			const int nStringIndex = wndObjectTypeComboBox.AddString( itObjectTypeName->c_str() );
			if ( nStringIndex != ( -1 ) )
			{
				wndObjectTypeComboBox.SetItemData( nStringIndex, nLocalObjectTypeNameIndex );  
				if (  nLocalObjectTypeNameIndex == nObjectTypeNameIndex )
				{
				}
			}
			++nLocalObjectTypeNameIndex;
		}
		
		{
			wndObjectTypeComboBox.SelectString( -1, objectTypeNameList[nObjectTypeNameIndex].c_str() );
		}
		wndObjectTypeComboBox.EnableWindow( nLocalObjectTypeNameIndex > 1 );
	}
	bCreateControls = false;
}


BOOL CNewObjectDialog::OnInitDialog()
{
	CResizeDialog::OnInitDialog();
	//	
	resizeDialogOptions.nParameters.resize( 1, 0 );
	bCreateControls = true;
	CheckDlgButton( IDC_NO_ADD_TYPE_CHECKBOX, ( ( resizeDialogOptions.nParameters[0] > 0 ) && bEnableType ) );
	bCreateControls = false;
	UpdateTypePostfix();
	//
	if ( CWnd *pWnd = GetDlgItem( IDC_NO_ADD_TYPE_CHECKBOX ) )
	{
		pWnd->EnableWindow( bEnableType );
	}
	CreateTypeList();
	UpdateOKButton();
	UpdateTitle();
	return true;
}


void CNewObjectDialog::SetBuildDataParams( const vector<string> &rObjectTypeNameList, int _nObjectTypeNameIndex, SBuildDataParams *_pBuildDataParams )
{
	objectTypeNameList = rObjectTypeNameList;
	nObjectTypeNameIndex = _nObjectTypeNameIndex;
	pBuildDataParams = _pBuildDataParams;
	if ( pBuildDataParams != 0 )
	{
		strName = pBuildDataParams->szObjectName.c_str();
		nNeedExport = pBuildDataParams->bNeedExport ? 1 : 0;
		if ( ( nObjectTypeNameIndex >= 0 ) && ( nObjectTypeNameIndex < objectTypeNameList.size() ) )
		{
			pBuildDataParams->szObjectTypeName = objectTypeNameList[nObjectTypeNameIndex];
		}
	}
	pBuildDataParams->szObjectName.clear();
	//
	string szObjectNameExtentionToCompare = pBuildDataParams->szObjectNameExtention;
	NStr::ToLower( &szObjectNameExtentionToCompare );
	bEnableType = ( szObjectNameExtentionToCompare == ".xdb" );
}


void CNewObjectDialog::UpdateTypePostfix()
{
	szObjectTypeNamePostrfix = string( "_" ) + pBuildDataParams->szObjectTypeName;
	const int nObjectTypeNameSize = szObjectTypeNamePostrfix.size();
	const int nObjectNameSize = pBuildDataParams->szObjectName.size();
	bool bPostfixExists = false;
	if ( nObjectTypeNameSize <= nObjectNameSize )
	{
		string szObjectNameToCompare = pBuildDataParams->szObjectName;
		string szObjectTypeNamePostrfixToCompare = szObjectTypeNamePostrfix;
		NStr::ToLower( &szObjectNameToCompare );
		NStr::ToLower( &szObjectTypeNamePostrfixToCompare );
		bPostfixExists = ( szObjectNameToCompare.compare( nObjectNameSize - nObjectTypeNameSize, nObjectTypeNameSize, szObjectTypeNamePostrfixToCompare ) == 0 );
	}
	if ( ( ( resizeDialogOptions.nParameters[0] > 0 ) && bEnableType ) && !bPostfixExists )
	{
		pBuildDataParams->szObjectName += szObjectTypeNamePostrfix;
	}
	else if ( ( ( resizeDialogOptions.nParameters[0] == 0 ) || !bEnableType ) && bPostfixExists )
	{
		pBuildDataParams->szObjectName = pBuildDataParams->szObjectName.substr( 0, nObjectNameSize - nObjectTypeNameSize );
	}
	bCreateControls = true;
	strName = pBuildDataParams->szObjectName.c_str();
	UpdateData( false );
	bCreateControls = false;
}


void CNewObjectDialog::OnNameEditChange()
{
	if ( !bCreateControls )
	{
		if ( pBuildDataParams != 0 )
		{
			UpdateData( true );
			pBuildDataParams->szObjectName = strName;
			UpdateOKButton();
		}
	}
}


void CNewObjectDialog::OnSelchangeObjectTypeComboBox()
{
	if ( !bCreateControls )
	{
		if ( pBuildDataParams != 0 )
		{
			UpdateData( true );
			const int nParam = resizeDialogOptions.nParameters[0];
			resizeDialogOptions.nParameters[0] = 0;
			UpdateTypePostfix();
			resizeDialogOptions.nParameters[0] = nParam;
			const int nStringIndex = wndObjectTypeComboBox.GetCurSel();
			if ( nStringIndex >= 0 )
			{
				nObjectTypeNameIndex = wndObjectTypeComboBox.GetItemData( nStringIndex );
				if ( ( nObjectTypeNameIndex >= 0 ) && ( nObjectTypeNameIndex < objectTypeNameList.size() ) )
				{
					pBuildDataParams->szObjectTypeName = objectTypeNameList[nObjectTypeNameIndex];
				}
			}
			UpdateTypePostfix();
			UpdateOKButton();
			UpdateTitle();
		}
	}
}


void CNewObjectDialog::OnExportCheckBox()
{
	if ( !bCreateControls )
	{
		if ( pBuildDataParams != 0 )
		{
			UpdateData( true );
			pBuildDataParams->bNeedExport = ( nNeedExport > 0 );
		}
	}
}


void CNewObjectDialog::OnAddTypeCheckBox()
{
	if ( !bCreateControls )
	{
		resizeDialogOptions.nParameters[0] = IsDlgButtonChecked( IDC_NO_ADD_TYPE_CHECKBOX );
		UpdateTypePostfix();
		UpdateOKButton();
	}
}


// basement storage  


