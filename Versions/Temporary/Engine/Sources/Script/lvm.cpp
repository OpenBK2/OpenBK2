#include "stdafx.h"
/*
** $Id: lvm.c,v 1.146 2000/10/26 12:47:05 roberto Exp $
** Lua virtual machine
** See Copyright Notice in lua.h
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "lopcodes.h"
#include "lstring.h"
#include "ltable.h"
#include "lvm.h"

#ifdef OLD_ANSI
#define strcoll(a,b)	strcmp(a,b)
#endif



/*
** Extra stack size to run a function:
** TAG_LINE(1), NAME(1), TM calls(3) (plus some extra...)
*/
#define	EXTRA_STACK	8



int luaV_tonumber ( TObject *obj ) 
{
	if ( obj->GetType() != LUA_TSTRING )
		return 1;
	else 
	{
		Number n;
    if ( !luaO_str2d( obj->GetS()->c_str(), &n ) )
      return 2;
    obj->SetN( n );
    return 0;
  }
}


int luaV_tostring ( lua_State *L, TObject *obj ) 
{
	/* LUA_NUMBER */
  if ( obj->GetType() != LUA_TNUMBER )
    return 1;
  else 
	{
    char s[32];  /* 16 digits, sign, point and \0  (+ some extra...) */
    lua_number2str( s, obj->GetN() );  /* convert `s' to number */
    obj->SetS( luaS_new(L, s) );
    return 0;
  }
}

Closure *luaV_closure (lua_State *L, int nelems) {
	int nCL = luaF_newclosure(L, nelems);
  Closure *c = L->closures[ nCL ];
  L->pCT->top -= nelems;
  while (nelems--)
    c->upvalues[nelems] = *LObj( L, L->pCT->top+nelems );
  LObj(L, L->pCT->top)->SetCL( nCL );
  incr_top;
  return c;
}


void luaV_Cclosure (lua_State *L, lua_CFunction c, int nelems) {
  Closure *cl = luaV_closure(L, nelems);
  cl->f.c = c;
  cl->isC = 1;
}


void luaV_Lclosure (lua_State *L, int nProto, int nelems) {
  Closure *cl = luaV_closure(L, nelems);
  cl->f.nProto = nProto;
  cl->isC = 0;
}


/*
** Function to index a table.
** Receives the table at `t' and the key at top.
*/
const TObject *luaV_gettable (lua_State *L, StkId t) {
  int tm;
  int tg = 0;
	bool bIsTable = LObj(L, t)->GetType() == LUA_TTABLE;
	bool bDefaultTag = false;
	if ( bIsTable )
	{
		int nHash = LObj(L, t)->GetH();
		tg = L->tables[nHash]->htag;
		bDefaultTag = ( tg == LUA_TTABLE || luaT_gettm(L, tg, TM_GETTABLE) == NULL );	
	}
  if ( bIsTable && bDefaultTag ) 
	{ 
    // do a primitive get
    const TObject *h = L->GetHash( t )->GetObj( LObj(L, L->pCT->top-1) );
    // result is no nil or there is no `index' tag method?
    if ( h->GetType() != LUA_TNIL || ((tm=luaT_gettm(L, tg, TM_INDEX)) == STK_NULL))
      return h;  /* return result */
    // else call `index' tag method
  }
  else {  // try a `gettable' tag method
    tm = luaT_gettmindex(L, LObj(L, t), TM_GETTABLE);
  }
  if (tm != STK_NULL) 
	{  // is there a tag method?
    luaD_checkstack(L, 2);
    *LObj(L, L->pCT->top+1) = *LObj(L, L->pCT->top-1);  /* key */
    *LObj(L, L->pCT->top ) = *LObj(L, t);  /* table */
    LObj(L, L->pCT->top-1)->SetCL( tm );
    L->pCT->top += 2;
    luaD_call(L, L->pCT->top - 3, 1);
    return LObj(L, L->pCT->top - 1);  // call result
  }
  else 
	{  // no tag method
    luaG_typeerror(L, LObj(L, t), "index");
    //return NULL;  // to avoid warnings
		return LObj(L, t); // avoid crash
  }
}


