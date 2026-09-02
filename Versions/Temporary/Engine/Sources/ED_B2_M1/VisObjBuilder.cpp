#include "stdafx.h"
#include <fmt/format.h>
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ResourceDefines.h"
#include "Misc/2Darray.h"
#include "CommandHandlerDefines.h"
#include "ResourceDefines.h"

#include "VisObjBuilder.h"
#include "SeasonMnemonics.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "MapEditorLib/BuilderFactory.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "libdb/ResourceManager.h"
#include "System/FileUtils.h"

#include <cstdint>

#include <zconf.h>

REGISTER_BUILDER_IN_DLL( VisObj, CVisObjBuilder )

const char CVisObjBuilder::VISOBJ_TYPE_NAME[]							= "VisObj";
const char CVisObjBuilder::MODEL_TYPE_NAME[]							= "Model";
const char CVisObjBuilder::MATERIAL_TYPE_NAME[]						= "Material";
const char CVisObjBuilder::TEXTURE_TYPE_NAME[]						= "Texture";
const char CVisObjBuilder::GEOMETRY_TYPE_NAME[]						= "Geometry";
const char CVisObjBuilder::AIGEOMETRY_TYPE_NAME[]					= "AIGeometry";
const char CVisObjBuilder::SKELETON_TYPE_NAME[]						= "Skeleton";
const char CVisObjBuilder::MODEL_FILE_NAME_EXTENTION[]		= ".mb";
const char CVisObjBuilder::TEXTURE_FILE_NAME_EXTENTION[]	= ".tga";
const std::string CVisObjBuilder::BUILD_DATA_TYPE_NAME					= "VisObjBuilder";
const std::string CVisObjBuilder::RESOURCE_PREFIX[RT_COUNT] =
{
	CVisObjBuilder::MODEL_TYPE_NAME,
	CVisObjBuilder::MATERIAL_TYPE_NAME,
	CVisObjBuilder::TEXTURE_TYPE_NAME,
	CVisObjBuilder::GEOMETRY_TYPE_NAME,
	CVisObjBuilder::AIGEOMETRY_TYPE_NAME,
	CVisObjBuilder::SKELETON_TYPE_NAME,
//	fmt::format( "{}", CVisObjBuilder::MODEL_TYPE_NAME ),
//	fmt::format( "{}\\{}", CVisObjBuilder::MODEL_TYPE_NAME, CVisObjBuilder::MATERIAL_TYPE_NAME ),
//	fmt::format( "{}\\{}\\{}", CVisObjBuilder::MODEL_TYPE_NAME, CVisObjBuilder::MATERIAL_TYPE_NAME, CVisObjBuilder::TEXTURE_TYPE_NAME ),
//	fmt::format( "{}\\{}", CVisObjBuilder::MODEL_TYPE_NAME, CVisObjBuilder::GEOMETRY_TYPE_NAME ),
//	fmt::format( "{}\\{}\\{}", CVisObjBuilder::MODEL_TYPE_NAME, CVisObjBuilder::GEOMETRY_TYPE_NAME, CVisObjBuilder::AIGEOMETRY_TYPE_NAME ),
//	fmt::format( "{}\\{}", CVisObjBuilder::MODEL_TYPE_NAME, CVisObjBuilder::SKELETON_TYPE_NAME ),
};


CVisObjBuilder::CVisObjBuilder()
{
	Singleton<ICommandHandlerContainer>()->Set( CHID_VISOBJ_BUILDER, this );
	Singleton<ICommandHandlerContainer>()->Register( CHID_VISOBJ_BUILDER, ID_TOOLS_CREATE_VIS_OBJ, ID_TOOLS_CREATE_VIS_OBJ );
}


CVisObjBuilder::~CVisObjBuilder()
{
	Singleton<ICommandHandlerContainer>()->UnRegister( CHID_VISOBJ_BUILDER );
	Singleton<ICommandHandlerContainer>()->Remove( CHID_VISOBJ_BUILDER );
}


void CVisObjBuilder::GetSeasonedFolderName( std::string *pszFileName, NDb::ESeason eSeason )
{
	if ( pszFileName )
	{
		std::string szFilePath;
		std::string szFileName;
		std::string szFileExtention;
		CStringManager::SplitFileName( &szFilePath, &szFileName, &szFileExtention, *pszFileName );
		const std::string szSeasonPostfix = typeSeasonFolderPostfixMnemonics.GetMnemonic( eSeason ) + std::string( "\\" );
		( *pszFileName ) = szFilePath + szSeasonPostfix + szFileName + szFileExtention;
	}
}


void CVisObjBuilder::GetSeasonedFileName( std::string *pszFileName, NDb::ESeason eSeason )
{
	if ( pszFileName )
	{
		std::string szFilePath;
		std::string szFileName;
		std::string szFileExtention;
		CStringManager::SplitFileName( &szFilePath, &szFileName, &szFileExtention, *pszFileName );
		const std::string szSeasonPostfix = typeSeasonFilePostfixMnemonics.GetMnemonic( eSeason );
		( *pszFileName ) = szFilePath + szFileName + szSeasonPostfix + szFileExtention;
	}
}


