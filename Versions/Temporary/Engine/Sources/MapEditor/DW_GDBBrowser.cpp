#include "stdafx.h"
#include "..\mapeditorlib\commandhandlerdefines.h"
#include "..\mapeditorlib\resourcedefines.h"
#include "wmdefines.h"

#include "../libdb/ResourceManager.h"
#include "SelectTablesDialog.h"
#include "DW_GDBBrowser.h"
#include "Tree_GDBBrowser.h"
#include "..\MapEditorLib\Tools_HashSet.h"
#include "..\MapEditorLib\StringManager.h"
#include "..\MapEditorLib\Interface_MainFrame.h"
#include "..\MapEditorLib\Interface_ChildFrame.h"
#include "..\MapEditorLib\Interface_Editor.h"
#include "..\MapEditorLib\Interface_Exporter.h"
#include "..\MapEditorLib\Interface_MOD.h"

//#include "..\MapEditorLib\Tools_SysCodes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDWGDBBrowser::CDWGDBBrowser( int _nGDBBrowserID ) : bCreateControls( true ), nGDBBrowserID( _nGDBBrowserID ), wndContents( nGDBBrowserID )
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDWGDBBrowser::~CDWGDBBrowser()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CDWGDBBrowser, SECControlBar)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	//ON_MESSAGE( TCM_TABSEL, OnTabSelected )
	ON_MESSAGE( WM_GDB_BROWSER, OnTabSelected )
	ON_CBN_SELCHANGE( IDC_TREE_GDB_BROWSER, OnTabSelected )
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CDWGDBBrowser::OnCreate( LPCREATESTRUCT pCreateStruct ) 
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	selectedTables = pUserData->tableSetMap[nGDBBrowserID];
	szCurrentTable = pUserData->szCurrentTableMap[nGDBBrowserID];

	if ( SECControlBar::OnCreate( pCreateStruct ) == -1 )
	{
		return -1;
	}
	//

	if ( !wndContents.Create( this, IDC_TREE_GDB_BROWSER ) )
	{
		return -1;
	}
	//
	if ( !wndEmptyContents.Create( 0,
																 0,
																 AFX_WS_DEFAULT_VIEW,
																 CRect( 0, 0, 0, 0 ),
																 this,
																 IDC_EMPTY_GDB_BROWSER,
																 0 ) )
	{
		return -1;
	}
	//
	SetTableManipulator( Singleton<IResourceManager>()->CreateTableManipulator() );
	//
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::OnDestroy()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_OBJECT_STORAGE, &wndContents );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MAIN, this );

	SECControlBar::OnDestroy();
	//
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	pUserData->tableSetMap[nGDBBrowserID] = selectedTables;
	pUserData->szCurrentTableMap[nGDBBrowserID] = szCurrentTable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::OnSize( UINT nType, int cx, int cy ) 
{
	SECControlBar::OnSize( nType, cx, cy );
	//
	CRect insideRect;
	GetInsideRect( insideRect );
	//
	wndContents.MoveWindow( insideRect );
	/**
	if ( wndContents.GetSafeHwnd() != NULL )
	{
		wndContents.SetWindowPos( 0,
															insideRect.left,
															insideRect.top,
															insideRect.Width(),
															insideRect.Height(),
															SWP_NOZORDER | SWP_NOACTIVATE );
	}
	/**/
	//
	if ( wndEmptyContents.GetSafeHwnd() != NULL )
	{
		wndEmptyContents.SetWindowPos( 0,
															insideRect.left,
															insideRect.top,
															insideRect.Width(),
															insideRect.Height(),
															SWP_NOZORDER | SWP_NOACTIVATE );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CDWGDBBrowser::OnGripperClose()
{
	/**
	if ( wndContents.GetTabCount() != 0 )
	{
		if ( wndContents.IsFocused() )
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_SET_FOCUS, 0 );
		}
	}
	/**/
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::OnLButtonDown( UINT nFlags, CPoint point )
{
	SECControlBar::OnLButtonDown( nFlags, point );
	//
	if ( nGDBBrowserID != -1 )
	{
		Singleton<IMainFrameContainer>()->Get()->SaveObjectStorage( nGDBBrowserID );
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::OnRButtonDown( UINT nFlags, CPoint point )
{
	SECControlBar::OnRButtonDown( nFlags, point );
	//
	if ( nGDBBrowserID != -1 )
	{
		Singleton<IMainFrameContainer>()->Get()->SaveObjectStorage( nGDBBrowserID );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::OnRButtonUp( UINT nFlags, CPoint point )
{
	SECControlBar::OnRButtonUp( nFlags, point );

	CMenu mainPopupMenu;
	mainPopupMenu.LoadMenu( IDM_MAIN_CONTEXT_MENU );
	CMenu *pMenu = mainPopupMenu.GetSubMenu( MCMN_DW_GDB_BROWSER );
	if ( pMenu )
	{
		ClientToScreen( &point ); 
		pMenu->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, Singleton<IMainFrameContainer>()->GetSECWorkbook(), 0 );
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	}
	mainPopupMenu.DestroyMenu();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CDWGDBBrowser::OnTabSelected( WPARAM wParam, LPARAM lParam )
{
	OnTabSelected();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//LRESULT CDWGDBBrowser::OnTabSelected( WPARAM wParam, LPARAM lParam )
void CDWGDBBrowser::OnTabSelected()
{
	if ( !bCreateControls )
	{
		wndContents.SwitchTabs();
		CRect insideRect;
		GetInsideRect( insideRect );
		wndContents.MoveWindow( insideRect );
		//
		CTreeGDBBrowserBase* pwndTreeGBDBrowserBase = 0;
		wndContents.GetActiveTabName( &szCurrentTable );
		wndContents.GetActiveTab( &pwndTreeGBDBrowserBase );
		if ( !pwndTreeGBDBrowserBase->IsTreeCreated() )
		{
			SObjectSet collectionObjectSet;
			collectionObjectSet.szObjectTypeName = szCurrentTable;
			InsertHashSetElement( &( collectionObjectSet.objectNameSet ), CDBID( VIEW_COLLECTION_ID ) );
			//
			pwndTreeGBDBrowserBase->SetViewManipulator( Singleton<IResourceManager>()->CreateFolderManipulator( szCurrentTable ), collectionObjectSet, string() );
			pwndTreeGBDBrowserBase->CreateTree();
			//DebugTrace( "CDWGDBBrowser::OnTabSelected(): wParam: 0x%X(%u), lParam: 0x%X\n", wParam, wParam, lParam );
		}
		else
		{
			pwndTreeGBDBrowserBase->UpdateSelectionManipulator( true );
		}
	}
	//return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::SetTableManipulator( IManipulator *_pTableManipulator )
{
	if ( pTableManipulator != _pTableManipulator )
	{
		pTableManipulator = _pTableManipulator;
		tables.clear();
		if ( !pTableManipulator )
		{
			return;
		}
		if ( CPtr<IManipulatorIterator> pTableManipulatorIterator = pTableManipulator->Iterate( true, ECT_CACHE_LOCAL ) )
		{
			string szName;
			while ( !pTableManipulatorIterator->IsEnd() )
			{
				pTableManipulatorIterator->GetName( &szName );
				tables.push_back( szName );
				pTableManipulatorIterator->Next();
			}
		}
		CreateTabs();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::CreateTabs()
{
	if ( !IsWindow( m_hWnd ) )
	{
		return;
	}
	// соберем таблицы которые уже заполнены
	/**
	{
		hash_map<string, CTreeGDBBrowser*> existingTabs;
		//
		char pBuffer[0xFFF];
		bool bSelected = false;
		CWnd *pwnd = 0;
		void *pExtra = 0;
		//
		const int nTabCount = wndContents.GetTabsCount();
		for ( int nTabIndex = 0; nTabIndex < nTabCount; ++nTabIndex )
		{
			wndContents.GetTabInfo( nTabIndex, pBuffer, bSelected, pWnd, pExtra );
			const string szName( pBuffer );
			if ( selectedTables.find( szName ) != selectedTables.end() )
			{
				existingTabs[szName] = dynamic_cast<CTreeGDBBrowser*>( pwnd );
			}
		}
	}	
	/**/
	//создаем таблицы
	{
		bCreateControls = true;
		wndContents.RemoveAllTabs();
		// Необходимо сначало добавить все панели, а потом устанавливать активную
		CTreeGDBBrowser* pwndActiveTreeGBDBrowser = 0;
		for ( list<string>::const_iterator itTable = tables.begin(); itTable != tables.end(); ++itTable )
		{
			if ( selectedTables.find( *itTable ) != selectedTables.end() )
			{
				if ( CTreeGDBBrowser* pwndTreeGBDBrowser = wndContents.AddNewTab( static_cast<CTreeGDBBrowser*>( 0 ), *itTable ) )
				{
					if ( ( *itTable ) == szCurrentTable )
					{
						pwndActiveTreeGBDBrowser = pwndTreeGBDBrowser;
					}
					pwndTreeGBDBrowser->SetPCDialogCommandHandlerID( CHID_PC_DIALOG, false );
				}
			}
		}
		bCreateControls = false;
		if ( pwndActiveTreeGBDBrowser )
		{
			wndContents.ActivateTab( pwndActiveTreeGBDBrowser );
		}
		else if ( wndContents.GetTabCount() > 0 )
		{
			wndContents.ActivateTab( wndContents.GetTab( 0 ) );
		}
	}

	if ( wndContents.GetTabCount() > 0 )
	{
		wndContents.ShowWindow( SW_SHOW );
		wndEmptyContents.ShowWindow( SW_HIDE );
	}
	else
	{
		wndContents.ShowWindow( SW_HIDE );
		wndEmptyContents.ShowWindow( SW_SHOW );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::SelectTables()
{
	CSelectTablesDialog selectTablesDialog( this );
	
	selectTablesDialog.tables = tables;
	selectTablesDialog.selectedTables = selectedTables;
	
	if ( selectTablesDialog.DoModal() == IDOK )
	{
		selectedTables = selectTablesDialog.selectedTables;

		CreateTabs();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::ClearTable()
{
	/**
	if ( CPtr<IManipulator> pFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( szCurrentTable ) )
	{
		CString strMessage;
		strMessage.LoadString( IDS_TREE_GDB_BROWSE_CFM );
		if ( MessageBox( strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 ) == IDYES )
		{
			int nObjectsDeleted = 0;
			int nFoldersDeleted = 0;
			list<string> obectNameList;
			BeginWaitCursor();
			// remove unreferenced objects
			if ( CPtr<IManipulatorIterator> pFolderManipulatorIterator = pFolderManipulator->Iterate( true, ECT_NO_CACHE ) )
			{
				while ( !pFolderManipulatorIterator->IsEnd() )
				{
					string szType;
					pFolderManipulatorIterator->GetType( &szType );
					if ( szType == "object" )
					{
						string szObjectName;
						pFolderManipulatorIterator->GetName( &szObjectName );
						if ( !szObjectName.empty() )
						{
							obectNameList.push_back( szObjectName );
						}
					}
					pFolderManipulatorIterator->Next();
				}
			}
			for ( list<string>::const_iterator itObjectName = obectNameList.begin(); itObjectName != obectNameList.end(); ++itObjectName )
			{
				DebugTrace( "Delete object: <%s>", itObjectName->c_str() ); 
				if ( pFolderManipulator->RemoveNode( *itObjectName ) )
				{
					++nObjectsDeleted;
				}
			}
			// remove emply folders
			list<string> folderNameList;
			list<string> folderStack;
			if ( CPtr<IManipulatorIterator> pFolderManipulatorIterator = pFolderManipulator->Iterate( true, ECT_NO_CACHE ) )
			{
				while ( !pFolderManipulatorIterator->IsEnd() )
				{
					string szName;
					pFolderManipulatorIterator->GetName( &szName );
					if ( !szName.empty() )
					{
						string szType;
						pFolderManipulatorIterator->GetType( &szType );
						
						if ( szType == "folder" )
						{
							folderStack.push_back( szName );
						}
						else if ( szType == "object" )
						{
							for ( list<string>::iterator itName = folderStack.begin(); itName != folderStack.end(); )
							{
								if ( szName.compare( 0, itName->size(), *itName ) == 0 )
								{
									folderStack.erase( itName++ );
								}
								else
								{
									++itName;
								}
							}
							if ( !folderStack.empty() )
							{
								for ( list<string>::const_iterator itName = folderStack.end(); itName != folderStack.begin(); )
								{
									--itName;
									folderNameList.push_back( *itName );
								}
							}
							folderStack.clear();
						}
					}
					pFolderManipulatorIterator->Next();
				}
				if ( !folderStack.empty() )
				{
					for ( list<string>::const_iterator itName = folderStack.end(); itName != folderStack.begin(); )
					{
						--itName;
						folderNameList.push_back( *itName );
					}
				}
			}
			for ( list<string>::const_iterator itFolderName = folderNameList.begin(); itFolderName != folderNameList.end(); ++itFolderName )
			{
				DebugTrace( "Delete Folder: <%s>", itFolderName->c_str() ); 
				if ( pFolderManipulator->RemoveNode( *itFolderName ) )
				{
					++nFoldersDeleted;
				}
			}
			// messages and refresh folders
			EndWaitCursor();
			if ( nObjectsDeleted == 1 )
			{
				CreateTabs();
				CString strMessagePattern;
				strMessagePattern.LoadString( IDS_TREE_GDB_BROWSE_CFM_OBJECT_FOUND );
				strMessage.Format( strMessagePattern, nObjectsDeleted );
			}
			else if ( nObjectsDeleted > 1 )
			{
				CreateTabs();
				CString strMessagePattern;
				strMessagePattern.LoadString( IDS_TREE_GDB_BROWSE_CFM_OBJECTS_FOUND );
				strMessage.Format( strMessagePattern, nObjectsDeleted );
			}
			else
			{
				strMessage.LoadString( IDS_TREE_GDB_BROWSE_CFM_NO_OBJECT_FOUND );
			}
			MessageBox( strMessage, Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(), MB_ICONINFORMATION | MB_OK );
		}
	}
	/**/
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::SelectObjectSet( const SObjectSet &rObjectSet )
{
	if ( !rObjectSet.objectNameSet.empty() )
	{
		if ( !rObjectSet.objectNameSet.begin()->first.IsEmpty() )
		{
			Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[rObjectSet.szObjectTypeName].szCurrentObject = rObjectSet.objectNameSet.begin()->first.ToString();
		}
		//
		if ( 	selectedTables.find( rObjectSet.szObjectTypeName ) != selectedTables.end() )
		{
			CTreeGDBBrowserBase *pwndTab = wndContents.GetTab( rObjectSet.szObjectTypeName );
			if ( pwndTab != 0 )
			{
				pwndTab->SetStrongSelection();
				wndContents.ActivateTab( pwndTab );
			}
		}
		else
		{
			bCreateControls = true;
			InsertHashSetElement( &selectedTables, rObjectSet.szObjectTypeName );
			CTreeGDBBrowser* pwndTreeGBDBrowser = wndContents.AddNewTab( static_cast<CTreeGDBBrowser*>( 0 ), rObjectSet.szObjectTypeName );
			if ( pwndTreeGBDBrowser != 0 )
			{
				pwndTreeGBDBrowser->SetPCDialogCommandHandlerID( CHID_PC_DIALOG, false );
			}
			bCreateControls = false;
			pwndTreeGBDBrowser->SetStrongSelection();
			wndContents.ActivateTab( pwndTreeGBDBrowser );
			wndContents.ShowWindow( SW_SHOW );
			wndEmptyContents.ShowWindow( SW_HIDE );
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::New( const string &rszObjectTypeName )
{
	if ( !Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, true ) )
	{
		return;
	}
	//
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IBuilderContainer *pBuilderContainer = Singleton<IBuilderContainer>();
	IExporterContainer *pExporterContainer = Singleton<IExporterContainer>();
	IEditorContainer *pEditorContainer = Singleton<IEditorContainer>();
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	//	
	string szObjectTypeName = rszObjectTypeName;
	if ( szObjectTypeName.empty() )
	{
		return;
	}
	CString strObjectName;
	strObjectName.LoadString( IDS_TREE_GDB_BROWSE_NEW_MAIN_OBJECT );
	string szDefaultFolder;
	pBuilderContainer->GetDefaultFolder( szObjectTypeName, &szDefaultFolder );
	string szObjectName = szDefaultFolder + string( strObjectName );
	pFolderCallback->UniqueName( szObjectTypeName, &szObjectName );
	//
	bool bCanChangeObjectName = true;
	bool bNeedExport = false;
	bool bNeedEdit = true;
	//
	if ( pBuilderContainer->InsertObject( &szObjectTypeName, &szObjectName, true, &bCanChangeObjectName, &bNeedExport, &bNeedEdit ) )
	{
		if ( CPtr<IManipulator> pObjectManipulator = pResourceManager->CreateObjectManipulator( szObjectTypeName, szObjectName ) )
		{
			// Экспортируем вновь созданный объект
			if ( bNeedExport )
			{
				pExporterContainer->StartExport( szObjectTypeName, FORCE_EXPORT, START_EXPORT_TOOLS, EXPORT_REFERENCES );
				pExporterContainer->ExportObject( pObjectManipulator, szObjectTypeName, szObjectName, FORCE_EXPORT, EXPORT_REFERENCES );
				pExporterContainer->FinishExport( szObjectTypeName, FORCE_EXPORT, FINISH_EXPORT_TOOLS, EXPORT_REFERENCES );
			}
			if ( pEditorContainer->CanCreate( szObjectTypeName ) )
			{
				SObjectSet objectSet;
				objectSet.szObjectTypeName = szObjectTypeName;
				InsertHashSetElement( &( objectSet.objectNameSet ), CDBID( szObjectName ) );
				//
				bool bMainObject = ( szObjectTypeName == pUserData->constUserData.szMainObjectType );
				string szName;
				CStringManager::CreateRecentListName( &szName, objectSet, bMainObject );
				CStringManager::AddToRecentList( szName, bMainObject );
				SelectObjectSet( objectSet );
				pEditorContainer->Create( pObjectManipulator, objectSet );
			}
			pFolderCallback->ClearUndoData();
		}
		else
		{
			pFolderCallback->UndoChanges();
		}
		Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_REMOVE_INPUT, 0 );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::Open( const string &rszObjectTypeName )
{
	if ( !Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, true ) )
	{
		return;
	}
	//
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IEditorContainer *pEditorContainer = Singleton<IEditorContainer>();
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();

	CDBID objectDBID;
	string szObjectTypeName = rszObjectTypeName;
	if ( Singleton<IMainFrameContainer>()->Get()->BrowseForObject( &objectDBID, &szObjectTypeName, false, false ) )
	{
		if ( !objectDBID.IsEmpty() )
		{
			if ( pEditorContainer->CanCreate( szObjectTypeName ) )
			{
				if ( CPtr<IManipulator> pObjectManipulator = pResourceManager->CreateObjectManipulator( szObjectTypeName, objectDBID ) )
				{
					SObjectSet objectSet;
					objectSet.szObjectTypeName = szObjectTypeName;
					InsertHashSetElement( &( objectSet.objectNameSet ), objectDBID );
					//
					bool bMainObject = ( szObjectTypeName == pUserData->constUserData.szMainObjectType );
					string szName;
					CStringManager::CreateRecentListName( &szName, objectSet, bMainObject );
					CStringManager::AddToRecentList( szName, bMainObject );
					SelectObjectSet( objectSet );
					pEditorContainer->Create( pObjectManipulator, objectSet );
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::OnRecentList( int nIndex, bool bMainObject )
{
	if ( !Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, true ) )
	{
		return;
	}
	//
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IEditorContainer *pEditorContainer = Singleton<IEditorContainer>();
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	string szName;
	if ( bMainObject )
	{
		if ( ( nIndex >= 0 ) && ( nIndex < pUserData->recentList.size () ) )
		{
			int nNameIndex = 0;
			for ( SUserData::CRecentList::const_iterator itRecentName = pUserData->recentList.begin(); itRecentName != pUserData->recentList.end(); ++itRecentName )
			{
				if ( nIndex == nNameIndex )
				{
					szName = ( *itRecentName );
					break;
				}
				++nNameIndex;
			}
		}
	}
	else
	{
		if ( ( nIndex >= 0 ) && ( nIndex < pUserData->recentResourceList.size () ) )
		{
			int nNameIndex = 0;
			for ( SUserData::CRecentList::const_iterator itRecentName = pUserData->recentResourceList.begin(); itRecentName != pUserData->recentResourceList.end(); ++itRecentName )
			{
				if ( nIndex == nNameIndex )
				{
					szName = ( *itRecentName );
					break;
				}
				++nNameIndex;
			}
		}
	}
	if ( !szName.empty() )
	{
		SObjectSet objectSet;
		CStringManager::CreateObjectSet( &objectSet, szName, bMainObject );
		if ( pEditorContainer->CanCreate( objectSet.szObjectTypeName ) )
		{
			if ( CPtr<IManipulator> pObjectManipulator = pResourceManager->CreateObjectManipulator( objectSet.szObjectTypeName, objectSet.objectNameSet.begin()->first ) )
			{
				CStringManager::AddToRecentList( szName, bMainObject );
				SelectObjectSet( objectSet );
				pEditorContainer->Create( pObjectManipulator, objectSet );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::OnCheckOut()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::OnCheckIn()
{
	CWaitCursor wc;
	//
	Singleton<IEditorContainer>()->DestroyActiveEditor( true );
	Singleton<IChildFrameContainer>()->Destroy();
	//
	bool bResult = false;
	try
	{
		bResult = Singleton<IResourceManager>()->CheckIn();
	}
	catch (...)	{}
	//
	if ( bResult )
	{
		AfxMessageBox( "Check in completed successfully.\r\n\r\n"
				"MapEditor will exit now.",
				MB_OK | MB_ICONINFORMATION
				);
	}
	else
	{
		AfxMessageBox( "Check in failed.\r\n\r\n"
				"(Detailed error description stored in MapEditor.log file)\r\n\r\n"
				"This situation is CRITICAL.\r\n"
				"If you do not want to lose changes you've done in your local database,\r\n"
				"call to MapEditor's gurus immediately and do not try to start MapEditor again until they come.\r\n\r\n"
				"MapEditor will exit now.",
				MB_OK | MB_ICONERROR
				);
	}
	AfxGetMainWnd()->PostMessage( WM_COMMAND, ID_APP_EXIT, 0 );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::OnGetLatest()
{
	CWaitCursor wc;
	//
	Singleton<IEditorContainer>()->DestroyActiveEditor( true );
	Singleton<IChildFrameContainer>()->Destroy();
	//
	bool bResult = false;
	try
	{
		bResult = Singleton<IResourceManager>()->GetLatest();
	}
	catch (...)	{}
	if ( bResult )
	{
		AfxMessageBox( "\"Get latest\" completed successfully\r\n\r\n"
				"MapEditor will exit now.",
				MB_OK | MB_ICONINFORMATION
				);
	}
	else
	{
		AfxMessageBox( "Game database Get Latest operation has failed.\r\n\r\n"
				"(Detailed error description stored in MapEditor.log file)\r\n\r\n"
				"This situation is CRITICAL.\r\n"
				"If you do not want to lose changes you've done in your local database, call to MapEditor's gurus immediately and do not try to start MapEditor again until they're come.\r\n\r\n"
				"MapEditor will exit now.",
				MB_OK | MB_ICONERROR
				);
	}
	AfxGetMainWnd()->PostMessage( WM_COMMAND, ID_APP_EXIT, 0 );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CDWGDBBrowser::HandleCommand( UINT nCommandID, DWORD dwData )
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	switch( nCommandID )
	{
		case ID_MAIN_NEW:
		{
			if ( !pUserData->constUserData.szMainObjectType.empty() )
			{
				New( pUserData->constUserData.szMainObjectType );
				return true;
			}
			break;
		}
		case ID_MAIN_OPEN:
		{
			if ( !pUserData->constUserData.szMainObjectType.empty() )
			{
				Open( pUserData->constUserData.szMainObjectType );
				return true;
			}
			break;
		}
		case ID_MAIN_CLOSE:
		{
			if ( Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, true ) )
			{
				Singleton<IEditorContainer>()->DestroyActiveEditor( false );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			}
			return true;
		}
		case ID_MAIN_NEW_RESOURCE:
		{
			New( string() );
			return true;
		}
		case ID_MAIN_OPEN_RESOURCE:
		{
			Open( string() );
			return true;
		}
		case ID_MAIN_SAVE:
		{
			Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, false );
			return true;
		}
		case ID_MAIN_RELOAD:
		{
			CreateTabs();
			return true;
		}
		case ID_MAIN_SELECT:
		{
			SelectTables();
			return true;
		}
		case ID_MAIN_CHECKOUT:
		{
			if ( NGlobal::GetVar( "enable_version_control", 0 ) == 1 )
			{
				OnCheckOut();
			}
			return true;
		}
		case ID_MAIN_CHECKIN:
		{
			if ( NGlobal::GetVar( "enable_version_control", 0 ) == 1 )
			{
				OnCheckIn();
			}
			return true;
		}
		case ID_MAIN_GETLATEST:
		{
			if ( NGlobal::GetVar( "enable_version_control", 0 ) == 1 )
			{
				OnGetLatest();
			}
			return true;
		}
		case ID_MAIN_RECENT_0:
		case ID_MAIN_RECENT_1:
		case ID_MAIN_RECENT_2:
		case ID_MAIN_RECENT_3:
		case ID_MAIN_RECENT_4:
		case ID_MAIN_RECENT_5:
		case ID_MAIN_RECENT_6:
		case ID_MAIN_RECENT_7:
		case ID_MAIN_RECENT_8:
		case ID_MAIN_RECENT_9:
		{
			OnRecentList( nCommandID - ID_MAIN_RECENT_0, true );
			break;
		}
		case ID_MAIN_RECENT_RESOURCE_0:
		case ID_MAIN_RECENT_RESOURCE_1:
		case ID_MAIN_RECENT_RESOURCE_2:
		case ID_MAIN_RECENT_RESOURCE_3:
		case ID_MAIN_RECENT_RESOURCE_4:
		case ID_MAIN_RECENT_RESOURCE_5:
		case ID_MAIN_RECENT_RESOURCE_6:
		case ID_MAIN_RECENT_RESOURCE_7:
		case ID_MAIN_RECENT_RESOURCE_8:
		case ID_MAIN_RECENT_RESOURCE_9:
		{
			OnRecentList( nCommandID - ID_MAIN_RECENT_RESOURCE_0, false );
			break;
		}
		case ID_MAIN_NEW_MOD:
		{
			if ( Singleton<IMODContainer>()->CanNewMOD() )
			{
				Singleton<IMODContainer>()->NewMOD();
			}
			break;
		}
		case ID_MAIN_OPEN_MOD:
		{
			if ( Singleton<IMODContainer>()->CanOpenMOD() )
			{
				Singleton<IMODContainer>()->OpenMOD();
			}
			break;
		}
		case ID_MAIN_CLOSE_MOD:
		{
			if ( Singleton<IMODContainer>()->CanCloseMOD() )
			{
				Singleton<IMODContainer>()->CloseMOD();
			}
			break;
		}
		case ID_MAIN_OBJECT_LOCATE:
		{
			LocateObject();
			break;
		}
		default:
			return false;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CDWGDBBrowser::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CDWGDBBrowser::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CDWGDBBrowser::UpdateCommand(), pbCheck == 0" );
	//
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	switch( nCommandID )
	{
		case ID_MAIN_NEW:
		case ID_MAIN_OPEN:
		{
			( *pbEnable ) = ( !pUserData->constUserData.szMainObjectType.empty() );
			( *pbCheck ) = false;
			return true;
		}
		case ID_MAIN_CLOSE:
		{
			( *pbEnable ) = ( Singleton<IEditorContainer>()->GetActiveEditor() != 0 );
			( *pbCheck ) = false;
			return true;
		}
		case ID_MAIN_NEW_RESOURCE:
		{
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		}
		case ID_MAIN_OPEN_RESOURCE:
		{
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		}
		case ID_MAIN_SAVE:
		{
			{
				bool bModified = ( Singleton<IEditorContainer>()->IsModified() || Singleton<IResourceManager>()->CanSyncDB() );
				( *pbEnable ) = bModified;
				( *pbCheck ) = false;
				//
				SSWTParams swtParams;
				swtParams.dwFlags = SWT_MODIFIED;
				swtParams.bModified = bModified;
				Singleton<IMainFrameContainer>()->Get()->SetWindowTitle( swtParams );
			}
			return true;
		}
		case ID_MAIN_RELOAD:
		case ID_MAIN_SELECT:
		{
			( *pbEnable ) = true;
			( *pbCheck ) = false;
			return true;
		}
		case ID_MAIN_CHECKOUT:
		case ID_MAIN_CHECKIN:
		case ID_MAIN_GETLATEST:
		{
			( *pbEnable ) = ( NGlobal::GetVar( "enable_version_control", 0 ) == 1 );
			( *pbCheck ) = false;
			return true;
		}
		case ID_MAIN_RECENT_0:
		case ID_MAIN_RECENT_1:
		case ID_MAIN_RECENT_2:
		case ID_MAIN_RECENT_3:
		case ID_MAIN_RECENT_4:
		case ID_MAIN_RECENT_5:
		case ID_MAIN_RECENT_6:
		case ID_MAIN_RECENT_7:
		case ID_MAIN_RECENT_8:
		case ID_MAIN_RECENT_9:
		{
			( *pbEnable ) = ( !pUserData->recentList.empty() );
			( *pbCheck ) = false;
			return true;
			break;
		}
		case ID_MAIN_RECENT_RESOURCE_0:
		case ID_MAIN_RECENT_RESOURCE_1:
		case ID_MAIN_RECENT_RESOURCE_2:
		case ID_MAIN_RECENT_RESOURCE_3:
		case ID_MAIN_RECENT_RESOURCE_4:
		case ID_MAIN_RECENT_RESOURCE_5:
		case ID_MAIN_RECENT_RESOURCE_6:
		case ID_MAIN_RECENT_RESOURCE_7:
		case ID_MAIN_RECENT_RESOURCE_8:
		case ID_MAIN_RECENT_RESOURCE_9:
		{
			( *pbEnable ) = ( !pUserData->recentResourceList.empty() );
			( *pbCheck ) = false;
			return true;
			break;
		}
		case ID_MAIN_NEW_MOD:
		{
			( *pbEnable ) = Singleton<IMODContainer>()->CanNewMOD();
			( *pbCheck ) = false;
			return true;
		}
		case ID_MAIN_OPEN_MOD:
		{
			( *pbEnable ) = Singleton<IMODContainer>()->CanOpenMOD();
			( *pbCheck ) = false;
			return true;
		}
		case ID_MAIN_CLOSE_MOD:
		{
			( *pbEnable ) = Singleton<IMODContainer>()->CanCloseMOD();
			( *pbCheck ) = false;
			return true;
		}
		case ID_MAIN_OBJECT_LOCATE:
		{
			if ( IEditor *pEditor = Singleton<IEditorContainer>()->GetActiveEditor() )
			{
				if ( IView *pView = pEditor->GetView() )
				{
					SObjectSet objectSet;
					pView->GetObjectSet( &objectSet );
					if ( !objectSet.szObjectTypeName.empty() && !objectSet.objectNameSet.empty() )
					{
						( *pbEnable ) = true;
						( *pbCheck ) = false;
						return true;
					}
				}
			}
			return false;
		}
		default:
			return false;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CDWGDBBrowser::LocateObject()
{
	if ( IEditor *pEditor = Singleton<IEditorContainer>()->GetActiveEditor() )
	{
		if ( IView *pView = pEditor->GetView() )
		{
			SObjectSet objectSet;
			pView->GetObjectSet( &objectSet );
			if ( !objectSet.szObjectTypeName.empty() && !objectSet.objectNameSet.empty() )
			{
				CWaitCursor waitCursor;
				SelectObjectSet( objectSet );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_VIEW, ID_VIEW_SHOW_PROPERTY_BROWSER, 1 );
			}
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
CWMMnemonicCodes mnemonicCodes;
LRESULT CDWGDBBrowser::WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) 
{
	if ( message == WM_NOTIFY )
	{
		DebugTrace( "Message: %s, wParam: 0x%X(%u), lParam: 0x%X\n", mnemonicCodes.Get( message ).c_str(), wParam, wParam, lParam );
	}
	return SECControlBar::WindowProc( message, wParam, lParam );
}
/**/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// basement storage  
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

