
#pragma once

#include "../libdb/Manipulator.h"


class CMaskManipulatorIterator;
class CMaskManipulator : public IManipulator
{
	friend class CMaskManipulatorIterator;

	OBJECT_BASIC_METHODS( CMaskManipulator )

public:
	enum EMaskMode
	{
		ORIGINAL_MODE	= 0,
		MASK_MODE			= 1,
		SMART_MODE		= 2,
	};
private:
	struct SProperty
	{
		bool bFilled;																		// Сигнализирует о заполненности информации

		string szName;																	// Короткое имя свойства
		string szType;																	// Имя типа (если есть)
		UINT nID;																				// ID (если есть)
		bool bHidden;																		// Скрытое ли поле?

		SProperty() : bFilled( false ), nID( INVALID_NODE_ID ), bHidden( false ) {}
		SProperty( const SProperty &rProperty ) : bFilled( rProperty.bFilled ), szName( rProperty.szName ), szType( rProperty.szType ), nID( rProperty.nID ), bHidden( rProperty.bHidden ) {}
		SProperty& operator=( const SProperty &rProperty )
		{
			if( &rProperty != this )
			{
				bFilled = rProperty.bFilled;
				szName = rProperty.szName;
				szType = rProperty.szType;
				nID = rProperty.nID;
				bHidden = rProperty.bHidden;
			}
			return *this;
		}	
	};
	typedef list<string> CPropertyList;								// для хранения порядка следования
	typedef hash_map<string, SProperty> CPropertyMap;	// для хранения данных
	typedef hash_map<int, string> CPropertyIDMap;			// для хранения данных

	EMaskMode maskMode;																// Тип путей воспринимаемых редактором
	CPropertyList propertyList;												// Информация по последовательности полей
	CPropertyMap propertyMap;													// Закешированная информация по полям
	CPropertyIDMap propertyIDMap;											// Список ID объектов (заполняется только не для пустых объектов)
	string szMask;																		// Путь добавляемый к элементам (с разделителем)
	CPtr<IManipulator> pTargetManipulator;						// Манипулятор, который мы маскируем

	// Перевести полученное имя в оригинальное в соответствии в установленным методом
	bool SetToOriginalName( string *pszName ) const;
	// Перевести полученное имя в короткое в соответствии в установленным методом
	bool SetToMaskName( string *pszName ) const;

	CMaskManipulator() {}
public:
	// Конструирование манипулятора 
	CMaskManipulator( const string& rszMask,  IManipulator *_pTargetManipulator, EMaskMode _maskMode );
	// Добавление имен ( имена могут быть длинными, а могут быть короткими, по усмотрению.
	bool AddName( const string &rszName, bool bFilled, const string& rszType, UINT nID, bool bHidden );
	// Установление типа работы (как воспринимаются все имена в методах IManipulator), возвращает старый тип работы
	inline EMaskMode SetMode( EMaskMode newMaskMode ) { const EMaskMode oldMaskMode = maskMode; maskMode = newMaskMode; return oldMaskMode; }
	// Получение типа работы (как воспринимаются все имена в методах IManipulator)
	inline EMaskMode GetMode() const { return maskMode; }
	// Установление пути
	inline void SetMask( const string &rszMask )
	{ 
		//DebugTrace( "CMaskManipulator::SetMask(): <%s>", rszMask.c_str() );
		szMask = rszMask;
	}
	// Получение пути
	inline void GetMask( string *pszMask ) const { ( *pszMask ) = szMask; }

	// IManipulator
	IManipulatorIterator* Iterate( bool bShowHidden, ECacheType eCache );
	const SIteratorDesc* GetDesc( const string &szName ) const;
	bool GetType( const string &rszName, string *pszType ) const;
	UINT GetID( const string &rszName ) const;
	bool GetName( UINT nID, string *pszName ) const;
	//
	bool InsertNode( const string &szName, int nNodeIndex = NODE_ADD_INDEX );
	bool RemoveNode( const string &szName, int nNodeIndex = NODE_REMOVEALL_INDEX );
	bool RemoveNodeByID( const string &szName, int nNodeID ) { return false; };
	bool RenameNode( const string &szName, const string &rszNewName );
	//
	bool GetValue( const string &szName, CVariant *pValue ) const;
	bool SetValue( const string &szName, const CVariant &value );
	bool CheckValue( const string &szName, const CVariant &value, bool *pResult ) const;
	NDb::IObjMan* GetObjMan();
	bool IsNameExists( const string &rszName ) const;
	void GetNameList( IManipulator::CNameMap *pNameMap ) const;
};


class CMaskManipulatorIterator : public IManipulatorIterator
{
	OBJECT_BASIC_METHODS( CMaskManipulatorIterator )
	
	CPtr<CMaskManipulator> pMaskManipulator;
	CMaskManipulator::CPropertyList::const_iterator propertyIterator;

	CMaskManipulatorIterator() {}
public:
	CMaskManipulatorIterator( CMaskManipulator *_pMaskManipulator );
	
	//IManipulatorIterator
	bool Next();
	bool IsEnd() const;
	const SIteratorDesc* GetDesc() const;
	bool GetName( string *pszName ) const;
	bool GetType( string *pszType ) const;
	UINT GetID() const;
	bool IsFolder() const { return false; }
};


