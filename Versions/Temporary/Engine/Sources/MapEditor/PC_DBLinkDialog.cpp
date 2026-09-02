#include "stdafx.h"
#include <fmt/format.h>
#include <fmt/printf.h>
#include "MapEditorLib/CommandHandlerDefines.h"
#include "WMDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "Tree_GDBBrowserBase_Constants.h"

#include "PC_DBLinkDialog.h"

#include "libdb/ResourceManager.h"
#include "Tree_GDBLinkBrowser.h"
#include "MapEditorLib/Tools_HashSet.h"

const unsigned	CPCDBLinkDialog::PC_DBL_TREE_COLUMN_NAME  [PC_DBL_TREE_COLUMN_COUNT] = { IDS_PC_PROPERTY_THN_0, IDS_PC_PROPERTY_THN_1, IDS_PC_PROPERTY_THN_2 };
const int		CPCDBLinkDialog::PC_DBL_TREE_COLUMN_FORMAT[PC_DBL_TREE_COLUMN_COUNT] = { LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_LEFT };
const int		CPCDBLinkDialog::PC_DBL_TREE_COLUMN_WIDTH [PC_DBL_TREE_COLUMN_COUNT] = { 100, 50, 100 };


CPCDBLinkDialog::CPCDBLinkDialog( EType _eType, bool _bMultiRef, bool _bTextEditor, int _nFixedWidth, int _nFixedHeight, CWnd* pParent )
: CResizeDialog( _bTextEditor ? IDD_PC_DB_LINK_EX : IDD_PC_DB_LINK, pParent ),
		bCreateControls( true ),
		pPreviousCommandHandler( 0 ),
		eType( _eType ),
		bMultiRef( _bMultiRef ),
		bTextEditor( _bTextEditor ),
		nFixedWidth( _nFixedWidth ),
		nFixedHeight( _nFixedHeight ),
		bEmpty( false ),
		tab( -1 ),
		tree( true, true )
{
	if ( nFixedWidth > 0 )
	{
		// Tab
		SetControlStyle( IDC_PC_DBL_TAB_LABEL,				ANCHORE_LEFT_TOP ); 
		SetControlStyle( IDC_PC_DBL_TAB_PLACEHOLDER,	ANCHORE_LEFT_TOP | RESIZE_VER ); 
		//SetControlStyle( IDC_PC_DBL_TAB,							ANCHORE_LEFT_TOP | RESIZE_VER );
		if ( _bTextEditor )
		{
			if ( nFixedHeight > 0 )
			{
				// Tree
				SetControlStyle( IDC_PC_DBL_TREE_LABEL,			ANCHORE_LEFT_TOP | RESIZE_HOR ); 
				SetControlStyle( IDC_PC_DBL_TREE,						ANCHORE_LEFT_TOP | RESIZE_HOR ); 
				SetControlStyle( IDC_PC_DBL_TREE_STATUSBAR,	ANCHORE_LEFT_TOP | RESIZE_HOR ); 
				// Editor
				SetControlStyle( IDC_PC_DBL_EDITOR_LABEL,				ANCHORE_LEFT_TOP | RESIZE_HOR ); 
				SetControlStyle( IDC_PC_DBL_EDITOR_PLACEHOLDER,	ANCHORE_LEFT_TOP | RESIZE_HOR_VER ); 
				SetControlStyle( IDC_PC_DBL_EDITOR,							ANCHORE_LEFT_TOP | RESIZE_HOR_VER ); 
				SetControlStyle( IDC_PC_DBL_EDITOR_STATUSBAR,		ANCHORE_LEFT_BOTTOM | RESIZE_HOR ); 
			}
			else
			{
				// Tree
				SetControlStyle( IDC_PC_DBL_TREE_LABEL,			ANCHORE_LEFT_TOP | RESIZE_HOR ); 
				SetControlStyle( IDC_PC_DBL_TREE,						ANCHORE_LEFT_TOP | RESIZE_HOR_VER, 0.5f, 0.5f, 1.0f, 0.5f  ); 
				SetControlStyle( IDC_PC_DBL_TREE_STATUSBAR,	ANCHORE_LEFT | ANCHORE_VER_CENTER | RESIZE_HOR ); 
				// Editor
				SetControlStyle( IDC_PC_DBL_EDITOR_LABEL,				ANCHORE_LEFT | ANCHORE_VER_CENTER | RESIZE_HOR ); 
				SetControlStyle( IDC_PC_DBL_EDITOR_PLACEHOLDER,	ANCHORE_LEFT_BOTTOM | RESIZE_HOR_VER, 0.5f, 0.5f, 1.0f, 0.5f ); 
				SetControlStyle( IDC_PC_DBL_EDITOR,							ANCHORE_LEFT_BOTTOM | RESIZE_HOR_VER, 0.5f, 0.5f, 1.0f, 0.5f ); 
				SetControlStyle( IDC_PC_DBL_EDITOR_STATUSBAR,		ANCHORE_LEFT_BOTTOM | RESIZE_HOR ); 
			}
		}
		else
		{
			// Tree
			SetControlStyle( IDC_PC_DBL_TREE_LABEL,			ANCHORE_LEFT_TOP | RESIZE_HOR ); 
			SetControlStyle( IDC_PC_DBL_TREE,						ANCHORE_LEFT_TOP | RESIZE_HOR_VER ); 
			SetControlStyle( IDC_PC_DBL_TREE_STATUSBAR,	ANCHORE_LEFT_BOTTOM | RESIZE_HOR ); 
		}

		// Other
		SetControlStyle( IDC_PC_DBL_CUR_SEL_LABEL_LEFT,		ANCHORE_LEFT_BOTTOM );
		SetControlStyle( IDC_PC_DBL_CUR_SEL_LABEL_RIGHT,	ANCHORE_LEFT_BOTTOM | RESIZE_HOR );
		SetControlStyle( IDC_PC_DBL_PREV_SEL_LABEL_LEFT,	ANCHORE_LEFT_BOTTOM );
		SetControlStyle( IDC_PC_DBL_PREV_SEL_LABEL_RIGHT,	ANCHORE_LEFT_BOTTOM | RESIZE_HOR );
		SetControlStyle( IDC_PC_DBL_SET_EMPTY_BUTTON,			ANCHORE_LEFT_BOTTOM );
		SetControlStyle( IDOK,														ANCHORE_RIGHT_BOTTOM );
		SetControlStyle( IDCANCEL,												ANCHORE_RIGHT_BOTTOM );
	}
	else
	{
		// Tab
		SetControlStyle( IDC_PC_DBL_TAB_LABEL,				ANCHORE_LEFT_TOP | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f ); 
		SetControlStyle( IDC_PC_DBL_TAB_PLACEHOLDER,	ANCHORE_LEFT_TOP | RESIZE_HOR_VER, 0.5f, 0.5f, 0.5f, 1.0f ); 
		//SetControlStyle( IDC_PC_DBL_TAB,							ANCHORE_LEFT_TOP | RESIZE_HOR_VER, 0.5f, 0.5f, 0.5f, 1.0f );
		if ( _bTextEditor )
		{
			if ( nFixedHeight > 0 )
			{
				// Tree
				SetControlStyle( IDC_PC_DBL_TREE_LABEL,			ANCHORE_RIGHT_TOP | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f ); 
				SetControlStyle( IDC_PC_DBL_TREE,						ANCHORE_RIGHT_TOP | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f ); 
				SetControlStyle( IDC_PC_DBL_TREE_STATUSBAR,	ANCHORE_RIGHT_TOP | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f ); 
				// Editor
				SetControlStyle( IDC_PC_DBL_EDITOR_LABEL,				ANCHORE_RIGHT_TOP | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f ); 
				SetControlStyle( IDC_PC_DBL_EDITOR_PLACEHOLDER,	ANCHORE_RIGHT_TOP | RESIZE_HOR_VER, 0.5f, 0.5f, 0.5f, 1.0f ); 
				SetControlStyle( IDC_PC_DBL_EDITOR,							ANCHORE_RIGHT_TOP | RESIZE_HOR_VER, 0.5f, 0.5f, 0.5f, 1.0f ); 
				SetControlStyle( IDC_PC_DBL_EDITOR_STATUSBAR,		ANCHORE_RIGHT_BOTTOM | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f ); 
			}
			else
			{
				// Tree
				SetControlStyle( IDC_PC_DBL_TREE_LABEL,			ANCHORE_RIGHT_TOP | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f ); 
				SetControlStyle( IDC_PC_DBL_TREE,						ANCHORE_RIGHT_TOP | RESIZE_HOR_VER, 0.5f, 0.5f, 0.5f, 0.5f  ); 
				SetControlStyle( IDC_PC_DBL_TREE_STATUSBAR,	ANCHORE_RIGHT | ANCHORE_VER_CENTER | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f ); 
				// Editor
				SetControlStyle( IDC_PC_DBL_EDITOR_LABEL,				ANCHORE_RIGHT | ANCHORE_VER_CENTER | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f ); 
				SetControlStyle( IDC_PC_DBL_EDITOR_PLACEHOLDER,	ANCHORE_RIGHT_BOTTOM | RESIZE_HOR_VER, 0.5f, 0.5f, 0.5f, 0.5f ); 
				SetControlStyle( IDC_PC_DBL_EDITOR,							ANCHORE_RIGHT_BOTTOM | RESIZE_HOR_VER, 0.5f, 0.5f, 0.5f, 0.5f ); 
				SetControlStyle( IDC_PC_DBL_EDITOR_STATUSBAR,		ANCHORE_RIGHT_BOTTOM | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f ); 
			}
		}
		else
		{
			// Tree
			SetControlStyle( IDC_PC_DBL_TREE_LABEL,			ANCHORE_RIGHT_TOP | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f ); 
			SetControlStyle( IDC_PC_DBL_TREE,						ANCHORE_RIGHT_TOP | RESIZE_HOR_VER, 0.5f, 0.5f, 0.5f, 1.0f ); 
			SetControlStyle( IDC_PC_DBL_TREE_STATUSBAR,	ANCHORE_RIGHT_BOTTOM | RESIZE_HOR, 0.5f, 0.5f, 0.5f, 1.0f ); 
		}

		// Other
		SetControlStyle( IDC_PC_DBL_CUR_SEL_LABEL_LEFT,		ANCHORE_LEFT_BOTTOM );
		SetControlStyle( IDC_PC_DBL_CUR_SEL_LABEL_RIGHT,	ANCHORE_LEFT_BOTTOM | RESIZE_HOR );
		SetControlStyle( IDC_PC_DBL_PREV_SEL_LABEL_LEFT,	ANCHORE_LEFT_BOTTOM );
		SetControlStyle( IDC_PC_DBL_PREV_SEL_LABEL_RIGHT,	ANCHORE_LEFT_BOTTOM | RESIZE_HOR );
		SetControlStyle( IDC_PC_DBL_SET_EMPTY_BUTTON,			ANCHORE_LEFT_BOTTOM );
		SetControlStyle( IDOK,														ANCHORE_RIGHT_BOTTOM );
		SetControlStyle( IDCANCEL,												ANCHORE_RIGHT_BOTTOM );
	}
}


