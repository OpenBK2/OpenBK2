#pragma once
/*
** $Id: lgc.h,v 1.8 2000/10/02 14:47:43 roberto Exp $
** Garbage Collector
** See Copyright Notice in lua.h
*/



#include "lobject.h"


void luaC_collect (lua_State *L, int all);
void luaC_checkGC (lua_State *L);



