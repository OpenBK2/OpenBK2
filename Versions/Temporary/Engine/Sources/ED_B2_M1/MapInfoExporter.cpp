#include "stdafx.h"
#include <fmt/format.h>

//#include "../misc/strproc.h"
//#include "../misc/2darray.h"
//#include "../stats_b2_m1/iconsset.h"
#include "CommandHandlerDefines.h"
#include "ResourceDefines.h"

#include "MapInfoExporter.h"
#include "MapEditorLib/ExporterFactory.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/Interface_Logger.h"
#include "MapEditorLib/Tools_IndexCollector.h"
#include "EditorScene.h"
#include "SceneB2/TerraGen.h"
#include "System/FileUtils.h"
#include "EditorMethods.h"
#include "Misc/HPTimer.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "Misc/StrProc.h"
#include "MapEditorLib/Interface_MOD.h"
#include "ExporterMethods.h"

#include <cstdint>

#include <zconf.h>

REGISTER_EXPORTER_IN_DLL( MapInfo, CMapInfoExporter )


bool CMapInfoExporter::SReGenerateGeometry::operator()( const std::string &rszObjectTypeName, const CDBID &rDBID )
{
	NLog::GetLogger()->Log( LT_NORMAL, fmt::format("Regenerating map: {}\n", rDBID.ToString().c_str() ) );
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	if ( const NDb::SMapInfo *pMapInfo = NDb::Get<NDb::SMapInfo>( rDBID ) )
	{
    CPtr<ITerraManager> pTerraManager = 0;
    if ( EditorScene()->DoesTerraManagerExist() && EditorScene()->GetTerraManager()->GetDesc()->GetDBID() == rDBID )
		{
      pTerraManager = EditorScene()->GetTerraManager();
			NEditor::SaveTerrain( pTerraManager );
		}
    else
		{
      pTerraManager = MakeObject<ITerraManager>( ITerraManager::tidTypeID );
		}
		//
    if ( pTerraManager )
    {
			//EditorScene()->SetLight( pMapInfo->pLight );
			if ( NEditor::LoadTerrain(pTerraManager, pMapInfo) )
			{
				pTerraManager->ReGenerate();
				NEditor::SaveTerrain( pTerraManager );
				NLog::GetLogger()->Log( LT_IMPORTANT, fmt::format("Map regenerated successfully in {:g} seconds\n", NHPTimer::GetTimePassed(&time)) );
				return true;
			}
    }
  }
	return false;
}


