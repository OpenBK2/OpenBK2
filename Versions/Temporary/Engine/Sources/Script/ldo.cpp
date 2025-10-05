#include "stdafx.h"
/*
** $Id: ldo.c,v 1.109 2000/10/30 12:38:50 roberto Exp $
** Stack and Call structure of Lua
** See Copyright Notice in lua.h
*/


#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "ldebug.h"
#include "ldo.h"
#include "lgc.h"
#include "lparser.h"
#include "ltable.h"
#include "lundump.h"
#include "lvm.h"


namespace NScript
{
	SLUAError luaLastError;
}

/**
// space to handle stack overflow errors
#define EXTRA_STACK	(2*LUA_MINSTACK)

void luaD_init (lua_State *L, int stacksize) {
	L->pCT->stack = luaM_newvector(L, stacksize+EXTRA_STACK, TObject);
	L->pCT->stack_last = L->pCT->stack+(stacksize-1);
	L->pCT->stacksize = stacksize;
	L->pCT->Cbase = L->pCT->top = L->pCT->stack;
}
/**/


void luaD_checkstack (lua_State *L, int n) 
{
	if ( L->pCT->stack.size() - 1 - L->pCT->top <= n ) 
		L->pCT->stack.resize( L->pCT->stack.size() * 2 );
}

/*
** Adjust stack. Set top to base+extra, pushing NILs if needed.
** (we cannot add base+extra unless we are sure it fits in the stack;
**  otherwise the result of such operation on pointers is undefined)
*/
void luaD_adjusttop (lua_State *L, StkId base, int extra)
{
	int diff = extra-(L->pCT->top-base);
	if (diff <= 0)
		L->pCT->top = base+extra;
	else {
		luaD_checkstack(L, diff);
		while (diff--)
			LObj(L, L->pCT->top++)->SetNil();
	}
}


/*
** Open a hole inside the stack at `pos'
*/
static void luaD_openstack (lua_State *L, StkId pos)
{
	int i = L->pCT->top-pos; 
	while (i--) 
		*LObj(L, pos+i+1) = *LObj(L, pos+i);
	incr_top;
}


static void dohook (lua_State *L, lua_Debug *ar, lua_Hook hook)
{
	StkId old_Cbase = L->pCT->Cbase;
	StkId old_top = L->pCT->Cbase = L->pCT->top;
	luaD_checkstack(L, LUA_MINSTACK);  /* ensure minimum stack size */
	L->allowhooks = 0;  /* cannot call hooks inside a hook */
	(*hook)(L, ar);
	LUA_ASSERT(L->allowhooks == 0, "invalid allow");
	L->allowhooks = 1;
	L->pCT->top = old_top;
	L->pCT->Cbase = old_Cbase;
}


void luaD_lineHook (lua_State *L, StkId func, int line, lua_Hook linehook)
{
	if (L->allowhooks) {
		lua_Debug ar;
		ar._func = LObj(L, func);
		ar.event = "line";
		ar.currentline = line;
		dohook(L, &ar, linehook);
	}
}


static void luaD_callHook (lua_State *L, TObject *func, lua_Hook callhook, const char *event)
{
	if (L->allowhooks) {
		lua_Debug ar;
		ar._func = func;
		ar.event = event;
		CallInfo *ci = L->callInfos[ func->GetCI() ];
		ci->nProto = STK_NULL;  /* function is not active */
		dohook(L, &ar, callhook);
	}
}


static StkId callCclosure (lua_State *L, const struct Closure *cl, StkId base)
{
	int nup = cl->upvalues.size();  /* number of upvalues */
	StkId old_Cbase = L->pCT->Cbase;
	int n;
	L->pCT->Cbase = base;       /* new base for C function */
	luaD_checkstack(L, nup+LUA_MINSTACK);  /* ensure minimum stack size */
	for (n=0; n<nup; n++)  /* copy upvalues as extra arguments */
		*LObj(L, L->pCT->top++) = cl->upvalues[n];
	n = (*cl->f.c)(L);  /* do the actual call */
	L->pCT->Cbase = old_Cbase;  /* restore old C base */
	return L->pCT->top - n;  /* return index of first result */
}


