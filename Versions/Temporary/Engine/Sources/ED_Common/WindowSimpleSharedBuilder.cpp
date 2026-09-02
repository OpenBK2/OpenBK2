#include "stdafx.h"

#include "WindowSimpleSharedBuilder.h"
#include "MapEditorLib/BuilderFactory.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/CommonExporterMethods.h"
#include "System/FileUtils.h"

//REGISTER_BUILDER_IN_DLL( WindowSimpleShared, CWindowSimpleSharedBuilder )


const char CWindowSimpleSharedBuilder::BACKGROUND_SIMPLE_TEXTURE_TYPE_NAME[] = "BackgroundSimpleTexture";
const char CWindowSimpleSharedBuilder::TEXTURE_TYPE_NAME[] = "Texture";
const std::string CWindowSimpleSharedBuilder::BUILD_DATA_TYPE_NAME = "WindowSimpleSharedBuilder";


bool CWindowSimpleSharedBuilder::IsValidBuildData( IManipulator *pBuildDataManipulator, std::string *pszDescription, IView *pBuildDataView )
{
	NI_ASSERT( pBuildDataManipulator != 0, "CWindowSimpleSharedBuilder::IsValidBuildData() pBuildDataManipulator == 0" );
	NI_ASSERT( pszDescription != 0, "CWindowSimpleSharedBuilder::IsValidBuildData() pszDescription == 0" );
	pszDescription->clear();	
	// Считываем данные
	CVariant value;
	pBuildDataManipulator->GetValue( "UseDefBuilder", &value );
	if ( (bool)value )
		return true;
	pBuildDataManipulator->GetValue( "Texture", &value );
	if ( !NFile::DoesFileExist( ( Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder + value.GetStr() ) ) )
	{
		( *pszDescription ) = "<Texture> is invalid file name. Cannot find file.";
		return false;
	}
	return true;
}


