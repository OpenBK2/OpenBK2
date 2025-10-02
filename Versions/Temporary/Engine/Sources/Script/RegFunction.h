#pragma once

#include "lua.h"
typedef lua_CFunction CFunction;

struct SRegFunction
{
	const char *name;
	CFunction func;
};

