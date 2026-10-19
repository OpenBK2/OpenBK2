#pragma once

#include "Manipulator.h"
#include "TypeDef.h"

/**
Класс управляющий финальными типами
*/
class CTableManipulatorWrapper : public IManipulator
{
	OBJECT_BASIC_METHODS( CTableManipulatorWrapper );

	friend class CTableManipulatorIteratorWrapper;

	//CPtr<CXSDParser> pXSD;
	typedef std::unordered_map<std::string, CPtr<NDb::NTypeDef::STypeClass>> CNamesMap;
	CNamesMap namesMap;
	typedef std::unordered_map<int, CPtr<NDb::NTypeDef::STypeClass>> CIDsMap;
	CIDsMap idsMap;
	// non-inherited
	const NDb::NTypeDef::STypeClass *GetType( const std::string &szName ) const;
	const NDb::NTypeDef::STypeClass *GetType( int nTypeID ) const;

	//
	CTableManipulatorWrapper() {}

public:
	CTableManipulatorWrapper( std::vector<NDb::NTypeDef::STypeClass *> &classes );
	// IManipulator
	IManipulatorIterator *Iterate( bool bShowHidden, ECacheType eCache ) override;

	const SIteratorDesc *GetDesc( const std::string &szName ) const override
	{
		return 0;
	}

	bool GetType( const std::string &rszName, std::string *pszType ) const override
	{
		return false;
	}

	unsigned GetID( const std::string &rszName ) const override;
	bool GetName( unsigned nID, std::string *pszName ) const override;

	bool InsertNode( const std::string &szName, int nNodeIndex = NODE_ADD_INDEX ) override
	{
		return false;
	}

	bool RemoveNode( const std::string &szName, int nNodeIndex = NODE_REMOVEALL_INDEX ) override
	{
		return false;
	}

	bool RemoveNodeByID( const std::string &szName, int nNodeID ) override
	{
		return false;
	}

	bool RenameNode( const std::string &szName, const std::string &szNewName ) override
	{
		return false;
	}

	bool GetValue( const std::string &szName, CVariant *pValue ) const override;

	bool SetValue( const std::string &szName, const CVariant &value ) override
	{
		return false;
	}

	bool IsNameExists( const std::string &rszName ) const override;

	void GetNameList( CNameMap *pNameMap ) const override {}

	bool CheckValue( const std::string &szName, const CVariant &value, bool *pResult ) const override
	{
		return false;
	}

	NDb::IObjMan *GetObjMan() override
	{
		return 0;
	}
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

	bool Next() override;
	bool IsEnd() const override;

	const SIteratorDesc *GetDesc() const override
	{
		return 0;
	}

	bool GetName( std::string *pszName ) const override;

	bool GetType( std::string *pszType ) const override
	{
		return false;
	}

	unsigned GetID() const override
	{
		return itCurrType->second->nClassTypeID;
	}

	bool IsFolder() const override
	{
		return false;
	}
};
