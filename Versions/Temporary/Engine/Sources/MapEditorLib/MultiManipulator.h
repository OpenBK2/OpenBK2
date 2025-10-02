#if !defined(__MULTI_MANIPULATOR__)
#define __MULTI_MANIPULATOR__

#pragma ONCE

#include "../libdb/Manipulator.h"
#include "Tools_UniqueList.h"


class CMultiManipulatorIterator;
class CMultiManipulator : public IManipulator
{
	friend class CMultiManipulatorIterator;

	OBJECT_BASIC_METHODS( CMultiManipulator )

	typedef hash_map<CDBID, CPtr<IManipulator> > CManipulatorMap;
	typedef list<string> CNameMap;
	typedef CUniqueList<CNameMap, string> CUniqueNameList;
	
	CManipulatorMap manipulatorMap;
	CDBID activeDBID;
	CPtr<IManipulator> pActiveManipulator;
	CDBID propertyDescDBID;
	CPtr<IManipulator> pPropertyDescManipulator;
	CPtr<IManipulator> pFirstManipulator;

	bool DescExists( const string &rszName ) const;
	bool TypeExists( const string &rszName ) const;
	bool IDExists( const string &rszName ) const;
	bool NameExists( UINT nID ) const;
	bool NameExists( const string &rszName ) const;
	int GetMinimalCount( const string &rszName, bool *pbMultiVariant ) const;
	bool GetMultiValue( const string &rszName, CVariant *pValue ) const;
	bool SetMultiValue( const string &rszName, const CVariant &rValue );
	bool CheckMultiValue( const string &rszName, const CVariant &rValue, bool *pResult ) const;

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
	const SIteratorDesc* GetDesc( const string &rszName ) const;
	bool GetType( const string &rszName, string *pszType ) const;
	UINT GetID( const string &rszName ) const;
	bool GetName( UINT nID, string *pszName ) const;
	//
	bool InsertNode( const string &rszName, int nNodeIndex = NODE_ADD_INDEX );
	bool RemoveNode( const string &rszName, int nNodeIndex = NODE_REMOVEALL_INDEX );
	bool RemoveNodeByID( const string &rszName, int nNodeID ) { return false; };
	bool RenameNode( const string &rszName, const string &rszNewName );
	//
	bool GetValue( const string &rszName, CVariant *pValue ) const;
	bool SetValue( const string &rszName, const CVariant &rValue );
	bool CheckValue( const string &rszName, const CVariant &rValue, bool *pResult ) const;
	NDb::IObjMan* GetObjMan();
	bool IsNameExists( const string &rszName ) const;
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
	bool GetName( string *pszName ) const;
	bool GetType( string *pszType ) const;
	UINT GetID() const;
	bool IsFolder() const { return false; }
};

#endif // !defined(__MULTI_MANIPULATOR__)

