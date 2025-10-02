#include "StdAfx.h"

#include "resource.h"
#include "ImportFromPAKDialog.h"
#include "MapEditorLib/StringManager.h"

const int CImportFromPAKDialog::vID[] = 
{
	IDC_IFP_PAK_BROWSE_LABEL,		//0
	IDC_IFP_PAK_BROWSE_EDIT,		//1
	IDC_IFP_PAK_BROWSE_BUTTON,	//2
	IDC_IFP_FILE_BROWSE_LABEL,	//3
	IDC_IFP_FILE_BROWSE_EDIT,		//4
	IDC_IFP_FILE_BROWSE_BUTTON,	//5
	IDOK,												//6
	IDCANCEL,										//7
};


CImportFromPAKDialog::CImportFromPAKDialog( CWnd* pParent )
	: CResizeDialog( CImportFromPAKDialog::IDD, pParent )
{
	SetControlStyle( IDC_IFP_PAK_BROWSE_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_IFP_PAK_BROWSE_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_IFP_PAK_BROWSE_BUTTON, ANCHORE_RIGHT_TOP );
	
	SetControlStyle( IDC_IFP_FILE_BROWSE_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_IFP_FILE_BROWSE_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_IFP_FILE_BROWSE_BUTTON, ANCHORE_RIGHT_TOP );

	SetControlStyle( IDOK, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER );
	SetControlStyle( IDCANCEL, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER );
}


void CImportFromPAKDialog::GetRegistryKey( string *pszRegistryKey )
{
	if ( pszRegistryKey )
	{
		CString strPath;
		CString strProgramKey;
		CString strKey;
		strPath.LoadString( IDS_REGISTRY_PATH );
		strProgramKey.LoadString( AFX_IDS_APP_TITLE );
		strKey.LoadString( IDS_IFP_REGISTRY_KEY );
		( *pszRegistryKey ) = StrFmt( _T( "Software\\%s\\%s\\%s" ), LPCTSTR( strPath ), LPCTSTR( strProgramKey ), LPCTSTR( strKey ) );
	}
}


void CImportFromPAKDialog::DoDataExchange(CDataExchange* pDX)
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control(pDX, IDC_IFP_PAK_BROWSE_EDIT, m_PAKEdit);
	DDX_Control(pDX, IDC_IFP_FILE_BROWSE_EDIT, m_FileEdit);
}


BEGIN_MESSAGE_MAP(CImportFromPAKDialog, CResizeDialog)
	ON_BN_CLICKED(IDC_IFP_PAK_BROWSE_BUTTON, OnPAKBrowseButton)
	ON_EN_CHANGE(IDC_IFP_PAK_BROWSE_EDIT, OnChangePAKBrowseEdit)
	ON_BN_CLICKED(IDC_IFP_FILE_BROWSE_BUTTON, OnFileBrowseButton)
	ON_EN_CHANGE(IDC_IFP_FILE_BROWSE_EDIT, OnChangeFileBrowseEdit)
END_MESSAGE_MAP()


BOOL CImportFromPAKDialog::OnInitDialog() 
{
	CResizeDialog::OnInitDialog();
	
	//0 PAK
	//1 file
	if ( resizeDialogOptions.szParameters.size() < 2 )
	{
		resizeDialogOptions.szParameters.resize( 2 );
	}

	m_PAKEdit.SetWindowText( resizeDialogOptions.szParameters[0].c_str() );
	m_FileEdit.SetWindowText( resizeDialogOptions.szParameters[1].c_str() );
	
	UpdateControls();
	return TRUE;
}


void CImportFromPAKDialog::GetPAKPath( string *pszPAKPath )
{
	NI_ASSERT( pszPAKPath != 0, StrFmt( _T( "CImportFromPAKDialog::GetPAKPath wrong parameter: pszPAKPath %x" ), pszPAKPath ) );
	if ( pszPAKPath )
	{
		( *pszPAKPath ) = resizeDialogOptions.szParameters[0];
	}
}


