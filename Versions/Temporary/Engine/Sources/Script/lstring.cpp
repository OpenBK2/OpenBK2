#include "stdafx.h"
/*
** $Id: lstring.c,v 1.45 2000/10/30 17:49:19 roberto Exp $
** String table (keeps all strings handled by Lua)
** See Copyright Notice in lua.h
*/


#include <string.h>


#include "lstring.h"
#include "ltm.h"

TString *luaS_newlstr (lua_State *L, const char *str) 
{
	TString res( str );
	CLuaStrings::iterator it = L->strings.insert( std::pair<TString,bool>( res, false ) ).first;
	return const_cast<TString*>( &(it->first) );
}


static int luaS_newudata (lua_State *L, CObjectBase *pData, int tag ) 
{
	UserData *pRes = new UserData;
	pRes->marked = 0;
	switch ( tag ) 
	{
		case tagLuaCPtr:
			pRes->pPtr = pData;
			break;
		case tagLuaCObj:
			pRes->pObj = pData;
			break;
		default:
			ASSERT(0);
			break;
	}
	ASSERT( tag >= 0 && tag < L->TMtable.size() );
	pRes->tag = tag;
	return L->userdatas.push( pRes );
}


int luaS_createudata (lua_State *L, CObjectBase *pData, int tag) 
{
	ASSERT( IsValid(pData) );
	for ( int it = 0; it < L->userdatas.size(); ++it )
	{
		UserData *p = L->userdatas[it];
		if ( !p )
			continue;
		if ( ( p->tag == tag || tag == LUA_ANYTAG ) && ( p->pPtr == pData || p->pObj == pData ) )
			return it;
	}
	if ( tag == LUA_ANYTAG )
		tag = 0;
	return luaS_newudata( L, pData, tag );
}


TString *luaS_new (lua_State *L, const char *str) {
  return luaS_newlstr(L, str);
}


TString *luaS_newfixed (lua_State *L, const char *str) {
  TString *ts = luaS_new(L, str);
  if (ts->marked == 0) ts->marked = FIXMARK;  /* avoid GC */
  return ts;
}


