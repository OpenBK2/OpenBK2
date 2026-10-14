// stdafx.h : include file for standard system include files,
//	or project specific include files that are used frequently, but
//			are changed infrequently
//

#pragma once
// No MFC: nothing in the editor links it, so the __AFX__ branch that
// included afxwin.h and friends is gone.

// The order of these includes is load-bearing and is not sorted. These are
// hub headers that do not all include what they use: Streams.h declares
// CMemoryStream, which BinSaver.h names without including it, so swapping
// the two stops the module compiling. Until the headers carry their own
// dependencies, this list stays in the order somebody worked out.
// clang-format off
#include <boost/predef.h>

// Off Windows this is DXVK Native's windows.h, on the include path through
// cmake/dxvk.cmake. It gives the Win32 types and macros the editor names
// throughout (HWND, HINSTANCE, COLORREF, RGB, MAX_PATH) as opaque types, and
// none of the functions: those calls get real replacements instead.
#include <windows.h>

#pragma component( mintypeinfo, on )
// System/System.h names std::type_info.
#include <typeinfo>
#include <cstdio>
#include <cstdlib>
#include <cassert>
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
