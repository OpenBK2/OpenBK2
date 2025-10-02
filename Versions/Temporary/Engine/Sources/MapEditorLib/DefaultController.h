#if !defined(__DEFAULT_CONTROLLER__)
#define __DEFAULT_CONTROLLER__
#pragma once

#include "Interface_Controller.h"
#include "../libdb/Manipulator.h"


class CDefaultView;
class CDefaultController : public IController
{
	friend class CDefaultView;
	
	// Operation Description it the Undo List
	CString strDescription;
	// Метка для последующего удаления Undo Operation из очереди
	string szTemporaryLabel;
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
	virtual void GetDescription( CString *pstrDescription ) const  { if ( pstrDescription ) { ( *pstrDescription ) = strDescription; } }
	virtual void SetDescription( const CString &rstrDescription ) { strDescription = rstrDescription; }
	//
	virtual void GetTemporaryLabel( string *pszTemporaryLabel ) const  { if ( pszTemporaryLabel ) { ( *pszTemporaryLabel ) = szTemporaryLabel; } }
	virtual void SetTemporaryLabel( const string &rszTemporaryLabel ) { szTemporaryLabel = rszTemporaryLabel; }
	//
	virtual IManipulator::CNameMap& GetNameList() { return nameMap; }
	virtual SetNameList( const IManipulator::CNameMap &rNameList ) { nameMap = rNameList; }
	virtual void GetNameListToUpdate( IManipulator::CNameMap *pNameMap, const IManipulator::CNameMap &rManipulatorNameMap, const string &rszName ) const;
};

#endif // !defined(__DEFAULT_CONTROLLER__)


