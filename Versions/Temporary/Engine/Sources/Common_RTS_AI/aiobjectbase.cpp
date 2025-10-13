#include "stdafx.h"
#include <cstdio>

/*
EXTERNVAR void (*pfnPenterCheck)();
EXTERNVAR bool bAllowPenterInstance;
EXTERNVAR bool bAllowPenterGlobal;

bool bAllowPenterInstance = false;		// allow single penter call
bool bAllowPenterGlobal = false;			// allow penter calls

void (*pfnPenterCheck)() = 0;

extern "C" void __declspec(naked) _cdecl _penter( void )
{
_asm
{
pusha
}

if ( bAllowPenterInstance )
{
bAllowPenterInstance = false;
if ( pfnPenterCheck )
(*pfnPenterCheck)();
}

_asm
{
popa
ret
}

*/
/*
typedef CObjectBase * PObjectBase;

PObjectBase & GetLast()
{
	static CObjectBase * pLast = 0;
	return pLast;
}

void CheckLast()
{
	static CCheckSumSaver saver;
	
	if ( GetLast() )
	{
		if ( NObjectFactory::IsRegistered( NObjectFactory::GetObjectTypeID( GetLast() ) ) )
		{
			GetLast()->operator &( saver );
			GetLast() = 0;
		}
	}
}

CAIObjectBase::CAIObjectBase()
{
	//pfnPenterCheck = CheckLast;
	if ( GetLast() != this )
		CheckLast();
	GetLast() = this;
	//bAllowPenterInstance = bAllowPenterGlobal;
}


*/

