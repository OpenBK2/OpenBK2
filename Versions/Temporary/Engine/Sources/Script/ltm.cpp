#include "stdafx.h"
/*
** $Id: ltm.c,v 1.56 2000/10/31 13:10:24 roberto Exp $
** Tag methods
** See Copyright Notice in lua.h
*/


#include <stdio.h>
#include <string.h>


#include "ldo.h"
#include "ltm.h"


const char *const luaT_eventname[] = {  /* ORDER TM */
  "gettable", "settable", "index", "getglobal", "setglobal", "add", "sub",
  "mul", "div", "pow", "unm", "lt", "concat", "gc", "function",
  "le", "gt", "ge",  /* deprecated options!! */
  NULL
};


static int findevent (const char *name) {
  int i;
  for (i=0; luaT_eventname[i]; i++)
    if (strcmp(luaT_eventname[i], name) == 0)
      return i;
  return -1;  /* name not found */
}


static int luaI_checkevent (lua_State *L, const char *name, int t) {
  int e = findevent(name);
  if (e >= TM_N)
    luaO_verror(L, "event `%.50s' is deprecated", name);
  if (e == TM_GC && t == LUA_TTABLE)
    luaO_verror(L, "event `gc' for tables is deprecated");
  if (e < 0)
    luaO_verror(L, "`%.50s' is not a valid event name", name);
  return e;
}



/* events in LUA_TNIL are all allowed, since this is used as a
*  'placeholder' for "default" fallbacks
*/
/* ORDER LUA_T, ORDER TM */
static const char luaT_validevents[NUM_TAGS][TM_N] = {
  {1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},  /* LUA_TUSERDATA */
  {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},  /* LUA_TNIL */
  {1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1},  /* LUA_TNUMBER */
  {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},  /* LUA_TSTRING */
  {0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},  /* LUA_TTABLE */
  {1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0}   /* LUA_TFUNCTION */
};

int luaT_validevent (int t, int e) {  /* ORDER LUA_T */
  return (t >= NUM_TAGS) ?  1 : luaT_validevents[t][e];
}


static void init_entry (lua_State *L, int tag) {
  int i;
  for (i=0; i<TM_N; i++)
    luaT_gettm(L, tag, i) = STK_NULL;
}


void luaT_init( lua_State *L ) 
{
	L->TMtable.resize( NUM_TAGS );
  for (int t=0; t<L->TMtable.size(); ++t)
    init_entry(L, t);
}


int lua_newtag (lua_State *L) {
	L->TMtable.push_back( TM() );
  init_entry(L, L->TMtable.size() - 1 );
  return L->TMtable.size() - 1;
}


static void checktag (lua_State *L, int tag) {
  if ( tag < 0 || tag >= L->TMtable.size() )
    luaO_verror(L, "%d is not a valid tag", tag);
}

void luaT_realtag (lua_State *L, int tag) {
	if ( !IsValidTag( L, tag ) )
    luaO_verror(L, "tag %d was not created by `newtag'", tag);
}


int lua_copytagmethods (lua_State *L, int tagto, int tagfrom) {
  int e;
  checktag(L, tagto);
  checktag(L, tagfrom);
  for (e=0; e<TM_N; e++) {
    if (luaT_validevent(tagto, e))
      luaT_gettm(L, tagto, e) = luaT_gettm(L, tagfrom, e);
  }
  return tagto;
}


int luaT_tag (lua_State *L, const TObject *o) 
{
  int t = o->GetType();
  switch (t) {
    case LUA_TUSERDATA: return L->userdatas[ o->GetUData() ]->tag;
    case LUA_TTABLE:    return L->tables[ o->GetH() ]->htag;
    default:            return t;
  }
}


void lua_gettagmethod (lua_State *L, int t, const char *event) 
{
  int e;
  e = luaI_checkevent(L, event, t);
  checktag(L, t);
	TObject *pTop = LObj( L, L->pCT->top );
  if ( luaT_validevent(t, e) && luaT_gettm(L, t, e) ) 
    pTop->SetCL( luaT_gettm(L, t, e) );
  else
    pTop->SetNil();
  incr_top;
}


void lua_settagmethod (lua_State *L, int t, const char *event) {
  int e = luaI_checkevent(L, event, t);
  checktag(L, t);
  if (!luaT_validevent(t, e))
    luaO_verror(L, "cannot change `%.20s' tag method for type `%.20s'%.20s",
                luaT_eventname[e], luaO_typenames[t],
                (t == LUA_TTABLE || t == LUA_TUSERDATA) ?
                   " with default tag" : "");
	TObject *pPrTop = LObj(L, L->pCT->top - 1);
  switch ( pPrTop->GetType() ) 
	{
    case LUA_TNIL:
      luaT_gettm(L, t, e) = STK_NULL;
      break;
    case LUA_TFUNCTION:
      luaT_gettm(L, t, e) = pPrTop->GetCL();
      break;
    default:
      lua_error(L, "tag method must be a function (or nil)");
  }
  L->pCT->top--;
}