void CPCDBLinkDialog::DoDataExchange( CDataExchange* pDX )
{
	CResizeDialog::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_PC_DBL_TREE_STATUSBAR, wndTreeStatusStringWindow );
	if ( bTextEditor )
	{
		DDX_Control( pDX, IDC_PC_DBL_EDITOR_STATUSBAR, wndEditorStatusStringWindow );
	}
}


BEGIN_MESSAGE_MAP(CPCDBLinkDialog, CResizeDialog)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	//ON_NOTIFY(TVN_SELCHANGED, IDC_PC_DBL_TREE, OnSelchangedTree)
	ON_MESSAGE(WM_TREE_GDB_BROWSER, OnMessageTreeGDBBrowser)
	ON_MESSAGE( WM_GDB_BROWSER, OnTabSelected )
	ON_CBN_SELCHANGE( IDC_PC_DBL_TAB, OnTabSelected )
	ON_BN_CLICKED(IDC_PC_DBL_SET_EMPTY_BUTTON, OnBnClickedSetEmptyButton)
END_MESSAGE_MAP()


void CPCDBLinkDialog::SetSelectedTables( const CTableSet &rSelectedTables )
{ 
	selectedTables = rSelectedTables;
}


void CPCDBLinkDialog::SetCurrentTable( const std::string &rszCurrentTable )
{ 
	szCurrentTable = rszCurrentTable;
	szPreviousTable = szCurrentTable;
}


