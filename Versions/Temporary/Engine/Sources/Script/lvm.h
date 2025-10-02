#pragma once
/*
** $Id: lvm.h,v 1.27 2000/10/05 12:14:08 roberto Exp $
** Lua virtual machine
** See Copyright Notice in lua.h
*/



#include "ltm.h"


#define tonumber(o)   ( ( (o)->GetType() != LUA_TNUMBER ) && (luaV_tonumber(o) != 0 ) )
#define tostring(L,o) ( ( (o)->GetType() != LUA_TSTRING ) && (luaV_tostring(L, o) != 0 ) )


int luaV_tonumber (TObject *obj);
int luaV_tostring (lua_State *L, TObject *obj);
const TObject *luaV_gettable (lua_State *L, StkId t);
void luaV_settable (lua_State *L, StkId t, StkId key);
const TObject *luaV_getglobal (lua_State *L, TString *s);
void luaV_setglobal (lua_State *L, TString *s);
StkId luaV_execute (lua_State *L, const Closure *cl, StkId base);
void luaV_Cclosure (lua_State *L, lua_CFunction c, int nelems);
void luaV_Lclosure (lua_State *L, int nProto, int nelems);
int luaV_lessthan (lua_State *L, StkId stL, StkId stR, StkId top);
void luaV_strconc (lua_State *L, int total, StkId top);

/// Step-by-step execution
void luaV_beginExecute(lua_State *L, int nClosure, StkId base, int nResults );
void luaV_stepExecute( lua_State *L );


