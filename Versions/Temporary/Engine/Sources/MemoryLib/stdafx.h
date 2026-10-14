// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once
#define _STLP_NO_THREADS

#ifdef _NSTL_HELP_DEBUG
#undef _NSTL_HELP_DEBUG
#endif

// normal stdafx.h
// The order of these includes is load-bearing and is not sorted. These are
// hub headers that do not all include what they use: Streams.h declares
// CMemoryStream, which BinSaver.h names without including it, so swapping
// the two stops the module compiling. Until the headers carry their own
// dependencies, this list stays in the order somebody worked out.
// clang-format off
#include <boost/predef.h>

#if BOOST_OS_WINDOWS
#include <windows.h>
#include <objbase.h>
#endif

#include <cassert>

#include "port/debugging.h"
#include "port/messagebox.h"
// clang-format on

#ifdef _DEBUG
#ifdef FAST_DEBUG
#define ASSERT( a ) \
	if ( !( a ) )     \
		breakpoint();
#else
#define ASSERT( aParam )                                                         \
	if ( !( aParam ) )                                                             \
	{                                                                              \
		char szBuf[1024];                                                            \
		sprintf( szBuf, "%s(%d) assertion %s failed", __FILE__, __LINE__, #aParam ); \
		MessageBox( 0, szBuf, "Error", MB_OK );                                      \
		breakpoint();                                                                \
	}
#endif
#else
#define ASSERT( a ) ( (void)0 )
#endif
