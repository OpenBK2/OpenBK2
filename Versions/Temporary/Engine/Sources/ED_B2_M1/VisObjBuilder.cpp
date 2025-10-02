#include "StdAfx.h"
#include "../mapeditorlib/commandhandlerdefines.h"
#include "../mapeditorlib/resourcedefines.h"
#include "../misc/2darray.h"
#include "CommandHandlerDefines.h"
#include "ResourceDefines.h"

#include "VisObjBuilder.h"
#include "SeasonMnemonics.h"
#include "../MapEditorLib/Tools_HashSet.h"
#include "../MapEditorLib/BuilderFactory.h"
#include "../MapEditorLib/Interface_UserData.h"
#include "../MapEditorLib/StringManager.h"
#include "../MapEditorLib/ManipulatorManager.h"
#include "../libdb/ResourceManager.h"
#include "../System/FileUtils.h"

#include <zconf.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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
const string CVisObjBuilder::BUILD_DATA_TYPE_NAME					= "VisObjBuilder";
const string CVisObjBuilder::RESOURCE_PREFIX[RT_COUNT] =
{
	CVisObjBuilder::MODEL_TYPE_NAME,
	CVisObjBuilder::MATERIAL_TYPE_NAME,
	CVisObjBuilder::TEXTURE_TYPE_NAME,
	CVisObjBuilder::GEOMETRY_TYPE_NAME,
	CVisObjBuilder::AIGEOMETRY_TYPE_NAME,
	CVisObjBuilder::SKELETON_TYPE_NAME,
//	StrFmt( "%s", CVisObjBuilder::MODEL_TYPE_NAME ),
//	StrFmt( "%s\\%s", CVisObjBuilder::MODEL_TYPE_NAME, CVisObjBuilder::MATERIAL_TYPE_NAME ),
//	StrFmt( "%s\\%s\\%s", CVisObjBuilder::MODEL_TYPE_NAME, CVisObjBuilder::MATERIAL_TYPE_NAME, CVisObjBuilder::TEXTURE_TYPE_NAME ),
//	StrFmt( "%s\\%s", CVisObjBuilder::MODEL_TYPE_NAME, CVisObjBuilder::GEOMETRY_TYPE_NAME ),
//	StrFmt( "%s\\%s\\%s", CVisObjBuilder::MODEL_TYPE_NAME, CVisObjBuilder::GEOMETRY_TYPE_NAME, CVisObjBuilder::AIGEOMETRY_TYPE_NAME ),
//	StrFmt( "%s\\%s", CVisObjBuilder::MODEL_TYPE_NAME, CVisObjBuilder::SKELETON_TYPE_NAME ),
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


void CVisObjBuilder::GetSeasonedFolderName( string *pszFileName, NDb::ESeason eSeason )
{
	if ( pszFileName )
	{
		string szFilePath;
		string szFileName;
		string szFileExtention;
		CStringManager::SplitFileName( &szFilePath, &szFileName, &szFileExtention, *pszFileName );
		const string szSeasonPostfix = typeSeasonFolderPostfixMnemonics.GetMnemonic( eSeason ) + string( "\\" );
		( *pszFileName ) = szFilePath + szSeasonPostfix + szFileName + szFileExtention;
	}
}


void CVisObjBuilder::GetSeasonedFileName( string *pszFileName, NDb::ESeason eSeason )
{
	if ( pszFileName )
	{
		string szFilePath;
		string szFileName;
		string szFileExtention;
		CStringManager::SplitFileName( &szFilePath, &szFileName, &szFileExtention, *pszFileName );
		const string szSeasonPostfix = typeSeasonFilePostfixMnemonics.GetMnemonic( eSeason );
		( *pszFileName ) = szFilePath + szFileName + szSeasonPostfix + szFileExtention;
	}
}


void CVisObjBuilder::GetResourceFileName( string *pszResourceFileName, EResourceType eResourceType, const string &rszVisObjFileName )
{
	if ( pszResourceFileName )
	{
		string szVisObjFilePath;
		CStringManager::SplitFileName( &szVisObjFilePath, 0, 0, rszVisObjFileName );
		//
		string szResourceFileName;
		CStringManager::SplitFileName( 0, &szResourceFileName, 0, *pszResourceFileName );
		( *pszResourceFileName ) = szVisObjFilePath + szResourceFileName + "_" + RESOURCE_PREFIX[eResourceType] + ".xdb";
//		( *pszResourceFileName ) = szVisObjFilePath + RESOURCE_PREFIX[eResourceType] + string( "\\" ) + szResourceFileName + ".xdb";
	}
}


bool CVisObjBuilder::AddVisObjEntry( const string &rszUniqueObjectName,
																		 IManipulator *pBuildDataManipulator,	
																		 const string &rszMBFullFileName,
																		 const string &rszTGAFullFileName,
																		 NDb::ESeason eSeason )
{
	NI_ASSERT( pBuildDataManipulator != 0, "CTerrainBuilder::AddVisObjEntry() pBuildDataManipulator == 0" );
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	// Считываем данные
	string szMBFullFileName;
	if ( rszMBFullFileName.empty() )
	{
		CManipulatorManager::GetValue( &szMBFullFileName, pBuildDataManipulator, "ModelFileName" );
	}
	else
	{
		szMBFullFileName = rszMBFullFileName;
	}
	string szTGAFullFileName;
	if ( rszTGAFullFileName.empty() )
	{
		CManipulatorManager::GetValue( &szTGAFullFileName, pBuildDataManipulator, "TextureFileName" );
	}
	else
	{
		szTGAFullFileName = rszTGAFullFileName;
	}
	//
	const string szSeasonName = typeSeasonMnemonics.GetMnemonic( eSeason );
	const string szExportSourceFolder = Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder;
	//
	string szMBSeasonedFileName;
	string szTGASeasonedFileName;
	CStringManager::SplitFileName( 0, &szMBSeasonedFileName, 0, szMBFullFileName );
	CStringManager::SplitFileName( 0, &szTGASeasonedFileName, 0, szTGAFullFileName );
	bool bMBSeasonedFileExists = false;
	bool bTGASeasonedFileExists = false;
	string szMBSeasonedFullFileName = szMBFullFileName;
	string szTGASeasonedFullFileName = szTGAFullFileName;
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
	string szTextureType;
	string szRootMesh;
	string szRootJoint;
	string szAIRootMesh;
	string szCommonSkeletonName;
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
	string szGeometryName = szMBSeasonedFileName;
	string szAIGeometryName = szMBSeasonedFileName;
	GetResourceFileName( &szGeometryName, RT_GEOMETRY, rszUniqueObjectName );
	GetResourceFileName( &szAIGeometryName, RT_AIGEOMETRY, rszUniqueObjectName );
	string szSkeletonName;
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
	string szMaterialName = szTGASeasonedFileName;
	string szTextureName = szTGASeasonedFileName;
	GetResourceFileName( &szMaterialName, RT_MATERIAL, rszUniqueObjectName );
	GetResourceFileName( &szTextureName, RT_TEXTURE, rszUniqueObjectName );
	//
	// Если есть новая модель или текстура - создаем новую модель
	string szModelName = szMBSeasonedFileName + "_" + szTGASeasonedFileName;
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
			const string szVisObjeEntryName = StrFmt( "Models.[%d].", ( nModelCount - 1 ) );
			bResult = bResult && pVisObjManipulator->SetValue( szVisObjeEntryName + "Model", szModelName );
			bResult = bResult && pVisObjManipulator->SetValue( szVisObjeEntryName + "Season", szSeasonName );
		}
	}
	return bResult;
}


