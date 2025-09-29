#include "StdAfx.h"



HINSTANCE theEDCommonInstance = 0;
#ifdef NIVAL_DLL
BOOL WINAPI DllMain( HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved )
{
	if ( ul_reason_for_call == DLL_PROCESS_ATTACH )
	{
		// Для подключения ресурсов из DLL
		theEDCommonInstance = (HINSTANCE)hInst;
	}
	return true;
}
#else
static struct SInitCommonDll {
	SInitCommonDll() { theEDCommonInstance = GetModuleHandle( 0 ); }
} init;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ManualLoadEDCommonLibrary()
{
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


