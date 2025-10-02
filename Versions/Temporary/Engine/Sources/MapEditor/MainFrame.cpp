#include "stdafx.h"
#include "../mapeditorlib/commandhandlerdefines.h"
#include "../mapeditorlib/resourcedefines.h"
#include "wmdefines.h"
#include "tree_gdbbrowserbase_constants.h"
#include "pc_constants.h"

//#include "../Scene/Scene.h"
//#include "../Scene/Camera.h"

#include "../System/FileUtils.h"
#include <HtmlHelp.h>

#include "../libdb/ResourceManager.h"
#include "MapeditorApp.h"
#include "MainFrame.h"
#include "../MapEditorLib/MapEditorModule.h"
#include "AboutDialog.h"
#include "../MapEditorLib/Interface_ChildFrame.h"
#include "../MapEditorLib/Interface_Editor.h"
#include "../MapEditorLib/Interface_Progress.h"
#include "DefaultChildFrame.h"
#include "DefaultDockingWindow.h"
#include "../Misc/StrProc.h"
#include "../MapEditorLib/StringManager.h"
#include "PC_DBLinkDialog.h"
#include "../MapEditorLib/CommonEditorMethods.h"
#include "../MapEditorLib/Tools_HashSet.h"
#include "MapEditorSingleton.h"
#include "../Main/MODs.h"

#include "../libdb/DB.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CMainFrame, SECWorkbook)


BEGIN_MESSAGE_MAP(CMainFrame, SECWorkbook)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	//
	ON_WM_QUERYENDSESSION()
	ON_WM_ENDSESSION()
	//
	ON_WM_COPYDATA()
	ON_WM_DROPFILES()
	//
	ON_REGISTERED_MESSAGE(WM_SECTOOLBARWNDNOTIFY, OnSECToolBarNotify)
	//
	ON_COMMAND_RANGE(ID_FIRST_COMMAND_ID, ID_LAST_COMMAND_ID, OnUserCommand)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FIRST_COMMAND_ID, ID_LAST_COMMAND_ID, OnUpdateUserCommand)
	//
	ON_COMMAND_RANGE(ID_VIEW_TOOLBAR_MAIN, ID_VIEW_TOOLBAR_VIEW, OnViewToolBar)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_TOOLBAR_MAIN, ID_VIEW_TOOLBAR_VIEW, OnUpdateViewToolBar)
	//
	//ON_COMMAND(ID_VIEW_DW_GDB_BROWSER, OnShowDWGDBBrowser)
	//ON_UPDATE_COMMAND_UI(ID_VIEW_DW_GDB_BROWSER, OnUpdateShowDWGDBBrowser)
	ON_COMMAND(ID_VIEW_DW_GDB_BROWSER_NEW, OnDWGDBBrowserNew)
	ON_COMMAND(ID_VIEW_DW_GDB_BROWSER_REMOVE, OnDWGDBBrowserRemove)
	ON_COMMAND_RANGE(ID_VIEW_DW_GDB_BROWSER_FIRST, ID_VIEW_DW_GDB_BROWSER_LAST, OnDWGDBBrowserWindow)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DW_GDB_BROWSER_NEW, OnUpdateDWGDBBrowserNew)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DW_GDB_BROWSER_REMOVE, OnUpdateDWGDBBrowserRemove)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_DW_GDB_BROWSER_FIRST, ID_VIEW_DW_GDB_BROWSER_LAST, OnUpdateDWGDBBrowserWindow)
	//
	ON_COMMAND(ID_VIEW_DW_PROPERTY_BROWSER, OnShowDWPropertyBrowser)
	ON_COMMAND(ID_VIEW_DW_LOG, OnShowDWLog)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DW_PROPERTY_BROWSER, OnUpdateShowDWPropertyBrowser)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DW_LOG, OnUpdateShowDWLog)
	//
	ON_COMMAND(ID_TOOLS_CUSTOMIZE, OnToolsCustomize)
	//
	ON_COMMAND(ID_HELP_CONTENTS, OnHelpContents)
	ON_COMMAND(ID_HELP_ABOUT, OnHelpAbout)
	ON_UPDATE_COMMAND_UI(ID_HELP_CONTENTS, OnUpdateHelpContents)
END_MESSAGE_MAP()


/**
CWMMnemonicCodes mnemonicCodes;
LRESULT CMainFrame::WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) 
{
	DebugTrace( "Message: %s, wParam: 0x%X(%u), lParam: 0x%X\n", mnemonicCodes.Get( message ).c_str(), wParam, wParam, lParam );
	return SECWorkbook::WindowProc( message, wParam, lParam );
}
/**/

CMainFrame::CMainFrame() : nFreeToolbarID( AFX_IDW_TOOLBAR + 9 ), hwndPreviousFocusedWindow( 0 )
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_VIEW, this );
	//
	//parent variables initialization
	m_pControlBarManager = new SECToolBarManager( this );
	m_pMenuBar = new SECMDIMenuBar();

	EnableBmpMenus();
}


CMainFrame::~CMainFrame()
{
	for ( list<CDWGDBBrowser*>::iterator itGDBBrowser = gdbBrowserList.begin(); itGDBBrowser != gdbBrowserList.end(); ++itGDBBrowser )
	{
		if ( *itGDBBrowser )
		{
			delete ( *itGDBBrowser );
		}
	}
	gdbBrowserList.clear();
	//parent variables uninitialization
	if ( m_pControlBarManager )
	{
		delete m_pControlBarManager;
		m_pControlBarManager = 0;
	}
	if ( m_pMenuBar )
	{
		delete m_pMenuBar;
		m_pMenuBar = 0;
	}
	//
	Singleton<ICommandHandlerContainer>()->Remove( CHID_VIEW );
}


BOOL CMainFrame::PreCreateWindow( CREATESTRUCT &rCreateStruct )
{
	if( !SECWorkbook::PreCreateWindow( rCreateStruct ) )
	{
		return FALSE;
	}
	//
	rCreateStruct.style &= ~FWS_ADDTOTITLE;
	return TRUE;
}


