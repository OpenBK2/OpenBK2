#include "StdAfx.h"

#include "MapInfoBuilder.h"
#include "../MapEditorLib/BuilderFactory.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "../MapEditorLib/Tools_HashSet.h"
#include "../MapEditorLib/Interface_Logger.h"
#include "../MapEditorLib/Interface_Exporter.h"
#include "../libdb/ResourceManager.h"
#include "../Misc/StrProc.h"
#include "../System/FilePath.h"
#include "../System/FileUtils.h"
#include "../Image/Targa.h"
#include "EditorOptions.h"
#include "BuilderMethods.h"
#include "EditorMethods.h"
#include "../Stats_B2_M1/DBMapInfo.h"
#include "../SceneB2/TerraGen.h"
#include "EditorScene.h"
#include "../System/VFSOperations.h"
#include "../MapEditorLib/Interface_MOD.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


REGISTER_BUILDER_IN_DLL( MapInfo, CMapInfoBuilder )


const int CMapInfoBuilder::MIN_PLAYER_COUNT = 3;
const int CMapInfoBuilder::MAX_PLAYER_COUNT = 17;
const int CMapInfoBuilder::MIN_TERRAIN_SIZE_X = 1;
const int CMapInfoBuilder::MIN_TERRAIN_SIZE_Y = 1;
const int CMapInfoBuilder::MAX_TERRAIN_SIZE_X = 20;
const int CMapInfoBuilder::MAX_TERRAIN_SIZE_Y = 20;
const char CMapInfoBuilder::TEXTURE_TYPE_NAME[] = "Texture";
const char CMapInfoBuilder::MATERIAL_TYPE_NAME[] = "Material";
const string CMapInfoBuilder::BUILD_DATA_TYPE_NAME = "MapInfoBuilder";
const string CMapInfoBuilder::COPY_DATA_TYPE_NAME = "MapInfoCopier";
const string CMapInfoBuilder::MAPINFO_TYPE_NAME = "MapInfo";
const string CMapInfoBuilder::MAPINFO_DEFAULT_FOLDER = "Custom\\Missions\\";



bool CMapInfoBuilder::EnsureMinimapMaterialAndTexture( IManipulator *pObjectManipulator, const CDBID &dbid )
{
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	//
	bool bResult = false;
	string szMiniMapMaterialName;
	CManipulatorManager::GetValue( &szMiniMapMaterialName, pObjectManipulator, "MiniMap" );
	if ( !szMiniMapMaterialName.empty() && szMiniMapMaterialName != " " ) 
	{
		if ( CPtr<IManipulator> pMaterialMan = pResourceManager->CreateObjectManipulator( "Material", szMiniMapMaterialName ) )
		{
			string szMiniMapTextureName;
			CManipulatorManager::GetValue( &szMiniMapTextureName, pMaterialMan, "Texture" );
			if ( !szMiniMapTextureName.empty() && szMiniMapTextureName != " " ) 
			{
				// delete texture source and destination
				if ( CPtr<IManipulator> pTexMan = pResourceManager->CreateObjectManipulator( "Texture", szMiniMapTextureName ) )
				{
					string szSrcName;
					if ( CManipulatorManager::GetValue( &szSrcName, pTexMan, "SrcName" ) != false && !szSrcName.empty() && szSrcName != " " )
					{
						bResult = true;
					}
				}
			}
		}
	}
	if ( !bResult )
	{
		if ( CPtr<IManipulator> pFolderManipulator = pResourceManager->CreateFolderManipulator( MAPINFO_TYPE_NAME ) )
		{
			bResult = true;
			//
			string szObjectName;
			string szMapFilesPath;
			string szMiniMapMaterialName;
			//
			const string szFolderName = NDb::GetFolderName( dbid );
//			bResult = bResult && CManipulatorManager::GetValue( &szMapFilesPath, pObjectManipulator, "MapFilesPath" );
			bResult = bResult && MakeMinimapMaterialAndTexture( &szMiniMapMaterialName, szFolderName );
			bResult = bResult && CManipulatorManager::SetValue( szMiniMapMaterialName, pObjectManipulator, "MiniMap", true );
		}
	}
	return bResult;
}


