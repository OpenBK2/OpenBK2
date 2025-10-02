#include "stdafx.h"
//
//** $Id: ldebug.c,v 1.50 2000/10/30 12:38:50 roberto Exp $
//** Debug Interface
//** See Copyright Notice in lua.h


#include "lcode.h"
#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "ltm.h"


static const char *getfuncname (lua_State *L, TObject *f, const char **name);

static void setnormalized( lua_State *L, TObject *d, const TObject *s ) 
{
  if ( s->GetType() == LUA_TMARK )
	{
		CallInfo *ci = L->callInfos[ s->GetCI() ];
    d->SetCL( ci->nFunc );
	}
  else 
		*d = *s;
}


static int isLmark( lua_State *L, TObject *o )
{
	if ( !o || o->GetType() != LUA_TMARK )
		return 0;
	CallInfo *ci = L->callInfos[ o->GetCI() ];
  return !L->closures[ ci->nFunc ]->isC;
}

/*
lua_Hook lua_setcallhook (lua_State *L, lua_Hook func) {
  lua_Hook oldhook = L->callhook;
  L->callhook = func;
  return oldhook;
}


lua_Hook lua_setlinehook (lua_State *L, lua_Hook func) {
  lua_Hook oldhook = L->linehook;
  L->linehook = func;
  return oldhook;
}

*/
static TObject* aux_stackedfunction (lua_State *L, int level, TObject *top) {
  TObject *pCurr;
  for ( pCurr = (top-1); pCurr >= &L->pCT->stack[0]; --pCurr ) 
	{
    if ( is_T_MARK( pCurr->GetType() ) ) {
      if (level == 0)
        return pCurr;
      level--;
    }
  }
  return NULL;
}


int lua_getstack (lua_State *L, int level, lua_Debug *ar) {
	TObject* f = aux_stackedfunction(L, level, LObj( L, L->pCT->top ) );
  if (f == NULL) return 0;  // there is no such level 
  else {
    ar->_func = f;
    return 1;
  }
}


static int nups( lua_State *L, TObject* f )
{
  switch ( f->GetType() ) 
	{
    case LUA_TFUNCTION:
      return L->closures[ f->GetCL() ]->upvalues.size();
    case LUA_TMARK:
			{
				CallInfo *ci = L->callInfos[ f->GetCI() ];
				return L->closures[ ci->nFunc ]->upvalues.size();
			}
    default:
      return 0;
  }
}


int luaG_getline ( const vector<int> &lineinfo, int pc, int refline, int *prefi) {
  int refi;
  if (lineinfo.empty() || pc == -1)
    return -1;  // no line info or function is not active
  refi = prefi ? *prefi : 0;
  if (lineinfo[refi] < 0)
    refline += -lineinfo[refi++]; 
  LUA_ASSERT(lineinfo[refi] >= 0, "invalid line info");
  while (lineinfo[refi] > pc) {
    refline--;
    refi--;
    if (lineinfo[refi] < 0)
      refline -= -lineinfo[refi--]; 
    LUA_ASSERT(lineinfo[refi] >= 0, "invalid line info");
  }
  for (;;) {
    int nextline = refline + 1;
    int nextref = refi + 1;
    if (lineinfo[nextref] < 0)
      nextline += -lineinfo[nextref++]; 
    LUA_ASSERT(lineinfo[nextref] >= 0, "invalid line info");
    if (lineinfo[nextref] > pc)
      break;
    refline = nextline;
    refi = nextref;
  }
  if (prefi) *prefi = refi;
  return refline;
}


static int currentpc (TObject *f) 
{
	return 0;
/*  CallInfo *ci = f->GetCI();
  LUA_ASSERT(isLmark(f), "function has no pc");
  if (ci->proto)
    return (ci->proto->code - ci->func->f.l->code) - 1;
  else
    return -1;  // function is not active
		*/
}

static int currentline (lua_State *L, TObject* f) 
{
  if (!isLmark(L, f))
    return -1;  // only active lua functions have current-line information 
  else 
	{
    CallInfo *ci = L->callInfos[ f->GetCI() ];
		Closure *cl = L->closures[ ci->nFunc ];
		Proto *pr = L->protos[ cl->f.nProto ];
    return luaG_getline( pr->lineinfo, currentpc(f), 1, NULL);
  }
}
/*


static Proto *getluaproto (StkId f) 
{
  return isLmark(f) ? f->GetCI()->func->f.l : NULL;
}


const char *lua_getlocal (lua_State *L, const lua_Debug *ar, int n) {
  const char *name;
  StkId f = ar->_func;
  Proto *fp = getluaproto(f);
  if (!fp) return NULL;  // `f' is not a Lua function?
  name = luaF_getlocalname(fp, n, currentpc(f));
  if (!name) return NULL;
  luaA_pushobject(L, (f+1)+(n-1));  // push value 
  return name;
}


const char *lua_setlocal (lua_State *L, const lua_Debug *ar, int n) {
  const char *name;
  StkId f = ar->_func;
  Proto *fp = getluaproto(f);
  L->pCT->top--;  // pop new value 
  if (!fp) return NULL;  // `f' is not a Lua function? 
  name = luaF_getlocalname(fp, n, currentpc(f));
  if (!name || name[0] == '(') return NULL;  // `(' starts private locals 
  *((f+1)+(n-1)) = *L->pCT->top;
  return name;
}
*/

