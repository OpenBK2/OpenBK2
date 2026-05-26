#include "stdafx.h"
#include "scriptPtr.h"
#include "scriptCallLUA.h"
#include "scriptCommon.h"
#include "Script/lstate.h"
#include "System/RandomGen.h"
#include "Misc/StrProc.h"

#include <cinttypes>

//
namespace NScript
{
extern Script * GetScript();
static bool bConfigLoaded = false;
static bool bShowLuaLog = false;

static void SetModeFromConfig()
{
	//bShowLuaLog = NGlobal::GetVar( "lua_showlog", 0.0f );
}
static std::string luaOutUserData( void *pData, bool bWriteToConsole = true );

static void CommandShowScriptError( const std::string &szID, const std::vector<std::wstring> &paramsSet, void *pContext )
{
	if ( luaLastError.szError.empty() )
	{
		csSystem << "there are no script errors" << endl;
		return;
	}
	//
	csSystem << endl << "Script last error:" << endl;
	csSystem << CC_RED << "Script error: " << CC_GREY << luaLastError.szError << endl;
	std::vector< SLUAError::SLUAStackTrace >::iterator i;
	for ( i = luaLastError.stack.begin(); i != luaLastError.stack.end(); ++i )
	{
		csSystem << CC_RED << "\t" << (*i).nDepth;
		csSystem << CC_GREY << "\tfile: \"" << (*i).szSource << "\"";
		csSystem << ",   function \"" << (*i).szFunctionName << "\"";
		csSystem << ",   defined at line  " << (*i).nDefinedAtLine;
		csSystem << endl;
	}
}

void ScriptWarning( const std::string &message )
{
	csSystem << CC_RED << "Script warning: " << CC_GREY << message << endl;
}

void ScriptError( const std::string &message )
{
	csSystem << CC_RED << "Script error: " << CC_GREY << message << endl;
}

static void ShowLuaLog( const std::string &szName, intptr_t nThreadID, const std::vector<SLuaParams> &params, bool bParsed )
{
	if ( !bShowLuaLog )
		return;
	char buf[32];
	sprintf( buf, "%" PRIxPTR, nThreadID);
	csScript << CC_GREY << "  LUA(" << buf << "):  " << CC_ORANGE << szName << "(  ";
	std::string szLog;

	for ( std::vector<SLuaParams>::const_iterator i = params.begin(); i != params.end(); ++i )
	{
		if ( IsValid( i->p ) )
			szLog += luaOutUserData( i->p.GetPtr(), false ) + ", ";
		else
			szLog += i->s + ", ";
	}
	if ( !params.empty() )
	{
		szLog.resize( szLog.size() - 2 );
		szLog += ' ';
	}
	csScript << CC_WHITE << szLog;
	csScript << CC_ORANGE <<  " )";
	if ( !bParsed )
		csScript << " check failed";
	csScript << endl;
}

int luaGetParamCount( lua_State* pState )
{
	ASSERT( pState != 0 );
	if ( pState == 0 )
		return 0;
	//
	return GetScript()->GetTop();
}

bool luaPrepareData( lua_State* pState, const std::string &szFuncName, const std::string &szParams, Script **ppScript, std::vector<SLuaParams> *pParams )
{
	*ppScript = 0;
	ASSERT( pState != 0 );
	if ( pState == 0 )
		return false;
	*ppScript = GetScript();
	ASSERT( (*ppScript)->m_state == pState );
	if ( (*ppScript)->m_state != pState )
		return false;
	if ( !bConfigLoaded )
	{
		SetModeFromConfig();
		bConfigLoaded = true;
	}
	if ( szParams != "" && !(*ppScript)->CheckArgs( szParams.c_str(), szFuncName.c_str(), pParams, bShowLuaLog ) )
	{
		ShowLuaLog( szFuncName, (intptr_t)pState->pCT.GetPtr(), *pParams, false );
		return false;
	}
	ShowLuaLog( szFuncName, (intptr_t)pState->pCT.GetPtr(), *pParams, true );
	return true;
}

static bool luaOutDBUserData( const Script::Object &o )
{
	//
	return false;
}

static std::string IToA( int n )
{
	char buf[64];
	return itoa( n, buf, 10 );
}
static std::string IToA( float f )
{
	char buf[128];
	sprintf( buf, "%f.2", f );
	return buf;
}

static std::string luaOutUserData( void *pData, bool bWriteToConsole )
{
	if ( GetScript() == 0 )
		return "";
	if ( bWriteToConsole )
		csScript << CC_RED << "[Script] error: Unregistered CPtr or CObj target";
	else
		return "Ptr[Unregistered CPtr or CObj target]";

	return "";
}

int luaOut(lua_State* state)
{
	Script script(state);
	int nTop = script.GetTop();
	for ( int i = 1; i <= nTop; ++i )
	{
		Script::Object o = script.GetObject(i);
		if ( o.Tag() >= tagLuaCPtr )
		{
			switch ( o.Tag() )
			{
				case tagLuaCPtr:
				case tagLuaCObj:
					{
						if ( !luaOutDBUserData( o ) )
						{
							if ( o.Tag() == tagLuaCPtr )
								csScript << " CPtr -> ";
							else if ( o.Tag() == tagLuaCObj )
								csScript << " CObj -> ";
							luaOutUserData( luaGetPtr( o ) );
							csScript << endl;
						}
					}
					break;
				default:
					csScript << CC_RED << "[Script] error: Incorrect user tag ( " << o.Tag() << " )" << endl;
					break;
			}
		}
		else if ( o.IsUserData() )
		{
			csScript << CC_RED << "[Script] error: User data" << endl;
			ASSERT( 0 ); // We can't work with user data
			break;
		}
		else if ( o.IsNil() )
			csScript << "NIL";
		else if ( o.IsString() )
			csScript << o.GetString();
		else
			ASSERT(0);
	}
	csScript << endl;
	return 0;
}

int luaTableGetSize(lua_State* state)
{
	Script script(state);
	int nTop = script.GetTop();
	if ( nTop > 0 )
	{
		Script::Object o = script.GetObject( 1 );
		if ( o.IsTable() )
		{
			lua_pushnumber( state, o.GetTableSize() );
			return 1;
		}
	}
	lua_pushnil( state );
	return 1;
}

int luaRandom( lua_State* state )
{
	Script script(state);
	if ( !script.GetObject( 1 ).IsNumber() )
		script.PushNumber( 0 );
	else if ( script.GetTop() == 0 )
		{ script.PushNumber( NRandom::Random() ); RecordRandomCall(); }
	else if ( script.GetTop() > 1 )
		{ script.PushNumber( NRandom::Random( script.GetObject(1).GetInteger(), script.GetObject(2).GetInteger() ) ); RecordRandomCall(); }
	else
		return 1;
	return 1;
}

bool luaGetBool( const Script::Object &o )
{
	if ( o.IsNil() )
		return false;
	else
		return true;
}

void luaMakeCallParamsVector( char *szParams, va_list *pL, std::vector< CObj<CLUACallParam> > *pParams )
{
	pParams->clear();
	for ( char *pCh = szParams; *pCh != ( char )0; ++pCh )
	{
		switch ( *pCh )
		{
			case 'i':
				pParams->push_back( new CLUACallParam( va_arg( *pL, int ) ) );
				break;
			case 'f':
				pParams->push_back( new CLUACallParam( va_arg( *pL, float ) ) );
				break;
			case 's':
				pParams->push_back( new CLUACallParam( std::string( va_arg( *pL, char * ) ) ) );
				break;
			case 'p':
				pParams->push_back( new CLUACallParam( va_arg( *pL, CObjectBase * ) ) );
				break;
			default:
				ASSERT( 0 );
				pParams->push_back( new CLUACallParam() );
				break;
		}
	}
}

static const int N_NO_TOP = -0xFFF;
//
static bool luaPushCallParameters( const std::string &szName, const std::vector< CObj<CLUACallParam> > &params, lua_State *pState )
{
	ASSERT( pState );
	if ( !pState )
		return false;
	//
	int nLuaTop = lua_gettop( pState );
	StkId stackTop = pState->pCT->top;
  lua_getglobal( pState, szName.c_str() );
	if ( lua_isfunction( pState, -1 ) )
	{
		for ( std::vector< CObj<CLUACallParam> >::const_iterator i = params.begin(); i != params.end(); ++i )
		{
			switch ( (*i)->type )
			{
				case CLUACallParam::PT_INT:
					lua_pushnumber( pState, (*i)->nInt );
					break;
				case CLUACallParam::PT_FLOAT:
					lua_pushnumber( pState, (*i)->fFloat );
					break;
				case CLUACallParam::PT_STRING:
					lua_pushstring( pState, (*i)->szString.c_str() );
					break;
				case CLUACallParam::PT_POINTER:
					luaPushCPtr( pState, (*i)->pObject );
					break;
				default:
					lua_pushnil( pState );
					ASSERT( 0 ); // unknown parameter type
					continue;
			}
		}
		//
		ASSERT( lua_gettop( pState ) - nLuaTop == params.size() + 1 ); // not all params was pushed in the stack
		return true;
	}
	else
	{
		lua_pop( pState, 1 ); // pop function from stack
		ASSERT( pState->pCT->top == stackTop ); // stack corrupted
		return false;
	}
}

void luaCallFunction( const std::string &szName, const std::vector< CObj<CLUACallParam> > &params )
{
	lua_State *pState = 0;
	Script *pScript = GetScript();
	if ( pScript )
		pState = pScript->GetState();
	if ( !pState )
	{
		ASSERT( 0 );
		ScriptError( "script is unavailable at the moment" );
		return;
	}
	//
	StkId stackTop = pState->pCT->top;
	//
	CLuaThread *pOld = pState->pCT;
	ASSERT( pOld );
	lua_setThread( pState, lua_newThread( pState, szName.c_str() ) );
	if ( luaPushCallParameters( szName, params, pState ) )
		lua_startThread( pState, params.size() );
	lua_setThread( pState, pOld );
	ASSERT( pState->pCT->top == stackTop ); // stack corrupted or current thread was changed
}

void luaCallFunction( const std::string &szName, char *szParams, ... )
{
	std::vector< CObj<CLUACallParam> > params;
	va_list l;
	va_start( l, szParams );
	luaMakeCallParamsVector( szParams, &l, &params );
	va_end( l );
	luaCallFunction( szName, params );
}

//BEGIN_SCRIPT_COMMAND( LuaTest, "ns[Hello!]n[100]b[false]b[true]" )
BEGIN_SCRIPT_COMMAND( LuaTest, "" )
	//csSystem << luaParams[ 0 ].n << endl;
	//csSystem << luaParams[ 1 ].s << endl;
	//csSystem << luaParams[ 2 ].f << endl;
	//csSystem << luaParams[ 3 ].b << endl;
	//csSystem << luaParams[ 4 ].b << endl;


