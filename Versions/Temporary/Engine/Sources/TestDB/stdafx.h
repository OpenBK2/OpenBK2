// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//


#pragma once
#ifndef __AFX__
#define WIN32_LEAN_AND_MEAN							// Exclude rarely-used stuff from Windows headers

#define _WIN32_WINNT 0x400
#include <windows.h>
#include <typeinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
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
#include <math.h>
#include <memory.h>
#include <string.h>
// 
#include "../Misc/Asserts.h"
//
#pragma warning( disable: 4018 4355 4800 4244 4267 )
#pragma warning( disable: 4127 4100 4201 4512 4389 )
#ifdef NIVAL_DLL
#pragma warning( disable: 4273)
#endif

#include "../Misc/nlist.h"
#pragma component( mintypeinfo, off )
#include "../Misc/nstring.h"
#include "../Misc/nvector.h"
#pragma component( mintypeinfo, on )
#include "../Misc/nhash_map.h"
#include "../Misc/nhash_set.h"
#include "../Misc/nset.h"
#pragma component( mintypeinfo, off )
//#pragma warning( disable : 4503 4018 4786 4800 4290 4146 4244 4284 4267 )

using namespace nstl;

#include "../Misc/nhelpdebug.h"

//
typedef __int64 int64;									// due to lack of 'long long' type support
typedef unsigned __int64 QWORD;					// quadra word
#define for if(false); else for					// to achive standard variable scope resolving, declared inside 'for'

namespace NTimer
{
	typedef DWORD STime;
};
//
#include "../System/System.h"
#include "../Misc/Tools.h"
#include "../System/Basic.h"
#include "../Misc/Geom.h"
#include "../System/StreamIO.h"
#include "../System/Streams.h"
#include "../System/BinSaver.h"
#include "../System/XmlSaver.h"
#include "../System/GlobalVars.h"
#include "../System/ConsoleBuffer.h"
#include "../System/LogStream.h"
#include "../System/DB.h"
// in the file 'Specific.h' one can define ow n project-specific includes
#include "Specific.h"

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


