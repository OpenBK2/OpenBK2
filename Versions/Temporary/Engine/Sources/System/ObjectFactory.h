#pragma once
#include "Misc/HashFuncs.h"


// factory is using RTTI
// objects should inherit T and T must have at least 1 virtual function
template <class T>
class CClassFactory
{
public:
	typedef const type_info *VFT;
private:
	typedef T* (*newFunc)();
	typedef std::unordered_map<int, newFunc> CTypeNewHash;                // typeID->newFunc()
	typedef std::unordered_map<VFT, int> CTypeIndexHash; // vftable->typeID

	CTypeIndexHash typeIndex;
	CTypeNewHash typeInfo;

	static VFT GetObjectType( T *pObject )
	{ 
		NI_ASSERT( pObject != 0, "null object passed" );
		if ( pObject == 0 )
			return 0;

		return &typeid(*pObject);
	}

	int VFT2TypeID( VFT t )
	{
		CTypeIndexHash::iterator i = typeIndex.find( t );
		if ( i != typeIndex.end() )
			return i->second;
		for ( i = typeIndex.begin(); i != typeIndex.end(); ++i )
		{
			if ( *i->first == *t )
			{
				typeIndex[t] = i->second;
				return i->second;
			}
		}
		return -1;
	}
public:
	void RegisterTypeBase( int nTypeID, newFunc func, VFT vft );
	void UnregisterTypeBase( int nTypeID, VFT vft )
	{
		typeInfo.erase( nTypeID );
		std::vector<VFT> toErase;
		for ( CTypeIndexHash::iterator i = typeIndex.begin(); i != typeIndex.end(); ++i )
		{
			if ( *i->first == *vft )
				toErase.push_back( i->first );
		}
		for ( int k = 0; k < toErase.size(); ++k )
			typeIndex.erase( toErase[k] );
	}
	template < class TT >
		void RegisterType( int nTypeID, newFunc func, TT* ) { RegisterTypeBase( nTypeID, func, &typeid(TT) ); }
	void RegisterTypeSafe( int nTypeID, newFunc func ) 
	{
		CPtr<T> pObj = func();
		VFT vft = GetObjectType( pObj );
		RegisterTypeBase( nTypeID, func, vft ); 
	}
	T* CreateObject( int nTypeID ) 
	{ 
		typename CTypeNewHash::iterator i = typeInfo.find( nTypeID );
		if ( i == typeInfo.end() )
			return 0;
		newFunc f = i->second;
		if ( f )
			return f(); 
		return 0; 
	}
	bool IsRegistered( int nTypeID ) const { return typeInfo.find( nTypeID ) != typeInfo.end(); }
	int GetObjectTypeID( T *pObject ) { return VFT2TypeID( GetObjectType( pObject ) ); }
	int GetObjectTypeID( VFT vft ) { return VFT2TypeID( vft ); }
	template<class TT>
		int GetTypeID( TT *p = 0 ) { return VFT2TypeID( &typeid(TT) ); }
};

template <class T>
void CClassFactory<T>::RegisterTypeBase( int nTypeID, newFunc func, VFT vft )
{
	ASSERT( typeInfo.find( nTypeID ) == typeInfo.end() );
	ASSERT( typeIndex.find( vft ) == typeIndex.end() );
	typeIndex[ vft ] = nTypeID;
	typeInfo[ nTypeID ] = func;
}


