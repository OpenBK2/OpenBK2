#pragma once
#include "Manipulator.h"
#include "ObjMan.h"
#include "ObjManIterator.h"

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
	CObjectManipulatorWrapper( NDb::IObjMan *_pObjMan )
		: pObjMan( _pObjMan )
	{
	}

	// IManipulator
	IManipulatorIterator *Iterate( bool bShowHidden, ECacheType eCache ) override;
	const SIteratorDesc *GetDesc( const std::string &szName ) const override;
	bool GetType( const std::string &rszName, std::string *pszType ) const override;
	unsigned GetID( const std::string &rszName ) const override;
	CDBID GetDBID() const override;
	bool GetName( unsigned nID, std::string *pszName ) const override;
	bool GetValue( const std::string &szName, CVariant *pValue ) const override;
	bool SetValue( const std::string &szName, const CVariant &value ) override;
	bool CheckValue( const std::string &szName, const CVariant &value, bool *pResult ) const override;

	NDb::IObjMan *GetObjMan() override
	{
		return pObjMan;
	}

	bool InsertNode( const std::string &szName, int nNodeIndex = NODE_ADD_INDEX ) override;
	bool RemoveNode( const std::string &szName, int nNodeIndex = NODE_REMOVEALL_INDEX ) override;
	bool RemoveNodeByID( const std::string &szName, int nNodeID ) override;

	bool RenameNode( const std::string &szName, const std::string &szNewName ) override
	{
		return false;
	}

	bool IsNameExists( const std::string &rszName ) const override;

	void GetNameList( CNameMap *pNameMap ) const override {}

	void ClearCache() override;
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
	CObjectManipulatorIteratorWrapper( NDb::IObjManIterator *_pIterator )
		: pIterator( _pIterator )
	{
	}

	//interface functions
	bool Next() override;
	bool IsEnd() const override;
	const SIteratorDesc *GetDesc() const override;
	bool GetType( std::string *pszType ) const override;
	bool GetName( std::string *pszName ) const override;

	unsigned GetID() const override
	{
		return INVALID_NODE_ID;
	}

	bool IsFolder() const override
	{
		return false;
	}
};
