#include "stdafx.h"
/*
** $Id: lgc.c,v 1.72 2000/10/26 12:47:05 roberto Exp $
** Garbage Collector
** See Copyright Notice in lua.h
*/


#include "ldo.h"
#include "lgc.h"
#include "ltm.h"


typedef struct GCState {
  Hash *tmark;  /* list of marked tables to be visited */
  Closure *cmark;  /* list of marked closures to be visited */
} GCState;



static void markobject (lua_State *L, GCState *st, const TObject *o);


/* mark a string; marks larger than 1 cannot be changed */
#define strmark(s)    {if ((s)->marked == 0) (s)->marked = 1;}



static void protomark (lua_State *L, Proto *f) {
  if (!f->marked) {
    int i;
    f->marked = 1;
    for (i=0; i<f->strings.size(); i++)
      strmark(f->strings[i]);
    for (i=0; i<f->protos.size(); i++)
		{
			Proto *p = L->protos[ f->protos[i] ];
      protomark(L, p);
		}
    for (i=0; i<f->locvars.size(); i++)  /* mark local-variable names */
      strmark(f->locvars[i].varname);
  }
}


static void markstack (lua_State *L, GCState *st) 
{
  StkId o;
	// that is how it was:
  //for (o=L->stack; o<L->pCT->top; o++)
  //  markobject(st, o);
	// we mark stacks for every thread:
	for ( CThreads::iterator i = L->threads.begin(); i != L->threads.end(); ++i )
	{
		CLuaThread *pT = *i;
		ASSERT( IsValid(pT) );
		if ( !IsValid( pT ) )
			continue;
		for ( o = 0; o < pT->top; ++o )
			markobject( L, st, &pT->stack[o] );
	}
}


static void marklock (lua_State *L, GCState *st) {
  int i;
  for ( i = 0; i < L->refArray.size(); i++) {
    if (L->refArray[i].st == LOCK)
      markobject( L, st, &L->refArray[i].o);
  }
}


static void markclosure (lua_State *L, GCState *st, Closure *cl) {
  if (!ismarked(cl)) {
    if (!cl->isC)
		{
			Proto *pr = L->protos[ cl->f.nProto ];
      protomark( L, pr );
		}
    cl->mark = st->cmark;  /* chain it for later traversal */
    st->cmark = cl;
  }
}


static void marktagmethods (lua_State *L, GCState *st) 
{
  for ( int e=0; e<TM_N; e++) 
	{
    for ( int t=0; t < L->TMtable.size(); ++t )
		{
			int nClosure = luaT_gettm(L, t, e);
			if ( nClosure != STK_NULL )
			{
			  Closure *cl = L->closures[nClosure];
		    if (cl)
					markclosure(L, st, cl);
			}
    }
  }
}


static void markobject( lua_State *L, GCState *st, const TObject *o ) 
{
  switch ( o->GetType() ) 
	{
    case LUA_TUSERDATA:  
      strmark( L->userdatas[ o->GetUData() ] );
      break;
		case LUA_TSTRING:
      strmark( o->GetS() );
      break;
    case LUA_TMARK:
			{
				CallInfo *ci = L->callInfos[ o->GetCI() ];
				Closure *pCL = L->closures[ ci->nFunc ];
				markclosure( L, st, pCL );
				break;
			}
    case LUA_TFUNCTION:
			{
				Closure *pCL = L->closures[ o->GetCL() ];
				markclosure( L, st, pCL );
				break;
			}
    case LUA_TTABLE: 
			{
				Hash *pH = L->tables[ o->GetH() ];
				if ( !ismarked( pH ) ) 
				{
					pH->mark = st->tmark;  /* chain it in list of marked */
					st->tmark = pH;
				}
				break;
			}
    default: break;  /* numbers, etc */
  }
}


static void markall (lua_State *L) {
  GCState st;
	Hash *gt = L->tables[ L->nGT ];
  st.cmark = NULL;
  st.tmark = gt;  /* put table of globals in mark list */
  gt->mark = NULL;
  marktagmethods(L, &st);  /* mark tag methods */
  markstack(L, &st); /* mark stack objects */
  marklock(L, &st); /* mark locked objects */
  for (;;) {  /* mark tables and closures */
    if (st.cmark) {
      int i;
      Closure *f = st.cmark;  /* get first closure from list */
      st.cmark = f->mark;  /* remove it from list */
      for (i=0; i<f->upvalues.size(); i++)  /* mark its upvalues */
        markobject(L, &st, &f->upvalues[i]);
    }
    else if (st.tmark) 
		{
      Hash *h = st.tmark;  /* get first table from list */
      st.tmark = h->mark;  /* remove it from list */
			for ( Hash::CObjHash::iterator it = h->h.begin(); it != h->h.end(); )
			{
				if ( it->first.GetType() != LUA_TNIL )
				{
					if ( it->second.GetType() == LUA_TNIL )
					{
						h->h.erase( it++ );
						continue;
					}
          markobject( L, &st, &it->first );
          markobject( L, &st, &it->second );
				}
				++it;
			}
    }
    else break;  /* nothing else to mark */
  }
}


static int hasmark(lua_State *L, const TObject *o) 
{
  /* valid only for locked objects */
  switch ( o->GetType() ) 
	{
    case LUA_TSTRING: 
			return o->GetS()->marked;
		case LUA_TUSERDATA:
      return L->userdatas[ o->GetUData() ]->marked;
    case LUA_TTABLE:
      return ismarked( L->tables[ o->GetH() ] );
    case LUA_TFUNCTION:
      return ismarked( L->closures[ o->GetCL() ] );
    default:  /* number */
      return 1;
  }
}


