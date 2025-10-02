#include "stdafx.h"

#include "OpenMODDialog.h"
#include "../MapEditorLib/EditParameter.h"
#include "../MapEditorLib/Tools_Resources.h"
#include "../MapEditorLib/StringManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


COpenMODDialog::COpenMODDialog( CWnd* pParent )
	: CResizeDialog( COpenMODDialog::IDD, pParent ), bCreateControls( true ), nMODIndex( -1 )
{
	SetControlStyle( IDC_OM_NAME_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_OM_NAME_COMBO, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_OM_DESC_LABEL, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_OM_DESC_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR_VER );
	SetControlStyle( IDOK, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDCANCEL, ANCHORE_RIGHT_BOTTOM );
}


void COpenMODDialog::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_OM_NAME_COMBO, wndNameComboBox );
	DDX_Text( pDX, IDC_OM_DESC_EDIT, strDescription );
}


BEGIN_MESSAGE_MAP(COpenMODDialog, CResizeDialog)
	ON_CBN_SELCHANGE(IDC_OM_NAME_COMBO, OnCbnSelchangeNameCombo)
END_MESSAGE_MAP()


BOOL COpenMODDialog::OnInitDialog() 
{
	CResizeDialog::OnInitDialog();
	bCreateControls = true;
	modList.clear();

	if ( resizeDialogOptions.nParameters.size() < 1 )
	{
		resizeDialogOptions.nParameters.push_back( 0 );
	}
	nMODIndex = resizeDialogOptions.nParameters[0];
	NMOD::GetAllMODs( &modList );
	vector<string> nameList;
	for ( int nIndex = 0; nIndex < modList.size(); ++nIndex )
	{
		CString strName;
		Unicode2MBSC( &strName, modList[nIndex].wszName, ::GetACP() );
		nameList.push_back( string( strName ) );
	}
	SetComboBoxEditParameters( nameList, nMODIndex, &wndNameComboBox, true, true );
	UpdateControls();
	bCreateControls = false;
	return true;
}


bool COpenMODDialog::GetMOD( NMOD::SMOD *pMOD )
{
	if ( pMOD )
	{
		if ( ( nMODIndex >= 0 ) && ( nMODIndex < modList.size() ) )
		{
			( *pMOD ) = modList[nMODIndex];
			return true;
		}
	}
	return false;
}


void COpenMODDialog::UpdateControls()
{
	bool bEnable = ( ( nMODIndex >= 0 ) && ( nMODIndex < modList.size() ) );
	NMOD::SMOD mod;
	if ( GetMOD( &mod ) )
	{
		CString strDescription;
		Unicode2MBSC( &strDescription, mod.wszDesc, ::GetACP() );
		//
		string szDescription( strDescription );
		szDescription.erase( remove( szDescription.begin(), szDescription.end(), 0x0D ), szDescription.end() );
		for ( int nIndex = 0; nIndex < szDescription.size(); ++nIndex )
		{
			if ( szDescription[nIndex] == 0x0A )
			{
				if ( ( nIndex == 0 ) || ( szDescription[nIndex - 1] != 0x0D ) )
				{
					szDescription.insert( szDescription.begin() + nIndex, 0x0D );
				}
			}
		}
		//
		SetDlgItemText( IDC_OM_DESC_EDIT, szDescription.c_str() );
		if ( bEnable )
		{
			NMOD::SMOD attachedMOD;
			if ( NMOD::GetAttachedMOD( &attachedMOD ) )
			{
				bEnable = ( CStringManager::Compare( attachedMOD.szFullFolderPath, mod.szFullFolderPath, true, true, false ) != 0 );
			}
		}
	}
	else
	{
		SetDlgItemText( IDC_OM_DESC_EDIT, "" );
	}
	if ( CWnd *pwndButton = GetDlgItem( IDOK ) )
	{
		pwndButton->EnableWindow( bEnable );
	}
}


void COpenMODDialog::OnCbnSelchangeNameCombo()
{
	if ( !bCreateControls )
	{
		UpdateData( true );
		vector<string> nameList;
		GetComboBoxEditParameters( &nameList, &nMODIndex, wndNameComboBox, false, true );
		resizeDialogOptions.nParameters[0] = nMODIndex;
		UpdateControls();
	}
}

// basement storage  