void CPCDBLinkDialog::SetCurrentObject( const std::string &rszCurrentObject )
{ 
	szCurrentObject = rszCurrentObject;
	szPreviousObject = szCurrentObject;
}


void CPCDBLinkDialog::GetCurrentTable( std::string *pszCurrentTable )
{
	if ( pszCurrentTable )
	{
		( *pszCurrentTable ) = szCurrentTable;
	}
}


void CPCDBLinkDialog::GetCurrentObject( std::string *pszCurrentObject )
{
	if ( pszCurrentObject )
	{
		( *pszCurrentObject ) = szCurrentObject;
	}
}


BOOL CPCDBLinkDialog::OnInitDialog() 
{
	bCreateControls = true;
	bEmpty = false;
	CResizeDialog::OnInitDialog();

	ICommandHandlerContainer* pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
	pPreviousCommandHandler	= pCommandHandlerContainer->Get( CHID_PC_DB_LINK_DIALOG );
	pCommandHandlerContainer->Set( CHID_PC_DB_LINK_DIALOG, this );

	//PC_DBL_TREE_COLUMN_WIDTH
	if ( resizeDialogOptions.nParameters.size() < PC_DBL_TREE_COLUMN_COUNT )
	{
		resizeDialogOptions.nParameters.resize( PC_DBL_TREE_COLUMN_COUNT, 0 );
	}
	//
	if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_PREV_SEL_LABEL_LEFT ) )
	{
		pWnd->ShowWindow( ( eType == TYPE_LINK ) ? SW_SHOW : SW_HIDE );
		pWnd->EnableWindow( eType == TYPE_LINK );
	}
	if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_SET_EMPTY_BUTTON ) )
	{
		pWnd->ShowWindow( ( eType == TYPE_LINK ) ? SW_SHOW : SW_HIDE );
		pWnd->EnableWindow( eType == TYPE_LINK );
	}
	//
	InitImageLists();
	//
	if ( nFixedWidth > 0 )
	{
		CRect rect;
		int nWidthDifference = 0;
		// Tab
		if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_TAB_LABEL ) )
		{
			pWnd->GetWindowRect( &rect );
			ScreenToClient( &rect );
			nWidthDifference = ( nFixedWidth - rect.Width() );
			rect.right += nWidthDifference;
			pWnd->MoveWindow( &rect );
		}
		if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_TAB_PLACEHOLDER ) )
		{
			pWnd->GetWindowRect( &rect );
			ScreenToClient( &rect );
			rect.right += nWidthDifference;
			pWnd->MoveWindow( &rect );
		}
		// Tree
		if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_TREE_LABEL ) )
		{
			pWnd->GetWindowRect( &rect );
			ScreenToClient( &rect );
			rect.left += nWidthDifference;
			pWnd->MoveWindow( &rect );
		}
		if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_TREE ) )
		{
			pWnd->GetWindowRect( &rect );
			ScreenToClient( &rect );
			rect.left += nWidthDifference;
			pWnd->MoveWindow( &rect );
		}
		if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_TREE_STATUSBAR ) )
		{
			pWnd->GetWindowRect( &rect );
			ScreenToClient( &rect );
			rect.left += nWidthDifference;
			pWnd->MoveWindow( &rect );
		}
		// Editor
		if ( bTextEditor )
		{
			if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_EDITOR_LABEL ) )
			{
				pWnd->GetWindowRect( &rect );
				ScreenToClient( &rect );
				rect.left += nWidthDifference;
				pWnd->MoveWindow( &rect );
			}
			if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_EDITOR_PLACEHOLDER ) )
			{
				pWnd->GetWindowRect( &rect );
				ScreenToClient( &rect );
				rect.left += nWidthDifference;
				pWnd->MoveWindow( &rect );
			}
			if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_EDITOR_STATUSBAR ) )
			{
				pWnd->GetWindowRect( &rect );
				ScreenToClient( &rect );
				rect.left += nWidthDifference;
				pWnd->MoveWindow( &rect );
			}
		}
	}
	//
	if ( bTextEditor )
	{
		if ( nFixedHeight > 0 )
		{
			CRect rect;
			int nHeightDifference = 0;
			// Tree
			if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_TREE ) )
			{
				pWnd->GetWindowRect( &rect );
				ScreenToClient( &rect );
				nHeightDifference = ( nFixedHeight - rect.Height() );
				rect.bottom += nHeightDifference;
				pWnd->MoveWindow( &rect );
			}
			if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_TREE_STATUSBAR ) )
			{
				pWnd->GetWindowRect( &rect );
				ScreenToClient( &rect );
				rect.top += nHeightDifference;
				rect.bottom += nHeightDifference;
				pWnd->MoveWindow( &rect );
			}
			// Editor
			if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_EDITOR_LABEL ) )
			{
				pWnd->GetWindowRect( &rect );
				ScreenToClient( &rect );
				rect.top += nHeightDifference;
				rect.bottom += nHeightDifference;
				pWnd->MoveWindow( &rect );
			}
			if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_EDITOR_PLACEHOLDER ) )
			{
				pWnd->GetWindowRect( &rect );
				ScreenToClient( &rect );
				rect.top += nHeightDifference;
				pWnd->MoveWindow( &rect );
			}
		}
		if ( pPCMultilineStringEditor = new CPCMultilineStringEditor() )
		{
			pPCMultilineStringEditor->CreateEx( this, WS_EX_CLIENTEDGE , WS_CHILD | WS_VISIBLE | WS_TABSTOP, CRect( 0, 0, 0, 0 ), IDC_PC_DBL_EDITOR );
			if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_EDITOR_PLACEHOLDER ) )
			{
				CRect windowRect;
				pWnd->GetWindowRect( &windowRect );
				ScreenToClient( &windowRect );
				pPCMultilineStringEditor->MoveWindow( &windowRect );
				if ( CFont* pFont = GetFont() )
				{
					pPCMultilineStringEditor->SetFont( pFont );
				}
				pPCMultilineStringEditor->SetStatusStringWindow( &wndEditorStatusStringWindow );
			}
		}
	}
	//
	tree.SubclassTreeCtrlId( IDC_PC_DBL_TREE, this );
	tree.SetStatusStringWindow( &wndTreeStatusStringWindow );
	tree.SetMultilineStringEditor( pPCMultilineStringEditor );
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
	strHeaderName.LoadString( PC_DBL_TREE_COLUMN_NAME[0] );
	tree.SetColumnHeading( 0, strHeaderName );
	tree.SetColumnFormat( 0, PC_DBL_TREE_COLUMN_FORMAT[0] );
	tree.SetColumnWidth( 0, ( resizeDialogOptions.nParameters[0] > 0 ) ? resizeDialogOptions.nParameters[0] : PC_DBL_TREE_COLUMN_WIDTH[0] );
	tree.SetColumnImage( 0, 0 );
	// вставляем другие колонки
	for ( int index = 1; index < PC_DBL_TREE_COLUMN_COUNT; ++index )
	{
		strHeaderName.LoadString( PC_DBL_TREE_COLUMN_NAME[index] );
		tree.InsertColumn( index, strHeaderName, PC_DBL_TREE_COLUMN_FORMAT[index], ( resizeDialogOptions.nParameters[index] > 0 ) ? resizeDialogOptions.nParameters[index] : PC_DBL_TREE_COLUMN_WIDTH[index], index, 0 );
		tree.SetColumnImage( index, index );
	}
	//disable visual errors in stingray tree 
	tree.MoveWindow( &treeRect );
	tree.EnableEdit( tree.IsEditEnabled() );
	//
	//tree.SetBkColor( ::GetSysColor( COLOR_3DFACE ) );
	//tree.SetIconBkColor( ::GetSysColor( COLOR_3DFACE ) );
	//tree.SetSelIconBkColor( ::GetSysColor( COLOR_3DFACE ) );

	tab.Create( this, IDC_PC_DBL_TAB );
	if ( IResourceManager *pResourceManager = Singleton<IResourceManager>() )
	{
		CPtr<IManipulator> pTableManipulator = pResourceManager->CreateTableManipulator();
		tables.clear();
		if ( pTableManipulator )
		{
			if ( CPtr<IManipulatorIterator> pTableManipulatorIterator = pTableManipulator->Iterate( true, ECT_NO_CACHE ) )
			{
				std::string szName;
				while ( !pTableManipulatorIterator->IsEnd() )
				{
					pTableManipulatorIterator->GetName( &szName );
					tables.push_back( szName );
					pTableManipulatorIterator->Next();
				}
			}
		}
	}
	tab.RemoveAllTabs();
	// Необходимо сначало добавить все панели, а потом устанавливать активную
	CTreeGDBLinkBrowser* pwndActiveTreeGBDBrowser = 0;
	for ( std::list<std::string>::const_iterator itTable = tables.begin(); itTable != tables.end(); ++itTable )
	{
		if ( selectedTables.find( *itTable ) != selectedTables.end() )
		{
			if ( CTreeGDBLinkBrowser* pwndTreeGBDBrowser = tab.AddNewTab( static_cast<CTreeGDBLinkBrowser*>( 0 ), *itTable ) )
			{
				if ( ( ( *itTable ) == szCurrentTable ) || ( pwndActiveTreeGBDBrowser == 0 ) )
				{
					pwndActiveTreeGBDBrowser = pwndTreeGBDBrowser;
				}
				pwndTreeGBDBrowser->SetPCDialogCommandHandlerID( CHID_PC_DB_LINK_DIALOG, false );
			}
		}
	}
	bCreateControls = false;
	//
	if ( pwndActiveTreeGBDBrowser )
	{
		tab.ActivateTab( pwndActiveTreeGBDBrowser );
	}
	else
	{
		tab.ActivateTab( tab.GetTab( 0 ) );
	}
	tab.ShowWindow( SW_SHOW );
	//	
	UpdateDialogTitle();
	UpdateCurrentObjectLabel();
	UpdatePreviousObjectLabel();
	UpdateControlPositions();
	UpdateOK();
  //Update();
	return true;
}


