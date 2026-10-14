// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once
#ifndef __AFX__

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

#include <typeinfo>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#else
//#define _STLP_USE_MFC 1

#include <afxwin.h>   // MFC core and standard components
#include <afxext.h>   // MFC extensions
#include <afxdtctl.h> // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h> // MFC support for Windows Common Controls
#endif              // _AFX_NO_AFXCMN_SUPPORT
#include <comutil.h>
#endif // __AFX__

#pragma component( mintypeinfo, on )
#include <cmath>
#include <cstring>
//
#include "Misc/Asserts.h"
//
#pragma warning( disable : 4018 4355 4800 4244 4267 )
#pragma warning( disable : 4127 4100 4201 4512 4389 )
#ifdef NIVAL_DLL
#pragma warning( disable : 4273 )
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
// clang-format on

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
