#include "stdafx.h"
//#include "CallStack.h"

#if BOOST_OS_WINDOWS
#include <commctrl.h>
#endif

#include <cstdint>

static HINSTANCE g_hInst = NULL ;
HINSTANCE GetBSUInstance()
{
	return g_hInst;
}

BOOL WINAPI DllMain ( HINSTANCE hInst, uint32_t dwReason, LPVOID )
{
	BOOL bRet = TRUE ;
	switch ( dwReason )
	{
	case DLL_PROCESS_ATTACH :
		// Save off the DLL hInstance.
		g_hInst = hInst ;
		InitCommonControls();
		// I don't need the thread notifications.
		DisableThreadLibraryCalls( g_hInst ) ;
		//#ifdef _DEBUG
		//            bRet = InternalMemStressInitialize ( ) ;
		//#endif
		break ;
	case DLL_PROCESS_DETACH :
		//#ifdef _DEBUG
		//            bRet = InternalMemStressShutdown ( ) ;
		//#endif
		break ;
	default                 :
		break ;
	}
	return ( bRet ) ;
}
