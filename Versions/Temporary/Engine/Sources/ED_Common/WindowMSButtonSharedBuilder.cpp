#include "StdAfx.h"

#include "WindowMSButtonSharedBuilder.h"
#include "../MapEditorLib/BuilderFactory.h"
#include "../libdb/ResourceManager.h"
#include "../MapEditorLib/CommonExporterMethods.h"
#include "../System/FileUtils.h"

//REGISTER_BUILDER_IN_DLL( WindowMSButtonShared, CWindowMSButtonSharedBuilder )


const char CWindowMSButtonSharedBuilder::BACKGROUND_SIMPLE_TEXTURE_TYPE_NAME[] = "BackgroundSimpleTexture";
const char CWindowMSButtonSharedBuilder::TEXTURE_TYPE_NAME[] = "Texture";
const string CWindowMSButtonSharedBuilder::BUILD_DATA_TYPE_NAME = "WindowMSButtonSharedBuilder";


bool CWindowMSButtonSharedBuilder::IsValidBuildData( IManipulator *pBuildDataManipulator, string *pszDescription, IView *pBuildDataView )
{
	NI_ASSERT( pBuildDataManipulator != 0, "CWindowMSButtonSharedBuilder::IsValidBuildData() pBuildDataManipulator == 0" );
	NI_ASSERT( pszDescription != 0, "CWindowMSButtonSharedBuilder::IsValidBuildData() pszDescription == 0" );
	pszDescription->clear();	
	
	// Считываем данные
	CVariant value;
	pBuildDataManipulator->GetValue( "UseDefBuilder", &value );
	if ( (bool)value )
		return true;

	pBuildDataManipulator->GetValue( "VisualStates", &value );
	const int nVisualStates = value;

	if ( nVisualStates < 1 )
	{
		( *pszDescription ) = "<VisualStates> is empty. At least one should be defined.";
		return false;
	}

	for ( int i = 0; i < nVisualStates; ++i )
	{
		const string elementName = StrFmt( "VisualStates.[%d]", i );
		
		pBuildDataManipulator->GetValue( elementName + ".NormalTexture", &value );
		if ( value.GetType() == CVariant::VT_NULL || (string (value.GetStr()).empty() ) )
		{
			( *pszDescription ) = "<NormalTexture> should be defined.";
			return false;
		}
		if ( !NFile::DoesFileExist( ( Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder + value.GetStr() ) ) )
		{
			( *pszDescription ) = "<NormalTexture> is invalid file name. Cannot find file.";
			return false;
		}
		pBuildDataManipulator->GetValue( elementName + ".PushedTexture", &value );
		if ( !NFile::DoesFileExist( ( Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder + value.GetStr() ) ) )
		{
			( *pszDescription ) = "<PushedTexture> is invalid file name. Cannot find file.";
			return false;
		}
		pBuildDataManipulator->GetValue( elementName + ".DisabledTexture", &value );
		if ( !NFile::DoesFileExist( ( Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder + value.GetStr() ) ) )
		{
			( *pszDescription ) = "<DisabledTexture> is invalid file name. Cannot find file.";
			return false;
		}
	}
	return true;
}


