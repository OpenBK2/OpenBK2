#include "stdafx.h"
#include "lsaver.h"
#include "lstring.h"

typedef hash_map<CLuaFuncID, int> CLuaIDToFuncMap;
typedef hash_map<int, CLuaFuncID> CLuaFuncToIDMap;
CLuaFuncToIDMap luaFuncToIDMap;
CLuaIDToFuncMap luaIDToFuncMap;
lua_State *pLUASaverState;

void lua_StartSerialize( lua_State *pL )
{ 
	lua_RegisterFunc( (lua_CFunction)0, "NOFUNC" );
	lua_RegisterFunc( (lua_Hook)0, "NOFUNC" );
	pLUASaverState = pL;
}

static void lua_GetID( CLuaFuncID *pID, lua_CFunction func )
{
	// Здесь можно добавить выбор имени с правильным префиксом
	// экземпляра lua-машины (pLUASaverState->szName)
	int nFunc = reinterpret_cast<int>( func );
	CLuaFuncToIDMap::iterator i = luaFuncToIDMap.find( nFunc );
	ASSERT( i != luaFuncToIDMap.end() );  // unregistered lua C function!
	*pID = i->second;
}

static void lua_GetID( CLuaFuncID *pID, lua_Hook func )
{
	// Здесь можно добавить выбор имени с правильным префиксом
	// экземпляра lua-машины (pLUASaverState->szName)
	int nFunc = reinterpret_cast<int>( func );
	CLuaFuncToIDMap::iterator i = luaFuncToIDMap.find( nFunc );
	ASSERT( i != luaFuncToIDMap.end() );  // unregistered lua C function!
	*pID = i->second;
}

static lua_CFunction lua_GetFunc( const CLuaFuncID& id )
{ 
	CLuaIDToFuncMap::iterator i = luaIDToFuncMap.find( id );
	if ( i == luaIDToFuncMap.end() )
	{
		ASSERT(0); // unregistered lua C function!
		return 0;
	}
	int nFunc = i->second;
	return ( lua_CFunction )nFunc;
}

static lua_Hook lua_GetHook( const CLuaFuncID& id )
{
	CLuaIDToFuncMap::iterator i = luaIDToFuncMap.find( id );
	ASSERT( i != luaIDToFuncMap.end() );  // unregistered lua C function!
	int nFunc = i->second;
	return ( lua_Hook )nFunc;
}

void lua_RegisterFunc( lua_CFunction func, const CLuaFuncID& id )
{
	int nFunc = reinterpret_cast<int>( func );
	luaFuncToIDMap[ nFunc ] = id;
	luaIDToFuncMap[ id ] = nFunc;
}

void lua_RegisterFunc( lua_Hook func, const CLuaFuncID& id )
{
	int nFunc = reinterpret_cast<int>( func );
	luaFuncToIDMap[ nFunc ] = id;
	luaIDToFuncMap[ id ] = nFunc;
}

int lua_AddCFunc( lua_CFunction *pFunc, IBinSaver &f, int nChunk )
{
	CLuaFuncID id;
	if ( f.IsReading() )
	{
		f.Add( nChunk, &id );
		*pFunc = lua_GetFunc( id );
	}
	else
	{
		lua_GetID( &id, *pFunc );
		f.Add( nChunk, &id );
	}
	return nChunk + 1;
}

int lua_AddHook( lua_Hook *pHook, IBinSaver &f, int nChunk )
{
	CLuaFuncID id;
	if ( f.IsReading() )
	{
		f.Add( nChunk, &id );
		*pHook = lua_GetHook( id );
	}
	else
	{
		lua_GetID( &id, *pHook );
		f.Add( nChunk, &id );
	}
	return nChunk + 1;
}

void lua_AddString( IBinSaver &f, IBinSaver::chunk_id idChunk, TString **ppszStr, int nChunk )
{
	if ( f.IsReading() )
	{
		string str;
		f.Add( idChunk, &str, nChunk );
		*ppszStr = luaS_new( pLUASaverState, str.c_str() );
	}
	else
	{
		string str = (*ppszStr)->GetStr();
		f.Add( idChunk, &str, nChunk );
	}
}

