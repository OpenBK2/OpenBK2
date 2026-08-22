#include "StdAfx.h"
#include "resource.h"
#include "MainFrm.h"

#include "ELK_Types.h"

#include "ImportFromGameDialog.h"
#include "ImportFromPAKDialog.h"
#include "ImportFromXLSDialog.h"
#include "ProgressDialog.h"
#include "StatisticDialog.h"
#include "CreateFilterDialog.h"

#include "System/FileUtils.h"
#include "Misc/StrProc.h"
#include "MapEditorLib/StringManager.h"

#include <HtmlHelp.h>

#include <cstdint>

#pragma warning( disable : 4786 )


bool CMainFrame::CheckGameApp( LPCSTR pszMainClass, LPCSTR pszMainTitle )
{
	HWND hwndFind;
	hwndFind = ::FindWindow( pszMainClass, pszMainTitle );
  if ( hwndFind )
  {
    return false;
  }
  return true;
}


void CMainFrame::AddToRecentList( const string &rszELKFileName )
{
	string szELKFileName = rszELKFileName;
	NStr::ToLower( &szELKFileName );
	int nRecentCount = 0;
	for ( list<string>::iterator itRecent = params.recentList.begin(); itRecent != params.recentList.end(); )
	{
		string szRecentName = ( *itRecent );
		NStr::ToLower( &szRecentName );
		if ( szRecentName == szELKFileName )
		{
			itRecent = params.recentList.erase( itRecent );
		}
		else
		{
			++itRecent;
			++nRecentCount;
		}
	}
	params.recentList.push_front( rszELKFileName );
	++nRecentCount;
	if ( nRecentCount > 10 )
	{
		params.recentList.pop_back();
	}
	UpdateRecentList();
}

void CMainFrame::RemoveFromRecentList( const string &rszELKFileName )
{
	string szELKFileName = rszELKFileName;
	NStr::ToLower( &szELKFileName );
	int nRecentCount = 0;
	for ( list<string>::iterator itRecent = params.recentList.begin(); itRecent != params.recentList.end(); )
	{
		string szRecentName = ( *itRecent );
		NStr::ToLower( &szRecentName );
		if ( szRecentName == szELKFileName )
		{
			itRecent = params.recentList.erase( itRecent );
		}
		else
		{
			++itRecent;
			++nRecentCount;
		}
	}
	UpdateRecentList();
}


void CMainFrame::UpdateRecentList()
{
}


IMPLEMENT_DYNAMIC(CMainFrame, SECFrameWnd)


int wmAppToolBarWndNotify = RegisterWindowMessage( _T( "WM_SECTOOLBARWNDNOTIFY" ) );
static unsigned WM_FINDREPLACE = ::RegisterWindowMessage( FINDMSGSTRING );


BEGIN_MESSAGE_MAP(CMainFrame, SECFrameWnd)
	ON_WM_CREATE()
	ON_COMMAND(ID_TOOLS_CUSTOMIZE, OnToolsCustomize)
	ON_WM_CLOSE()
	ON_COMMAND(ID_VIEW_STATISTIC, OnViewStatistic)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STATISTIC, OnUpdateViewStatistic)
	ON_COMMAND(ID_VIEW_TREE_STATISTIC, OnViewTreeStatistic)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TREE_STATISTIC, OnUpdateViewTreeStatistic)
	ON_COMMAND(ID_VIEW_TREE, OnViewTree)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TREE, OnUpdateViewTree)
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_IMPORT_FROM_GAME, OnImportFromGame)
	ON_UPDATE_COMMAND_UI(ID_IMPORT_FROM_GAME, OnUpdateImportFromGame)
	ON_COMMAND(ID_IMPORT_FROM_PAK, OnImportFromPak)
	ON_UPDATE_COMMAND_UI(ID_IMPORT_FROM_PAK, OnUpdateImportFromPak)
	ON_UPDATE_COMMAND_UI(ID_RECENT_ELK_0, OnUpdateRecentElk)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPEN, OnUpdateFileOpen)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSE, OnUpdateFileClose)
	ON_COMMAND(ID_FILE_CLOSE, OnFileClose)
	ON_COMMAND(ID_EXPORT_TO_EXCEL, OnExportToExcel)
	ON_UPDATE_COMMAND_UI(ID_EXPORT_TO_EXCEL, OnUpdateExportToExcel)
	ON_UPDATE_COMMAND_UI(IDC_IV_BACK_PAGE_BUTTON, OnUpdateBackPageButton)
	ON_COMMAND(IDC_IV_BACK_PAGE_BUTTON, OnBackPageButton)
	ON_COMMAND(IDC_IV_NEXT_PAGE_BUTTON, OnNextPageButton)
	ON_UPDATE_COMMAND_UI(IDC_IV_NEXT_PAGE_BUTTON, OnUpdateNextPageButton)
	ON_UPDATE_COMMAND_UI(IDC_IV_NEXT_BUTTON, OnUpdateNextButton)
	ON_COMMAND(IDC_IV_NEXT_BUTTON, OnNextButton)
	ON_COMMAND(IDC_IV_BACK_BUTTON, OnBackButton)
	ON_UPDATE_COMMAND_UI(IDC_IV_BACK_BUTTON, OnUpdateBackButton)
	ON_COMMAND(ID_IMPORT_FROM_EXCEL, OnImportFromExcel)
	ON_UPDATE_COMMAND_UI(ID_IMPORT_FROM_EXCEL, OnUpdateImportFromExcel)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_UPDATE_COMMAND_UI(ID_EXPORT_TO_PACK, OnUpdateExportToPack)
	ON_COMMAND(ID_EXPORT_TO_PACK, OnExportToPack)
	ON_UPDATE_COMMAND_UI(ID_EXPORT_TO_PACK_BY_FILTER, OnUpdateExportToPackByFilter)
	ON_COMMAND(ID_EXPORT_TO_PACK_BY_FILTER, OnExportToPackByFilter)
	ON_COMMAND(IDC_BROWSE_COLLAPSE_ITEM, OnBrowseCollapseItem)
	ON_UPDATE_COMMAND_UI(IDC_BROWSE_COLLAPSE_ITEM, OnUpdateBrowseCollapseItem)
	ON_COMMAND(IDC_BROWSE_SKIP_OPTIONS, OnBrowseSkipOptions)
	ON_UPDATE_COMMAND_UI(IDC_BROWSE_SKIP_OPTIONS, OnUpdateBrowseSkipOptions)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SELECT_ALL, OnUpdateEditSelectAll)
	ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CLEAR, OnUpdateEditClear)
	ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
	ON_UPDATE_COMMAND_UI(ID_EDIT_FIND, OnUpdateEditFind)
	ON_COMMAND(ID_TOOLS_RUN_GAME, OnToolsRunGame)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_RUN_GAME, OnUpdateToolsRunGame)
	ON_COMMAND(IDCLOSE, OnCloseButton)
	ON_UPDATE_COMMAND_UI(IDCLOSE, OnUpdateClose)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_SPELLING, OnUpdateToolsSpelling)
	ON_COMMAND(ID_TOOLS_SPELLING, OnToolsSpelling)
	ON_UPDATE_COMMAND_UI(ID_HELP_CONTENTS, OnUpdateHelpContents)
	ON_COMMAND(ID_HELP_CONTENTS, OnHelpContents)
	ON_COMMAND(ID_FILE_DELETE, OnFileDelete)
	ON_UPDATE_COMMAND_UI(ID_FILE_DELETE, OnUpdateFileDelete)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_CHOOSE_FONS, OnUpdateToolsChooseFons)
	ON_COMMAND_RANGE(ID_RECENT_ELK_0, ID_RECENT_ELK_9, OnRecentElk)
	ON_UPDATE_COMMAND_UI_RANGE(ID_RECENT_ELK_0, ID_RECENT_ELK_9, OnUpdateRecentElkRange)
	ON_CBN_SELCHANGE( IDC_IV_FILTER_COMBO_BOX, OnChangeFilter )
	ON_REGISTERED_MESSAGE(wmAppToolBarWndNotify, OnCreateCombo)
  ON_REGISTERED_MESSAGE( WM_FINDREPLACE, OnFindReplace)
END_MESSAGE_MAP()

//	ON_COMMAND_RANGE(ID_RECENT_ELK_0, ID_RECENT_ELK_9, OnRecentElk)
//	ON_UPDATE_COMMAND_UI_RANGE(ID_RECENT_ELK_0, ID_RECENT_ELK_9, OnUpdateRecentElkRange)


