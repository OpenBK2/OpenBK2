#include "stdafx.h"
/*
** $Id: lapi.c,v 1.110 2000/10/30 12:50:09 roberto Exp $
** Lua API
** See Copyright Notice in lua.h
*/


#include <string.h>


#include "lapi.h"
#include "ldo.h"
#include "lgc.h"
#include "lstring.h"
#include "ltable.h"
#include "lvm.h"


const char lua_ident[] = "$Lua: " LUA_VERSION " " LUA_COPYRIGHT " $\n"
                               "$Authors: " LUA_AUTHORS " $";



#define Index(L,i)	((i) >= 0 ? (L->pCT->Cbase+((i)-1)) : (L->pCT->top+(i)))

#define api_incr_top(L)	incr_top

StkId luaA_index (lua_State *L, int index) {
  return Index(L, index);
}


StkId luaA_indexAcceptable (lua_State *L, int index) {
  if (index >= 0) {
    StkId o = L->pCT->Cbase+(index-1);
    if (o >= L->pCT->top) return STK_NULL;
    else return o;
  }
  else return L->pCT->top+index;
}


void luaA_pushobject (lua_State *L, const TObject *o) {
  *LObj(L, L->pCT->top) = *o;
  incr_top;
}

/*
** basic stack manipulation
*/


int lua_gettop (lua_State *L) {
  return (L->pCT->top - L->pCT->Cbase);
}


void lua_settop (lua_State *L, int index) {
  if (index >= 0)
    luaD_adjusttop(L, L->pCT->Cbase, index);
  else
    L->pCT->top = L->pCT->top+index+1;  /* index is negative */
}


void lua_remove (lua_State *L, int index) {
  StkId p = luaA_index(L, index);
  while (++p < L->pCT->top) *LObj(L, p-1) = *LObj(L, p);
  L->pCT->top--;
}


void lua_insert (lua_State *L, int index) {
  StkId p = luaA_index(L, index);
  StkId q;
  for (q = L->pCT->top; q>p; q--)
    *LObj(L, q) = *LObj(L, q-1);
  *LObj(L, p) = *LObj(L, L->pCT->top);
}


void lua_pushvalue (lua_State *L, int index) {
  *LObj(L, L->pCT->top) = *LObj(L, luaA_index(L, index));
  api_incr_top(L);
}



/*
** access functions (stack -> C)
*/


int lua_type (lua_State *L, int index) {
  StkId o = luaA_indexAcceptable(L, index);
  return (o == STK_NULL) ? LUA_TNONE : LObj(L, o)->GetType();
}

const char *lua_typename (lua_State *L, int t) {
  UNUSED(L);
  return (t == LUA_TNONE) ? "no value" : luaO_typenames[t];
}


int lua_iscfunction (lua_State *L, int index) {
  StkId o = luaA_indexAcceptable(L, index);
  return (o == STK_NULL) ? 0 : iscfunction( L, LObj(L, o) );
}

int lua_isnumber (lua_State *L, int index) {
  StkId o = luaA_indexAcceptable(L, index);
  return (o == STK_NULL) ? 0 : (tonumber( LObj(L, o) ) == 0);
}

int lua_isnumber_only(lua_State *L, int index) {
	StkId o = luaA_indexAcceptable(L, index);
	return (o == STK_NULL) ? 0 : LUA_TNUMBER == lua_type(L, index);
}

int lua_isstring (lua_State *L, int index) {
  int t = lua_type(L, index);
  return (t == LUA_TSTRING || t == LUA_TNUMBER);
}


int lua_tag (lua_State *L, int index) {
  StkId o = luaA_indexAcceptable(L, index);
  return (o == STK_NULL) ? LUA_NOTAG : luaT_tag( L, LObj(L, o) );
}