void CVisObjBuilder::GetResourceFileName( std::string *pszResourceFileName, EResourceType eResourceType, const std::string &rszVisObjFileName )
{
	if ( pszResourceFileName )
	{
		std::string szVisObjFilePath;
		CStringManager::SplitFileName( &szVisObjFilePath, 0, 0, rszVisObjFileName );
		//
		std::string szResourceFileName;
		CStringManager::SplitFileName( 0, &szResourceFileName, 0, *pszResourceFileName );
		( *pszResourceFileName ) = szVisObjFilePath + szResourceFileName + "_" + RESOURCE_PREFIX[eResourceType] + ".xdb";
//		( *pszResourceFileName ) = szVisObjFilePath + RESOURCE_PREFIX[eResourceType] + std::string( "\\" ) + szResourceFileName + ".xdb";
	}
}


bool CVisObjBuilder::AddVisObjEntry( const std::string &rszUniqueObjectName,
																		 IManipulator *pBuildDataManipulator,	
																		 const std::string &rszMBFullFileName,
																		 const std::string &rszTGAFullFileName,
																		 NDb::ESeason eSeason )
{
	NI_ASSERT( pBuildDataManipulator != 0, "CTerrainBuilder::AddVisObjEntry() pBuildDataManipulator == 0" );
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	// Считываем данные
	std::string szMBFullFileName;
	if ( rszMBFullFileName.empty() )
	{
		CManipulatorManager::GetValue( &szMBFullFileName, pBuildDataManipulator, "ModelFileName" );
	}
	else
	{
		szMBFullFileName = rszMBFullFileName;
	}
	std::string szTGAFullFileName;
	if ( rszTGAFullFileName.empty() )
	{
		CManipulatorManager::GetValue( &szTGAFullFileName, pBuildDataManipulator, "TextureFileName" );
	}
	else
	{
		szTGAFullFileName = rszTGAFullFileName;
	}
	//
	const std::string szSeasonName = typeSeasonMnemonics.GetMnemonic( eSeason );
	const std::string szExportSourceFolder = Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder;
	//
	std::string szMBSeasonedFileName;
	std::string szTGASeasonedFileName;
	CStringManager::SplitFileName( 0, &szMBSeasonedFileName, 0, szMBFullFileName );
	CStringManager::SplitFileName( 0, &szTGASeasonedFileName, 0, szTGAFullFileName );
	bool bMBSeasonedFileExists = false;
	bool bTGASeasonedFileExists = false;
	std::string szMBSeasonedFullFileName = szMBFullFileName;
	std::string szTGASeasonedFullFileName = szTGAFullFileName;
	{
		GetSeasonedFileName( &szMBSeasonedFullFileName, eSeason );
		bMBSeasonedFileExists = NFile::DoesFileExist( szExportSourceFolder + szMBSeasonedFullFileName );
		if ( bMBSeasonedFileExists )
		{
			GetSeasonedFileName( &szMBSeasonedFileName, eSeason );
		}
		else
		{
			szMBSeasonedFullFileName = szMBFullFileName;
			GetSeasonedFileName( &szMBSeasonedFullFileName, NDB_DEFAULT_SEASON );
			GetSeasonedFileName( &szMBSeasonedFileName, NDB_DEFAULT_SEASON );
		}
		//
		GetSeasonedFileName( &szTGASeasonedFullFileName, eSeason );
		bTGASeasonedFileExists = NFile::DoesFileExist( szExportSourceFolder + szTGASeasonedFullFileName );
		if( bTGASeasonedFileExists )
		{
			GetSeasonedFileName( &szTGASeasonedFileName, eSeason );
		}
		else
		{
			szTGASeasonedFullFileName = szTGAFullFileName;
			GetSeasonedFileName( &szTGASeasonedFullFileName, NDB_DEFAULT_SEASON );
			GetSeasonedFileName( &szTGASeasonedFileName, NDB_DEFAULT_SEASON );
		}
	}
	//
	std::string szTextureType;
	std::string szRootMesh;
	std::string szRootJoint;
	std::string szAIRootMesh;
	std::string szCommonSkeletonName;
	CManipulatorManager::GetValue( &szTextureType, pBuildDataManipulator, "TextureType" );
	CManipulatorManager::GetValue( &szRootMesh, pBuildDataManipulator, "RootMesh" );
	CManipulatorManager::GetValue( &szRootJoint, pBuildDataManipulator, "RootJoint" );
	CManipulatorManager::GetValue( &szAIRootMesh, pBuildDataManipulator, "AIRootMesh" );
	CManipulatorManager::GetValue( &szCommonSkeletonName, pBuildDataManipulator, "Skeleton" );
	//
	CPtr<IManipulator> pVisObjManipulator = pResourceManager->CreateObjectManipulator( VISOBJ_TYPE_NAME, rszUniqueObjectName );
	if ( pVisObjManipulator == 0 )
	{
		return false;
	}
	// Опередяем наличие файла модели, если есть новая модель создаем новые геометрию аигеометрию и скелет
	std::string szGeometryName = szMBSeasonedFileName;
	std::string szAIGeometryName = szMBSeasonedFileName;
	GetResourceFileName( &szGeometryName, RT_GEOMETRY, rszUniqueObjectName );
	GetResourceFileName( &szAIGeometryName, RT_AIGEOMETRY, rszUniqueObjectName );
	std::string szSkeletonName;
	if ( szCommonSkeletonName.empty() )
	{
		szSkeletonName = szMBSeasonedFileName;
		GetResourceFileName( &szSkeletonName, RT_SKELETON, rszUniqueObjectName );
	}
	else
	{
		szSkeletonName = szCommonSkeletonName;
	}
	//
	// Опередяем наличие файла текструры, если есть новая текстура создаем новые материал и текстуру
	std::string szMaterialName = szTGASeasonedFileName;
	std::string szTextureName = szTGASeasonedFileName;
	GetResourceFileName( &szMaterialName, RT_MATERIAL, rszUniqueObjectName );
	GetResourceFileName( &szTextureName, RT_TEXTURE, rszUniqueObjectName );
	//
	// Если есть новая модель или текстура - создаем новую модель
	std::string szModelName = szMBSeasonedFileName + "_" + szTGASeasonedFileName;
	GetResourceFileName( &szModelName, RT_MODEL, rszUniqueObjectName );
	//
	// добавляем модель
	bool bResult = true;
	if ( pFolderCallback->IsUniqueName( MODEL_TYPE_NAME, szModelName ) )
	{
		bResult = bResult && pFolderCallback->InsertObject( MODEL_TYPE_NAME, szModelName );
	}
	// добавляем материал
	if ( pFolderCallback->IsUniqueName( MATERIAL_TYPE_NAME, szMaterialName ) )
	{
		bResult = bResult && pFolderCallback->InsertObject( MATERIAL_TYPE_NAME, szMaterialName );
		//добавляем текстуру
		if ( pFolderCallback->IsUniqueName( TEXTURE_TYPE_NAME, szTextureName ) )
		{
			bResult = bResult && pFolderCallback->InsertObject( TEXTURE_TYPE_NAME, szTextureName );
			//Устанавливаем имя файла текстуры
			if ( bResult )
			{
				if ( CPtr<IManipulator> pTextureManipulator = pResourceManager->CreateObjectManipulator( TEXTURE_TYPE_NAME, szTextureName ) )
				{
					bResult = bResult && CManipulatorManager::SetValue( szTGASeasonedFullFileName, pTextureManipulator, "SrcName", false );
					bResult = bResult && CManipulatorManager::SetValue( "CONVERT_ORDINARY", pTextureManipulator, "ConversionType", false );
					if ( szTextureType == "A_OPAQUE" )
					{
						bResult = bResult && CManipulatorManager::SetValue( "CONVERT_ORDINARY", pTextureManipulator, "ConversionType", false );
						bResult = bResult && CManipulatorManager::SetValue( "TF_DXT1", pTextureManipulator, "Format", false );
					}
					else
					{
						bResult = bResult && CManipulatorManager::SetValue( "CONVERT_TRANSPARENT", pTextureManipulator, "ConversionType", false );
						bResult = bResult && CManipulatorManager::SetValue( "TF_DXT3", pTextureManipulator, "Format", false );
					}
				}
			}
		}
		// устанавливаем текстуру и ее параметры
		if ( bResult )
		{
			if ( CPtr<IManipulator> pMaterialManipulator = pResourceManager->CreateObjectManipulator( MATERIAL_TYPE_NAME, szMaterialName ) )
			{
				bResult = bResult && CManipulatorManager::SetValue( szTextureName, pMaterialManipulator, "Texture", true );
				bResult = bResult && CManipulatorManager::SetValue( szTextureType, pMaterialManipulator, "AlphaMode", false );
			}
		}
	}
	// добавляем геометрию
	if ( pFolderCallback->IsUniqueName( GEOMETRY_TYPE_NAME, szGeometryName ) )
	{
		bResult = bResult && pFolderCallback->InsertObject( GEOMETRY_TYPE_NAME, szGeometryName );
		//добавляем аигеометрию
		if ( pFolderCallback->IsUniqueName( AIGEOMETRY_TYPE_NAME, szAIGeometryName ) )
		{
			bResult = bResult && pFolderCallback->InsertObject( AIGEOMETRY_TYPE_NAME, szAIGeometryName );
			//Устанавливаем имя модели для аигеометрии
			if ( bResult )
			{
				if ( CPtr<IManipulator> pAIGeometryManipulator = pResourceManager->CreateObjectManipulator( AIGEOMETRY_TYPE_NAME, szAIGeometryName ) )
				{
					bResult = bResult && CManipulatorManager::SetValue( szMBSeasonedFullFileName, pAIGeometryManipulator, "SrcName", false );
					bResult = bResult && CManipulatorManager::SetValue( szAIRootMesh, pAIGeometryManipulator, "RootMesh", false );
					// check for single-skin mode (szRootJoint != szRootMesh) and lbodypart model (szRootJoint == szRootMesh)
					if ( szRootJoint == szRootMesh ) 
					{
						bResult = bResult && CManipulatorManager::SetValue( szAIRootMesh, pAIGeometryManipulator, "RootJoint", false );
					}
					else
					{
						bResult = bResult && CManipulatorManager::SetValue( szRootJoint, pAIGeometryManipulator, "RootJoint", false );
					}
				}
			}
		}
		// устанавливаем имя файла модели для геометрии и аигеометрию
		if ( bResult )
		{
			if ( CPtr<IManipulator> pGeometryManipulator = pResourceManager->CreateObjectManipulator( GEOMETRY_TYPE_NAME, szGeometryName ) )
			{
				bResult = bResult && CManipulatorManager::SetValue( szMBSeasonedFullFileName, pGeometryManipulator, "SrcName", false );
				bResult = bResult && CManipulatorManager::SetValue( szRootMesh, pGeometryManipulator, "RootMesh", false );
				bResult = bResult && CManipulatorManager::SetValue( szRootJoint, pGeometryManipulator, "RootJoint", false );
				bResult = bResult && CManipulatorManager::SetValue( szAIGeometryName, pGeometryManipulator, "AIGeometry", true );
			}
		}
	}
	// добавляем скелет
	if ( pFolderCallback->IsUniqueName( SKELETON_TYPE_NAME, szSkeletonName ) )
	{
		bResult = bResult && pFolderCallback->InsertObject( SKELETON_TYPE_NAME, szSkeletonName );
		// устанавливаем имя файла модели для скелета
		if ( bResult )
		{
			if ( CPtr<IManipulator> pSkeletonManipulator = pResourceManager->CreateObjectManipulator( SKELETON_TYPE_NAME, szSkeletonName ) )
			{
				bResult = bResult && CManipulatorManager::SetValue( szMBSeasonedFullFileName, pSkeletonManipulator, "SrcName", false );
				bResult = bResult && CManipulatorManager::SetValue( szRootJoint, pSkeletonManipulator, "RootJoint", false );
			}
		}
	}
	// устанавливаем материал, геометрию и скелет
	if ( bResult )
	{
		if ( CPtr<IManipulator> pModelManipulator = pResourceManager->CreateObjectManipulator( MODEL_TYPE_NAME, szModelName ) )
		{
			int nMaterialCount = 0;
			bResult = bResult && CManipulatorManager::GetValue( &nMaterialCount, pModelManipulator, "Materials" );
			if ( bResult && ( nMaterialCount == 0 ) )
			{
				bResult = bResult && pModelManipulator->InsertNode( "Materials" );
			}
			bResult = bResult && pModelManipulator->SetValue( "Materials.[0]", szMaterialName );
			bResult = bResult && pModelManipulator->SetValue( "Geometry", szGeometryName );
			bResult = bResult && pModelManipulator->SetValue( "Skeleton", szSkeletonName );
		}
	}
	// устанавливаем модель
	bResult = bResult && pVisObjManipulator->InsertNode( "Models" );
	if ( bResult )
	{
		int nModelCount = 0;
		bResult = bResult && CManipulatorManager::GetValue( &nModelCount, pVisObjManipulator, "Models" );
		if ( bResult && ( nModelCount > 0 ) )
		{
			const std::string szVisObjeEntryName = fmt::format( "Models.[{}].", ( nModelCount - 1 ) );
			bResult = bResult && pVisObjManipulator->SetValue( szVisObjeEntryName + "Model", szModelName );
			bResult = bResult && pVisObjManipulator->SetValue( szVisObjeEntryName + "Season", szSeasonName );
		}
	}
	return bResult;
}


