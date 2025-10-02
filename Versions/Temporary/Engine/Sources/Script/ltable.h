#pragma once
/*
** $Id: ltable.h,v 1.24 2000/08/31 14:08:27 roberto Exp $
** Lua tables (hash)
** See Copyright Notice in lua.h
*/


#include "lobject.h"

int luaH_new (lua_State *L, int nhash);
const TObject *luaH_getglobal (lua_State *L, const char *name);