static unsigned INDICATORS[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


static unsigned BASED_CODE FILE_BUTTONS[] =
{
	ID_FILE_OPEN,
	ID_FILE_SAVE,
	ID_FILE_DELETE,
	ID_FILE_CLOSE,
	ID_TOOLS_RUN_GAME,
	ID_SEPARATOR,
	ID_HELP_CONTENTS,
};


static unsigned BASED_CODE EDIT_BUTTONS[] =
{
	ID_EDIT_CUT,
	ID_EDIT_COPY,
	ID_EDIT_PASTE,
	ID_EDIT_CLEAR,
	ID_SEPARATOR,
	ID_EDIT_UNDO,
	ID_SEPARATOR,
	ID_EDIT_FIND,
	ID_SEPARATOR,
	ID_TOOLS_SPELLING,
};


static unsigned BASED_CODE  BROWSE_BUTTONS[] =
{
	IDC_BROWSE_COLLAPSE_ITEM,
	ID_SEPARATOR,
	IDC_IV_FILTER_COMBO_BOX,
	IDC_IV_BACK_PAGE_BUTTON,
	IDC_IV_BACK_BUTTON,
	IDC_IV_NEXT_BUTTON,
	IDC_IV_NEXT_PAGE_BUTTON,
	ID_SEPARATOR,
	IDC_BROWSE_SKIP_OPTIONS,
};


static unsigned BASED_CODE VIEW_BUTTONS[] =
{
	ID_VIEW_TREE,
	ID_VIEW_STATISTIC,
};


static unsigned BASED_CODE SHORT_BUTTONS[] =
{
	ID_TOOLS_RUN_GAME,
	ID_EXPORT_TO_PACK,
	ID_SEPARATOR,
	IDC_IV_FILTER_COMBO_BOX,
	IDC_IV_BACK_BUTTON,
	IDC_IV_NEXT_BUTTON,
	ID_SEPARATOR,
	ID_EXPORT_TO_EXCEL,
	ID_IMPORT_FROM_EXCEL,
	ID_SEPARATOR,
	ID_EDIT_FIND,
	ID_TOOLS_SPELLING,
	ID_SEPARATOR,
	ID_VIEW_TREE,
	ID_VIEW_STATISTIC,
	ID_SEPARATOR,
	ID_APP_EXIT,
};


BEGIN_BUTTON_MAP(BUTTON_MAP)
	STD_BUTTON( IDC_BROWSE_COLLAPSE_ITEM, TBBS_CHECKBOX )
	STD_BUTTON( ID_VIEW_TREE, TBBS_CHECKBOX )
	COMBO_BUTTON( IDC_IV_FILTER_COMBO_BOX, IDC_IV_FILTER_COMBO_BOX, 0, CBS_DROPDOWNLIST | CBS_SORT, 150, 40, 150 )
END_BUTTON_MAP()


BEGIN_BUTTON_MAP(SHORT_BUTTON_MAP)
	TEXT_BUTTON( ID_TOOLS_RUN_GAME, IDS_TOOLS_RUN_GAME )
	TEXT_BUTTON( ID_EXPORT_TO_PACK, IDS_EXPORT_TO_PACK )
	TEXT_BUTTON( ID_APP_EXIT, IDS_APP_EXIT )
	TEXT_BUTTON( IDC_IV_BACK_BUTTON, IDS_IV_BACK_BUTTON )
	TEXT_BUTTON( IDC_IV_NEXT_BUTTON, IDS_IV_NEXT_BUTTONT )
	TEXT_BUTTON( ID_EXPORT_TO_EXCEL, IDS_EXPORT_TO_EXCEL )
	TEXT_BUTTON( ID_IMPORT_FROM_EXCEL, IDS_IMPORT_FROM_EXCEL )
	TEXT_BUTTON( ID_EDIT_FIND, IDS_EDIT_FIND )
	TEXT_BUTTON( ID_TOOLS_SPELLING, IDS_TOOLS_SPELLING )
	TEXT_BUTTON_EX( ID_VIEW_TREE, IDS_VIEW_TREE, TBBS_CHECKBOX )
	TEXT_BUTTON( ID_VIEW_STATISTIC, IDS_VIEW_STATISTIC )
	COMBO_BUTTON( IDC_IV_FILTER_COMBO_BOX, IDC_IV_FILTER_COMBO_BOX, 0, CBS_DROPDOWNLIST | CBS_SORT, 150, 40, 150 )
END_BUTTON_MAP()


CMainFrame::CMainFrame() : bGameExists( false ), bShortApperence( false ), pDefButtonGroup( 0 ), nDefButtonCount( 0 ), hIcon( 0 ), pwndFiltersComboBox( 0 ), pwndFindReplaceDialog( 0 )
{
	m_pControlBarManager = new SECToolBarManager( this );	// this is a base class member
	m_pMenuBar = new SECMenuBar;	// this is a base class member
	EnableBmpMenus();
}


CMainFrame::~CMainFrame()
{
	if ( m_pControlBarManager )
	{
		delete m_pControlBarManager;
	}
	if ( pDefButtonGroup )
	{
		delete [] pDefButtonGroup;
	}
	if ( m_pMenuBar )
	{
		delete m_pMenuBar;
	}
}


int CMainFrame::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
	string szRegistryKey;
	{
		CString strPath;
		CString strProgramKey;
		CString strKey;
		strPath.LoadString( IDS_REGISTRY_PATH );
		strProgramKey.LoadString( AFX_IDS_APP_TITLE );
		strKey.LoadString( IDS_REGISTRY_KEY );
		szRegistryKey = StrFmt( _T( "Software\\%s\\%s\\%s" ), LPCTSTR( strPath ), LPCTSTR( strProgramKey ), LPCTSTR( strKey ) );
	}
	params.LoadFromRegistry( szRegistryKey, bShortApperence );

  hIcon = AfxGetApp()->LoadIcon( IDR_MAINFRAME );
  SetIcon( hIcon, true );
  SetIcon( hIcon, false );

	if ( SECFrameWnd::OnCreate( lpCreateStruct ) == -1 )
	{
		return -1;
	}
	
	//создаем толбары
	SECToolBarManager* pToolBarManager = dynamic_cast<SECToolBarManager*>( m_pControlBarManager );	
	if ( bShortApperence )
	{
		if ( !pToolBarManager->LoadToolBarResource( MAKEINTRESOURCE( IDT_SHORT_TOOLBAR ), MAKEINTRESOURCE( IDT_SHORT_TOOLBARLARGE ) ) )
		{
			return -1;
		}
		CString strToolbarName;
		strToolbarName.LoadString( IDS_SHORT_TOOLBAR_NAME );
		pToolBarManager->DefineDefaultToolBar( AFX_IDW_TOOLBAR + 9, 
																					 strToolbarName,
																					 NUMELEMENTS( SHORT_BUTTONS ),
																					 SHORT_BUTTONS,
																					 CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM,
																					 AFX_IDW_DOCKBAR_TOP );

		// Initialize menubar support
		pToolBarManager->SetMenuInfo( 1, IDR_SHORT_MAINFRAME );
		pToolBarManager->SetButtonMap( SHORT_BUTTON_MAP );
		pToolBarManager->EnableLargeBtns( true );
	}
	else
	{
		if ( !pToolBarManager->LoadToolBarResource( MAKEINTRESOURCE( IDT_TOOLBAR ), MAKEINTRESOURCE( IDT_TOOLBARLARGE ) ) )
		{
			return -1;
		}
		CString strToolbarName;
		strToolbarName.LoadString( IDS_FILE_TOOLBAR_NAME );
		pToolBarManager->DefineDefaultToolBar( AFX_IDW_TOOLBAR + 5, 
																					 strToolbarName,
																					 NUMELEMENTS( FILE_BUTTONS ),
																					 FILE_BUTTONS,
																					 CBRS_ALIGN_ANY,
																					 AFX_IDW_DOCKBAR_TOP );

		strToolbarName.LoadString( IDS_EDIT_TOOLBAR_NAME );
		pToolBarManager->DefineDefaultToolBar( AFX_IDW_TOOLBAR + 6, 
																					 strToolbarName,
																					 NUMELEMENTS( EDIT_BUTTONS ),
																					 EDIT_BUTTONS,
																					 CBRS_ALIGN_ANY,
																					 AFX_IDW_DOCKBAR_TOP,
																					 AFX_IDW_TOOLBAR + 5,
																					 true,
																					 true );
		
		strToolbarName.LoadString( IDS_BROWSE_TOOLBAR_NAME );
		pToolBarManager->DefineDefaultToolBar( AFX_IDW_TOOLBAR + 7,
																					 strToolbarName,
																					 NUMELEMENTS( BROWSE_BUTTONS ),
																					 BROWSE_BUTTONS,
																					 CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM,
																					 AFX_IDW_DOCKBAR_TOP,
																					 AFX_IDW_TOOLBAR + 6,
																					 true,
																					 true );

		strToolbarName.LoadString( IDS_VIEW_TOOLBAR_NAME );
		pToolBarManager->DefineDefaultToolBar( AFX_IDW_TOOLBAR + 8, 
																					 strToolbarName,
																					 NUMELEMENTS( VIEW_BUTTONS ),
																					 VIEW_BUTTONS,
																					 CBRS_ALIGN_ANY,
																					 AFX_IDW_DOCKBAR_TOP,
																					 AFX_IDW_TOOLBAR + 7,
																					 true,
																					 true );
		// Initialize menubar support
		pToolBarManager->SetMenuInfo( 1, IDR_MAINFRAME );
		pToolBarManager->SetButtonMap( BUTTON_MAP );
	}

	//create status bar
	if ( !wndStatusBar.Create( this ) || !wndStatusBar.SetIndicators( INDICATORS, sizeof( INDICATORS ) / sizeof( unsigned ) ) )
	{
		return -1;
	}

	// enable cool look
	pToolBarManager->EnableCoolLook( true );

	EnableDocking( CBRS_ALIGN_ANY );

	// create a view to occupy the client area of the frame
	wndInputView.bGameExists = bGameExists;
	if ( !wndInputView.Create( 0, 0, AFX_WS_DEFAULT_VIEW, CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, 0 ) )
	{
		return -1;
	}

	// Dockable window
	uint32_t dwStyle = WS_CHILD | WS_VISIBLE | CBRS_RIGHT | CBRS_TOOLTIPS | CBRS_SIZE_DYNAMIC;
	uint32_t dwStyleEx = CBRS_EX_COOL | CBRS_EX_BORDERSPACE;
	unsigned nID = SECControlBar::GetUniqueBarID( this, 100 );
	CString strDockingWindowTitle;
	strDockingWindowTitle.LoadString( IDS_ELK_TREE_WINDOW_TITLE );
	if ( !wndBaseTree.Create( this, strDockingWindowTitle, dwStyle, dwStyleEx, nID ) )
	{
		return -1;
	}

	// TODO: select where you want this control bar to dock
	wndBaseTree.EnableDocking( CBRS_ALIGN_ANY );
	DockControlBarEx( &wndBaseTree, AFX_IDW_DOCKBAR_LEFT, 0, 0, 1.0f, 300 );
	
	wndInputView.SetMainFrameWindow( this );
	wndBaseTree.SetMainFrameWindow( this );

	CloseELK();
	params.LoadFromRegistry( szRegistryKey, bShortApperence );

	CString strRegistryKeyName;
	if ( bShortApperence )
	{
		strRegistryKeyName.LoadString( IDS_SHORT_TOOLBAR_KEY );
		pToolBarManager->LoadState( _T( "" ) );//strRegistryKeyName );
		strRegistryKeyName.LoadString( IDS_SHORT_WINDOWBAR_KEY );
		LoadBarState( _T( "" ) );//strRegistryKeyName );
	}
	else
	{
		strRegistryKeyName.LoadString( IDS_TOOLBAR_KEY );
		pToolBarManager->LoadState( strRegistryKeyName );
		
		strRegistryKeyName.LoadString( IDS_WINDOWBAR_KEY );
		LoadBarState( strRegistryKeyName );
	}
	
	wndBaseTree.SetCollapseItem( params.bCollapseItem );
	wndBaseTree.SetFormWindow( &( wndInputView.wndForm ) );
	
	//ShowWindow( SW_SHOW );
	
	if ( bShortApperence )
	{
		OpenLastELK();
	}
	else
	{
		if ( !params.szLastOpenedELKName.empty() )
		{
			OpenELK( params.szLastOpenedELKName );
		}
	}
	return 0;
}