/*
** Receives table at `t', key at `key' and value at top.
*/
void luaV_settable (lua_State *L, StkId t, StkId key) {
  int tg;
  if ( LObj(L, t)->GetType() == LUA_TTABLE &&  /* `t' is a table? */
      ( ( tg = L->GetHash(t)->htag ) == LUA_TTABLE ||  /* with default tag? */
        luaT_gettm(L, tg, TM_SETTABLE) == NULL) ) /* or no TM? */
		*( L->GetHash(t)->SetObj( LObj(L, key) ) ) = *LObj(L, L->pCT->top-1);  /* do a primitive set */
  else 
	{  /* try a `settable' tag method */
		TObject *tObj = LObj(L, t);
		ASSERT( tObj->GetType() < 1000 && tObj->GetType() > -1000 );
    int tm = luaT_gettmindex(L, tObj, TM_SETTABLE);
    if (tm != STK_NULL) 
		{
      luaD_checkstack(L, 3);
      *LObj(L, L->pCT->top+2) = *LObj(L, L->pCT->top-1);
      *LObj(L, L->pCT->top+1) = *LObj(L, key);
      *LObj(L, L->pCT->top) = *LObj(L, t);
      LObj(L, L->pCT->top-1)->SetCL( tm );
      L->pCT->top += 3;
      luaD_call(L, L->pCT->top - 4, 0);  /* call `settable' tag method */
    }
    else  /* no tag method... */
      luaG_typeerror(L, LObj(L, t), "index");
  }
}


const TObject *luaV_getglobal (lua_State *L, TString *s) {
	Hash *gt = L->tables[ L->nGT ];
  const TObject *value = gt->GetStr(s);
	if ( value->GetType() == LUA_TNIL && s->GetStr() != "FALSE" )
	{
		csSystem << CC_RED << "[Script warning!] Value was NIL when getting global with name '" << s->GetStr() << "'" << endl;
	}
  int tm = luaT_gettmindex(L, value, TM_GETGLOBAL);
  if (tm == STK_NULL)  /* is there a tag method? */
    return value;  /* default behavior */
  else 
	{  /* tag method */
    luaD_checkstack(L, 3);
    LObj(L, L->pCT->top)->SetCL( tm );
    LObj(L, L->pCT->top+1)->SetS( s );  /* global name */
    *LObj(L, L->pCT->top+2) = *value;
    L->pCT->top += 3;
    luaD_call(L, L->pCT->top - 3, 1);
    return LObj( L, L->pCT->top - 1 );
  }
}


void luaV_setglobal (lua_State *L, TString *s) {
	Hash *gt = L->tables[ L->nGT ];
  const TObject *oldvalue = gt->GetStr(s);
  int tm = luaT_gettmindex(L, oldvalue, TM_SETGLOBAL);
  if (tm == STK_NULL) {  /* is there a tag method? */
    if (oldvalue != &luaO_nilobject) {
      /* cast to remove `const' is OK, because `oldvalue' != luaO_nilobject */
      *(TObject *)oldvalue = *LObj(L, L->pCT->top - 1);
    }
    else 
		{
      TObject key;
			key.SetS( s );
			Hash *gt = L->tables[ L->nGT ];
			*gt->SetObj( &key ) = *LObj(L, L->pCT->top - 1);
    }
  }
  else 
	{
    luaD_checkstack(L, 3);
    *LObj(L, L->pCT->top + 2) = *LObj(L, L->pCT->top-1);  /* new value */
    *LObj(L, L->pCT->top + 1) = *oldvalue;
     LObj(L, L->pCT->top    )->SetS( s );
     LObj(L, L->pCT->top - 1)->SetCL( tm );
    L->pCT->top += 3;
    luaD_call(L, L->pCT->top - 4, 0);
  }
}


static int call_binTM (lua_State *L, StkId top, TMS event) {
  /* try first operand */
  int tm = luaT_gettmindex(L, LObj(L, top-2), event);
  L->pCT->top = top;
  if (tm == STK_NULL) {
    tm = luaT_gettmindex(L, LObj(L, top-1), event);  /* try second operand */
    if (tm == STK_NULL) {
      tm = luaT_gettm(L, 0, event);  /* try a `global' method */
      if (tm == STK_NULL)
        return 0;  /* error */
    }
  }
  lua_pushstring(L, luaT_eventname[event]);
  luaD_callTM(L, tm, 3, 1);
  return 1;
}


