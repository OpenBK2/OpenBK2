#include "StdAfx.h"

#include "resource.h"
#include "ImportFromGameDialog.h"
#include "../MapEditorLib/StringManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const int CImportFromGameDialog::vID[] = 
{
	IDC_IFG_FOLDER_BROWSE_LABEL,	//0
	IDC_IFG_FOLDER_BROWSE_EDIT,		//1
	IDC_IFG_FOLDER_BROWSE_BUTTON,	//2
	IDC_IFG_FILE_BROWSE_LABEL,		//3
	IDC_IFG_FILE_BROWSE_EDIT,			//4
	IDC_IFG_FILE_BROWSE_BUTTON,		//5
	IDOK,													//6
	IDCANCEL,											//7
};


CImportFromGameDialog::CImportFromGameDialog( CWnd* pParent )
	: CResizeDialog( CImportFromGameDialog::IDD, pParent )
{
	SetControlStyle( IDC_IFG_FOLDER_BROWSE_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_IFG_FOLDER_BROWSE_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_IFG_FOLDER_BROWSE_BUTTON, ANCHORE_RIGHT_TOP );
	
	SetControlStyle( IDC_IFG_FILE_BROWSE_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_IFG_FILE_BROWSE_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_IFG_FILE_BROWSE_BUTTON, ANCHORE_RIGHT_TOP );

	SetControlStyle( IDOK, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER );
	SetControlStyle( IDCANCEL, ANCHORE_BOTTOM | ANCHORE_HOR_CENTER );
}


void CImportFromGameDialog::GetRegistryKey( string *pszRegistryKey )
{
	if ( pszRegistryKey )
	{
		CString strPath;
		CString strProgramKey;
		CString strKey;
		strPath.LoadString( IDS_REGISTRY_PATH );
		strProgramKey.LoadString( AFX_IDS_APP_TITLE );
		strKey.LoadString( IDS_IFG_REGISTRY_KEY );
		( *pszRegistryKey ) = StrFmt( _T( "Software\\%s\\%s\\%s" ), LPCTSTR( strPath ), LPCTSTR( strProgramKey ), LPCTSTR( strKey ) );
	}
}


void CImportFromGameDialog::DoDataExchange(CDataExchange* pDX)
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control(pDX, IDC_IFG_FOLDER_BROWSE_EDIT, m_FolderEdit);
	DDX_Control(pDX, IDC_IFG_FILE_BROWSE_EDIT, m_FileEdit);
}


BEGIN_MESSAGE_MAP(CImportFromGameDialog, CResizeDialog)
	ON_BN_CLICKED(IDC_IFG_FILE_BROWSE_BUTTON, OnFileBrowseButton)
	ON_BN_CLICKED(IDC_IFG_FOLDER_BROWSE_BUTTON, OnFolderBrowseButton)
	ON_EN_CHANGE(IDC_IFG_FILE_BROWSE_EDIT, OnChangeFileBrowseEdit)
	ON_EN_CHANGE(IDC_IFG_FOLDER_BROWSE_EDIT, OnChangeFolderBrowseEdit)
END_MESSAGE_MAP()


BOOL CImportFromGameDialog::OnInitDialog() 
{
	CResizeDialog::OnInitDialog();
	
	//0 folder
	//1 file
	if ( resizeDialogOptions.szParameters.size() < 2 )
	{
		resizeDialogOptions.szParameters.resize( 2 );
	}

	m_FolderEdit.SetWindowText( resizeDialogOptions.szParameters[0].c_str() );
	m_FileEdit.SetWindowText( resizeDialogOptions.szParameters[1].c_str() );
	
	UpdateControls();
	return TRUE;
}


void CImportFromGameDialog::GetGamePath( string *pszGamePath )
{
	NI_ASSERT( pszGamePath != 0, StrFmt( _T( "CImportFromGameDialog::GetGamePath() wrong parameter: pszGamePath %x" ), pszGamePath ) );
	if ( pszGamePath )
	{
		( *pszGamePath ) = resizeDialogOptions.szParameters[0];
	}
}


void CImportFromGameDialog::GetFilePath( string *pszFilePath )
{
	NI_ASSERT( pszFilePath != 0, StrFmt( _T( "CImportFromGameDialog::GetFilePath() wrong parameter: pszFilePath %x" ), pszFilePath ) );
	if ( pszFilePath )
	{
		( *pszFilePath ) = resizeDialogOptions.szParameters[1];
	}
}


void CImportFromGameDialog::OnFolderBrowseButton() 
{
	CString strDialogTitle;
	CString strFolderName;

	strDialogTitle.LoadString( IDS_IFG_BROWSE_FOR_FOLDER_DIALOG_TITLE );
	m_FolderEdit.GetWindowText( strFolderName );
	
	SECDirSelectDlg dirSelectDialog( SECDirSelectDlg::win32Style, strDialogTitle, strFolderName, this );
	
	if ( dirSelectDialog.DoModal() == IDOK )
	{
		dirSelectDialog.GetPath( strFolderName );
		resizeDialogOptions.szParameters[0] = strFolderName;
		m_FolderEdit.SetWindowText( resizeDialogOptions.szParameters[0].c_str() );
		
		UpdateControls();
	}
}


void CImportFromGameDialog::OnFileBrowseButton() 
{
	CString strDialogTitle;
	CString strFileName;
	CString strFolderName;

	strDialogTitle.LoadString( IDS_IFG_BROWSE_FOR_FILE_DIALOG_TITLE );

	m_FileEdit.GetWindowText( strFileName );
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
		resizeDialogOptions.szParameters[1] = strFileName;
		m_FileEdit.SetWindowText( resizeDialogOptions.szParameters[1].c_str() );

		UpdateControls();
	}
}


void CImportFromGameDialog::OnChangeFolderBrowseEdit() 
{
	CString strFolderName;
	m_FolderEdit.GetWindowText( strFolderName );
	resizeDialogOptions.szParameters[0] = strFolderName;
	UpdateControls();
}


void CImportFromGameDialog::OnChangeFileBrowseEdit() 
{
	CString strFileName;
	m_FileEdit.GetWindowText( strFileName );
	resizeDialogOptions.szParameters[1] = strFileName;
	UpdateControls();
}


void	CImportFromGameDialog::UpdateControls()
{
	if ( CWnd *pWnd = GetDlgItem( IDOK ) )
	{
		pWnd->EnableWindow( !resizeDialogOptions.szParameters[0].empty() && !resizeDialogOptions.szParameters[1].empty() );
	}
}


void CImportFromGameDialog::OnOK() 
{
	if ( resizeDialogOptions.szParameters[0][resizeDialogOptions.szParameters[0].size() - 1] != '\\' )
	{
		resizeDialogOptions.szParameters[0] += string( "\\" );	
	}
	CStringManager::ExtendFileExtention( &( resizeDialogOptions.szParameters[1] ), ".pak" );
	CResizeDialog::OnOK();
}


// basement storage  



