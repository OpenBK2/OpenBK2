#include "stdafx.h"
#include <fmt/format.h>

#include "BaseUIBuilder.h"
#include "libdb/ResourceManager.h"
#include "MapEditorLib/StringManager.h"
#include "MapEditorLib/ManipulatorManager.h"

bool CBaseUIBuilder::CopyObject( const std::string &rszObjectTypeName, const std::string &rszDestination, const std::string &rszSource )
{
	// call default implementation of copying (copy object itself)
	if ( !CBuilderBase::CopyObject( rszObjectTypeName, rszDestination, rszSource ) )
	{
		return false;
	}

	IResourceManager *pResourceManager = Singleton<IResourceManager>();

	bool bResult = true;

	CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( rszObjectTypeName, rszDestination );
	bResult = bResult && (pManipulator != 0);

	int nChildren = 0;
	bResult = bResult && CManipulatorManager::GetValue( &nChildren, pManipulator, "Children" );

	// duplicate instances of children
	for ( int i = 0; i < nChildren && bResult; ++i )
	{
		std::string szSrcChildRefName;
		bResult = bResult && CManipulatorManager::GetValue( &szSrcChildRefName, pManipulator, fmt::format( "Children.[{}]", i) );
		// avoid empty refs
		if ( bResult && !szSrcChildRefName.empty() )
		{
			std::string szChildTypeName, szSrcChildName;
			CStringManager::GetTypeAndNameFromRefValue( &szChildTypeName,	&szSrcChildName, szSrcChildRefName, TYPE_SEPARATOR_CHAR, std::string() );

			const int npos = szSrcChildName.rfind( PATH_SEPARATOR_CHAR );
			const std::string szChildShortName = ( npos == std::string::npos ) ? szSrcChildName : szSrcChildName.substr( npos+1 );

			// copy child
			const std::string szDstChildName = rszDestination + PATH_SEPARATOR_CHAR + szChildShortName;
			bResult = bResult && Singleton<IBuilderContainer>()->CopyObject( szChildTypeName, szDstChildName, szSrcChildName );
			if ( bResult )
			{
				std::string szChildRefName;
				CStringManager::GetRefValueFromTypeAndName( &szChildRefName, szChildTypeName, szDstChildName, TYPE_SEPARATOR_CHAR );
				bResult = bResult && pManipulator->SetValue( fmt::format( "Children.[{}]", i), szChildRefName );
			}
		}
	}
	NI_ASSERT( bResult, "CBaseUIBuilder::CopyObject() bResult == false" );
	return bResult;
}


bool CBaseUIBuilder::RemoveObject( const std::string &rszObjectTypeName, const std::string &rszObjectName )
{
	return false;
	//
	IResourceManager *pResourceManager = Singleton<IResourceManager>();

	bool bResult = true;

	CPtr<IManipulator> pManipulator = pResourceManager->CreateObjectManipulator( rszObjectTypeName, rszObjectName );
	bResult = bResult && (pManipulator != 0);

	int nChildren = 0;
	bResult = bResult && CManipulatorManager::GetValue( &nChildren, pManipulator, "Children" );

	std::list<std::string> children;

	// collect references to instances of children
	for ( int i = 0; i < nChildren && bResult; ++i )
	{
		std::string szChildRefName;
		bResult = bResult && CManipulatorManager::GetValue( &szChildRefName, pManipulator, fmt::format( "Children.[{}]", i) );
		children.push_back( szChildRefName );
	}

	// first, remove object itself
	bResult = bResult && CBuilderBase::RemoveObject( rszObjectTypeName, rszObjectName );

	// second, remove instances of children
	for ( std::list<std::string>::iterator it = children.begin(); it != children.end() && bResult; ++it )
	{
		// avoid empty refs
		if ( !it->empty() )
		{
			std::string szChildTypeName, szChildName;
			CStringManager::GetTypeAndNameFromRefValue( &szChildTypeName,	&szChildName, *it, TYPE_SEPARATOR_CHAR, std::string() );
			bResult = bResult && Singleton<IBuilderContainer>()->RemoveObject( szChildTypeName, szChildName );
		}
	}
	NI_ASSERT( bResult, "Cannot remove UI-object with all its children" );
	return bResult;
}


// basement storage  


