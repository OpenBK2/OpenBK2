#pragma once

#include "Interface_Controller.h"
#include "libdb/Manipulator.h"

#include "MapEditorLib_export.h"

class CDefaultView;
class MAPEDITORLIB_EXPORT CDefaultController : public IController
{
	friend class CDefaultView;
	
	// Operation Description it the Undo List
	std::string szDescription;
	// Метка для последующего удаления Undo Operation из очереди
	std::string szTemporaryLabel;
	// Список масок ( задается манипулятором при создании Controller )
	IManipulator::CNameMap nameMap;
	// Набор изменяемых объектов
	SObjectSet objectSet;

protected:
	// New methods
	virtual bool UndoWithoutUpdateViews() = 0;
	virtual bool RedoWithoutUpdateViews() = 0;
public:	
	CDefaultController() {}
	//
	inline const SObjectSet& GetObjectSet() const { return objectSet; }
	inline void SetObjectSet( const SObjectSet &rObjectSet ) { objectSet = rObjectSet; }
	//
	bool Undo(  bool bUpdateManipulator, bool bUpdateViews, IView *pViewToExlude );
	bool Redo(  bool bUpdateManipulator, bool bUpdateViews, IView *pViewToExlude );
	//
	virtual void GetDescription( std::string *pszDescription ) const  { if ( pszDescription ) { ( *pszDescription ) = szDescription; } }
	virtual void SetDescription( const std::string &rszDescription ) { szDescription = rszDescription; }
	//
	virtual void GetTemporaryLabel( std::string *pszTemporaryLabel ) const  { if ( pszTemporaryLabel ) { ( *pszTemporaryLabel ) = szTemporaryLabel; } }
	virtual void SetTemporaryLabel( const std::string &rszTemporaryLabel ) { szTemporaryLabel = rszTemporaryLabel; }
	//
	virtual IManipulator::CNameMap& GetNameList() { return nameMap; }
	virtual void SetNameList( const IManipulator::CNameMap &rNameList ) { nameMap = rNameList; }
	virtual void GetNameListToUpdate( IManipulator::CNameMap *pNameMap, const IManipulator::CNameMap &rManipulatorNameMap, const std::string &rszName ) const;
};