int lua_equal (lua_State *L, int index1, int index2) {
  StkId o1 = luaA_indexAcceptable(L, index1);
  StkId o2 = luaA_indexAcceptable(L, index2);
  if (o1 == STK_NULL || o2 == STK_NULL) 
		return 0;  /* index out-of-range */
  else return luaO_equalObj( LObj(L, o1), LObj(L, o2) );
}

int lua_lessthan (lua_State *L, int index1, int index2) {
  StkId o1 = luaA_indexAcceptable(L, index1);
  StkId o2 = luaA_indexAcceptable(L, index2);
  if (o1 == STK_NULL || o2 == STK_NULL) 
		return 0;  /* index out-of-range */
  else return luaV_lessthan(L, o1, o2, L->pCT->top);
}



double lua_tonumber (lua_State *L, int index) 
{
  StkId o = luaA_indexAcceptable(L, index);
  return (o == STK_NULL || tonumber( LObj(L, o) )) ? 0 : LObj(L, o)->GetN();
}

const char *lua_tostring (lua_State *L, int index) 
{
  StkId o = luaA_indexAcceptable(L, index);
  return (o == STK_NULL || tostring(L, LObj(L, o))) ? NULL : LObj(L, o)->GetS()->c_str();
}

size_t lua_strlen (lua_State *L, int index) {
  StkId o = luaA_indexAcceptable(L, index);
  return (o == STK_NULL || tostring(L, LObj(L, o))) ? 0 : LObj(L, o)->GetS()->GetStr().size();
}

lua_CFunction lua_tocfunction (lua_State *L, int index) 
{
  StkId o = luaA_indexAcceptable(L, index);
  return ( o == STK_NULL || !iscfunction( L, LObj(L, o) ) ) ? NULL : L->GetClosure(o)->f.c;
}

CObjectBase *lua_touserdata (lua_State *L, int index) 
{
  StkId o = luaA_indexAcceptable(L, index);
	if ( o == STK_NULL || LObj(L, o)->GetType() != LUA_TUSERDATA )
		return NULL;
  UserData *pUD = L->GetUData(o);
	return pUD->tag == tagLuaCPtr ? pUD->pPtr.GetPtr() : pUD->pObj.GetPtr();
}

const void *lua_topointer( lua_State *L, int index ) 
{
  StkId o = luaA_indexAcceptable( L, index );
  if ( o == STK_NULL ) 
		return NULL;
  switch ( LObj(L, o)->GetType() ) 
	{
    case LUA_TTABLE: 
      return L->GetHash(o);
    case LUA_TFUNCTION:
      return L->GetClosure(o);
    default: 
			return NULL;
  }
}



/*
** push functions (C -> stack)
*/


void lua_pushnil (lua_State *L) {
  LObj(L, L->pCT->top)->SetNil();
  api_incr_top(L);
}


void lua_pushnumber (lua_State *L, double n) {
  LObj(L, L->pCT->top)->SetN( n );
  api_incr_top(L);
}


void lua_pushlstring (lua_State *L, const char *s, size_t len) {
	std::string szSemi( s, len );
  LObj(L, L->pCT->top)->SetS( luaS_newlstr(L, szSemi.c_str()) );
  api_incr_top(L);
}


void lua_pushstring (lua_State *L, const char *s) {
  if (s == NULL)
    lua_pushnil(L);
  else
    lua_pushlstring(L, s, strlen(s));
}


void lua_pushcclosure (lua_State *L, lua_CFunction fn, int n) {
  return luaV_Cclosure(L, fn, n);
}


void lua_pushusertag (lua_State *L, CObjectBase *u, int tag) {
  /* ORDER LUA_T */
  if ( tag != LUA_ANYTAG && tag != LUA_TUSERDATA && !IsValidTag( L, tag ) )
    luaO_verror(L, "invalid tag for a userdata (%d)", tag);
  LObj(L, L->pCT->top)->SetUData( luaS_createudata(L, u, tag) );
  api_incr_top(L);
}



/*
** get functions (Lua -> stack)
*/


