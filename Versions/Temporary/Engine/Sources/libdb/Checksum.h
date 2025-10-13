#pragma once

#include "libdb_export.h"

#include "port/cdecl.h"

#include <cstdint>

namespace NDb
{
LIBDB_EXPORT uint32_t GetDefaultCheckSum();
LIBDB_EXPORT uint32_t CalcCheckSum( const uint32_t dwLastCheckSum, const uint8_t *pBuf, const int nLen );

class CCheckSum
{
	uint32_t dwCheckSum;
	IBinSaver *p;
	
	char PORT_CDECL TestType(...) { return 0; }
	template<class T1>
		int PORT_CDECL TestType( std::vector<T1>* ) { return 0; }
	int PORT_CDECL TestType( std::string* ) { return 0; }
	int PORT_CDECL TestType( std::wstring* ) { return 0; }

	template<class T>
	void DataCheckSum( T *p, const int nLen )
	{
		dwCheckSum = CalcCheckSum( dwCheckSum, (const uint8_t*)p, nLen );
	}

	template<class T>
	void PORT_CDECL SeparateCheckSum( const T &data, SInt2Type<1> *pp )
	{
		DataCheckSum( &data, sizeof( T ) );
	}

	template<class T>
	void PORT_CDECL SeparateCheckSum( const T &data, SInt2Type<4> *pp )
	{
		uint32_t dataCheckSum = data.CalcCheckSum();
		dataCheckSum = CalcCheckSum( dwCheckSum, (const uint8_t*)&dataCheckSum, sizeof(uint32_t) );
	}
public:
	CCheckSum() : dwCheckSum( GetDefaultCheckSum() ) { }

	template<class T>
	CCheckSum& operator<<( const T &data )
	{
		// const_cast is ok, needed only to determine the type of T
		T &non_const_data = const_cast<T&>( data );
		const int N_HAS_SERIALIZE_TEST = sizeof( non_const_data&(*p) );
		SInt2Type<N_HAS_SERIALIZE_TEST> separator;
		SeparateCheckSum( data, &separator );
		return *this;
	}
	
	template<class T>
	CCheckSum& operator<<( const std::vector<T> &vec )
	{
		const int nSize = vec.size();
		DataCheckSum( &nSize, sizeof( nSize ) );
		if ( nSize == 0 )
			return *this;

		// const_cast is ok, needed only to determine the type of T
		T &el = const_cast<T&>(vec[0]);
		if ( sizeof( TestType( &vec[0] ) ) == 1 && sizeof( el&(*p) ) == 1 )
			DataCheckSum( &(vec[0]), vec.size() * sizeof(T) );
		else
		{
			for ( std::vector<T>::const_iterator iter = vec.begin(); iter != vec.end(); ++iter )
				(*this) << *iter;
		}

		return *this;
	}

	CCheckSum& operator<<( const std::string &sz )
	{
		const int nSize = sz.size();
		DataCheckSum( &nSize, sizeof(nSize) );
		if ( nSize != 0 )
			DataCheckSum( &(sz[0]), sz.size() * sizeof(sz[0]) );

		return *this;
	}

	CCheckSum& operator<<( const std::wstring &wsz )
	{
		const int nSize = wsz.size();
		DataCheckSum( &nSize, sizeof( nSize ) );
		if ( nSize != 0 )
			DataCheckSum( &(wsz[0]), wsz.size() * sizeof(wsz[0]) );

		return *this;
	}

	uint32_t GetCheckSum() const { return dwCheckSum; }
};

}

template <class TUserObj, typename TPtr>
uint32_t CDBPtr<TUserObj,TPtr>::CalcCheckSum() const
{
	if ( pObj == 0 )
		return GetDefaultCheckSum();
	else
		return GetBarePtr()->CalcCheckSum();
}


