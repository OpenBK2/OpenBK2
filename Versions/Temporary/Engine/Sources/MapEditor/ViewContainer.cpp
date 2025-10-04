#include "stdafx.h"

#include "MapEditorLib/Tools_HashSet.h"
#include "ViewContainer.h"

void CViewContainer::Add( IView *pView, const string &rszObjectTypeName, const CDBID &rObjectDBID )
{
	NI_ASSERT( pView != 0, "CViewContainer::Add, pView == 0" );
	if ( !rszObjectTypeName.empty() )
	{
		InsertHashSetElement( &( viewSetMapTypeMap[rszObjectTypeName][rObjectDBID] ), pView );
	}
}


void CViewContainer::Remove( IView *pView, const string &rszObjectTypeName, const CDBID &rObjectDBID )
{
	NI_ASSERT( pView != 0, "CViewContainer::Remove, pView == 0" );
	if ( !rszObjectTypeName.empty() )
	{
		CViewSetMapTypeMap::iterator posViewSetMapType = viewSetMapTypeMap.find( rszObjectTypeName );
		if ( posViewSetMapType != viewSetMapTypeMap.end() )
		{
			CViewSetMap::iterator posViewSet = posViewSetMapType->second.find( rObjectDBID );
			if ( posViewSet != posViewSetMapType->second.end() )
			{
				posViewSet->second.erase( pView );
			}
		}
	}
}


bool CViewContainer::GetViewSet( CViewSet *pViewSet, const string &rszObjectTypeName, const CDBID &rObjectDBID ) const
{
	if ( !rszObjectTypeName.empty() )
	{
		CViewSetMapTypeMap::const_iterator posViewSetMapType = viewSetMapTypeMap.find( rszObjectTypeName );
		if ( posViewSetMapType != viewSetMapTypeMap.end() )
		{
			CViewSetMap::const_iterator posViewSet = posViewSetMapType->second.find( rObjectDBID );
			if ( posViewSet != posViewSetMapType->second.end() )
			{
				if ( pViewSet )
				{
					pViewSet->insert( posViewSet->second.begin(), posViewSet->second.end() );
				}
				return true;
			}
		}
	}
	return false;
}


void CViewContainer::Add( IView *pView, const SObjectSet &rObjectSet )
{
	if ( !rObjectSet.szObjectTypeName.empty() )
	{
		for ( CObjectNameSet::const_iterator itObjectName = rObjectSet.objectNameSet.begin(); itObjectName != rObjectSet.objectNameSet.end(); ++itObjectName )
		{
			Add( pView, rObjectSet.szObjectTypeName, itObjectName->first );
		}
	}
}


void CViewContainer::Remove( IView *pView, const SObjectSet &rObjectSet )
{
	if ( !rObjectSet.szObjectTypeName.empty() )
	{
		for ( CObjectNameSet::const_iterator itObjectName = rObjectSet.objectNameSet.begin(); itObjectName != rObjectSet.objectNameSet.end(); ++itObjectName )
		{
			Remove( pView, rObjectSet.szObjectTypeName, itObjectName->first );
		}
	}
}


bool CViewContainer::GetViewSet( CViewSet *pViewSet, const SObjectSet &rObjectSet, IView *pViewToExlude ) const
{
	bool bResult = false;
	if ( !rObjectSet.szObjectTypeName.empty() )
	{
		if ( pViewSet )
		{
			pViewSet->clear();
		}
		//
		for ( CObjectNameSet::const_iterator itObjectName = rObjectSet.objectNameSet.begin(); itObjectName != rObjectSet.objectNameSet.end(); ++itObjectName )
		{
			if ( GetViewSet( pViewSet, rObjectSet.szObjectTypeName, itObjectName->first ) )
			{
				bResult = true;
			}
		}
		// удаление ненужного View
		if ( pViewSet )
		{
			CViewSet::iterator posExludeView = pViewSet->find( pViewToExlude );
			if ( posExludeView != pViewSet->end() )
			{
				pViewSet->erase( posExludeView );
			}
			if ( pViewSet->empty() )
			{
				bResult = false;
			}
		}
	}
	return bResult;
}

// basement storage  


