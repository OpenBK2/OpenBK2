#pragma once
/*
** $Id: ldebug.h,v 1.7 2000/10/05 12:14:08 roberto Exp $
** Auxiliary functions from Debug Interface module
** See Copyright Notice in lua.h
*/



#include "luadebug.h"


void luaG_typeerror (lua_State *L, TObject *o, const char *op); 
void luaG_binerror (lua_State *L, TObject *p1, int t, const char *op);
//int luaG_getline (const vector<int> &lineinfo, int pc, int refline, int *refi);
void luaG_ordererror (lua_State *L, TObject *top);



