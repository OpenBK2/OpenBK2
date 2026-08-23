#include "stdafx.h"
#include "HPTimer.h"

#include <cstdint>
#include <thread>
#include <chrono>

#include <boost/predef.h>

// __rdtsc is a compiler intrinsic, not a library function, so it needs a
// header: <intrin.h> on Windows, which MSVC and clang-cl both provide, and
// <x86intrin.h> with GCC and clang elsewhere. Keyed on the target OS rather
// than on the compiler for the reason port/stdcall.h spells out: under
// clang-cl BOOST_COMP_MSVC and BOOST_COMP_GNUC are both the *_EMULATED
// variants and evaluate to 0, and BOOST_COMP_GNUC is 0 for clang on Linux
// too, so a compiler test would include neither header.
#if BOOST_OS_WINDOWS
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

using namespace NHPTimer;

// Seconds per TSC tick, refined by UpdateHPTimerFrequency.
static double fProcFreq1 = 1;

namespace
{
	// The clock the TSC is calibrated against. This was QueryPerformanceCounter,
	// which is what MSVC implements steady_clock on, so on Windows the reference
	// has not actually changed.
	//
	// The TSC stays the thing being read: it is a register read of a few
	// nanoseconds, where clock_gettime is only comparable while the kernel's
	// clocksource is tsc. When it falls back to hpet or acpi_pm the vDSO cannot
	// serve the call from user space and every reading becomes a syscall, which
	// is exactly the cost this timer exists to avoid.
	using CRefClock = std::chrono::steady_clock;
}

double NHPTimer::GetSeconds( const NHPTimer::STime &a )
{
	return (static_cast<double>(a)) * fProcFreq1;
}

// Time counters

static inline void GetCounter( int64_t *pTime )
{
	*pTime = __rdtsc();
}

double NHPTimer::GetClockRate()
{
	return 1 / fProcFreq1;
}

void NHPTimer::GetTime( STime *pTime )
{
	GetCounter( pTime );
}

double NHPTimer::GetTimePassed( STime *pTime )
{
	STime old(*pTime );
	GetTime( pTime );
	return GetSeconds( *pTime - old );
}

void NHPTimer::UpdateHPTimerFrequency()
{
	// How long a sample has to run before the tick ratio is worth believing.
	static const double F_MIN_SAMPLE_SECONDS = 0.05;

	static CRefClock::time_point refStart;
	static STime tscStart;
	static bool bUpdateInitiated = false;
	if ( bUpdateInitiated )
	{
		const double fRefPassed =
			std::chrono::duration<double>( CRefClock::now() - refStart ).count();
		// Too short to be useful: keep the baseline where it is and let the next
		// call measure across a longer span rather than restarting the sample.
		if ( fRefPassed < F_MIN_SAMPLE_SECONDS )
			return;
		STime tscNow;
		GetTime( &tscNow );
		const STime tscPassed = tscNow - tscStart;
		if ( tscPassed > 0 )
			fProcFreq1 = fRefPassed / static_cast<double>( tscPassed );
	}
	bUpdateInitiated = true;
	refStart = CRefClock::now();
	GetTime( &tscStart );
}

static void InitHPTimer()
{
	for(;;)
	{
		UpdateHPTimerFrequency();
		if ( fProcFreq1 != 1 )
			break;
		std::this_thread::sleep_for( std::chrono::milliseconds(100) );
	}
	//cout << "freq = " << fpProcFreq / 1000000 <<  "Mhz" << endl;
}

// это вспомогательная структура для автоматической инициализации HP timer'а
struct SHPTimerInit
{
	SHPTimerInit() { InitHPTimer(); }
};
static SHPTimerInit hptInit;


