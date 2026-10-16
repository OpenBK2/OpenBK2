#pragma once

#include <cstdint>
#include <ctime>

#include <boost/predef.h>

struct SWin32Time
{
	union
	{
		struct
		{
			uint32_t seconds : 5; // seconds (0..29 with 2 sec. interval)
			uint32_t minutes : 6; // minutes (0..59)
			uint32_t hours : 5;   // hours (0..23)
			uint32_t day : 5;     // day (1..31)
			uint32_t month : 4;   // month(1..12)
			uint32_t year : 7;    // year (0..127 relative to 1980)
		};

		struct
		{
			uint16_t wTime;
			uint16_t wDate;
		};

		// One full-width default also initializes the overlapping date/time bitfields.
		uint32_t dwFulltime = 0;
	};

	//
	SWin32Time() {}

	SWin32Time( const uint32_t _dwFulltime )
		: dwFulltime( _dwFulltime )
	{
	}

	uint16_t GetDate() const
	{
		return wDate;
	}

	uint16_t GetTime() const
	{
		return wTime;
	}

	operator uint32_t() const
	{
		return dwFulltime;
	}
};

// Pack a filesystem timestamp as UTC in the MS-DOS layout used by the VFS.
// ZIP's DOS fields have no timezone. We interpret them as UTC for file selection,
// preserving archive ordering without guessing the archive author's timezone.
// Using local time here made identical loose files override a .pak in one
// player's timezone but lose to it in another, changing the game checksum.
inline uint32_t PackFileTime( std::time_t fileTime )
{
	std::tm utc;
	// Both variants use caller-owned storage, including on the resource loader thread.
#if BOOST_OS_WINDOWS
	if ( ::gmtime_s( &utc, &fileTime ) != 0 )
#else
	if ( ::gmtime_r( &fileTime, &utc ) == nullptr )
#endif
	{
		return 0;
	}
	// the layout counts years from 1980 and std::tm counts from 1900, so a
	// year outside 1980..2107 has nowhere to go in seven bits.
	const int nYear = utc.tm_year - 80;
	if ( nYear < 0 )
	{
		return 0;
	}
	SWin32Time packed;
	packed.year = nYear > 127 ? 127 : nYear;
	packed.month = utc.tm_mon + 1;   // std::tm counts months from 0, this layout from 1
	packed.day = utc.tm_mday;        // 1..31
	packed.hours = utc.tm_hour;      // 0..23
	packed.minutes = utc.tm_min;     // 0..59
	packed.seconds = utc.tm_sec / 2; // two second resolution, so 0..29

	return packed;
}

// The inverse uses the same UTC convention, with no local offset or DST correction.
inline std::time_t UnpackFileTime( const uint32_t _w32time )
{
	const SWin32Time w32time( _w32time );
	std::tm tmTime = {};
	tmTime.tm_year = int( w32time.year ) + 80;
	tmTime.tm_mon = int( w32time.month ) - 1;
	tmTime.tm_mday = int( w32time.day );
	tmTime.tm_hour = int( w32time.hours );
	tmTime.tm_min = int( w32time.minutes );
	tmTime.tm_sec = int( w32time.seconds ) * 2;
#if BOOST_OS_WINDOWS
	return ::_mkgmtime( &tmTime );
#else
	return ::timegm( &tmTime );
#endif
}