BOOL CMainFrame::PreCreateWindow( CREATESTRUCT& cs )
{
	if( !SECFrameWnd::PreCreateWindow( cs ) )
	{
		return false;
	}

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass( 0 );
	return true;
}


void CMainFrame::OnToolsCustomize() 
{
	SECToolBarsPage toolbarPage;
	toolbarPage.SetManager( dynamic_cast<SECToolBarManager*>( m_pControlBarManager ) );

	SECToolBarCmdPage cmdPage( SECToolBarCmdPage::IDD, IDS_COMMANDS );
	cmdPage.SetManager( dynamic_cast<SECToolBarManager*>( m_pControlBarManager ) );
	
	CString strToolbarName;
	strToolbarName.LoadString( IDS_FILE_TOOLBAR_NAME );
	cmdPage.DefineBtnGroup( strToolbarName, NUMELEMENTS( FILE_BUTTONS ), FILE_BUTTONS );
	strToolbarName.LoadString( IDS_EDIT_TOOLBAR_NAME );
	cmdPage.DefineBtnGroup( strToolbarName, NUMELEMENTS( EDIT_BUTTONS ), EDIT_BUTTONS );
	strToolbarName.LoadString( IDS_BROWSE_TOOLBAR_NAME );
	cmdPage.DefineBtnGroup( strToolbarName, NUMELEMENTS( BROWSE_BUTTONS ), BROWSE_BUTTONS );
	strToolbarName.LoadString( IDS_VIEW_TOOLBAR_NAME );
	cmdPage.DefineBtnGroup( strToolbarName, NUMELEMENTS( VIEW_BUTTONS ), VIEW_BUTTONS );
	strToolbarName.LoadString( IDS_MENU_TOOLBAR_NAME );
	cmdPage.DefineMenuGroup( strToolbarName );

	SECToolBarSheet toolBarSheet;
	toolBarSheet.AddPage( &toolbarPage );
	toolBarSheet.AddPage( &cmdPage );

	toolBarSheet.DoModal();
}


void CMainFrame::OnClose() 
{
	string szLastOpenedELKName = params.szLastOpenedELKName;
	CloseELK();
	params.szLastOpenedELKName = szLastOpenedELKName;

	WINDOWPLACEMENT windowPlacement;
	GetWindowPlacement( &windowPlacement );
	params.bFullScreen = ( windowPlacement.showCmd == SW_SHOWMAXIMIZED );
	params.rect = CTRect<int>( windowPlacement.rcNormalPosition.left,
														 windowPlacement.rcNormalPosition.top,
														 windowPlacement.rcNormalPosition.right,
														 windowPlacement.rcNormalPosition.bottom );

	CString strRegistryKeyName;
	if ( bShortApperence )
	{
		if ( SECToolBarManager* pToolBarManager = dynamic_cast<SECToolBarManager*>( m_pControlBarManager ) )
		{
			strRegistryKeyName.LoadString( IDS_SHORT_TOOLBAR_KEY );
			pToolBarManager->SaveState( strRegistryKeyName );
		}
		strRegistryKeyName.LoadString( IDS_SHORT_WINDOWBAR_KEY );
		SaveBarState( strRegistryKeyName );
	}
	else
	{
		if ( SECToolBarManager* pToolBarManager = dynamic_cast<SECToolBarManager*>( m_pControlBarManager ) )
		{
			strRegistryKeyName.LoadString( IDS_TOOLBAR_KEY );
			pToolBarManager->SaveState( strRegistryKeyName );
		}
		strRegistryKeyName.LoadString( IDS_WINDOWBAR_KEY );
		SaveBarState( strRegistryKeyName );
	}

	CString strPath;
	CString strProgramKey;
	CString strKey;
	strPath.LoadString( IDS_REGISTRY_PATH );
	strProgramKey.LoadString( AFX_IDS_APP_TITLE );
	strKey.LoadString( IDS_REGISTRY_KEY );
	string szRegistryKey = StrFmt( _T( "Software\\%s\\%s\\%s" ), LPCTSTR( strPath ), LPCTSTR( strProgramKey ), LPCTSTR( strKey ) );
	params.SaveToRegistry( szRegistryKey, bShortApperence );
	
	SECFrameWnd::OnClose();
}


void CMainFrame::OnViewStatistic()
{
	if ( elk.IsOpened() )
	{
		CProgressDialog progressDialog;
		progressDialog.Create( IDD_PROGRESS, this );
		CELK::CreateStatistic( &( elk.statistic ), &wndBaseTree, string(), params.nCodePage, &progressDialog );
		if ( progressDialog.GetSafeHwnd() != 0 )
		{
			progressDialog.DestroyWindow();
		}
		CStatisticDialog statisticDialog;
		statisticDialog.pELK = &elk;
		if ( statisticDialog.DoModal() == IDOK )
		{
		}
	}
}


void CMainFrame::OnUpdateViewStatistic( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( elk.IsOpened() );
}


void CMainFrame::OnViewTreeStatistic()
{
	string szPath;
	wndBaseTree.GetItemPath( &szPath, true );
	if ( elk.IsOpened() && ( !szPath.empty() ) )
	{
		CProgressDialog progressDialog;
		progressDialog.Create( IDD_PROGRESS, this );
		//
		CELK::CreateStatistic( &( elk.statistic ), &wndBaseTree, szPath, params.nCodePage, &progressDialog );
		//
		if ( progressDialog.GetSafeHwnd() != 0 )
		{
			progressDialog.DestroyWindow();
		}
		CStatisticDialog statisticDialog;
		statisticDialog.pELK = &elk;
		if ( statisticDialog.DoModal() == IDOK )
		{
		}
	}
}


