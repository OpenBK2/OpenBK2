#include "stdafx.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "ResourceDefines.h"

#include "libdb/ResourceManager.h"
#include "SelectTablesView.h"
#include "GDBBrowserPane.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/Interface_Builder.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_ChildFrame.h"
#include "MapEditorLib/Interface_Editor.h"
#include "MapEditorLib/Interface_Exporter.h"
#include "MapEditorLib/Interface_FolderCallback.h"
#include "MapEditorLib/Interface_MOD.h"

#include <cstdint>

// Moved from DW_GDBBrowser.cpp unchanged, but for what the pane did with its
// own window: placing the browser after a table is shown, showing the empty
// face, and owning Select Tables are asked of IPane. ClearTable, whose body was
// commented out and which nothing called, did not come along.

CGDBBrowserContents::CGDBBrowserContents( IPane *_pPane, int _nGDBBrowserID )
	: pPane( _pPane ), nGDBBrowserID( _nGDBBrowserID ), bCreateControls( true ), pContents( 0 )
{
}


CGDBBrowserContents::~CGDBBrowserContents()
{
	delete pContents;
	pContents = 0;
}


void CGDBBrowserContents::Start( IObjectBrowser *pBrowser )
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	selectedTables = pUserData->tableSetMap[nGDBBrowserID];
	szCurrentTable = pUserData->szCurrentTableMap[nGDBBrowserID];
	//
	if ( pContents != pBrowser )
	{
		delete pContents;
		pContents = pBrowser;
	}
	//
	SetTableManipulator( Singleton<IResourceManager>()->CreateTableManipulator() );
}


void CGDBBrowserContents::Stop()
{
	Singleton<ICommandHandlerContainer>()->Remove( CHID_OBJECT_STORAGE, GetObjectStorage() );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MAIN, this );
	//
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	pUserData->tableSetMap[nGDBBrowserID] = selectedTables;
	pUserData->szCurrentTableMap[nGDBBrowserID] = szCurrentTable;
}


void CGDBBrowserContents::OnTableSelected()
{
	if ( !bCreateControls && ( pContents != 0 ) )
	{
		pContents->ShowActiveTable();
		pPane->LayoutContents();
		//
		pContents->GetActiveTableName( &szCurrentTable );
		IObjectTree *const pwndTreeGBDBrowserBase = pContents->GetActiveTable();
		if ( pwndTreeGBDBrowserBase == 0 )
		{
			return;
		}
		if ( !pwndTreeGBDBrowserBase->IsTreeCreated() )
		{
			SObjectSet collectionObjectSet;
			collectionObjectSet.szObjectTypeName = szCurrentTable;
			InsertHashSetElement( &( collectionObjectSet.objectNameSet ), CDBID( VIEW_COLLECTION_ID ) );
			//
			pwndTreeGBDBrowserBase->GetView()->SetViewManipulator( Singleton<IResourceManager>()->CreateFolderManipulator( szCurrentTable ), collectionObjectSet, std::string() );
			pwndTreeGBDBrowserBase->CreateTree();
		}
		else
		{
			pwndTreeGBDBrowserBase->UpdateSelectionManipulator( true );
		}
	}
}


void CGDBBrowserContents::SetTableManipulator( IManipulator *_pTableManipulator )
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
			std::string szName;
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