void lua_getglobal (lua_State *L, const char *name) {
  StkId top = L->pCT->top;
  *LObj(L, top) = *luaV_getglobal(L, luaS_new(L, name));
  L->pCT->top = top;
  api_incr_top(L);
}


void lua_gettable (lua_State *L, int index) {
  StkId t = Index(L, index);
  StkId top = L->pCT->top;
  *LObj(L, top-1) = *luaV_gettable(L, t);
  L->pCT->top = top;  /* tag method may change top */
}


void lua_rawget( lua_State *L, int index ) 
{
  StkId t = Index(L, index);
  LUA_ASSERT( LObj(L, t)->GetType() == LUA_TTABLE, "table expected" );
	TObject *pPrTop = LObj(L, L->pCT->top - 1);
	Hash *pH = L->GetHash(t);
	*pPrTop = *( pH->GetObj( pPrTop ) );
}


void lua_rawgeti (lua_State *L, int index, int n) {
  StkId o = Index(L, index);
  LUA_ASSERT( LObj(L, o)->GetType() == LUA_TTABLE, "table expected");
  *LObj(L, L->pCT->top) = *L->GetHash( o )->GetNum( n );
  api_incr_top(L);
}


void lua_getglobals (lua_State *L) 
{
  LObj(L, L->pCT->top)->SetH( L->nGT );
  api_incr_top(L);
}


int lua_getref (lua_State *L, int ref) {
  if (ref == LUA_REFNIL)
    LObj(L, L->pCT->top)->SetNil();
  else if (0 <= ref && ref < L->refArray.size() &&
          (L->refArray[ref].st == LOCK || L->refArray[ref].st == HOLD))
    *LObj(L, L->pCT->top) = L->refArray[ref].o;
  else
    return 0;
  api_incr_top(L);
  return 1;
}


void lua_newtable (lua_State *L) 
{
  LObj(L, L->pCT->top)->SetH( luaH_new(L, 0) );
  api_incr_top(L);
}

/*
** set functions (stack -> Lua)
*/


void lua_setglobal (lua_State *L, const char *name) {
  StkId top = L->pCT->top;
  luaV_setglobal(L, luaS_new(L, name));
  L->pCT->top = top-1;  /* remove element from the top */
}


void lua_settable (lua_State *L, int index) {
  StkId t = Index(L, index);
  StkId top = L->pCT->top;
  luaV_settable(L, t, top-2);
  L->pCT->top = top-2;  /* pop index and value */
}


void lua_rawset (lua_State *L, int index) 
{
  StkId t = Index(L, index);
  LUA_ASSERT( LObj(L, t)->GetType() == LUA_TTABLE, "table expected" );
	*L->GetHash(t)->SetObj( LObj(L, L->pCT->top-2) ) = *LObj(L, L->pCT->top-1);
  L->pCT->top -= 2;
}


void lua_rawseti (lua_State *L, int index, int n) {
  StkId o = Index(L, index);
  LUA_ASSERT( LObj(L, o)->GetType() == LUA_TTABLE, "table expected" );
	*( L->GetHash(o)->SetInt( n ) ) = *LObj(L, L->pCT->top-1);
  L->pCT->top--;
}


void lua_setglobals (lua_State *L)
{
  StkId newtable = --L->pCT->top;
  LUA_ASSERT( LObj(L, newtable)->GetType() == LUA_TTABLE, "table expected" );
  L->nGT = LObj(L, newtable)->GetH();
}


int lua_ref (lua_State *L,  int lock) 
{
  int ref;
  if ( LObj(L, L->pCT->top-1)->GetType() == LUA_TNIL )
    ref = LUA_REFNIL;
  else {
    if (L->refFree != NONEXT) {  /* is there a free place? */
      ref = L->refFree;
      L->refFree = L->refArray[ref].st;
    }
    else /* no more free places */
		{  
      ref = L->refArray.size();
			L->refArray.push_back( Ref() );
    }
    L->refArray[ref].o = *LObj(L, L->pCT->top-1);
    L->refArray[ref].st = lock ? LOCK : HOLD;
  }
  L->pCT->top--;
  return ref;
}


