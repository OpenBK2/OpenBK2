
#pragma once

#include "libdb/Manipulator.h"
#include "Tools_UniqueList.h"

#include "MapEditorLib_export.h"

class CMultiManipulatorIterator;
class MAPEDITORLIB_EXPORT CMultiManipulator : public IManipulator
{
	friend class CMultiManipulatorIterator;

	OBJECT_BASIC_METHODS( CMultiManipulator )

	typedef std::unordered_map<CDBID, CPtr<IManipulator> > CManipulatorMap;
	typedef std::list<std::string> CNameMap;
	typedef CUniqueList<CNameMap, std::string> CUniqueNameList;
	
	CManipulatorMap manipulatorMap;
	CDBID activeDBID;
	CPtr<IManipulator> pActiveManipulator;
	CDBID propertyDescDBID;
	CPtr<IManipulator> pPropertyDescManipulator;
	CPtr<IManipulator> pFirstManipulator;

	bool DescExists( const std::string &rszName ) const;
	bool TypeExists( const std::string &rszName ) const;
	bool IDExists( const std::string &rszName ) const;
	bool NameExists( unsigned nID ) const;
	bool NameExists( const std::string &rszName ) const;
	int GetMinimalCount( const std::string &rszName, bool *pbMultiVariant ) const;
	bool GetMultiValue( const std::string &rszName, CVariant *pValue ) const;
	bool SetMultiValue( const std::string &rszName, const CVariant &rValue );
	bool CheckMultiValue( const std::string &rszName, const CVariant &rValue, bool *pResult ) const;

public:
	// Конструирование манипулятора 
	CMultiManipulator() : pActiveManipulator( 0 ), pPropertyDescManipulator( 0 ), pFirstManipulator( 0 ) {}

	// Добавить манипулятор в список
	void InsertManipulator( const CDBID &rDBID, IManipulator* pManipulator, bool bActive, bool bPropertyDesc );
	// Удалить манипулятор из списка
	void RemoveManipulator( const CDBID &rDBID );
	// установить активный манипулятор
	bool SetActiveManipulator( const CDBID &rDBID );
	// установить манипулятор у которого будут опрашивать свойства полей
	bool SetPropertyDescManipulator( const CDBID &rDBID );
	//
	int IsEmpty() { return manipulatorMap.empty(); }
	// вернуть текущий активный манипулятор
	inline const CDBID& GetActiveDBID() { return activeDBID; }
	inline IManipulator* GetActiveManipulator() { return pActiveManipulator; }
	// вернуть текущий активный манипулятор
	inline const CDBID& GetPropertyDescDBID() { return propertyDescDBID; }
	inline IManipulator* GetPropertyDescManipulator() { return pPropertyDescManipulator; }
	// вернуть манипулятор, который определяет ID и прочее (при отсутствии первых двух )
	inline IManipulator* GetFirstManipulator() { return pFirstManipulator; }

	// IManipulator
	IManipulatorIterator* Iterate( bool bShowHidden, ECacheType eCache );
	const SIteratorDesc* GetDesc( const std::string &rszName ) const;
	bool GetType( const std::string &rszName, std::string *pszType ) const;
	unsigned GetID( const std::string &rszName ) const;
	bool GetName( unsigned nID, std::string *pszName ) const;
	//
	bool InsertNode( const std::string &rszName, int nNodeIndex = NODE_ADD_INDEX );
	bool RemoveNode( const std::string &rszName, int nNodeIndex = NODE_REMOVEALL_INDEX );
	bool RemoveNodeByID( const std::string &rszName, int nNodeID ) { return false; };
	bool RenameNode( const std::string &rszName, const std::string &rszNewName );
	//
	bool GetValue( const std::string &rszName, CVariant *pValue ) const;
	bool SetValue( const std::string &rszName, const CVariant &rValue );
	bool CheckValue( const std::string &rszName, const CVariant &rValue, bool *pResult ) const;
	NDb::IObjMan* GetObjMan();
	bool IsNameExists( const std::string &rszName ) const;
	void GetNameList( IManipulator::CNameMap *pNameMap ) const;
};


class CMultiManipulatorIterator : public IManipulatorIterator
{
	OBJECT_BASIC_METHODS( CMultiManipulatorIterator )
	
	CPtr<CMultiManipulator> pMultiManipulator;
	CPtr<IManipulatorIterator> pManipulatorIterator;

	CMultiManipulatorIterator() {}
public:
	CMultiManipulatorIterator( CMultiManipulator *_pMultiManipulator, bool bShowHidden, ECacheType eCache );
	
	//IManipulatorIterator
	bool Next();
	bool IsEnd() const;
	const SIteratorDesc* GetDesc() const;
	bool GetName( std::string *pszName ) const;
	bool GetType( std::string *pszType ) const;
	unsigned GetID() const;
	bool IsFolder() const { return false; }
};


