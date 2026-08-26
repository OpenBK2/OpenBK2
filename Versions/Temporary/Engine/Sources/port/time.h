#pragma once

#include <chrono>
#include <cstdint>
#include <ctime>
#include <tuple>

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


// The layout of Win32's SYSTEMTIME, field for field and width for width.
//
// This is not a convenience: NSaveLoad::SSavegameEntry and the replay list each
// hold one and hand it to IBinSaver, which has no operator& for a plain struct and
// so writes sizeof(T) raw bytes through DataChunk. Those entries live inside the
// interface stack that MainLoopInternal serialises into a savegame, so the field
// order and the widths here are an on-disk format. Do not reorder them, and do not
// replace the struct with a time_t.
struct SSystemTime
{
	uint16_t wYear;
	uint16_t wMonth;
	uint16_t wDayOfWeek;
	uint16_t wDay;
	uint16_t wHour;
	uint16_t wMinute;
	uint16_t wSecond;
	uint16_t wMilliseconds;
};

static_assert( sizeof( SSystemTime ) == 16,
	"SSystemTime is written into savegames as raw bytes, so its size is an on-disk format" );


// Local broken-down time for a time_t, which is what FileTimeToLocalFileTime
// followed by FileTimeToSystemTime did to a file's last write time.
//
// wMilliseconds is zero, because a time_t has no sub-second part. Neither did the
// file times this replaces in practice: they were whole seconds by the time
// boost::filesystem handed them over.
//
// Returns false when the time_t cannot be represented, and leaves the result
// untouched, the same contract GetLocalTime has.
inline bool GetLocalSystemTime( SSystemTime *pResult, std::time_t t )
{
	std::tm tmLocal;
	if ( !GetLocalTime( &tmLocal, t ) )
	{
		return false;
	}
	pResult->wYear = static_cast<uint16_t>( tmLocal.tm_year + 1900 );
	pResult->wMonth = static_cast<uint16_t>( tmLocal.tm_mon + 1 );
	pResult->wDayOfWeek = static_cast<uint16_t>( tmLocal.tm_wday );
	pResult->wDay = static_cast<uint16_t>( tmLocal.tm_mday );
	pResult->wHour = static_cast<uint16_t>( tmLocal.tm_hour );
	pResult->wMinute = static_cast<uint16_t>( tmLocal.tm_min );
	pResult->wSecond = static_cast<uint16_t>( tmLocal.tm_sec );
	pResult->wMilliseconds = 0;
	return true;
}


// Now, in local time. What Win32's GetLocalTime( LPSYSTEMTIME ) did, milliseconds
// included: two saves made in the same second are still ordered by this field.
inline bool GetLocalSystemTime( SSystemTime *pResult )
{
	const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	if ( !GetLocalSystemTime( pResult, std::chrono::system_clock::to_time_t( now ) ) )
	{
		return false;
	}
	pResult->wMilliseconds = static_cast<uint16_t>(
		( std::chrono::duration_cast<std::chrono::milliseconds>( now.time_since_epoch() )
			% std::chrono::seconds( 1 ) ).count() );
	return true;
}


// Chronological order, which is what converting both to FILETIME and comparing the
// two 64 bit values did.
//
// wDayOfWeek is deliberately skipped. It is derived from the date, and comparing it
// in field order would sort Sunday ahead of Monday within the same day.
inline bool operator<( const SSystemTime &a, const SSystemTime &b )
{
	return std::tie( a.wYear, a.wMonth, a.wDay, a.wHour, a.wMinute, a.wSecond, a.wMilliseconds )
		< std::tie( b.wYear, b.wMonth, b.wDay, b.wHour, b.wMinute, b.wSecond, b.wMilliseconds );
}