/*
** "do" functions (run Lua code)
** (most of them are in ldo.c)
*/

void lua_startCall (lua_State *L, int nargs, int nresults) {
  luaD_startCall( L, L->pCT->top-(nargs+1), nresults );
}

void lua_startThread( lua_State *L, int nArgs )
{
	StkId stackOld = L->pCT->top;
	//
	CLuaThread *pOld = L->pCT;
	ASSERT( pOld );
	CLuaThread *pNew = lua_newThread( L, "thread made by StartThread" );
	// get parameters from old thread
	TObject *params = LObj(L, L->pCT->top - nArgs - 1);
	// push parameters into a new thread
	lua_setThread( L, pNew );
	for ( int i = 0; i < nArgs + 1; ++i )
	{
	  *LObj(L, L->pCT->top) = *(params+i);
		api_incr_top( L );
	}
	lua_startCall( L, nArgs, 0 );
	lua_setThread( L, pOld );
	// pop parameters
	ASSERT( stackOld == L->pCT->top ); // stack corrupted
	lua_pop( L, nArgs + 1 );
	ASSERT( L->pCT == pOld ); // old thread was't restored
}

int LuaCFuncStartThread( lua_State *L )
{
	int nArgs = lua_gettop( L );
	ASSERT( nArgs > 0 ); // must be at least one argument - function name
	//
	if ( nArgs > 0 )
	{
		if ( !lua_isfunction( L, 1 ) )
		{
			ASSERT( 0 );
			luaO_verror( L, "StartThread first parameter must be a function" );
		}
		else
			lua_startThread( L, nArgs - 1 );
	}
	else
	{
		luaO_verror( L, "Must be at least one parameter for StartThread" );
	}
	//
	return 0;
}

int LuaCFuncSleep( lua_State *L )
{
	if ( L->nNoWait > 0 )
		return 0;
	CLuaThread *pCur = L->pCT;
	ASSERT( pCur );
	if ( pCur && !pCur->bErrorInThread )
	{
		int nArgs = lua_gettop( L );
		int &thisThreadIsSleeping = pCur->thisThreadIsSleeping;
		if ( nArgs == 0 )
		{
			thisThreadIsSleeping = 1;
		}
		else
		{
			double lfToSleep = lua_tonumber( L, -1 );
			thisThreadIsSleeping = lfToSleep * 50 / 200; //50 - old AI_SEGMENT_DURATION, 200 - new one
		}
	}
	return 0;
}

void lua_executeThreads( lua_State *L )
{
	for ( CThreads::iterator i = L->threads.begin(); i != L->threads.end(); )
	{
		CLuaThread *pThread = *i;
		if ( IsValid(pThread) )
		{
			int &thisThreadIsSleeping = pThread->thisThreadIsSleeping;
			if ( !pThread->bErrorInThread && thisThreadIsSleeping > 0 )
			{
				--thisThreadIsSleeping;
				if ( thisThreadIsSleeping > 0 )
				{
					++i;
					continue;
				}
			}
			//
			lua_setThread( L, pThread );
			ASSERT( L->pCT == pThread );
			//
			luaV_stepExecute( L );
			//
			if ( pThread->bErrorInThread || L->pCT->executedCalls.empty() )
			{
				/*
				if ( pThread->bErrorInThread )
					DebugTrace("[SCRIPT] Thread closed (error). Thread descr: %s\n", pThread->name.c_str() );
				if ( L->pCT->executedCalls.empty() )
					DebugTrace("[SCRIPT] Thread closed (finished). Thread descr: %s\n", pThread->name.c_str() );
					*/
				i = L->threads.erase( i );
			}
			else
				++i;
		}
		else
		{
			//DebugTrace("[SCRIPT] Thread closed (became invalid). Thread descr: %s\n", pThread->name.c_str() );
			i = L->threads.erase( i );
		}
	}
	if ( L->threads.empty() )
	{
		// we are in trouble :)
		lua_newThread( L, "Thread to have at least one" );
	}
	// we must have at least one valid thread
	lua_setThread( L, L->threads.front() );
	ASSERT( L->pCT );
}


