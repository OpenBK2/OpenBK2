#pragma once

#include <cstdint>
#include <ctime>

#include "port/time.h"

struct SWin32Time
{
	union
	{
		struct
		{
			uint32_t seconds : 5;								// seconds (0..29 with 2 sec. interval)
			uint32_t minutes : 6;								// minutes (0..59)
			uint32_t hours   : 5;								// hours (0..23)
			uint32_t day     : 5;								// day (1..31)
			uint32_t month   : 4;								// month(1..12)
			uint32_t year    : 7;								// year (0..119 relative to 1980)
		};
		struct
		{
			uint16_t wTime;
			uint16_t wDate;
		};
		uint32_t dwFulltime;
	};
	//
	SWin32Time() {  }
	SWin32Time( const uint32_t _dwFulltime ) : dwFulltime( _dwFulltime ) {  }
	uint16_t GetDate() const { return wDate; }
	uint16_t GetTime() const { return wTime; }
	operator uint32_t() const { return dwFulltime; }
};

// pack a time_t into the MS-DOS layout above, via local time.
// the zip central directory stores this layout, so packing the loose file's
// time is what makes the two comparable
inline uint32_t PackFileTime( std::time_t fileTime )
{
	std::tm local;
	if ( !GetLocalTime( &local, fileTime ) )
	{
		return 0;
	}
	// the layout counts years from 1980 and std::tm counts from 1900, so a
	// year outside 1980..2107 has nowhere to go in seven bits. Clamping keeps
	// the ordering monotonic instead of wrapping into a wrong century.
	const int nYear = local.tm_year - 80;
	if ( nYear < 0 )
	{
		return 0;
	}
	SWin32Time packed;
	packed.year    = nYear > 127 ? 127 : nYear;
	packed.month   = local.tm_mon + 1;		// std::tm counts months from 0, this layout from 1
	packed.day     = local.tm_mday;			// 1..31
	packed.hours   = local.tm_hour;			// 0..23
	packed.minutes = local.tm_min;			// 0..59
	packed.seconds = local.tm_sec / 2;		// two second resolution, so 0..29

	return packed;
}

// the inverse of PackFileTime
inline time_t UnpackFileTime( const uint32_t _w32time )
{
	struct SConvert
	{
		union
		{
			struct  
			{
				uint32_t seconds : 5;								// seconds (0..29 with 2 sec. interval)
				uint32_t minutes : 6;								// minutes (0..59)
				uint32_t hours   : 5;								// hours (0..23)
				uint32_t day     : 5;								// day (1..31)
				uint32_t month   : 4;								// month(1..12)
				uint32_t year    : 7;								// year (0..119 relative to 1980)
			};
			uint32_t dwFullTime;
		};
	};
	SConvert w32time;
	w32time.dwFullTime = _w32time;
	// compose 'tm' structure. for details you can see a function above
	tm tmTime;
	Zero( tmTime );
	tmTime.tm_year = int( w32time.year ) + 80;
	tmTime.tm_mon  = int( w32time.month ) - 1;
	tmTime.tm_mday = int( w32time.day );
	tmTime.tm_hour = int( w32time.hours );
	tmTime.tm_min  = int( w32time.minutes );
	tmTime.tm_sec  = int( w32time.seconds ) * 2;
	// convert 'tm' to 'time_t'
	time_t result = mktime( &tmTime );
	return result;
}