void CImportFromPAKDialog::GetFilePath( string *pszFilePath )
{
	NI_ASSERT( pszFilePath != 0, StrFmt( _T( "CImportFromPAKDialog::GetFilePath wrong parameter: pszFilePath %x" ), pszFilePath ) );
	if ( pszFilePath )
	{
		( *pszFilePath ) = resizeDialogOptions.szParameters[1];
	}
}


void CImportFromPAKDialog::OnPAKBrowseButton() 
{
	CString strDialogTitle;
	CString strFileName;
	CString strFolderName;

	strDialogTitle.LoadString( IDS_IFP_BROWSE_FOR_PAK_DIALOG_TITLE );

	m_PAKEdit.GetWindowText( strFileName );
	int nSlashPos = strFileName.ReverseFind( '\\' );
	if ( nSlashPos >= 0 )
	{
		strFolderName = strFileName.Left( nSlashPos );
	}
	
	CFileDialog fileDialog( true, _T( ".pak" ), strFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T( "All supported Files (*.pak; *.upd)|*.pak; *.upd|PAK files (*.pak)|*.pak|UPD files (*.upd)|*.upd|All Files (*.*)|*.*||" ), this );
	fileDialog.m_ofn.lpstrTitle = strDialogTitle;
	fileDialog.m_ofn.lpstrInitialDir = strFolderName;
	
	if ( fileDialog.DoModal() == IDOK )
	{
		strFileName = fileDialog.GetPathName();
		CStringManager::ExtendFileExtention( &strFileName, _T( ".pak" ) );
		resizeDialogOptions.szParameters[0] = strFileName;
		m_PAKEdit.SetWindowText( resizeDialogOptions.szParameters[0].c_str() );

		UpdateControls();
	}
}


void CImportFromPAKDialog::OnFileBrowseButton() 
{
	CString strDialogTitle;
	CString strFileName;
	CString strFolderName;

	strDialogTitle.LoadString( IDS_IFP_BROWSE_FOR_FILE_DIALOG_TITLE );

	m_FileEdit.GetWindowText( strFileName );
	int nSlashPos = strFileName.ReverseFind( '\\' );
	if ( nSlashPos >= 0 )
	{
		strFolderName = strFileName.Left( nSlashPos );
	}
	
	CFileDialog fileDialog( true, _T( ".xml" ), strFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T( "XML files (*.xml)|*.xml|All Files (*.*)|*.*||" ), this );
	fileDialog.m_ofn.lpstrTitle = strDialogTitle;
	fileDialog.m_ofn.lpstrInitialDir = strFolderName;
	
	if ( fileDialog.DoModal() == IDOK )
	{
		strFileName = fileDialog.GetPathName();
		CStringManager::ExtendFileExtention( &strFileName, _T( ".xml" ) );
		resizeDialogOptions.szParameters[1] = strFileName;
		m_FileEdit.SetWindowText( resizeDialogOptions.szParameters[1].c_str() );

		UpdateControls();
	}
}


void CImportFromPAKDialog::OnChangePAKBrowseEdit() 
{
	CString strFolderName;
	m_PAKEdit.GetWindowText( strFolderName );
	resizeDialogOptions.szParameters[0] = strFolderName;
	UpdateControls();
}


void CImportFromPAKDialog::OnChangeFileBrowseEdit() 
{
	CString strFileName;
	m_FileEdit.GetWindowText( strFileName );
	resizeDialogOptions.szParameters[1] = strFileName;
	UpdateControls();
}


void	CImportFromPAKDialog::UpdateControls()
{
	if ( CWnd *pWnd = GetDlgItem( IDOK ) )
	{
		pWnd->EnableWindow( !resizeDialogOptions.szParameters[0].empty() && !resizeDialogOptions.szParameters[1].empty() );
	}
}


void CImportFromPAKDialog::OnOK() 
{
	CStringManager::ExtendFileExtention( &( resizeDialogOptions.szParameters[0] ), ".pak" );
	CStringManager::ExtendFileExtention( &( resizeDialogOptions.szParameters[1] ), ".xml" );
	CResizeDialog::OnOK();
}


// basement storage  


