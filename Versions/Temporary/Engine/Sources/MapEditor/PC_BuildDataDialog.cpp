#include "stdafx.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "wmdefines.h"

//#include "..\MapEditorLib\Tools_SysCodes.h"
#include "PC_BuildDataDialog.h"

const UINT	CPCBuildDataDialog::PC_BD_TREE_COLUMN_NAME  [PC_BD_TREE_COLUMN_COUNT] = { IDS_PC_PROPERTY_THN_0, IDS_PC_PROPERTY_THN_1, IDS_PC_PROPERTY_THN_2 };
const int		CPCBuildDataDialog::PC_BD_TREE_COLUMN_FORMAT[PC_BD_TREE_COLUMN_COUNT] = { LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_LEFT };
const int		CPCBuildDataDialog::PC_BD_TREE_COLUMN_WIDTH [PC_BD_TREE_COLUMN_COUNT] = { 100, 50, 100 };


CPCBuildDataDialog::CPCBuildDataDialog( CWnd* pParent )
	: CResizeDialog( CPCBuildDataDialog::IDD, pParent ),
		bCreateControls( false ),
		pPreviousCommandHandler( 0 ),
		pBuildDataParams( 0 ),
		pBuildDataCallback( 0 ),
		tree( true, true )
{
	SetControlStyle( IDC_PC_BD_NAME_LABEL, ANCHORE_LEFT_TOP );
	SetControlStyle( IDC_PC_BD_NAME_EDIT, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_PC_BD_TREE_LABEL, ANCHORE_LEFT_TOP | RESIZE_HOR );
	SetControlStyle( IDC_PC_BD_TREE, ANCHORE_LEFT_TOP | RESIZE_HOR | RESIZE_VER );
	SetControlStyle( IDC_PC_BD_TREE_STATUSBAR, ANCHORE_LEFT_BOTTOM | RESIZE_HOR );
	SetControlStyle( IDC_PC_BD_NEED_EXPORT_CHEKBOX, ANCHORE_LEFT_BOTTOM | RESIZE_HOR );
	SetControlStyle( IDOK, ANCHORE_RIGHT_BOTTOM );
	SetControlStyle( IDCANCEL, ANCHORE_RIGHT_BOTTOM );
}

void CPCBuildDataDialog::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_PC_BD_TREE_STATUSBAR, wndTreeStatusStringWindow );
}


BEGIN_MESSAGE_MAP(CPCBuildDataDialog, CResizeDialog)
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_PC_BD_NAME_EDIT, OnChangeNameEdit)
	ON_MESSAGE(WM_PC_MANIPULATOR_CHANGE, OnMessagePCManipulatorChange)
	ON_BN_CLICKED( IDC_PC_BD_NEED_EXPORT_CHEKBOX, OnNeedExportCheckBox )
END_MESSAGE_MAP()