static void call_arith (lua_State *L, StkId top, TMS event) {
  if (!call_binTM(L, top, event))
    luaG_binerror(L, LObj(L, top-2), LUA_TNUMBER, "perform arithmetic on");
}


static int luaV_strcomp (const TString *ls, const TString *rs) 
{
	return strcmp( ls->c_str(), rs->c_str() );
}


int luaV_lessthan (lua_State *L, StkId stL, StkId stR, StkId top) 
{
	TObject *l = LObj( L, stL ), *r = LObj( L, stR );
  if ( l->GetType() == LUA_TNUMBER && r->GetType() == LUA_TNUMBER )
    return l->GetN() < r->GetN();
  else if ( l->GetType() == LUA_TSTRING && r->GetType() == LUA_TSTRING )
    return luaV_strcomp( l->GetS(), r->GetS() ) < 0;
  else 
	{  
		/* call TM */
    luaD_checkstack(L, 2);
    *LObj(L, top++) = *l;
    *LObj(L, top++) = *r;
    if (!call_binTM(L, top, TM_LT))
      luaG_ordererror( L, LObj(L, top-2) );
    L->pCT->top--;
    return LObj(L, L->pCT->top)->GetType() != LUA_TNIL;
  }
}


void luaV_strconc (lua_State *L, int total, StkId top) {
  do {
    int n = 2;  /* number of elements handled in this pass (at least 2) */
    if (tostring(L, LObj(L, top-2)) || tostring(L, LObj(L, top-1))) {
      if (!call_binTM(L, top, TM_CONCAT))
        luaG_binerror(L, LObj(L, top-2), LUA_TSTRING, "concat");
    }
    else if ( !LObj(L, top-1)->GetS()->empty() ) 
		{  
			string str;
      while (n < total && !tostring(L, LObj(L, top-n-1))) 
        n++;
      for ( int i = n; i > 0; i-- ) 
				str += LObj(L, top-i)->GetS()->c_str();
			LObj(L, top-n)->SetS( luaS_newlstr( L, str.c_str() ) );
    }
    total -= n-1;  /* got `n' strings to create 1 new */
    top -= n-1;
  } while (total > 1);  /* repeat until only 1 result left */
}


static void luaV_pack (lua_State *L, StkId firstelem) {
  int i;
	int nTab = luaH_new(L, 0);
  Hash *htab = L->tables[nTab];
  for (i=0; firstelem+i<L->pCT->top; i++)
		*htab->SetInt( i+1 ) = *LObj(L, firstelem+i);
  /* store counter in field `n' */
	htab->SetStr( luaS_new(L, "n") )->SetN( i );
  L->pCT->top = firstelem;  /* remove elements from the stack */
  LObj(L, L->pCT->top)->SetH( nTab );
  incr_top;
}


static void adjust_varargs (lua_State *L, StkId base, int nfixargs) {
  int nvararg = (L->pCT->top-base) - nfixargs;
  if (nvararg < 0)
    luaD_adjusttop(L, base, nfixargs);
  luaV_pack(L, base+nfixargs);
}



#define dojump(pc, i)	{ int d = GETARG_S(i); pc += d; }

/*
** Executes the given Lua function. Parameters are between [base,top).
** Returns n such that the the results are between [n,top).
*/

void luaV_beginExecute(lua_State *L, int nClosure, StkId base, int nResults )
{
	// general Lua execution header
	Closure *cl = L->closures[ nClosure ];
	const Proto *const tf = L->protos[ cl->f.nProto ];
	L->GetCallInfo( base - 1 )->nProto = cl->f.nProto;
  luaD_checkstack(L, tf->maxstacksize+EXTRA_STACK);
  if (tf->is_vararg)  /* varargs? */
    adjust_varargs(L, base, tf->numparams);
  else
    luaD_adjusttop(L, base, tf->numparams);
	// saving call
	SLuaVMState state;
	state.base = base;
	state.nClosure = nClosure;
	state.nProto = cl->f.nProto;
	state.currPC = 0;
	state.nResults = nResults;
	L->pCT->executedCalls.push_back( state );
}