//CRAP{ PLAIN_TEXT
bool CVisObjBuilder::IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView )
{
	NI_ASSERT( pBuildDataManipulator != 0, "CVisObjBuilder::IsValidBuildData() pBuildDataManipulator == 0" );
	NI_ASSERT( pszDescription != 0, "CVisObjBuilder::IsValidBuildData() pszDescription == 0" );
	pszDescription->clear();	
	// Считываем данные
	string szMBFullFileName;
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
	string szTGAFullFileName;
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
	string szTextureType;
	if ( !CManipulatorManager::GetValue( &szTextureType, pBuildDataManipulator, "TextureType" ) || szTextureType.empty() )
	{
		( *pszDescription ) = "<TextureType> must be filled.";
		return false;
	}
	string szRootMesh;
	if ( !CManipulatorManager::GetValue( &szRootMesh, pBuildDataManipulator, "RootMesh" ) || szRootMesh.empty() )
	{
		( *pszDescription ) = "<RootMesh> must be filled.";
		return false;
	}
	string szRootJoint;
	if ( !CManipulatorManager::GetValue( &szRootJoint, pBuildDataManipulator, "RootJoint" ) || szRootJoint.empty() )
	{
		( *pszDescription ) = "<RootJoint> must be filled.";
		return false;
	}
	string szAIRootMesh;
	if ( !CManipulatorManager::GetValue( &szAIRootMesh, pBuildDataManipulator, "AIRootMesh" ) || szAIRootMesh.empty() )
	{
		( *pszDescription ) = "<AIRootMesh> must be filled.";
		return false;
	}
	return true;
}
//CRAP} PLAIN_TEXT


