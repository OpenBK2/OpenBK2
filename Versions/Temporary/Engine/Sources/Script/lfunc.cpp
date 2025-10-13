#include "stdafx.h"
/*
** $Id: lfunc.c,v 1.34 2000/10/30 12:20:29 roberto Exp $
** Auxiliary functions to manipulate prototypes and closures
** See Copyright Notice in lua.h
*/


#include <cstdlib>


#include "lfunc.h"
#include "lstate.h"


#define sizeclosure(n)	((int)sizeof(Closure) + (int)sizeof(TObject)*((n)-1))


int luaF_newclosure (lua_State *L, int nelems) 
{
  Closure *c = new Closure;
  c->mark = c;
  c->upvalues.resize( nelems, luaO_nilobject );
  return L->closures.push( c );
}


int luaF_newproto (lua_State *L) 
{
  Proto *f = new Proto;
	return L->protos.push( f );
}



/*
** Look for n-th local variable at line `line' in function `func'.
** Returns NULL if not found.
*/
const char *luaF_getlocalname (const Proto *f, int local_number, int pc) {
  int i;
  for (i = 0; i<f->locvars.size() && f->locvars[i].startpc <= pc; i++) {
    if (pc < f->locvars[i].endpc) {  /* is variable active? */
      local_number--;
      if (local_number == 0)
        return f->locvars[i].varname->c_str();
    }
  }
  return NULL;  /* not found */
}


