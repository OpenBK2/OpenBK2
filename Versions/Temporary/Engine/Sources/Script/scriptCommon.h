#pragma once

#include "Script.h"
//
union ULuaParams;
//
//
namespace NScript
{

#define DECLARE_SCRIPT_COMMAND( Name )											\
int lua##Name( lua_State* pState );
#define BEGIN_SCRIPT_COMMAND( Name, Params )								\
int lua##Name( lua_State* pState )																			\
{																																				\
	ASSERT( pState != 0 );																								\
	if ( pState == 0 )																										\
		return 0;																														\
	Script *pScript;																											\
	std::vector<SLuaParams> luaParams;																					\
	if ( !luaPrepareData( pState, #Name, Params, &pScript, &luaParams ) )	\
		return 0;																														
#define END_SCRIPT_COMMAND }


DECLARE_SCRIPT_COMMAND( Out );
DECLARE_SCRIPT_COMMAND( Random );
DECLARE_SCRIPT_COMMAND( IsRealTime );
DECLARE_SCRIPT_COMMAND( IsEqual );
DECLARE_SCRIPT_COMMAND( GetGlobalVar );
DECLARE_SCRIPT_COMMAND( SetGlobalVar );
DECLARE_SCRIPT_COMMAND( LuaTest );

bool luaGetBool( const Script::Object &o );
void luaPushBool( lua_State *pState, bool bValue );
void ScriptWarning( const std::string &message );
void ScriptError( const std::string &message );
}
//

