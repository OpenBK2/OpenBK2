// Where the stub trace goes, and how it gets switched on.
//
// Every function in this library logs its arguments before it returns, because
// that trace is what the library is for: it says which parts of the toolkit the
// editor actually reaches, in what order and with what values, which is the list
// a real implementation has to satisfy.
//
// None of it was reaching anywhere. InitLogging() below used to have to be
// called, nothing called it, and spdlog's default logger writes to stdout at
// info level, so all 269 trace lines were dropped. The logger is built on first
// use now and there is nothing to call.
//
// This is a static library linked into seven DLLs, and spdlog is header only, so
// each of those DLLs has its own spdlog registry and its own default logger.
// That is why the file sink is named after the module it is logging for: seven
// modules opening one file would interleave badly at best. OutputDebugString has
// no such problem, being process wide, so the debugger's output window is where
// the seven traces appear in one order, and that is the one worth reading.

#include "logging.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#if defined( _WIN32 )
#include <windows.h>
#endif

namespace
{

const char *Environment( const char *pszName )
{
#if defined( _MSC_VER )
	// getenv is deprecated here and _dupenv_s allocates; the value is read once
	// at startup and never freed, which is what a leak checker would call a leak
	// and is in fact the process's lifetime.
	char *pszValue = nullptr;
	size_t nLength = 0;
	if ( _dupenv_s( &pszValue, &nLength, pszName ) != 0 )
	{
		return nullptr;
	}
	return pszValue;
#else
	return std::getenv( pszName );
#endif
}

//! OBK2_STINGRAY_LOG=off|error|warn|info|debug|trace, debug by default.
//!
//! Every call this library is asked to make is logged, because a stub library
//! that is not saying what was called is doing nothing at all. They log at
//! debug; the handful MFC makes on every idle log at trace, one level further
//! down, so that a default run says what the editor did without the 1,764 lines
//! an idle loop added to a startup. OBK2_STINGRAY_LOG=trace asks for those too.
spdlog::level::level_enum LevelFromEnvironment()
{
	const char *pszLevel = Environment( "OBK2_STINGRAY_LOG" );
	if ( pszLevel == nullptr )
	{
		return spdlog::level::debug;
	}
	const spdlog::level::level_enum eLevel = spdlog::level::from_str( pszLevel );
	// from_str answers "off" for anything it does not recognise, which would
	// silently discard the trace over a typo.
	if ( eLevel == spdlog::level::off && std::string( pszLevel ) != "off" )
	{
		return spdlog::level::debug;
	}
	return eLevel;
}

//! The module this copy of the library is linked into, for naming its log.
std::string ModuleName()
{
#if defined( _WIN32 )
	HMODULE hModule = nullptr;
	if ( ::GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
	                               | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                           reinterpret_cast<LPCSTR>( &ModuleName ), &hModule ) )
	{
		char szPath[MAX_PATH] = { 0 };
		if ( ::GetModuleFileNameA( hModule, szPath, MAX_PATH ) != 0 )
		{
			std::string szName( szPath );
			const size_t nSlash = szName.find_last_of( "\\/" );
			if ( nSlash != std::string::npos )
			{
				szName.erase( 0, nSlash + 1 );
			}
			const size_t nDot = szName.find_last_of( '.' );
			if ( nDot != std::string::npos )
			{
				szName.erase( nDot );
			}
			return szName;
		}
	}
#endif
	return "stingray";
}

std::shared_ptr<spdlog::logger> MakeLogger()
{
	const std::string szModule = ModuleName();
	std::vector<spdlog::sink_ptr> sinks;

#if defined( _WIN32 )
	// The one stream that shows all seven modules in the order the calls
	// happened, because OutputDebugString is process wide. Read it with a
	// debugger attached or with DebugView.
	sinks.push_back( std::make_shared<spdlog::sinks::msvc_sink_mt>() );
#endif

	// And a file, so that a run without a debugger still leaves something to
	// read. Truncated per run: this is the record of one run, not a history.
	try
	{
		sinks.push_back( std::make_shared<spdlog::sinks::basic_file_sink_mt>(
			"stingray_" + szModule + ".log", true ) );
	}
	catch ( const spdlog::spdlog_ex & )
	{
		// An unwritable working directory is not a reason to take the editor
		// down before it has drawn anything.
		sinks.push_back( std::make_shared<spdlog::sinks::stderr_sink_mt>() );
	}

	auto logger = std::make_shared<spdlog::logger>( szModule, sinks.begin(), sinks.end() );
	logger->set_pattern( "[%H:%M:%S.%e] [" + szModule + "] %v" );
	logger->set_level( LevelFromEnvironment() );

	// Every line, immediately. What is being traced is a process that is expected
	// to stop on an assert, and the last few calls before it are exactly the part
	// worth reading. The volume here is startup and user interface, so the cost
	// is one that can be afforded.
	logger->flush_on( spdlog::level::trace );
	return logger;
}

//! Installs the logger on first use, once per module.
//!
//! A function local static rather than anything at namespace scope: this is a
//! static library in seven DLLs, and a stub can run during another module's
//! static initialisation, which is the one time an initialiser at namespace
//! scope would not have run yet.
struct SLoggingInstaller
{
	SLoggingInstaller() { spdlog::set_default_logger( MakeLogger() ); }
};

}

//! Idempotent, and nothing has to call it: logging.h defines an object in every
//! translation unit of this library whose constructor does, which is also what
//! makes the linker pull this object file out of the static library at all.
void InitLogging()
{
	static SLoggingInstaller installer;
}
