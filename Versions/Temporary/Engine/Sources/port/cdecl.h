#pragma once

#include <boost/predef.h>

// Portable spelling of the cdecl calling convention. See port/stdcall.h -- the
// same reasoning applies, and the two must stay consistent.
//
// Keyed on the target OS rather than on the compiler, because every Windows
// compiler we care about -- MSVC, clang-cl, clang targeting mingw-w64 and
// MinGW GCC -- accepts the __cdecl spelling.
//
// Compiler-based detection is wrong here: Boost.Predef includes
// compiler/clang.h before compiler/visualc.h and compiler/gcc.h, so under
// clang-cl both BOOST_COMP_MSVC and BOOST_COMP_GNUC are reported as the
// *_EMULATED variants and evaluate to 0, leaving PORT_CDECL empty.
//
// Getting this wrong is less damaging than for stdcall -- cdecl is MSVC's
// default under /Gd, which this project uses -- but the declarations it guards
// include IBinSaver's TestDataPath overload set, which selects the on-disk
// save format, so it is worth being exact.
#if BOOST_OS_WINDOWS
#define PORT_CDECL __cdecl
#else
// Elsewhere the SysV ABIs have one calling convention; expand to nothing.
#define PORT_CDECL
#endif