void CMainFrame::OnUpdateViewTreeStatistic( CCmdUI* pCmdUI ) 
{
	string szPath;
	wndBaseTree.GetItemPath( &szPath, true );
	//
	pCmdUI->Enable( elk.IsOpened() && ( !szPath.empty() ) );
}


void CMainFrame::OnViewTree() 
{
	ShowControlBar( &wndBaseTree, !wndBaseTree.IsVisible(), true );
}


void CMainFrame::OnUpdateViewTree( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( true );
	pCmdUI->SetCheck( wndBaseTree.IsVisible() );	
}


void CMainFrame::OnSetFocus(CWnd* pOldWnd)
{
	wndInputView.SetFocus();
}


void CMainFrame::OnImportFromGame() 
{
	CImportFromGameDialog dialog( this );
	if ( dialog.DoModal() == IDOK )
	{
		string szGamePath;
		string szFilePath;
		dialog.GetGamePath( &szGamePath );
		dialog.GetFilePath( &szFilePath );
		params.ValidatePath( &szGamePath, true );
		params.ValidatePath( &szFilePath, false );

		CProgressDialog progressDialog;
		progressDialog.Create( IDD_PROGRESS, this );
		CELK::CreatePAK( szGamePath, szFilePath, params.szZIPToolPath, &progressDialog );
		if ( progressDialog.GetSafeHwnd() != 0 )
		{
			progressDialog.DestroyWindow();
		}
	}
}


void CMainFrame::OnUpdateImportFromGame( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( true );
}


void CMainFrame::OnUpdateExportToPack( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( elk.IsOpened() && ( wndBaseTree.wndTree.GetSelectedItem() != 0 ) );
}


void CMainFrame::OnExportToPack() 
{
	OnExportToPAKInternal( false );
}


void CMainFrame::OnUpdateExportToPackByFilter( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( elk.IsOpened() && ( wndBaseTree.wndTree.GetSelectedItem() != 0 ) );
}


void CMainFrame::OnExportToPackByFilter() 
{
	OnExportToPAKInternal( true );
}


void CMainFrame::OnExportToPAKInternal( bool bUseFilter )
{
	if ( elk.IsOpened() )
	{
		OnFileSave();

		CString strDialogTitle;
		CString strFileName;
		CString strFolderName;

		if ( bShortApperence )
		{
			CString strProgramTitle;
			strProgramTitle.LoadString( AFX_IDS_APP_TITLE );

			CString strMessage = _T( "PAK Created:\n" );

			CProgressDialog progressDialog;
			progressDialog.Create( IDD_PROGRESS, this );
			string szELKFolder = elk.szPath.substr( 0, elk.szPath.rfind( '\\' ) + 1 );

			for ( int nElementIndex = 0; nElementIndex < elk.elementList.size(); ++nElementIndex )
			{
				string szPAKName = elk.elementList[nElementIndex].description.szPAKName;
				if ( szPAKName.empty() )
				{
					szPAKName = elk.elementList[nElementIndex].szPath;
					szPAKName = szPAKName.substr( szPAKName.rfind( '\\' ) + 1 );
					szPAKName += string( CELK::PAK_EXTENTION );
				}
				CELK::ExportToPAK( elk.elementList[nElementIndex].szPath,
													 szELKFolder + szPAKName,
													 params.szZIPToolPath,
													 &wndBaseTree,
													 true,
													 elk.elementList[nElementIndex].description.bGenerateFonts,
													 elk.elementList[nElementIndex].description.bUsedChars,
													 params.nCodePage,
													 &progressDialog,
													 0 );
				strMessage += ( szELKFolder + szPAKName + _T( "\n" ) ).c_str();
			}
			if ( progressDialog.GetSafeHwnd() != 0 )
			{
				progressDialog.DestroyWindow();
			}
			//strMessage += _T( "Please, send PAK files to developers." );
			::MessageBox( GetSafeHwnd(), strMessage, strProgramTitle, MB_OK | MB_ICONINFORMATION );
		}
		else
		{
			strDialogTitle.LoadString( IDS_EXPORT_TO_PAK_DIALOG_TITLE );

			strFileName = params.szLastOpenedPAKName.c_str();
			int nSlashPos = strFileName.ReverseFind( '\\' );
			if ( nSlashPos >= 0 )
			{
				strFolderName = strFileName.Left( nSlashPos );
			}

			CFileDialog fileDialog( false, _T( ".pak" ), strFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T( "PAK files (*.pak)|*.pak|All Files (*.*)|*.*||" ), this );
			fileDialog.m_ofn.lpstrTitle = strDialogTitle;
			fileDialog.m_ofn.lpstrInitialDir = strFolderName;
			
			if ( fileDialog.DoModal() == IDOK )
			{
				strFileName = fileDialog.GetPathName();
				CStringManager::ExtendFileExtention( &strFileName, _T( ".pak" ) );
				params.szLastOpenedPAKName = strFileName;
				CProgressDialog progressDialog;
				progressDialog.Create( IDD_PROGRESS, this );

				int nELKElementNumber = wndBaseTree.GetELKElementNumber();
				if ( nELKElementNumber < 0 )
				{
					nELKElementNumber = 0;
				}
				const SELKElement &rELKElement = elk.elementList[nELKElementNumber];
				CELK::ExportToPAK( rELKElement.szPath,
													 params.szLastOpenedPAKName,
													 params.szZIPToolPath,
													 &wndBaseTree,
													 true,
													 rELKElement.description.bGenerateFonts,
													 rELKElement.description.bUsedChars,
													 params.nCodePage,
													 &progressDialog,
													 bUseFilter ? params.GetCurrentFilter() : 0 );
				if ( progressDialog.GetSafeHwnd() != 0 )
				{
					progressDialog.DestroyWindow();
				}
			}	
		}
	}
}


void CMainFrame::OnImportFromPak() 
{
	CImportFromPAKDialog dialog( this );
	if ( dialog.DoModal() == IDOK )
	{
		string szPAKPath;
		string szELKPath;
		dialog.GetPAKPath( &szPAKPath );
		dialog.GetFilePath( &szELKPath );
		params.ValidatePath( &szPAKPath, false );
		params.ValidatePath( &szELKPath, false );

		string szLastELKPath;
		if ( elk.IsOpened() )
		{
			szLastELKPath = elk.szPath;
			NStr::ToLower( &szLastELKPath );
			if ( szLastELKPath == szELKPath )
			{
				CloseELK();
			}
			else
			{
				szLastELKPath.clear();
			}
		}

		CProgressDialog progressDialog;
		progressDialog.Create( IDD_PROGRESS, this );
		bool bUpdated = CELK::UpdateELK( szELKPath, szPAKPath, &progressDialog );
		if ( progressDialog.GetSafeHwnd() != 0 )
		{
			progressDialog.DestroyWindow();
		}
		if ( !bUpdated )
		{
			CString strProgramTitle;
			strProgramTitle.LoadString( AFX_IDS_APP_TITLE );
			
			string szMessage = StrFmt( _T( "Can't find PAK or UPD file: %s." ), szPAKPath.c_str() );
			::MessageBox( GetSafeHwnd(), szMessage.c_str(), strProgramTitle, MB_OK | MB_ICONSTOP );
		}
		if ( !szLastELKPath.empty() )
		{
			OpenELK( szLastELKPath );
		}
	}
}


void CMainFrame::OnUpdateImportFromPak( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( true );
}


void CMainFrame::OnUpdateRecentElk( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( !params.recentList.empty() );
	if ( CMenu *pFileMenu = pCmdUI->m_pMenu )
	{
		if ( CMenu *pRecentMenu = pFileMenu->GetSubMenu( 6 ) )
		{
			while( pRecentMenu->GetMenuItemCount() > 0 )
			{
				pRecentMenu->RemoveMenu( 0 , MF_BYPOSITION );
			}
			int nRecentCount = 0;
			for ( list<string>::const_iterator itRecent = params.recentList.begin(); itRecent != params.recentList.end(); ++itRecent )
			{
				pRecentMenu->InsertMenu( -1, MF_BYPOSITION, ID_RECENT_ELK_0 + nRecentCount, itRecent->c_str() );
				//pRecentMenu->EnableMenuItem( ID_RECENT_ELK_0 + nRecentCount, MF_ENABLED | MF_BYCOMMAND );
				++nRecentCount;
			}
			if ( pRecentMenu->GetMenuItemCount() == 0 )
			{
				CString strMenuLabel;
				strMenuLabel.LoadString( IDS_RECENT_ELK );
				pRecentMenu->InsertMenu( -1, MF_BYPOSITION, ID_RECENT_ELK_0, strMenuLabel );
				//pRecentMenu->EnableMenuItem( ID_RECENT_ELK_0, MF_GRAYED | MF_BYCOMMAND );
			}
		}
	}
}


