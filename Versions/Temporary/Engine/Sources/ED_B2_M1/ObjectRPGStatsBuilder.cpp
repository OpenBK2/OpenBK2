#include "stdafx.h"

#include "ObjectRPGStatsBuilder.h"
#include "MapEditorLib/BuilderFactory.h"

#include "libdb/ResourceManager.h"
#include "MapEditorLib/ManipulatorManager.h"

REGISTER_BUILDER_IN_DLL( ObjectRPGStats, CObjectRPGStatsBuilder )


const string CObjectRPGStatsBuilder::BUILD_DATA_TYPE_NAME = "ObjectRPGStatsBuilder";


bool CObjectRPGStatsBuilder::IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView )
{
	NI_ASSERT( pBuildDataManipulator != 0, "CObjectRPGStatsBuilder::IsValidBuildData() pBuildDataManipulator == 0" );
	NI_ASSERT( pszDescription != 0, "CObjectRPGStatsBuilder::IsValidBuildData() pszDescription == 0" );
	pszDescription->clear();	
	// Считываем данные
	string szVisualObject;
	if ( !CManipulatorManager::GetValue( &szVisualObject, pBuildDataManipulator, "VisualObject" ) || szVisualObject.empty() )
	{
		( *pszDescription ) = "<VisualObject> must be filled.";
		return false;
	}
	string szSource;
	if ( !CManipulatorManager::GetValue( &szSource, pBuildDataManipulator, "Source" ) || szSource.empty() )
	{
		( *pszDescription ) = "Warning! <Source> is empty!";
		return true;
	}
	return true;
}


bool CObjectRPGStatsBuilder::InternalInsertObject( string *pszObjectTypeName,
																									 string *pszUniqueObjectName,
																									 bool bFromMainMenu,
																									 bool *pbCanChangeObjectName,
																									 bool *pbNeedExport,
																									 bool *pbNeedEdit,
																									 IManipulator *pBuildDataManipulator )
{
	NI_ASSERT( pszObjectTypeName != 0, "CObjectRPGStatsBuilder::InternalInsertObject() pszObjectTypeName == 0" );
	NI_ASSERT( pszUniqueObjectName != 0, "CObjectRPGStatsBuilder::InternalInsertObject() pszUniqueObjectName == 0" );
	NI_ASSERT( pBuildDataManipulator != 0, "CObjectRPGStatsBuilder::InternalInsertObject() pBuildDataManipulator == 0" );
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	//
	string szDescription;
	if ( !IsValidBuildData( pBuildDataManipulator, &szDescription, 0 ) )
	{
		return false;
	}
	// Считываем данные
	string szVisualObject;
	string szSource;
	CManipulatorManager::GetValue( &szVisualObject, pBuildDataManipulator, "VisualObject" );
	CManipulatorManager::GetValue( &szSource, pBuildDataManipulator, "Source" );
	//
	bool bResult = pFolderCallback->InsertObject( *pszObjectTypeName, *pszUniqueObjectName );
	if ( bResult )
	{
		CPtr<IManipulator> pObjectRPGStatsManipulator = pResourceManager->CreateObjectManipulator( *pszObjectTypeName, *pszUniqueObjectName );
		NI_ASSERT( pObjectRPGStatsManipulator != 0, "CObjectRPGStatsBuilder::InternalInsertObject() pObjectRPGStatsManipulator == 0" );
		if ( !szSource.empty() )
		{
			CPtr<IManipulator> pSourceObjectRPGStatsManipulator = pResourceManager->CreateObjectManipulator( *pszObjectTypeName, szSource );
			if ( pSourceObjectRPGStatsManipulator )
			{
				CManipulatorManager::CloneDBManipulator( pObjectRPGStatsManipulator, pSourceObjectRPGStatsManipulator, true );
			}
		}
		// Проставляем основные параметры
		bResult = bResult && pObjectRPGStatsManipulator->SetValue( "GameType", string( "SGVOGT_OBJECT" ) );
		bResult = bResult && pObjectRPGStatsManipulator->SetValue( "visualObject", szVisualObject );
	}
	return bResult;
}

// basement storage  


