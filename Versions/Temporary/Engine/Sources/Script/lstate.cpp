#include "stdafx.h"
/*
** $Id: lstate.c,v 1.48 2000/10/30 16:29:59 roberto Exp $
** Global State
** See Copyright Notice in lua.h
*/


#include <cstdio>


#include "lgc.h"
#include "llex.h"
#include "lstate.h"
#include "ltable.h"
#include "ltm.h"


#ifdef _DEBUG
static lua_State *lua_state = NULL;
void luaB_opentests (lua_State *L) {}
#endif

//
//
//
const std::string MakeUniqueName( lua_State *L, const char *name )
{
	return (L->szName.empty() ? name : L->szName + "." + name);
}

//
// Makes global named name for function f and registers f for serialization.
//
void lua_register( lua_State *L, const char *name, lua_CFunction f )
{
	lua_pushcclosure(L, f, 0);
	lua_setglobal(L, name);
	lua_RegisterFunc(f, MakeUniqueName(L, name));
}

//
// built-in implementation for ERRORMESSAGE. In a "correct" environment
// ERRORMESSAGE should have an external definition, and so this function
// would not be used.
//
static int errormessage (lua_State *L)
{
	const char *s = lua_tostring(L, 1);
	if (s == NULL) s = "(no message)";
	DebugTrace( "error: %s\n", s );
	return 0;
}

/*
** open parts that may cause memory-allocation errors
*/
static void f_luaopen (lua_State *L, void *ud)
{
	int stacksize = *(int *)ud;
	if (stacksize == 0)
		stacksize = DEFAULT_STACK_SIZE;
	else
		stacksize += LUA_MINSTACK;
	L->nGT = luaH_new(L, 10);	/* table of globals */

	DebugTrace( "Opening LUA stack f_luaOpen" );
	luaX_init(L);
	luaT_init(L);
	lua_newtable(L);
	lua_ref(L, 1);	/* create registry */
	lua_register(L, LUA_ERRORMESSAGE, errormessage);
#ifdef _DEBUG
	luaB_opentests(L);
	if (lua_state == NULL) lua_state = L;	/* keep first state to be opened */
#endif
	LUA_ASSERT(lua_gettop(L) == 0, "wrong API stack");
}


lua_State *lua_open (int stacksize, const char* instanceName)
{
	lua_State *L = new lua_State;
	if (L == NULL) return NULL;	/* memory allocation error */
	if (instanceName)
		L->szName = instanceName;
	L->refFree = NONEXT;
	L->nGCAvoid = 1;	/* to avoid GC during pre-definitions */
	L->callhook = NULL;
	L->linehook = NULL;
	L->allowhooks = 1;
	L->nGCticks = 0;
	L->nNoWait = 0;
	CLuaThread *pThr = lua_newThread( L, "First thread" );
	lua_setThread( L, pThr );
	f_luaopen( L, &stacksize );
	L->nGCAvoid = 0;
 	DebugTrace( "Opening LUA stack (new thread)" );
	return L;
}

void lua_close (lua_State *L)
{
	luaC_collect(L, 1);	/* collect all elements */
	
	LUA_ASSERT( L->protos.empty(), "list should be empty" );
	LUA_ASSERT( L->closures.empty(), "list should be empty");
	LUA_ASSERT( L->tables.empty(), "list should be empty");
	L->callInfos.clear();

	DebugTrace( "Freeing LUA stack" );
	delete L;
}


CLuaThread::CLuaThread() : L(0), errorHookRef(LUA_REFNIL)
{
}

CLuaThread::CLuaThread( lua_State *_L, const char *_name )
	: top(0), Cbase(0), thisThreadIsSleeping(0), bErrorInThread(false),
	  L(_L), errorHookRef(LUA_REFNIL)
{
	stack.resize( DEFAULT_STACK_SIZE, TObject() );
	if ( _name )
	{
		name = _name;
	}
}

CLuaThread::~CLuaThread()
{
	clearErrorHook();
}

void CLuaThread::setErrorHook()
{
	clearErrorHook();
	errorHookRef = lua_ref(L, 1);
}

void CLuaThread::clearErrorHook()
{
	lua_unref(L, errorHookRef);
	errorHookRef = LUA_REFNIL;
}


void lua_setThread( lua_State *L, CLuaThread *pThread )
{
	ASSERT( IsValid( pThread ) );
	L->pCT = pThread;
}

CLuaThread* lua_newThread( lua_State *L, const char* name )
{
	CLuaThread* pThr = new CLuaThread( L, name );
	L->threads.push_back( pThr );
	return pThr;
}


//				Serialize

template<class T>
int CVectorList<T>::operator&( IBinSaver &f )
{
	if ( f.IsReading() )
		clear();
	f.Add( 2, &nFirstFree );
	int nSize = data.size();
	f.Add( 3, &nSize );
	if ( f.IsReading() )
		data.resize( nSize );
	for ( int i = 0; i < nSize; ++i )
		f.Add( 4, &data[i], i + 1 );
	return 0;
}


lua_State::~lua_State()
{
	for ( CThreads::iterator i = threads.begin(); i != threads.end(); ++i )
	{
		CLuaThread *pThread = *i;
		if ( IsValid(pThread) )
		{
			pThread->clearErrorHook();
		}
	}
}

int lua_State::operator&( IBinSaver &f )
{
	lua_StartSerialize( this );
	f.Add( 2, &pCT );
	f.Add( 3, &Mbuffer );
	f.Add( 4, &threads );
	f.Add( 5, &nGT );
	f.Add( 6, &TMtable );
	f.Add( 7, &refArray );
	f.Add( 8, &refFree );
	f.Add( 9, &nGCticks );
	f.Add( 10, &nGCAvoid );
	f.Add( 11, &allowhooks );
	f.Add( 12, &nNoWait );
	f.Add( 13, &strings );

	f.Add( 14, &protos );
	f.Add( 15, &closures );
	f.Add( 16, &tables );
	f.Add( 17, &userdatas );
	f.Add( 18, &callInfos );

	lua_AddHook( &callhook, f, 19 );
	lua_AddHook( &linehook, f, 20 );

	f.Add( 21, &pContext );
	f.Add( 22, &szName );

	// restoring threads to lua_state connections
	if ( f.IsReading() )
	{
		for ( CThreads::iterator i = threads.begin(); i != threads.end(); ++i )
		{
			CLuaThread *pThread = *i;
			if ( IsValid(pThread) )
			{
				pThread->L = this;
			}
		}
	}

	return 0;
}

REGISTER_SAVELOAD_CLASS( SCRIPT, 0x70813170, CLuaThread );

