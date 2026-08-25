#pragma once

#include <boost/predef.h>

#include <cstdint>

// How much physical memory the machine has.
//
// There is no portable way to ask. The standard library has nothing, and
// neither does Boost: Boost.Interprocess covers mapped files and shared memory,
// not the size of installed RAM.
//
// Keyed on the target OS rather than on the compiler, for the reason
// port/stdcall.h spells out.
#if BOOST_OS_WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif

//! Total physical memory in bytes, or 0 when it cannot be determined.
inline uint64_t total_physical_memory()
{
#if BOOST_OS_WINDOWS
	// GlobalMemoryStatusEx rather than GlobalMemoryStatus. The older call has no
	// way to report failure, and Microsoft documents its MEMORYSTATUS fields as
	// unable to express the size of a large machine, returning -1 on overflow.
	MEMORYSTATUSEX status = {};
	status.dwLength = sizeof( status );
	if ( !GlobalMemoryStatusEx( &status ) )
	{
		return 0;
	}
	return status.ullTotalPhys;
#else
	// sysconf rather than Linux's sysinfo() or a read of /proc/meminfo: it is
	// POSIX, there is no struct whose layout varies between platforms, and there
	// is no text to parse. Both values are -1 when the name is not supported.
	const long nPages = sysconf( _SC_PHYS_PAGES );
	const long nPageSize = sysconf( _SC_PAGESIZE );
	if ( nPages <= 0 || nPageSize <= 0 )
	{
		return 0;
	}
	return static_cast<uint64_t>( nPages ) * static_cast<uint64_t>( nPageSize );
#endif
}