static void infoLproto (lua_Debug *ar, Proto *f) {
#ifdef  LUA_DEBUG
	ar->source = f->source.c_str();
#endif
  ar->linedefined = f->lineDefined;
  ar->what = "Lua";
}


static void funcinfo( lua_State *L, lua_Debug *ar, TObject* func ) 
{
  Closure *cl = NULL;
  switch ( func->GetType() ) 
	{
    case LUA_TFUNCTION:
      cl = L->closures[ func->GetCL() ];
      break;
    case LUA_TMARK:
			{
				CallInfo *ci = L->callInfos[ func->GetCI() ];
	      cl = L->closures[ ci->nFunc ];
		    break;
			}
    default:
			ASSERT(0);
      lua_error(L, "value for `lua_getinfo' is not a function");
			break;
  }
  if (cl->isC) 
	{
    ar->source = "=C";
    ar->linedefined = -1;
    ar->what = "C";
  }
  else
	{
		if ( cl->f.nProto != STK_NULL )
		{
			Proto *pr = L->protos[ cl->f.nProto ];
			infoLproto( ar, pr );
		}
		else
		{
			ASSERT(0);
		}
	}
  luaO_chunkid(ar->short_src, ar->source, sizeof(ar->short_src));
  if (ar->linedefined == 0)
    ar->what = "main";
}


static const char *travtagmethods (lua_State *L, const TObject *o) 
{
  if ( o->GetType() == LUA_TFUNCTION ) 
	{
    for ( int e = 0; e < TM_N; ++e ) 
		{
      for ( int t = 0; t < L->TMtable.size(); ++t )
			{
        if ( o->GetCL() == luaT_gettm(L, t, e) )
					return luaT_eventname[e];
			}
    }
  }
  return NULL;
}


static const char *travglobals (lua_State *L, const TObject *o) {
  Hash *g = L->tables[ L->nGT ];
	for ( Hash::CObjHash::iterator it = g->h.begin(); it != g->h.end(); ++it )
	{
		if ( luaO_equalObj( o, &it->second ) && it->first.GetType() == LUA_TSTRING ) 
			return it->first.GetS()->c_str();
	}
  return NULL;
}


static void getname (lua_State *L, TObject *f, lua_Debug *ar) {
  TObject o;
  setnormalized( L, &o, f);
  // try to find a name for given function 
  if ((ar->name = travglobals(L, &o)) != NULL)
    ar->namewhat = "global";
  // not found: try tag methods 
  else if ((ar->name = travtagmethods(L, &o)) != NULL)
    ar->namewhat = "tag-method";
  else ar->namewhat = "";  // not found at all 
}

int lua_getinfo (lua_State *L, const char *what, lua_Debug *ar) {
  TObject *func;
  int isactive = (*what != '>');
  if (isactive)
    func = ar->_func;
  else {
    what++;  // skip the '>' 
    func = LObj(L, L->pCT->top - 1);
  }
  for (; *what; what++) {
    switch (*what) {
      case 'S': {
        funcinfo(L, ar, func );
        break;
      }
      case 'l': {
        ar->currentline = currentline(L, func);
        break;
      }
      case 'u': {
        ar->nups = nups( L, func );
        break;
      }
      case 'n': {
        ar->namewhat = (isactive) ? getfuncname(L, func, &ar->name) : NULL;
        if (ar->namewhat == NULL)
          getname(L, func, ar);
        break;
      }
      case 'f': {
        setnormalized( L, LObj(L, L->pCT->top), func);
        incr_top;  // push function 
        break;
      }
      default: return 0;  // invalid option 
    }
  }
  if (!isactive) L->pCT->top--;  // pop function 
  return 1;
}


//
//** {======================================================
//** Symbolic Execution
//** =======================================================

static int pushpc (int *stack, int pc, int top, int n) {
  while (n--)
    stack[top++] = pc-1;
  return top;
}