bool CMapInfoBuilder::MakeMinimapMaterialAndTexture( string *pszObjectName, const string &szFolder )
{
	try
	{
		IResourceManager *pRM = Singleton<IResourceManager>();
		IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
		bool bResult = true;
		// create texture
		const string szTextureName = szFolder + "minimap_texture.xdb";
		if ( pFolderCallback->InsertObject(TEXTURE_TYPE_NAME, szTextureName) == false )
			return false;
		CPtr<IManipulator> pTexMan = pRM->CreateObjectManipulator( TEXTURE_TYPE_NAME, szTextureName );
		bResult = bResult && CManipulatorManager::SetValue( "TEXTURE_2D", pTexMan, "Type", false );
		bResult = bResult && CManipulatorManager::SetValue( "CONVERT_TRANSPARENT", pTexMan, "ConversionType", false );
		bResult = bResult && CManipulatorManager::SetValue( "TF_8888", pTexMan, "Format", false );
		bResult = bResult && CManipulatorManager::SetValue( szFolder + "minimap.tga", pTexMan, "SrcName", false );
		bResult = bResult && CManipulatorManager::SetValue( 1, pTexMan, "NMips" );
		if ( bResult ) 
		{
			// create empty texture image
			CArray2D<DWORD> image( 256, 256 );
			image.FillEvery( 0xffffffff );
			const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
			CFileStream stream( pUserData->constUserData.szExportSourceFolder + szFolder + "minimap.tga", CFileStream::WIN_CREATE );
			NImage::SaveAsTGA( image, &stream );
			// export new texture
			Singleton<IExporterContainer>()->ExportObject( pTexMan, TEXTURE_TYPE_NAME, szTextureName, true, false );
		}
		// create material
		const string szMaterialName = szFolder + "minimap_material.xdb";
		if ( pFolderCallback->InsertObject(MATERIAL_TYPE_NAME, szMaterialName) == false )
			return false;
		CPtr<IManipulator> pMatMan = pRM->CreateObjectManipulator( MATERIAL_TYPE_NAME, szMaterialName );
		bResult = bResult && CManipulatorManager::SetValue( szTextureName, pMatMan, "Texture", true );
		bResult = bResult && CManipulatorManager::SetValue( "AM_TRANSPARENT", pMatMan, "AlphaMode", false );
		//
		*pszObjectName = szMaterialName;
		return bResult;
	}
	catch ( ... ) 
	{
		return false;
	}
}

bool CMapInfoBuilder::IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView )
{
	NI_ASSERT( pBuildDataManipulator != 0, "CMapInfoBuilder::IsValidBuildData() pBuildDataManipulator == 0" );
	NI_ASSERT( pszDescription != 0, "CMapInfoBuilder::IsValidBuildData() pszDescription == 0" );
	pszDescription->clear();	

	string szTypeName;
	if ( pBuildDataManipulator->GetType( string(), &szTypeName ) )
	{
		if ( szTypeName == BUILD_DATA_TYPE_NAME )
		{
			return IsValidDataBuilder( pBuildDataManipulator, pszDescription, pBuildDataView );
		}
		else if ( szTypeName == COPY_DATA_TYPE_NAME )
		{
			return IsValidDataCopier( pBuildDataManipulator, pszDescription, pBuildDataView );
		}
	}
	return false;
}


//CRAP{ PLAIN_TEXT
bool CMapInfoBuilder::IsValidDataBuilder( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView )
{
	// Считываем данные
	int nPlayerCount = 0;
	if ( !CManipulatorManager::GetValue( &nPlayerCount, pBuildDataManipulator, "Players" ) ||
			 ( nPlayerCount < MIN_PLAYER_COUNT ) ||
			 ( nPlayerCount > MAX_PLAYER_COUNT ) )
	{
		( *pszDescription ) = StrFmt( "<Players> count must be in range [%d...%d].", MIN_PLAYER_COUNT, MAX_PLAYER_COUNT );
		return false;
	}
	else
	{
		for ( int nPlayerIndex = 0; nPlayerIndex < nPlayerCount; ++nPlayerIndex )
		{
			if ( CheckStringValue(pszDescription, StrFmt("Players.[%d]", nPlayerIndex), pBuildDataManipulator) == false ) 
			{
				return false;
			}
		}
	}
	if ( CheckIntValue(pszDescription, "SizeX", MIN_TERRAIN_SIZE_X, MAX_TERRAIN_SIZE_X, pBuildDataManipulator) == false )
	{
		return false;
	}
	if ( CheckIntValue(pszDescription, "SizeY", MIN_TERRAIN_SIZE_Y, MAX_TERRAIN_SIZE_Y, pBuildDataManipulator) == false )
	{
		return false;
	}
	if ( CheckStringValue(pszDescription, "Season", pBuildDataManipulator) == false ) 
	{
		return false;
	}
	if ( CheckStringValue(pszDescription, "DayTime", pBuildDataManipulator) == false ) 
	{
		return false;
	}
	return true;
}
//CRAP} PLAIN_TEXT