int CMainFrame::OnCreate( LPCREATESTRUCT pCreateStruct )
{
	if ( SECWorkbook::OnCreate( pCreateStruct ) == -1 )
	{
		return -1;
	}
	//
	CEditorApp *pApp = dynamic_cast<CEditorApp *>( AfxGetApp() );
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	//Создаем singeton для приложения
	mapEditorSingletonApp.CreateMapFile( GetSafeHwnd() );
	params.Load( true );
	//Путь до Help файла
	{
		char pBuffer[0xFFF + 1];
		::GetCurrentDirectory( 0xFFF, pBuffer );
		CString strHelpFileName;
		strHelpFileName.LoadString( IDS_HELP_FILE_NAME );
		strHelpFilePath = CString( pBuffer ) + CString( "\\" ) + strHelpFileName;
	}
	//
	//Создаем ToolBars
	SECToolBarManager* pToolBarMgr = static_cast<SECToolBarManager*>( m_pControlBarManager );	
	if ( TOOLBARS_COUNT > 0 )
	{
		VERIFY( pToolBarMgr->LoadToolBarResource( MAKEINTRESOURCE( TOOLBAR_ID[0] ),
																							MAKEINTRESOURCE( TOOLBAR_ID[0] ) ) );
		//нулевой элемент пропускаем
		for ( DWORD nElementIndex = 1; nElementIndex < TOOLBARS_COUNT; ++nElementIndex )
		{
			VERIFY( pToolBarMgr->AddToolBarResource( MAKEINTRESOURCE( TOOLBAR_ID[nElementIndex] ),
																							MAKEINTRESOURCE( TOOLBAR_ID[nElementIndex] ) ) );
		}
		//
		CString strToolbarName;
		strToolbarName.LoadString( TOOLBAR_NAME_ID[0] );
		pToolBarMgr->DefineDefaultToolBar( TOOLBAR_CONTROL_ID[0],
																			 strToolbarName,
																			 TOOLBAR_ELEMENTS_COUNT[0],
																			 const_cast<UINT*>( TOOLBAR_ELEMENTS_ID[0] ),
																			 TOOLBAR_STYLE[0],
																			 AFX_IDW_DOCKBAR_TOP,
																			 0,
																			 true,
																			 TOOLBAR_SHOW[0] );
		//нулевой элемент пропускаем
		for ( DWORD nElementIndex = 1; nElementIndex < TOOLBARS_COUNT; ++nElementIndex )
		{
			strToolbarName.LoadString( TOOLBAR_NAME_ID[nElementIndex] );
			pToolBarMgr->DefineDefaultToolBar( TOOLBAR_CONTROL_ID[nElementIndex],
																				 strToolbarName,
																				 TOOLBAR_ELEMENTS_COUNT[nElementIndex],
																				 const_cast<UINT*>( TOOLBAR_ELEMENTS_ID[nElementIndex] ),
																				 TOOLBAR_STYLE[nElementIndex],
																				 AFX_IDW_DOCKBAR_TOP,
																				 TOOLBAR_CONTROL_ID[nElementIndex - 1],
																				 true,
																				 TOOLBAR_SHOW[nElementIndex] );
		}
	}	
	//Создаем StatusBar
	if ( !wndStatusBar.Create( this ) ||
			 !wndStatusBar.SetIndicators( STATUSBAR_INDICATORS_ID,
																		sizeof( STATUSBAR_INDICATORS_ID ) / sizeof( UINT ) ) )
	{
		return -1;
	}
	//нулевой элемент пропускаем
	for ( DWORD nElementIndex = 1; nElementIndex < STATUSBAR_ELEMENTS; ++nElementIndex )
	{
		const int nPaneIndex = wndStatusBar.CommandToIndex( STATUSBAR_INDICATORS_ID[nElementIndex] );
		wndStatusBar.SetPaneInfo( nPaneIndex,
															STATUSBAR_INDICATORS_ID[nElementIndex],
															SBPS_NORMAL,
															STATUSBAR_INDICATORS_SIZE[nElementIndex] );
	}
	//Создаем Menubar
	pApp->CreateMenus( this );
	EnableDocking( CBRS_ALIGN_ANY );
	//pToolBarMgr->SetDefaultDockState();
	//
	UINT nDWID = 0;
	CString strDWName;
	const DWORD dwDWStyle = WS_CHILD | CBRS_LEFT | CBRS_TOOLTIPS | CBRS_SIZE_DYNAMIC;
	const DWORD dwDWStyleEx = CBRS_EX_COOL | CBRS_EX_BORDERSPACE;
	//	
	//
	nDWID = SECControlBar::GetUniqueBarID( this, ID_DW_LOG_WINDOW );
	strDWName.LoadString( IDS_DW_LOG_NAME );
	if ( !wndLog.Create( this, strDWName, dwDWStyle, dwDWStyleEx, nDWID ) )
	{
		return -1;
	}
	//
	nDWID = SECControlBar::GetUniqueBarID( this, ID_DW_PROPERTY_BROWSER );
	strDWName.LoadString( IDS_DW_PROPERTY_BROWSE_NAME );
	wndPropertyBrowser.SetPCDialogXMLOptionsLabel( "CDWPropertyBrowser" );
	if ( !wndPropertyBrowser.Create( this, strDWName, dwDWStyle, dwDWStyleEx, nDWID ) )
	{
		return -1;
	}
	//
	if ( pUserData->gdbBrowserIDList.empty() )
	{
		pUserData->gdbBrowserIDList.push_back( SECControlBar::GetUniqueBarID( this, ID_DW_GDB_BROWSER ) );
	}
	//
	{
		int nWindowIndex = 0;
		for ( list<int>::const_iterator itGDBBrowserID = pUserData->gdbBrowserIDList.begin(); itGDBBrowserID != pUserData->gdbBrowserIDList.end(); ++itGDBBrowserID )
		{
			strDWName.LoadString( IDS_DW_GDB_BROWSE_NAME );
			if ( CDWGDBBrowser *pwndGDBBrowser = new CDWGDBBrowser( *itGDBBrowserID ) )
			{
				if ( !pwndGDBBrowser->Create( this, StrFmt( strDWName, nWindowIndex ), dwDWStyle, dwDWStyleEx, *itGDBBrowserID ) )
				{
					delete pwndGDBBrowser;
					return -1;
				}
				gdbBrowserList.push_back( pwndGDBBrowser );
			}
			++nWindowIndex;
		}
	}
	for ( list<CDWGDBBrowser*>::iterator itGDBBrowser = gdbBrowserList.begin(); itGDBBrowser != gdbBrowserList.end(); ++itGDBBrowser )
	{
		if ( ( *itGDBBrowser ) )
		{
			( *itGDBBrowser )->EnableDocking( DOCKING_WINDOWS_DOCK_STYLE[0] );
			DockControlBarEx( *itGDBBrowser,
												DOCKING_WINDOWS_DOCK_PLACE[0],
												0,
												0,
												DOCKING_WINDOWS_RATE[0],
												DOCKING_WINDOWS_WIDTH[0] );
		}
	}
	//
	wndPropertyBrowser.EnableDocking( DOCKING_WINDOWS_DOCK_STYLE[1] );
	DockControlBarEx( &wndPropertyBrowser,
										DOCKING_WINDOWS_DOCK_PLACE[1],
										0,
										0,
										DOCKING_WINDOWS_RATE[1],
										DOCKING_WINDOWS_WIDTH[1] );
	//
	wndLog.EnableDocking( DOCKING_WINDOWS_DOCK_STYLE[2] );
	DockControlBarEx( &wndLog,
										DOCKING_WINDOWS_DOCK_PLACE[2],
										0,
										0,
										DOCKING_WINDOWS_RATE[2],
										DOCKING_WINDOWS_WIDTH[2] );
	//создаем Logger
	nDWID = SECControlBar::GetUniqueBarID( this, ID_DW_LOG_WINDOW );
	//
	// Создаем постоянные панели в различных редакторах
	Singleton<IMainFrameContainer>()->Set( this );
	// Создаем дополнительные Controls
	for ( int nModuleIndex = 0; nModuleIndex < pApp->GetEditorModules().size(); ++nModuleIndex )
	{
		pApp->GetEditorModules()[nModuleIndex]->ModuleCreateControls();
	}
	// Создаем дополнительные Controls в зарегистрированных редакторах
	Singleton<IEditorContainer>()->CreateControls();
	//
	pToolBarMgr->SetButtonMap( pApp->GetToolbarButtonsMap() );
	//pToolBarMgr->EnableToolTips( TRUE );
	//pToolBarMgr->EnableFlyBy( TRUE );
	pToolBarMgr->EnableCoolLook( true );

	//Грузим расположение панелей
	CString strRegistryKeyName;
	strRegistryKeyName.LoadString( IDS_REGISTRY_KEY_WINDOWBAR );
	LoadBarState( strRegistryKeyName );
	pToolBarMgr->LoadState( strRegistryKeyName );
	// Создаем дополнительные Controls
	for ( int nModuleIndex = 0; nModuleIndex < pApp->GetEditorModules().size(); ++nModuleIndex )
	{
		pApp->GetEditorModules()[nModuleIndex]->ModulePostCreateControls();
	}
	Singleton<IEditorContainer>()->PostCreateControls();
	if ( ( params.rect.Width() != 0 ) && ( params.rect.Height() != 0 ) )
	{
		WINDOWPLACEMENT windowPlacement;
		windowPlacement.length = sizeof( WINDOWPLACEMENT );
		GetWindowPlacement( &windowPlacement );
		windowPlacement.rcNormalPosition.left = params.rect.left;
		windowPlacement.rcNormalPosition.top = params.rect.top;
		windowPlacement.rcNormalPosition.right = params.rect.right;
		windowPlacement.rcNormalPosition.bottom = params.rect.bottom;
		windowPlacement.showCmd = SW_SHOWNORMAL;
		SetWindowPlacement( &windowPlacement );
	}
	if ( params.bMaximized )
	{
		PostMessage( WM_SYSCOMMAND, SC_MAXIMIZE, 0 );
	}
	SetWindowText( pUserData->constUserData.szApplicationTitle.c_str() );
	//
	const bool bEnableEdit = ( NGlobal::GetVar( "disable_edit", 0 ) == 0 );
	ICommandHandlerContainer *pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
	bool bObjectStorageNotSet = true;
	for ( list<CDWGDBBrowser*>::iterator itGDBBrowser = gdbBrowserList.begin(); itGDBBrowser != gdbBrowserList.end(); ++itGDBBrowser )
	{
		if ( *itGDBBrowser )
		{
			if( ( *itGDBBrowser )->GetDWGDBBrowserID() == pUserData->nFocusedGDBBrowserID )
			{
				pCommandHandlerContainer->Set( CHID_OBJECT_STORAGE, ( *itGDBBrowser )->GetContents() );
				pCommandHandlerContainer->Set( CHID_MAIN, ( *itGDBBrowser ) );
				bObjectStorageNotSet = false;
			}
			( *itGDBBrowser )->EnableEdit( bEnableEdit );
		}
	}
	if ( bObjectStorageNotSet )
	{
		if ( *gdbBrowserList.begin() )
		{
			pCommandHandlerContainer->Set( CHID_OBJECT_STORAGE, ( *gdbBrowserList.begin() )->GetContents() );
			pCommandHandlerContainer->Set( CHID_MAIN, ( *gdbBrowserList.begin() ) );
			pUserData->nFocusedGDBBrowserID = ( *gdbBrowserList.begin() )->GetDWGDBBrowserID();
		}
	}
	wndPropertyBrowser.EnableEdit( bEnableEdit );
	//
  DragAcceptFiles( true );

	SSWTParams swtParams;
	swtParams.dwFlags = SWT_ALL;
	swtParams.bFillMODFromBase = true;
	swtParams.bModified = false;
	SetWindowTitle( swtParams );
	//
	return 0;
}


LRESULT CMainFrame::OnSECToolBarNotify( WPARAM wParam, LPARAM lParam )
{
	/**
	UINT nNotifyCode = HIWORD( wParam );
	UINT nControlID = LOWORD( wParam );
	
	HWND hWnd	= reinterpret_cast<HWND>( lParam );
	NI_ASSERT( ::IsWindow( hWnd ), StrFmt( "Not a window: 0x%X", hWnd ) );
	CWnd *pwnd = CWnd::FromHandle( hWnd );
	/**/

	/**
	switch( nControlID )
	{
		case ...:
		{
			ASSERT_KINDOF( CComboBox, pWnd );
			CComboBox* pCombo = static_cast<CComboBox*>( pWnd );
			switch( nNotifyCode )
			{
				case SECWndBtn::WndInit:
				{
					pwndBrushSizeComboBox = pCombo;
				}
			}
			break;
		}
		default:
		{
		}
	}
	/**/
	return 0;
}


