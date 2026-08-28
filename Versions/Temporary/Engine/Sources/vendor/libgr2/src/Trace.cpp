#include "Trace.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#if defined( _WIN32 )
#include <spdlog/sinks/msvc_sink.h>
#endif

#include <atomic>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

namespace NGr2
{

namespace
{

//! The level to log at, from LIBGR2_LOG_LEVEL, defaulting to trace.
//!
//! Everything is on by default because the whole point of this stage is the log.
//! Once entry points start being implemented, setting this to warn leaves only
//! the list of the ones that still are not.
spdlog::level::level_enum LevelFromEnvironment()
{
	// MSVC deprecates getenv in favour of _dupenv_s, which allocates and is not
	// portable. Suppressed around this one call rather than by defining
	// _CRT_SECURE_NO_WARNINGS, so that the rest of the library keeps the warning.
	// The pragma cannot be spelled with warning( suppress ) here, because that
	// applies to the next line and the #endif would be it.
	//
	// Raw _MSC_VER rather than boost/predef because this library depends on
	// nothing outside the standard library and spdlog.
#if defined( _MSC_VER )
	#pragma warning( push )
	#pragma warning( disable : 4996 )
#endif
	const char *pszLevel = std::getenv( "LIBGR2_LOG_LEVEL" );
#if defined( _MSC_VER )
	#pragma warning( pop )
#endif
	if ( pszLevel == nullptr )
	{
		return spdlog::level::trace;
	}

	const spdlog::level::level_enum level = spdlog::level::from_str( pszLevel );
	// from_str answers "off" for anything it does not recognise, which would
	// silently discard the log because of a typo. Treat that as unset unless the
	// caller really did ask for off.
	if ( level == spdlog::level::off && std::string( pszLevel ) != "off" )
	{
		return spdlog::level::trace;
	}
	return level;
}

std::shared_ptr<spdlog::logger> MakeLogger()
{
	std::vector<spdlog::sink_ptr> sinks;

	// granny_calls.log beside the executable is the deliverable this stage
	// exists to produce. Truncated per run, because it is read as the record of
	// one run and not as a history.
	try
	{
		sinks.push_back( std::make_shared<spdlog::sinks::basic_file_sink_mt>( "granny_calls.log",
		                                                                     true ) );
	}
	catch ( const spdlog::spdlog_ex & )
	{
		// An unwritable working directory is not a reason to take the host
		// process down, and stderr still carries the log.
		sinks.push_back( std::make_shared<spdlog::sinks::stderr_sink_mt>() );
	}

#if defined( _WIN32 )
	// So that the trace shows up in the debugger's output window when the game
	// is run under one, which is how it will usually be run.
	sinks.push_back( std::make_shared<spdlog::sinks::msvc_sink_mt>() );
#endif

	auto logger = std::make_shared<spdlog::logger>( "libgr2", sinks.begin(), sinks.end() );
	logger->set_pattern( "[%H:%M:%S.%e] [%l] %v" );
	logger->set_level( LevelFromEnvironment() );
	// The process this is meant to observe is expected to stop abruptly, and a
	// buffered tail is exactly the part worth reading.
	logger->flush_on( spdlog::level::trace );
	return logger;
}

//! The entry points already reported at warn, and the lock that guards them.
//!
//! Keyed on the name's contents rather than on the pointer, because __func__
//! produces a distinct array per function and nothing promises identical names
//! are pooled.
std::set<std::string> &Reported()
{
	static std::set<std::string> reported;
	return reported;
}

std::mutex &ReportedMutex()
{
	static std::mutex mutex;
	return mutex;
}

}

spdlog::logger &Logger()
{
	static std::shared_ptr<spdlog::logger> logger = MakeLogger();
	return *logger;
}

uint64_t NextCallOrdinal()
{
	static std::atomic<uint64_t> ordinal{ 0 };
	return ordinal.fetch_add( 1, std::memory_order_relaxed ) + 1;
}

void ReportUnimplemented( const char *pszFunction )
{
	{
		std::lock_guard<std::mutex> lock( ReportedMutex() );
		if ( !Reported().insert( pszFunction ).second )
		{
			return;
		}
	}
	Logger().warn( "{} is not implemented", pszFunction );
}

}