bool CMapInfoBuilder::IsValidDataCopier( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView )
{
	return true;
}


bool CMapInfoBuilder::InternalInsertObject( string *pszObjectTypeName,
																						string *pszUniqueObjectName,
																						bool bFromMainMenu,
																						bool *pbCanChangeObjectName,
																						bool *pbNeedExport,
																						bool *pbNeedEdit,
																						IManipulator *pBuildDataManipulator )
{
	NI_ASSERT( pszObjectTypeName != 0, "CMapInfoBuilder::InternalInsertObject() pszObjectTypeName == 0" );
	NI_ASSERT( pszUniqueObjectName != 0, "CMapInfoBuilder::InternalInsertObject() pszUniqueObjectName == 0" );
	NI_ASSERT( pBuildDataManipulator != 0, "CMapInfoBuilder::InternalInsertObject() pBuildDataManipulator == 0" );
	IResourceManager *pRM = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	string szDescription;
	if ( !IsValidBuildData( pBuildDataManipulator, &szDescription, 0 ) )
	{
		return false;
	}
	const string szFullObjectName = *pszUniqueObjectName;
	string szMapInfoFolder;
	CStringManager::SplitFileName( &szMapInfoFolder, 0, 0, szFullObjectName );
	// Считываем данные
	CTPoint<int> size( 0, 0 );
	string szSeasonName;
	string szDayTimeName;
	int nPlayerCount = 0;
	CManipulatorManager::GetValue( &( size.x ), pBuildDataManipulator, "SizeX" );
	CManipulatorManager::GetValue( &( size.y ), pBuildDataManipulator, "SizeY" );
	CManipulatorManager::GetValue( &szSeasonName, pBuildDataManipulator, "Season" );
	CManipulatorManager::GetValue( &szDayTimeName, pBuildDataManipulator, "DayTime" );
	CManipulatorManager::GetValue( &nPlayerCount, pBuildDataManipulator, "Players" );
	NFile::CreatePath( (pUserData->constUserData.szExportSourceFolder + szMapInfoFolder).c_str() );
	NFile::CreatePath( (Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + szMapInfoFolder).c_str() );
	// retrieve from options
	const string szTilesetName = NEditorOptions::GetTileset( szSeasonName );
	const string szLightName = NEditorOptions::GetLight( szSeasonName, szDayTimeName );
	const string szPreLightName = NEditorOptions::GetPreLight( szSeasonName, szDayTimeName );
	const string szOceanWater = NEditorOptions::GetOceanWater( szSeasonName );
	if ( szTilesetName.empty() || szLightName.empty() || szPreLightName.empty() || szOceanWater.empty() ) 
		return false;
	//
	bool bResult = pFolderCallback->InsertObject( *pszObjectTypeName, szFullObjectName );
	if ( bResult )
	{
		CPtr<IManipulator> pMapInfoManipulator = pRM->CreateObjectManipulator( *pszObjectTypeName, szFullObjectName );
		NI_ASSERT( pMapInfoManipulator != 0, "CMapInfoBuilder::InternalInsertObject() pMapInfoManipulator == 0" );
		// Проставляем сезон & daytime
		bResult = bResult && pMapInfoManipulator->SetValue( "Season", szSeasonName );
		bResult = bResult && pMapInfoManipulator->SetValue( "DayTime", szDayTimeName );
		// Проставляем размеры
		bResult = bResult && pMapInfoManipulator->SetValue( "NumPatchesX", size.x );
		bResult = bResult && pMapInfoManipulator->SetValue( "NumPatchesY", size.y );
		// Проставляем свет
		bResult = bResult && pMapInfoManipulator->SetValue( "Light", szLightName );
		bResult = bResult && pMapInfoManipulator->SetValue( "PreLight", szPreLightName );
		// tileset
		bResult = bResult && pMapInfoManipulator->SetValue( "TerraSet", szTilesetName );
		// ocean water
		bResult = bResult && pMapInfoManipulator->SetValue( "OceanWater", szOceanWater );
		// Добавляем информацию о игроках
		const int nNeutralPlayerIndex = nPlayerCount - 1;
		const int nNumFriendlyPlayers = int( (nPlayerCount - 1) / 2 );

		int nExistingPlayersNumber = 0;
		bResult = bResult && CManipulatorManager::GetValue( &nExistingPlayersNumber, pMapInfoManipulator, "Players" );
		if ( bResult )
		{
			for ( int nPlayerIndex = nExistingPlayersNumber; nPlayerIndex < nPlayerCount; ++nPlayerIndex )
			{
				bResult = bResult && pMapInfoManipulator->InsertNode( "Players" );
				if ( !bResult ) break;
			}
			for ( int nPlayerIndex = 0; nPlayerIndex < nPlayerCount; ++nPlayerIndex )
			{
				string szParty;
				bResult = bResult && CManipulatorManager::GetValue( &szParty, pBuildDataManipulator, StrFmt( "Players.[%d]", nPlayerIndex ) );
				bResult = bResult && CManipulatorManager::SetValue( szParty, pMapInfoManipulator, StrFmt( "Players.[%d].PartyInfo", nPlayerIndex ), true );
				// set automatic diplomacy
				int nPlayerDiplomacySide = 1;
				if ( nPlayerIndex < nNumFriendlyPlayers )
					nPlayerDiplomacySide = 0;
				else if ( nPlayerIndex == nNeutralPlayerIndex )
					nPlayerDiplomacySide = 2;
				bResult = bResult && CManipulatorManager::SetValue( nPlayerDiplomacySide, pMapInfoManipulator, StrFmt( "Players.[%d].DiplomacySide", nPlayerIndex ) );
				if ( !bResult ) break;
			}
		}
		// Добавляем информацию о дипломатических отношениях
		int nExistingDiplomaciesNumber = 0;
		bResult = bResult && CManipulatorManager::GetValue( &nExistingDiplomaciesNumber, pMapInfoManipulator, "Diplomacies" );
		if ( bResult )
		{
			for ( int nDiplomacyIndex = nExistingDiplomaciesNumber; nDiplomacyIndex < nPlayerCount; ++nDiplomacyIndex )
			{
				bResult = bResult && pMapInfoManipulator->InsertNode( "Diplomacies" );
				if ( !bResult ) break;
			}
			bResult = bResult && pMapInfoManipulator->SetValue( "Diplomacies.[0]", 0 );
			for ( int nDiplomacyIndex = 1; nDiplomacyIndex < ( nPlayerCount - 1 ); ++nDiplomacyIndex )
			{
				bResult = bResult && pMapInfoManipulator->SetValue( StrFmt( "Diplomacies.[%d]", nDiplomacyIndex ), 1 );
				if ( !bResult ) break;
			}
			bResult = bResult && pMapInfoManipulator->SetValue( StrFmt( "Diplomacies.[%d]", ( nPlayerCount - 1 ) ), 2 );
		}
		// make new minimap material and texture
		string szMinimapMaterialName;
		if ( MakeMinimapMaterialAndTexture( &szMinimapMaterialName, szMapInfoFolder ) != false )
		{
			bResult = bResult && CManipulatorManager::SetValue( szMinimapMaterialName, pMapInfoManipulator, "MiniMap", true );
		}
		// regenerate geometry for terrain
		if ( const NDb::SMapInfo *pMapInfo = NDb::Get<NDb::SMapInfo>( CDBID( szFullObjectName ) ) )
		{
			if ( CPtr<ITerraManager> pTerraManager = MakeObject<ITerraManager>( ITerraManager::tidTypeID ) )
			{
				//pTerrain->SetStreamPathes( pUserData->szExportDestinationFolder, pUserData->szExportSourceFolder );
				NEditor::CreateTerrain( pTerraManager, pMapInfo );
				//pTerrain->ReGenerate();
				NEditor::SaveTerrain( pTerraManager );
			}
		}
	}
  //
	return bResult;
}


