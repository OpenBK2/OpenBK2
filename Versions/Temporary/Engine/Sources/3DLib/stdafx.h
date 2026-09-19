// stdafx.h : include file for standard system include files,
//	or project specific include files that are used frequently, but
//			are changed infrequently
//


#pragma once
// No MFC: nothing links it, so the __AFX__ branch that
// included afxwin.h and friends is gone.

#include <boost/predef.h>

#if BOOST_OS_WINDOWS
#include <windows.h>
#endif

#include <typeinfo>
#include <cstdio>
#include <cstdlib>
#include <cassert>


#pragma component( mintypeinfo, on )

#include <cmath>
#include <cstring>
// 
#include "Misc/Asserts.h"
//
#pragma warning( disable: 4018 4355 4800 4244 4267 )
#pragma warning( disable: 4127 4100 4201 4512 4389 )
#ifdef NIVAL_DLL
#pragma warning( disable: 4273)
#endif

#include <list>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <cstdint>

namespace NTimer
{
	typedef uint32_t STime;
}
//
#include "System/System.h"
#include "Misc/Tools.h"
#include "System/Basic.h"
#include "Misc/Geom.h"
#include "System/Streams.h"
#include "System/BinSaver.h"
#include "System/GlobalVars.h"
#include "System/ConsoleBuffer.h"
#include "System/LogStream.h"
#include "System/DB.h"
// in the file 'Specific.h' one can define ow n project-specific includes
#include "Specific.h"

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
