#pragma once

#include <boost/predef.h>

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include <client/crash_report_database.h>
#include <client/crashpad_client.h>
#include <client/settings.h>
#include <util/misc/capture_context.h>
#include <util/misc/paths.h>

namespace NCrashpadDetail
{
struct SState
{
	crashpad::CrashpadClient client;
	bool attempted = false;
	bool ready = false;
	std::filesystem::path executableDirectory;
	std::filesystem::path databasePath;
	std::string status;
};

inline SState &State()
{
	// Crashpad owns the Windows heap-corruption vectored exception handler.
	// A stack-local client unregisters it when InitCrashpad returns. Keep the
	// client through static destruction too, when the engine can still crash.
	static SState *state = new SState;
	return *state;
}

inline std::filesystem::path UserDatabasePath()
{
#if BOOST_OS_WINDOWS
	if ( const wchar_t *directory = _wgetenv( L"LOCALAPPDATA" ) )
		return std::filesystem::path( directory ) / L"OpenBK2" / L"crashpad_db";
#else
	if ( const char *directory = std::getenv( "XDG_STATE_HOME" ) )
	{
		if ( std::filesystem::path( directory ).is_absolute() )
			return std::filesystem::path( directory ) / "OpenBK2" / "crashpad_db";
	}
	if ( const char *directory = std::getenv( "HOME" ) )
		return std::filesystem::path( directory ) / ".local" / "state" / "OpenBK2" / "crashpad_db";
#endif
	return {};
}

inline std::unique_ptr<crashpad::CrashReportDatabase> OpenDatabase( const std::filesystem::path &path )
{
	if ( path.empty() || !path.is_absolute() )
		return nullptr;
	std::error_code error;
	std::filesystem::create_directories( path, error );
	if ( error )
		return nullptr;
	return crashpad::CrashReportDatabase::Initialize( base::FilePath( path.native() ) );
}

inline void WriteStatus()
{
	const SState &state = State();
	std::fprintf( stderr, "%s\n", state.status.c_str() );
	std::fflush( stderr );
#if BOOST_OS_WINDOWS
	OutputDebugStringA( ( state.status + "\n" ).c_str() );
#endif
	// GUI launches may have no stderr. Leave a diagnostic even when the game
	// never reaches its own log setup; also record the selected fallback path.
	if ( !state.executableDirectory.empty() )
		std::ofstream( state.executableDirectory / "crashpad-status.txt" ) << state.status << '\n';
	if ( !state.databasePath.empty() )
		std::ofstream( state.databasePath / "crashpad-status.txt" ) << state.status << '\n';
}

#if BOOST_COMP_MSVC
inline void __cdecl InvalidParameter( const wchar_t *, const wchar_t *, const wchar_t *, unsigned int, uintptr_t )
{
	// The CRT's default invokes Watson/fast-fail, bypassing the normal exception
	// filter. abort() instead reaches the SIGABRT handler Crashpad installed.
	std::abort();
}

inline void __cdecl PureCall()
{
	std::abort();
}
#endif
}

// Call once on the entry thread, before configuration loading or worker startup.
// Reporting failure is nonfatal, but must be visible in startup diagnostics.
inline bool InitCrashpad( const char *application = "OpenBK2", const char *build = "" )
{
	using namespace NCrashpadDetail;
	SState &state = State();
	if ( state.attempted )
		return state.ready;
	state.attempted = true;
	try
	{
		base::FilePath executable;
		if ( !crashpad::Paths::Executable( &executable ) )
		{
			state.status = "Crashpad disabled: cannot resolve the executable path";
			WriteStatus();
			return false;
		}
		state.executableDirectory = std::filesystem::path( executable.value() ).parent_path();
#if BOOST_OS_WINDOWS
		const auto handler = state.executableDirectory / L"crashpad_handler.exe";
#else
		const auto handler = state.executableDirectory / "crashpad_handler";
#endif
		// Preserve the installed game's report location. Protected installations
		// get a persistent per-user fallback instead of silently losing reporting.
		state.databasePath = state.executableDirectory / "crashpad_db";
		auto database = OpenDatabase( state.databasePath );
		if ( !database )
		{
			state.databasePath = UserDatabasePath();
			database = OpenDatabase( state.databasePath );
		}

		const std::string paths = "; handler=" + handler.u8string()
			+ "; database=" + state.databasePath.u8string();
		std::error_code error;
		if ( !database )
			state.status = "Crashpad disabled: cannot create a writable report database" + paths;
		else if ( !database->GetSettings()->SetUploadsEnabled( false ) )
			state.status = "Crashpad disabled: cannot disable report uploads" + paths;
		else if ( !std::filesystem::is_regular_file( handler, error ) )
			state.status = "Crashpad disabled: handler executable is missing or inaccessible" + paths;
		else
		{
			// All paths are absolute: shortcuts and editor dialogs can use or
			// change the working directory. No metrics directory is needed.
			state.ready = state.client.StartHandler( base::FilePath( handler.native() ),
				base::FilePath( state.databasePath.native() ), base::FilePath(), "", "",
				{ { "product", application }, { "build", build } }, {}, true, false );
			state.status = ( state.ready ? "Crashpad enabled" : "Crashpad disabled: handler startup failed" ) + paths;
#if BOOST_COMP_MSVC
			if ( state.ready )
			{
				_set_invalid_parameter_handler( InvalidParameter );
				_set_purecall_handler( PureCall );
				// Avoid the debug CRT's modal abort box before its signal handler.
				_set_abort_behavior( 0, _WRITE_ABORT_MSG );
			}
#endif
		}
	}
	catch ( const std::exception &error )
	{
		state.status = std::string( "Crashpad initialization failed: " ) + error.what();
	}
	WriteStatus();
	return state.ready;
}

inline const std::string &GetCrashpadStatus()
{
	return NCrashpadDetail::State().status;
}

inline void DumpCrashpadWithoutCrash()
{
	if ( !NCrashpadDetail::State().ready )
		return;
	// Fatal C++ exceptions caught by the entry point never reach Crashpad's
	// unhandled-exception filter. Capture before logging/dialogs allocate more
	// memory. This records the catch site, not the already-unwound throw site.
	crashpad::NativeCPUContext context;
	crashpad::CaptureContext( &context );
#if BOOST_OS_WINDOWS
	crashpad::CrashpadClient::DumpWithoutCrash( context );
#else
	crashpad::CrashpadClient::DumpWithoutCrash( &context );
#endif
}
