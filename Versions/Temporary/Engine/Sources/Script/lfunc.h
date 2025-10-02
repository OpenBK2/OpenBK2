/*
** $Id: lfunc.h,v 1.13 2000/09/29 12:42:13 roberto Exp $
** Auxiliary functions to manipulate prototypes and closures
** See Copyright Notice in lua.h
*/

#ifndef lfunc_h
#define lfunc_h


#include "lobject.h"



int luaF_newproto (lua_State *L);
int luaF_newclosure (lua_State *L, int nelems);

const char *luaF_getlocalname (const Proto *func, int local_number, int pc);


#endif