//CRAP{ PLAIN_TEXT
bool CVisObjBuilder::IsValidBuildData( IManipulator *pBuildDataManipulator, std::string *pszDescription, IView *pBuildDataView )
{
	NI_ASSERT( pBuildDataManipulator != 0, "CVisObjBuilder::IsValidBuildData() pBuildDataManipulator == 0" );
	NI_ASSERT( pszDescription != 0, "CVisObjBuilder::IsValidBuildData() pszDescription == 0" );
	pszDescription->clear();	
	// Считываем данные
	std::string szMBFullFileName;
	if ( !CManipulatorManager::GetValue( &szMBFullFileName, pBuildDataManipulator, "ModelFileName" ) || szMBFullFileName.empty() )
	{
		( *pszDescription ) = "<ModelFileName> must be filled.";
		return false;
	}
	if ( !NFile::DoesFileExist( ( Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder + szMBFullFileName ) ) )
	{
		( *pszDescription ) = "<ModelFileName> is invalid file name. Can't find file.";
		return false;
	}
	std::string szTGAFullFileName;
	if ( !CManipulatorManager::GetValue( &szTGAFullFileName, pBuildDataManipulator, "TextureFileName" ) || szTGAFullFileName.empty() )
	{
		( *pszDescription ) = "<TextureFileName> must be filled.";
		return false;
	}
	if ( !NFile::DoesFileExist( ( Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder + szTGAFullFileName ) ) )
	{
		( *pszDescription ) = "<TextureFileName> is invalid file name. Can't find file.";
		return false;
	}
	std::string szTextureType;
	if ( !CManipulatorManager::GetValue( &szTextureType, pBuildDataManipulator, "TextureType" ) || szTextureType.empty() )
	{
		( *pszDescription ) = "<TextureType> must be filled.";
		return false;
	}
	std::string szRootMesh;
	if ( !CManipulatorManager::GetValue( &szRootMesh, pBuildDataManipulator, "RootMesh" ) || szRootMesh.empty() )
	{
		( *pszDescription ) = "<RootMesh> must be filled.";
		return false;
	}
	std::string szRootJoint;
	if ( !CManipulatorManager::GetValue( &szRootJoint, pBuildDataManipulator, "RootJoint" ) || szRootJoint.empty() )
	{
		( *pszDescription ) = "<RootJoint> must be filled.";
		return false;
	}
	std::string szAIRootMesh;
	if ( !CManipulatorManager::GetValue( &szAIRootMesh, pBuildDataManipulator, "AIRootMesh" ) || szAIRootMesh.empty() )
	{
		( *pszDescription ) = "<AIRootMesh> must be filled.";
		return false;
	}
	return true;
}
//CRAP} PLAIN_TEXT