void CMainFrame::OnClose()
{
	if ( Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, true ) )
	{
		DestroyProgressDialog();
		//
		SECToolBarManager* pToolBarMgr = static_cast<SECToolBarManager*>( m_pControlBarManager );	
		// Закрываем текущий редактор
		Singleton<IEditorContainer>()->DestroyActiveEditor( false );
		// Удаляем дополнительные Controls
		Singleton<IEditorContainer>()->PreDestroyControls();
		CEditorApp *pApp = dynamic_cast<CEditorApp *>( AfxGetApp() );
		for ( int nModuleIndex = 0; nModuleIndex < pApp->GetEditorModules().size(); ++nModuleIndex )
		{
			pApp->GetEditorModules()[nModuleIndex]->ModulePreDestroyControls();
		}
		//
		CString strRegistryKeyName;
		strRegistryKeyName.LoadString( IDS_REGISTRY_KEY_WINDOWBAR );
		SaveBarState( strRegistryKeyName );
		pToolBarMgr->SaveState( strRegistryKeyName );
		//Закрываем текущий Child Frame ( если он уже не был закрыт из редактора )
		Singleton<IChildFrameContainer>()->Destroy();
		// Удаляем постоянные панели в различных редакторах
		Singleton<IEditorContainer>()->DestroyControls();
		// Удаляем дополнительные Controls
		for ( int nModuleIndex = 0; nModuleIndex < pApp->GetEditorModules().size(); ++nModuleIndex )
		{
			pApp->GetEditorModules()[nModuleIndex]->ModuleDestroyControls();
		}
		//
		WINDOWPLACEMENT windowPlacement;
		windowPlacement.length = sizeof( WINDOWPLACEMENT );
		GetWindowPlacement( &windowPlacement );
		params.bMaximized = ( windowPlacement.showCmd == SW_SHOWMAXIMIZED );
		params.rect = CTRect<int>( windowPlacement.rcNormalPosition.left,
															windowPlacement.rcNormalPosition.top,
															windowPlacement.rcNormalPosition.right,
															windowPlacement.rcNormalPosition.bottom );
		params.Save( true );
		//
		mapEditorSingletonApp.RemoveMapFile();
		//
		Singleton<IMainFrameContainer>()->Set( 0 );
		//
		SECWorkbook::OnClose();
	}
}


BOOL CMainFrame::OnQueryEndSession()
{
	if ( Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, false ) )
	{
		return SECWorkbook::OnQueryEndSession();
	}
	return false;
}
  

void CMainFrame::OnEndSession( BOOL bEnding )
{
	SECWorkbook::OnEndSession( bEnding );
}


BOOL CMainFrame::OnCopyData( CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct )
{
	if ( ( pWnd == 0 ) && ( pCopyDataStruct != 0 ) && ( pCopyDataStruct->dwData == CMapEditorSingletonBase::OPEN_FILE ) )
	{
		string szCommandLine = static_cast<char*>( pCopyDataStruct->lpData );
		OpenResource( szCommandLine );
		return true;
	}
	else
	{
		return SECWorkbook::OnCopyData( pWnd, pCopyDataStruct );
	}
}


void CMainFrame::OnDropFiles( HDROP hDropInfo ) 
{
	int nFileCount = ::DragQueryFile( hDropInfo, 0xFFFFFFFF, 0, 0 );
	for ( int nFileIndex = 0; nFileIndex < nFileCount; ++nFileIndex )
	{
		CString strFileName;
		::DragQueryFile( hDropInfo,
										 nFileIndex,
										 strFileName.GetBuffer( 0xFFF ),
										 0xFFF );
		strFileName.ReleaseBuffer();
		string szResourceName = strFileName;
		OpenResource( szResourceName );
		break;
	}
	::DragFinish( hDropInfo );
}


void CMainFrame::OpenResource( const string &rszResourceName )
{
	if ( SUserData *pUserData = Singleton<IUserDataContainer>()->Get() )
	{
		string szResourceName = rszResourceName;
		NStr::TrimBoth( szResourceName, '"' );
		if ( pUserData->NormalizePath( &szResourceName, true, true, false, SUserData::NPT_DATA_STORAGE, 0 ) )
		{
			string szExtention;
			CStringManager::SplitFileName( 0, 0, &szExtention, szResourceName );
			NStr::ToLower( &szExtention );
			if ( szExtention == ".xdb" )
			{
				if ( IResourceManager *pResourceManager = Singleton<IResourceManager>() )
				{
					CDBID resourceDBID = CDBID( szResourceName );
					SObjectSet objectSet;
					objectSet.szObjectTypeName = NDb::GetClassTypeName( resourceDBID );
					InsertHashSetElement( &( objectSet.objectNameSet ), resourceDBID );
					if ( CPtr<IManipulator> pObjectManipulator = CManipulatorManager::CreateObectSetManipulator( objectSet ) )
					{
						const bool bMainObject = ( objectSet.szObjectTypeName == pUserData->constUserData.szMainObjectType );
						string szName;
						CStringManager::CreateRecentListName( &szName, objectSet, bMainObject );
						CStringManager::AddToRecentList( szName, bMainObject );
						//
						IView *pView = 0;
						Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_GET_VIEW, reinterpret_cast<DWORD>( &pView ) );
						if ( pView != 0 )
						{
							pView->SetViewManipulator( pObjectManipulator, objectSet, string() );
							Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_PC_DIALOG, ID_PC_DIALOG_CREATE_TREE, 0 );
						}
						if ( CWnd *pwndParent = GetParent() )
						{
							pwndParent->SendMessage( WM_TREE_GDB_BROWSER, MAKEWPARAM( TREE_GDB_BROWSER_CHANGE_SELECTION, 0 ), reinterpret_cast<LPARAM>( this ) );
						}
						if ( CPtr<IManipulator> pEditorManipulator = pResourceManager->CreateObjectManipulator( objectSet.szObjectTypeName, objectSet.objectNameSet.begin()->first ) )
						{
							Singleton<IEditorContainer>()->Create( pEditorManipulator, objectSet );
						}
					}
				}
			}
		}
	}
}


void CMainFrame::OnUserCommand( UINT nCommandID )
{
	bool bEnable = false;
	bool bChecked = false;
	if ( Singleton<ICommandHandlerContainer>()->UpdateCommand( nCommandID, &bEnable, &bChecked ) && bEnable  )
	{
		Singleton<ICommandHandlerContainer>()->HandleCommand( nCommandID, 0 );
	}
}


void CMainFrame::OnUpdateUserCommand( CCmdUI *pCmdUI )
{
	UINT nMenuID = INVALID_NODE_ID;
	const SUserData::CRecentList *pRecentList = 0;
	if ( ( pCmdUI->m_nID >= ID_MAIN_RECENT_0 )  && ( pCmdUI->m_nID <= ID_MAIN_RECENT_0 ) )
	{
		nMenuID = ID_MAIN_RECENT_0;
		pRecentList = &( Singleton<IUserDataContainer>()->Get()->recentList );
	}
	else if ( ( pCmdUI->m_nID >= ID_MAIN_RECENT_RESOURCE_0 )  && ( pCmdUI->m_nID <= ID_MAIN_RECENT_RESOURCE_0 ) )
	{
		nMenuID = ID_MAIN_RECENT_RESOURCE_0;
		pRecentList = &( Singleton<IUserDataContainer>()->Get()->recentResourceList );
	}
	if ( pRecentList != 0 )
	{
		//DebugTrace( "Update RecentList: %d", pCmdUI->m_nID );
		pCmdUI->Enable( !( pRecentList->empty() ) );
		if ( pCmdUI->m_nID == nMenuID )
		{
			if ( CMenu *pMenu = pCmdUI->m_pMenu )
			{
				int nMenuItemCount = pMenu->GetMenuItemCount();
				for ( int nMenuItemIndex = 0; nMenuItemIndex < nMenuItemCount; ++nMenuItemIndex )
				{
					if ( CMenu *pSubMenu = pMenu->GetSubMenu( nMenuItemIndex ) )
					{
						if ( ( pSubMenu->GetMenuItemCount() > 0 ) && ( pSubMenu->GetMenuItemID( 0 ) == nMenuID ) )
						{
							while( pSubMenu->GetMenuItemCount() > 0 )
							{
								pSubMenu->RemoveMenu( 0 , MF_BYPOSITION );
							}
							int nRecentCount = 0;
							for ( SUserData::CRecentList::const_iterator itResentName = pRecentList->begin(); itResentName != pRecentList->end(); ++itResentName )
							{
								pSubMenu->InsertMenu( -1, MF_BYPOSITION, nMenuID + nRecentCount, itResentName->c_str() );
								++nRecentCount;
							}
							if ( pSubMenu->GetMenuItemCount() == 0 )
							{
								CString strMenuLabel;
								strMenuLabel.LoadString( IDS_RECENT_EMPTY );
								pSubMenu->InsertMenu( -1, MF_BYPOSITION, nMenuID, strMenuLabel );
							}
							break;
						}
					}
				}
			}
		}
	}
	else	
	{	
		bool bEnable = false;
		bool bChecked = false;
		if ( !Singleton<ICommandHandlerContainer>()->UpdateCommand( pCmdUI->m_nID, &bEnable, &bChecked ) )
		{
			bEnable = false;
			bChecked = false;
		}
		pCmdUI->Enable( bEnable );
		pCmdUI->SetCheck( bChecked );	
	}
}