BOOL CPCBuildDataDialog::OnInitDialog()
{
	NI_ASSERT( pBuildDataParams != 0, "CPCBuildDataDialog::OnInitDialog() pBuildDataParams == 0" );
	CResizeDialog::OnInitDialog();
	if ( pBuildDataParams == 0 )
	{
		return true;
	}
	bCreateControls = true;
	ICommandHandlerContainer* pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
	pPreviousCommandHandler	= pCommandHandlerContainer->Get( CHID_PC_BUILD_DATA_DIALOG );
	pCommandHandlerContainer->Set( CHID_PC_BUILD_DATA_DIALOG, this );

	SetDlgItemText( IDC_PC_BD_NAME_EDIT, pBuildDataParams->szObjectName.c_str() );
	CheckDlgButton( IDC_PC_BD_NEED_EXPORT_CHEKBOX, pBuildDataParams->bNeedExport ? BST_CHECKED : BST_UNCHECKED );

	//PC_BD_TREE_COLUMN_WIDTH
	if ( resizeDialogOptions.nParameters.size() < PC_BD_TREE_COLUMN_COUNT )
	{
		resizeDialogOptions.nParameters.resize( PC_BD_TREE_COLUMN_COUNT, 0 );
	}
	InitImageLists();

	CRect clientRect;
	GetClientRect( &clientRect );

	tree.SubclassTreeCtrlId( IDC_PC_BD_TREE, this );
	//tree.SetStatusStringWindow( &wndTreeStatusStringWindow );
	//disable visual errors in stingray tree 
	CRect treeRect;
	tree.GetWindowRect( &treeRect );
	ScreenToClient( &treeRect );

	tree.EnableHeaderCtrl( true, false );
	tree.StoreSubItemText( true );
	tree.ModifyListCtrlStyle( 0, LVS_NOSORTHEADER );
	tree.ModifyListCtrlStyleEx( 0, LVXS_HILIGHTSUBITEMS );
	tree.ModifyTreeCtrlStyleEx( 0, TVXS_MULTISEL );

	//tree.ModifyListCtrlStyleEx( 0, LVXS_LINESBETWEENCOLUMNS );
	//tree.ModifyListCtrlStyleEx( 0, LVXS_LINESBETWEENITEMS );

	tree.SetImageList( &typesImageList, TVSIL_NORMAL );
	tree.SetImageList( &headerImageList, LVSIL_HEADER );

	CString strHeaderName;
	tree.MoveWindow( CRect( 0, 0, 0, 0 ) ); //disable visual errors in stingray tree 
	
	// первая колонка уже существует
	strHeaderName.LoadString( PC_BD_TREE_COLUMN_NAME[0] );
	tree.SetColumnHeading( 0, strHeaderName );
	tree.SetColumnFormat( 0, PC_BD_TREE_COLUMN_FORMAT[0] );
	tree.SetColumnWidth( 0, ( resizeDialogOptions.nParameters[0] > 0 ) ? resizeDialogOptions.nParameters[0] : PC_BD_TREE_COLUMN_WIDTH[0] );
	tree.SetColumnImage( 0, 0 );
	// вставляем другие колонки
	for ( int index = 1; index < PC_BD_TREE_COLUMN_COUNT; ++index )
	{
		strHeaderName.LoadString( PC_BD_TREE_COLUMN_NAME[index] );
		tree.InsertColumn( index, strHeaderName, PC_BD_TREE_COLUMN_FORMAT[index], ( resizeDialogOptions.nParameters[index] > 0 ) ? resizeDialogOptions.nParameters[index] : PC_BD_TREE_COLUMN_WIDTH[index], index, 0 );
		tree.SetColumnImage( index, index );
	}
	tree.MoveWindow( &treeRect ); //disable visual errors in stingray tree 
	tree.EnableEdit( tree.IsEditEnabled() );

	//tree.SetBkColor( ::GetSysColor( COLOR_3DFACE ) );
	//tree.SetIconBkColor( ::GetSysColor( COLOR_3DFACE ) );
	//tree.SetSelIconBkColor( ::GetSysColor( COLOR_3DFACE ) );

	bCreateControls = false;
	CreateTree();
	UpdateOKButton();
	UpdateTitle();
	return true;
}


//CRAP{ PLAIN_TEXT
void CPCBuildDataDialog::UpdateOKButton()
{
	bool bEnableOKButton = true;
	string szErrorMessage;
	// Проверяем имя на уникальность
	if ( bEnableOKButton )
	{
		if ( pBuildDataParams->nFlags & BDF_CHECK_FILE_NAME )
		{
			string szObjectName;
			pBuildDataParams->GetObjectName( &szObjectName );
			if ( pBuildDataParams->szObjectName.empty() )
			{
				bEnableOKButton = false;
				szErrorMessage = "Object name is invalid. Name can't be empty.";
			}
			else if ( !pBuildDataCallback->IsUniqueObjectName( pBuildDataParams->szObjectTypeName, szObjectName ) )
			{
				bEnableOKButton = false;
				szErrorMessage = "Object name is invalid. Object already exists!";
			}
		}
	}
	// Проверяем поля на правильность заполнения
	if ( bEnableOKButton && ( pBuildDataParams->nFlags & BDF_CHECK_PROPERTIES ) )
	{
		if ( pBuildDataCallback && !pBuildDataCallback->IsValidBuildData( GetView()->GetViewManipulator(),  &szErrorMessage, &tree ) )
		{
			bEnableOKButton = false;
		}
	}
	//
	SetDlgItemText( IDC_PC_BD_TREE_STATUSBAR, szErrorMessage.c_str() );
	//
	if ( CWnd *pwndOKButton = GetDlgItem( IDOK ) )
	{
		pwndOKButton->EnableWindow( bEnableOKButton );
	}
}
//CRAP} PLAIN_TEXT