bool CWindowMSButtonSharedBuilder::InternalInsertObject( string *pszObjectTypeName,
																												 string *pszUniqueObjectName,
																												 bool bFromMainMenu,
																												 bool *pbCanChangeObjectName,
																												 bool *pbNeedExport,
																												 bool *pbNeedEdit,
																												 IManipulator *pBuildDataManipulator )
{
	NI_ASSERT( pszObjectTypeName != 0, "CWindowMSButtonSharedBuilder::InternalInsertObject() pszObjectTypeName == 0" );
	NI_ASSERT( pszUniqueObjectName != 0, "CWindowMSButtonSharedBuilder::InternalInsertObject() pszUniqueObjectName == 0" );
	NI_ASSERT( pBuildDataManipulator != 0, "CWindowMSButtonSharedBuilder::InternalInsertObject() pBuildDataManipulator == 0" );
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();
	bool bResult = true;

	string szDescription;
	if ( !IsValidBuildData( pBuildDataManipulator, &szDescription, 0 ) )
	{
		return false;
	}

	CVariant value;
	pBuildDataManipulator->GetValue( "UseDefBuilder", &value );
	if ( (bool)value )
	{
		// create default WindowMSButtonShared object
		return pFolderCallback->InsertObject( *pszObjectTypeName, *pszUniqueObjectName );
	}

	// get texture size (Normal, State 0)
	pBuildDataManipulator->GetValue( "VisualStates.[0].NormalTexture", &value );
	const string szTextureFileName = value.GetStr();
		
	CTPoint<int> imageSize(0,0);
	if ( !szTextureFileName.empty() )
	{
		const string szTextureFilePath = Singleton<IUserDataContainer>()->Get()->constUserData.szExportSourceFolder + szTextureFileName;
		GetTGAImageSize( szTextureFilePath, &imageSize );
	}
	else
	{
		imageSize.Set( 128, 128 ); // default button size (w/o texture)
	}

	// create WindowMSButtonShared object
	if ( !pFolderCallback->InsertObject( *pszObjectTypeName, *pszUniqueObjectName ) )
		return false;

	CPtr<IManipulator> pMSBManipulator = pResourceManager->CreateObjectManipulator( *pszObjectTypeName, *pszUniqueObjectName );
	NI_ASSERT( pMSBManipulator != 0, "CWindowMSButtonSharedBuilder::InternalInsertObject() pMSBManipulator == 0" );

	// Проставляем основные параметры WindowMSButtonShared
	bResult = bResult && pMSBManipulator->SetValue( "Placement.VerAllign.First", string("EPA_LOW_END") );
	bResult = bResult && pMSBManipulator->SetValue( "Placement.VerAllign.Second", true );
	bResult = bResult && pMSBManipulator->SetValue( "Placement.HorAllign.First", string("EPA_LOW_END") );
	bResult = bResult && pMSBManipulator->SetValue( "Placement.HorAllign.Second", true );
	bResult = bResult && pMSBManipulator->SetValue( "Placement.Size.First.x", imageSize.x );
	bResult = bResult && pMSBManipulator->SetValue( "Placement.Size.First.y", imageSize.y );
	bResult = bResult && pMSBManipulator->SetValue( "Placement.Size.Second", true );

	// Считываем данные
	pBuildDataManipulator->GetValue( "VisualStates", &value );
	const int nVisualStates = value;

	for ( int i = 0; i < nVisualStates; ++i )
	{
		if ( !CreateVisualState( *pszUniqueObjectName, pBuildDataManipulator, pMSBManipulator, i ) )
		{
			return false;
		}
	}

	return bResult;
}


bool CWindowMSButtonSharedBuilder::CreateVisualState( const string		 & rszUniqueObjectName, 
																										  IManipulator		 * pBuildDataManipulator, 
																											IManipulator		 * pMSBManipulator,
																											int								 index )
{
	if( !pMSBManipulator->InsertNode( "VisualStates" ) )
		return false;

	if ( !CreateButtonState( rszUniqueObjectName, pBuildDataManipulator, pMSBManipulator, index, "Normal", true ) )
		return false;
	if ( !CreateButtonState( rszUniqueObjectName, pBuildDataManipulator, pMSBManipulator, index, "Pushed", false ) )
		return false;
	if ( !CreateButtonState( rszUniqueObjectName, pBuildDataManipulator, pMSBManipulator, index, "Disabled", false ) )
		return false;
	
	return true;
}


