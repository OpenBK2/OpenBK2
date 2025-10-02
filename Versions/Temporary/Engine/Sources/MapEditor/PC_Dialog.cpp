#include "stdafx.h"
#include "..\mapeditorlib\commandhandlerdefines.h"

#include "PC_Dialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const UINT	CPCDialog::PC_TREE_COLUMN_NAME  [PC_TREE_COLUMN_COUNT] = { IDS_PC_PROPERTY_THN_0, IDS_PC_PROPERTY_THN_1, IDS_PC_PROPERTY_THN_2 };
const int		CPCDialog::PC_TREE_COLUMN_FORMAT[PC_TREE_COLUMN_COUNT] = { LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_LEFT };
const int		CPCDialog::PC_TREE_COLUMN_WIDTH [PC_TREE_COLUMN_COUNT] = { 100, 50, 100 };


/**
static MSG staticPCDialogMsg;
LRESULT CPCDialog::WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) 
{
	if ( ( message == WM_KEYDOWN ) && ( AfxGetMainWnd() != 0 ) )
	{
		HACCEL hAccel = ::LoadAccelerators( AfxGetResourceHandle(), MAKEINTRESOURCE( IDR_EDITORTYPE ) );
		staticPCDialogMsg.hwnd = AfxGetMainWnd()->m_hWnd;
		staticPCDialogMsg.message = message;
    staticPCDialogMsg.wParam = wParam;
    staticPCDialogMsg.lParam = lParam;
		staticPCDialogMsg.time = ::GetTickCount();
    staticPCDialogMsg.pt.x = 0;
    staticPCDialogMsg.pt.y = 0;
		::TranslateAccelerator( AfxGetMainWnd()->m_hWnd, hAccel, &staticPCDialogMsg );
	}
	return CResizeDialog::WindowProc( message, wParam, lParam );
}
**/


CPCDialog::CPCDialog( CWnd* pParent )
	: CResizeDialog( CPCDialog::IDD, pParent ),
		bCreateControls( false ),
		pPreviousCommandHandler( 0 ),
		tree( true, false )
{
	SetControlStyle( IDC_PC_TREE, ANCHORE_LEFT_TOP | RESIZE_HOR | RESIZE_VER );
	SetControlStyle( IDC_PC_STATUSBAR, ANCHORE_LEFT_BOTTOM | RESIZE_HOR );
}


void CPCDialog::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_PC_STATUSBAR, wndTreeStatusStringWindow );
}


BEGIN_MESSAGE_MAP(CPCDialog, CResizeDialog)
	ON_WM_DESTROY()
	//ON_NOTIFY(TVN_SELCHANGED, IDC_PC_TREE, OnSelchangedTree)
END_MESSAGE_MAP()


BOOL CPCDialog::OnInitDialog()
{
	bCreateControls = true;

	CResizeDialog::OnInitDialog();

	ICommandHandlerContainer* pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
	pPreviousCommandHandler	= pCommandHandlerContainer->Get( CHID_PC_DIALOG );
	pCommandHandlerContainer->Set( CHID_PC_DIALOG, this );

	//PC_TREE_COLUMN_WIDTH
	if ( resizeDialogOptions.nParameters.size() < PC_TREE_COLUMN_COUNT )
	{
		resizeDialogOptions.nParameters.resize( PC_TREE_COLUMN_COUNT, 0 );
	}
	InitImageLists();

	CRect clientRect;
	GetClientRect( &clientRect );

	tree.SubclassTreeCtrlId( IDC_PC_TREE, this );
	tree.SetStatusStringWindow( &wndTreeStatusStringWindow );
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
	strHeaderName.LoadString( PC_TREE_COLUMN_NAME[0] );
	tree.SetColumnHeading( 0, strHeaderName );
	tree.SetColumnFormat( 0, PC_TREE_COLUMN_FORMAT[0] );
	tree.SetColumnWidth( 0, ( resizeDialogOptions.nParameters[0] > 0 ) ? resizeDialogOptions.nParameters[0] : PC_TREE_COLUMN_WIDTH[0] );
	tree.SetColumnImage( 0, 0 );
	// вставляем другие колонки
	for ( int index = 1; index < PC_TREE_COLUMN_COUNT; ++index )
	{
		strHeaderName.LoadString( PC_TREE_COLUMN_NAME[index] );
		tree.InsertColumn( index, strHeaderName, PC_TREE_COLUMN_FORMAT[index], ( resizeDialogOptions.nParameters[index] > 0 ) ? resizeDialogOptions.nParameters[index] : PC_TREE_COLUMN_WIDTH[index], index, 0 );
		tree.SetColumnImage( index, index );
	}
	tree.MoveWindow( &treeRect ); //disable visual errors in stingray tree 
	tree.EnableEdit( tree.IsEditEnabled() );

	bCreateControls = false;
	CreateTree();
	
	return true;
}


