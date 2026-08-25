#pragma once

#include <boost/predef.h>

// CPUID and XGETBV, spelled the same way on every compiler.
//
// Both are compiler intrinsics rather than library functions, so they need a
// header, and the two spellings do not line up:
//
//   MSVC <intrin.h>   __cpuid( int[4], leaf )
//                     __cpuidex( int[4], leaf, subleaf )
//                     _xgetbv( xcr ) -> unsigned __int64
//   GCC  <cpuid.h>    __cpuid( leaf, eax, ebx, ecx, edx )
//                     __cpuid_count( leaf, subleaf, eax, ebx, ecx, edx )
//
// The GCC forms are macros taking the four registers as separate lvalues, so
// they cannot be aliased onto the MSVC forms; a wrapper is the only way to get
// one spelling. These take the MSVC argument order because that is what the
// existing call sites are written against.
//
// Keyed on the target OS rather than on the compiler, for the reason
// port/stdcall.h spells out: under clang-cl both BOOST_COMP_MSVC and
// BOOST_COMP_GNUC are the *_EMULATED variants and evaluate to 0, so a compiler
// test would include neither header.
//
// x86 only, as the callers are: everything here is a dispatch between MMX, SSE2
// and AVX2 kernels, and there is nothing to ask CPUID about on a target that has
// none of them.
#if BOOST_OS_WINDOWS
#include <intrin.h>
#else
#include <cpuid.h>
#endif

//! CPUID for a leaf that takes no subleaf. Fills eax, ebx, ecx, edx in that order.
inline void cpuid( int cpuInfo[4], int nLeaf )
{
#if BOOST_OS_WINDOWS
	__cpuid( cpuInfo, nLeaf );
#else
	__cpuid( nLeaf, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3] );
#endif
}

//! CPUID for a leaf that takes a subleaf in ecx.
inline void cpuid_count( int cpuInfo[4], int nLeaf, int nSubLeaf )
{
#if BOOST_OS_WINDOWS
	__cpuidex( cpuInfo, nLeaf, nSubLeaf );
#else
	__cpuid_count( nLeaf, nSubLeaf, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3] );
#endif
}

//! Read an extended control register, to find out which state the OS has agreed
//! to preserve across a context switch. Faults unless CPUID reported OSXSAVE,
//! so check that first.
inline unsigned long long xgetbv( unsigned int nXcr )
{
#if BOOST_OS_WINDOWS
	return _xgetbv( nXcr );
#else
	// Written out rather than calling the _xgetbv that GCC and clang put in
	// <immintrin.h>: theirs is gated on __XSAVE__, so it would need -mxsave on
	// the whole translation unit, and a dispatch file is the one place that must
	// stay buildable for the baseline target. The instruction itself is always
	// encodable; whether it faults is a runtime question, which is what the
	// OSXSAVE check answers.
	unsigned int nLow = 0, nHigh = 0;
	__asm__ __volatile__( "xgetbv" : "=a"( nLow ), "=d"( nHigh ) : "c"( nXcr ) );
	return ( static_cast<unsigned long long>( nHigh ) << 32 ) | nLow;
#endif
}