bool CVisObjBuilder::InternalInsertObject( std::string *pszObjectTypeName,
																					 std::string *pszUniqueObjectName,
																					 bool bFromMainMenu,
																					 bool *pbCanChangeObjectName,
																					 bool *pbNeedExport,
																					 bool *pbNeedEdit,
																					 IManipulator *pBuildDataManipulator )
{
	NI_ASSERT( pszObjectTypeName != 0, "CVisObjBuilder::InternalInsertObject() pszObjectTypeName == 0" );
	NI_ASSERT( pszUniqueObjectName != 0, "CVisObjBuilder::InternalInsertObject() pszUniqueObjectName == 0" );
	NI_ASSERT( pBuildDataManipulator != 0, "CVisObjBuilder::InternalInsertObject() pBuildDataManipulator == 0" );
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	//
	std::string szDescription;
	if ( !IsValidBuildData( pBuildDataManipulator, &szDescription, 0 ) )
	{
		return false;
	}
	//
	bool bResult = pFolderCallback->InsertObject( *pszObjectTypeName, *pszUniqueObjectName );
	if ( bResult )
	{
		// Добавляем VisObjEntry для каждого сезона
		bResult = bResult && AddVisObjEntry( *pszUniqueObjectName, pBuildDataManipulator, std::string(), std::string(), NDb::SEASON_WINTER );
		bResult = bResult && AddVisObjEntry( *pszUniqueObjectName, pBuildDataManipulator, std::string(), std::string(), NDb::SEASON_SPRING );
		bResult = bResult && AddVisObjEntry( *pszUniqueObjectName, pBuildDataManipulator, std::string(), std::string(), NDb::SEASON_SUMMER );
		bResult = bResult && AddVisObjEntry( *pszUniqueObjectName, pBuildDataManipulator, std::string(), std::string(), NDb::SEASON_AUTUMN );
		bResult = bResult && AddVisObjEntry( *pszUniqueObjectName, pBuildDataManipulator, std::string(), std::string(), NDb::SEASON_AFRICA );
		bResult = bResult && AddVisObjEntry( *pszUniqueObjectName, pBuildDataManipulator, std::string(), std::string(), NDb::SEASON_ASIA );
	}
	return bResult;
}


