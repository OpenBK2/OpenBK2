#include "StdAfx.h"

#include "MapEditorLib/BuilderFactory.h"
#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/Interface_UserData.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/Tools_HashSet.h"
#include "LibDB/DB.h"
#include "BuilderContainer.h"
#include "PC_BuildDataDialog.h"
#include "NewObjectDialog.h"
#include "Misc/StrProc.h"

bool CBuilderContainer::CanBuildObject( const string &rszObjectTypeName )
{
	return NBuilderFactory::CanCreateBuilder( rszObjectTypeName );
}


bool CBuilderContainer::CanDefaultBuildObject( const string &rszObjectTypeName )
{
	return NBuilderFactory::CanCreateBuilder( DEFAULT_BUILDER_LABEL_TXT );
}


void CBuilderContainer::Create( const string &rszObjectTypeName )
{
	CBuilderMap::iterator posBuilder = builderMap.find( rszObjectTypeName );

	if( posBuilder == builderMap.end() )
	{
		builderMap[rszObjectTypeName] = NBuilderFactory::CreateBuilder( rszObjectTypeName );
	}
}


void CBuilderContainer::Destroy( const string &rszObjectTypeName )
{
	CBuilderMap::iterator posBuilder = builderMap.find( rszObjectTypeName );
	if ( posBuilder != builderMap.end() )
	{
		builderMap.erase( posBuilder );
	}
}


IBuilder* CBuilderContainer::GetBuilder( const string &rszObjectTypeName )
{
	string szBuilderType = CanBuildObject( rszObjectTypeName ) ? rszObjectTypeName : DEFAULT_BUILDER_LABEL_TXT;
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


bool CBuilderContainer::InsertObject( string *pszObjectTypeName,
																			string *pszUniqueObjectName,
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


bool CBuilderContainer::CopyObject( const string &rszObjectTypeName,
																		const string &rszDestination,
																		const string &rszSource )
{
	if ( IBuilder *pBuilder = GetBuilder( rszObjectTypeName ) )
	{
		return pBuilder->CopyObject( rszObjectTypeName, rszDestination, rszSource );
	}
	return false;
}


bool CBuilderContainer::RenameObject( const string &rszObjectTypeName,
																			const string &rszDestination,
																			const string &rszSource )
{
	if ( IBuilder *pBuilder = GetBuilder( rszObjectTypeName ) )
	{
		return pBuilder->RenameObject( rszObjectTypeName, rszDestination, rszSource );
	}
	return false;
}


bool CBuilderContainer::RemoveObject( const string &rszObjectTypeName,
																			const string &rszObjectName )
{
	if ( IBuilder *pBuilder = GetBuilder( rszObjectTypeName ) )
	{
		return pBuilder->RemoveObject( rszObjectTypeName, rszObjectName );
	}
	return false;
}


void CBuilderContainer::GetDefaultFolder( const string &rszObjectTypeName, string *pszDefaultFolder )
{
	if ( IBuilder *pBuilder = GetBuilder( rszObjectTypeName ) )
	{
		pBuilder->GetDefaultFolder( rszObjectTypeName, pszDefaultFolder );
	}
}


bool CBuilderContainer::FillBuildData( string *pszBuildDataTypeName,
																			 string *pszBuildDataName,
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
		( *pszBuildDataName ) = StrFmt( "Editor\\Builder\\%s.xdb", pszBuildDataTypeName->c_str() );
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
		const string szTemporaryLabel = StrFmt( "%s%c%s", pszBuildDataTypeName->c_str(), TYPE_SEPARATOR_CHAR, pszBuildDataName->c_str() );
		//
		CPCBuildDataDialog buildDataDialog( AfxGetMainWnd() );
		buildDataDialog.SetBuildDataParams( pBuildDataParams );
		buildDataDialog.SetTemporaryLabel( szTemporaryLabel );
		buildDataDialog.SetBuildDataCallback( pBuildDataCallback );
		buildDataDialog.GetView()->SetViewManipulator( pManipulator, objectSet, szTemporaryLabel );
		//
		bool bResult = ( buildDataDialog.DoModal() == IDOK );
		pBuildDataParams->szObjectName = pBuildDataParams->szObjectName;
		buildDataDialog.GetView()->RemoveViewManipulator();
		return bResult;
	}
	return false;
}


bool CBuilderContainer::FillNewObjectName( SBuildDataParams *pBuildDataParams )
{
	if ( pBuildDataParams != 0 )
	{
		CNewObjectDialog wndNewObjectDialog( AfxGetMainWnd() );
		vector<string> objectTypeNameList;
		NStr::SplitString( pBuildDataParams->szObjectTypeName, &objectTypeNameList, TYPE_SEPARATOR_CHAR );
		for ( vector<string>::iterator itObjectTypeName = objectTypeNameList.begin(); itObjectTypeName != objectTypeNameList.end(); ++itObjectTypeName )
		{
			NStr::TrimBoth( *itObjectTypeName );
		}
		wndNewObjectDialog.SetBuildDataParams( objectTypeNameList, 0, pBuildDataParams );
		return ( wndNewObjectDialog.DoModal() == IDOK );
	}
	return false;
}


// basement storage  