bool CMapInfoBuilder::CopyObject( const string &rszObjectTypeName, const string &rszDestination, const string &rszSource )
{
	SBuildDataParams buildDataParams;
	buildDataParams.szObjectTypeName = rszObjectTypeName;
	CStringManager::SplitFileName( &( buildDataParams.szObjectNamePrefix ), 0, 0, rszDestination );
	buildDataParams.szObjectName = "New Map";
	buildDataParams.szObjectNamePostfix = "\\mapinfo.xdb";
	buildDataParams.szObjectNameExtention = ".xdb";
	buildDataParams.bNeedExport = false;
	buildDataParams.bNeedEdit = false;
	//
	string szBuildDataTypeName = COPY_DATA_TYPE_NAME;
	string szBuildDataName;
	if ( Singleton<IBuilderContainer>()->FillBuildData( &szBuildDataTypeName, &szBuildDataName, &buildDataParams, this ) )
	{
		if ( CPtr<IManipulator> pBuildDataManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szBuildDataTypeName, szBuildDataName ) )
		{
			string szDescription;
			if ( IsValidDataCopier( pBuildDataManipulator, &szDescription, 0 ) )
			{
				string szObjectName;
				buildDataParams.GetObjectName( &szObjectName );
				// call default implementation of copying
				if ( CBuilderBase::CopyObject( rszObjectTypeName, szObjectName, rszSource ) )
				{
					return InternalCopy( rszObjectTypeName, szObjectName, rszSource, pBuildDataManipulator );
				}
			}
		}
	}
	return false;
}


