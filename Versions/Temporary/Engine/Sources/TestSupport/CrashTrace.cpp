// What a crashing test says before it dies.
//
// ctest reports a test that is killed by a signal as "SEGFAULT" or "Subprocess
// aborted" and nothing else, so until now the only way to see where one died
// was to rebuild it and run it under a debugger, which is a slow answer when
// the failure is on a runner and a guess when it is not reproducible. These
// handlers print the stack to stderr first, which --output-on-failure carries
// into the job log.
//
// They are installed by the constructor of a namespace-scope object, before
// main, and are left installed: a test whose assertions all pass and which then
// faults in static destruction, while libraries are unloading, is exactly the
// case that says the least and the one this is most useful for.
//
// Two cases it cannot report. A fault in a static initializer that runs before
// this one, since the handlers are not in yet, and a stack overflow, since the
// handler has no stack of its own to run on.
//
// Nothing references anything here, which is why TestSupport/CMakeLists.txt
// builds it as an object library: its objects go on each test's link line. From
// inside a static library the installer below would be dropped as unreferenced
// and the handlers would never go in.

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>

#include <boost/predef.h>
#include <boost/stacktrace.hpp>

namespace
{
const char *SignalName( int signo )
{
	switch ( signo )
	{
		case SIGSEGV:
			return "SIGSEGV";
		case SIGABRT:
			return "SIGABRT";
		case SIGILL:
			return "SIGILL";
		case SIGFPE:
			return "SIGFPE";
#if !BOOST_OS_WINDOWS
		case SIGBUS:
			return "SIGBUS";
#endif
		default:
			return "signal";
	}
}

// nSkip counts the caller's own frames to leave out, so that frame 0 is about
// the crash rather than about this file. The frame the kernel or the runtime
// enters the handler through stays in: it is what says the next frame down is
// where the process actually was.
void Report( const std::string &szWhat, std::size_t nSkip )
{
	// The reason goes out first and on its own: walking the stack can fault in
	// turn, and then this line is all there is.
	std::fprintf( stderr, "\n*** test crashed: %s\n", szWhat.c_str() );
	std::fflush( stderr );
	const boost::stacktrace::stacktrace trace( nSkip + 1, static_cast<std::size_t>( -1 ) );
	std::fprintf( stderr, "%s", boost::stacktrace::to_string( trace ).c_str() );
	std::fflush( stderr );
}

extern "C" void OnFatalSignal( int signo )
{
	// Default first, so a fault while reporting ends the process instead of
	// re-entering this handler.
	std::signal( signo, SIG_DFL );
	Report( SignalName( signo ), 1 );
	// Raised again rather than exited, so ctest still reports the signal that
	// killed the test rather than an exit code this handler invented.
	std::raise( signo );
}

void OnTerminate()
{
	std::string szWhat = "std::terminate";
	if ( std::current_exception() )
	{
		try
		{
			std::rethrow_exception( std::current_exception() );
		}
		catch ( const std::exception &error )
		{
			szWhat += ": ";
			szWhat += error.what();
		}
		catch ( ... )
		{
			szWhat += ": an exception not derived from std::exception";
		}
	}
	Report( szWhat, 1 );
	// The trace is already out, so the handler below would only print a second
	// one for the SIGABRT this raises.
	std::signal( SIGABRT, SIG_DFL );
	std::abort();
}

struct SInstallCrashTrace
{
	SInstallCrashTrace()
	{
		std::signal( SIGSEGV, OnFatalSignal );
		std::signal( SIGABRT, OnFatalSignal );
		std::signal( SIGILL, OnFatalSignal );
		std::signal( SIGFPE, OnFatalSignal );
#if !BOOST_OS_WINDOWS
		std::signal( SIGBUS, OnFatalSignal );
#endif
		// Replaces the standard one, which names the exception but not where it
		// came from. Report puts the what() back.
		std::set_terminate( OnTerminate );
	}
};

const SInstallCrashTrace installCrashTrace;
} // namespace
