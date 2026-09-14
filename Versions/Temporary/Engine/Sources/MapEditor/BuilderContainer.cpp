#include "stdafx.h"
#include <fmt/format.h>

#include "MapEditorLib/BuilderFactory.h"
#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/Interface_UserData.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "libdb/Db.h"
#include "BuilderContainer.h"
#include "BuildDataView.h"
#include "NewObjectView.h"
#include "MapEditorLib/MfcWidget.h"
#include "Misc/StrProc.h"

bool CBuilderContainer::CanBuildObject( const std::string &rszObjectTypeName )
{
	return NBuilderFactory::CanCreateBuilder( rszObjectTypeName );
}


bool CBuilderContainer::CanDefaultBuildObject( const std::string &rszObjectTypeName )
{
	return NBuilderFactory::CanCreateBuilder( DEFAULT_BUILDER_LABEL_TXT );
}


void CBuilderContainer::Create( const std::string &rszObjectTypeName )
{
	CBuilderMap::iterator posBuilder = builderMap.find( rszObjectTypeName );

	if( posBuilder == builderMap.end() )
	{
		builderMap[rszObjectTypeName] = NBuilderFactory::CreateBuilder( rszObjectTypeName );
	}
}


void CBuilderContainer::Destroy( const std::string &rszObjectTypeName )
{
	CBuilderMap::iterator posBuilder = builderMap.find( rszObjectTypeName );
	if ( posBuilder != builderMap.end() )
	{
		builderMap.erase( posBuilder );
	}
}


IBuilder* CBuilderContainer::GetBuilder( const std::string &rszObjectTypeName )
{
	std::string szBuilderType = CanBuildObject( rszObjectTypeName ) ? rszObjectTypeName : DEFAULT_BUILDER_LABEL_TXT;
	CBuilderMap::iterator posBuilder = builderMap.find( szBuilderType );
	if( posBuilder == builderMap.end() )
	{
		builderMap[szBuilderType] = NBuilderFactory::CreateBuilder( szBuilderType );
		posBuilder = builderMap.find( szBuilderType );
	}
	if ( posBuilder != builderMap.end() )
	{
		return posBuilder->second;
	}
	return 0;
}


bool CBuilderContainer::InsertObject( std::string *pszObjectTypeName,
																			std::string *pszUniqueObjectName,
																			bool bFromMainMenu,
																			bool *pbCanChangeObjectName,
																			bool *pbNeedExport,
																			bool *pbNeedEdit )
{
	if ( IBuilder *pBuilder = GetBuilder( *pszObjectTypeName ) )
	{
		return pBuilder->InsertObject( pszObjectTypeName, pszUniqueObjectName, bFromMainMenu, pbCanChangeObjectName, pbNeedExport, pbNeedEdit );
	}
	return false;
}


bool CBuilderContainer::CopyObject( const std::string &rszObjectTypeName,
																		const std::string &rszDestination,
																		const std::string &rszSource )
{
	if ( IBuilder *pBuilder = GetBuilder( rszObjectTypeName ) )
	{
		return pBuilder->CopyObject( rszObjectTypeName, rszDestination, rszSource );
	}
	return false;
}


bool CBuilderContainer::RenameObject( const std::string &rszObjectTypeName,
																			const std::string &rszDestination,
																			const std::string &rszSource )
{
	if ( IBuilder *pBuilder = GetBuilder( rszObjectTypeName ) )
	{
		return pBuilder->RenameObject( rszObjectTypeName, rszDestination, rszSource );
	}
	return false;
}


bool CBuilderContainer::RemoveObject( const std::string &rszObjectTypeName,
																			const std::string &rszObjectName )
{
	if ( IBuilder *pBuilder = GetBuilder( rszObjectTypeName ) )
	{
		return pBuilder->RemoveObject( rszObjectTypeName, rszObjectName );
	}
	return false;
}


