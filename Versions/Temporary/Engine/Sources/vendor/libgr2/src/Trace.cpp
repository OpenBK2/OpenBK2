#include "Trace.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#if defined( _WIN32 )
#include <spdlog/sinks/msvc_sink.h>
// For IsDebuggerPresent. spdlog's msvc sink already pulls windows.h in, but
// relying on somebody else's include for a symbol used here is how a header
// reshuffle upstream becomes a build break down here.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <atomic>
#include <cstdlib>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
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
//! One environment variable, or null.
//!
//! MSVC deprecates getenv in favour of _dupenv_s, which allocates and is not
//! portable. Suppressed around this one call rather than by defining
//! _CRT_SECURE_NO_WARNINGS, so that the rest of the library keeps the warning.
//! The pragma cannot be spelled with warning( suppress ) here, because that
//! applies to the next line and the #endif would be it.
//!
//! Raw _MSC_VER rather than boost/predef because this library depends on
//! nothing outside the standard library and spdlog.
const char *Environment( const char *pszName )
{
#if defined( _MSC_VER )
	#pragma warning( push )
	#pragma warning( disable : 4996 )
#endif
	return std::getenv( pszName );
#if defined( _MSC_VER )
	#pragma warning( pop )
#endif
}

spdlog::level::level_enum LevelFromEnvironment()
{
	const char *pszLevel = Environment( "LIBGR2_LOG_LEVEL" );
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
	//
	// LIBGR2_LOG_FILE names it something else, which is what recording the same
	// play twice needs: this library writes one log, the forwarding shim over
	// the real DLL writes another, and the two are then diffed. Without a way to
	// rename it the second run overwrites the first.
	const char *pszFile = Environment( "LIBGR2_LOG_FILE" );
	try
	{
		sinks.push_back( std::make_shared<spdlog::sinks::basic_file_sink_mt>(
			pszFile != nullptr ? pszFile : "granny_calls.log", true ) );
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
	//
	// Only under one, though. OutputDebugString is a kernel transition and a
	// process-wide lock whether or not anybody is listening, and the busiest
	// entry point here is called 17,880 times in a few seconds of play. Adding
	// this sink unconditionally is most of what made a traced run unplayable,
	// which matters now that recording a real session is the point rather than a
	// side effect.
	if ( IsDebuggerPresent() )
	{
		sinks.push_back( std::make_shared<spdlog::sinks::msvc_sink_mt>() );
	}
#endif

	auto logger = std::make_shared<spdlog::logger>( "libgr2", sinks.begin(), sinks.end() );
	logger->set_pattern( "[%H:%M:%S.%e] [%l] %v" );
	logger->set_level( LevelFromEnvironment() );

	// The process this is meant to observe is expected to stop abruptly, and a
	// buffered tail is exactly the part worth reading, so a warning or an error
	// always reaches the disk before the next line is formatted.
	//
	// Not every trace line, which is what this used to do. A flush per call over
	// a whole level's worth of animation is the difference between recording a
	// session and watching a slide show, and a trace is read as a whole rather
	// than for its last line. LIBGR2_LOG_FLUSH=always asks for the old behaviour,
	// for the case where the last few calls before a crash are the question.
	const char *pszFlush = Environment( "LIBGR2_LOG_FLUSH" );
	const bool bFlushEverything = pszFlush != nullptr && std::string( pszFlush ) == "always";
	logger->flush_on( bFlushEverything ? spdlog::level::trace : spdlog::level::warn );
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

//! The handles already named, and how many of each kind have been.
//!
//! Keyed on the address, holding the kind alongside the number so that an
//! address which comes back as a different kind of object is caught rather than
//! inheriting the old name. Kinds are compared by content: SHandleKind::NAME is
//! a string literal and nothing promises one address for it across translation
//! units.
struct SHandleTable
{
	std::map<const void *, std::pair<std::string, uint64_t>> Named;
	std::map<std::string, uint64_t> Counters;
};

SHandleTable &Handles()
{
	static SHandleTable table;
	return table;
}

std::mutex &HandlesMutex()
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

std::string HandleName( const char *pszKind, const void *pHandle )
{
	if ( pHandle == nullptr )
	{
		// Named rather than numbered. A null is not an object, and giving it an
		// id would make every null of a kind look like one long-lived thing.
		return fmt::format( "{}#null", pszKind );
	}

	std::lock_guard<std::mutex> lock( HandlesMutex() );
	SHandleTable &table = Handles();
	std::map<const void *, std::pair<std::string, uint64_t>>::iterator it =
		table.Named.find( pHandle );
	if ( it == table.Named.end() || it->second.first != pszKind )
	{
		const uint64_t nId = ++table.Counters[pszKind];
		it = table.Named.insert_or_assign( pHandle,
		                                   std::make_pair( std::string( pszKind ), nId ) )
			     .first;
	}
	return fmt::format( "{}#{}@{}", it->second.first, it->second.second, pHandle );
}

void RetireHandle( const void *pHandle )
{
	if ( pHandle == nullptr )
	{
		return;
	}
	std::lock_guard<std::mutex> lock( HandlesMutex() );
	Handles().Named.erase( pHandle );
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
