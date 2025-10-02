#include "stdafx.h"
//
#include "scriptPtr.h"
#include "scriptCommon.h"
//
namespace NScript
{

int luaGarbageCollector( lua_State* pState );

void luaPushCPtr( lua_State* pState, CObjectBase *pObj )
{
	Script script( pState );
	if ( !IsValid( pObj ) )
		script.PushNil();
	else
		script.PushUserTag( pObj, tagLuaCPtr );
}

void luaPushCObj( lua_State* pState, CObjectBase *pObj )
{
	Script script( pState );
	if ( !IsValid( pObj ) )
		script.PushNil();
	else
		script.PushUserTag( pObj, tagLuaCObj );
}

CObjectBase* luaGetPtr( const Script::Object &o )
{
	if ( !o.IsUserData() )
		return 0;
	if ( o.Tag() != tagLuaCPtr && o.Tag() != tagLuaCObj )
		return 0;
	return o.GetUserData();
}

int luaMakeCPtr( lua_State* pState )
{
	Script script( pState );
	Script::Object o = script.GetTopObject();
	if ( o.Tag() != tagLuaCPtr && o.Tag() != tagLuaCObj )
		return 0;
	//
	luaPushCPtr( pState, luaGetPtr( o ) );
	return 1;
}

int luaMakeCObj( lua_State* pState )
{
	Script script( pState );
	Script::Object o = script.GetTopObject();
	if ( o.Tag() != tagLuaCPtr && o.Tag() != tagLuaCObj )
		return 0;
	//
	luaPushCObj( pState, luaGetPtr( o ) );
	return 1;
}

bool luaIsPtr( const Script::Object &o )
{
	return o.IsUserData() && ( o.Tag() == tagLuaCPtr || o.Tag() == tagLuaCObj );
}

bool luaIsValidPtr( const Script::Object &o )
{
	return o.IsUserData() && IsValid( luaGetPtr( o ) );
}

int luaIsValid( lua_State* pState )
{
	Script script( pState );
	luaPushBool( pState, luaIsValidPtr( script.GetObject( 1 ) ) );
	return 1;
}

}
using namespace NScript;
//
//REGISTER_SAVELOAD_TEMPL_CLASS( 0x11061440, CDBPtrWrapper<NDb::CDBCamera> ,CDBPtrWrapper )