void TrippleCopy( const string &szSrcFileName, const string &szDstFileName, const SUserData *pUserData )
{
	NFile::CopyFile( szSrcFileName, pUserData->constUserData.szDataStorageFolder + szDstFileName );
	//NFile::CopyFile( szSrcFileName, Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + szDstFileName );
	//NFile::CopyFile( szSrcFileName, pUserData->constUserData.szExportSourceFolder + szDstFileName );
}


bool CMapInfoBuilder::InternalCopy( const string &rszObjectTypeName, const string &_szDestination, const string &rszSource, IManipulator *pBuildDataManipulator )
{
	ILogger *pLogger = NLog::GetLogger();
	IResourceManager *pRM = Singleton<IResourceManager>();
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	bool bResult = true;

	const string szDestination = _szDestination;
	CPtr<IManipulator> pMapInfoManipulator = pRM->CreateObjectManipulator( rszObjectTypeName, szDestination );
	NI_ASSERT( pMapInfoManipulator != 0, "CMapInfoBuilder::InternalCopy() pMapInfoManipulator == 0" );

	const NDb::SMapInfo *pMapInfo = NDb::Get<NDb::SMapInfo>( CDBID( rszSource ) );
	if ( !pMapInfo )
		return false;

	const string szOldFolderName = rszSource.substr( 0, rszSource.rfind('\\') + 1 );
	const string szNewFolderName = szDestination.substr( 0, szDestination.rfind('\\') + 1 );
	// copy file with map
	{
		string szSourceMapFileName;
		string szMapFileName = pUserData->constUserData.szDataStorageFolder + szOldFolderName + "map.b2m";
		if ( NFile::DoesFileExist( szMapFileName ) )
		{
			szSourceMapFileName = szMapFileName;
		}
		//
		if ( szSourceMapFileName.empty() )
			return false;
		//
		TrippleCopy( szSourceMapFileName, szNewFolderName + "map.b2m", pUserData );
	}
	// make new minimap material & texture
	{
		string szMinimapMaterialName;
		if ( MakeMinimapMaterialAndTexture( &szMinimapMaterialName, szNewFolderName ) != false )
			bResult = bResult && CManipulatorManager::SetValue( szMinimapMaterialName, pMapInfoManipulator, "MiniMap", true );
	}
	// copy raw script file
	{
		string szSrcScriptFileName = pUserData->constUserData.szExportSourceFolder + szOldFolderName + "script.lua";
		if ( NFile::DoesFileExist(szSrcScriptFileName) )
			TrippleCopy( szSrcScriptFileName, szNewFolderName + "script.lua", pUserData );
		else
		{
			szSrcScriptFileName = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + szOldFolderName + "script.lua";
			if ( NFile::DoesFileExist(szSrcScriptFileName) )
				TrippleCopy( szSrcScriptFileName, szNewFolderName + "script.lua", pUserData );
			else
			{
				szSrcScriptFileName = pUserData->constUserData.szDataStorageFolder + szOldFolderName + "script.lua";
				if ( NFile::DoesFileExist(szSrcScriptFileName) )
					TrippleCopy( szSrcScriptFileName, szNewFolderName + "script.lua", pUserData );
			}
		}

	}
	return bResult;
}