void luaD_callTM (lua_State *L, int nClosure, int nParams, int nResults) 
{
	StkId base = L->pCT->top - nParams;
	luaD_openstack(L, base);
	LObj(L, base)->SetCL( nClosure );
	luaD_call(L, base, nResults);
}


/*
** Call a function (C or Lua). The function to be called is at *func.
** The arguments are on the stack, right after the function.
** When returns, the results are on the stack, starting at the original
** function position.
** The number of results is nResults, unless nResults=LUA_MULTRET.
*/ 

bool luaD_startCall( lua_State *L, StkId func, int nResults ) 
{
	//
	TObject *pO = LObj(L, func);
	if ( pO->GetType() != LUA_TFUNCTION )
	{
		// `func' is not a function; check the `function' tag method
		int nCl = luaT_gettmindex(L, pO, TM_FUNCTION);
		if (nCl == STK_NULL)
		{
			luaG_typeerror( L, pO, "call" );
			//luaO_verror( L, "attempt to call a nil or non function value" );
			return false;
		}
		luaD_openstack( L, func );
		LObj(L, func)->SetCL( nCl );  // tag method is the new function to be called
	}
	int nCL = LObj( L, func )->GetCL();
	Closure *cl = L->GetClosure(func);
	CallInfo *ci = new CallInfo;
	int nCallInfo = L->callInfos.push( ci );
	ci->nFunc = LObj(L, func)->GetCL();
	LObj(L, func)->SetCI( nCallInfo );
	if ( L->callhook )
		luaD_callHook( L, LObj(L, func), L->callhook, "call" );
	ASSERT( L->pCT->Cbase <= L->pCT->top );
	if ( cl->isC )
	{
		StkId firstresult = callCclosure( L, cl, func+1 );
		// end call just now!
		ASSERT( L->pCT->Cbase <= L->pCT->top );
		luaD_endCall( L, func, nResults, firstresult );
		return false;
	}
	else
	{
		luaV_beginExecute( L, nCL, func+1, nResults );
		return true;
	}
}

void luaD_endCall (lua_State *L, StkId func, int nResults, StkId firstResult ) 
{
	ASSERT( firstResult );
	if ( L->callhook ) 
		luaD_callHook(L, LObj(L, func), L->callhook, "return");
	LUA_ASSERT( LObj(L, func)->GetType() == LUA_TMARK, "invalid tag");
	CallInfo* ci = L->GetCallInfo( func );
	int nCI = LObj( L, func )->GetCI();
	/* move results to `func' (to erase parameters and function) */
	if (nResults == LUA_MULTRET) {
		while (firstResult < L->pCT->top)  /* copy all results */
			*LObj(L, func++) = *LObj(L, firstResult++);
		L->pCT->top = func;
	}
	else {  /* copy at most `nResults' */
		for (; nResults > 0 && firstResult < L->pCT->top; nResults--)
			*LObj(L, func++) = *LObj(L, firstResult++);
		L->pCT->top = func;
		for (; nResults > 0; nResults--) {  /* if there are not enough results */
			LObj(L, L->pCT->top)->SetNil();  /* adjust the stack */
			incr_top;  /* must check stack space */
		}	
		ASSERT( L->pCT->Cbase <= L->pCT->top );
	}
	luaC_checkGC(L);
	L->callInfos.erase( nCI );
}

void luaD_call( lua_State *L, StkId func, int nResults )
{
	++L->nNoWait;
	if ( luaD_startCall( L, func, nResults ) )
		luaV_stepExecute( L );
	--L->nNoWait;
}


