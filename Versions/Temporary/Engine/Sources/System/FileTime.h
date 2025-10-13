#pragma once

#include <boost/predef.h>

#if BOOST_OS_WINDOWS
#include <winbase.h>
#endif

#include <time.h>

struct SWin32Time
{
	union
	{
		struct
		{
			DWORD seconds : 5;								// seconds (0..29 with 2 sec. interval)
			DWORD minutes : 6;								// minutes (0..59)
			DWORD hours   : 5;								// hours (0..23)
			DWORD day     : 5;								// day (1..31)
			DWORD month   : 4;								// month(1..12)
			DWORD year    : 7;								// year (0..119 relative to 1980)
		};
		struct
		{
			WORD wTime;
			WORD wDate;
		};
		DWORD dwFulltime;
	};
	//
	SWin32Time() {  }
	SWin32Time( const DWORD _dwFulltime ) : dwFulltime( _dwFulltime ) {  }
	WORD GetDate() const { return wDate; }
	WORD GetTime() const { return wTime; }
	operator DWORD() const { return dwFulltime; }
};

// transforms DOS date/time format (time_t) to the Win32 date/time format (SWin32Time)
inline DWORD DOSToWin32DateTime( time_t dostime )
{
	// transform DOS time to local time 'tm' structure
	tm *pTime = localtime( &dostime );
	// fill 'SWin32Time' structure to automagically convert to Win32 date/time format
	SWin32Time filetime;
	filetime.year    = pTime->tm_year - 80;	// due to 'tm' year relative to 1900 year, but we need relative to 1980
	filetime.month   = pTime->tm_mon + 1;		// due to the month represented in the '0..11' format, but we need in '1..12'
	filetime.day     = pTime->tm_mday;			// day in '1..31' format
	filetime.hours   = pTime->tm_hour;			// hours in '0..23' format
	filetime.minutes = pTime->tm_min;				// minutes in '0..59' format
	filetime.seconds = pTime->tm_sec / 2;		// due to win32 seconds resolution are 2 sec. (i.e. seconds represented in '0..29' format)

	return filetime;
}

// transforms Win32 date/time format (SWin32Time) to the DOS date/time format (time_t)
inline time_t Win32ToDOSDateTime( const DWORD _w32time )
{
	struct SConvert
	{
		union
		{
			struct  
			{
				DWORD seconds : 5;								// seconds (0..29 with 2 sec. interval)
				DWORD minutes : 6;								// minutes (0..59)
				DWORD hours   : 5;								// hours (0..23)
				DWORD day     : 5;								// day (1..31)
				DWORD month   : 4;								// month(1..12)
				DWORD year    : 7;								// year (0..119 relative to 1980)
			};
			DWORD dwFullTime;
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

// transforms FILETIME to Win32 date/time (SWin32Time)
inline DWORD FILETIMEToWin32DateTime( const FILETIME &filetime )
{
	FILETIME localfiletime;
	FileTimeToLocalFileTime( &filetime, &localfiletime );
	SWin32Time win32time;
	FileTimeToDosDateTime( &localfiletime, &win32time.wDate, &win32time.wTime );
	return win32time;
}

// transforms FILETIME to TM time 
inline void FILETIMEToTMTime( const FILETIME &filetime, tm *pTMTime )
{
	struct SConvert
	{
		union
		{
			struct  
			{
				DWORD seconds : 5;								// seconds (0..29 with 2 sec. interval)
				DWORD minutes : 6;								// minutes (0..59)
				DWORD hours   : 5;								// hours (0..23)
				DWORD day     : 5;								// day (1..31)
				DWORD month   : 4;								// month(1..12)
				DWORD year    : 7;								// year (0..119 relative to 1980)
			};
			DWORD dwFullTime;
		};
	};
	SConvert w32time;
	FILETIME localfiletime;
	FileTimeToLocalFileTime( &filetime, &localfiletime );
	SWin32Time win32time;
	FileTimeToDosDateTime( &localfiletime, &win32time.wDate, &win32time.wTime );
	w32time.dwFullTime = win32time;
	// compose 'tm' structure. for details you can see a function above
	Zero( *pTMTime );
	pTMTime->tm_year = int( w32time.year ) + 80;
	pTMTime->tm_mon  = int( w32time.month ) - 1;
	pTMTime->tm_mday = int( w32time.day );
	pTMTime->tm_hour = int( w32time.hours );
	pTMTime->tm_min  = int( w32time.minutes );
	pTMTime->tm_sec  = int( w32time.seconds ) * 2;
}

// transforms Win32 date/time (SWin32Time) to FILETIME
inline FILETIME Win32DateTimeToFILETIME( const DWORD win32time )
{
	FILETIME localft;
	DosDateTimeToFileTime( HIWORD(win32time), LOWORD(win32time), &localft );
	FILETIME filetime;
	LocalFileTimeToFileTime( &localft, &filetime );
	return filetime;
}