bool CWindowSimpleSharedBuilder::InternalInsertObject( std::string *pszObjectTypeName,
																											 std::string *pszUniqueObjectName,
																											 bool bFromMainMenu,
																											 bool *pbCanChangeObjectName,
																											 bool *pbNeedExport,
																											 bool *pbNeedEdit,
																											 IManipulator *pBuildDataManipulator )
{
	NI_ASSERT( pszObjectTypeName != 0, "CWindowSimpleSharedBuilder::InternalInsertObject() pszObjectTypeName == 0" );
	NI_ASSERT( pszUniqueObjectName != 0, "CWindowSimpleSharedBuilder::InternalInsertObject() pszUniqueObjectName == 0" );
	NI_ASSERT( pBuildDataManipulator != 0, "CWindowSimpleSharedBuilder::InternalInsertObject() pBuildDataManipulator == 0" );
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
	//
	std::string szDescription;
	if ( !IsValidBuildData( pBuildDataManipulator, &szDescription, 0 ) )
	{
		return false;
	}

	CVariant value;
	pBuildDataManipulator->GetValue( "UseDefBuilder", &value );
	if ( (bool)value )
	{
		// create default WindowSimpleShared object
		return pFolderCallback->InsertObject( *pszObjectTypeName, *pszUniqueObjectName );
	}

	// Считываем данные
	pBuildDataManipulator->GetValue( "Texture", &value );
	const std::string szTextureFileName = value.GetStr();
	pBuildDataManipulator->GetValue( "Color", &value );
	const int nColor = (int)value;

	CTPoint<int> imageSize(0,0);
	bool bResult = true;

	const std::string szBuilderGenPrefix = *pszUniqueObjectName;

	const std::string szTexObjectName = szBuilderGenPrefix;
	const std::string szBSTObjectName = szBuilderGenPrefix;
	const std::string szWSSObjectName = *pszUniqueObjectName;
	
	if ( !szTextureFileName.empty() )
	{
		const std::string szTextureFilePath = pUserData->constUserData.szExportSourceFolder + szTextureFileName;
		GetTGAImageSize( szTextureFilePath, &imageSize );

		// create Texture object
		if ( !pFolderCallback->InsertObject( TEXTURE_TYPE_NAME, szTexObjectName ) )
			return false;

		CPtr<IManipulator> pTexManipulator = pResourceManager->CreateObjectManipulator( TEXTURE_TYPE_NAME, szTexObjectName );
		NI_ASSERT( pTexManipulator != 0, "CWindowSimpleSharedBuilder::InternalInsertObject() pTexManipulator == 0" );

		// Проставляем основные параметры Texture
		bResult = bResult && pTexManipulator->SetValue( "SrcName", szTextureFileName );
		bResult = bResult && pTexManipulator->SetValue( "Type", std::string("TEXTURE_2D") );
		bResult = bResult && pTexManipulator->SetValue( "ConversionType", std::string("CONVERT_ORDINARY") );
		bResult = bResult && pTexManipulator->SetValue( "AddrType", std::string("CLAMP") );
		bResult = bResult && pTexManipulator->SetValue( "Format", std::string("TF_8888") );
		bResult = bResult && pTexManipulator->SetValue( "NMips", 1 );
	}
	else
	{
		imageSize.Set( 128, 128 ); // default window size (w/o texture)
	}

	// create BackgroundSimpleTexture object
	if ( !pFolderCallback->InsertObject( BACKGROUND_SIMPLE_TEXTURE_TYPE_NAME, szBSTObjectName ) )
		return false;

	CPtr<IManipulator> pBSTManipulator = pResourceManager->CreateObjectManipulator( BACKGROUND_SIMPLE_TEXTURE_TYPE_NAME, szBSTObjectName );
	NI_ASSERT( pBSTManipulator != 0, "CWindowSimpleSharedBuilder::InternalInsertObject() pBSTManipulator == 0" );

	// Проставляем основные параметры BackgroundSimpleTexture
	if ( !szTextureFileName.empty() )
	{
		bResult = bResult && pBSTManipulator->SetValue( "Texture", szTexObjectName );
	}
	bResult = bResult && pBSTManipulator->SetValue( "Color", nColor );
	bResult = bResult && pBSTManipulator->SetValue( "TextureX", std::string("EPA_LOW_END") );
	bResult = bResult && pBSTManipulator->SetValue( "TextureY", std::string("EPA_LOW_END") );

	// create WindowSimpleShared object
	if ( !pFolderCallback->InsertObject( *pszObjectTypeName, szWSSObjectName ) )
		return false;

	CPtr<IManipulator> pWSSManipulator = pResourceManager->CreateObjectManipulator( *pszObjectTypeName, szWSSObjectName );
	NI_ASSERT( pWSSManipulator != 0, "CWindowSimpleSharedBuilder::InternalInsertObject() pWSSManipulator == 0" );

	// Проставляем основные параметры
	std::string szBSTObjectRefName;
	CStringManager::GetRefValueFromTypeAndName( &szBSTObjectRefName, BACKGROUND_SIMPLE_TEXTURE_TYPE_NAME, szBSTObjectName, TYPE_SEPARATOR_CHAR );
	bResult = bResult && pWSSManipulator->SetValue( "Background", szBSTObjectRefName );
	bResult = bResult && pWSSManipulator->SetValue( "Placement.VerAllign.First", std::string("EPA_LOW_END") );
	bResult = bResult && pWSSManipulator->SetValue( "Placement.VerAllign.Second", true );
	bResult = bResult && pWSSManipulator->SetValue( "Placement.HorAllign.First", std::string("EPA_LOW_END") );
	bResult = bResult && pWSSManipulator->SetValue( "Placement.HorAllign.Second", true );
	bResult = bResult && pWSSManipulator->SetValue( "Placement.Size.First.x", imageSize.x );
	bResult = bResult && pWSSManipulator->SetValue( "Placement.Size.First.y", imageSize.y );
	bResult = bResult && pWSSManipulator->SetValue( "Placement.Size.Second", true );

	return bResult;
}


// basement storage  


