#include "StdAfx.h"

#include "StaticDebrisSetBuilder.h"
#include "../libdb/ResourceManager.h"

//REGISTER_BUILDER_IN_DLL( StaticDebrisSet, CStaticDebrisSetBuilder )


const string CStaticDebrisSetBuilder::BUILD_DATA_TYPE_NAME = "StaticDebrisSetBuilder";


bool CStaticDebrisSetBuilder::IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView )
{
	NI_ASSERT( pBuildDataManipulator != 0, "CStaticDebrisSetBuilder::IsValidBuildData() pBuildDataManipulator == 0" );
	NI_ASSERT( pszDescription != 0, "CStaticDebrisSetBuilder::IsValidBuildData() pszDescription == 0" );
	pszDescription->clear();	
	return true;
}


bool CStaticDebrisSetBuilder::InternalInsertObject( string *pszObjectTypeName,
																										string *pszUniqueObjectName,
																										bool bFromMainMenu,
																										bool *pbCanChangeObjectName,
																										bool *pbNeedExport,
																										bool *pbNeedEdit,
																										IManipulator *pBuildDataManipulator )
{
	NI_ASSERT( pszObjectTypeName != 0, "CStaticDebrisSetBuilder::InternalInsertObject() pszObjectTypeName == 0" );
	NI_ASSERT( pszUniqueObjectName != 0, "CStaticDebrisSetBuilder::InternalInsertObject() pszUniqueObjectName == 0" );
	NI_ASSERT( pBuildDataManipulator != 0, "CStaticDebrisSetBuilder::InternalInsertObject() pBuildDataManipulator == 0" );
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


