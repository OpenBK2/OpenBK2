#include "stdafx.h"

#include "RefListView.h"

#include "libdb/ResourceManager.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/StringManager.h"

#include <unordered_map>

// The reference list's database work, which is most of what its two dialogs
// (RefListViewWx.cpp) are.

namespace NRefList
{
	std::string FullName( const std::string &rszTypeName, const std::string &rszName )
	{
		std::string szFullName;
		CStringManager::GetRefValueFromTypeAndName( &szFullName, rszTypeName, rszName,
																								TYPE_SEPARATOR_CHAR );
		return szFullName;
	}


	void BuildObjects( std::vector<SReferenceObject> *pObjects,
										 const std::list<std::string> &rReferenceObjects )
	{
		NI_VERIFY( pObjects != 0, "NRefList::BuildObjects(): pObjects == 0", return );
		pObjects->clear();

		// Sorted by full name, through a map keyed on it: the scan answers in
		// whatever order it finds things, and the same object can come back more
		// than once.
		std::unordered_map<std::string, SReferenceObject> objectsByName;
		std::list<std::string> fullNames;
		for ( std::list<std::string>::const_iterator it = rReferenceObjects.begin();
					it != rReferenceObjects.end(); ++it )
		{
			SReferenceObject object;
			CStringManager::GetTypeAndNameFromRefValue( &object.szTypeName, &object.szObjectName,
																									( *it ), TYPE_SEPARATOR_CHAR, "" );
			if ( object.szTypeName.empty() )
			{
				// something wrong with the supplied object's subscript (full name)
				continue;
			}
			object.szDisplayName = object.szTypeName + TYPE_SEPARATOR_CHAR + object.szObjectName;
			objectsByName[FullName( object.szTypeName, object.szObjectName )] = object;
			fullNames.push_back( FullName( object.szTypeName, object.szObjectName ) );
		}
		fullNames.sort();
		for ( std::list<std::string>::const_iterator it = fullNames.begin();
					it != fullNames.end(); ++it )
		{
			pObjects->push_back( objectsByName[*it] );
		}
	}


	CPtr<IManipulator> FindFields( std::list<std::string> *pFields, std::string *pszText,
																 const SReferenceObject &rObject,
																 const std::string &rszTargetTypeName,
																 const std::string &rszTargetName )
	{
		NI_VERIFY( pFields != 0 && pszText != 0, "NRefList::FindFields(): null out", return 0 );
		pFields->clear();
		( *pszText ).clear();

		IResourceManager *pResourceManager = Singleton<IResourceManager>();
		NI_VERIFY( pResourceManager, "Cannot find resource manager", return 0 )
		CPtr<IManipulator> pManipulator =
			pResourceManager->CreateObjectManipulator( rObject.szTypeName, rObject.szObjectName );
		if ( !pManipulator )
		{
			( *pszText ) = "Object has disappeared from the base since RefList was constructed";
			return 0;
		}
		std::string szFieldName;
		std::string szRefTargetTypeName;
		std::string szRefTargetName;
		CPtr<IManipulatorIterator> pFieldIt = pManipulator->Iterate( true, ECT_NO_CACHE );
		while ( !pFieldIt->IsEnd() )
		{
			pFieldIt->GetName( &szFieldName );
			if ( CManipulatorManager::GetParamsFromReference( szFieldName, pManipulator,
																												&szRefTargetTypeName, &szRefTargetName, 0 ) &&
					 szRefTargetTypeName == rszTargetTypeName && szRefTargetName == rszTargetName )
			{
				pFields->push_back( szFieldName );
				( *pszText ) += szFieldName + "\r\n";
			}
			pFieldIt->Next();
		}
		return pManipulator;
	}


	bool ClearFields( std::list<std::string> *pFields, std::string *pszText,
										IManipulator *pManipulator )
	{
		NI_VERIFY( pFields != 0 && pszText != 0, "NRefList::ClearFields(): null out", return false );
		if ( pManipulator == 0 )
		{
			return false;
		}
		const CVariant nullRef;
		bool bEverythingIsOK = true;
		for ( std::list<std::string>::iterator it = pFields->begin();
					it != pFields->end() && bEverythingIsOK; )
		{
			( *pszText ) += ( *it );
			if ( pManipulator->SetValue( ( *it ), nullRef ) )
			{
				( *pszText ) += " - ok\r\n";
				it = pFields->erase( it );
			}
			else
			{
				( *pszText ) += " - cannot set!\r\n";
				bEverythingIsOK = false;
			}
		}
		if ( pFields->empty() )
		{
			( *pszText ) += "Complete.\r\n";
			return true;
		}
		return false;
	}


	bool ClearAll( const std::vector<SReferenceObject> &rObjects,
								 const std::string &rszTargetTypeName, const std::string &rszTargetName )
	{
		IResourceManager *pResourceManager = Singleton<IResourceManager>();
		NI_VERIFY( pResourceManager, "Cannot find resource manager", return false )
		const CVariant nullRef;
		bool bSuccess = true;
		std::string szFieldName;
		std::string szRefTargetTypeName;
		std::string szRefTargetName;
		for ( size_t i = 0; i < rObjects.size(); ++i )
		{
			CPtr<IManipulator> pManipulator =
				pResourceManager->CreateObjectManipulator( rObjects[i].szTypeName,
																									 rObjects[i].szObjectName );
			if ( !pManipulator )
			{
				continue;
			}
			CPtr<IManipulatorIterator> pFieldIt = pManipulator->Iterate( true, ECT_NO_CACHE );
			while ( !pFieldIt->IsEnd() )
			{
				pFieldIt->GetName( &szFieldName );
				if ( CManipulatorManager::GetParamsFromReference( szFieldName, pManipulator,
																													&szRefTargetTypeName, &szRefTargetName, 0 ) &&
						 szRefTargetTypeName == rszTargetTypeName && szRefTargetName == rszTargetName )
				{
					bSuccess = pManipulator->SetValue( szFieldName, nullRef ) && bSuccess;
				}
				pFieldIt->Next();
			}
		}
		return bSuccess;
	}
}