bool CVisObjBuilder::CreateVisObj( const std::string &rszVisObjFolder )
{
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	//
	SBuildDataParams buildDataParams;
	buildDataParams.nFlags = BDF_CHECK_PROPERTIES;
	buildDataParams.szObjectTypeName = VISOBJ_TYPE_NAME;
	buildDataParams.szObjectNamePrefix = rszVisObjFolder;
	buildDataParams.szObjectNamePostfix = "\\";
	buildDataParams.bNeedExport = false;
	buildDataParams.bNeedEdit = false;
	//
	std::string szBuildDataTypeName = GetBuildDataTypeName();
	std::string szBuildDataName;
	if ( Singleton<IBuilderContainer>()->FillBuildData( &szBuildDataTypeName, &szBuildDataName, &buildDataParams, this ) )
	{
		if ( CPtr<IManipulator> pBuildDataManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szBuildDataTypeName, szBuildDataName ) )
		{
			std::string szDescription;
			if ( !IsValidBuildData( pBuildDataManipulator, &szDescription, 0 ) )
			{
				return false;
			}
			std::string szVisObjFolder;
			buildDataParams.GetObjectName( &szVisObjFolder );
			//
			std::string szMBFileFolder;
			std::string szTGAFileFolder;
			CManipulatorManager::GetValue( &szMBFileFolder, pBuildDataManipulator, "ModelFileName" );
			CManipulatorManager::GetValue( &szTGAFileFolder, pBuildDataManipulator, "TextureFileName" );
			CStringManager::CutFileName( &szMBFileFolder );
			CStringManager::CutFileName( &szTGAFileFolder );
			//
			bool bResult = true;
			//
			//определяем количество mb и tga файлов
			int nMBFileCount = 0;
			int nTGAFileCount = 0;
			while ( NFile::DoesFileExist( fmt::format( "{}{}.mb", ( pUserData->constUserData.szExportSourceFolder + szMBFileFolder ).c_str(), nMBFileCount + 1 ) ) )
			{
				++nMBFileCount;
			}	
			while ( NFile::DoesFileExist( fmt::format( "{}{}.tga", ( pUserData->constUserData.szExportSourceFolder + szTGAFileFolder ).c_str(), nTGAFileCount + 1 ) ) )
			{
				++nTGAFileCount;
			}	
			bResult = ( nMBFileCount > 0 ) && ( nTGAFileCount > 0 ); 
			//whole
			if (  bResult )
			{
				const std::string szVisObjName = szVisObjFolder + "whole.xdb";
				const std::string szMBFullFileName = szMBFileFolder + "1.mb";
				const std::string szTGAFullFileName = szTGAFileFolder + "1.tga";
				if ( pFolderCallback->IsUniqueName( VISOBJ_TYPE_NAME, szVisObjName ) )
				{
					pFolderCallback->InsertObject( VISOBJ_TYPE_NAME, szVisObjName );
				}
				else
				{
					CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( VISOBJ_TYPE_NAME, szVisObjName );
					pManipulator->RemoveNode( "Models" );
				}
				// Добавляем VisObjEntry для каждого сезона
				bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_WINTER );
				bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_SPRING );
				bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_SUMMER );
				bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_AUTUMN );
				bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_AFRICA );
				bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_ASIA );
			}
			//destroyed
			if (  bResult && ( nTGAFileCount > 1 ) )
			{
				const std::string szVisObjName = szVisObjFolder + "destroyed.xdb";
				std::string szMBFullFileName = szMBFileFolder + "2.mb";
				if ( !NFile::DoesFileExist( ( pUserData->constUserData.szExportSourceFolder + szMBFullFileName ) ) )
				{
					szMBFullFileName = szMBFileFolder + "1.mb";
				}
				const std::string szTGAFullFileName = szTGAFileFolder + "2.tga";
				//
				if ( pFolderCallback->IsUniqueName( VISOBJ_TYPE_NAME, szVisObjName ) )
				{
					pFolderCallback->InsertObject( VISOBJ_TYPE_NAME, szVisObjName );
				}
				else
				{
					CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( VISOBJ_TYPE_NAME, szVisObjName );
					pManipulator->RemoveNode( "Models" );
				}
				// Добавляем VisObjEntry для каждого сезона
				bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_WINTER );
				bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_SPRING );
				bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_SUMMER );
				bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_AUTUMN );
				bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_AFRICA );
				bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_ASIA );
			}
			//damaged
			if (  bResult )
			{
				for ( int nTGAFileIndex = 3; nTGAFileIndex <= nTGAFileCount; ++nTGAFileIndex )
				{
					const std::string szVisObjName = szVisObjFolder + fmt::format( "damaged{}.xdb", nTGAFileIndex - 2 );
					const std::string szMBFullFileName = szMBFileFolder + "1.mb";
					const std::string szTGAFullFileName = szTGAFileFolder + fmt::format( "{}.tga", nTGAFileIndex );
					//
					if ( pFolderCallback->IsUniqueName( VISOBJ_TYPE_NAME, szVisObjName ) )
					{
						pFolderCallback->InsertObject( VISOBJ_TYPE_NAME, szVisObjName );
					}
					else
					{
						CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( VISOBJ_TYPE_NAME, szVisObjName );
						pManipulator->RemoveNode( "Models" );
					}
					// Добавляем VisObjEntry для каждого сезона
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_WINTER );
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_SPRING );
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_SUMMER );
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_AUTUMN );
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_AFRICA );
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_ASIA );
				}
			}
			//anim
			if (  bResult )
			{
				const std::string szMBFullFileName = szMBFileFolder + "2.mb";
				const std::string szTGAFullFileName = szTGAFileFolder + "1.tga";
				if ( NFile::DoesFileExist( ( pUserData->constUserData.szExportSourceFolder + szMBFullFileName ) ) )
				{
					const std::string szVisObjName = szVisObjFolder + "anim.xdb";
					if ( pFolderCallback->IsUniqueName( VISOBJ_TYPE_NAME, szVisObjName ) )
					{
						pFolderCallback->InsertObject( VISOBJ_TYPE_NAME, szVisObjName );
					}
					else
					{
						CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( VISOBJ_TYPE_NAME, szVisObjName );
						pManipulator->RemoveNode( "Models" );
					}
					// Добавляем VisObjEntry для каждого сезона
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_WINTER );
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_SPRING );
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_SUMMER );
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_AUTUMN );
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_AFRICA );
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_ASIA );
				}
			}
			//transp
			if (  bResult )
			{
				const std::string szMBFullFileName = szMBFileFolder + "3.mb";
				const std::string szTGAFullFileName = szTGAFileFolder + "1.tga";
				if ( NFile::DoesFileExist( ( pUserData->constUserData.szExportSourceFolder + szMBFullFileName ) ) )
				{
					const std::string szVisObjName = szVisObjFolder + "transp.xdb";
					if ( pFolderCallback->IsUniqueName( VISOBJ_TYPE_NAME, szVisObjName ) )
					{
						pFolderCallback->InsertObject( VISOBJ_TYPE_NAME, szVisObjName );
					}
					else
					{
						CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( VISOBJ_TYPE_NAME, szVisObjName );
						pManipulator->RemoveNode( "Models" );
					}
					// Добавляем VisObjEntry для каждого сезона
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_WINTER );
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_SPRING );
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_SUMMER );
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_AUTUMN );
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_AFRICA );
					bResult = bResult && AddVisObjEntry( szVisObjName, pBuildDataManipulator, szMBFullFileName, szTGAFullFileName, NDb::SEASON_ASIA );
				}
			}
			return bResult;
		}
	}
	return false;
}