void CBuilderContainer::GetDefaultFolder( const std::string &rszObjectTypeName, std::string *pszDefaultFolder )
{
	if ( IBuilder *pBuilder = GetBuilder( rszObjectTypeName ) )
	{
		pBuilder->GetDefaultFolder( rszObjectTypeName, pszDefaultFolder );
	}
}


bool CBuilderContainer::FillBuildData( std::string *pszBuildDataTypeName,
																			 std::string *pszBuildDataName,
																			 SBuildDataParams *pBuildDataParams,					
																			 IBuildDataCallback *pBuildDataCallback )
{
	NI_ASSERT( pszBuildDataTypeName != 0, "CBuilderContainer::FillBuildData() pszBuildDataTypeName == 0" );
	NI_ASSERT( pBuildDataParams != 0, "CBuilderContainer::FillBuildData() pBuildDataParams == 0" );
	NI_ASSERT( pszBuildDataName != 0, "CBuilderContainer::FillBuildData() pszBuildDataName == 0" );
	NI_ASSERT( pBuildDataCallback != 0, "CBuilderContainer::FillBuildData() pBuildDataCallback == 0" );
	//
	SUserData::CBuildDataTypeNameMap &rBuildDataTypeNameMap = Singleton<IUserDataContainer>()->Get()->buildDataTypeNameMap;
	SUserData::CBuildDataTypeNameMap::const_iterator posBuildDataTypeName = rBuildDataTypeNameMap.find( *pszBuildDataName );
	if ( posBuildDataTypeName != rBuildDataTypeNameMap.end() )
	{
		( *pszBuildDataName ) = posBuildDataTypeName->second;
	}
	else
	{
		( *pszBuildDataName ) = fmt::format( "Editor\\Builder\\{}.xdb", pszBuildDataTypeName->c_str() );
		rBuildDataTypeNameMap[( *pszBuildDataTypeName )] = ( *pszBuildDataName );
	}
	//
	if ( Singleton<IFolderCallback>()->IsUniqueName( *pszBuildDataTypeName, *pszBuildDataName ) )
	{
		Singleton<IFolderCallback>()->InsertObject( *pszBuildDataTypeName, *pszBuildDataName );
	}
	if ( CPtr<IManipulator> pManipulator = Singleton<IResourceManager>()->CreateObjectManipulator( *pszBuildDataTypeName, *pszBuildDataName ) )
	{
		SObjectSet objectSet;
		objectSet.szObjectTypeName = ( *pszBuildDataName ); 
		InsertHashSetElement( &( objectSet.objectNameSet ), CDBID( *pszBuildDataName ) );
		//
		const std::string szTemporaryLabel = fmt::format( "{}{:c}{}", pszBuildDataTypeName->c_str(), TYPE_SEPARATOR_CHAR, pszBuildDataName->c_str() );
		//
		// Which toolkit draws it is NBuildData's business.
		CWndWidget mainWindow( AfxGetMainWnd() );
		return NBuildData::Run( &mainWindow, pManipulator, objectSet, szTemporaryLabel, pBuildDataParams, pBuildDataCallback );
	}
	return false;
}


bool CBuilderContainer::FillNewObjectName( SBuildDataParams *pBuildDataParams )
{
	if ( pBuildDataParams != 0 )
	{
		std::vector<std::string> objectTypeNameList;
		NStr::SplitString( pBuildDataParams->szObjectTypeName, &objectTypeNameList, TYPE_SEPARATOR_CHAR );
		for ( std::vector<std::string>::iterator itObjectTypeName = objectTypeNameList.begin(); itObjectTypeName != objectTypeNameList.end(); ++itObjectTypeName )
		{
			NStr::TrimBoth( *itObjectTypeName );
		}
		// Which toolkit draws it is NNewObject's business.
		CWndWidget mainWindow( AfxGetMainWnd() );
		return NNewObject::Run( &mainWindow, objectTypeNameList, 0, pBuildDataParams );
	}
	return false;
}


// basement storage  