void CGDBBrowserContents::CreateTabs()
{
	if ( pContents == 0 )
	{
		return;
	}
	//создаем таблицы
	{
		bCreateControls = true;
		pContents->RemoveAllTables();
		// Необходимо сначало добавить все панели, а потом устанавливать активную
		IObjectTree* pwndActiveTreeGBDBrowser = 0;
		for ( std::list<std::string>::const_iterator itTable = tables.begin(); itTable != tables.end(); ++itTable )
		{
			if ( selectedTables.find( *itTable ) != selectedTables.end() )
			{
				if ( IObjectTree* pwndTreeGBDBrowser = pContents->AddTable( *itTable ) )
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
			pContents->ActivateTable( pwndActiveTreeGBDBrowser );
		}
		else if ( pContents->GetTableCount() > 0 )
		{
			pContents->ActivateTable( pContents->GetTable( 0 ) );
		}
	}

	if ( pContents->GetTableCount() > 0 )
	{
		pContents->Show( true );
		pPane->ShowEmpty( false );
	}
	else
	{
		pContents->Show( false );
		pPane->ShowEmpty( true );
	}
}


void CGDBBrowserContents::SelectTables()
{
	// Which dialog answers -- the MFC one or the wx one -- is NSelectTables'
	// business, not this browser's. See SelectTablesView.h.
	if ( NSelectTables::Run( pPane->GetOwner(), tables, &selectedTables ) )
	{
		CreateTabs();
	}
}


void CGDBBrowserContents::SelectObjectSet( const SObjectSet &rObjectSet )
{
	if ( !rObjectSet.objectNameSet.empty() && ( pContents != 0 ) )
	{
		if ( !rObjectSet.objectNameSet.begin()->first.IsEmpty() )
		{
			Singleton<IUserDataContainer>()->Get()->objectTypeDataMap[rObjectSet.szObjectTypeName].szCurrentObject = rObjectSet.objectNameSet.begin()->first.ToString();
		}
		//
		if ( 	selectedTables.find( rObjectSet.szObjectTypeName ) != selectedTables.end() )
		{
			IObjectTree *pwndTab = pContents->GetTable( rObjectSet.szObjectTypeName );
			if ( pwndTab != 0 )
			{
				pwndTab->SetStrongSelection();
				pContents->ActivateTable( pwndTab );
			}
		}
		else
		{
			bCreateControls = true;
			InsertHashSetElement( &selectedTables, rObjectSet.szObjectTypeName );
			IObjectTree* pwndTreeGBDBrowser = pContents->AddTable( rObjectSet.szObjectTypeName );
			bCreateControls = false;
			// The table is added before anything is asked of its tree; a tree that
			// could not be made used to be dereferenced here all the same.
			if ( pwndTreeGBDBrowser == 0 )
			{
				return;
			}
			pwndTreeGBDBrowser->SetPCDialogCommandHandlerID( CHID_PC_DIALOG, false );
			pwndTreeGBDBrowser->SetStrongSelection();
			pContents->ActivateTable( pwndTreeGBDBrowser );
			pContents->Show( true );
			pPane->ShowEmpty( false );
		}
	}
}


void CGDBBrowserContents::New( const std::string &rszObjectTypeName )
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
	std::string szObjectTypeName = rszObjectTypeName;
	if ( szObjectTypeName.empty() )
	{
		return;
	}
	CString strObjectName;
	strObjectName.LoadString( IDS_TREE_GDB_BROWSE_NEW_MAIN_OBJECT );
	std::string szDefaultFolder;
	pBuilderContainer->GetDefaultFolder( szObjectTypeName, &szDefaultFolder );
	std::string szObjectName = szDefaultFolder + std::string( strObjectName );
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
				std::string szName;
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


void CGDBBrowserContents::Open( const std::string &rszObjectTypeName )
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
	std::string szObjectTypeName = rszObjectTypeName;
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
					std::string szName;
					CStringManager::CreateRecentListName( &szName, objectSet, bMainObject );
					CStringManager::AddToRecentList( szName, bMainObject );
					SelectObjectSet( objectSet );
					pEditorContainer->Create( pObjectManipulator, objectSet );
				}
			}
		}
	}
}


void CGDBBrowserContents::OnRecentList( int nIndex, bool bMainObject )
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
	std::string szName;
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


void CGDBBrowserContents::OnCheckOut()
{
}


void CGDBBrowserContents::OnCheckIn()
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


void CGDBBrowserContents::OnGetLatest()
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


bool CGDBBrowserContents::HandleCommand( unsigned nCommandID, uintptr_t dwData )
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
			New( std::string() );
			return true;
		}
		case ID_MAIN_OPEN_RESOURCE:
		{
			Open( std::string() );
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


bool CGDBBrowserContents::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CGDBBrowserContents::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CGDBBrowserContents::UpdateCommand(), pbCheck == 0" );
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


void CGDBBrowserContents::LocateObject()
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
