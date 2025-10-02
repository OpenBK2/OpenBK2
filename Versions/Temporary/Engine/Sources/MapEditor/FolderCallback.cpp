#include "StdAfx.h"

#include "../libdb/ResourceManager.h"
#include "../libdb/DB.h"
#include "../MapEditorLib/Tools_HashSet.h"
#include "../MapEditorLib/FolderController.h"
#include "../MapEditorLib/Interface_Logger.h"
#include "FolderCallback.h"

void CFolderCallback::UndoChanges()
{
	for ( CUndoDataList::const_iterator itUndoData = undoDataList.begin(); itUndoData != undoDataList.end(); ++itUndoData )
	{
		RemoveObject( itUndoData->szObjectTypeName, itUndoData->szObjectName, false );
	}
	ClearUndoData();
}


void CFolderCallback::LockObjects( const SObjectSet &rObjectSet )
{
	if ( !rObjectSet.szObjectTypeName.empty() && !rObjectSet.objectNameSet.empty() )
	{
		CLockObjectMap::iterator itLockObject = lockObjectMap.find( rObjectSet.szObjectTypeName );
		if ( itLockObject == lockObjectMap.end() )
		{
			lockObjectMap[rObjectSet.szObjectTypeName] = rObjectSet.objectNameSet;
		}
		else
		{
			for ( CObjectNameSet::const_iterator itObjectName = rObjectSet.objectNameSet.begin(); itObjectName != rObjectSet.objectNameSet.end(); ++itObjectName )
			{
				InsertHashSetElement( &( itLockObject->second ), itObjectName->first );
			}
		}
	}
}


void CFolderCallback::UnockObjects( const SObjectSet &rObjectSet )
{
	if ( !rObjectSet.szObjectTypeName.empty() && !rObjectSet.objectNameSet.empty() )
	{
		CLockObjectMap::iterator itLockObject = lockObjectMap.find( rObjectSet.szObjectTypeName );
		if ( itLockObject != lockObjectMap.end() )
		{
			for ( CObjectNameSet::const_iterator itObjectName = rObjectSet.objectNameSet.begin(); itObjectName != rObjectSet.objectNameSet.end(); ++itObjectName )
			{
				itLockObject->second.erase( itObjectName->first );
			}
			if ( itLockObject->second.empty() )
			{
				lockObjectMap.erase( itLockObject );  
			}
		}
	}
}


bool CFolderCallback::IsObjectLocked( const string &rszTypeName, const CDBID &rDBID ) const
{
	if ( !rszTypeName.empty() && !rDBID.IsEmpty() )
	{
		CLockObjectMap::const_iterator itLockObject = lockObjectMap.find( rszTypeName );
		if ( itLockObject != lockObjectMap.end() )
		{
			return ( itLockObject->second.find( rDBID ) != itLockObject->second.end() );
		}
	}
	return false;
}


bool CFolderCallback::IsUniqueName( const string &rszTypeName, const string &rszName )
{
	if ( rszTypeName.empty() || rszName.empty() )
	{
		return false;
	}
	return !NDb::DoesObjectExist( CDBID( rszName ) );
}


bool CFolderCallback::UniqueName( const string &szTypeName, string *pszName )
{
	NI_ASSERT( pszName != 0, "Wrong parameter: pszObjectName == 0" );
	if ( szTypeName.empty() || pszName->empty() )
	{
		return false;
	}
	bool bExists = NDb::DoesObjectExist( CDBID( *pszName ) );
	if ( bExists )
	{
		const DWORD dwMaxNumber = 0x10000;
		DWORD dwNumber = 2;
		const bool bFolder = ( ( *pszName )[pszName->size() - 1] == PATH_SEPARATOR_CHAR );
		string szName;
		if ( bFolder )
		{
			szName = pszName->substr( 0, pszName->size() - 1 );
		}
		else
		{
			szName = ( *pszName );
		}
		while ( bExists && ( dwNumber < dwMaxNumber ) )
		{
			( *pszName ) = szName + StrFmt( " (%u)", dwNumber );
			if ( bFolder )
			{
				szName += StrFmt( "%c", PATH_SEPARATOR_CHAR );
			}
			bExists = NDb::DoesObjectExist( CDBID( *pszName ) );
		}
	}
	return !bExists;
}


bool CFolderCallback::InsertObject( const string &rszObjectTypeName, const string &rszObjectName )
{
	if ( rszObjectTypeName.empty() || rszObjectName.empty() )
	{
		return false;
	}
	//
	SObjectSet objectSet;
	objectSet.szObjectTypeName = rszObjectTypeName;
	InsertHashSetElement( &( objectSet.objectNameSet ), VIEW_COLLECTION_ID );
	//
	CFolderController folderController;
	folderController.SetObjectSet( objectSet );
	//
	folderController.AddInsertOperation( rszObjectName );
	//
	if ( folderController.Redo( true, true, 0 ) )
	{
		CUndoDataList::iterator posUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
		posUndoData->szObjectTypeName = rszObjectTypeName;
		posUndoData->szObjectName = rszObjectName;
		return true;
	}
	return false;
}