void CMainFrame::OnUpdateRecentElkRange( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( true );
}


void CMainFrame::OnUpdateFileOpen( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( true );
}


void CMainFrame::OnFileOpen() 
{
	CString strDialogTitle;
	CString strFileName;
	CString strFolderName;

	strDialogTitle.LoadString( IDS_OPEN_ELK_DIALOG_TITLE );

	strFileName = params.szLastOpenedELKName.c_str();
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
		CloseELK();
		OpenELK( string( strFileName ) );
	}	
}


void CMainFrame::OnRecentElk( unsigned nID ) 
{
	int nRecentCount = 0;
	for ( list<string>::const_iterator itRecent = params.recentList.begin(); itRecent != params.recentList.end(); ++itRecent )
	{
		if ( nRecentCount == ( nID - ID_RECENT_ELK_0 ) )
		{
			CloseELK();
			string szELKFileName = ( *itRecent );
			OpenELK( szELKFileName );
			return;
		}
		++nRecentCount;
	}
}


bool CMainFrame::CloseELK()
{
	if ( elk.IsOpened() )
	{
		OnFileSave();

		wndBaseTree.GetItemPath( &( params.szLastPath ), false );
		params.nLastELKElement = wndBaseTree.GetELKElementNumber();
	}

	wndBaseTree.ClearTree();
	wndInputView.ClearControls();
	
	elk.Close();
	params.szLastOpenedELKName.clear();
	params.szPreviousPath.clear();
	spellChecker.nCharIndex = 0;
	CString strProgramTitle;
  strProgramTitle.LoadString( AFX_IDS_APP_TITLE );
	
	string szTitle = StrFmt( _T( " %s" ), LPCTSTR( strProgramTitle ) );

	SetWindowText( szTitle.c_str() );
	return true;
}


bool CMainFrame::OpenELK( const string &rszELKFileName )
{
	if ( !NFile::DoesFileExist( rszELKFileName.c_str() ) )
	{
		CString strProgramTitle;
		strProgramTitle.LoadString( AFX_IDS_APP_TITLE );

		string szMessage = StrFmt( _T( "Can't find file: %s." ), rszELKFileName.c_str() );
		::MessageBox( GetSafeHwnd(), szMessage.c_str(), strProgramTitle, MB_OK | MB_ICONSTOP );
		
		RemoveFromRecentList( rszELKFileName );
		params.szLastOpenedELKName.clear();
		return false;
	}

	CProgressDialog progressDialog;
	progressDialog.Create( IDD_PROGRESS, this );
	if ( progressDialog.GetSafeHwnd() != 0 )
	{
		progressDialog.ShowWindow( SW_SHOW );
		progressDialog.SetProgressMessage( _T( "Getting file structure..." ) );	
		progressDialog.SetWindowText( StrFmt( _T( "Open ELK: %s" ), rszELKFileName.c_str() ) );
	}
	AddToRecentList( rszELKFileName );
	elk.Open( rszELKFileName, true );

  CString strProgramTitle;
  strProgramTitle.LoadString( AFX_IDS_APP_TITLE );
	string szTitle = StrFmt( _T( " ELK - %s" ), LPCTSTR( strProgramTitle ) );
	SetWindowText( szTitle.c_str() );
	
	wndBaseTree.FillTree( elk, params.szLastPath, params.nLastELKElement, &progressDialog );
	//OnChangeFilter();
	if ( progressDialog.GetSafeHwnd() != 0 )
	{
		progressDialog.DestroyWindow();
	}
	params.szLastOpenedELKName = rszELKFileName;
	return true;
}


bool CMainFrame::OpenLastELK()
{
	params.szLastOpenedELKName = params.szCurrentFolder + CELK::ELK_FILE_NAME + _T( ".xml" );
	CELK selectedELK;
	selectedELK.Open( params.szLastOpenedELKName, false );

	for ( int nElementIndex = 0; nElementIndex < selectedELK.elementList.size(); ++nElementIndex )
	{
		string szDataBaseFolder;
		selectedELK.elementList[nElementIndex].GetDataBaseFolder( &szDataBaseFolder );
		if ( !NFile::DoesFolderExist( szDataBaseFolder.c_str() ) ) 
		{
			::DeleteFile( params.szLastOpenedELKName.c_str() );
			break;
		}
	}
	
	CProgressDialog progressDialog;
	progressDialog.Create( IDD_PROGRESS, this );
	bool bUpdated = CELK::UpdateELK( params.szLastOpenedELKName, "", &progressDialog );
	if ( progressDialog.GetSafeHwnd() != 0 )
	{
		progressDialog.DestroyWindow();
	}
	if ( !bUpdated )
	{
		CString strProgramTitle;
		strProgramTitle.LoadString( AFX_IDS_APP_TITLE );
		
		string szMessage = StrFmt( _T( "Can't find any UPD file in ELK work directory: %s. " ), params.szCurrentFolder.c_str() );
		::MessageBox( GetSafeHwnd(), szMessage.c_str(), strProgramTitle, MB_OK | MB_ICONSTOP );
		
		OnClose();
		return false;
	}
	OpenELK( params.szLastOpenedELKName );
	return true;
}


void CMainFrame::OnUpdateFileClose( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( elk.IsOpened() );
}


void CMainFrame::OnFileClose() 
{
	CloseELK();
}


void CMainFrame::OnUpdateFileDelete( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( elk.IsOpened() && ( elk.elementList.size() > 1 ) && ( wndBaseTree.wndTree.GetSelectedItem() != 0 ) );
}


void CMainFrame::OnFileDelete() 
{
	CString strProgramTitle;
	strProgramTitle.LoadString( AFX_IDS_APP_TITLE );
	CString strMessage;
	strMessage.LoadString( IDS_DELETE_TREE_QUASTION );
	if ( ::MessageBox( GetSafeHwnd(), strMessage, strProgramTitle, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 ) == IDYES )
	{
		int nELKElementNumber = wndBaseTree.GetELKElementNumber();
		if ( nELKElementNumber >= 0 && nELKElementNumber < elk.elementList.size() )
		{
			elk.elementList.erase( elk.elementList.begin() + nELKElementNumber );
			string szELKPath = elk.szPath;
			CloseELK();
			OpenELK( szELKPath );
		}
	}
}


LRESULT CMainFrame::WindowProc( unsigned message, WPARAM wParam, LPARAM lParam ) 
{
	if ( message == WM_INPUT_FORM_NOTIFY )
	{
		switch ( wParam )
		{
			case IFN_STATE_CHANGED:
			{
				return OnIFNStateChanged( lParam );
			}
			case IFN_NEXT_BUTTON_PRESED:
			{
				OnNextButton();
				return 0;
			}
			case IFN_BACK_BUTTON_PRESED:
			{
				OnBackButton();
				return 0;
			}
			case IFN_TRANSLATION_CHANGED:
			{
				spellChecker.nCharIndex = 0;
				return 0;
			}
		}
	}
	else if ( message == WM_ELK_TREE_NOTIFY )
	{
		switch ( wParam )
		{
			case ETN_TEXT_SELECTED:
			{
				return OnETNTextSelected( lParam );
			}
			case ETN_FOLDER_SELECTED:
			{
				return OnETNFolderSelected( lParam );
			}
			case ETN_STATE_CHANGED:
			{
				return OnIFNStateChanged( lParam );
			}
			case ETN_SHOW_TREE_STATISTIC:
			{
				OnViewTreeStatistic();
				return true;
			}
		}
	}
	else if ( message == WM_ELK_TRANSLATE_EDIT_NOTIFY )
	{
		switch ( wParam )
		{
			case TEN_NKEYDOWN:
			{
				return OnTENKeyDown( lParam );
			}
		}
	}
	return SECFrameWnd::WindowProc(message, wParam, lParam);
}


int CMainFrame::OnETNTextSelected( int nState )
{
	if ( elk.IsOpened() )
	{
		OnFileSave();

		CString strProgramTitle;
		strProgramTitle.LoadString( AFX_IDS_APP_TITLE );
		
		string szTextName;
		wndBaseTree.GetItemPath( &szTextName, false );
		
		string szTitle = StrFmt( _T( " ELK - %s - [%s]" ), LPCTSTR( strProgramTitle ), szTextName.c_str() );
		SetWindowText( szTitle.c_str() );

		string szPath;
		wndBaseTree.GetItemPath( &szPath, true );

		wndInputView.EnableControlsForText( true );
		wndInputView.SetFocus();

		if ( bGameExists )
		{
			string szGameImagePath;
			if ( !params.szGameFolder.empty() )
			{
				szGameImagePath = params.szGameFolder + CELK::GAME_DATA_FOLDER + szTextName;
				szGameImagePath = szGameImagePath.substr( 0, szGameImagePath.rfind( '\\' ) );
				szGameImagePath += _T( "\\icon.tga" );
				if ( !NFile::DoesFileExist( szGameImagePath.c_str() ) )
				{
					szGameImagePath.clear();
				}
			}
			wndInputView.LoadGameImage( szGameImagePath );
		}

		CString strText;
		CELK::GetOriginalText( szPath, &strText, params.nCodePage, false );
		wndInputView.SetOriginalText( strText );
		
		CELK::GetTranslatedText( szPath, &strText, params.nCodePage, false );
		wndInputView.SetTranslatedText( strText );
		
		CELK::GetDescription( szPath, &strText, params.nCodePage, false );
		wndInputView.SetDescription( strText );
		
		wndInputView.SetState( nState );
		
		spellChecker.nCharIndex = 0;
		
		params.szPreviousPath = szPath;
	}
	return 0;
}


