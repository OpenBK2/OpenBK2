// The prelude TestType.cpp is compiled with; dbcodegen emits #include "stdafx.h"
// at the top of every generated source.

#pragma once

#include <boost/predef.h>

#if BOOST_OS_WINDOWS
#include <windows.h>
#endif

#include <typeinfo>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <cstring>

#include "Misc/Asserts.h"

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

#include "System/System.h"
#include "Misc/Tools.h"
#include "System/Basic.h"
#include "Misc/Geom.h"
#include "System/Streams.h"
#include "System/BinSaver.h"
#include "System/XmlSaver.h"
#include "System/GlobalVars.h"
#include "System/DB.h"
#include "Specific.h"
