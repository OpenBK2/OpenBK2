#include "StdAfx.h"

#pragma comment( lib, "N:\\Dev\\Soft\\Utils\\ModuleHook.lib" )

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C" void __cdecl ReferenceAllModules();
void DoReferenceAllModules()
{
	ReferenceAllModules();
}

