#pragma once

#include <boost/predef.h>

// Portable spelling of the stdcall calling convention.
//
// Keyed on the target OS rather than on the compiler, because every Windows
// compiler we care about -- MSVC, clang-cl, clang targeting mingw-w64 and
// MinGW GCC -- accepts the __stdcall spelling.
//
// Compiler-based detection is actively wrong here: Boost.Predef includes
// compiler/clang.h before compiler/visualc.h and compiler/gcc.h, so under
// clang-cl both BOOST_COMP_MSVC and BOOST_COMP_GNUC are reported as the
// *_EMULATED variants and evaluate to 0. A test on those would silently expand
// PORT_STDCALL to nothing and break x86 linkage, where the decorated name
// _ReportAssert@16 would no longer match the declared _ReportAssert.
// (BOOST_COMP_MSVC_AVAILABLE would work but is a valueless #define, so it needs
// defined() rather than #if -- another easy way to get this wrong.)
//
// On 64-bit Windows there is a single calling convention and __stdcall is
// accepted and ignored, so this needs no separate architecture case.
#if BOOST_OS_WINDOWS
#define PORT_STDCALL __stdcall
#else
// Elsewhere the SysV ABIs have one calling convention; expand to nothing.
#define PORT_STDCALL
#endif
