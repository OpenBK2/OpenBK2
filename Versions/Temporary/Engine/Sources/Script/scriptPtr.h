#pragma once

#include "script.h"
//
//class CDBRecord;
//
namespace NScript
{
/*
	////////////////////////////////////////////////////////////////////////////////////////////////////
// CDBPtrWrapper

template< class T >
class CDBPtrWrapper: public CObjectBase
{
	OBJECT_BASIC_METHODS( CDBPtrWrapper );
	ZDATA
public:
	CDBPtr<T> pRecord;
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&pRecord); return 0; }
	//
	CDBPtrWrapper() {}
	CDBPtrWrapper( T *_pRecord ): pRecord( _pRecord ) {}
};*/

void luaPushCPtr( lua_State* pState, CObjectBase *pObj );
void luaPushCObj( lua_State* pState, CObjectBase *pObj );
CObjectBase* luaGetPtr( const Script::Object &o );
int luaMakeCPtr( lua_State* pState );
int luaMakeCObj( lua_State* pState );
bool luaIsDBPtr( const Script::Object &o );
bool luaIsPtr( const Script::Object &o );
bool luaIsValidPtr( const Script::Object &o );
int luaIsValid( lua_State* pState );
//
/*
template < class T > T* luaGetDBPtr( const Script::Object &o )
{
	if ( CDynamicCast< CDBPtrWrapper<T> > pWrapper = luaGetPtr( o ) )
		return pWrapper->pRecord;
	else
		return 0;
}
//
template < class T > void luaPushCDBPtr( lua_State* pState, T *pObj )
{
	Script script( pState );
	if ( !IsValid( pObj ) )
		script.PushNil();
	else
		luaPushCObj( pState, new CDBPtrWrapper<T>( pObj ) );
}
//

template < class T > bool luaIsDBPtr( const Script::Object &o )
{
	if ( CDynamicCast< CDBPtrWrapper<T> > pDBPtr = luaGetPtr( o ) )
		return true;
	else
		return false;
}*/

}
//

