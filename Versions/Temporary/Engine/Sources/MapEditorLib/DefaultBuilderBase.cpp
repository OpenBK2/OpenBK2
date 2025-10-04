#include "stdafx.h"

#include "DefaultBuilderBase.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/StringManager.h"

bool CDefaultBuilderBase::InsertObject( string *pszObjectTypeName, string *pszUniqueObjectName, bool bFromMainMenu, bool *pbCanChangeObjectName, bool *pbNeedExport, bool *pbNeedEdit )
{
	if ( ( pszObjectTypeName != 0 ) && ( pszUniqueObjectName != 0 ) && ( pbNeedExport != 0 ) && ( pbNeedEdit != 0 ) )
	{
		( *pbCanChangeObjectName ) = true;
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
		if ( Singleton<IBuilderContainer>()->FillNewObjectName( &buildDataParams ) )
		{
			( *pszObjectTypeName ) = buildDataParams.szObjectTypeName;
			buildDataParams.GetObjectName( pszUniqueObjectName );
			( *pbNeedExport ) = buildDataParams.bNeedExport;
			( *pbNeedEdit ) = buildDataParams.bNeedEdit;
			return CBuilderBase::InsertObject( pszObjectTypeName, pszUniqueObjectName, bFromMainMenu, pbCanChangeObjectName, pbNeedExport, pbNeedEdit );
		}
	}
	return false;
}

// basement storage  


