#include "StdAfx.h"
/**

typedef ISaveLoadSystem* (STDCALL *GETSLS_HOOK)();
typedef ISingleton* (STDCALL *GETSINGLETONGLOBAL_HOOK)();
typedef void* (STDCALL *GETTEMPRAWBUFFER_HOOK)( int nAmount, int nBufferIndex );

#ifdef _STREAMIO_DLL
ISaveLoadSystem* STDCALL GetSLS_Hook();
ISingleton* STDCALL GetSingletonGlobal_Hook();
void* STDCALL GetTempRawBuffer_Hook( int nSize, int nIndex );
#endif // _STREAMIO_DLL

IRandomGen *g_pGlobalRandomGen = 0;
ISaveLoadSystem *g_pGlobalSaveLoadSystem = 0;
ISingleton *g_pGlobalSingleton = 0;
GETTEMPRAWBUFFER_HOOK g_pfnGlobalGetTempRawBuffer = 0;

static list<NWin32Helper::CDLLHandle*> dllhandles;

NWin32Helper::CDLLHandle* LoadModule( const char *pszName )
{
	NWin32Helper::CDLLHandle *pDLL = new NWin32Helper::CDLLHandle( pszName );
	if ( !pDLL->IsLoaded() ) 
	{
		delete pDLL;
		return 0;
	}
	return pDLL;
}

class CGlobalsLoader
{
public:
	CGlobalsLoader()
	{
		// load StreamIO.dll
#ifndef _DONT_LOAD_STREAMIO
		NWin32Helper::CDLLHandle *pStreamIO = LoadModule( "streamio.dll" );
		// hook GetGlobalSaveLoadSystem() function
		if ( GETSLS_HOOK pfnGetSLS_Hook = pStreamIO->GetProcAddress( "GetSLS_Hook", (GETSLS_HOOK)0 ) ) 
			g_pGlobalSaveLoadSystem = (*pfnGetSLS_Hook)();
		// hook GetSingletonGlobal() function
		if ( GETSINGLETONGLOBAL_HOOK pfnGetSingletonGlobal_Hook = pStreamIO->GetProcAddress( "GetSingletonGlobal_Hook", (GETSINGLETONGLOBAL_HOOK)0 ) ) 
			g_pGlobalSingleton = (*pfnGetSingletonGlobal_Hook)();
		// hook GetTempRawBuffer() function
		g_pfnGlobalGetTempRawBuffer = pStreamIO->GetProcAddress( "GetTempRawBuffer_Hook", (GETTEMPRAWBUFFER_HOOK)0 );

		dllhandles.push_back( pStreamIO );
#endif // _DONT_LOAD_STREAMIO
		//
#ifdef _STREAMIO_DLL
		g_pGlobalSaveLoadSystem = GetSLS_Hook();
		g_pGlobalSingleton = GetSingletonGlobal_Hook();
		g_pfnGlobalGetTempRawBuffer = GetTempRawBuffer_Hook;
#endif // _STREAMIO_DLL
		//
		//
#ifndef _DONT_LOAD_SINGLETONS		
		ISingleton *pSingleton = GetSingletonGlobal();
		if ( pSingleton )
		{
			g_pGlobalRandomGen = GetSingleton<IRandomGen>( pSingleton );
		}
#endif // _DONT_LOAD_SINGLETONS		
	}
	//
	//
	//
	~CGlobalsLoader()
	{
		for ( list<NWin32Helper::CDLLHandle*>::iterator it = dllhandles.begin(); it != dllhandles.end(); ++it )
			delete (*it);
		dllhandles.clear();
	}
};

static CGlobalsLoader theGlobalsLoader;

/**/
