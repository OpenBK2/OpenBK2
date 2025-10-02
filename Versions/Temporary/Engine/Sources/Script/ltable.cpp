#include "stdafx.h"
/*
** $Id: ltable.c,v 1.58 2000/10/26 12:47:05 roberto Exp $
** Lua tables (hash)
** See Copyright Notice in lua.h
*/


/*
** Implementation of tables (aka arrays, objects, or hash tables);
** uses a mix of chained scatter table with Brent's variation.
** A main invariant of these tables is that, if an element is not
** in its main position (i.e. the `original' position that its hash gives
** to it), then the colliding element is in its own main position.
** In other words, there are collisions only when two elements have the
** same main position (i.e. the same hash values for that table size).
** Because of that, the load factor of these tables can be 100% without
** performance penalties.
*/



#include "lstring.h"
#include "ltable.h"

/*
** returns the `main' position of an element in a table (that is, the index
** of its hash value)
*/
/*Node *luaH_mainposition (const Hash *t, const TObject *key) {
  unsigned long h;
  switch ( key->GetType() ) 
	{
    case LUA_TNUMBER:
      h = (unsigned long)(long)( key->GetN() );
      break;
    case LUA_TSTRING:
      h = IntPoint( key->GetS() );
      break;
    case LUA_TUSERDATA:
      h = IntPoint( key->GetS() );
      break;
    case LUA_TTABLE:
      h = IntPoint( key->GetH() );
      break;
    case LUA_TFUNCTION:
      h = IntPoint( key->GetCL() );
      break;
    default:
			ASSERT( 0 );
      return NULL;  // invalid key 
  }
  LUA_ASSERT(h%(unsigned int)t->size == (h&((unsigned int)t->size-1)),
            "a&(x-1) == a%x, for x power of 2");
  return &t->node[h&(t->size-1)];
}*/

int luaH_new (lua_State *L, int size) {
  Hash *t = new Hash;
  return L->tables.push( t );
}

const TObject *luaH_getglobal (lua_State *L, const char *name) {
	Hash *gt = L->tables[ L->nGT ];
  return gt->GetStr( luaS_new(L, name) );
}