bool CVisObjBuilder::HandleCommand( unsigned nCommandID, uint32_t dwData )
{
	switch( nCommandID )
	{
		case ID_TOOLS_CREATE_VIS_OBJ:
		{	
			SSelectionSet selectionSet;
			bool bResult = Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_SELECTION, reinterpret_cast<uint32_t>( &selectionSet ) );
			const std::string szObjectTypeName = selectionSet.szObjectTypeName;
			bResult = bResult && ( szObjectTypeName == "VisObj" );
			bResult = bResult && ( !selectionSet.objectNameList.empty() );
			if ( bResult )
			{
				const std::string szObjectName = selectionSet.objectNameList.front().ToString();
				bResult = bResult && ( szObjectName )[szObjectName.size() - 1] == PATH_SEPARATOR_CHAR;
				bResult = bResult && CreateVisObj( szObjectName );
			}
			return bResult;
		}
		default:
			return false;
	}
	return false;
}


bool CVisObjBuilder::UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CAnimationBuilder::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CAnimationBuilder::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_TOOLS_CREATE_VIS_OBJ:
		{
			SSelectionSet selectionSet;
			bool bResult = Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_SELECTION, reinterpret_cast<uint32_t>( &selectionSet ) );
			const std::string szObjectTypeName = selectionSet.szObjectTypeName;
			bResult = bResult && ( szObjectTypeName == "VisObj" );
			bResult = bResult && ( !selectionSet.objectNameList.empty() );
			if ( bResult )
			{
				const std::string szObjectName = selectionSet.objectNameList.front().ToString();
				bResult = bResult && ( szObjectName )[szObjectName.size() - 1] == PATH_SEPARATOR_CHAR;
				( *pbEnable ) = bResult;
				( *pbCheck ) = false;
			}
			return true;
		}
		default:
			return false;
	}
	return false;
}