bool CVisObjBuilder::InternalInsertObject( string *pszObjectTypeName,
																					 string *pszUniqueObjectName,
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
	string szDescription;
	if ( !IsValidBuildData( pBuildDataManipulator, &szDescription, 0 ) )
	{
		return false;
	}
	//
	bool bResult = pFolderCallback->InsertObject( *pszObjectTypeName, *pszUniqueObjectName );
	if ( bResult )
	{
		// Добавляем VisObjEntry для каждого сезона
		bResult = bResult && AddVisObjEntry( *pszUniqueObjectName, pBuildDataManipulator, string(), string(), NDb::SEASON_WINTER );
		bResult = bResult && AddVisObjEntry( *pszUniqueObjectName, pBuildDataManipulator, string(), string(), NDb::SEASON_SPRING );
		bResult = bResult && AddVisObjEntry( *pszUniqueObjectName, pBuildDataManipulator, string(), string(), NDb::SEASON_SUMMER );
		bResult = bResult && AddVisObjEntry( *pszUniqueObjectName, pBuildDataManipulator, string(), string(), NDb::SEASON_AUTUMN );
		bResult = bResult && AddVisObjEntry( *pszUniqueObjectName, pBuildDataManipulator, string(), string(), NDb::SEASON_AFRICA );
		bResult = bResult && AddVisObjEntry( *pszUniqueObjectName, pBuildDataManipulator, string(), string(), NDb::SEASON_ASIA );
	}
	return bResult;
}


