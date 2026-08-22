#pragma once
#include "ObjMan.h"
#include "ObjManIterator.h"
#include "Manipulator.h"

/**
Класс управляющий полями объекта финального типа
*/
class CObjectManipulatorWrapper : public IManipulator
{
	OBJECT_BASIC_METHODS( CObjectManipulatorWrapper );
	//
	CObj<NDb::IObjMan> pObjMan;
	//
	CObjectManipulatorWrapper() {}
public:
	CObjectManipulatorWrapper( NDb::IObjMan *_pObjMan ): pObjMan( _pObjMan ) {}

	// IManipulator
	IManipulatorIterator* Iterate( bool bShowHidden, ECacheType eCache );
	const SIteratorDesc* GetDesc( const std::string &szName ) const;
	bool GetType( const std::string &rszName, std::string *pszType ) const;
	unsigned GetID( const std::string &rszName ) const;
	CDBID GetDBID() const;
	bool GetName( unsigned nID, std::string *pszName ) const;
	bool GetValue( const std::string &szName, CVariant *pValue ) const;
	bool SetValue( const std::string &szName, const CVariant &value );
	bool CheckValue( const std::string &szName, const CVariant &value, bool *pResult ) const;
	NDb::IObjMan* GetObjMan() { return pObjMan; }
	bool InsertNode( const std::string &szName, int nNodeIndex = NODE_ADD_INDEX );
	bool RemoveNode( const std::string &szName, int nNodeIndex = NODE_REMOVEALL_INDEX );
	bool RemoveNodeByID( const std::string &szName, int nNodeID );
	bool RenameNode( const std::string &szName, const std::string &szNewName ) { return false; }
	bool IsNameExists( const std::string &rszName ) const;
	void GetNameList( CNameMap *pNameMap ) const {}
	void ClearCache();
};

class CObjectManipulatorIteratorWrapper : public IManipulatorIterator
{
	OBJECT_BASIC_METHODS( CObjectManipulatorIteratorWrapper );
	//
	CObj<NDb::IObjManIterator> pIterator;
	//
protected:
	CObjectManipulatorIteratorWrapper() {}
public:
	CObjectManipulatorIteratorWrapper( NDb::IObjManIterator *_pIterator ): pIterator( _pIterator ) {}
	
	//interface functions
	bool Next();
	bool IsEnd() const;
	const SIteratorDesc* GetDesc() const;
	bool GetType( std::string *pszType ) const;
	bool GetName( std::string *pszName ) const;
	unsigned GetID() const { return INVALID_NODE_ID; }
	bool IsFolder() const { return false; }
};


