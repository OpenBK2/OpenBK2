#include "port/crashpad.h"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <string>
#include <thread>

#if defined(_WIN32)
#include <crtdbg.h>
#endif

namespace
{
// Keep the invalid address opaque to optimization: this test needs a real
// hardware fault with a usable exception context, not a compiler-generated trap.
volatile std::uintptr_t crashAddress = 0;

void AccessViolation()
{
	*reinterpret_cast<volatile unsigned char *>( crashAddress ) = 1;
}

#if defined(_WIN32)
LONG WINAPI SwallowHeapCorruption( EXCEPTION_POINTERS *exception )
{
	return exception->ExceptionRecord->ExceptionCode == 0xC0000374
		? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
}
#endif
}

int main( int argc, char **argv )
{
	if ( argc != 2 )
		return 2;

#if defined(_WIN32)
	// Intentional test failures must not wait for an unattended debug CRT box.
	SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );
	_set_abort_behavior( 0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT );
#endif

	const std::string mode( argv[1] );
	const bool initialized = InitCrashpad();
	if ( mode == "missing-handler" )
		return initialized ? 3 : 0;
	if ( !initialized )
		return 4;

	if ( mode == "access-violation" )
		AccessViolation();
	else if ( mode == "worker-access-violation" )
	{
		std::thread worker( AccessViolation );
		worker.join();
	}
	else if ( mode == "abort" )
		std::abort();
	else if ( mode == "terminate" )
		std::terminate();
	else if ( mode == "caught-exception" || mode == "fallback-directory" )
	{
		try
		{
			throw std::runtime_error( "intentional caught fatal exception" );
		}
		catch ( const std::exception & )
		{
			DumpCrashpadWithoutCrash();
			return 0;
		}
	}
#if defined(_WIN32)
	else if ( mode == "heap-corruption" )
	{
		// A later handler swallows this exception just as Windows can. A plain
		// unhandled RaiseException would also pass with the old, destroyed client
		// because its process-wide UEF survives. This requires the client's VEH.
		AddVectoredExceptionHandler( 0, SwallowHeapCorruption );
		RaiseException( 0xC0000374, 0, 0, nullptr );
	}
	else if ( mode == "invalid-parameter" )
		_invalid_parameter_noinfo_noreturn();
	else if ( mode == "purecall" )
		_purecall();
#endif
	else
		return 5;

	// Any fatal path that returns failed to reach the crash handler.
	return 6;
}