int CMainFrame::OnETNFolderSelected( int nState )
{
	if ( elk.IsOpened() )
	{
		OnFileSave();
		
		CString strProgramTitle;
		strProgramTitle.LoadString( AFX_IDS_APP_TITLE );
		string szTitle = StrFmt( _T( " ELK - %s" ), LPCTSTR( strProgramTitle ) );
		SetWindowText( szTitle.c_str() );

		wndInputView.ClearControls();
		params.szPreviousPath.clear();
		spellChecker.nCharIndex = 0;
		
		//if ( ( nState == SELKTextProperty::STATE_TRANSLATED ) || ( nState == SELKTextProperty::STATE_APPROVED ) )
		{
			wndInputView.EnableControlsForFolder( true );
		}

		if ( bGameExists )
		{
			wndInputView.LoadGameImage( "" );
		}

		string szPath;
		wndBaseTree.GetItemPath( &szPath, true );
		szPath += CELK::FOLDER_DESC_FILE_NAME;

		CString strText;
		CELK::GetDescription( szPath, &strText, false, params.nCodePage );
		wndInputView.SetDescription( strText );

		spellChecker.nCharIndex = 0;

		wndInputView.SetState( nState );
	}
	return 0;
}


int CMainFrame::OnIFNStateChanged( int nState )
{
	if ( elk.IsOpened() )
	{
		CEdit *pwndEdit = wndInputView.GetTranslateEdit();
		if ( pwndEdit && pwndEdit->IsWindowEnabled() )
		{
			wndBaseTree.UpdateSelectedText( &elk, nState );
		}
		else
		{
			BeginWaitCursor();
			wndBaseTree.UpdateSelectedFolder( &elk, nState );
			EndWaitCursor();
		}
	}	
	return 0;
}


int CMainFrame::OnTENKeyDown( unsigned nChar )
{
	if ( nChar == 'N' )
	{
		bool bEnable = elk.IsOpened() && wndBaseTree.IsNextItemExists( params.GetCurrentFilter() );
		if ( bEnable )
		{
			OnNextButton();
		}
	}
	else if ( nChar == 'P' )
	{
		bool bEnable = elk.IsOpened() && wndBaseTree.IsPreviousItemExists( params.GetCurrentFilter() );
		if ( bEnable )
		{
			OnBackButton();
		}
	}
	else if ( nChar == 'F' )
	{
		bool bEnable = elk.IsOpened();
		if ( bEnable )
		{
			OnEditFind();
		}
	}
	else if ( nChar == VK_F7 )
	{
		if ( elk.IsOpened() )
		{
			CEdit *pwndEdit = wndInputView.GetTranslateEdit();
			if ( pwndEdit && pwndEdit->IsWindowEnabled() )
			{
				if ( spellChecker.IsAvailiable() )
				{
					OnToolsSpelling();
				}
			}
		}
	}
	else if ( nChar == VK_F1 )
	{
		if ( NFile::DoesFileExist( params.szHelpFilePath.c_str() ) )
		{
			OnHelpContents();
		}
	}
	return 0;
}


void CMainFrame::OnExportToExcel() 
{
	if ( elk.IsOpened() )
	{
		OnFileSave();

		CString strDialogTitle;
		CString strFileName;
		CString strFolderName;

		strDialogTitle.LoadString( IDS_EXPORT_TO_XLS_DIALOG_TITLE );

		strFileName = params.szLastOpenedXLSName.c_str();
		int nSlashPos = strFileName.ReverseFind( '\\' );
		if ( nSlashPos >= 0 )
		{
			strFolderName = strFileName.Left( nSlashPos );
		}

		CFileDialog fileDialog( false, _T( ".xls" ), strFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T( "Excel files (*.xls)|*.xls|All Files (*.*)|*.*||" ), this );
		fileDialog.m_ofn.lpstrTitle = strDialogTitle;
		fileDialog.m_ofn.lpstrInitialDir = strFolderName;
		
		if ( fileDialog.DoModal() == IDOK )
		{
			strFileName = fileDialog.GetPathName();
			CStringManager::ExtendFileExtention( &strFileName, _T( ".xls" ) );
			params.szLastOpenedXLSName = strFileName;
			CProgressDialog progressDialog;
			progressDialog.Create( IDD_PROGRESS, this );
			CELK::ExportToXLS( elk, params.szLastOpenedXLSName, &wndBaseTree, params.nCodePage, &progressDialog );
			if ( progressDialog.GetSafeHwnd() != 0 )
			{
				progressDialog.DestroyWindow();
			}
		}	
	}
}


void CMainFrame::OnUpdateExportToExcel( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( elk.IsOpened() );
}


void CMainFrame::OnImportFromExcel() 
{
	string szXLSPath;
	string szELKPath;
	bool bOK = false;
	if ( bShortApperence )
	{
		if ( elk.IsOpened() )
		{
			CString strDialogTitle;
			CString strFileName;
			CString strFolderName;

			strDialogTitle.LoadString( IDS_IMPORT_FROM_XLS_DIALOG_TITLE );

			strFileName = params.szLastOpenedXLSName.c_str();
			int nSlashPos = strFileName.ReverseFind( '\\' );
			if ( nSlashPos >= 0 )
			{
				strFolderName = strFileName.Left( nSlashPos );
			}

			CFileDialog fileDialog( true, _T( ".xls" ), strFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T( "Excel files (*.xls)|*.xls|All Files (*.*)|*.*||" ), this );
			fileDialog.m_ofn.lpstrTitle = strDialogTitle;
			fileDialog.m_ofn.lpstrInitialDir = strFolderName;
			
			if ( fileDialog.DoModal() == IDOK )
			{
				szXLSPath = fileDialog.GetPathName();
				CStringManager::ExtendFileExtention( &strFileName, _T( ".xls" ) );
				szELKPath = elk.szPath;
				bOK = true;
			}
		}
	}
	else
	{
		CImportFromXLSDialog dialog( this );
		if ( dialog.DoModal() == IDOK )
		{
			dialog.GetXLSPath( &szXLSPath );
			dialog.GetFilePath( &szELKPath );
			bOK = true;
		}
	}
	
	params.ValidatePath( &szXLSPath, false );
	params.ValidatePath( &szELKPath, false );
	
	if ( bOK )
	{
		string szLastELKPath;
		if ( elk.IsOpened() )
		{
			szLastELKPath = elk.szPath;
			NStr::ToLower( &szLastELKPath );
			if ( szLastELKPath == szELKPath )
			{
				CloseELK();
			}
			else
			{
				szLastELKPath.clear();
			}
		}

		CProgressDialog progressDialog;
		progressDialog.Create( IDD_PROGRESS, this );
					
		CELK selectedELK;
		selectedELK.Open( szELKPath, false );

		if ( selectedELK.elementList.empty() )
		{
			vector<SELKElement>::iterator posELKElement = selectedELK.elementList.insert( selectedELK.elementList.end(), SELKElement() );	
			posELKElement->szPath = StrFmt( _T( "%s\\%s" ), szELKPath.substr( szELKPath.rfind( '\\' ) + 1 ).c_str(), CELK::ELK_FILE_NAME );
		}

		string szVersion;
		CELK::ImportFromXLS( selectedELK, szXLSPath, &szVersion, params.nCodePage, &progressDialog );

		for ( int nElementIndex = 0; nElementIndex < selectedELK.elementList.size(); ++nElementIndex )
		{
			if ( selectedELK.elementList[nElementIndex].szVersion.empty() )
			{
				selectedELK.elementList[nElementIndex].szVersion = szVersion;
			}
		}
					
		params.szLastOpenedXLSName = szXLSPath;
		selectedELK.szPath = params.szLastOpenedELKName;
		selectedELK.Save(); 

		if ( progressDialog.GetSafeHwnd() != 0 )
		{
			progressDialog.DestroyWindow();
		}

		if ( !szLastELKPath.empty() )
		{
			OpenELK( szLastELKPath );
		}
	}
}