void CPCBuildDataDialog::UpdateTitle()
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


void CPCBuildDataDialog::InitImageLists()
{
	{
		CBitmap bmp;
		typesImageList.Create( 16, 16, ILC_MASK, PCIE_COUNT * 2, PCIE_COUNT * 2 );
		NI_ASSERT( typesImageList.m_hImageList != 0, "CPCBuildDataDialog::InitImageLists, can't create typesImageList" );

		bmp.LoadBitmap( IDB_PC_TYPES_IMAGE_LIST );
		typesImageList.Add( &bmp, RGB( 255, 0, 255 ) );
		bmp.DeleteObject();
	}
	{
		CBitmap bmp;
		headerImageList.Create( 16, 16, ILC_MASK, PC_BD_TREE_COLUMN_COUNT, PC_BD_TREE_COLUMN_COUNT );
		NI_ASSERT( headerImageList.m_hImageList != 0, "CPCBuildDataDialog::InitImageLists, can't create headerImageList" );

		bmp.LoadBitmap( IDB_PC_HEADER_IMAGE_LIST );
		headerImageList.Add( &bmp, RGB( 255, 0, 255 ) );
		bmp.DeleteObject();
	}
}


void CPCBuildDataDialog::OnDestroy() 
{
	for ( int nColumnIndex = 0; nColumnIndex < PC_BD_TREE_COLUMN_COUNT; ++nColumnIndex )
	{
		resizeDialogOptions.nParameters[nColumnIndex] = tree.GetColumnWidth( nColumnIndex );
	}
	SaveResizeDialogOptions();
	CResizeDialog::OnDestroy();

	Singleton<ICommandHandlerContainer>()->Set( CHID_PC_BUILD_DATA_DIALOG, pPreviousCommandHandler );
}


void CPCBuildDataDialog::OnChangeNameEdit()
{
	CString strNewObjectName;
	GetDlgItemText( IDC_PC_BD_NAME_EDIT, strNewObjectName );
	//
	pBuildDataParams->szObjectName = strNewObjectName;
	UpdateOKButton();	 
}


void CPCBuildDataDialog::OnNeedExportCheckBox()
{
	pBuildDataParams->bNeedExport = ( IsDlgButtonChecked( IDC_PC_BD_NEED_EXPORT_CHEKBOX ) > 0 );
}


LRESULT CPCBuildDataDialog::OnMessagePCManipulatorChange( WPARAM wParam, LPARAM lParam )
{
	UpdateOKButton();
	return 0;
}


IView* CPCBuildDataDialog::GetView()
{ 
	return checked_cast<IView*>( &tree );
}


ICommandHandler* CPCBuildDataDialog::GetCommandHandler()
{
	return checked_cast<ICommandHandler*>( &tree );
}


void CPCBuildDataDialog::CreateTree()
{
	DebugTrace( "CPCBuildDataDialog::CreateTree()" );
	if ( ::IsWindow( m_hWnd ) )
	{
		bCreateControls = true;
		tree.CreateTree( TVI_ROOT, true, true );
		bCreateControls = false;
	}
}


void CPCBuildDataDialog::UpdateValues()
{
	DebugTrace( "CPCBuildDataDialog::UpdateValues()" );
	if ( ::IsWindow( m_hWnd ) )
	{
		bCreateControls = true;
		tree.CreateTree( TVI_ROOT, false, true );
		bCreateControls = false;
	}
}

// basement storage  