static Instruction luaG_symbexec (const Proto *pt, int lastpc, int stackpos) {
  int stack[MAXSTACK];  // stores last instruction that changed a stack entry 
  const Instruction *code = &pt->code[0];
  int top = pt->numparams;
  int pc = 0;
  if (pt->is_vararg)  // varargs? 
    top++;  // `arg' 
  while (pc < lastpc) {
    const Instruction i = code[pc++];
    LUA_ASSERT(0 <= top && top <= pt->maxstacksize, "wrong stack");
    switch (GET_OPCODE(i)) {
      case OP_RETURN: {
        LUA_ASSERT(top >= GETARG_U(i), "wrong stack");
        top = GETARG_U(i);
        break;
      }
      case OP_TAILCALL: {
        LUA_ASSERT(top >= GETARG_A(i), "wrong stack");
        top = GETARG_B(i);
        break;
      }
      case OP_CALL: {
        int nresults = GETARG_B(i);
        if (nresults == MULT_RET) nresults = 1;
        LUA_ASSERT(top >= GETARG_A(i), "wrong stack");
        top = pushpc(stack, pc, GETARG_A(i), nresults);
        break;
      }
      case OP_PUSHNIL: {
        top = pushpc(stack, pc, top, GETARG_U(i));
        break;
      }
      case OP_POP: {
        top -= GETARG_U(i);
        break;
      }
      case OP_SETTABLE:
      case OP_SETLIST: {
        top -= GETARG_B(i);
        break;
      }
      case OP_SETMAP: {
        top -= 2*GETARG_U(i);
        break;
      }
      case OP_CONCAT: {
        top -= GETARG_U(i);
        stack[top++] = pc-1;
        break;
      }
      case OP_CLOSURE: {
        top -= GETARG_B(i);
        stack[top++] = pc-1;
        break;
      }
      case OP_JMPONT:
      case OP_JMPONF: {
        int newpc = pc + GETARG_S(i);
        // jump is forward and do not skip `lastpc'? 
        if (pc < newpc && newpc <= lastpc) {
          stack[top-1] = pc-1;  // value comes from `and'/`or' 
          pc = newpc;  // do the jump 
        }
        else
          top--;  // do not jump; pop value 
        break;
      }
      default: {
        OpCode op = GET_OPCODE(i);
        LUA_ASSERT(luaK_opproperties[op].push != VD,
                   "invalid opcode for default");
        top -= luaK_opproperties[op].pop;
        LUA_ASSERT(top >= 0, "wrong stack");
        top = pushpc(stack, pc, top, luaK_opproperties[op].push);
      }
    }
  }
  return code[stack[stackpos]];
}

static const char *getobjname (lua_State *L, TObject *obj, const char **name) {
	*name = 0;
  TObject *func = aux_stackedfunction(L, 0, obj);
  if (!isLmark(L, func))
    return NULL;  // not an active Lua function 
  else {
		CallInfo *ci = L->callInfos[ func->GetCI() ];
		Closure *cl = L->closures[ ci->nFunc ];
    Proto *p = L->protos[ cl->f.nProto ];
    int pc = currentpc(func);
    int stackpos = obj - (func+1);  // func+1 == function base 
    Instruction i = luaG_symbexec(p, pc, stackpos);
    LUA_ASSERT(pc != -1, "function must be active");
    switch (GET_OPCODE(i)) {
      case OP_GETGLOBAL: {
        *name = p->strings[GETARG_U(i)]->c_str();
        return "global";
      }
      case OP_GETLOCAL: {
        *name = luaF_getlocalname(p, GETARG_U(i)+1, pc);
        LUA_ASSERT(*name, "local must exist");
        return "local";
      }
      case OP_PUSHSELF:
      case OP_GETDOTTED: {
        *name = p->strings[GETARG_U(i)]->c_str();
        return "field";
      }
      default:
        return NULL;  // no useful name found 
    }
  }
}


static const char *getfuncname (lua_State *L, TObject *f, const char **name) {
  TObject* func = aux_stackedfunction(L, 0, f);  // calling function 
  if (!isLmark(L, func))
    return NULL;  // not an active Lua function 
  else {
		CallInfo *ci = L->callInfos[ func->GetCI() ];
		Closure *cl = L->closures[ ci->nFunc ];
		Proto *p = L->protos[ cl->f.nProto ];
    int pc = currentpc(func);
    Instruction i;
    if (pc == -1) return NULL;  // function is not activated 
    i = p->code[pc];
    switch (GET_OPCODE(i)) {
      case OP_CALL: case OP_TAILCALL:
        return getobjname(L, (func+1)+GETARG_A(i), name);
      default:
        return NULL;  // no useful name found 
    }
  }
}


// }====================================================== 

void luaG_typeerror ( lua_State *L, TObject *o, const char *op ) 
{
  //const char *name = 0;
  //const char *kind = getobjname(L, o, &name);
	const char *t = luaO_typename(o);
	//if ( kind && name )
	//{
	//	luaO_verror(L, "attempt to %.30s %.20s `%.40s' (a %.10s value)", op, kind, name, t);
	//}
	//else	
	{
   luaO_verror(L, "attempt to %.30s a %.10s value", op, t);
	}
}


void luaG_binerror (lua_State *L, TObject *p1, int t, const char *op) {
  if ( p1->GetType() == t ) p1++;
  LUA_ASSERT( p1->GetType() != t, "must be an error");
  luaG_typeerror(L, p1, op);
}


void luaG_ordererror (lua_State *L, TObject *top) {
  const char *t1 = luaO_typename(top-2);
  const char *t2 = luaO_typename(top-1);
  if (t1[2] == t2[2])
    luaO_verror(L, "attempt to compare two %.10s values", t1);
  else
    luaO_verror(L, "attempt to compare %.10s with %.10s", t1, t2);
}


