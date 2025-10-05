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
	typedef std::unordered_map<std::string, CPtr<STypeClass> > CNamesMap;
	CNamesMap namesMap;
	typedef std::unordered_map<int, CPtr<STypeClass> > CIDsMap;
	CIDsMap idsMap;
	// non-inherited
	const STypeClass* GetType( const std::string &szName ) const;
	const STypeClass* GetType( int nTypeID ) const;
	//
	CTableManipulatorWrapper() {}
public:
	CTableManipulatorWrapper( std::vector<STypeClass *> &classes );
	// IManipulator
	IManipulatorIterator* Iterate( bool bShowHidden, ECacheType eCache );
	const SIteratorDesc* GetDesc( const std::string &szName ) const { return 0; }
	bool GetType( const std::string &rszName, std::string *pszType ) const { return false; }
	UINT GetID( const std::string &rszName ) const;
	bool GetName( UINT nID, std::string *pszName ) const;
	bool InsertNode( const std::string &szName, int nNodeIndex = NODE_ADD_INDEX ) { return false; }
	bool RemoveNode( const std::string &szName, int nNodeIndex = NODE_REMOVEALL_INDEX ) { return false; }
	bool RemoveNodeByID( const std::string &szName, int nNodeID ) { return false; }
	bool RenameNode( const std::string &szName, const std::string &szNewName ) { return false; }
	bool GetValue( const std::string &szName, CVariant *pValue ) const;
	bool SetValue( const std::string &szName, const CVariant &value ) { return false; }
	bool IsNameExists( const std::string &rszName ) const;
	void GetNameList( CNameMap *pNameMap ) const {}
	bool CheckValue( const std::string &szName, const CVariant &value, bool *pResult ) const { return false; }
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
	bool GetName( std::string *pszName ) const;
	bool GetType( std::string *pszType ) const {	return false;	}
	UINT GetID() const { return itCurrType->second->nClassTypeID; }
	bool IsFolder() const { return false; }
};