void CPCDBLinkDialog::InitImageLists()
{
	{
		CBitmap bmp;
		typesImageList.Create( 16, 16, ILC_MASK, PCIE_COUNT * 2, PCIE_COUNT * 2 );
		NI_ASSERT( typesImageList.m_hImageList != 0, "CPCDBLinkDialog::InitImageLists, can't create typesImageList" );

		bmp.LoadBitmap( IDB_PC_TYPES_IMAGE_LIST );
		typesImageList.Add( &bmp, RGB( 255, 0, 255 ) );
		bmp.DeleteObject();
	}
	{
		CBitmap bmp;
		headerImageList.Create( 16, 16, ILC_MASK, PC_DBL_TREE_COLUMN_COUNT, PC_DBL_TREE_COLUMN_COUNT );
		NI_ASSERT( headerImageList.m_hImageList != 0, "CPCDBLinkDialog::InitImageLists, can't create headerImageList" );

		bmp.LoadBitmap( IDB_PC_HEADER_IMAGE_LIST );
		headerImageList.Add( &bmp, RGB( 255, 0, 255 ) );
		bmp.DeleteObject();
	}
}


void CPCDBLinkDialog::OnDestroy() 
{
	for ( int nColumnIndex = 0; nColumnIndex < PC_DBL_TREE_COLUMN_COUNT; ++nColumnIndex )
	{
		resizeDialogOptions.nParameters[nColumnIndex] = tree.GetColumnWidth( nColumnIndex );
	}
	SaveResizeDialogOptions();
	pPCMultilineStringEditor = 0;
	tab.RemoveAllTabs();
	CResizeDialog::OnDestroy();

	Singleton<ICommandHandlerContainer>()->Set( CHID_PC_DB_LINK_DIALOG, pPreviousCommandHandler );
}