bool CVisObjBuilder::RemoveTexture( const std::string &rszObjectTypeName, const std::string &rszObjectName )
{
	Singleton<IFolderCallback>()->RemoveObject( rszObjectTypeName, rszObjectName, true );
	return true;
}


bool CVisObjBuilder::RemoveAIGeometry( const std::string &rszObjectTypeName, const std::string &rszObjectName )
{
	Singleton<IFolderCallback>()->RemoveObject( rszObjectTypeName, rszObjectName, true );
	return true;
}


bool CVisObjBuilder::RemoveAnimation( const std::string &rszObjectTypeName, const std::string &rszObjectName )
{
	Singleton<IFolderCallback>()->RemoveObject( rszObjectTypeName, rszObjectName, true );
	return true;
}


bool CVisObjBuilder::RemoveMaterial( const std::string &rszObjectTypeName, const std::string &rszObjectName )
{
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( rszObjectTypeName, rszObjectName );
	if ( pManipulator == 0 )
	{
		return false;
	}
	//
	std::string szTextureTypeName;
	std::string szTextureName;
	{
		const std::string szRefValueName = "Texture";
		CManipulatorManager::GetParamsFromReference( szRefValueName, pManipulator, &szTextureTypeName, &szTextureName, 0 ); 
	}
	if ( pFolderCallback->RemoveObject( rszObjectTypeName, rszObjectName, true ) )
	{
		if ( !szTextureName.empty() )
		{
			RemoveTexture( szTextureTypeName, szTextureName );
		}
	}
	return true;
}


bool CVisObjBuilder::RemoveGeometry( const std::string &rszObjectTypeName, const std::string &rszObjectName )
{
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( rszObjectTypeName, rszObjectName );
	if ( pManipulator == 0 )
	{
		return false;
	}
	//
	std::string szAIGeometryTypeName;
	std::string szAIGeometryName;
	{
		const std::string szRefValueName = "AIGeometry";
		CManipulatorManager::GetParamsFromReference( szRefValueName, pManipulator, &szAIGeometryTypeName, &szAIGeometryName, 0 ); 
	}
	if ( pFolderCallback->RemoveObject( rszObjectTypeName, rszObjectName, true ) )
	{
		if ( !szAIGeometryName.empty() )
		{
			RemoveAIGeometry( szAIGeometryTypeName, szAIGeometryName );
		}
	}
	return true;
}