void CMainFrame::OnUpdateImportFromExcel( CCmdUI* pCmdUI )
{
	if ( bShortApperence )
	{
		pCmdUI->Enable( elk.IsOpened() );
	}
	else
	{
		pCmdUI->Enable( true );
	}
}


void CMainFrame::OnUpdateBackPageButton( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( elk.IsOpened() && wndBaseTree.IsPreviousItemExists( params.GetCurrentFilter() ) );
}


void CMainFrame::OnUpdateNextPageButton( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( elk.IsOpened() && wndBaseTree.IsNextItemExists( params.GetCurrentFilter() ) );
}


void CMainFrame::OnUpdateNextButton( CCmdUI* pCmdUI ) 
{
	bool bEnable = elk.IsOpened() && wndBaseTree.IsNextItemExists( params.GetCurrentFilter() );
	pCmdUI->Enable( bEnable );
	wndInputView.EnableNextButton( bEnable );
}


void CMainFrame::OnUpdateBackButton( CCmdUI* pCmdUI ) 
{
	bool bEnable = elk.IsOpened() && wndBaseTree.IsPreviousItemExists( params.GetCurrentFilter() );
	pCmdUI->Enable( bEnable );
	wndInputView.EnableBackButton( bEnable );
}


void CMainFrame::OnBackPageButton() 
{
	wndBaseTree.GetFirstItem( params.GetCurrentFilter() );
	wndInputView.SetFocus();
}


void CMainFrame::OnNextPageButton() 
{
	wndBaseTree.GetLastItem( params.GetCurrentFilter() );
	wndInputView.SetFocus();
}


void CMainFrame::OnNextButton() 
{
	wndBaseTree.GetNextItem( params.GetCurrentFilter() );
	wndInputView.SetFocus();
}


void CMainFrame::OnBackButton() 
{
	wndBaseTree.GetPreviousItem( params.GetCurrentFilter() );
	wndInputView.SetFocus();
}

void CMainFrame::OnUpdateFileSave(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( elk.IsOpened() );
}


void CMainFrame::OnFileSave() 
{
	if ( !params.szPreviousPath.empty() && wndInputView.IsTranslatedTextChanged() )
	{
		CString strText;
		wndInputView.GetTranslatedText( &strText );
		CELK::SetTranslatedText( params.szPreviousPath, strText, params.nCodePage );
	}
	params.szPreviousPath.clear();
}


void CMainFrame::OnBrowseCollapseItem() 
{
	params.bCollapseItem = !params.bCollapseItem;
	wndBaseTree.SetCollapseItem( params.bCollapseItem );
}


void CMainFrame::OnUpdateBrowseCollapseItem(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( elk.IsOpened() );
	pCmdUI->SetCheck( params.bCollapseItem );	
}


int32_t CMainFrame::OnCreateCombo(unsigned wParam, int32_t lParam)
{
	HWND hWnd = reinterpret_cast<HWND>( lParam );
	unsigned nNotifyCode = HIWORD( wParam );
	unsigned nIDCtl = LOWORD( wParam );
	NI_ASSERT( ::IsWindow(hWnd), StrFmt( _T( "CMainFrame::OnCreateCombo(), not a window" ) ) );
	CWnd *pWnd = CWnd::FromHandle( hWnd );
	
	switch( nIDCtl )
	{
		case IDC_IV_FILTER_COMBO_BOX:
		{
			ASSERT_KINDOF( CComboBox, pWnd );
			CComboBox *pCombo = dynamic_cast<CComboBox*>( pWnd );
			switch( nNotifyCode )
			{
				case SECWndBtn::WndInit:
				{
					pwndFiltersComboBox = pCombo;
					FillFiltersComboBox();
				}
			}
			break;
		}
	}
	return 0;
}


void CMainFrame::FillFiltersComboBox()
{
	if ( pwndFiltersComboBox )
	{
		pwndFiltersComboBox->ResetContent();
		for ( CFilterMap::const_iterator itFilter = params.filterMap.begin(); itFilter != params.filterMap.end(); ++itFilter )
		{
			pwndFiltersComboBox->AddString( itFilter->first.c_str() );
		}
		if ( !params.szCurrentFilterName.empty() )
		{
			pwndFiltersComboBox->SelectString( -1, params.szCurrentFilterName.c_str() );
		}
		else
		{
			pwndFiltersComboBox->SetCurSel( 0 );
		}
	}
}


void CMainFrame::OnChangeFilter()
{
	if ( pwndFiltersComboBox )
	{
		int nSelection = pwndFiltersComboBox->GetCurSel();
		CString strText;
		pwndFiltersComboBox->GetLBText( nSelection, strText );
		if ( params.szCurrentFilterName != string ( LPCTSTR( strText ) ) )
		{
			params.szCurrentFilterName = string ( LPCTSTR( strText ) );
			wndBaseTree.SetFilterChanged();
/**
			if ( elk.IsOpened() )
			{
				wndBaseTree.GetFirstItem( params.GetCurrentFilter() );
			}
/**/
		}
	}
}


void CMainFrame::OnBrowseSkipOptions() 
{
	CCreateFilterDialog createFilterDialog;
	createFilterDialog.filterMap = params.filterMap;
	createFilterDialog.folderMap = elk.enumFolderStructureParameter.enumFolderMap;
	if ( createFilterDialog.DoModal() == IDOK )
	{
		params.filterMap = createFilterDialog.filterMap;
		FillFiltersComboBox();
	}
}


void CMainFrame::OnUpdateBrowseSkipOptions( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( true );
}


void CMainFrame::OnUpdateEditPaste( CCmdUI* pCmdUI ) 
{
	CEdit *pwndEdit = wndInputView.GetTranslateEdit();
	if ( pwndEdit && pwndEdit->IsWindowEnabled() )
	{
		COleDataObject oleDataObject;
		oleDataObject.AttachClipboard();
		pCmdUI->Enable( oleDataObject.IsDataAvailable( CF_TEXT ) );
		return;
	}
	pCmdUI->Enable( false );
}


void CMainFrame::OnEditPaste() 
{
	CEdit *pwndEdit = wndInputView.GetTranslateEdit();
	if ( pwndEdit && pwndEdit->IsWindowEnabled() )
	{
		pwndEdit->Paste();
	}
}


void CMainFrame::OnUpdateEditUndo( CCmdUI* pCmdUI ) 
{
	CEdit *pwndEdit = wndInputView.GetTranslateEdit();
	if ( pwndEdit && pwndEdit->IsWindowEnabled() )
	{
		pCmdUI->Enable( pwndEdit->CanUndo() );
		return;
	}
	pCmdUI->Enable( false );
}


void CMainFrame::OnEditUndo() 
{
	CEdit *pwndEdit = wndInputView.GetTranslateEdit();
	if ( pwndEdit && pwndEdit->IsWindowEnabled() )
	{
		pwndEdit->Undo();
	}
}


void CMainFrame::OnUpdateEditSelectAll( CCmdUI* pCmdUI ) 
{
	CEdit *pwndEdit = wndInputView.GetTranslateEdit();
	if ( pwndEdit && pwndEdit->IsWindowEnabled() )
	{
		CString strText;
		int nStartChar, nEndChar;
		pwndEdit->GetSel( nStartChar, nEndChar );
		pwndEdit->GetWindowText( strText );
		pCmdUI->Enable( ( nStartChar > 0 ) || ( nEndChar < strText.GetLength() ) );
		return;
	}
	pCmdUI->Enable( false );
}


void CMainFrame::OnEditSelectAll() 
{
	CEdit *pwndEdit = wndInputView.GetTranslateEdit();
	if ( pwndEdit && pwndEdit->IsWindowEnabled() )
	{
		pwndEdit->SetSel( 0, -1 );
	}
}


void CMainFrame::OnUpdateEditClear( CCmdUI* pCmdUI ) 
{
	CEdit *pwndEdit = wndInputView.GetTranslateEdit();
	if ( pwndEdit && pwndEdit->IsWindowEnabled() )
	{
		int nStartChar, nEndChar;
		pwndEdit->GetSel( nStartChar, nEndChar );
		pCmdUI->Enable( nStartChar != nEndChar );
		return;
	}
	pCmdUI->Enable( false );
}


void CMainFrame::OnEditClear() 
{
	CEdit *pwndEdit = wndInputView.GetTranslateEdit();
	if ( pwndEdit && pwndEdit->IsWindowEnabled() )
	{
		pwndEdit->Clear();
	}
}


void CMainFrame::OnUpdateEditCopy( CCmdUI* pCmdUI ) 
{
	CEdit *pwndEdit = wndInputView.GetTranslateEdit();
	if ( pwndEdit && pwndEdit->IsWindowEnabled() )
	{
		int nStartChar, nEndChar;
		pwndEdit->GetSel( nStartChar, nEndChar );
		pCmdUI->Enable( nStartChar != nEndChar );
		return;
	}
	pCmdUI->Enable( false );
}