/*
** Execute a protected call.
*/
/**
struct CallS {  // data to `f_call'
	StkId func;
	int nresults;
};

static void f_call (lua_State *L, void *ud) {
	struct CallS *c = (struct CallS *)ud;
	luaD_call(L, c->func, c->nresults);
}


int lua_call (lua_State *L, int nargs, int nresults) {
	StkId func = L->pCT->top - (nargs+1);  // function to be called 
	struct CallS c;
	int status;
	c.func = func; c.nresults = nresults;
	status = luaD_runprotected(L, f_call, &c);
	if (status != 0)  // an error occurred? 
		L->pCT->top = func;  // remove parameters from the stack 
	return status;
}
/**/


/*
** Execute a protected parser.
*/
struct ParserS {  /* data to `f_parser' */
	ZIO *z;
	int bin;
};

static void f_parser (lua_State *L, void *ud)
{
	struct ParserS *p = (struct ParserS *)ud;
	ASSERT( !p->bin );
	int nGeneratedProto = p->bin ? luaU_undump(L, p->z) : luaY_parser(L, p->z);
	luaV_Lclosure(L, nGeneratedProto, 0);
}

static int nTryCount;
static int luaD_runprotected (lua_State *L, void (*f)(lua_State *, void *), void *ud) {
	StkId oldCbase = L->pCT->Cbase;
	StkId oldtop = L->pCT->top;
	int allowhooks = L->allowhooks;
	int nRet = 0;
	try
	{
		++nTryCount;
		(*f)(L, ud);
	}
	catch ( int nErr )
	{
		nRet = nErr;
		L->allowhooks = allowhooks;
		L->pCT->Cbase = oldCbase;
		L->pCT->top = oldtop;
		//restore_stack_limit(L);
	}
	catch(...)
	{
		ASSERT(0);
	}
	--nTryCount;
	ASSERT( nTryCount >= 0 );
	return nRet;
}

static int protectedparser (lua_State *L, ZIO *z, int bin) {
	struct ParserS p;
	int status;
	p.z = z; p.bin = bin;
	luaC_checkGC(L);
	status = luaD_runprotected(L, f_parser, &p);
	if (status == LUA_ERRRUN)  /* an error occurred: correct error code */
		status = LUA_ERRSYNTAX;
	return status;
}


static int parse_file (lua_State *L, const char *filename) {
	ZIO z;
	int status;
	int bin;  /* flag for file mode */
	int c;    /* look ahead char */
	FILE *f = (filename == NULL) ? stdin : fopen(filename, "r");
	if (f == NULL) return LUA_ERRFILE;  /* unable to open file */
	c = fgetc(f);
	ungetc(c, f);
	bin = (c == ID_CHUNK);
	if (bin && f != stdin) {
		f = freopen(filename, "rb", f);  /* set binary mode */
		if (f == NULL) return LUA_ERRFILE;  /* unable to reopen file */
	}
	lua_pushstring(L, "@");
	lua_pushstring(L, (filename == NULL) ? "(stdin)" : filename);
	lua_concat(L, 2);
	filename = lua_tostring(L, -1);  /* filename = '@'..filename */
	lua_pop(L, 1);  /* OK: there is no GC during parser */
	luaZ_Fopen(&z, f, filename);
	status = protectedparser(L, &z, bin);
	if (f != stdin)
		fclose(f);
	return status;
}

void lua_startCall (lua_State *L, int nargs, int nresults);
// defined in lapi.cpp

int lua_dofile (lua_State *L, const char *filename) 
{
	CObj<CLuaThread> pOld = L->pCT;
	lua_setThread( L, lua_newThread( L, filename ) );
	int status = parse_file(L, filename);
	if ( status == 0 )  // parse OK?
		lua_startCall( L, 0, LUA_MULTRET );
	lua_setThread( L, pOld );
	//if ( status == 0 )
	//	lua_executeThreads( L );
	return status;
}