void CMainFrame::OnViewToolBar( UINT nCommandID )
{
	if ( CControlBar *pBar = GetControlBar( TOOLBAR_CONTROL_ID[nCommandID - ID_VIEW_TOOLBAR_MAIN] ) )
	{
		ShowControlBar( pBar, !pBar->IsVisible(), true );
	}
}


void CMainFrame::OnUpdateViewToolBar( CCmdUI *pCmdUI )
{
	if ( CControlBar *pBar = GetControlBar( TOOLBAR_CONTROL_ID[pCmdUI->m_nID - ID_VIEW_TOOLBAR_MAIN] ) )
	{
		pCmdUI->Enable( true );
		pCmdUI->SetCheck( pBar->IsVisible() );	
	}
}

/**

void CMainFrame::OnShowDWGDBBrowser() 
{
	ShowControlBar( &wndGDBBrowser, !wndGDBBrowser.IsVisible(), true );
}


void CMainFrame::OnUpdateShowDWGDBBrowser( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( true );
	pCmdUI->SetCheck( wndGDBBrowser.IsVisible() );	
}
/**/


void CMainFrame::OnDWGDBBrowserNew()
{
	if ( gdbBrowserList.size() < ( ID_VIEW_DW_GDB_BROWSER_LAST - ID_VIEW_DW_GDB_BROWSER_FIRST ) )
	{
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		ICommandHandlerContainer *pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
		//	
		const DWORD dwDWStyle = WS_CHILD | WS_VISIBLE | CBRS_LEFT | CBRS_TOOLTIPS | CBRS_SIZE_DYNAMIC;
		const DWORD dwDWStyleEx = CBRS_EX_COOL | CBRS_EX_BORDERSPACE;
		const int nUniqueBarID = SECControlBar::GetUniqueBarID( this, ID_DW_GDB_BROWSER );
		CString strDWName;
		strDWName.LoadString( IDS_DW_GDB_BROWSE_NAME );
		const bool bEnableEdit = ( NGlobal::GetVar( "disable_edit", 0 ) == 0 );
		if ( CDWGDBBrowser *pwndGDBBrowser = new CDWGDBBrowser( nUniqueBarID ) )
		{
			if ( !pwndGDBBrowser->Create( this, StrFmt( strDWName, gdbBrowserList.size() ), dwDWStyle, dwDWStyleEx, nUniqueBarID ) )
			{
				delete pwndGDBBrowser;
				return;
			}
			pwndGDBBrowser->EnableDocking( DOCKING_WINDOWS_DOCK_STYLE[0] );
			DockControlBarEx( pwndGDBBrowser,
												DOCKING_WINDOWS_DOCK_PLACE[0],
												0,
												0,
												DOCKING_WINDOWS_RATE[0],
												DOCKING_WINDOWS_WIDTH[0] );
			pwndGDBBrowser->EnableEdit( bEnableEdit );
			pCommandHandlerContainer->Set( CHID_OBJECT_STORAGE, pwndGDBBrowser->GetContents() );
			pCommandHandlerContainer->Set( CHID_MAIN, pwndGDBBrowser );
			pUserData->nFocusedGDBBrowserID = pwndGDBBrowser->GetDWGDBBrowserID();
			//
			pUserData->gdbBrowserIDList.push_back( nUniqueBarID );
			gdbBrowserList.push_back( pwndGDBBrowser );
		}
	}
}


void CMainFrame::OnDWGDBBrowserRemove()
{
	if ( gdbBrowserList.size() > 1 )
	{
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		ICommandHandlerContainer *pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
		//
		list<int>::iterator itGDBBrowserID = pUserData->gdbBrowserIDList.begin();
		int nWindowIndex = 0;
		CString strDWName;
		strDWName.LoadString( IDS_DW_GDB_BROWSE_NAME );
		for ( list<CDWGDBBrowser*>::iterator itGDBBrowser = gdbBrowserList.begin(); itGDBBrowser != gdbBrowserList.end(); ++itGDBBrowser )
		{
			if ( *itGDBBrowser )
			{
				if( ( *itGDBBrowser )->GetDWGDBBrowserID() == pUserData->nFocusedGDBBrowserID )
				{
					( *itGDBBrowser )->DestroyWindow();
					delete ( *itGDBBrowser );
					( *itGDBBrowser ) = 0;
					//
					itGDBBrowser = gdbBrowserList.erase( itGDBBrowser );
					pUserData->gdbBrowserIDList.erase( itGDBBrowserID );
					//
					for ( ;itGDBBrowser != gdbBrowserList.end(); ++itGDBBrowser )
					{
						( *itGDBBrowser )->SetWindowText( StrFmt( strDWName, nWindowIndex ) );
						++nWindowIndex;
					}
					//
					if ( *gdbBrowserList.begin() )
					{
						pCommandHandlerContainer->Set( CHID_OBJECT_STORAGE, ( *gdbBrowserList.begin() )->GetContents() );
						pCommandHandlerContainer->Set( CHID_MAIN, ( *gdbBrowserList.begin() ) );
						pUserData->nFocusedGDBBrowserID = ( *gdbBrowserList.begin() )->GetDWGDBBrowserID();
					}
					return;
				}
			}
			++itGDBBrowserID;
			++nWindowIndex;
		}
	}
}


void CMainFrame::OnDWGDBBrowserWindow( UINT nCommandID )
{
	const int nGDBBrowserIndex = nCommandID - ID_VIEW_DW_GDB_BROWSER_FIRST;
	int nWindowIndex = 0;
	for ( list<CDWGDBBrowser*>::iterator itGDBBrowser = gdbBrowserList.begin(); itGDBBrowser != gdbBrowserList.end(); ++itGDBBrowser )
	{
		if ( nWindowIndex == nGDBBrowserIndex )
		{
			ShowControlBar( ( *itGDBBrowser ), !( *itGDBBrowser )->IsVisible(), true );
			return;
		}
		++nWindowIndex;
	}	
}


void CMainFrame::OnUpdateDWGDBBrowserNew( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( gdbBrowserList.size() < ( ID_VIEW_DW_GDB_BROWSER_LAST - ID_VIEW_DW_GDB_BROWSER_FIRST ) );
	pCmdUI->SetCheck( false );	
}


void CMainFrame::OnUpdateDWGDBBrowserRemove( CCmdUI* pCmdUI )
{
	pCmdUI->SetCheck( false );	
	//
	if ( gdbBrowserList.size() > 1 )
	{
		SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		for ( list<CDWGDBBrowser*>::iterator itGDBBrowser = gdbBrowserList.begin(); itGDBBrowser != gdbBrowserList.end(); ++itGDBBrowser )
		{
			if ( *itGDBBrowser )
			{
				if( ( *itGDBBrowser )->GetDWGDBBrowserID() == pUserData->nFocusedGDBBrowserID )
				{
					pCmdUI->Enable( true );
					return;
				}
			}
		}
	}
	pCmdUI->Enable( false );
}


void CMainFrame::OnUpdateDWGDBBrowserWindow( CCmdUI *pCmdUI )
{
	//DebugTrace( "Update RecentList: %d", pCmdUI->m_nID );
	if ( pCmdUI->m_nID == ID_VIEW_DW_GDB_BROWSER_FIRST )
	{
		const int nFirstMenuItemIndex = 0;
		const int nEmptyMenuItemCount = 3;
		if ( CMenu *pMenu = pCmdUI->m_pMenu )
		{
			CString strMenuLabel;
			CString strMenuLabelShort;
			strMenuLabel.LoadString( IDS_DW_GDB_BROWSE_MENU_LABEL );
			strMenuLabelShort.LoadString( IDS_DW_GDB_BROWSE_MENU_LABEL_SHORT );
			if ( ( pMenu->GetMenuItemCount() > nEmptyMenuItemCount ) && ( pMenu->GetMenuItemID( nFirstMenuItemIndex ) == ID_VIEW_DW_GDB_BROWSER_FIRST ) )
			{
				while( pMenu->GetMenuItemCount() > nEmptyMenuItemCount )
				{
					pMenu->RemoveMenu( nFirstMenuItemIndex , MF_BYPOSITION );
				}
				int nWindowIndex = 0;
				for ( list<CDWGDBBrowser*>::const_iterator itGDBBrowser = gdbBrowserList.begin(); itGDBBrowser != gdbBrowserList.end(); ++itGDBBrowser )
				{
					string szLabel = ( nWindowIndex == 0 )  ? StrFmt( strMenuLabel, nWindowIndex ) : StrFmt( strMenuLabelShort, nWindowIndex );
;
					pMenu->InsertMenu( nFirstMenuItemIndex + nWindowIndex, MF_BYPOSITION, ID_VIEW_DW_GDB_BROWSER_FIRST + nWindowIndex, szLabel.c_str() );
					pMenu->EnableMenuItem( ID_VIEW_DW_GDB_BROWSER_FIRST + nWindowIndex, MF_ENABLED | MF_BYCOMMAND );
					pMenu->CheckMenuItem( ID_VIEW_DW_GDB_BROWSER_FIRST + nWindowIndex, ( ( *itGDBBrowser )->IsVisible() ? MF_CHECKED : MF_UNCHECKED ) | MF_BYCOMMAND );
					++nWindowIndex;
				}
				if ( pMenu->GetMenuItemCount() == nEmptyMenuItemCount )
				{
					strMenuLabel.LoadString( IDS_DW_GDB_BROWSE_EMPTY_MENU_LABEL );
					pMenu->InsertMenu( nFirstMenuItemIndex + nWindowIndex, MF_BYPOSITION, ID_VIEW_DW_GDB_BROWSER_FIRST, strMenuLabel );
					pMenu->EnableMenuItem( ID_VIEW_DW_GDB_BROWSER_FIRST, MF_GRAYED | MF_BYCOMMAND );
					pMenu->CheckMenuItem( ID_VIEW_DW_GDB_BROWSER_FIRST, MF_UNCHECKED | MF_BYCOMMAND );
				}
			}
		}
		else
		{
			if ( gdbBrowserList.empty() || ( gdbBrowserList.front() == 0 ) )
			{
				pCmdUI->Enable( false );
				pCmdUI->SetCheck( false );
			}
			else
			{
				pCmdUI->Enable( true );
				pCmdUI->SetCheck( gdbBrowserList.front()->IsVisible() );
			}
		}
	}
}