bool CMapInfoBuilder::RemoveObject( const string &rszObjectTypeName, const string &rszObjectName )
{
	IResourceManager *pRM = Singleton<IResourceManager>();
	const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	CPtr<IManipulator> pMapInfoMan = pRM->CreateObjectManipulator( rszObjectTypeName, rszObjectName );
	NI_ASSERT( pMapInfoMan != 0, "CMapInfoBuilder::RemoveObject() pMapInfoManipulator == 0" );
	// check for minimap
	string szMiniMapMaterialName;
	CManipulatorManager::GetValue( &szMiniMapMaterialName, pMapInfoMan, "MiniMap" );
	// remove map.b2m
	{
		const string szMapFileName = NDb::GetFileName( CDBID(rszObjectName) );
		const string szDstFolderPath = Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION );
		const string szFilePath = NFile::GetFilePath( szDstFolderPath + szMapFileName );
		::DeleteFile( (szFilePath + "map.b2m").c_str() );
	}
	// remove MapInfo object
	CBuilderBase::RemoveObject( rszObjectTypeName, rszObjectName );
	// remove minimap
	if ( !szMiniMapMaterialName.empty() && szMiniMapMaterialName != " " ) 
	{
		if ( CPtr<IManipulator> pMaterialMan = pRM->CreateObjectManipulator(MATERIAL_TYPE_NAME, szMiniMapMaterialName) )
		{
			string szMiniMapTextureName;
			CManipulatorManager::GetValue( &szMiniMapTextureName, pMaterialMan, "Texture" );
			// remove minimap material
			CBuilderBase::RemoveObject( MATERIAL_TYPE_NAME, szMiniMapMaterialName );
			//
			if ( !szMiniMapTextureName.empty() && szMiniMapTextureName != " " ) 
			{
				// delete texture source and destination
				if ( CPtr<IManipulator> pTextureMan = pRM->CreateObjectManipulator(TEXTURE_TYPE_NAME, szMiniMapTextureName) )
				{
					string szSrcName;
					if ( CManipulatorManager::GetValue(&szSrcName, pTextureMan, "SrcName") != false && !szSrcName.empty() )
					{
						::DeleteFile( (pUserData->constUserData.szExportSourceFolder + szSrcName).c_str() );
					}
					string szDstName;
					if ( CManipulatorManager::GetValue(&szDstName, pTextureMan, "DestName") != false && !szDstName.empty() )
					{
						::DeleteFile( (Singleton<IMODContainer>()->GetDataFolder( SUserData::NPT_EXPORT_DESTINATION ) + szDstName).c_str() );
					}
				}
				// remove minimap texture
				CBuilderBase::RemoveObject( TEXTURE_TYPE_NAME, szMiniMapTextureName );
			}
		}
	}
	//
	return true;
}


void CMapInfoBuilder::GetDefaultFolder( const string &rszObjectTypeName, string *pszDefaultFolder )
{
	if ( rszObjectTypeName == MAPINFO_TYPE_NAME )
	{
		if ( pszDefaultFolder )
		{
			( *pszDefaultFolder ) = MAPINFO_DEFAULT_FOLDER;
		}
	}
}




bool CMapInfoBuilder::InsertObject( string *pszObjectTypeName,
																		string *pszUniqueObjectName,
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
	CStringManager::SplitFileName( &( buildDataParams.szObjectNamePrefix ), 0, 0, ( *pszUniqueObjectName ) );
	buildDataParams.szObjectName = "New Map";
	buildDataParams.szObjectNamePostfix = "\\mapinfo.xdb";
	buildDataParams.szObjectNameExtention = ".xdb";
	buildDataParams.bNeedExport = ( *pbNeedExport );
	buildDataParams.bNeedEdit = ( *pbNeedEdit );
	//
	string szBuildDataTypeName = GetBuildDataTypeName();
	string szBuildDataName;
	if ( Singleton<IBuilderContainer>()->FillBuildData( &szBuildDataTypeName, &szBuildDataName, &buildDataParams, this ) )
	{
		( *pszObjectTypeName ) = buildDataParams.szObjectTypeName;
		buildDataParams.GetObjectName( pszUniqueObjectName );
		( *pbNeedExport ) = buildDataParams.bNeedExport;
		( *pbNeedEdit ) = buildDataParams.bNeedEdit;
		if ( CPtr<IManipulator> pBuildDataManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szBuildDataTypeName, szBuildDataName ) )
		{
			string szDescription;
			if ( IsValidBuildData( pBuildDataManipulator, &szDescription, 0 ) )
			{
				return InternalInsertObject( pszObjectTypeName, pszUniqueObjectName, bFromMainMenu, pbCanChangeObjectName, pbNeedExport, pbNeedEdit, pBuildDataManipulator );
			}
		}
	}
	return false;
}

// basement storage  


