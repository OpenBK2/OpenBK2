#pragma once

// The C++26 <debugging> header, ahead of time.
//
// P2546 adds std::is_debugger_present, std::breakpoint and
// std::breakpoint_if_debugging, which is exactly the trio this codebase had
// spelled out by hand as IsDebuggerPresent and __debugbreak. Once a standard
// library here ships them this file becomes a using-declaration and then
// nothing.

#include <boost/predef.h>

#if BOOST_OS_WINDOWS
#include <windows.h>
#else
#include <csignal>
#endif

#if BOOST_OS_LINUX
#include <cstdlib>
#include <fstream>
#include <string>
#endif

//! Whether a debugger is currently attached to this process.
inline bool is_debugger_present()
{
#if BOOST_OS_WINDOWS
	return ::IsDebuggerPresent() != FALSE;
#elif BOOST_OS_LINUX
	// /proc/self/status carries a TracerPid line holding the pid of whatever
	// is tracing this process, or 0 when nothing is. Reading it is what both
	// Catch2 and Boost.Test do, neither of which exports the function.
	//
	// Deliberately not cached: a debugger can attach at any point in the run,
	// and IsDebuggerPresent reports that, so caching would make the two
	// platforms disagree about a case that matters precisely when someone is
	// trying to debug something.
	std::ifstream status( "/proc/self/status" );
	std::string line;
	while ( std::getline( status, line ) )
	{
		if ( line.rfind( "TracerPid:", 0 ) == 0 )
		{
			return std::strtol( line.c_str() + 10, 0, 10 ) != 0;
		}
	}
	return false;
#else
	// no way to ask, so assume the unattended case and take the branch that
	// reports rather than the one that traps
	return false;
#endif
}

//! Stop in the debugger. With nothing attached this terminates the process on
//! every platform here, which is what breakpoint_if_debugging is for.
inline void breakpoint()
{
#if BOOST_COMP_MSVC
	__debugbreak();
#elif BOOST_COMP_CLANG
	__builtin_debugtrap();
#elif BOOST_COMP_GNUC && ( BOOST_ARCH_X86_32 || BOOST_ARCH_X86_64 )
	__asm__ __volatile__( "int3" );
#else
	// int3 raises SIGTRAP anyway; this is the same thing without the
	// assumption that the target is x86
	std::raise( SIGTRAP );
#endif
}

//! Stop in the debugger if there is one, and do nothing at all if there is
//! not. Breaking unconditionally would kill an unattended process.
inline void breakpoint_if_debugging()
{
	if ( is_debugger_present() )
	{
		breakpoint();
	}
}