void CMainFrame::OnShowDWPropertyBrowser() 
{
	ShowControlBar( &wndPropertyBrowser, !wndPropertyBrowser.IsVisible(), true );
}


void CMainFrame::OnUpdateShowDWPropertyBrowser( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( true );
	pCmdUI->SetCheck( wndPropertyBrowser.IsVisible() );	
}


void CMainFrame::OnShowDWLog() 
{
	ShowControlBar( &wndLog, !wndLog.IsVisible(), true );
}


void CMainFrame::OnUpdateShowDWLog( CCmdUI* pCmdUI ) 
{
	pCmdUI->Enable( true );
	pCmdUI->SetCheck( wndLog.IsVisible() );	
}


void CMainFrame::OnToolsCustomize() 
{
	SECToolBarsPage toolBarPage;
	toolBarPage.SetManager( dynamic_cast<SECToolBarManager*>( m_pControlBarManager ) );

	SECToolBarCmdPage cmdPage( SECToolBarCmdPage::IDD, IDS_TOOLBAR_CUSTOMIZE_COMMAND );
	cmdPage.SetManager( dynamic_cast<SECToolBarManager*>( m_pControlBarManager ) );
	
	CString strToolbarName;
	for ( DWORD nElementIndex = 0; nElementIndex < TOOLBARS_COUNT; ++nElementIndex )
	{
		strToolbarName.LoadString( TOOLBAR_NAME_ID[nElementIndex] );
		cmdPage.DefineBtnGroup( strToolbarName,
														TOOLBAR_ELEMENTS_COUNT[nElementIndex],
														const_cast<UINT*>( TOOLBAR_ELEMENTS_ID[nElementIndex] ) );
	}

	CEditorApp *pApp = dynamic_cast<CEditorApp *>( AfxGetApp() );
	CCursomToolBarInfoList cursomToolBarInfoList;
	pApp->GetCursomToolBarsInfo( &cursomToolBarInfoList );
	for ( DWORD nElementIndex = 0; nElementIndex < cursomToolBarInfoList.size(); ++nElementIndex )
	{
		cmdPage.DefineBtnGroup( cursomToolBarInfoList[nElementIndex].strName,
														cursomToolBarInfoList[nElementIndex].nCount,
														const_cast<UINT*>( cursomToolBarInfoList[nElementIndex].pButtons ) );
	}

	strToolbarName.LoadString( IDS_TOOLBAR_MENU );
	cmdPage.DefineMenuGroup( strToolbarName );

	SECToolBarSheet toolBarSheet( IDS_TOOLBAR_CUSTOMIZE, AfxGetMainWnd() );
	toolBarSheet.AddPage( &toolBarPage );
	toolBarSheet.AddPage( &cmdPage );

	toolBarSheet.DoModal();
}


#pragma comment( lib, "htmlhelp.lib" )
void CMainFrame::OnHelpContents() 
{
	if ( NFile::DoesFileExist( (const char*)strHelpFilePath ) )
	{
		::HtmlHelp( ::GetDesktopWindow(), strHelpFilePath, HH_DISPLAY_TOPIC, 0 );
	}
  else
  {
    CString strMessagePattern;
    CString strMessage;
    
    strMessagePattern.LoadString( IDS_NO_HELP_FILE_MESSAGE );
		strMessage.Format( strMessagePattern, strHelpFilePath );
    MessageBox( strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONERROR | MB_OK );
  }
}


void CMainFrame::OnHelpAbout() 
{
	CAboutDialog aboutDialog( AfxGetMainWnd() );
	aboutDialog.DoModal();
}


void CMainFrame::OnUpdateHelpContents( CCmdUI *pCmdUI ) 
{
	pCmdUI->Enable( NFile::DoesFileExist( (const char*)strHelpFilePath ) );
}


