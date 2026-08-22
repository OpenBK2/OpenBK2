#include "stdafx.h"

#include "DefaultBuilder.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_UserData.h"
#include "libdb/ResourceManager.h"

bool CBuilderBase::InsertObject( string *pszObjectTypeName, string *pszUniqueObjectName, bool bFromMainMenu, bool *pbCanChangeObjectName, bool *pbNeedExport, bool *pbNeedEdit )
{
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	NI_ASSERT( pszObjectTypeName != 0, "CBuilderBase::InsertObject() pszObjectTypeName == 0" );
	NI_ASSERT( pszUniqueObjectName != 0, "CBuilderBase::InsertObject() pszUniqueObjectName == 0" );
	NI_ASSERT( pbCanChangeObjectName != 0, "CBuilderBase::InsertObject() pbCanChangeObjectName == 0" );
	NI_ASSERT( pbNeedExport != 0, "CBuilderBase::InsertObject() pbNeedExport == 0" );
	NI_ASSERT( pbNeedEdit != 0, "CBuilderBase::InsertObject() pbNeedEdit == 0" );
	( *pbCanChangeObjectName ) = true;
	( *pbNeedExport ) = false;
	( *pbNeedEdit ) = false;
	return pFolderCallback->InsertObject( *pszObjectTypeName, *pszUniqueObjectName );
}


bool CBuilderBase::CopyObject( const string &rszObjectTypeName, const string &rszDestination, const string &rszSource )
{
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	return pFolderCallback->CopyObject( rszObjectTypeName, rszDestination, rszSource );
}


bool CBuilderBase::RenameObject( const string &rszObjectTypeName, const string &rszDestination, const string &rszSource )
{
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	return pFolderCallback->RenameObject( rszObjectTypeName, rszDestination, rszSource );
}


bool CBuilderBase::RemoveObject( const string &rszObjectTypeName, const string &rszObjectName )
{
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	return pFolderCallback->RemoveObject( rszObjectTypeName, rszObjectName, false );
}
/**

bool HierarchicalDelete( const string &szObjectTypeName, const string &szObjectName )
{
	IResourceManager *pRM = Singleton<IResourceManager>();
	if ( CPtr<IManipulator> pMan = pRM->CreateObjectManipulator(szObjectTypeName, szObjectName) ) 
	{
		CManipulatorManager::CReferenceInfoList refsList;
		const unsigned nFlags = REFINFO_MAKE_UNIQUE_LIST | REFINFO_OBJECT_TYPE_NAME | REFINFO_OBJECT_NAME;
		CManipulatorManager::EnumReferences( &refsList, pMan, nFlags, true, ECT_CACHE_LOCAL );

		if ( Singleton<IBuilderContainer>()->RemoveObject( szObjectTypeName, szObjectName, pFolderCallback ) )
		{
			for ( CManipulatorManager::CReferenceInfoList::const_iterator it = refsList.begin(); it != refsList.end(); ++it ) 
			{
				HierarchicalDelete( it->szObjectTypeName, it->szObjectName, pFolderCallback );
			}
			return true;
		}
	}
	return false;
}
/**/

// basement storage  