bool CMapInfoExporter::SCheck::operator()( const std::string &rszObjectTypeName, const CDBID &rDBID )
{
	std::string szObjectName = rDBID.ToString();
	CPtr<IManipulator> pObjectManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( rszObjectTypeName, rDBID );
	if ( !pObjectManipulator )
	{
		return false;
	}
	bool bResult = true;
	// Проверяем окопы:
	//CIndexCollector<unsigned> objectIDToIndexCollector( INVALID_NODE_ID );			// Список индексов объектов ( прямое отображение ID->Index )
	//CIndexCollector<unsigned> objectLinkIDToIndexCollector( INVALID_NODE_ID );	// Список индексов объектов ( прямое отображение LinkID->Index )
	//CIndexCollector<unsigned> entrenchmentIDToIndexCollector( INVALID_NODE_ID );
	CIndexCollector<unsigned> entrenchmentLinkIDToIndexCollector( INVALID_NODE_ID );

	NLog::GetLogger()->Log( LT_NORMAL, fmt::format( "Check map: {}\n", szObjectName.c_str() ) );

	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	int nObjectCount = 0;
	CManipulatorManager::GetValue( &nObjectCount, pObjectManipulator, "Objects" );
	for ( unsigned nObjectIndex = 0; nObjectIndex < nObjectCount; ++nObjectIndex )
	{
		const std::string szObjectPrefix = fmt::format( "Objects.[{}]", nObjectIndex );
		//
		std::string szRPGStatsTypeName;
		std::string szRPGStatsName;
		CManipulatorManager::GetParamsFromReference( szObjectPrefix + ".Object", pObjectManipulator, &szRPGStatsTypeName, &szRPGStatsName, 0 );
		if ( szRPGStatsTypeName.empty() || szRPGStatsName.empty() )
		{
			NLog::GetLogger()->Log( LT_ERROR, fmt::format("\tEmpty object: {}\n", nObjectIndex) );
			continue;
		}
		//
		unsigned nObjectLinkID = INVALID_NODE_ID;
		CManipulatorManager::GetValue( &nObjectLinkID, pObjectManipulator, fmt::format("{}.Link.LinkID", szObjectPrefix.c_str()) );
		if ( nObjectLinkID == INVALID_NODE_ID )
		{
			NLog::GetLogger()->Log( LT_ERROR, fmt::format("\tInvalid object LinkID: {}\n", nObjectIndex) );
			continue;
		}
		//
		if ( szRPGStatsTypeName == "EntrenchmentRPGStats" )
		{
			entrenchmentLinkIDToIndexCollector.Insert( nObjectLinkID, nObjectIndex, false );
		}
	}
	//
	int nEntrenchmentCount = 0;
	std::unordered_map<unsigned, unsigned> entrenchmentUsedLinkIDList;
	CManipulatorManager::GetValue( &nEntrenchmentCount, pObjectManipulator, "Entrenchments" );
	for ( unsigned nEntrenchmentIndex = 0; nEntrenchmentIndex < nObjectCount; ++nEntrenchmentIndex )
	{
		const std::string szEntrenchmentPrefix = fmt::format( "Entrenchments.[{}]", nEntrenchmentIndex );
		//
		int nSectionCount = 0;
		CManipulatorManager::GetValue( &nSectionCount, pObjectManipulator, fmt::format("{}.sections", szEntrenchmentPrefix.c_str()) );
		for ( unsigned nSectionIndex = 0; nSectionIndex < nSectionCount; ++nSectionIndex )
		{
			const std::string szSectionPrefix = fmt::format( "{}.sections.[{}]", szEntrenchmentPrefix.c_str(), nSectionIndex );
			//
			std::vector<int> sectionPartList;
			CManipulatorManager::GetArray<std::vector<int>, int>( &sectionPartList, pObjectManipulator, fmt::format("{}.data", szSectionPrefix.c_str()) );
			for ( int nSectionPartIndex = 0; nSectionPartIndex < sectionPartList.size(); ++nSectionPartIndex )
			{
				if ( entrenchmentLinkIDToIndexCollector.Get(sectionPartList[nSectionPartIndex]) == INVALID_NODE_ID )
				{
					NLog::GetLogger()->Log( LT_ERROR, fmt::format("\tInvalid entrenchment part: entrenchment: {} section: {}, part: {}\n", nEntrenchmentIndex, nSectionIndex, nSectionPartIndex) );
				}
				entrenchmentUsedLinkIDList[sectionPartList[nSectionPartIndex]] = 0;
			}
		}
	}
	for ( CIndexCollector<unsigned>::CIDToIndexMap::const_iterator itIDToIndex = entrenchmentLinkIDToIndexCollector.GetIDToIndexMap().begin(); itIDToIndex != entrenchmentLinkIDToIndexCollector.GetIDToIndexMap().end(); ++itIDToIndex )
	{
		if ( entrenchmentUsedLinkIDList.find( itIDToIndex->first ) == entrenchmentUsedLinkIDList.end() )
		{
			NLog::GetLogger()->Log( LT_ERROR, fmt::format("\tInvalid entrenchment part: object index: {}\n", itIDToIndex->second) );
		}
	}
	NLog::GetLogger()->Log( LT_NORMAL, "Done.\n" );
	return bResult;
}


CMapInfoExporter::CMapInfoExporter()
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_MAPINFO_EXPORTER, this );
	Singleton<ICommandHandlerContainer>()->Register( CHID_MAPINFO_EXPORTER, ID_TOOLS_DEBUG_CHECK_MAP, ID_TOOLS_REGEN_GEOMETRY );
}


