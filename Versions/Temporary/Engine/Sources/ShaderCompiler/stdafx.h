// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// The order of these includes is load-bearing and is not sorted. These are
// hub headers that do not all include what they use: Streams.h declares
// CMemoryStream, which BinSaver.h names without including it, so swapping
// the two stops the module compiling. Until the headers carry their own
// dependencies, this list stays in the order somebody worked out.
// clang-format off
#include <boost/predef.h>

#if BOOST_OS_WINDOWS
#include <windows.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <cassert>

#include "port/debugging.h"

#pragma warning( disable : 4267 4018 )

#undef ASSERT
#ifdef _DEBUG
#define ASSERT( a ) \
	if ( !( a ) )     \
		breakpoint();
#else
#define ASSERT( a ) ( (void)0 )
#endif
#define NI_ASSERT( a, b ) ASSERT( ( a ) && ( b ) )

#include <cstring>

#include <list>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>

#include <iostream>
#include <fstream>
// clang-format on

// The sources were written against an STL that put these in the global
// namespace. Using-declarations rather than `using namespace std`, because the
// latter makes std::byte collide with the Windows SDK's byte in rpcndr.h.
using std::string;
using std::vector;
using std::list;

inline std::ostream &operator<<( std::ostream &a, const string &sz )
{
	return a << sz.c_str();
}

//inline std::ostream &operator<<( std::ostream &a, const char *psz ) { return a << psz; }

using std::cout;
using std::endl;
using std::hex;
using std::dec;
using std::ofstream;
//#include <vector>
//using namespace std;
//#include <tchar.h>

// TODO: reference additional headers your program requires here
