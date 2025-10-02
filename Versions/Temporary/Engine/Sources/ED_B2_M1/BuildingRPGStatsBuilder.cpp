#include "StdAfx.h"

#include "BuildingRPGStatsBuilder.h"
#include "../MapEditorLib/BuilderFactory.h"
//
#include "../libdb/ResourceManager.h"
//
#include "../MapEditorLib/Interface_UserData.h"
#include "../MapEditorLib/DefaultView.h"
#include "../MapEditorLib/ObjectController.h"

REGISTER_BUILDER_IN_DLL( BuildingRPGStats, CBuildingRPGStatsBuilder )


const string CBuildingRPGStatsBuilder::BUILD_DATA_TYPE_NAME = "BuildingRPGStatsBuilder";


bool CBuildingRPGStatsBuilder::IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView )
{
	NI_ASSERT( pBuildDataManipulator != 0, "CBuildingRPGStatsBuilder::IsValidBuildData() pBuildDataManipulator == 0" );
	NI_ASSERT( pszDescription != 0, "CBuildingRPGStatsBuilder::IsValidBuildData() pszDescription == 0" );
	pszDescription->clear();	
	// Считываем данные
	string szVisualObject;
	if ( !CManipulatorManager::GetValue( &szVisualObject, pBuildDataManipulator, "VisualObject" ) || szVisualObject.empty() )
	{
		( *pszDescription ) = "<VisualObject> must be filled.";
		return false;
	}
	string szDBType;
	if ( !CManipulatorManager::GetValue( &szDBType, pBuildDataManipulator, "Type" ) || szDBType.empty() )
	{
		( *pszDescription ) = "<Type> must be filled.";
		return false;
	}
	string szSource;
	CManipulatorManager::GetValue( &szSource, pBuildDataManipulator, "Source" );
	if ( szPreviousDBType != szDBType )
	{
		szPreviousDBType = szDBType;
		const SUserData::CObjectDBTypeMap &rBuildingDBTypeMap = Singleton<IUserDataContainer>()->Get()->buildingDBTypeMap;
		SUserData::CObjectDBTypeMap::const_iterator posBuildingDBType = rBuildingDBTypeMap.find( szPreviousDBType );
		if ( ( posBuildingDBType != rBuildingDBTypeMap.end() ) && ( !posBuildingDBType->second.empty() ) )
		{
			if ( pBuildDataView )
			{
				CPtr<CObjectBaseController> pObjectController = dynamic_cast<CDefaultView*>(pBuildDataView)->CDefaultView::CreateController<CObjectController>( static_cast<CObjectController*>( 0 ) );
				if ( pObjectController->AddChangeOperation( "Source", posBuildingDBType->second, pBuildDataManipulator ) )
				{
					pObjectController->Redo( false, true, 0 );
					Singleton<IControllerContainer>()->Add( pObjectController );
				}
			}
		}
	}
	if ( !CManipulatorManager::GetValue( &szSource, pBuildDataManipulator, "Source" ) || szSource.empty() )
	{
		( *pszDescription ) = "<Source> must be filled.";
		return false;
	}
	return true;
}