bool CMainFrame::GetToolBarButtonLeftBottomPos( const CTPoint<int> &rMousePoint,
																								UINT nButtonID,
																								CTPoint<int> *pLeftBottomPos )
{
	CControlBar* pControlBar;
	POSITION pos = m_listControlBars.GetHeadPosition();
	while( pos )
	{
		pControlBar = static_cast<CControlBar*>( m_listControlBars.GetNext( pos ) );
		if ( pControlBar->IsKindOf( RUNTIME_CLASS( SECCustomToolBar ) ) )
		{
			CRect controlRect;
			pControlBar->GetWindowRect( &controlRect );
			CPoint point( rMousePoint.x, rMousePoint.y );
			if ( controlRect.PtInRect( point ) )
			{
				SECCustomToolBar *pSECCustomToolBar = dynamic_cast<SECCustomToolBar*>( pControlBar );
				int nButtons = pSECCustomToolBar->GetBtnCount();
				for ( int nIndex = 0; nIndex < nButtons; ++nIndex )
				{
					if ( pSECCustomToolBar->m_btns[nIndex]->m_nID == nButtonID ) 
					{
						CRect itemRect;
						pSECCustomToolBar->GetItemRect( nIndex, &itemRect );

						itemRect.left += controlRect.left;
						itemRect.right += controlRect.left;
						itemRect.top += controlRect.top;
						itemRect.bottom += controlRect.top;
						
						if ( itemRect.PtInRect( point ) )
						{
							if ( pLeftBottomPos )
							{
								pLeftBottomPos->x = itemRect.left;
								pLeftBottomPos->y = itemRect.bottom;
							}
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}


SECWorksheet* CMainFrame::CreateChildFrame( UINT nResource )
{
	return dynamic_cast<SECWorksheet*>( CreateNewChild( RUNTIME_CLASS( CDefaultChildFrame ), nResource, 0, 0 ) );
}


bool CMainFrame::SetChildFrameWindowContents( SECWorksheet* _pwndChildFrame, class CWnd *pwndContents )
{
	if ( _pwndChildFrame )
	{
		if ( CDefaultChildFrame *pwndChildFrame = checked_cast<CDefaultChildFrame*>( _pwndChildFrame ) )
		{
			pwndChildFrame->pwndContents = pwndContents;
			return true;
		}
	}
	return false;
}


SECControlBar* CMainFrame::CreateControlBar( UINT *pnID,
																						 const CString &rstrTitle,
																						 const UINT nStyle,
																						 const UINT nPlace,
																						 const float fRate,
																						 const int nWidth )
{
	NI_ASSERT( pnID != 0, "CMainFrame::CreateControlBar() pnID == 0" );

	DWORD dwDWStyle = WS_CHILD | WS_VISIBLE | CBRS_LEFT | CBRS_TOOLTIPS | CBRS_SIZE_DYNAMIC;
	DWORD dwDWStyleEx = CBRS_EX_COOL | CBRS_EX_BORDERSPACE;
	//	
	( *pnID ) = SECControlBar::GetUniqueBarID( this, ( *pnID ) );
	CDefaultDockingWindow *pwndDefaultDockingWindow = new CDefaultDockingWindow();
	if ( pwndDefaultDockingWindow->Create( this, rstrTitle, dwDWStyle, dwDWStyleEx, ( *pnID ) ) )
	{
		pwndDefaultDockingWindow->EnableDocking( nStyle );
		DockControlBarEx( pwndDefaultDockingWindow,
											nPlace,
											0,
											0,
											fRate,
											nWidth );
		return pwndDefaultDockingWindow;
	}
	else
	{
		delete pwndDefaultDockingWindow;
		return 0;
	}
}


bool CMainFrame::SetControlBarWindowContents( SECControlBar* _pwndDockingWindow, class CWnd *pwndContents )
{
	if ( _pwndDockingWindow )
	{
		if ( CDefaultDockingWindow *pwndDockingWindow = checked_cast<CDefaultDockingWindow*>( _pwndDockingWindow ) )
		{
			pwndDockingWindow->pwndContents = pwndContents;
			return true;
		}
	}
	return false;
}


bool CMainFrame::AddMenuResources( vector<UINT> &rIDs )
{
	if ( SECToolBarManager* pToolBarMgr = static_cast<SECToolBarManager*>( m_pControlBarManager ) )
	{
		switch( rIDs.size() )
		{
			case 1:
				pToolBarMgr->SetMenuInfo( 1, rIDs[0] );
				LoadAdditionalMenus( 1, rIDs[0] );
				break;
			case 2:
				pToolBarMgr->SetMenuInfo( 2, rIDs[0], rIDs[1] );
				LoadAdditionalMenus( 2, rIDs[0], rIDs[1] );
				break;
			case 3:
				pToolBarMgr->SetMenuInfo( 3, rIDs[0], rIDs[1], rIDs[2] );
				LoadAdditionalMenus( 3, rIDs[0], rIDs[1], rIDs[2] );
				break;
			case 4:
				pToolBarMgr->SetMenuInfo( 4, rIDs[0], rIDs[1], rIDs[2], rIDs[3] );
				LoadAdditionalMenus( 4, rIDs[0], rIDs[1], rIDs[2], rIDs[3] );
				break;
			case 5:
				pToolBarMgr->SetMenuInfo( 5, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4] );
				LoadAdditionalMenus( 5, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4] );
				break;
			case 6:
				pToolBarMgr->SetMenuInfo( 6, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5] );
				LoadAdditionalMenus( 6, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5] );
				break;
			case 7:
				pToolBarMgr->SetMenuInfo( 7, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6] );
				LoadAdditionalMenus( 7, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6] );
				break;
			case 8:
				pToolBarMgr->SetMenuInfo( 8, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7] );
				LoadAdditionalMenus( 8, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7] );
				break;
			case 9:
				pToolBarMgr->SetMenuInfo( 9, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8] );
				LoadAdditionalMenus( 9, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8] );
				break;
			case 10:
				pToolBarMgr->SetMenuInfo( 10, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9] );
				LoadAdditionalMenus( 10, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9] );
				break;
			case 11:
				pToolBarMgr->SetMenuInfo( 11, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10] );
				LoadAdditionalMenus( 11, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10] );
				break;
			case 12:
				pToolBarMgr->SetMenuInfo( 12, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11] );
				LoadAdditionalMenus( 12, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11] );
				break;
			case 13:
				pToolBarMgr->SetMenuInfo( 13, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11], rIDs[12] );
				LoadAdditionalMenus( 13, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11], rIDs[12] );
				break;
			case 14:
				pToolBarMgr->SetMenuInfo( 14, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11], rIDs[12], rIDs[13] );
				LoadAdditionalMenus( 14, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11], rIDs[12], rIDs[13] );
				break;
			case 15:
				pToolBarMgr->SetMenuInfo( 15, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11], rIDs[12], rIDs[13], rIDs[14] );
				LoadAdditionalMenus( 15, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11], rIDs[12], rIDs[13], rIDs[14] );
				break;
			case 16:
				pToolBarMgr->SetMenuInfo( 16, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11], rIDs[12], rIDs[13], rIDs[14], rIDs[15] );
				LoadAdditionalMenus( 16, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11], rIDs[12], rIDs[13], rIDs[14], rIDs[15] );
				break;
			case 17:
				pToolBarMgr->SetMenuInfo( 17, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11], rIDs[12], rIDs[13], rIDs[14], rIDs[15], rIDs[16] );
				LoadAdditionalMenus( 17, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11], rIDs[12], rIDs[13], rIDs[14], rIDs[15], rIDs[16] );
				break;
			case 18:
				pToolBarMgr->SetMenuInfo( 18, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11], rIDs[12], rIDs[13], rIDs[14], rIDs[15], rIDs[16], rIDs[17] );
				LoadAdditionalMenus( 18, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11], rIDs[12], rIDs[13], rIDs[14], rIDs[15], rIDs[16], rIDs[17] );
				break;
			case 19:
				pToolBarMgr->SetMenuInfo( 19, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11], rIDs[12], rIDs[13], rIDs[14], rIDs[15], rIDs[16], rIDs[17], rIDs[18] );
				LoadAdditionalMenus( 19, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11], rIDs[12], rIDs[13], rIDs[14], rIDs[15], rIDs[16], rIDs[17], rIDs[18] );
				break;
			case 20:
				pToolBarMgr->SetMenuInfo( 20, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11], rIDs[12], rIDs[13], rIDs[14], rIDs[15], rIDs[16], rIDs[17], rIDs[18], rIDs[19] );
				LoadAdditionalMenus( 20, rIDs[0], rIDs[1], rIDs[2], rIDs[3], rIDs[4], rIDs[5], rIDs[6], rIDs[7], rIDs[8], rIDs[9], rIDs[10], rIDs[11], rIDs[12], rIDs[13], rIDs[14], rIDs[15], rIDs[16], rIDs[17], rIDs[18], rIDs[19] );
				break;
			case 0:
			default:
				NI_ASSERT( 0, "Can't support over max menus or empty menus" );
				return false;
		}
		ShowMenu( rIDs[0] );
		return true;
	}
	return false;
}


void CMainFrame::ShowMenu( const UINT nResourceID )
{
	m_nIDCurMenuRsrc = nResourceID;
	if ( ::IsWindow( m_pMenuBar->m_hWnd ) )
	{
		// Switch to the new menu	
		m_pMenuBar->SwitchMenu( m_nIDCurMenuRsrc );
		// And force the toolbar to be redrawn
		RecalcLayout();
		m_pMenuBar->Invalidate( TRUE );
	}
}


bool CMainFrame::AddToolBarResource( const UINT nStandartResourceID, const UINT nLargeResourceID )
{
	if ( SECToolBarManager* pToolBarMgr = static_cast<SECToolBarManager*>( m_pControlBarManager ) )
	{
		return pToolBarMgr->AddToolBarResource( MAKEINTRESOURCE( nStandartResourceID ),
																						MAKEINTRESOURCE( nLargeResourceID ) );
	}
	return false;
}


void CMainFrame::CreateToolBar( UINT *pnID,
																const CString &rstrTitle,
																const UINT nButtonCount,
																const UINT* pButtonIDMap,
																const DWORD dwAlignment,
																const UINT nStyle,
																const bool bDocked,
																const bool bVisible,
																const bool bMainToolBar )
{
	if ( SECToolBarManager* pToolBarMgr = static_cast<SECToolBarManager*>( m_pControlBarManager ) )
	{
		NI_ASSERT( pnID != 0, "CMainFrame::CreateToolBar() pnID == 0" );

		if ( ( *pnID ) == 0xFFFFFFFF )
		{
			( *pnID ) = nFreeToolbarID;
			++nFreeToolbarID;
		}

		pToolBarMgr->DefineDefaultToolBar( *pnID,
																			 rstrTitle,
																			 nButtonCount,
																			 const_cast<UINT*>( pButtonIDMap ),
																			 dwAlignment,
																			 nStyle,
																			 bMainToolBar ? TOOLBAR_CONTROL_ID_TO_ARRANGE : ( *pnID ) - 1,
																			 bDocked,
																			 bVisible );
	}
}


SECCustomToolBar* CMainFrame::GetToolBar( UINT nID )
{
	if ( SECToolBarManager* pToolBarMgr = static_cast<SECToolBarManager*>( m_pControlBarManager ) )
	{
		return pToolBarMgr->ToolBarFromID( nID );
	}
	return 0;
}


void CMainFrame::SetStatusBarText( int nPaneIndex, const string &szText )
{
	wndStatusBar.SetPaneText( nPaneIndex, szText.c_str() );
}


