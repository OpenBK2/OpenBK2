// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//


#pragma once
#ifndef __AFX__

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

#include <afxwin.h>											// MFC core and standard components
#include <afxext.h>											// MFC extensions
#include <afxdtctl.h>										// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>											// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <comutil.h>
#endif // __AFX__

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

namespace NTimer
{
	typedef DWORD STime;
};
//
#include "System/System.h"
#include "Misc/Tools.h"
#include "System/Basic.h"
#include "Misc/Geom.h"
#include "System/StreamIO.h"
#include "System/Streams.h"
#include "System/BinSaver.h"
#include "System/XmlSaver.h"
#include "System/GlobalVars.h"
#include "System/ConsoleBuffer.h"
#include "System/LogStream.h"
#include "System/DB.h"
// in the file 'Specific.h' one can define ow n project-specific includes
#include "Specific.h"

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