bool CBuildingRPGStatsBuilder::InternalInsertObject( string *pszObjectTypeName,
																										 string *pszUniqueObjectName,
																										 bool bFromMainMenu,
																										 bool *pbCanChangeObjectName,
																										 bool *pbNeedExport,
																										 bool *pbNeedEdit,
																										 IManipulator *pBuildDataManipulator )
{
	NI_ASSERT( pszObjectTypeName != 0, "CBuildingRPGStatsBuilder::InternalInsertObject() pszObjectTypeName == 0" );
	NI_ASSERT( pszUniqueObjectName != 0, "CBuildingRPGStatsBuilder::InternalInsertObject() pszUniqueObjectName == 0" );
	NI_ASSERT( pBuildDataManipulator != 0, "CBuildingRPGStatsBuilder::InternalInsertObject() pBuildDataManipulator == 0" );
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
	string szDBType;
	string szSource;
	CManipulatorManager::GetValue( &szVisualObject, pBuildDataManipulator, "VisualObject" );
	CManipulatorManager::GetValue( &szDBType, pBuildDataManipulator, "Type" );
	CManipulatorManager::GetValue( &szSource, pBuildDataManipulator, "Source" );

	// default window
	string szDefaultWindowDay = "";
	string szDefaultWindowNight = "";
	string szDefaultWindowDestroyed = "";
	string szDefaultWindowEffect = "";
	CManipulatorManager::GetValue( &szDefaultWindowDay, pBuildDataManipulator, "DefaultWindow.DayObj" );
	CManipulatorManager::GetValue( &szDefaultWindowNight, pBuildDataManipulator, "DefaultWindow.NightObj" );
	CManipulatorManager::GetValue( &szDefaultWindowDestroyed, pBuildDataManipulator, "DefaultWindow.DestroyedObj" );
	CManipulatorManager::GetValue( &szDefaultWindowEffect, pBuildDataManipulator, "DefaultWindow.DestroyEffect" );
	//
	bool bResult = pFolderCallback->InsertObject( *pszObjectTypeName, *pszUniqueObjectName );
	if ( bResult )
	{
		CPtr<IManipulator> pBuildingRPGStatsManipulator = pResourceManager->CreateObjectManipulator( *pszObjectTypeName, *pszUniqueObjectName );
		NI_ASSERT( pBuildingRPGStatsManipulator != 0, "CBuildingRPGStatsBuilder::InternalInsertObject() pBuildingRPGStatsManipulator == 0" );
		if ( !szSource.empty() )
		{
			CPtr<IManipulator> pSourceBuildingRPGStatsManipulator = pResourceManager->CreateObjectManipulator( *pszObjectTypeName, szSource );
			if ( pSourceBuildingRPGStatsManipulator )
			{
				CManipulatorManager::CloneDBManipulator( pBuildingRPGStatsManipulator, pSourceBuildingRPGStatsManipulator, true );
			}
		}
		// Проставляем основные параметры
		bResult = bResult && pBuildingRPGStatsManipulator->SetValue( "GameType", string( "SGVOGT_BUILDING" ) );
		bResult = bResult && pBuildingRPGStatsManipulator->SetValue( "type", szDBType );
		bResult = bResult && pBuildingRPGStatsManipulator->SetValue( "visualObject", szVisualObject );

		// clear default window
		bResult = bResult && pBuildingRPGStatsManipulator->SetValue( "DefaultWindow.DayObj", CVariant() );
		bResult = bResult && pBuildingRPGStatsManipulator->SetValue( "DefaultWindow.NightObj", CVariant() );
		bResult = bResult && pBuildingRPGStatsManipulator->SetValue( "DefaultWindow.DestroyedObj", CVariant() );
		bResult = bResult && pBuildingRPGStatsManipulator->SetValue( "DefaultWindow.DestroyEffect", CVariant() );
		
		// fill default window
		if ( szDefaultWindowDay != "" )
			bResult = bResult && pBuildingRPGStatsManipulator->SetValue( "DefaultWindow.DayObj", szDefaultWindowDay );
		if ( szDefaultWindowNight != "" )
			bResult = bResult && pBuildingRPGStatsManipulator->SetValue( "DefaultWindow.NightObj", szDefaultWindowNight );
		if ( szDefaultWindowDestroyed != "" )
			bResult = bResult && pBuildingRPGStatsManipulator->SetValue( "DefaultWindow.DestroyedObj", szDefaultWindowDestroyed );
		if ( szDefaultWindowEffect != "" )
			bResult = bResult && pBuildingRPGStatsManipulator->SetValue( "DefaultWindow.DestroyEffect", szDefaultWindowEffect );
	}
	return bResult;
}

// basement storage  