bool CWindowMSButtonSharedBuilder::CreateButtonState( const string		 & rszUniqueObjectName, 
																										  IManipulator		 * pBuildDataManipulator, 
																											IManipulator		 * pMSBManipulator,
																											int								 index,
																											const char       * szSuffixName, 
																											bool							 bNormalState	)
{
	bool bResult = true;
	
	IResourceManager *pResourceManager = Singleton<IResourceManager>();
	IFolderCallback *pFolderCallback = Singleton<IFolderCallback>();

	CVariant value;

	pBuildDataManipulator->GetValue( StrFmt( "VisualStates.[%d].%sTexture", index, szSuffixName ), &value );
	const string szTextureFileName = value.GetStr();
	
	pBuildDataManipulator->GetValue( StrFmt( "VisualStates.[%d].%sColor", index, szSuffixName ), &value );
	const int nColor = value;

	// extract short name
	string szShortName;
	const nSlashPos = rszUniqueObjectName.rfind( PATH_SEPARATOR_CHAR );
	if ( nSlashPos == string::npos )
		szShortName = rszUniqueObjectName;
	else
		szShortName = rszUniqueObjectName.substr( nSlashPos+1 );

	const string szBuilderGenPrefix = rszUniqueObjectName + PATH_SEPARATOR_CHAR +  StrFmt( "%s_%d", szShortName.c_str(), index );
	const string szTexObjectName = szBuilderGenPrefix + PATH_SEPARATOR_CHAR + szSuffixName;
	const string szBSTObjectName = szBuilderGenPrefix + PATH_SEPARATOR_CHAR + szSuffixName;
	
	if ( !szTextureFileName.empty() )
	{
		// create Texture object
		if ( !pFolderCallback->InsertObject( TEXTURE_TYPE_NAME, szTexObjectName ) )
			return false;

		CPtr<IManipulator> pTexManipulator = pResourceManager->CreateObjectManipulator( TEXTURE_TYPE_NAME, szTexObjectName );
		NI_ASSERT( pTexManipulator != 0, "CWindowMSButtonSharedBuilder::InternalInsertObject() pTexManipulator == 0" );

		// Проставляем основные параметры Texture
		bResult = bResult && pTexManipulator->SetValue( "SrcName", szTextureFileName );
		bResult = bResult && pTexManipulator->SetValue( "Type", string("TEXTURE_2D") );
		bResult = bResult && pTexManipulator->SetValue( "ConversionType", string("CONVERT_ORDINARY") );
		bResult = bResult && pTexManipulator->SetValue( "AddrType", string("CLAMP") );
		bResult = bResult && pTexManipulator->SetValue( "Format", string("TF_8888") );
		bResult = bResult && pTexManipulator->SetValue( "NMips", 1 );
	}

	// create BackgroundSimpleTexture object
	if ( !pFolderCallback->InsertObject( BACKGROUND_SIMPLE_TEXTURE_TYPE_NAME, szBSTObjectName ) )
		return false;

	CPtr<IManipulator> pBSTManipulator = pResourceManager->CreateObjectManipulator( BACKGROUND_SIMPLE_TEXTURE_TYPE_NAME, szBSTObjectName );
	NI_ASSERT( pBSTManipulator != 0, "CWindowMSButtonSharedBuilder::InternalInsertObject() pBSTManipulator == 0" );

	// Проставляем основные параметры BackgroundSimpleTexture
	if ( !szTextureFileName.empty() )
	{
		// set reference to own texture
		bResult = bResult && pBSTManipulator->SetValue( "Texture", szTexObjectName );
	}
	else
	{
		// set reference to texture of Normal state
		bResult = bResult && pBSTManipulator->SetValue( "Texture", szNormalStateTexObjectName );
	}
	bResult = bResult && pBSTManipulator->SetValue( "Color", nColor );
	bResult = bResult && pBSTManipulator->SetValue( "TextureX", string("EPA_LOW_END") );
	bResult = bResult && pBSTManipulator->SetValue( "TextureY", string("EPA_LOW_END") );

	// Проставляем параметры WindowMSButtonShared
	const string backgroundElementName = StrFmt( "VisualStates.[%d].%s.Background", index, szSuffixName );
	const string szBSTObjectRefName = string(BACKGROUND_SIMPLE_TEXTURE_TYPE_NAME) + TYPE_SEPARATOR_CHAR + szBSTObjectName;
	pMSBManipulator->SetValue( backgroundElementName, szBSTObjectRefName );

	if ( bNormalState )
	{
		szNormalStateTexObjectName = szTexObjectName;
	}

	return bResult;
}

// basement storage  


