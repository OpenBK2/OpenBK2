#pragma once

#include <cstdint>

#include <boost/uuid/uuid.hpp>

//#include "DBIDDef.h"

/**
 * Variant class
 */
class CVariant
{
public:
	enum EVariantType
	{
		VT_NULL = 0,
		VT_INT,
		VT_FLOAT,
		VT_STR,
		VT_BOOL,
		VT_POINTER,
		VT_WSTR,
		VT_RESERVED,
		VT_DBID,
		VT_MULTIVARIANT
	};

	typedef std::unordered_map<CDBID, CVariant> CMultiVariantMap;
private:
	EVariantType m_eType;

	/**
	контейнер для указателя на блок памяти
	*/
	struct SBlob
	{
		void *m_ptr;
		size_t m_nSize;
		bool m_bDelete;

		SBlob(): m_ptr(0), m_nSize(0), m_bDelete(false) {}
		SBlob(void *_ptr): m_ptr(_ptr), m_nSize(0), m_bDelete(false) {}
		SBlob(const void *_ptr, size_t _nSize, bool _bDelete = true): 
			m_ptr(new uint8_t[_nSize]), m_nSize(_nSize), m_bDelete(_bDelete)
		{
			ASSERT( _ptr != 0 );
			memcpy( m_ptr, _ptr, m_nSize );
		}
		SBlob(const SBlob &_blob):
			m_nSize(_blob.m_nSize), m_bDelete(_blob.m_bDelete) 
		{
			if(m_bDelete){
				ASSERT( _blob.m_ptr != 0 );
				m_ptr = new uint8_t[m_nSize];
				memcpy( m_ptr, _blob.m_ptr, m_nSize );
			}
			else
			{
				m_ptr = _blob.m_ptr;
			}
		}

		~SBlob()
		{
			if(m_bDelete)
			{
				delete[] m_ptr;
			}
		}

		int operator & (IBinSaver &saver)
		{
			saver.Add(1, &m_nSize);

			if( saver.IsReading() )
			{
				m_bDelete = true;
				m_ptr = new uint8_t[m_nSize];
			}

			saver.AddRawData(2, m_ptr, m_nSize);

			return 0;
		}
	};

	struct SMultiVariant
	{
		CMultiVariantMap *m_pMultiVariantMap;

		SMultiVariant() {}
		SMultiVariant(CMultiVariantMap *_pMultiVariantMap)
		{
			ASSERT( _pMultiVariantMap != 0 );

			m_pMultiVariantMap = new CMultiVariantMap();
			*( m_pMultiVariantMap ) = *( _pMultiVariantMap );
		}
		SMultiVariant(const SMultiVariant &_mvar)
		{
			ASSERT( _mvar.m_pMultiVariantMap != 0 );

			m_pMultiVariantMap = new CMultiVariantMap();
			*( m_pMultiVariantMap ) = *( _mvar.m_pMultiVariantMap );
		}

		~SMultiVariant()
		{
			ASSERT( m_pMultiVariantMap != 0 );
			delete m_pMultiVariantMap;
		}
	};

	union 
	{
		float m_float;
		int m_int;
		bool m_bool;
		SBlob *m_pblob;
		std::string *m_pstr;
		std::wstring *m_pwstr;
		CDBID *m_dbid;
		SMultiVariant *m_pmvar;
	};

	void Clear();