CMapInfoExporter::~CMapInfoExporter()
{
	Singleton<ICommandHandlerContainer>()->UnRegister( CHID_MAPINFO_EXPORTER );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_MAPINFO_EXPORTER );
}


bool CMapInfoExporter::StartExport( const std::string &rszObjectTypeName, bool bForce )
{
	return true;
}


EXPORT_RESULT CMapInfoExporter::ExportObject( IManipulator* pManipulator,
																							const std::string &rszObjectTypeName,
																							const std::string &rszObjectName,
																							bool bForce,
																							EXPORT_TYPE exportType )
{
	NI_ASSERT( pManipulator != 0, "CMapInfoExporter::ExportObject() pManipulator == 0" );
	if ( ( exportType != ET_BEFORE_REF ) && ( exportType != ET_NO_REF ) )
	{
		return ER_SUCCESS;
	}
	//
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	std::string szMapFilesPath;
	if ( CManipulatorManager::GetValue(&szMapFilesPath, pManipulator, "MapFilesPath") == false || szMapFilesPath.empty() )
		return ER_FAIL;
	const std::string szSrcPath = pUserData->constUserData.szExportSourceFolder + szMapFilesPath + "map.b2m";
	//const std::string szDstPath = pUserData->szExportDestinationFolder + fmt::format( "bin\\maps\\{}", nObjectID );
	CVariant vtGUID;
	if ( pManipulator->GetValue( "uid", &vtGUID ) == false || vtGUID.GetType() != CVariant::VT_POINTER || vtGUID.GetBlobSize() != 16 )
		return ER_FAIL;
	GUID guid = *( (GUID*)vtGUID.GetPtr() );
//	const NDb::SMapInfo *pMapInfo = NDb::Get<NDb::SMapInfo>( nObjectID );
//	if ( !pMapInfo )
//		return ER_FAIL;
	std::string szGUID;
	NStr::GUID2String( &szGUID, guid );
	const std::string szDstPath = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + "bin\\maps\\" + szGUID;
	//
	if ( NFile::CopyFile( szSrcPath, szDstPath ) == 0 )
	{
		ILogger *pLogger = NLog::GetLogger();
		pLogger->Log( LT_ERROR, "Can't export map\n" );
		pLogger->Log( LT_ERROR, fmt::format("\tName: {}\n", rszObjectName.c_str()) );
		return ER_FAIL;
	}
	//
	return ER_SUCCESS;
}


bool CMapInfoExporter::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	SObjectSet objectSet;
	bool bResult = false;
	//
	switch( nCommandID )
	{
		case ID_TOOLS_REGEN_GEOMETRY:
		{	
			if ( !Singleton<ICommandHandlerContainer>()->HandleCommand( ID_VIEW_SAVE_CHANGES, true ) )
			{
				return false;
			}
			if ( GetSelectedObjects(&objectSet, "MapInfo") )
			{
				bResult = ForEachObject( objectSet, SReGenerateGeometry() );
			}
			break;
		}
		case ID_TOOLS_DEBUG_CHECK_MAP:
		{
			if ( GetSelectedObjects(&objectSet, "MapInfo") )
			{
				bResult = ForEachObject( objectSet, SCheck() );
			}
			break;
		}
	}
	/*
	if ( bResult ) 
	{
		Singleton<IEditorContainer>()->ReloadActiveEditor( true );
	}
	*/
	//
	return bResult;
}


bool CMapInfoExporter::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CAnimationBuilder::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CAnimationBuilder::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_TOOLS_REGEN_GEOMETRY:
		case ID_TOOLS_DEBUG_CHECK_MAP:
		{
			SObjectSet objectSet;
			bool bResult = GetSelectedObjects( &objectSet, "MapInfo" );
			( *pbEnable ) = bResult;
			( *pbCheck ) = false;
			return true;
		}
		default:
			return false;
	}
	return false;
}


