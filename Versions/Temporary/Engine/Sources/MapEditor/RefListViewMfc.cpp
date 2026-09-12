#include "stdafx.h"

#include "RefListView.h"
#include "RefListDialog.h"
#include "RefListWaitDialog.h"

#include "libdb/ResourceManager.h"
#include "MapEditorLib/ManipulatorManager.h"
#include "MapEditorLib/MfcWidget.h"
#include "MapEditorLib/StringManager.h"

#include <cstdlib>
#include <unordered_map>

// The reference list as it has always been: CRefListWaitDialog over
// IDD_REF_LIST_WAIT and CRefListDialog over IDD_REF_LIST. Their call sites,
// moved behind the boundary, the dispatcher in front of both implementations,
// and the database work both of them do -- which is most of what these two
// dialogs are.

namespace
{
	// The same flag every migrated dialog follows, so a session runs either the
	// MFC set or the wx set.
	bool UseWx()
	{
		const char *pszUseWx = std::getenv( "OBK2_WX_DIALOGS" );
		return ( pszUseWx != 0 ) && ( pszUseWx[0] != '0' ) && ( pszUseWx[0] != '\0' );
	}
}


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


	bool RunScanMfc( IWidget *pParent, const std::string &rszTypeName, const std::string &rszName,
									 std::list<std::string> *pReferenceObjects )
	{
		CRefListWaitDialog dialog( ToCWnd( pParent ) );
		dialog.SetData( pReferenceObjects, rszTypeName, rszName, Singleton<IResourceManager>() );
		dialog.DoModal();
		// The modal result says nothing: the button marked Cancel is IDOK. What
		// the caller has always asked is whether the scan finished.
		return dialog.IsComplete();
	}


	bool RunScan( IWidget *pParent, const std::string &rszTypeName, const std::string &rszName,
								std::list<std::string> *pReferenceObjects )
	{
		if ( pReferenceObjects == 0 )
		{
			return false;
		}
#ifdef OBK2_WITH_WX
		if ( UseWx() )
		{
			return RunScanWx( pParent, rszTypeName, rszName, pReferenceObjects );
		}
#endif
		return RunScanMfc( pParent, rszTypeName, rszName, pReferenceObjects );
	}


	void RunMfc( IWidget *pParent, const std::string &rszTypeName, const std::string &rszName,
							 std::list<std::string> *pReferenceObjects )
	{
		CRefListDialog dialog( ToCWnd( pParent ) );
		dialog.SetData( rszTypeName, rszName, pReferenceObjects );
		dialog.DoModal();
	}


	void Run( IWidget *pParent, const std::string &rszTypeName, const std::string &rszName,
						std::list<std::string> *pReferenceObjects )
	{
		if ( pReferenceObjects == 0 )
		{
			return;
		}
#ifdef OBK2_WITH_WX
		if ( UseWx() )
		{
			RunWx( pParent, rszTypeName, rszName, pReferenceObjects );
			return;
		}
#endif
		RunMfc( pParent, rszTypeName, rszName, pReferenceObjects );
	}
}
