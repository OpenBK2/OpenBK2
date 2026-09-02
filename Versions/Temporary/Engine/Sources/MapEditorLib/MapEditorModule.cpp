#include "stdafx.h"

#include "MapEditorModule.h"

class CEditorModuleRegistrator : public IEditorModuleRegistrator
{
	friend void RegisterMapEditorTypeDelayed( const string &szType, const string &szName, ObjectFactoryNewFunc pfnNewFunc );
	//
	typedef list< pair<string, ObjectFactoryNewFunc> > CMapEditorFactoryNewFuncsList;
	typedef std::unordered_map<string, CMapEditorFactoryNewFuncsList> CMapEditorFactoryNewFuncsMap;
	CMapEditorFactoryNewFuncsMap newFuncs;
	//
	void AddMapEditorType( const string &szType, const string &szName, ObjectFactoryNewFunc pfnNewFunc )
	{
		newFuncs[szType].push_back( CMapEditorFactoryNewFuncsList::value_type(szName, pfnNewFunc) );
	}
public:
	//
	void RegisterTypes( const string &szType, RegisterEditorType pfnRegistrator ) const
	{
		CMapEditorFactoryNewFuncsMap::const_iterator posList = newFuncs.find( szType );
		if ( posList == newFuncs.end() ) 
			return;
		for ( CMapEditorFactoryNewFuncsList::const_iterator it = posList->second.begin(); it != posList->second.end(); ++it )
			(*pfnRegistrator)( it->first, it->second );
	}
	void UnRegisterTypes( const string &szType, UnRegisterEditorType pfnUnRegistrator ) const
	{
		CMapEditorFactoryNewFuncsMap::const_iterator posList = newFuncs.find( szType );
		if ( posList == newFuncs.end() ) 
			return;
		for ( CMapEditorFactoryNewFuncsList::const_iterator it = posList->second.begin(); it != posList->second.end(); ++it )
			(*pfnUnRegistrator)( it->first );
	}
};


static CEditorModuleRegistrator* GetEditorModuleRegistrator()
{
	static CEditorModuleRegistrator theMapEditorModuleRegistrator;
	return &theMapEditorModuleRegistrator;
}


void RegisterMapEditorTypeDelayed( const string &szType, const string &szName, ObjectFactoryNewFunc pfnNewFunc )
{
	GetEditorModuleRegistrator()->AddMapEditorType( szType, szName, pfnNewFunc );
}


