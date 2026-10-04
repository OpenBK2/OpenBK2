#pragma once

#include "lua.h"
typedef lua_CFunction CFunction;

struct SRegFunction
{
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	const char *name = {};
	CFunction func = {};
};

