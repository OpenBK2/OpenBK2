#include "StdAfx.h"

#include "DynamicDebrisSetBuilder.h"
#include "libdb/ResourceManager.h"

//REGISTER_BUILDER_IN_DLL( DynamicDebrisSet, CDynamicDebrisSetBuilder )


const string CDynamicDebrisSetBuilder::BUILD_DATA_TYPE_NAME = "DynamicDebrisSetBuilder";


bool CDynamicDebrisSetBuilder::IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView )
{
	NI_ASSERT( pBuildDataManipulator != 0, "CDynamicDebrisSetBuilder::IsValidBuildData() pBuildDataManipulator == 0" );
	NI_ASSERT( pszDescription != 0, "CDynamicDebrisSetBuilder::IsValidBuildData() pszDescription == 0" );
	pszDescription->clear();	
	return true;
}


bool CDynamicDebrisSetBuilder::InternalInsertObject( string *pszObjectTypeName,
																										 string *pszUniqueObjectName,
																										 bool bFromMainMenu,
																										 bool *pbCanChangeObjectName,
																										 bool *pbNeedExport,
																										 bool *pbNeedEdit,
																										 IManipulator *pBuildDataManipulator )
{
	NI_ASSERT( pszObjectTypeName != 0, "CDynamicDebrisSetBuilder::InternalInsertObject() pszObjectTypeName == 0" );
	NI_ASSERT( pszUniqueObjectName != 0, "CDynamicDebrisSetBuilder::InternalInsertObject() pszUniqueObjectName == 0" );
	NI_ASSERT( pBuildDataManipulator != 0, "CDynamicDebrisSetBuilder::InternalInsertObject() pBuildDataManipulator == 0" );
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	//
	string szDescription;
	if ( !IsValidBuildData( pBuildDataManipulator, &szDescription, 0 ) )
	{
		return false;
	}
	// Считываем данные
	
	bool bResult = true;
	return bResult;
}

// basement storage  