void CPCDialog::InitImageLists()
{
	{
		CBitmap bmp;
		typesImageList.Create( 16, 16, ILC_MASK, PCIE_COUNT * 2, PCIE_COUNT * 2 );
		NI_ASSERT( typesImageList.m_hImageList != 0, "CPCDialog::InitImageLists, can't create typesImageList" );

		bmp.LoadBitmap( IDB_PC_TYPES_IMAGE_LIST );
		typesImageList.Add( &bmp, RGB( 255, 0, 255 ) );
		bmp.DeleteObject();
	}
	{
		CBitmap bmp;
		headerImageList.Create( 16, 16, ILC_MASK, PC_TREE_COLUMN_COUNT, PC_TREE_COLUMN_COUNT );
		NI_ASSERT( headerImageList.m_hImageList != 0, "CPCDialog::InitImageLists, can't create headerImageList" );

		bmp.LoadBitmap( IDB_PC_HEADER_IMAGE_LIST );
		headerImageList.Add( &bmp, RGB( 255, 0, 255 ) );
		bmp.DeleteObject();
	}
}


void CPCDialog::OnDestroy() 
{
	for ( int nColumnIndex = 0; nColumnIndex < PC_TREE_COLUMN_COUNT; ++nColumnIndex )
	{
		resizeDialogOptions.nParameters[nColumnIndex] = tree.GetColumnWidth( nColumnIndex );
	}
	SaveResizeDialogOptions();
	CResizeDialog::OnDestroy();
	//
	Singleton<ICommandHandlerContainer>()->Set( CHID_PC_DIALOG, pPreviousCommandHandler );
}

/**

void CPCDialog::OnSelchangedTree( NMHDR* pNMHDR, LRESULT* pResult ) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	//
	string szItemName;
	tree.GetSelectedPCItemDescription( &szItemName );
	SetStatusBarMessage( szItemName );
	//
	*pResult = 0;
}


void CPCDialog::SetStatusBarMessage( const string &rszMessage )
{
	SetDlgItemText( IDC_PC_STATUSBAR, rszMessage.c_str() );
}
/**/


IView* CPCDialog::GetView()
{ 
	return checked_cast<IView*>( &tree );
}


ICommandHandler* CPCDialog::GetCommandHandler()
{
	return checked_cast<ICommandHandler*>( &tree );
}


void CPCDialog::CreateTree()
{
	//DebugTrace( "CPCDialog::CreateTree()" );
	if ( ::IsWindow( m_hWnd ) )
	{
		bCreateControls = true;
		tree.CreateTree( TVI_ROOT, true, true );
		bCreateControls = false;
	}
}


void CPCDialog::UpdateValues()
{
	DebugTrace( "CPCDialog::UpdateValues()" );
	if ( ::IsWindow( m_hWnd ) )
	{
		bCreateControls = true;
		tree.CreateTree( TVI_ROOT, false, true );
		bCreateControls = false;
	}
}

// basement storage  