//
// implementation for "errorHook".
//
int LuaCFuncSetThreadErrorHook( lua_State *L )
{
	int nArgs = lua_gettop(L);
	if ( 1 == nArgs )
	{
		if ( lua_isfunction(L, 1) )
		{
			L->pCT->setErrorHook();
		}
		else if ( lua_isnil(L, 1) )
		{
			L->pCT->clearErrorHook();
		}
		else
		{
			// error: arg type must be function or nil
			luaO_verror( L, "errorHook: arg type must be function or nil" );
		}
	}
	else
	{
		// error: not enough arguments
		luaO_verror( L, "errorHook: not enough arguments, must be one exactly" );
	}
	return 0;
}

/*
** Garbage-collection functions
*/

/* GC values are expressed in Kbytes: #bytes/2^10 */
#define GCscale(x)		((int)((x)>>10))
#define GCunscale(x)		((unsigned long)(x)<<10)

/*
** miscellaneous functions
*/

void lua_settag (lua_State *L, int tag) {
  luaT_realtag(L, tag);
  switch ( LObj(L, L->pCT->top-1)->GetType() ) 
	{
    case LUA_TTABLE:
			L->GetHash( L->pCT->top-1 )->htag = tag;
      break;
    case LUA_TUSERDATA:
      L->GetUData( L->pCT->top-1 )->tag = tag;
      break;
    default:
			ASSERT(0);
      luaO_verror(L, "cannot change the tag of a %.20s",
                  luaO_typename( LObj(L, L->pCT->top-1) ));
			break;
  }
}


void lua_unref (lua_State *L, int ref) {
  if (ref >= 0) {
    LUA_ASSERT(ref < L->refArray.size() && L->refArray[ref].st < 0, "invalid ref");
    L->refArray[ref].st = L->refFree;
    L->refFree = ref;
  }
}


int lua_nextAbsolute (lua_State *L, int index) {
  StkId t = index;
	Hash::Node n;
  LUA_ASSERT( LObj(L, t)->GetType() == LUA_TTABLE, "table expected" );
	L->GetHash(t)->NextNode( LObj(L, luaA_index(L, -1)), &n );
  if ( n.key ) {
    *LObj(L, L->pCT->top-1) = *n.key;
    *LObj(L, L->pCT->top) = *n.val;
    api_incr_top(L);
    return 1;
  }
  else {  /* no more elements */
    L->pCT->top -= 1;  /* remove key */
    return 0;
  }
}

int lua_next (lua_State *L, int index) {
  StkId t = luaA_index(L, index);
	Hash::Node n;
  LUA_ASSERT( LObj(L, t)->GetType() == LUA_TTABLE, "table expected" );
	L->GetHash(t)->NextNode( LObj(L, luaA_index(L, -1)), &n );
  if ( n.key ) {
    *LObj(L, L->pCT->top-1) = *n.key;
    *LObj(L, L->pCT->top) = *n.val;
    api_incr_top(L);
    return 1;
  }
  else {  /* no more elements */
    L->pCT->top -= 1;  /* remove key */
    return 0;
  }
}


int lua_getn (lua_State *L, int index) {
  Hash *h = L->GetHash( luaA_index(L, index) );
  const TObject *value = h->GetStr( luaS_new(L, "n") );  /* value = h.n */
  if ( value->GetType() == LUA_TNUMBER )
    return (int)value->GetN();
  else 
		return (int) h->GetMaxIntKey();
}


void lua_concat (lua_State *L, int n) {
  StkId top = L->pCT->top;
  luaV_strconc(L, n, top);
  L->pCT->top = top-(n-1);
  luaC_checkGC(L);
}