void CMainFrame::OnEditCopy() 
{
	CEdit *pwndEdit = wndInputView.GetTranslateEdit();
	if ( pwndEdit && pwndEdit->IsWindowEnabled() )
	{
		pwndEdit->Copy();
	}
}


void CMainFrame::OnUpdateEditCut( CCmdUI* pCmdUI ) 
{
	CEdit *pwndEdit = wndInputView.GetTranslateEdit();
	if ( pwndEdit && pwndEdit->IsWindowEnabled() )
	{
		int nStartChar, nEndChar;
		pwndEdit->GetSel( nStartChar, nEndChar );
		pCmdUI->Enable( nStartChar != nEndChar );
		return;
	}
	pCmdUI->Enable( false );
}


void CMainFrame::OnEditCut() 
{
	CEdit *pwndEdit = wndInputView.GetTranslateEdit();
	if ( pwndEdit && pwndEdit->IsWindowEnabled() )
	{
		pwndEdit->Cut();
	}
}


void CMainFrame::OnEditFind() 
{
	if ( elk.IsOpened() )
	{
		if ( pwndFindReplaceDialog && ( pwndFindReplaceDialog->GetSafeHwnd() != 0 ) )
		{
			pwndFindReplaceDialog->ShowWindow( SW_SHOW );
		}
		pwndFindReplaceDialog = new CFindReplaceDialog();
		uint32_t dwFlags = 0;
		if ( params.searchParam.bFindDown )
		{
			dwFlags |= FR_DOWN;
		}
		if ( params.searchParam.bFindMatchCase )
		{
			dwFlags |= FR_MATCHCASE;
		}
		if ( params.searchParam.bFindWholeWord )
		{
			dwFlags |= FR_WHOLEWORD;
		}
		pwndFindReplaceDialog->Create( true, params.searchParam.strFindText, 0, dwFlags, this );
	}
}


void CMainFrame::OnUpdateEditFind( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( elk.IsOpened() );
}


int32_t CMainFrame::OnFindReplace( WPARAM wParam, LPARAM lParam )
{
	if ( lParam != 0 )
	{
		LPFINDREPLACE lpfr = reinterpret_cast<LPFINDREPLACE>( lParam );

		if ( lpfr->Flags & FR_DIALOGTERM )
		{ 
			delete pwndFindReplaceDialog;
			pwndFindReplaceDialog = 0;
			params.searchParam.nWindowType = SMainFrameParams::SSearchParam::WT_ORIGINAL;
			params.searchParam.nPosition = 0;
			return 0; 
		} 
		else if ( lpfr->Flags & FR_FINDNEXT ) 
		{
			if ( elk.IsOpened() )
			{
				params.searchParam.bFindDown = ( ( lpfr->Flags & FR_DOWN ) > 0 );
				params.searchParam.bFindMatchCase = ( ( lpfr->Flags & FR_MATCHCASE ) > 0 );
				params.searchParam.bFindWholeWord = ( ( lpfr->Flags & FR_WHOLEWORD ) > 0 );
				params.searchParam.strFindText = lpfr->lpstrFindWhat;

				if ( wndBaseTree.FindItem( elk, &( params.searchParam ), params.nCodePage ) )
				{
					wndInputView.SelectText( params.searchParam );
					params.searchParam.nPosition += 1;
				}
			}
		}
	}
	return 0;
}


void CMainFrame::OnToolsRunGame()
{
	if ( bGameExists && elk.IsOpened() && CheckGameApp( 0, _T( " Blitzkrieg Game" ) ) )
	{
		CProgressDialog progressDialog;
		progressDialog.Create( IDD_PROGRESS, this );
		CELK::UpdateGame( elk,
											params.szZIPToolPath,
											&wndBaseTree,
											true,
											params.nCodePage,
											&progressDialog );
		if ( progressDialog.GetSafeHwnd() != 0 )
		{
			progressDialog.DestroyWindow();
		}
	}
}


void CMainFrame::OnUpdateToolsRunGame(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( bGameExists && elk.IsOpened() && CheckGameApp( 0, _T( " Blitzkrieg Game" ) ) );
}


void CMainFrame::OnCloseButton() 
{
	OnClose();
}


void CMainFrame::OnUpdateClose( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( true );
}


void CMainFrame::OnUpdateToolsSpelling(CCmdUI* pCmdUI) 
{
	if ( elk.IsOpened() )
	{
		CEdit *pwndEdit = wndInputView.GetTranslateEdit();
		if ( pwndEdit && pwndEdit->IsWindowEnabled() )
		{
			pCmdUI->Enable( spellChecker.IsAvailiable() );
			return;
		}
	}
	pCmdUI->Enable( false );
}


void CMainFrame::OnToolsSpelling() 
{
	CString strTranslatedText;
	wndInputView.GetTranslatedText( &strTranslatedText );
	
	if ( ( spellChecker.nCharIndex > 0 ) && ( spellChecker.nCharIndex < strTranslatedText.GetLength() ) )
	{
		strTranslatedText = strTranslatedText.Mid( spellChecker.nCharIndex );
	}
	else
	{
		spellChecker.nCharIndex = 0;
	}

	int nPosition = spellChecker.nCharIndex;
	int nCharCount = 0;
	bool bNotChecked = false;
	CString strWord;
	
	while ( !strTranslatedText.IsEmpty() )
	{
		nCharCount = CSpellChecker::GetWord( &strTranslatedText, &strWord );
		nPosition += nCharCount;
		if ( !spellChecker.Check( strWord ) )
		{
			bNotChecked = true;
			break;
		}
		nPosition += strWord.GetLength();
	}

	CString strProgramTitle;
	strProgramTitle.LoadString( AFX_IDS_APP_TITLE );

	CString strMessage;
	CEdit *pwndEdit = wndInputView.GetTranslateEdit();
	if ( pwndEdit )
	{
		pwndEdit->SetActiveWindow();
		if ( bNotChecked )
		{
			pwndEdit->SetSel( nPosition, nPosition + strWord.GetLength() );
			
			vector<CString> variants;
			/**
			if ( spellChecker.GetVariants( strWord, &variants ) > 0 )
			{
				strMessage = _T( "Word <" ) + strWord + _T( "> seems to be invalid.\nHere some valid variants:\n" );
				for ( int nWordIndex = 0; nWordIndex < variants.size(); ++nWordIndex )
				{
					if ( !variants[nWordIndex].IsEmpty() )
					{
						strMessage += variants[nWordIndex] + "\n";
					}
				}
			}
			else
			/**/
			{
				strMessage = _T( "Word <" ) + strWord + _T( "> seems to be invalid.\n" );
			}
			strMessage += _T( "ADD <" ) + strWord + _T( "> to CUSTOM DICTIONARY?" );
			if ( ::MessageBox( GetSafeHwnd(), strMessage, strProgramTitle, MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2 ) == IDYES )
			{
				spellChecker.Ignore( strWord );
			}
			
			pwndEdit->SetActiveWindow();
			pwndEdit->SetSel( nPosition, nPosition + strWord.GetLength() );
			
			spellChecker.nCharIndex = nPosition + strWord.GetLength() + 1;
		}
		else
		{
			int nStart;
			int nFinish;
			pwndEdit->GetSel( nStart, nFinish );

			strMessage.LoadString( IDS_SPELL_SUCCESS_MESSAGE );
			::MessageBox( GetSafeHwnd(), strMessage, strProgramTitle, MB_OK | MB_ICONINFORMATION );
			
			pwndEdit->SetActiveWindow();
			pwndEdit->SetSel( nStart, nFinish );
			
			spellChecker.nCharIndex = 0;
		}
	}
}



void CMainFrame::RunExternalHelpFile( const string &rszHelpFilePath )
{
	if ( NFile::DoesFileExist( rszHelpFilePath.c_str() ) )
	{
		::HtmlHelp( ::GetDesktopWindow(), rszHelpFilePath.c_str(), HH_DISPLAY_TOPIC, 0 );
  }
  else
  {
		CString strProgramTitle;
		strProgramTitle.LoadString( AFX_IDS_APP_TITLE );

    CString strMessagePattern;
    CString strMessage;
    
    strMessagePattern.LoadString( IDS_NO_HELP_FILE_MESSAGE );
		strMessage.Format( strMessagePattern, rszHelpFilePath.c_str() );
    ::MessageBox( GetSafeHwnd(), strMessage, strProgramTitle, MB_ICONERROR | MB_OK );
  }
}


void CMainFrame::OnUpdateHelpContents(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( NFile::DoesFileExist( params.szHelpFilePath.c_str() ) );
}


void CMainFrame::OnHelpContents() 
{
	RunExternalHelpFile( params.szHelpFilePath );
}


void CMainFrame::OnUpdateToolsChooseFons(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( elk.IsOpened() );
}


/**
#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	SECFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	SECFrameWnd::Dump(dc);
}
#endif //_DEBUG
/**/

