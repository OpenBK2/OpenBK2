#include "stdafx.h"
//
#include "scriptPtr.h"
#include "scriptCommon.h"

#define REG_FUNCTION( Name ) { #Name, lua##Name }
//
namespace NScript
{
static int Error_out( lua_State* state )
{
	Script script( state );
	Script::Object obj = script.GetObject(script.GetTop());
	csSystem << CC_RED << "Script error: " << CC_GREY << obj.GetString() << endl;
	return 0;
}

SRegFunction pCommonRegList[] =
{
	// Common functions
	{ "_ERRORMESSAGE", Error_out },
	{ "out", luaOut },
	{ "Sleep", LuaCFuncSleep },
	{ "StartThread", LuaCFuncStartThread },
	{ "random", luaRandom },

	// Ptr
	{ "Ptr", luaMakeCPtr },
	{ "ObjPtr", luaMakeCObj },
	{ "IsValid", luaIsValid },
	REG_FUNCTION( IsEqual ),
	REG_FUNCTION( SetGlobalVar ),
	REG_FUNCTION( GetGlobalVar ),
	// Test
	REG_FUNCTION( LuaTest ),

	//
	{ 0, 0 } // End
};
//
}