bool CVisObjBuilder::CreateVisObj( const string &rszVisObjFolder )
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
	string szBuildDataTypeName = GetBuildDataTypeName();
	string szBuildDataName;
	if ( Singleton<IBuilderContainer>()->FillBuildData( &szBuildDataTypeName, &szBuildDataName, &buildDataParams, this ) )
	{
		if ( CPtr<IManipulator> pBuildDataManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( szBuildDataTypeName, szBuildDataName ) )
		{
			string szDescription;
			if ( !IsValidBuildData( pBuildDataManipulator, &szDescription, 0 ) )
			{
				return false;
			}
			string szVisObjFolder;
			buildDataParams.GetObjectName( &szVisObjFolder );
			//
			string szMBFileFolder;
			string szTGAFileFolder;
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
			while ( NFile::DoesFileExist( StrFmt( "%s%d.mb", ( pUserData->constUserData.szExportSourceFolder + szMBFileFolder ).c_str(), nMBFileCount + 1 ) ) )
			{
				++nMBFileCount;
			}	
			while ( NFile::DoesFileExist( StrFmt( "%s%d.tga", ( pUserData->constUserData.szExportSourceFolder + szTGAFileFolder ).c_str(), nTGAFileCount + 1 ) ) )
			{
				++nTGAFileCount;
			}	
			bResult = ( nMBFileCount > 0 ) && ( nTGAFileCount > 0 ); 
			//whole
			if (  bResult )
			{
				const string szVisObjName = szVisObjFolder + "whole.xdb";
				const string szMBFullFileName = szMBFileFolder + "1.mb";
				const string szTGAFullFileName = szTGAFileFolder + "1.tga";
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
				const string szVisObjName = szVisObjFolder + "destroyed.xdb";
				string szMBFullFileName = szMBFileFolder + "2.mb";
				if ( !NFile::DoesFileExist( ( pUserData->constUserData.szExportSourceFolder + szMBFullFileName ) ) )
				{
					szMBFullFileName = szMBFileFolder + "1.mb";
				}
				const string szTGAFullFileName = szTGAFileFolder + "2.tga";
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
					const string szVisObjName = szVisObjFolder + StrFmt( "damaged%d.xdb", nTGAFileIndex - 2 );
					const string szMBFullFileName = szMBFileFolder + "1.mb";
					const string szTGAFullFileName = szTGAFileFolder + StrFmt( "%d.tga", nTGAFileIndex );
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
				const string szMBFullFileName = szMBFileFolder + "2.mb";
				const string szTGAFullFileName = szTGAFileFolder + "1.tga";
				if ( NFile::DoesFileExist( ( pUserData->constUserData.szExportSourceFolder + szMBFullFileName ) ) )
				{
					const string szVisObjName = szVisObjFolder + "anim.xdb";
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
				const string szMBFullFileName = szMBFileFolder + "3.mb";
				const string szTGAFullFileName = szTGAFileFolder + "1.tga";
				if ( NFile::DoesFileExist( ( pUserData->constUserData.szExportSourceFolder + szMBFullFileName ) ) )
				{
					const string szVisObjName = szVisObjFolder + "transp.xdb";
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


bool CVisObjBuilder::HandleCommand( UINT nCommandID, DWORD dwData )
{
	switch( nCommandID )
	{
		case ID_TOOLS_CREATE_VIS_OBJ:
		{	
			SSelectionSet selectionSet;
			bool bResult = Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_SELECTION, reinterpret_cast<DWORD>( &selectionSet ) );
			const string szObjectTypeName = selectionSet.szObjectTypeName;
			bResult = bResult && ( szObjectTypeName == "VisObj" );
			bResult = bResult && ( !selectionSet.objectNameList.empty() );
			if ( bResult )
			{
				const string szObjectName = selectionSet.objectNameList.front().ToString();
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


bool CVisObjBuilder::UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck )
{
	NI_ASSERT( pbEnable != 0, "CAnimationBuilder::UpdateCommand(), pbEnable == 0" );
	NI_ASSERT( pbCheck != 0, "CAnimationBuilder::UpdateCommand(), pbCheck == 0" );
	//
	switch( nCommandID )
	{
		case ID_TOOLS_CREATE_VIS_OBJ:
		{
			SSelectionSet selectionSet;
			bool bResult = Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_OBJECT_STORAGE, ID_OS_GET_SELECTION, reinterpret_cast<DWORD>( &selectionSet ) );
			const string szObjectTypeName = selectionSet.szObjectTypeName;
			bResult = bResult && ( szObjectTypeName == "VisObj" );
			bResult = bResult && ( !selectionSet.objectNameList.empty() );
			if ( bResult )
			{
				const string szObjectName = selectionSet.objectNameList.front().ToString();
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


bool CVisObjBuilder::RemoveTexture( const string &rszObjectTypeName, const string &rszObjectName )
{
	Singleton<IFolderCallback>()->RemoveObject( rszObjectTypeName, rszObjectName, true );
	return true;
}


bool CVisObjBuilder::RemoveAIGeometry( const string &rszObjectTypeName, const string &rszObjectName )
{
	Singleton<IFolderCallback>()->RemoveObject( rszObjectTypeName, rszObjectName, true );
	return true;
}


bool CVisObjBuilder::RemoveAnimation( const string &rszObjectTypeName, const string &rszObjectName )
{
	Singleton<IFolderCallback>()->RemoveObject( rszObjectTypeName, rszObjectName, true );
	return true;
}


bool CVisObjBuilder::RemoveMaterial( const string &rszObjectTypeName, const string &rszObjectName )
{
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( rszObjectTypeName, rszObjectName );
	if ( pManipulator == 0 )
	{
		return false;
	}
	//
	string szTextureTypeName;
	string szTextureName;
	{
		const string szRefValueName = "Texture";
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


bool CVisObjBuilder::RemoveGeometry( const string &rszObjectTypeName, const string &rszObjectName )
{
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( rszObjectTypeName, rszObjectName );
	if ( pManipulator == 0 )
	{
		return false;
	}
	//
	string szAIGeometryTypeName;
	string szAIGeometryName;
	{
		const string szRefValueName = "AIGeometry";
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


bool CVisObjBuilder::RemoveSkeleton( const string &rszObjectTypeName, const string &rszObjectName )
{
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( rszObjectTypeName, rszObjectName );
	if ( pManipulator == 0 )
	{
		return false;
	}
	//
	string szAnimationTypeName;
	list<string> animationNameList;
	{
		int nAnimationCount = 0;
		if ( CManipulatorManager::GetValue( &nAnimationCount, pManipulator, "Animations" ) )
		{
			for ( int nAnimationIndex = 0; nAnimationIndex < nAnimationCount; ++nAnimationIndex )
			{
				const string szRefValueName = StrFmt( "Animations.[%d]", nAnimationIndex );
				string szAnimationName;
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
		for ( list<string>::const_iterator itAnimationName = animationNameList.begin(); itAnimationName != animationNameList.end(); ++itAnimationName )
		{
			RemoveAnimation( szAnimationTypeName, *itAnimationName );
		}
	}
	return true;
}


bool CVisObjBuilder::RemoveModel( const string &rszObjectTypeName, const string &rszObjectName )
{
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( rszObjectTypeName, rszObjectName );
	if ( pManipulator == 0 )
	{
		return false;
	}
	//
	string szMaterialTypeName;
	list<string> materialNameList;
	{
		int nMaterialCount = 0;
		if ( CManipulatorManager::GetValue( &nMaterialCount, pManipulator, "Materials" ) )
		{
			for ( int nMaterialIndex = 0; nMaterialIndex < nMaterialCount; ++nMaterialIndex )
			{
				const string szRefValueName = StrFmt( "Materials.[%d]", nMaterialIndex );
				string szMaterialName;
				CManipulatorManager::GetParamsFromReference( szRefValueName, pManipulator, &szMaterialTypeName, &szMaterialName, 0 ); 
				if ( !szMaterialName.empty() )
				{
					materialNameList.push_back( szMaterialName );
				}
			}
		}
	}
	//
	string szGeometryTypeName;
	string szGeometryName;
	{
		const string szRefValueName = "Geometry";
		CManipulatorManager::GetParamsFromReference( szRefValueName, pManipulator, &szGeometryTypeName, &szGeometryName, 0 ); 
	}
	//
	string szSkeletonTypeName;
	string szSkeletonName;
	{
		const string szRefValueName = "Skeleton";
		CManipulatorManager::GetParamsFromReference( szRefValueName, pManipulator, &szSkeletonTypeName, &szSkeletonName, 0 ); 
	}
	if ( pFolderCallback->RemoveObject( rszObjectTypeName, rszObjectName, true ) )
	{
		for ( list<string>::const_iterator itMaterialName = materialNameList.begin(); itMaterialName != materialNameList.end(); ++itMaterialName )
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


bool CVisObjBuilder::RemoveObject( const string &rszObjectTypeName, const string &rszObjectName )
{
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( rszObjectTypeName, rszObjectName );
	if ( pManipulator == 0 )
	{
		return false;
	}
	//
	string szModelTypeName;
	list<string> modelNameList;
	{
		int nModelCount = 0;
		if ( CManipulatorManager::GetValue( &nModelCount, pManipulator, "Models" ) )
		{
			for ( int nModelIndex = 0; nModelIndex < nModelCount; ++nModelIndex )
			{
				const string szRefValueName = StrFmt( "Models.[%d].Model", nModelIndex );
				string szModelName;
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
		for ( list<string>::const_iterator itModelName = modelNameList.begin(); itModelName != modelNameList.end(); ++itModelName )
		{
			RemoveModel( szModelTypeName, *itModelName );
		}
	}
	return true;
}

// basement storage  