bool CFolderCallback::CopyObject( const string &rszObjectTypeName, const string &rszDestination, const string &rszSource )
{
	if ( rszObjectTypeName.empty() || rszDestination.empty() || rszSource.empty() )
	{
		return false;
	}
	//
	SObjectSet objectSet;
	objectSet.szObjectTypeName = rszObjectTypeName;
	InsertHashSetElement( &( objectSet.objectNameSet ), VIEW_COLLECTION_ID );
	//
	CFolderController folderController;
	folderController.SetObjectSet( objectSet );
	//
	folderController.AddCopyOperation( rszDestination, rszSource );
	//
	if ( folderController.Redo( true, true, 0 ) )
	{
		CUndoDataList::iterator posUndoData = undoDataList.insert( undoDataList.end(), SUndoData() );
		posUndoData->szObjectTypeName = rszObjectTypeName;
		posUndoData->szObjectName = rszDestination;
		return true;
	}
	return false;
}


bool CFolderCallback::RenameObject( const string &rszObjectTypeName, const string &rszDestination, const string &rszSource )
{
	if ( rszObjectTypeName.empty() || rszDestination.empty() || rszSource.empty() )
	{
		return false;
	}
	//
	SObjectSet objectSet;
	objectSet.szObjectTypeName = rszObjectTypeName;
	InsertHashSetElement( &( objectSet.objectNameSet ), VIEW_COLLECTION_ID );
	//
	CFolderController folderController;
	folderController.SetObjectSet( objectSet );
	//
	folderController.AddRenameOperation( rszDestination, rszSource, true );
	//
	return folderController.Redo( true, true, 0 );
}


bool CFolderCallback::RemoveObject( const string &rszObjectTypeName, const string &rszObjectName, bool bRecursive )
{
	if ( rszObjectTypeName.empty() || rszObjectName.empty() )
	{
		return false;
	}
	//
	if ( CPtr<IManipulator> pFolderManipulator = Singleton<IResourceManager>()->CreateFolderManipulator( rszObjectTypeName ) )
	{
		bool bFolder = ( !rszObjectName.empty() && ( rszObjectName[rszObjectName.size() - 1] == PATH_SEPARATOR_CHAR ) );
		if ( !bFolder && IsObjectLocked( rszObjectTypeName, CDBID( rszObjectName ) ) )
		{
			NLog::Log( LT_IMPORTANT, "Can't remove object. Object is locked. %s:%s\n", rszObjectTypeName.c_str(), rszObjectName.c_str() ); 
			return false;
		}
	}
	SObjectSet objectSet;
	objectSet.szObjectTypeName = rszObjectTypeName;
	InsertHashSetElement( &( objectSet.objectNameSet ), VIEW_COLLECTION_ID );
	//
	CFolderController folderController;
	folderController.SetObjectSet( objectSet );
	//
	folderController.AddRemoveOperation( rszObjectName );
	bool bResult = folderController.Redo( true, true, 0 );
	if ( bResult && bRecursive )
	{
		string szObjectName = rszObjectName;
		bool bLocalResult = true;
		while ( bLocalResult )
		{
			CFolderController folderController;
			folderController.SetObjectSet( objectSet );
			//
			if ( szObjectName[szObjectName.size() - 1] == PATH_SEPARATOR_CHAR )
			{
				szObjectName = szObjectName.substr( 0, szObjectName.size() - 1 );
			}
			const int nSlashPos = szObjectName.rfind( PATH_SEPARATOR_CHAR );
			if ( nSlashPos != string::npos )
			{
				szObjectName = szObjectName.substr( 0, nSlashPos + 1 );
				folderController.AddRemoveOperation( szObjectName );
				bLocalResult = folderController.Redo( true, true, 0 );
			}
			else
			{
				break;
			}
		}
	}
	return bResult;
}


bool CFolderCallback::SetColor( const string &rszObjectTypeName, const string &rszObjectName, const int nNewColor )
{
	if ( rszObjectTypeName.empty() || rszObjectName.empty() )
	{
		return false;
	}
	//
	SObjectSet objectSet;
	objectSet.szObjectTypeName = rszObjectTypeName;
	InsertHashSetElement( &( objectSet.objectNameSet ), VIEW_COLLECTION_ID );
	//
	CFolderController folderController;
	folderController.SetObjectSet( objectSet );
	//
	folderController.AddColorOperation( rszObjectName, nNewColor );
	//
	return folderController.Redo( true, true, 0 );
}

// basement storage  


