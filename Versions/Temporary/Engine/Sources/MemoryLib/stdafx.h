// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//


#pragma once
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _STLP_NO_THREADS

#ifdef _NSTL_HELP_DEBUG
#undef _NSTL_HELP_DEBUG
#endif

// fake new.h
#include "DumbPow2Alloc.h"

// normal stdafx.h

#include <windows.h>
#include <objbase.h>
#include <assert.h>


#ifdef _DEBUG
#  ifdef FAST_DEBUG
#    define ASSERT( a ) if ( !(a) ) __debugbreak();
#  else
#    define ASSERT( aParam ) if ( !(aParam) ) { char szBuf[1024]; sprintf( szBuf, "%s(%d) assertion %s failed", __FILE__, __LINE__, #aParam ); MessageBox( 0, szBuf, "Error", MB_OK ); __debugbreak(); }
#  endif
#else
#  define ASSERT( a ) ((void)0)
#endif

void __cdecl DebugTraceMMgr( const char *pszFormat, ... );


