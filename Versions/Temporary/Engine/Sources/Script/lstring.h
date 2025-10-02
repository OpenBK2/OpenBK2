/*
** $Id: lstring.h,v 1.24 2000/10/30 17:49:19 roberto Exp $
** String table (keep all strings handled by Lua)
** See Copyright Notice in lua.h
*/

#ifndef lstring_h
#define lstring_h


#include "lstate.h"


/*
** any TString with mark>=FIXMARK is never collected.
** Marks>=RESERVEDMARK are used to identify reserved words.
*/
#define FIXMARK		2
#define RESERVEDMARK	3


#define sizestring(l)	((long)sizeof(TString) + \
                         ((long)(l+1)-TSPACK)*(long)sizeof(char))


int luaS_createudata (lua_State *L, CObjectBase *pData, int tag);
TString *luaS_newlstr (lua_State *L, const char *str);
TString *luaS_new (lua_State *L, const char *str);
TString *luaS_newfixed (lua_State *L, const char *str);


#endif