	void Copy(const CVariant &var);

public:
	//конструкторы
	CVariant() : m_eType( VT_NULL ), m_pblob(NULL) {}
	CVariant( const CVariant &var );
	CVariant& operator=( const CVariant &var );
	void SetMultiVariant( CMultiVariantMap *pMultiVariantMap, bool bDeleteInDestructor = true /* in fact it is true anyway */);
	void SetDestructorDeleted( bool bDeleteInDestructor, int nBlobSize )
	{ 
		ASSERT( m_eType == VT_POINTER );
		m_pblob->m_bDelete = bDeleteInDestructor;
		m_pblob->m_nSize = nBlobSize;
	}
	//конструкторы с приведением типов
	CVariant( const int nVal )          : m_eType( VT_INT ), m_int( nVal ) {}
	CVariant( const float fVal )        : m_eType( VT_FLOAT ), m_float(fVal) {}
	CVariant( const std::string &szVal )     : m_eType( VT_STR ), m_pstr( new std::string(szVal) ) {}
	CVariant( const char *pszVal )			: m_eType( VT_STR ), m_pstr( new std::string(pszVal) ) {}
	CVariant( const std::wstring &wszVal )   : m_eType( VT_WSTR ), m_pwstr( new std::wstring(wszVal) ) {}
	CVariant( const wchar_t *pwszVal )	: m_eType( VT_WSTR ), m_pwstr( new std::wstring(pwszVal) ) {}
	CVariant( const bool bVal )         : m_eType( VT_BOOL ), m_bool( bVal ) {}
	CVariant( const CDBID &dbid )				: m_eType( VT_DBID ), m_dbid( new CDBID(dbid) ) {}
	//
	CVariant( const uint32_t dwVal )				: m_eType( VT_INT ), m_int( dwVal ) { }
	CVariant( const long lVal )					: m_eType( VT_INT ), m_int( lVal ) { }
	CVariant( const uint16_t wVal )					: m_eType( VT_INT ), m_int( wVal ) { }
	CVariant( const short nVal )				: m_eType( VT_INT ), m_int( nVal ) { }
	CVariant( const char cVal )					: m_eType( VT_INT ), m_int( cVal ) { }
	CVariant( const uint8_t cVal )					: m_eType( VT_INT ), m_int( cVal ) { }
	//
	CVariant( const boost::uuids::uuid &guid )				: m_eType( VT_POINTER ), m_pblob( new SBlob((void*)&guid, sizeof(guid)) ) {}
private:
	//CVariant не может определить размер блока по указателю, поэтому без указания размера
	//конструктор от void* не имеет смысла
	// Намеренно его закрываем, чтобы не происходило неявного преобразования void * к bool
	CVariant( void *pVar )                  : m_eType( VT_POINTER ) {}
public:
	CVariant( const void *pVar, int nBlobSize )   : m_eType( VT_POINTER ), m_pblob( new SBlob(pVar, nBlobSize) ) {}

	CVariant( CMultiVariantMap *pMultiVariantMap/*, bool bDelete = false*/ ) : 
		m_eType( VT_MULTIVARIANT ), m_pmvar( new SMultiVariant(pMultiVariantMap) )
	{}

	~CVariant();
	//
	EVariantType GetType() const { return m_eType; }
	bool IsNull() const { return m_eType == VT_NULL; }
	//приведение типов
	operator int() const;
	operator float() const;
	//operator const string&() const;
	//operator const wstring&() const;
	//operator const char*() const;
	//operator const wchar_t*() const;
	operator bool() const;
	//получение значений
	bool             GetBool() const;
	int              GetInt() const;
	float            GetFloat() const;
	const char      *GetStr() const;
	const wchar_t   *GetWStr() const;
	const CDBID &GetDBID() const;
	const void      *GetPtr() const;
	const CMultiVariantMap* GetMultiVariant() const;
	bool ToText( std::string *pszText ) const;
	// CRAP{ special function to cross get MBCS <=> UNICODE
	std::string GetStringRecode() const;
	std::wstring GetWStringRecode() const;
	// CRAP}

	size_t GetBlobSize() const;
	//
  bool operator!=( const CVariant &var ) const; //не точное сравнение ( с приведением типов )
	bool operator==( const CVariant &var ) const; //точное совпадение согласно типам

	int operator & ( IBinSaver &saver );
	int operator & ( IXmlSaver &saver );

	void Trace() const;
};

