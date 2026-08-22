#pragma once

#include <chrono>
#include <cstdint>
#include <ctime>

#include <boost/predef.h>

// Portable replacement for GetTickCount: milliseconds from a monotonic clock.
//
// steady_clock rather than system_clock, because every caller here measures an
// interval, and system_clock can step backwards when the machine's wall clock
// is corrected.
//
// The result is 32 bits on purpose, matching what GetTickCount returned.
// Callers keep these readings in uint32_t fields, in NTimer::STime and in
// floats, and then subtract two of them. Unsigned subtraction gives the right
// interval even across the wrap at 2^32 milliseconds, roughly 49.7 days of
// uptime, which is exactly how GetTickCount behaved. Returning 64 bits would
// quietly break every such pair, because "now" would keep growing after the
// stored 32-bit value had already wrapped, turning a small difference into an
// enormous one.
//
// The epoch is unspecified and differs between platforms, so only the
// difference between two readings means anything. No caller depends on where
// zero sits.
inline uint32_t GetCurrentTimeMilliseconds()
{
	const auto sinceEpoch = std::chrono::steady_clock::now().time_since_epoch();
	return static_cast<uint32_t>(
		std::chrono::duration_cast<std::chrono::milliseconds>( sinceEpoch ).count() );
}


// Thread-safe local time. The C localtime hands back a pointer into a static
// buffer, so two threads calling it race with each other, and this is reached
// from the resource loader thread as well as the main loop.
//
// Returns false when the time_t cannot be represented, which the callers treat
// as "no usable timestamp" rather than substituting a wrong one.
//
// boost::date_time::c_time::localtime looks like it would do this, but it only
// reaches for a reentrant function when
// BOOST_DATE_TIME_HAS_REENTRANT_STD_FUNCTIONS is set, and
// date_time/compiler_config.hpp explicitly clears that for _MSC_VER, "no
// reentrant posix functions (eg: localtime_r)". So on Windows it falls back to
// plain std::localtime, which is the race being avoided here, and it reports
// failure by throwing. C++20 would offer std::chrono::zoned_time instead.
inline bool GetLocalTime( std::tm *pResult, std::time_t t )
{
#if BOOST_OS_WINDOWS
	// the argument order is reversed between the two, and so is the result
	return ::localtime_s( pResult, &t ) == 0;
#else
	return ::localtime_r( &t, pResult ) != nullptr;
#endif
}
