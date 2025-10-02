
#pragma once

inline UINT64 GetLongTickCount()
{
	FILETIME sysTime;
	GetSystemTimeAsFileTime( &sysTime );
	return ( ( UINT64( sysTime.dwHighDateTime ) << 32 ) | sysTime.dwLowDateTime ) / 10000;
}

