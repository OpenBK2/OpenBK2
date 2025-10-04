// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#pragma warning( disable: 4267 4018 )

#undef ASSERT
#ifdef _DEBUG
#  define ASSERT( a ) if ( !(a) ) __debugbreak();
#else
#  define ASSERT( a ) ((void)0)
#endif
#define NI_ASSERT(a,b) ASSERT( (a) && (b) )

#include <string.h>
#include "Misc/nvector.h"
#include "Misc/nlist.h"
#include "Misc/nstring.h"

#pragma component( mintypeinfo, on )
#include "Misc/nhash_map.h"
#include "Misc/nhash_set.h"
#include "Misc/nset.h"
#pragma component( mintypeinfo, off )

using namespace nstl;
#include "Misc/nhelpdebug.h"

#include <iostream>
#include <fstream>

inline std::ostream &operator<<( std::ostream &a, const string &sz ) { return a << sz.c_str(); }
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

