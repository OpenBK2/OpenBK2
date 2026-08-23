#include "stdafx.h"
#include "GLightPerVertexKernels.h"

#include <intrin.h>

namespace NGScene
{
namespace
{

enum { CPUID_1_EDX_SSE2 = 1 << 26 };
enum { CPUID_1_ECX_OSXSAVE = 1 << 27, CPUID_1_ECX_AVX = 1 << 28 };
enum { CPUID_7_EBX_AVX2 = 1 << 5 };
enum { XCR0_SSE_STATE = 1 << 1, XCR0_AVX_STATE = 1 << 2 };

bool DetectSSE2()
{
	int cpuInfo[4];
	__cpuid( cpuInfo, 1 );
	return 0 != ( cpuInfo[3] & CPUID_1_EDX_SSE2 );
}

bool DetectAVX2()
{
	int cpuInfo[4];
	__cpuid( cpuInfo, 0 );
	// Leaf 7 carries the AVX2 bit; older parts do not implement it at all.
	if ( cpuInfo[0] < 7 )
		return false;
	__cpuid( cpuInfo, 1 );
	// XGETBV itself faults unless the OS enabled XSAVE, so OSXSAVE has to be
	// checked before the call, not after it.
	if ( 0 == ( cpuInfo[2] & CPUID_1_ECX_OSXSAVE ) || 0 == ( cpuInfo[2] & CPUID_1_ECX_AVX ) )
		return false;
	// The CPU can have AVX2 while the OS declines to preserve YMM across a context
	// switch; using it then silently corrupts the upper halves.
	const unsigned long long nXcr0 = _xgetbv( 0 );
	if ( ( nXcr0 & ( XCR0_SSE_STATE | XCR0_AVX_STATE ) ) != ( XCR0_SSE_STATE | XCR0_AVX_STATE ) )
		return false;
	__cpuidex( cpuInfo, 7, 0 );
	return 0 != ( cpuInfo[1] & CPUID_7_EBX_AVX2 );
}

const SLightingKernels &SelectLightingKernels()
{
	if ( IsAVX2Present() )
		return avx2LightingKernels;
	if ( IsSSE2Present() )
		return sse2LightingKernels;
	return refLightingKernels;
}

}

bool IsSSE2Present()
{
	static const bool bPresent = DetectSSE2();
	return bPresent;
}

bool IsAVX2Present()
{
	static const bool bPresent = DetectAVX2();
	return bPresent;
}

const SLightingKernels &GetLightingKernels()
{
	static const SLightingKernels &kernels = SelectLightingKernels();
	return kernels;
}

}
