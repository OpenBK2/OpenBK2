#pragma once

#include "MapEditorLib/Interface_View.h"

typedef std::unordered_map<CDBID, CViewSet> CViewSetMap;
typedef std::unordered_map<string, CViewSetMap> CViewSetMapTypeMap;


class CViewContainer : public IViewContainer
{
	OBJECT_NOCOPY_METHODS( CViewContainer );

	CViewSetMapTypeMap viewSetMapTypeMap;

	void Add( IView *pView, const string &rszObjectTypeName, const CDBID &rObjectDBID );
	void Remove( IView *pView, const string &rszObjectTypeName, const CDBID &rObjectDBID );
	bool GetViewSet( CViewSet *pViewSet, const string &rszObjectTypeName, const CDBID &rObjectDBID ) const;

public:
	void Add( IView *pView, const SObjectSet &rObjectSet );
	void Remove( IView *pView, const SObjectSet &rObjectSet );
	bool GetViewSet( CViewSet *pViewSet, const SObjectSet &rObjectSet, IView *pViewToExlude ) const;
};