void CPCDBLinkDialog::OnSize( unsigned nType, int cx, int cy ) 
{
	CResizeDialog::OnSize( nType, cx, cy );
	//
	if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_TAB_PLACEHOLDER ) )
	{
		CRect rect;
		pWnd->GetWindowRect( &rect );
		ScreenToClient( &rect );
		tab.MoveWindow( &rect );
	}
}

/**

void CPCDBLinkDialog::OnSelchangedTree( NMHDR* pNMHDR, LRESULT* pResult ) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	//
	std::string szItemName;
	tree.GetSelectedPCItemDescription( &szItemName );
	SetStatusBarMessage( szItemName );
	//
	*pResult = 0;
}
/**/


LRESULT CPCDBLinkDialog::OnTabSelected( WPARAM wParam, LPARAM lParam )
{
	OnTabSelected();
	return 0;
}


void CPCDBLinkDialog::OnTabSelected()
{
	if ( !bCreateControls )
	{
		tab.SwitchTabs();
		if ( CWnd *pWnd = GetDlgItem( IDC_PC_DBL_TAB_PLACEHOLDER ) )
		{
			CRect rect;
			pWnd->GetWindowRect( &rect );
			ScreenToClient( &rect );
			tab.MoveWindow( &rect );
		}
		//
		CTreeGDBBrowserBase* pwndTreeGBDBrowserBase = 0;
		tab.GetActiveTabName( &szCurrentTable );
		tab.GetActiveTab( &pwndTreeGBDBrowserBase );
		if ( !pwndTreeGBDBrowserBase->IsTreeCreated() )
		{
			SObjectSet collectionObjectSet;
			collectionObjectSet.szObjectTypeName = szCurrentTable;
			InsertHashSetElement( &( collectionObjectSet.objectNameSet ), VIEW_COLLECTION_ID );
			//
			IResourceManager *pResourceManager = Singleton<IResourceManager>();
			pwndTreeGBDBrowserBase->SetViewManipulator( pResourceManager->CreateFolderManipulator( szCurrentTable ), collectionObjectSet, std::string() );
			pwndTreeGBDBrowserBase->SetCurrentTreeItemName( szPreviousObject, false );
			pwndTreeGBDBrowserBase->CreateTree();
			//DebugTrace( "CDWGDBBrowser::OnTabSelected(): wParam: 0x%X(%u), lParam: 0x%X\n", wParam, wParam, lParam );
		}
		else
		{
			pwndTreeGBDBrowserBase->UpdateSelectionManipulator( true );
		}
		
		UpdateDialogTitle();
		UpdateCurrentObjectLabel();
		UpdateOK();
	}
}