static int parse_buffer (lua_State *L, const char *buff, size_t size,
		                 		const char *name) {
	ZIO z;
	if (!name) name = "?";
	luaZ_mopen(&z, buff, size, name);
	int nRes = protectedparser( L, &z, buff[0]==ID_CHUNK );
	if ( nRes != 0 )
		L->pCT->bErrorInThread = true;
	return nRes;
}

int lua_parsebuffer (lua_State *L, const char *buff, size_t size, const char *name)
{
	return parse_buffer(L, buff, size, name);
}

int lua_dobuffer( lua_State *L, const char *buff, size_t size, const char *name ) 
{
	CObj<CLuaThread> pOld = L->pCT;
	lua_setThread( L, lua_newThread( L, "Buffer thread" ) );
	int status = parse_buffer( L, buff, size, name );
	if ( status == 0 )
		lua_startCall( L, 0, LUA_MULTRET );
	lua_setThread( L, pOld );
	//if ( status == 0 )
	//	lua_executeThreads( L );
	return status;
}

int lua_dostring (lua_State *L, const char *str) {
	return lua_dobuffer(L, str, strlen(str), str);
}


/*
** {======================================================
** Error-recover functions (based on long jumps)
** =======================================================
*/

static void message( lua_State *L, const char *s ) 
{
	const TObject *em = luaH_getglobal( L, LUA_ERRORMESSAGE );
	if ( em->GetType() == LUA_TFUNCTION ) 
	{
		// выводим сообщение в новом thread-е
		CObj<CLuaThread> pOld = L->pCT;
		ASSERT( pOld );
		lua_setThread( L, lua_newThread( L, "Message thread" ) );
		*LObj(L, L->pCT->top) = *em;
		incr_top;
		lua_pushstring( L, s );
		luaD_call( L, L->pCT->top-2, 0 );
		lua_setThread( L, pOld );
	}
}

static void SaveStackForErrorMessage( lua_State *pState, const char *s )
{
	NScript::luaLastError.stack.clear();
	NScript::luaLastError.szError = std::string( s );
	lua_Debug debugInfo;
	int nDepth = 1;
	while ( lua_getstack( pState, nDepth, &debugInfo ) != 0 )
	{
		if ( lua_getinfo ( pState, "lnuS", &debugInfo ) ) 
		{
			SLUAError::SLUAStackTrace trace;
			trace.nDepth = nDepth;
			if ( strlen( debugInfo.source ) < 20 )
				trace.szSource = debugInfo.source;
			else
				trace.szSource = "no file";
			if ( debugInfo.name != 0 )
				trace.szFunctionName = debugInfo.name;
			else
				trace.szFunctionName = "main scope";
			if ( debugInfo.linedefined >= 0 )
				trace.nDefinedAtLine = debugInfo.linedefined;
			else
				trace.nDefinedAtLine = 0;
			NScript::luaLastError.stack.push_back( trace );
		}
		//
		++nDepth;
	}
}

static void callErrorHook( lua_State *L )
{
	if ( L->pCT->errorHookRef != LUA_REFNIL && lua_getref(L, L->pCT->errorHookRef) )
	{
		const TObject *pHook = LObj(L, L->pCT->top-1);
		if ( pHook->GetType() == LUA_TFUNCTION ) 
		{
			CObj<CLuaThread> pOld = L->pCT;
			ASSERT( pOld );
			lua_setThread( L, lua_newThread( L, "error hook thread" ) );
			*LObj(L, L->pCT->top) = *pHook;
			incr_top;
			luaD_call( L, L->pCT->top-1, 0 );
			lua_setThread( L, pOld );
		}
	}
}

void lua_error( lua_State *L, const char *s )
{
	if ( s ) message( L, s );
	SaveStackForErrorMessage( L, s );
	callErrorHook( L );
	luaD_breakrun( L, LUA_ERRRUN );
}

void luaD_breakrun( lua_State *L, int errcode )
{
	if ( nTryCount > 0 )
		throw errcode;
	else
	{
//	ASSERT( 0 );
		L->pCT->bErrorInThread = true;
	}
}

/* }====================================================== */


