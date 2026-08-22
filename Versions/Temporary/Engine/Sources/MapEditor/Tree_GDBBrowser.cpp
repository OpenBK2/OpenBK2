#include "stdafx.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "WMDefines.h"
#include "Tree_GDBBrowserBase_Constants.h"
#include "ResourceDefines.h"
//#include "afxdlgs.h"

#include "Tree_GDBBrowser.h"
#include "MapEditorLib/Interface_Editor.h"
#include "MapEditorLib/Interface_UserData.h"
#include "libdb/ResourceManager.h"

const char CTreeGDBBrowser::TABGDBB_TREE_HEAD_WIDTH_UPDATE_TYPE[] = "__CTGDBB__";


void CTreeGDBBrowser::Load()
{
	// Может быть ноль ( если нет выделенных объектов )
	SObjectSet objectSet;
	if ( GetViewManipulator() )
	{
		if ( GetCurrentObjectSet( &objectSet ) && ( !objectSet.objectNameSet.empty() ) )
		{
			if ( !Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, true ) )
			{
				return;
			}
			if ( Singleton<IEditorContainer>()->CanCreate( objectSet.szObjectTypeName ) )
			{
				IResourceManager *pResourceManager = Singleton<IResourceManager>();
				if ( CPtr<IManipulator> pObjectManipulator = pResourceManager->CreateObjectManipulator( objectSet.szObjectTypeName, objectSet.objectNameSet.begin()->first ) )
				{
					Singleton<IEditorContainer>()->Create( pObjectManipulator, objectSet );
				}
			}
			else if ( Singleton<IEditorContainer>()->GetActiveEditor() != 0 )
			{
				Singleton<IEditorContainer>()->DestroyActiveEditor( false );
				Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
			}
		}
	}
	//
	if ( CWnd *pwndParent = GetParent() )
	{
		pwndParent->SendMessage( WM_TREE_GDB_BROWSER, MAKEWPARAM( TREE_GDB_BROWSER_LOAD, 0 ), reinterpret_cast<LPARAM>( this ) );
	}
}


bool CTreeGDBBrowser::CanLoad()
{
	if ( GetSelectedCount() == 1 )
	{
		if ( HTREEITEM hFocusedItem = GetSelectedItem() )
		{
			const EGDBOType nType = GetTreeItemType( hFocusedItem );
			if ( nType == GDBO_OBJECT )
			{
				string szLocalObjectTypeName = GetObjectSet().szObjectTypeName;
				//
				return ( Singleton<IEditorContainer>()->CanCreate( szLocalObjectTypeName ) ||
								 ( Singleton<IEditorContainer>()->GetActiveEditor() != 0 ) );
			}
		}
	}
	return false;
}


bool CTreeGDBBrowser::GetLoadContextMenuLabel( string *pszLabel )
{
	if ( pszLabel )
	{
		CString strLabel;
		strLabel.LoadString( IDS_TREE_GDB_BROWSE_LOAD );
		( *pszLabel ) = strLabel;
	}
	return true;
}


bool CTreeGDBBrowser::GetSaveHeaderWidthLabel( string *pszLabel )
{
	if ( pszLabel )
	{
		( *pszLabel ) = TABGDBB_TREE_HEAD_WIDTH_UPDATE_TYPE;
	}
	return true;
}


void CTreeGDBBrowser::LoadHeaderWidth()
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	pUserData->tableHeaderWidthList.resize( TABGDBB_TREE_COLUMN_COUNT, 0 );
	//
	for ( int index = 0; index < TABGDBB_TREE_COLUMN_COUNT; ++index )
	{
		SetColumnWidth( index, ( pUserData->tableHeaderWidthList[index] > 0 ) ? pUserData->tableHeaderWidthList[index] : TABGDBB_TREE_COLUMN_WIDTH[index] );
	}
	RecalcScrollBars();
	RedrawWindow();
}


void CTreeGDBBrowser::SaveHeaderWidth()
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	pUserData->tableHeaderWidthList.resize( TABGDBB_TREE_COLUMN_COUNT, 0 );
	//
	for ( int index = 0; index < TABGDBB_TREE_COLUMN_COUNT; ++index )
	{
		pUserData->tableHeaderWidthList[index] = GetColumnWidth( index );
	}
}

// basement storage


