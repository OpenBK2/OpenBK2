#pragma once

#include "Manipulator.h"
#include "TypeDef.h"

using namespace NDb::NTypeDef;

/**
Класс управляющий финальными типами
*/
class CTableManipulatorWrapper : public IManipulator
{
	OBJECT_BASIC_METHODS( CTableManipulatorWrapper );

	friend class CTableManipulatorIteratorWrapper;

	//CPtr<CXSDParser> pXSD;
	typedef hash_map<string, CPtr<STypeClass> > CNamesMap;
	CNamesMap namesMap;
	typedef hash_map<int, CPtr<STypeClass> > CIDsMap;
	CIDsMap idsMap;
	// non-inherited
	const STypeClass* GetType( const string &szName ) const;
	const STypeClass* GetType( int nTypeID ) const;
	//
	CTableManipulatorWrapper() {}
public:
	CTableManipulatorWrapper( vector<STypeClass *> &classes );
	// IManipulator
	IManipulatorIterator* Iterate( bool bShowHidden, ECacheType eCache );
	const SIteratorDesc* GetDesc( const string &szName ) const { return 0; }
	bool GetType( const string &rszName, string *pszType ) const { return false; }
	UINT GetID( const string &rszName ) const;
	bool GetName( UINT nID, string *pszName ) const;
	bool InsertNode( const string &szName, int nNodeIndex = NODE_ADD_INDEX ) { return false; }
	bool RemoveNode( const string &szName, int nNodeIndex = NODE_REMOVEALL_INDEX ) { return false; }
	bool RemoveNodeByID( const string &szName, int nNodeID ) { return false; }
	bool RenameNode( const string &szName, const string &szNewName ) { return false; }
	bool GetValue( const string &szName, CVariant *pValue ) const;
	bool SetValue( const string &szName, const CVariant &value ) { return false; }
	bool IsNameExists( const string &rszName ) const;
	void GetNameList( CNameMap *pNameMap ) const {}
	bool CheckValue( const string &szName, const CVariant &value, bool *pResult ) const { return false; }
	NDb::IObjMan* GetObjMan() { return 0; }
};


class CTableManipulatorIteratorWrapper : public IManipulatorIterator
{
	OBJECT_BASIC_METHODS( CTableManipulatorIteratorWrapper );
	//
	CPtr<CTableManipulatorWrapper> pTableMan;
	CTableManipulatorWrapper::CNamesMap::const_iterator itCurrType;
	bool bShowHidden;
	//
	CTableManipulatorIteratorWrapper() {}
public:
	CTableManipulatorIteratorWrapper( CTableManipulatorWrapper *pMan, bool _bShowHidden = false );

	bool Next();
	bool IsEnd() const;
	const SIteratorDesc* GetDesc() const { return 0; }
	bool GetName( string *pszName ) const;
	bool GetType( string *pszType ) const {	return false;	}
	UINT GetID() const { return itCurrType->second->nClassTypeID; }
	bool IsFolder() const { return false; }
};