void luaV_returnFromExecute( lua_State *L, StkId id )
{
	ASSERT( !L->pCT->executedCalls.empty() );
	StkId base = L->pCT->executedCalls.back().base;
	int nResults = L->pCT->executedCalls.back().nResults;
	StkId func = base - 1;
	L->pCT->executedCalls.pop_back();
	luaD_endCall( L, func, nResults, id );
}

struct SLuaEntryCount
{
	static int n;
	SLuaEntryCount() {++n;}
	~SLuaEntryCount() {--n;}
};
int SLuaEntryCount::n;
void luaV_stepExecute( lua_State *L )
{
	SLuaEntryCount entryCount;
	ASSERT( entryCount.n == 1 || L->nNoWait > 0 );
	ASSERT( IsValid( L->pCT ) );
	if ( L->pCT->bErrorInThread )
	{
		csSystem << CC_RED << "Script failed, unknown error" << endl;
		//		ASSERT( 0 ); // error in thread - thread must be deleted
		return;
	}
	/*if ( !L->pCT->executedCalls.empty() )
	{
		SLuaVMState &state = L->pCT->executedCalls.back();
		ASSERT( IsInSet( L->protos, state.tf ) );
	}*/

  StkId &top = L->pCT->top;  /* first free slot in the stack */
  StkId stack = 0;  /* stack base */

	while ( 
		!L->pCT->bErrorInThread &&
		( L->nNoWait > 0 || L->pCT->thisThreadIsSleeping <= 0 ) &&
		!L->pCT->executedCalls.empty() )
	{
		SLuaVMState &state = L->pCT->executedCalls.back();
		Proto *pProto = L->protos[ state.nProto ];
		const Instruction i = pProto->code[ state.currPC ];
		++state.currPC;
		switch (GET_OPCODE(i)) {
			case OP_END: {
				luaV_returnFromExecute( L, top );//state.top );
				break;
			}
			case OP_RETURN: {
				luaV_returnFromExecute( L, state.base+GETARG_U(i) );
				break;
			}
			case OP_CALL: {
				int nres = GETARG_B(i);
				if (nres == MULT_RET) nres = LUA_MULTRET;
				bool bNewCallAdded = luaD_startCall(L, state.base+GETARG_A(i), nres);
				// state is now invalid reference, because pExecuteCalls could change!
/*				vector<SLuaVMState> &states = L->pCT->executedCalls;
				if ( bNewCallAdded )
				{
					ASSERT( states.size() > 1 );
					SLuaVMState &stateValid = states[ states.size() - 2 ];
					stateValid.top = L->pCT->top;
				}
				else
				{
					ASSERT( states.size() > 0 );
					SLuaVMState &stateValid = states[ states.size() - 1 ];
					stateValid.top = L->pCT->top;
				}*/
				break;
			}
			case OP_TAILCALL: {
				StkId base = state.base; // "state" reference could become invalid after startCall
				luaD_startCall(L, state.base+GETARG_A(i), LUA_MULTRET);
				luaV_returnFromExecute( L, base+GETARG_B(i) );
				break;
			}
			case OP_PUSHNIL: {
				int n = GETARG_U(i);
				LUA_ASSERT(n>0, "invalid argument");
				do {
					LObj(L, top++)->SetNil();
				} while (--n > 0);
				break;
			}
			case OP_POP: {
				top -= GETARG_U(i);
				break;
			}
			case OP_PUSHINT: {
				LObj(L, top)->SetN( (Number)GETARG_S(i) );
				top++;
				break;
			}
			case OP_PUSHSTRING: {
				LObj(L, top)->SetS( pProto->strings[GETARG_U(i)] );
				top++;
				break;
			}
			case OP_PUSHNUM: {
				LObj(L, top)->SetN( pProto->numbers[GETARG_U(i)] );
				top++;
				break;
			}
			case OP_PUSHNEGNUM: {
				LObj(L, top)->SetN( -pProto->numbers[GETARG_U(i)] );
				top++;
				break;
			}
			case OP_PUSHUPVALUE: {
				{
					Closure *cl = L->closures[ state.nClosure ];
					*LObj(L, top++) = cl->upvalues[GETARG_U(i)];
					break;
				}
			}
			case OP_GETLOCAL: {
				*LObj(L, top++) = *LObj(L, state.base+GETARG_U(i));
				break;
			}
			case OP_GETGLOBAL: {
				ASSERT( L->pCT->HasValidTop() );
				*LObj(L, top) = *luaV_getglobal(L, pProto->strings[GETARG_U(i)]);
				ASSERT( L->pCT->HasValidTop() );
				top++;
				break;
			}
			case OP_GETTABLE: {
				*LObj(L, top-2) = *luaV_gettable(L, top-2);
				top--;
				break;
			}
			case OP_GETDOTTED: {
				LObj(L, top)->SetS( pProto->strings[GETARG_U(i)] );
				++top;
				*LObj(L, top-2) = *luaV_gettable(L, top-2);
				--top;
				break;
			}
			case OP_GETINDEXED: {
				*LObj(L, top) = *LObj(L, state.base+GETARG_U(i));
				++top;
				*LObj(L, top-2) = *luaV_gettable(L, top-2);
				--top;
				break;
			}
			case OP_PUSHSELF: {
				TObject receiver;
				receiver = *LObj(L, top-1);
				LObj(L, top)->SetS( pProto->strings[GETARG_U(i)] );
				top++;
				*LObj(L, top-2) = *luaV_gettable(L, top-2);
				*LObj(L, top-1) = receiver;
				break;
			}
			case OP_CREATETABLE: {
				luaC_checkGC(L);
				LObj(L, top)->SetH( luaH_new(L, GETARG_U(i)) );
				top++;
				break;
			}
			case OP_SETLOCAL: {
				*LObj(L, state.base+GETARG_U(i)) = *LObj(L, --top);
				break;
			}
			case OP_SETGLOBAL: {
				luaV_setglobal(L, pProto->strings[GETARG_U(i)]);
				--top;
				break;
			}
			case OP_SETTABLE: {
				StkId t = top-GETARG_A(i);
				luaV_settable(L, t, t+1);
				top -= GETARG_B(i);  /* pop values */
				break;
			}
			case OP_SETLIST: {
				int aux = GETARG_A(i) * LFIELDS_PER_FLUSH;
				int n = GETARG_B(i);
				Hash *arr = L->GetHash( top-n-1 );
				for (; n; n--)
					*arr->SetInt( n+aux ) = *LObj(L, --top);
				break;
			}
			case OP_SETMAP: {
				int n = GETARG_U(i);
				StkId finaltop = top-2*n;
				Hash *arr = L->GetHash(finaltop-1);
				for (; n; n--) {
					top-=2;
					*arr->SetObj( LObj(L, top) ) = *LObj(L, top+1);
				}
				break;
			}
			case OP_ADD: {
				if (tonumber( LObj(L, top-2) ) || tonumber( LObj(L, top-1) ))
					call_arith(L, top, TM_ADD);
				else
					LObj(L, top-2)->SetN( LObj(L, top-2)->GetN() + LObj(L, top-1)->GetN() );
				top--;
				break;
			}
			case OP_ADDI: {
				if (tonumber( LObj(L, top-1) )) 
				{
					LObj(L, top)->SetN( (Number)GETARG_S(i) );
					call_arith(L, top+1, TM_ADD);
				}
				else
					LObj(L, top-1)->SetN( LObj(L, top-1)->GetN() + (Number)GETARG_S(i) );
				break;
			}
			case OP_SUB: {
				if (tonumber( LObj(L, top-2) ) || tonumber( LObj(L, top-1) ))
					call_arith(L, top, TM_SUB);
				else
					LObj(L, top-2)->SetN( LObj(L, top-2)->GetN() - LObj(L, top-1)->GetN() );
				top--;
				break;
			}
			case OP_MULT: {
				if (tonumber( LObj(L, top-2) ) || tonumber( LObj(L, top-1) ))
					call_arith(L, top, TM_MUL);
				else
					LObj(L, top-2)->SetN( LObj(L, top-2)->GetN() * LObj(L, top-1)->GetN() );
				top--;
				break;
			}
			case OP_DIV: {
				if (tonumber( LObj(L, top-2) ) || tonumber( LObj(L, top-1) ))
					call_arith(L, top, TM_DIV);
				else
					LObj(L, top-2)->SetN( LObj(L, top-2)->GetN() / LObj(L, top-1)->GetN() );
				top--;
				break;
			}
			case OP_POW: {
				if (!call_binTM(L, top, TM_POW))
					lua_error(L, "undefined operation");
				top--;
				break;
			}
			case OP_CONCAT: {
				int n = GETARG_U(i);
				luaV_strconc(L, n, top);
				top -= n-1;
				luaC_checkGC(L);
				break;
			}
			case OP_MINUS: {
				if (tonumber( LObj(L, top-1) )) {
					LObj(L, top)->SetNil();
					call_arith(L, top+1, TM_UNM);
				}
				else
					LObj(L, top-1)->SetN( -LObj(L, top-1)->GetN() );
				break;
			}
			case OP_NOT: {
				if ( LObj(L, top-1)->GetType() == LUA_TNIL )
					LObj(L, top-1)->SetN( 1 );
				else
					LObj(L, top-1)->SetNil();
				break;
			}
			case OP_JMPNE: {
				top -= 2;
				TObject *o1 = LObj( L, top ), *o2 = LObj( L, top+1 );
				if (!luaO_equalObj(o1, o2)) dojump(state.currPC, i);
				break;
			}
			case OP_JMPEQ: {
				top -= 2;
				TObject *o1 = LObj( L, top ), *o2 = LObj( L, top+1 );
				if (luaO_equalObj(o1, o2)) dojump(state.currPC, i);
				break;
			}
			case OP_JMPLT: {
				top -= 2;
				if (luaV_lessthan(L, top, top+1, top+2)) dojump(state.currPC, i);
				break;
			}
			case OP_JMPLE: {  /* a <= b  ===  !(b<a) */
				top -= 2;
				if (!luaV_lessthan(L, top+1, top, top+2)) dojump(state.currPC, i);
				break;
			}
			case OP_JMPGT: {  /* a > b  ===  (b<a) */
				top -= 2;
				if (luaV_lessthan(L, top+1, top, top+2)) dojump(state.currPC, i);
				break;
			}
			case OP_JMPGE: {  /* a >= b  ===  !(a<b) */
				top -= 2;
				if (!luaV_lessthan(L, top, top+1, top+2)) dojump(state.currPC, i);
				break;
			}
			case OP_JMPT: {
				if ( LObj(L, --top)->GetType() != LUA_TNIL ) 
					dojump(state.currPC, i);
				break;
			}
			case OP_JMPF: {
				if ( LObj(L, --top)->GetType() == LUA_TNIL ) 
					dojump(state.currPC, i);
				break;
			}
			case OP_JMPONT: {
				if ( LObj(L, top-1)->GetType() == LUA_TNIL) 
					top--;
				else 
					dojump(state.currPC, i);
				break;
			}
			case OP_JMPONF: {
				if ( LObj(L, top-1)->GetType() != LUA_TNIL) 
					top--;
				else 
					dojump(state.currPC, i);
				break;
			}
			case OP_JMP: {
				dojump(state.currPC, i);
				break;
			}
			case OP_PUSHNILJMP: {
				LObj(L, top++)->SetNil();
				state.currPC++;
				break;
			}
			case OP_FORPREP: {
				if (tonumber( LObj(L, top-1) ))
					lua_error(L, "`for' step must be a number");
				if (tonumber( LObj(L, top-2) ))
					lua_error(L, "`for' limit must be a number");
				if (tonumber( LObj(L, top-3) ))
					lua_error(L, "`for' initial value must be a number");
				if ( LObj(L, top-1)->GetN() > 0 ?
					LObj(L, top-3)->GetN() > LObj(L, top-2)->GetN() :
					LObj(L, top-3)->GetN() < LObj(L, top-2)->GetN() )
				{
					/* `empty' loop? */
					top -= 3;  /* remove control variables */
					dojump(state.currPC, i);  /* jump to loop end */
				}
				break;
			}
			case OP_FORLOOP: {
				TObject *pInc = LObj(L, top-1);
				TObject *pLimit = LObj(L, top-2);
				TObject *pNow = LObj(L, top-3);
				LUA_ASSERT( pInc->GetType() == LUA_TNUMBER, "invalid step");
				LUA_ASSERT( pLimit->GetType() == LUA_TNUMBER, "invalid limit");
				if ( pNow->GetType() != LUA_TNUMBER )
					lua_error(L, "`for' index must be a number");

				pNow->SetN( pNow->GetN() + pInc->GetN() );

				if ( pInc->GetN() > 0 ?
					pNow->GetN() > pLimit->GetN() :
					pNow->GetN() < pLimit->GetN() )
					top -= 3;  /* end loop: remove control variables */
				else
					dojump(state.currPC, i);  /* repeat loop */
				break;
			}
			case OP_LFORPREP: {
				Hash::Node node;
				TObject *pTab = LObj(L, top-1);
				if ( pTab->GetType() != LUA_TTABLE)
					lua_error(L, "`for' table must be a table");
				L->GetHash(top-1)->NextNode( &luaO_nilobject, &node );
				if ( node.key == NULL ) {  /* `empty' loop? */
					top--;  /* remove table */
					dojump(state.currPC, i);  /* jump to loop end */
				}
				else {
					top += 2;  /* index,value */
					*LObj(L, top-2) = *node.key;
					*LObj(L, top-1) = *node.val;
				}
				break;
			}
			case OP_LFORLOOP: 
			{
				Hash::Node node;
				LUA_ASSERT( LObj(L, top-3)->GetType() == LUA_TTABLE, "invalid table" );
        L->GetHash(top-3)->NextNode( LObj(L, top-2), &node );
				if ( node.key == NULL )  /* end loop? */
					top -= 3;  /* remove table, key, and value */
				else {
					*LObj(L, top-2) = *node.key;
					*LObj(L, top-1) = *node.val;
					dojump(state.currPC, i);  /* repeat loop */
				}
				break;
			}
			case OP_CLOSURE: {
				int nProto = pProto->protos[GETARG_A(i)];
				luaV_Lclosure(L, nProto, GETARG_B(i));
				luaC_checkGC(L);
				break;
			}
		}
  }
#ifdef _DEBUG
	// didn't we spoil the CBase?
//	ASSERT( L->Cbase <= L->pCT->top );
	// didn't we spoil the stack?
	ASSERT( L->pCT->HasValidTop() );
	// didn't we spoil the stack objects?
	if ( L->pCT->HasValidTop() )
		for ( StkId j = 0; j < L->pCT->top ; ++j )
			ASSERT( LObj( L, j )->HasValidType() );
#endif
}

/*
** Executes the given Lua function. Parameters are between [base,top).
** Returns n such that the the results are between [n,top).
*/
/*StkId luaV_execute (lua_State *L, const Closure *cl, StkId base) {
  const Proto *const tf = cl->f.l;
  StkId top;  // keep top local, for performance 
  const Instruction *pc = tf->code;
  const vector<TString*> &strings = tf->strings;
  const lua_Hook linehook = L->linehook;
  base[-1].GetCI()->proto = const_cast<Proto*>( tf );
  luaD_checkstack(L, tf->maxstacksize+EXTRA_STACK);
  if (tf->is_vararg)  // varargs? 
    adjust_varargs(L, base, tf->numparams);
  else
    luaD_adjusttop(L, base, tf->numparams);
  top = L->pCT->top;
  // main loop of interpreter 
  for (;;) {
    const Instruction i = *pc++;
    if (linehook)
      traceexec(L, base, top, linehook);
    switch (GET_OPCODE(i)) {
      case OP_END: {
        L->pCT->top = top;
        return top;
      }
      case OP_RETURN: {
        L->pCT->top = top;
        return base+GETARG_U(i);
      }
*/
