
#pragma once

#include "libdb/Manipulator.h"

#include "MapEditorLib_export.h"

class CMaskManipulatorIterator;

class MAPEDITORLIB_EXPORT CMaskManipulator : public IManipulator
{
	friend class CMaskManipulatorIterator;

	OBJECT_BASIC_METHODS( CMaskManipulator )

public:
	enum EMaskMode
	{
		ORIGINAL_MODE = 0,
		MASK_MODE = 1,
		SMART_MODE = 2,
	};

private:
	struct SProperty
	{
		bool bFilled; // Сигнализирует о заполненности информации

		std::string szName; // Короткое имя свойства
		std::string szType; // Имя типа (если есть)
		unsigned nID;       // ID (если есть)
		bool bHidden;       // Скрытое ли поле?

		SProperty()
			: bFilled( false )
			, nID( INVALID_NODE_ID )
			, bHidden( false )
		{
		}

		SProperty( const SProperty &rProperty )
			: bFilled( rProperty.bFilled )
			, szName( rProperty.szName )
			, szType( rProperty.szType )
			, nID( rProperty.nID )
			, bHidden( rProperty.bHidden )
		{
		}

		SProperty &operator=( const SProperty &rProperty )
		{
			if ( &rProperty != this )
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

	typedef std::list<std::string> CPropertyList;                    // для хранения порядка следования
	typedef std::unordered_map<std::string, SProperty> CPropertyMap; // для хранения данных
	typedef std::unordered_map<int, std::string> CPropertyIDMap;     // для хранения данных

	EMaskMode maskMode;                    // Тип путей воспринимаемых редактором
	CPropertyList propertyList;            // Информация по последовательности полей
	CPropertyMap propertyMap;              // Закешированная информация по полям
	CPropertyIDMap propertyIDMap;          // Список ID объектов (заполняется только не для пустых объектов)
	std::string szMask;                    // Путь добавляемый к элементам (с разделителем)
	CPtr<IManipulator> pTargetManipulator; // Манипулятор, который мы маскируем

	// Перевести полученное имя в оригинальное в соответствии в установленным методом
	bool SetToOriginalName( std::string *pszName ) const;
	// Перевести полученное имя в короткое в соответствии в установленным методом
	bool SetToMaskName( std::string *pszName ) const;

	CMaskManipulator() {}

public:
	// Конструирование манипулятора
	CMaskManipulator( const std::string &rszMask, IManipulator *_pTargetManipulator, EMaskMode _maskMode );
	// Добавление имен ( имена могут быть длинными, а могут быть короткими, по усмотрению.
	bool AddName( const std::string &rszName, bool bFilled, const std::string &rszType, unsigned nID, bool bHidden );

	// Установление типа работы (как воспринимаются все имена в методах IManipulator), возвращает старый тип работы
	inline EMaskMode SetMode( EMaskMode newMaskMode )
	{
		const EMaskMode oldMaskMode = maskMode;
		maskMode = newMaskMode;
		return oldMaskMode;
	}

	// Получение типа работы (как воспринимаются все имена в методах IManipulator)
	inline EMaskMode GetMode() const
	{
		return maskMode;
	}

	// Установление пути
	inline void SetMask( const std::string &rszMask )
	{
		//DebugTrace( "CMaskManipulator::SetMask(): <%s>", rszMask.c_str() );
		szMask = rszMask;
	}

	// Получение пути
	inline void GetMask( std::string *pszMask ) const
	{
		( *pszMask ) = szMask;
	}

	// IManipulator
	IManipulatorIterator *Iterate( bool bShowHidden, ECacheType eCache ) override;
	const SIteratorDesc *GetDesc( const std::string &szName ) const override;
	bool GetType( const std::string &rszName, std::string *pszType ) const override;
	unsigned GetID( const std::string &rszName ) const override;
	bool GetName( unsigned nID, std::string *pszName ) const override;
	//
	bool InsertNode( const std::string &szName, int nNodeIndex = NODE_ADD_INDEX ) override;
	bool RemoveNode( const std::string &szName, int nNodeIndex = NODE_REMOVEALL_INDEX ) override;

	bool RemoveNodeByID( const std::string &szName, int nNodeID ) override
	{
		return false;
	}

	bool RenameNode( const std::string &szName, const std::string &rszNewName ) override;
	//
	bool GetValue( const std::string &szName, CVariant *pValue ) const override;
	bool SetValue( const std::string &szName, const CVariant &value ) override;
	bool CheckValue( const std::string &szName, const CVariant &value, bool *pResult ) const override;
	NDb::IObjMan *GetObjMan() override;
	bool IsNameExists( const std::string &rszName ) const override;
	void GetNameList( IManipulator::CNameMap *pNameMap ) const override;
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
	bool Next() override;
	bool IsEnd() const override;
	const SIteratorDesc *GetDesc() const override;
	bool GetName( std::string *pszName ) const override;
	bool GetType( std::string *pszType ) const override;
	unsigned GetID() const override;

	bool IsFolder() const override
	{
		return false;
	}
};