	luaCallFunction( "UnexistentFunction", "ii", 100, 200 );
	return 0;
END_SCRIPT_COMMAND

BEGIN_SCRIPT_COMMAND( IsEqual, "uu" )
	if ( luaParams[ 0 ].p == luaParams[ 1 ].p )
		pScript->PushNumber( 1 );
	else
		pScript->PushNil();
	return 1;
END_SCRIPT_COMMAND

BEGIN_SCRIPT_COMMAND( GetGlobalVar, "ss[]" )
	std::string szDef = luaParams[ 1 ].s;
	std::string sz = NStr::ToMBCS( NGlobal::GetVar( luaParams[ 0 ].s, szDef ) );
	if ( sz == "" )
		pScript->PushNil();
	else
		pScript->PushString( sz.c_str() );
	return 1;
END_SCRIPT_COMMAND

BEGIN_SCRIPT_COMMAND( SetGlobalVar, "ss" )
	NGlobal::SetVar( luaParams[ 0 ].s, luaParams[ 1 ].s );
	return 0;
END_SCRIPT_COMMAND

void luaPushBool( lua_State *pState, bool bValue )
{
	if ( bValue )
		lua_pushnumber( pState, 1 );
	else
		lua_pushnil( pState );
}

void luaCreateCPtrVar( const std::string &szVarName, CObjectBase *pObj )
{
	lua_State *pState = 0;
	Script *pScript = GetScript();
	if ( pScript )
		pState = pScript->GetState();
	if ( !pState )
	{
		ScriptError( "script is unavailable at the moment" );
		return;
	}
	if ( IsValid( pObj ) )
	{
		luaPushCPtr( pState, pObj );
    lua_setglobal (pState, szVarName.c_str() );
		csSystem << CC_GREY << "Object saved to variable " << szVarName << endl;
	}
}

START_REGISTER(Script)
	////
FINISH_REGISTER
}
//
using namespace NScript;
REGISTER_SAVELOAD_CLASS( 0x11061442, CLUACallParam )