void CPCDBLinkDialog::OnBnClickedSetEmptyButton()
{
	bEmpty = true;
	OnOK();
}


LRESULT CPCDBLinkDialog::OnMessageTreeGDBBrowser( WPARAM wParam, LPARAM lParam )
{
	if ( CTreeGDBLinkBrowser* pwndActiveTreeGBDBrowser = reinterpret_cast<CTreeGDBLinkBrowser*>( lParam ) )
	{
		if ( LOWORD( wParam ) == TREE_GDB_BROWSER_CHANGE_SELECTION )
		{
			std::string szName;
			if ( pwndActiveTreeGBDBrowser->GetCurrentTreeItemName( &szName ) )
			{
				szCurrentObject = szName;
				szCurrentTable = pwndActiveTreeGBDBrowser->GetObjectSet().szObjectTypeName;
				UpdateDialogTitle();
				UpdateCurrentObjectLabel();
				UpdateOK();
			}
		}
		else if ( LOWORD( wParam ) == TREE_GDB_BROWSER_LOAD )
		{
			OnOK();
		}
	}
	return 0;
}


void CPCDBLinkDialog::UpdateDialogTitle()
{
	CString strFormatString;
	switch(  eType )
	{
		case TYPE_OPEN:
		{
			strFormatString.LoadString( szCurrentTable.empty() ? IDS_PC_OPEN_DIALOG_EMPTY_TITLE : IDS_PC_OPEN_DIALOG_TITLE );
			break;
		}
		case TYPE_LINK:
		{
			strFormatString.LoadString( szCurrentTable.empty() ? IDS_PC_LINK_DIALOG_EMPTY_TITLE : IDS_PC_LINK_DIALOG_TITLE );
			break;
		}
		default:
		{
			if ( !szCurrentTable.empty() )
			{
				strFormatString = "%s";
			}
		}
	}
	SetWindowText( fmt::sprintf( strFormatString.GetString(), szCurrentTable.c_str() ) );
}


