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
// Deliberately not GCC's __get_cpuid and __get_cpuid_count, which do the
// max-leaf check themselves and return a success flag: that would leave the GCC
// build checking something the MSVC build does not. The check belongs in the
// caller, where both platforms run it. See DetectAVX2 in
// 3Dmotor/GLightPerVertexDispatch.cpp, which does exactly that before reaching
// for leaf 7.
//
// No EFLAGS.ID test for whether CPUID exists at all. It arrived on the late 486
// and is architectural on x86-64; on 32-bit, cmake/arch.cmake leaves
// ARCHITECTURE at NONE, so no /arch: flag is passed and MSVC's x86 default of
// /arch:SSE2 applies. Anything that can execute this binary postdates CPUID by
// about a decade, so the test could never take its false branch.
//
// Everything the callers ask about lives in the basic leaves and means the same
// thing on Intel and AMD: SSE2 is leaf 1 EDX[26], OSXSAVE leaf 1 ECX[27], AVX
// ECX[28], AVX2 leaf 7 subleaf 0 EBX[5]. The vendor split is in the extended
// 0x80000000 leaves, which are AMD's, and in cache and topology enumeration.
// Neither is queried here, so there is no vendor branch.
#if BOOST_ARCH_X86
#	if BOOST_OS_WINDOWS
#		include <intrin.h>
#	else
#		include <cpuid.h>
#	endif
#endif

//! CPUID for a leaf that takes no subleaf. Fills eax, ebx, ecx, edx in that order.
inline void cpuid( int cpuInfo[4], int nLeaf )
{
#if !BOOST_ARCH_X86
	// Nothing to ask. Reporting all-zero leaves every caller here on its
	// reference path, which is the honest answer on a target that has none of
	// the instruction sets this exists to detect.
	(void)nLeaf;
	cpuInfo[0] = cpuInfo[1] = cpuInfo[2] = cpuInfo[3] = 0;
#elif BOOST_OS_WINDOWS
	__cpuid( cpuInfo, nLeaf );
#else
	__cpuid( nLeaf, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3] );
#endif
}

//! CPUID for a leaf that takes a subleaf in ecx. Check the leaf against the
//! maximum reported by leaf 0 first: an out-of-range leaf does not fault, it
//! returns whatever the highest supported leaf holds.
inline void cpuid_count( int cpuInfo[4], int nLeaf, int nSubLeaf )
{
#if !BOOST_ARCH_X86
	(void)nLeaf;
	(void)nSubLeaf;
	cpuInfo[0] = cpuInfo[1] = cpuInfo[2] = cpuInfo[3] = 0;
#elif BOOST_OS_WINDOWS
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
#if !BOOST_ARCH_X86
	(void)nXcr;
	return 0;
#elif BOOST_OS_WINDOWS
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
