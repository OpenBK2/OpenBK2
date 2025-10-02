#include "StdAfx.h"



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