void CPCDBLinkDialog::UpdateCurrentObjectLabel()
{
	if ( bMultiRef )
	{
		SetDlgItemText( IDC_PC_DBL_CUR_SEL_LABEL_RIGHT, fmt::format( "{}{:c}{}", szCurrentTable.c_str(), TYPE_SEPARATOR_CHAR, szCurrentObject.c_str() ) );
	}
	else
	{
		SetDlgItemText( IDC_PC_DBL_CUR_SEL_LABEL_RIGHT, fmt::format( "{}", szCurrentObject.c_str() ) );
	}
}


void CPCDBLinkDialog::UpdatePreviousObjectLabel()
{
	if ( eType == TYPE_LINK )
	{
		if ( bMultiRef )
		{
			SetDlgItemText( IDC_PC_DBL_PREV_SEL_LABEL_RIGHT, fmt::format( "{}{:c}{}", szPreviousTable.c_str(), TYPE_SEPARATOR_CHAR, szPreviousObject.c_str() ) );
		}
		else
		{
			SetDlgItemText( IDC_PC_DBL_PREV_SEL_LABEL_RIGHT, fmt::format( "{}", szPreviousObject.c_str() ) );
		}
	}
	else
	{
		SetDlgItemText( IDC_PC_DBL_PREV_SEL_LABEL_RIGHT, "" );
	}
}


IView* CPCDBLinkDialog::GetView()
{ 
	return checked_cast<IView*>( &tree );
}


ICommandHandler* CPCDBLinkDialog::GetCommandHandler()
{
	return checked_cast<ICommandHandler*>( &tree );
}


void CPCDBLinkDialog::CreateTree()
{
	DebugTrace( "CPCDBLinkDialog::CreateTree()" );
	if ( ::IsWindow( m_hWnd ) )
	{
		bCreateControls = true;
		tree.CreateTree( TVI_ROOT, true, true );
		bCreateControls = false;
	}
}


void CPCDBLinkDialog::UpdateValues()
{
	DebugTrace( "CPCDBLinkDialog::UpdateValues()" );
	if ( ::IsWindow( m_hWnd ) )
	{
		bCreateControls = true;
		tree.CreateTree( TVI_ROOT, false, true );
		bCreateControls = false;
	}
}


void CPCDBLinkDialog::UpdateOK()
{
	if ( CWnd *pwndControl = GetDlgItem( IDOK ) )
	{
		pwndControl->EnableWindow( ( !szCurrentObject.empty() ) && ( szCurrentObject[szCurrentObject.size() - 1] != PATH_SEPARATOR_CHAR ) );
	}
}


// basement storage  