/* macro for internal debugging; check if a link of free refs is valid */
#define VALIDLINK(L, st,n)      (NONEXT <= (st) && (st) < (n))

static void invalidaterefs (lua_State *L) {
  int n = L->refArray.size();
  int i;
  for (i=0; i<n; i++) {
    struct Ref *r = &L->refArray[i];
    if (r->st == HOLD && !hasmark(L, &r->o))
      r->st = COLLECTED;
    LUA_ASSERT((r->st == LOCK && hasmark(L, &r->o)) ||
               (r->st == HOLD && hasmark(L, &r->o)) ||
                r->st == COLLECTED ||
                r->st == NONEXT ||
               (r->st < n && VALIDLINK(L, L->refArray[r->st].st, n)),
               "inconsistent ref table");
  }
  LUA_ASSERT(VALIDLINK(L, L->refFree, n), "inconsistent ref table");
}



template<class T>
static void Collect( CVectorList<T> *pRes, lua_State *L ) 
{
	for ( int i = 0; i < pRes->size(); ++i )
	{
		T *pCur = (*pRes)[i];
		if ( !pCur )
			continue;
		if ( pCur->marked )
			pCur->marked = 0;
		else
			pRes->erase(i);
	}
}

template<class T>
static void CollectMarkList( CVectorList<T> *pRes, lua_State *L ) 
{
	for ( int i = 0; i < pRes->size(); ++i )
	{
		T *pCur = (*pRes)[i];
		if ( !pCur )
			continue;
    if ( ismarked(pCur) ) 
      pCur->mark = pCur;  /* unmark */
    else 
			pRes->erase( i );
	}
}

static void collectproto( lua_State *L ) 
{
	Collect( &L->protos, L );
}

static void collectclosure( lua_State *L ) 
{
	CollectMarkList( &L->closures, L );
}

static void collecttable (lua_State *L) 
{
	CollectMarkList( &L->tables, L );
}

/*static void collectstrings (lua_State *L, int all) {
  int i;

  for ( i = 0; i < L->strings.size(); i++ ) 
	{ 
    TString **p = &L->strt.hash[i];
    TString *next;
    while ((next = *p) != NULL) {
      if (next->marked && !all) {  // preserve? 
        if (next->marked < FIXMARK)  // does not change FIXMARKs 
          next->marked = 0;
        p = &next->nexthash;
      } 
      else {  // collect 
        *p = next->nexthash;
        L->strt.nuse--;
        luaM_free(L, next);
      }
    }
  }
}*/


static void collectudata (lua_State *L, int all) 
{
	for ( int i = 0; i < L->userdatas.size(); ++i )
	{
		UserData *pCur = L->userdatas[i];
		if ( !pCur )
			continue;
    LUA_ASSERT(pCur->marked <= 1, "udata cannot be fixed");
		if ( pCur->marked )
		{
			pCur->marked = 0;
			++i;
		}
		else
		{
      int tag = pCur->tag;
      L->TMtable[tag].collected.push_back( i );
		}
	}
}


#define MINBUFFER	256
static void checkMbuffer (lua_State *L) 
{
  if ( L->Mbuffer.size() > MINBUFFER*2 ) 
	{  
    size_t newsize = L->Mbuffer.size()/2;  /* still larger than MINBUFFER */
    L->Mbuffer.resize( newsize );
  }
}


static void callgcTM (lua_State *L, const TObject *o) 
{
  int tm = luaT_gettmindex(L, o, TM_GC);
  if (tm != STK_NULL) 
	{
    int oldah = L->allowhooks;
    L->allowhooks = 0;  // stop debug hooks during GC tag methods
    luaD_checkstack(L, 2);
    LObj(L, L->pCT->top)->SetCL( tm );
    *LObj(L, L->pCT->top+1) = *o;
    L->pCT->top += 2;
    luaD_call(L, L->pCT->top-2, 0);
    L->allowhooks = oldah;  // restore hooks 
  }
}


static void callgcTMudata (lua_State *L) 
{
  TObject o;
  o.SetUData( 0 );
  ++L->nGCAvoid;
  for ( int tag = L->TMtable.size() - 1; tag >= 0; --tag ) 
	{ 
		list<int> &collect = L->TMtable[tag].collected;
		for ( list<int>::iterator i = collect.begin(); i != collect.end(); ++i )
		{
	    int udata = *i;
      o.SetUData( udata );
      callgcTM(L, &o);
			L->userdatas.erase(udata);
		}
		collect.clear();
  }
	--L->nGCAvoid;
}


void luaC_collect (lua_State *L, int all) {
  collectudata(L, all);
  callgcTMudata(L);
  //collectstrings(L, all);
  collecttable(L);
  collectproto(L);
  collectclosure(L);
}


static void luaC_collectgarbage (lua_State *L) {
  markall(L);
  invalidaterefs(L);  /* check unlocked references */
  luaC_collect(L, 0);
  checkMbuffer(L);
  callgcTM(L, &luaO_nilobject);
}


void luaC_checkGC (lua_State *L) 
{
	if ( L->nGCAvoid > 0 )
		return;
  if ( ++L->nGCticks > 1000 )
	{
    luaC_collectgarbage(L);
		L->nGCticks = 0;
	}
}


