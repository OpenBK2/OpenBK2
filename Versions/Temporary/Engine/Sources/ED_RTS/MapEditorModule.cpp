#include "stdafx.h"

#include "MapEditorLib/MapEditorModule.h"

class CEditorModuleRegistrator : public IEditorModuleRegistrator
{
	friend void RegisterMapEditorTypeDelayed( const std::string &szType, const std::string &szName, ObjectFactoryNewFunc pfnNewFunc );
	//
	typedef std::list< std::pair<std::string, ObjectFactoryNewFunc> > CMapEditorFactoryNewFuncsList;
	typedef std::unordered_map<std::string, CMapEditorFactoryNewFuncsList> CMapEditorFactoryNewFuncsMap;
	CMapEditorFactoryNewFuncsMap newFuncs;
	//
	void AddMapEditorType( const std::string &szType, const std::string &szName, ObjectFactoryNewFunc pfnNewFunc )
	{
		newFuncs[szType].push_back( CMapEditorFactoryNewFuncsList::value_type(szName, pfnNewFunc) );
	}
public:
	//
	void RegisterTypes( const std::string &szType, RegisterEditorType pfnRegistrator ) const
	{
		CMapEditorFactoryNewFuncsMap::const_iterator posList = newFuncs.find( szType );
		if ( posList == newFuncs.end() ) 
			return;
		for ( CMapEditorFactoryNewFuncsList::const_iterator it = posList->second.begin(); it != posList->second.end(); ++it )
			(*pfnRegistrator)( it->first, it->second );
	}
	void UnRegisterTypes( const std::string &szType, UnRegisterEditorType pfnUnRegistrator ) const
	{
		CMapEditorFactoryNewFuncsMap::const_iterator posList = newFuncs.find( szType );
		if ( posList == newFuncs.end() ) 
			return;
		for ( CMapEditorFactoryNewFuncsList::const_iterator it = posList->second.begin(); it != posList->second.end(); ++it )
			(*pfnUnRegistrator)( it->first );
	}
};

static CEditorModuleRegistrator theMapEditorModuleRegistrator;

void RegisterMapEditorTypeDelayed( const std::string &szType, const std::string &szName, ObjectFactoryNewFunc pfnNewFunc )
{
	theMapEditorModuleRegistrator.AddMapEditorType( szType, szName, pfnNewFunc );
}

const IEditorModuleRegistrator* GetEditorModuleRegistrator() { return &theMapEditorModuleRegistrator; }


