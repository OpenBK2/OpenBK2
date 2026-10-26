#include "stdafx.h"

#include "BuildDataView.h"
#include "BuilderContainer.h"
#include "libdb/Db.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/BuilderFactory.h"
#include "MapEditorLib/Interface_MainFrame.h"
#include "MapEditorLib/Interface_UserData.h"
#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "Misc/StrProc.h"
#include "NewObjectView.h"

#include <fmt/format.h>

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

	if ( posBuilder == builderMap.end() )
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

IBuilder *CBuilderContainer::GetBuilder( const std::string &rszObjectTypeName )
{
	std::string szBuilderType = CanBuildObject( rszObjectTypeName ) ? rszObjectTypeName : DEFAULT_BUILDER_LABEL_TXT;
	CBuilderMap::iterator posBuilder = builderMap.find( szBuilderType );
	if ( posBuilder == builderMap.end() )
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
		return pBuilder->InsertObject(
				pszObjectTypeName, pszUniqueObjectName, bFromMainMenu, pbCanChangeObjectName, pbNeedExport, pbNeedEdit );
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

bool CBuilderContainer::RemoveObject( const std::string &rszObjectTypeName, const std::string &rszObjectName )
{
	// A stale browser entry has no fields for a specialised builder to inspect.
	// Still remove its index/tree entry through the ordinary folder controller.
	CPtr<IManipulator> resource =
			Singleton<IResourceManager>()->CreateObjectManipulator( rszObjectTypeName, rszObjectName );
	if ( !resource )
	{
		return Singleton<IFolderCallback>()->RemoveObject( rszObjectTypeName, rszObjectName, false );
	}
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
	SUserData::CBuildDataTypeNameMap &rBuildDataTypeNameMap =
			Singleton<IUserDataContainer>()->Get()->buildDataTypeNameMap;
	SUserData::CBuildDataTypeNameMap::const_iterator posBuildDataTypeName =
			rBuildDataTypeNameMap.find( *pszBuildDataTypeName );
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
	if ( CPtr<IManipulator> pManipulator =
	         Singleton<IResourceManager>()->CreateObjectManipulator( *pszBuildDataTypeName, *pszBuildDataName ) )
	{
		SObjectSet objectSet;
		// Controllers must address the builder's type, not its XDB filename.
		objectSet.szObjectTypeName = ( *pszBuildDataTypeName );
		InsertHashSetElement( &( objectSet.objectNameSet ), CDBID( *pszBuildDataName ) );
		//
		const std::string szTemporaryLabel =
				fmt::format( "{}{:c}{}", pszBuildDataTypeName->c_str(), TYPE_SEPARATOR_CHAR, pszBuildDataName->c_str() );
		//
		// Which toolkit draws it is NBuildData's business, over the main window.
		return NBuildData::Run( Singleton<IMainFrameContainer>()->GetMainWindow(),
		                        pManipulator,
		                        objectSet,
		                        szTemporaryLabel,
		                        pBuildDataParams,
		                        pBuildDataCallback );
	}
	return false;
}

bool CBuilderContainer::FillNewObjectName( SBuildDataParams *pBuildDataParams )
{
	if ( pBuildDataParams != 0 )
	{
		std::vector<std::string> objectTypeNameList;
		NStr::SplitString( pBuildDataParams->szObjectTypeName, &objectTypeNameList, TYPE_SEPARATOR_CHAR );
		for ( std::vector<std::string>::iterator itObjectTypeName = objectTypeNameList.begin();
		      itObjectTypeName != objectTypeNameList.end();
		      ++itObjectTypeName )
		{
			NStr::TrimBoth( *itObjectTypeName );
		}
		// Which toolkit draws it is NNewObject's business, over the main window.
		return NNewObject::Run(
				Singleton<IMainFrameContainer>()->GetMainWindow(), objectTypeNameList, 0, pBuildDataParams );
	}
	return false;
}

// basement storage
