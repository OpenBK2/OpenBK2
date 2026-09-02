#include "stdafx.h"

#include "BuildDataBuilder.h"

#include "Interface_UserData.h"
#include "libdb/ResourceManager.h"
#include "Tools_HashSet.h"

bool CBuildDataBuilder::InsertObject( std::string *pszObjectTypeName,
																			std::string *pszUniqueObjectName,
																			bool bFromMainMenu,
																			bool *pbCanChangeObjectName,
																			bool *pbNeedExport,
																			bool *pbNeedEdit )
{
	NI_ASSERT( pszObjectTypeName != 0, "CBuildDataBuilder::InsertObject() pszObjectTypeName == 0" );
	NI_ASSERT( pszUniqueObjectName != 0, "CBuildDataBuilder::InsertObject() pszUniqueObjectName == 0" );
	NI_ASSERT( pbCanChangeObjectName != 0, "CBuildDataBuilder::InsertObject() pbCanChangeObjectName == 0" );
	NI_ASSERT( pbNeedExport != 0, "CBuildDataBuilder::InsertObject() pbNeedExport == 0" );
	NI_ASSERT( pbNeedEdit != 0, "CBuildDataBuilder::InsertObject() pbNeedEdit == 0" );
	//	
	if ( !NeedBuildDataDialog() )
	{
		// call default implementation
		return CBuilderBase::InsertObject( pszObjectTypeName, pszUniqueObjectName, bFromMainMenu, pbCanChangeObjectName, pbNeedExport, pbNeedEdit );
	}
	//
	( *pbCanChangeObjectName ) = false;
	( *pbNeedExport ) = false;
	( *pbNeedEdit ) = false;
	SBuildDataParams buildDataParams;
	buildDataParams.szObjectTypeName = ( *pszObjectTypeName );
	CStringManager::SplitFileName( &( buildDataParams.szObjectNamePrefix ),
																 &( buildDataParams.szObjectName ),
																 &( buildDataParams.szObjectNameExtention ),
																 ( *pszUniqueObjectName ) );
	buildDataParams.bNeedExport = ( *pbNeedExport );
	buildDataParams.bNeedEdit = ( *pbNeedEdit );
	//
	std::string szBuildDataTypeName = GetBuildDataTypeName();
	std::string szBuildDataName;
	if ( Singleton<IBuilderContainer>()->FillBuildData( &szBuildDataTypeName, &szBuildDataName, &buildDataParams, this ) )
	{
		( *pszObjectTypeName ) = buildDataParams.szObjectTypeName;
		buildDataParams.GetObjectName( pszUniqueObjectName );
		( *pbNeedExport ) = buildDataParams.bNeedExport;
		( *pbNeedEdit ) = buildDataParams.bNeedEdit;
		if ( CPtr<IManipulator> pBuildDataManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szBuildDataTypeName, szBuildDataName ) )
		{
			std::string szDescription;
			if ( IsValidBuildData( pBuildDataManipulator, &szDescription, 0 ) )
			{
				return InternalInsertObject( pszObjectTypeName, pszUniqueObjectName, bFromMainMenu, pbCanChangeObjectName, pbNeedExport, pbNeedEdit, pBuildDataManipulator );
			}
		}
	}
	return false;
}


bool CBuildDataBuilder::IsUniqueObjectName( const std::string &szObjectType, const std::string &szObjectName )
{
	return Singleton<IFolderCallback>()->IsUniqueName( szObjectType, szObjectName );
}

// basement storage  