bool CVisObjBuilder::RemoveSkeleton( const std::string &rszObjectTypeName, const std::string &rszObjectName )
{
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( rszObjectTypeName, rszObjectName );
	if ( pManipulator == 0 )
	{
		return false;
	}
	//
	std::string szAnimationTypeName;
	std::list<std::string> animationNameList;
	{
		int nAnimationCount = 0;
		if ( CManipulatorManager::GetValue( &nAnimationCount, pManipulator, "Animations" ) )
		{
			for ( int nAnimationIndex = 0; nAnimationIndex < nAnimationCount; ++nAnimationIndex )
			{
				const std::string szRefValueName = fmt::format( "Animations.[{}]", nAnimationIndex );
				std::string szAnimationName;
				CManipulatorManager::GetParamsFromReference( szRefValueName, pManipulator, &szAnimationTypeName, &szAnimationName, 0 ); 
				if ( !szAnimationName.empty() )
				{
					animationNameList.push_back( szAnimationName );
				}
			}
		}
	}
	if ( pFolderCallback->RemoveObject( rszObjectTypeName, rszObjectName, true ) )
	{
		for ( std::list<std::string>::const_iterator itAnimationName = animationNameList.begin(); itAnimationName != animationNameList.end(); ++itAnimationName )
		{
			RemoveAnimation( szAnimationTypeName, *itAnimationName );
		}
	}
	return true;
}


bool CVisObjBuilder::RemoveModel( const std::string &rszObjectTypeName, const std::string &rszObjectName )
{
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( rszObjectTypeName, rszObjectName );
	if ( pManipulator == 0 )
	{
		return false;
	}
	//
	std::string szMaterialTypeName;
	std::list<std::string> materialNameList;
	{
		int nMaterialCount = 0;
		if ( CManipulatorManager::GetValue( &nMaterialCount, pManipulator, "Materials" ) )
		{
			for ( int nMaterialIndex = 0; nMaterialIndex < nMaterialCount; ++nMaterialIndex )
			{
				const std::string szRefValueName = fmt::format( "Materials.[{}]", nMaterialIndex );
				std::string szMaterialName;
				CManipulatorManager::GetParamsFromReference( szRefValueName, pManipulator, &szMaterialTypeName, &szMaterialName, 0 ); 
				if ( !szMaterialName.empty() )
				{
					materialNameList.push_back( szMaterialName );
				}
			}
		}
	}
	//
	std::string szGeometryTypeName;
	std::string szGeometryName;
	{
		const std::string szRefValueName = "Geometry";
		CManipulatorManager::GetParamsFromReference( szRefValueName, pManipulator, &szGeometryTypeName, &szGeometryName, 0 ); 
	}
	//
	std::string szSkeletonTypeName;
	std::string szSkeletonName;
	{
		const std::string szRefValueName = "Skeleton";
		CManipulatorManager::GetParamsFromReference( szRefValueName, pManipulator, &szSkeletonTypeName, &szSkeletonName, 0 ); 
	}
	if ( pFolderCallback->RemoveObject( rszObjectTypeName, rszObjectName, true ) )
	{
		for ( std::list<std::string>::const_iterator itMaterialName = materialNameList.begin(); itMaterialName != materialNameList.end(); ++itMaterialName )
		{
			RemoveMaterial( szMaterialTypeName, *itMaterialName );
		}
		if ( !szGeometryName.empty() )
		{
			RemoveGeometry( szGeometryTypeName, szGeometryName );
		}
		if ( !szSkeletonName.empty() )
		{
			RemoveSkeleton( szSkeletonTypeName, szSkeletonName );
		}
	}
	return true;
}


bool CVisObjBuilder::RemoveObject( const std::string &rszObjectTypeName, const std::string &rszObjectName )
{
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( rszObjectTypeName, rszObjectName );
	if ( pManipulator == 0 )
	{
		return false;
	}
	//
	std::string szModelTypeName;
	std::list<std::string> modelNameList;
	{
		int nModelCount = 0;
		if ( CManipulatorManager::GetValue( &nModelCount, pManipulator, "Models" ) )
		{
			for ( int nModelIndex = 0; nModelIndex < nModelCount; ++nModelIndex )
			{
				const std::string szRefValueName = fmt::format( "Models.[{}].Model", nModelIndex );
				std::string szModelName;
				CManipulatorManager::GetParamsFromReference( szRefValueName, pManipulator, &szModelTypeName, &szModelName, 0 ); 
				if ( !szModelName.empty() )
				{
					modelNameList.push_back( szModelName );
				}
			}
		}
	}
	if ( pFolderCallback->RemoveObject( rszObjectTypeName, rszObjectName, false ) )
	{
		for ( std::list<std::string>::const_iterator itModelName = modelNameList.begin(); itModelName != modelNameList.end(); ++itModelName )
		{
			RemoveModel( szModelTypeName, *itModelName );
		}
	}
	return true;
}

// basement storage  