void CMainFrame::SetWindowTitle( const SSWTParams &rSWTParams )
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	const int MAX_NAME_SIZE = 133;
	//
	bool bModified = false;
	if ( rSWTParams.dwFlags & SWT_MOD )
	{
		string szMOD = rSWTParams.szMOD;
		if ( rSWTParams.bFillMODFromBase )
		{
			NMOD::SMOD mod;
			NMOD::GetAttachedMOD( &mod );
			if ( mod.wszName.empty() )
			{
				szMOD.clear();
			}
			else
			{
				CString strMOD;
				Unicode2MBSC( &strMOD, mod.wszName, ::GetACP() );
				szMOD = strMOD;
			}
		}
		if ( currentSWTParams.szMOD != szMOD )
		{
			currentSWTParams.szMOD = szMOD;
			bModified = true;
		}
	}
	if ( rSWTParams.dwFlags & SWT_TYPE )
	{
		if ( currentSWTParams.szType.empty() )
		{
			currentSWTParams.szType = pUserData->constUserData.szMainObjectType;
		}
		if ( currentSWTParams.szType != rSWTParams.szType )
		{
			currentSWTParams.szType = rSWTParams.szType;
			bModified = true;
		}
		if ( currentSWTParams.szType == pUserData->constUserData.szMainObjectType )
		{
			currentSWTParams.szType.clear();
		}
	}
	if ( rSWTParams.dwFlags & SWT_OBJECT )
	{
		if ( currentSWTParams.szObject != rSWTParams.szObject )
		{
			currentSWTParams.szObject = rSWTParams.szObject;
			bModified = true;
		}
	}
	if ( rSWTParams.dwFlags & SWT_PARAMS )
	{
		if ( currentSWTParams.szParams != rSWTParams.szParams )
		{
			currentSWTParams.szParams = rSWTParams.szParams;
			bModified = true;
		}
	}
	if ( rSWTParams.dwFlags & SWT_MODIFIED )
	{
		if ( currentSWTParams.bModified != rSWTParams.bModified )
		{
			currentSWTParams.bModified = rSWTParams.bModified;
			bModified = true;
		}
	}
	if ( bModified )
	{
		string szFilePath;
		string szFileName;
		string szFileExtention;
		CStringManager::SplitFileName( &szFilePath, &szFileName, &szFileExtention, currentSWTParams.szObject ); 
		//
		string szTitle = szFilePath + szFileName;
		string szExtention = szFileExtention;
		//
		if ( !currentSWTParams.szType.empty() )
		{
			szTitle = currentSWTParams.szType + ":" + szTitle;
		}
		if ( currentSWTParams.bModified )
		{
			szExtention += "*";
		}
		if ( !currentSWTParams.szParams.empty() )
		{
			szExtention += " " + currentSWTParams.szParams;
		}
		if ( !currentSWTParams.szMOD.empty() )
		{
			szExtention += " MOD: " + currentSWTParams.szMOD;
		}
		//
		if ( ( szTitle.size() + szExtention.size() ) > MAX_NAME_SIZE )
		{
			if ( ( MAX_NAME_SIZE - szExtention.size() - 4 ) >= 0 )
			{
				szTitle = szTitle.substr( 0, MAX_NAME_SIZE - szExtention.size() - 4 ) + "..." + szTitle.substr( szTitle.size() - 1 );
			}
		}
		if ( szTitle.empty() && szExtention.empty() )
		{
			szTitle = pUserData->constUserData.szApplicationTitle;
		}
		else
		{
			szTitle = pUserData->constUserData.szApplicationTitle + " - [" + szTitle + szExtention + "]";
		}
		SetWindowText( szTitle.c_str() );
	}
}


void CMainFrame::Log( ELogOutputType eLogOutputType, const string &szText )
{
	if ( ( eLogOutputType == LT_ERROR ) && !wndLog.IsVisible() )
	{
		//ShowControlBar( &wndLog, true, true );
	}
	wndLog.Log( eLogOutputType, szText );
}


void CMainFrame::ClearLog()
{
	wndLog.ClearLog();
}


void CMainFrame::SaveObjectStorage( int nGDBBrowserID )
{
	Singleton<IUserDataContainer>()->Get()->nFocusedGDBBrowserID = nGDBBrowserID;
	RestoreObjectStorage();
}


void CMainFrame::RestoreObjectStorage()
{
	ICommandHandlerContainer *pCommandHandlerContainer = Singleton<ICommandHandlerContainer>();
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	for ( list<CDWGDBBrowser*>::iterator itGDBBrowser = gdbBrowserList.begin(); itGDBBrowser != gdbBrowserList.end(); ++itGDBBrowser )
	{
		if ( *itGDBBrowser )
		{
			if( ( *itGDBBrowser )->GetDWGDBBrowserID() == pUserData->nFocusedGDBBrowserID )
			{
				pCommandHandlerContainer->Set( CHID_OBJECT_STORAGE, ( *itGDBBrowser )->GetContents() );
				pCommandHandlerContainer->Set( CHID_MAIN, ( *itGDBBrowser ) );
			}
		}
	}
}


bool CMainFrame::BrowseLink( string *pszResult, const string &rszInitialValue, const SPropertyDesc* pPropertyDesc, bool bMultiRef, bool bEnableEdit )
{
	NI_ASSERT( pPropertyDesc != 0, "CMainFrame::CreateToolBar() pPropertyDesc == 0" );
	//
	string szValues = pPropertyDesc->szStringParam;
	NStr::ToLowerASCII( &szValues );
	//
	const int	nWidth = CStringManager::GetIntValueFromString( szValues, PCSPL_WIDTH, 0, PCSP_DIVIDERS, 0 );
	const int	nHeight = CStringManager::GetIntValueFromString( szValues, PCSPL_HEIGHT, 0, PCSP_DIVIDERS, 0 );
	const bool bTextEditor = CStringManager::GetBoolValueFromString( szValues, PCSPL_EDITOR, 0, PCSP_DIVIDERS, false );
	//
	CPCDBLinkDialog pcDBLinkDialog( CPCDBLinkDialog::TYPE_LINK, bMultiRef, bTextEditor, nWidth, nHeight, AfxGetMainWnd() );
	pcDBLinkDialog.SetSelectedTables( pPropertyDesc->refTypes );
	if ( !pPropertyDesc->refTypes.empty() )
	{
		string szTableName;
		string szObjectName;
		const int nPos = rszInitialValue.find( TYPE_SEPARATOR_CHAR );
		if ( nPos >= 0 )
		{
			szTableName = rszInitialValue.substr( 0, nPos );
			szObjectName = rszInitialValue.substr( nPos + 1 );
		}
		else
		{
			szTableName = pPropertyDesc->refTypes.begin()->first;
			szObjectName = rszInitialValue;
		}
		//
		SUserData::CRefPathMap &rRefPathMap = Singleton<IUserDataContainer>()->Get()->refPathMap;
		string szRefKey;
		CreateRefKey( &szRefKey, pPropertyDesc );
		//
		if ( szObjectName.empty() )
		{
			string szRefValue = rRefPathMap[szRefKey];
			string szLocalTableName;
			CStringManager::GetTypeAndNameFromRefValue( &szLocalTableName, &szObjectName, szRefValue, TYPE_SEPARATOR_CHAR, szTableName );
			if ( !szLocalTableName.empty() )
			{
				szTableName = szLocalTableName;
			}
		}
		//
		pcDBLinkDialog.SetCurrentTable( szTableName );
		pcDBLinkDialog.SetCurrentObject( szObjectName );
		pcDBLinkDialog.EnableEdit( bEnableEdit );
		//
		if ( ( pcDBLinkDialog.DoModal() == IDOK ) && bEnableEdit && ( pszResult != 0 ) )
		{
			pcDBLinkDialog.GetCurrentTable( &szTableName );
			pcDBLinkDialog.GetCurrentObject( &szObjectName );
			//
			string szRefValue;
			CStringManager::GetRefValueFromTypeAndName( &szRefValue, szTableName, szObjectName, TYPE_SEPARATOR_CHAR );
			rRefPathMap[szRefKey] = szRefValue;
			//
			if ( pcDBLinkDialog.IsEmpty() )
			{
				pszResult->clear();
			}
			else
			{
				( *pszResult ) = szRefValue;
			}
			return true;
		}
	}
	return false;
}


bool CMainFrame::BrowseForObject( CDBID *pObjectDBID, string *pszObjectTypeName, bool bEnableEdit, bool bEnableEmpty )
{
	const string szObjectTypeName = ( pszObjectTypeName != 0 ) ? ( *pszObjectTypeName ) : string();
	//
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IEditorContainer *pEditorContainer = Singleton<IEditorContainer>();
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	SUserData::CRefPathMap &rRefPathMap = pUserData->refPathMap;
	const string szRefKey = StrFmt( "_OPEN:%s", szObjectTypeName.c_str() );
	//
	CPCDBLinkDialog pcDBLinkDialog( bEnableEmpty ? CPCDBLinkDialog::TYPE_LINK : CPCDBLinkDialog::TYPE_OPEN, false, false, 0, 0, AfxGetMainWnd() );
	CTableSet tableSet;
	if ( szObjectTypeName.empty() )
	{
		if ( CPtr<IManipulator> pTableManipulator = pResourceManager->CreateTableManipulator() )
		{
			if ( CPtr<IManipulatorIterator> pTableManipulatorIterator = pTableManipulator->Iterate( true, ECT_NO_CACHE ) )
			{
				string szName;
				while ( !pTableManipulatorIterator->IsEnd() )
				{
					pTableManipulatorIterator->GetName( &szName );
					if ( pEditorContainer->CanCreate( szName ) )
					{
						InsertHashSetElement( &tableSet, szName );
					}
					pTableManipulatorIterator->Next();
				}
			}
		}
	}
	else
	{
		InsertHashSetElement( &tableSet, szObjectTypeName );
	}
	pcDBLinkDialog.SetSelectedTables( tableSet );
	pcDBLinkDialog.SetCurrentTable( szObjectTypeName );
	{
		string szRefValue = rRefPathMap[szRefKey];
		string szTableName;
		string szObjectName;
		CStringManager::GetTypeAndNameFromRefValue( &szTableName, &szObjectName, szRefValue, TYPE_SEPARATOR_CHAR, szTableName );
		if ( !szTableName.empty() )
		{
			pcDBLinkDialog.SetCurrentTable( szTableName );
		}
		if ( !szObjectName.empty() )
		{
			pcDBLinkDialog.SetCurrentObject( szObjectName );
		}
	}
	pcDBLinkDialog.EnableEdit( bEnableEdit );
	const bool bResult = ( pcDBLinkDialog.DoModal() == IDOK );
	if ( bResult )
	{
		string szTableName;
		string szObjectName;
		pcDBLinkDialog.GetCurrentTable( &szTableName );
		pcDBLinkDialog.GetCurrentObject( &szObjectName );
		if ( !szObjectName.empty() )
		{
			string szRefValue;
			CStringManager::GetRefValueFromTypeAndName( &szRefValue, szTableName, szObjectName, TYPE_SEPARATOR_CHAR );
			rRefPathMap[szRefKey] = szRefValue;
		}
		if ( pObjectDBID != 0 )
		{
			if ( !pcDBLinkDialog.IsEmpty() )
			{
				( *pObjectDBID ) = CDBID( szObjectName );
			}
			else
			{
				pObjectDBID->Clear();
			}
		}
		if ( pszObjectTypeName != 0 )
		{
			( *pszObjectTypeName ) = szTableName;
		}
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	return bResult;
}


bool CMainFrame::HandleCommand( UINT nCommandID, DWORD dwData )
{
	switch( nCommandID )
	{
		case ID_VIEW_SHOW_PROPERTY_BROWSER:
		{	
			ShowControlBar( &wndPropertyBrowser, ( dwData != 0 ), true );
			return true;
		}
		case ID_VIEW_SHOW_LOG:
		{
			ShowControlBar( &wndLog, ( dwData != 0 ), true );
			return true;
		}
		case ID_VIEW_SHOW_GDB_BROWSER:
		{
			if ( !gdbBrowserList.empty() && ( ( *( gdbBrowserList.begin() ) ) != 0 ) )
			{
				ShowControlBar( *( gdbBrowserList.begin() ), ( dwData != 0 ), true );
			}
			return true;
		}
		case ID_VIEW_SAVE_CHANGES:
		{
			return SaveChanges( dwData > 0 );
		}
		case ID_VIEW_RELOAD:
		{
			ReloadData();
			return true;
		}
		default:
			return false;
	}
}


bool CMainFrame::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CMainFrame::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CMainFrame::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_VIEW_SHOW_PROPERTY_BROWSER:
		case ID_VIEW_SHOW_LOG:
		case ID_VIEW_SHOW_GDB_BROWSER:
		case ID_VIEW_SAVE_CHANGES:
		case ID_VIEW_RELOAD:
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		default:
			return false;
	}
}


