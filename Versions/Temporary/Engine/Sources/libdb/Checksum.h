#pragma once

#include "libdb_export.h"


namespace NDb
{
LIBDB_EXPORT DWORD GetDefaultCheckSum();
LIBDB_EXPORT DWORD CalcCheckSum( const DWORD dwLastCheckSum, const BYTE *pBuf, const int nLen );

class CCheckSum
{
	DWORD dwCheckSum;
	IBinSaver *p;
	
	char __cdecl TestType(...) { return 0; }	
	template<class T1>
		int __cdecl TestType( vector<T1>* ) { return 0; }
	int __cdecl TestType( string* ) { return 0; }
	int __cdecl TestType( wstring* ) { return 0; }

	template<class T>
	void DataCheckSum( T *p, const int nLen )
	{
		dwCheckSum = CalcCheckSum( dwCheckSum, (const BYTE*)p, nLen );
	}

	template<class T>
	void __cdecl SeparateCheckSum( const T &data, SInt2Type<1> *pp )
	{
		DataCheckSum( &data, sizeof( T ) );
	}

	template<class T>
	void __cdecl SeparateCheckSum( const T &data, SInt2Type<4> *pp )
	{
		DWORD dataCheckSum = data.CalcCheckSum();
		dataCheckSum = CalcCheckSum( dwCheckSum, (const BYTE*)&dataCheckSum, sizeof(DWORD) );
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
	CCheckSum& operator<<( const vector<T> &vec )
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
			for ( vector<T>::const_iterator iter = vec.begin(); iter != vec.end(); ++iter )
				(*this) << *iter;
		}

		return *this;
	}

	CCheckSum& operator<<( const string &sz )
	{
		const int nSize = sz.size();
		DataCheckSum( &nSize, sizeof(nSize) );
		if ( nSize != 0 )
			DataCheckSum( &(sz[0]), sz.size() * sizeof(sz[0]) );

		return *this;
	}

	CCheckSum& operator<<( const wstring &wsz ) 
	{
		const int nSize = wsz.size();
		DataCheckSum( &nSize, sizeof( nSize ) );
		if ( nSize != 0 )
			DataCheckSum( &(wsz[0]), wsz.size() * sizeof(wsz[0]) );

		return *this;
	}

	DWORD GetCheckSum() const { return dwCheckSum; }
};

}

template <class TUserObj, typename TPtr>
DWORD CDBPtr<TUserObj,TPtr>::CalcCheckSum() const
{
	if ( pObj == 0 )
		return GetDefaultCheckSum();
	else
		return GetBarePtr()->CalcCheckSum();
}