bool CMainFrame::SaveChanges( bool bShowConfirmDialog )
{
	if ( IEditorContainer *pEditorContainer = Singleton<IEditorContainer>() )
	{
		bool bModified = ( Singleton<IEditorContainer>()->IsModified() || Singleton<IResourceManager>()->CanSyncDB() );
		bShowConfirmDialog = bShowConfirmDialog && bModified;
		bool bConfirmed = true;
		if ( bShowConfirmDialog	)
		{
			if ( pEditorContainer->GetActiveEditor() )
			{
				SObjectSet objectSet;
				pEditorContainer->GetActiveEditor()->GetView()->GetObjectSet( &objectSet );
				//
				CString strMessagePattern;
				strMessagePattern.LoadString( IDS_CONFIRM_SAVE_MESSAGE_LONG );
				string szName;
				{
					CStringManager::GetRefValueFromTypeAndName( &szName, objectSet.szObjectTypeName, objectSet.objectNameSet.begin()->first.ToString(), TYPE_SEPARATOR_CHAR );
				}
				CString strMessage;
				strMessage.Format( strMessagePattern, szName.c_str() );
				const int nButtonPressed = ::MessageBox( Singleton<IMainFrameContainer>()->GetSECWorkbook()->GetSafeHwnd(), strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON2 );
				if ( nButtonPressed == IDCANCEL )
				{
					return false;
				}
				bConfirmed = ( nButtonPressed == IDYES );
			}
			else
			{
				CString strMessagePattern;
				strMessagePattern.LoadString( IDS_CONFIRM_SAVE_MESSAGE_SHORT );
				const int nButtonPressed = ::MessageBox( Singleton<IMainFrameContainer>()->GetSECWorkbook()->GetSafeHwnd(), strMessagePattern, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON2 );
				if ( nButtonPressed == IDCANCEL )
				{
					return false;
				}
				bConfirmed = ( nButtonPressed == IDYES );
			}
		}
		//
		if ( bModified && bConfirmed )
		{
			NProgress::Create( true );
			CString strPM;
			strPM.LoadString( IDS_PM_SAVE );
			NProgress::SetMessage( string( strPM ) );
			NProgress::SetRange( 0, pEditorContainer->GetActiveEditor() ? 2 : 1 );
		}
		CWaitCursor waitCursor;
		if ( pEditorContainer->GetActiveEditor() )	
		{
			SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
			SObjectSet objectSet;
			pEditorContainer->GetActiveEditor()->GetView()->GetObjectSet( &objectSet );
			const bool bMainObject = ( objectSet.szObjectTypeName == pUserData->constUserData.szMainObjectType );
			string szName;
			CStringManager::CreateRecentListName( &szName, objectSet, bMainObject );
			CStringManager::AddToRecentList( szName, bMainObject );
		}
		//
		if ( bModified && bConfirmed )
		{
			if ( pEditorContainer->GetActiveEditor() )
			{
				pEditorContainer->Save( true );
				NProgress::SetPosition( 1 );
			}
			Singleton<IResourceManager>()->SyncDB();
		}
		else if ( bModified && !bConfirmed )
		{
			if ( pEditorContainer->GetActiveEditor() )
			{
				pEditorContainer->Save( false );
				NProgress::SetPosition( 2 );
			}
			Singleton<IResourceManager>()->ResetCache();
			ReloadData();
		}
		if ( bModified && bConfirmed )
		{
			NProgress::Destroy();	
		}
		//
		{
			bModified = ( Singleton<IEditorContainer>()->IsModified() || Singleton<IResourceManager>()->CanSyncDB() );
			//
			SSWTParams swtParams;
			swtParams.dwFlags = SWT_MODIFIED;
			swtParams.bModified = bModified;
			Singleton<IMainFrameContainer>()->Get()->SetWindowTitle( swtParams );
		}
	}
	return true;	
}


void CMainFrame::ReloadData()
{
	for ( list<CDWGDBBrowser*>::iterator itDWGDBBrowser = gdbBrowserList.begin(); itDWGDBBrowser != gdbBrowserList.end(); ++itDWGDBBrowser )
	{
		if ( ( *itDWGDBBrowser ) )
		{
			( *itDWGDBBrowser )->HandleCommand( ID_MAIN_RELOAD, 0 );
		}
	}
}


void CMainFrame::CreateProgressDialog()
{
	hwndPreviousFocusedWindow = ::GetFocus();
	if ( ::IsWindow( progressDialog.GetSafeHwnd() ) )
	{
		progressDialog.ShowWindow( SW_SHOW );	
		progressDialog.UpdateControls();
	}
	else
	{
		progressDialog.Create( CProgressDialog::IDD, this );
	}
}


void CMainFrame::DestroyProgressDialog()
{
	if ( ::IsWindow( progressDialog.GetSafeHwnd() ) )
	{
		progressDialog.DestroyWindow();
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	if ( ::IsWindow( hwndPreviousFocusedWindow ) )
	{
		::SetFocus( hwndPreviousFocusedWindow );
	}
}


void CMainFrame::SetProgressDialogTitle( const string &rszTitle )
{
	if ( ::IsWindow( progressDialog.GetSafeHwnd() ) )
	{
		progressDialog.SetProgressTitle( rszTitle );
	}
}


void CMainFrame::SetProgressDialogMessage( const string &rszMessage )
{
	if ( ::IsWindow( progressDialog.GetSafeHwnd() ) )
	{
		progressDialog.SetProgressMessage( rszMessage );
	}
}


void CMainFrame::SetProgressDialogRange( int nStart, int nFinish )
{
	if ( ::IsWindow( progressDialog.GetSafeHwnd() ) )
	{
		progressDialog.SetProgressRange( nStart, nFinish );
	}
}


void CMainFrame::SetProgressDialogPosition( int nPosition )
{
	if ( ::IsWindow( progressDialog.GetSafeHwnd() ) )
	{
		progressDialog.SetProgressPosition( nPosition );
	}
}


void CMainFrame::IterateProgressDialogPosition()
{
	if ( ::IsWindow( progressDialog.GetSafeHwnd() ) )
	{
		progressDialog.IterateProgressPosition();
	}
}

// basement storage  


/**

void CMainFrame::ExecuteCommand( 	bool bCloseEditor, const wstring &rszCommandString )
{
	if ( bCloseEditor )
	{
		Singleton<IEditorContainer>()->DestroyActiveEditor( false );
	}
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_GAME_INPUT, 0 );
	NGlobal::ProcessCommand( rszCommandString );
}
/**/
/**
WINDOWPLACEMENT wp;
wp.length = sizeof( WINDOWPLACEMENT );
GetWindowPlacement( &wp );
wp.showCmd = SW_SHOWMAXIMIZED;
SetWindowPlacement( &wp );
//ShowWindow( SW_SHOWNORMAL );
//ShowWindow( SW_SHOWNORMAL );
if ( params.bMaximized )
{
	PostMessage( WM_SYSCOMMAND, SC_MAXIMIZE, 0 );
}
/**/

