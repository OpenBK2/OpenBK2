#include "stdafx.h"

#include "GameX/GetConsts.h"

#include "GameX_export.h"

#include <chrono>
#include <thread>

#if USE_MIMALLOC
// Non-Windows executable-level overrides. Windows links a separate override
// object into each participating module without redirecting the process CRT.
#include <mimalloc-new-delete.h>
#endif
#include "3Dmotor/Gfx.h"
#include "AILogic/CreateAI.h"
#include "Input/Input.h"
#include "libdb/Db.h"
#include "Main/MainLoopCommands.h"
#include "Main/MODs.h"
#include "Main/Profiles.h"
#include "Misc/2Darray.h"
#include "Misc/StrProc.h"
#include "port/debugging.h"
#include "port/messagebox.h"
#include "resource.h"
#include "SceneB2/Cursor.h"
#include "SceneB2/Scene.h"
#include "Sound/SFX.h"
#include "Stats_B2_M1/IconsSet.h"
#include "System/Commands.h"
#include "System/FilePath.h"
#include "System/FileUtils.h"
#include "System/GResource.h"
#include "System/SplashScreen.h"
#include "System/VFSOperations.h"
#include "VersionInfo.h"
#include "WinFrame2Input.h"

#include <exception>
#include <iterator>
#include <typeinfo>

#if !BOOST_OS_WINDOWS
#include <cxxabi.h>
#include <unistd.h> // getpid, for the profiling pause
#endif

#if !BOOST_OS_WINDOWS
// Captured where the exception was thrown rather than where it was caught, which
// is the difference between a stack that names the failure and one that names
// this file. Boost records it by overriding __cxa_throw, so it needs no
// cooperation from whatever threw, and no MSVC equivalent exists.
#include <boost/stacktrace/stacktrace.hpp>
#endif

#include "port/crashpad.h"

#include <fmt/format.h>
#include <zconf.h>

//

namespace NDb
{
void SaveChanges();
}

namespace NGameX
{
bool Initialize();
GAMEX_EXPORT void PostStorageInitialize();
}; // namespace NGameX

// Explicit editor arguments are kept outside profile variables so loading a
// profile cannot replace the map or mod selected in the editor.
static std::string szEditorMap;
static std::string szEditorMod;
// --replay=, kept here for the same reason.
static std::string szReplayFile;
// --replay-player=, the player a --replay= is watched as. -1 when not given,
// which plays it as player 0, the same as the console command with no player.
static int nReplayPlayer = -1;
// --mission= and --campaign=, which start a mission straight from the command
// line so nobody has to edit a config file to do it. Kept here and applied at
// the same late point as the editor's map for the reason given there: a config
// is read before the main menu's own line in game.cfg, and that line
// unregisters the scenario tracker a mission started from a config just made.
static std::string szStartMission;
static std::string szStartCampaign;
// --replay-verify and --setvar. Recorded rather than applied while parsing,
// because arguments are read long before autoexec.cfg and game.cfg are, and
// anything set here would be overwritten by them.
static bool bReplayVerify = false;
// --exit-after-load, for the same reason: it becomes a variable the mission
// interface reads once the map is in.
static bool bExitAfterLoad = false;
// --mod=, the folder under MODs to attach. Kept outside profile variables the
// way the editor's map and mod are, since the point of it is to beat what the
// profile remembers. The flag is separate from the string because an empty
// --mod= is a meaningful answer, "no mod", and not the same as not asking.
static std::string szStartMod;
static bool bModFromCommandLine = false;
static std::vector<std::pair<std::string, std::string>> commandLineVars;

static bool ProcessCommandLine( const std::vector<std::string> &arguments );
static int RunGame( const std::vector<std::string> &arguments );
static int RunGameGuarded( const std::vector<std::string> &arguments );

//! This process's id, for the profiling pause below to print.
//!
//! There is no helper for this in port/, because nothing else in the tree has
//! wanted one; the two names differ only in spelling.
static int CurrentProcessId()
{
#if BOOST_OS_WINDOWS
	return static_cast<int>( ::GetCurrentProcessId() );
#else
	return static_cast<int>( ::getpid() );
#endif
}

static void StoreBuildInfo()
{
	const std::string szVersion =
			fmt::format( "Build {}, {} {}", NVersionInfo::nBuild, NVersionInfo::szDate, NVersionInfo::szTime );
	NGlobal::SetVar( "version.info", szVersion );
}

//! The name of the type currently being thrown, for the catch all below.
//!
//! DXVK is why this exists. dxvk::DxvkError does not derive from std::exception,
//! so it has no what() to read and a plain catch ( ... ) could only report that
//! something was thrown. The Itanium ABI can still name the type, and the name
//! alone says which subsystem to go and look at.
static std::string CurrentExceptionTypeName()
{
#if BOOST_OS_WINDOWS
	// MSVC has no equivalent of __cxa_current_exception_type
	return "unknown";
#else
	const std::type_info *pType = abi::__cxa_current_exception_type();
	if ( pType == 0 )
	{
		return "unknown";
	}
	int nStatus = 0;
	char *pszDemangled = abi::__cxa_demangle( pType->name(), 0, 0, &nStatus );
	if ( pszDemangled == 0 )
	{
		// the mangled name is still better than nothing
		return pType->name();
	}
	const std::string szName( pszDemangled );
	free( pszDemangled );
	return szName;
#endif
}

//! Where the exception being handled was thrown, if that can be recovered.
//!
//! boost_stacktrace_from_exception keeps a trace for every live exception, taken
//! at the throw, and this looks up the one currently being handled. It comes back
//! empty when the throw happened before that library was loaded, when the trace
//! could not be stored, or when the process was not built with the frame
//! information to walk.
//!
//! **The names in it are only as good as the debug information.** A build with no
//! -g gives addresses and shared library names and nothing else, which still
//! locates the failure to a library but not to a line. The Windows presets map
//! Release to RelWithDebInfo, so those carry -g; a Linux build directory
//! configured as plain Release does not.
static std::string CurrentExceptionStack()
{
#if BOOST_OS_WINDOWS
	return "";
#else
	const boost::stacktrace::stacktrace trace = boost::stacktrace::stacktrace::from_current_exception();
	if ( !trace )
	{
		return "";
	}
	return boost::stacktrace::to_string( trace );
#endif
}

//! Say that the game is stopping, everywhere someone might be looking.
//!
//! Three places on purpose. stderr always works, including before the console
//! buffer exists and after the renderer has failed. csSystem reaches log.txt,
//! which is what a user will be asked for. The message box is what the other
//! startup failures in RunGame use, and is the only one of the three that a
//! player launching from a desktop entry will ever see.
static void ReportFatalError( const std::string &szWhat )
{
	const std::string szMessage = "Fatal: " + szWhat;

	const std::string szStack = CurrentExceptionStack();
	const std::string szFull = szStack.empty() ? szMessage : szMessage + "\nthrown from:\n" + szStack;

	fmt::print( stderr, "{}\n", szFull );
	fflush( stderr );

	if ( Singleton<IConsoleBuffer>() != 0 )
	{
		csSystem << CC_RED << szFull.c_str() << endl;
	}

	// The box gets the message without the stack: it is for a player, and the
	// stack is for whoever reads log.txt afterwards.
	MessageBox( 0, szMessage.c_str(), "Error", MB_OK | MB_ICONERROR );
}

//! Run the game, and turn anything thrown out of it into a report.
//!
//! Without this an exception that reaches the entry point calls std::terminate,
//! which prints one line naming the type and aborts, taking the log, the message
//! box and the exit code with it. That is what a DXVK initialisation failure did.
static int RunGameGuarded( const std::vector<std::string> &arguments )
{
	try
	{
		return RunGame( arguments );
	}
	catch ( const std::exception &e )
	{
		DumpCrashpadWithoutCrash();
		ReportFatalError( fmt::format( "{}: {}", CurrentExceptionTypeName(), e.what() ) );
	}
	catch ( ... )
	{
		DumpCrashpadWithoutCrash();
		// Where DXVK lands. There is no message to be had, only the type.
		ReportFatalError( fmt::format( "unhandled exception of type {}", CurrentExceptionTypeName() ) );
	}
	return 0xDEAD;
}

#if BOOST_OS_WINDOWS

//! The command line as a list of arguments, which is what every other platform
//! hands the program already.
//!
//! Windows does not: WinMain is given one string with the arguments still in it,
//! so the quoting has to be undone here. Off Windows the shell has already done
//! it, and better, because it knows what the user actually typed.
static std::vector<std::string> SplitCommandLine( LPSTR lpCmdLine )
{
	std::vector<std::string> arguments;
	for ( NStr::CStringIterator<char, std::string, NStr::CBracketSeparator<char, NStr::SBracketsQuoteTest<char>>> it(
						lpCmdLine, ' ' );
	      !it.IsEnd();
	      it.Next() )
	{
		std::string szArgument;
		it.Get( &szArgument );
		NStr::TrimBoth( szArgument );
		if ( !szArgument.empty() )
		{
			arguments.push_back( szArgument );
		}
	}
	return arguments;
}

// WinMain rather than wWinMain: utf8.manifest sets the process code page to
// UTF-8, so the narrow command line is already the encoding the rest of the tree
// reads, and a wide entry point would only mean converting it back.
//
// hPrevInstance has meant nothing since 16 bit Windows, nCmdShow is not consulted
// because the window is created borderless at the desktop's size, and hInstance
// is now asked for by the one place that wants it.
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int )
{
	// Cover command-line/configuration work as well as the main game loop.
	InitCrashpad( "OpenBK2 Game", REVISION_NUMBER_STR " " BUILD_DATE_TIME_STR );
	return RunGameGuarded( SplitCommandLine( lpCmdLine ) );
}

#else

int main( int argc, char *argv[] )
{
	InitCrashpad( "OpenBK2 Game", REVISION_NUMBER_STR " " BUILD_DATE_TIME_STR );
	// argv[0] is the program itself, which WinMain is not given either
	return RunGameGuarded( std::vector<std::string>( argv + 1, argv + argc ) );
}

#endif

static int RunGame( const std::vector<std::string> &arguments )
{
#if USE_MIMALLOC
	// Retain the non-Windows allocator dependency even with link-time pruning.
	(void)mi_version();
#endif

	NGlobal::LoadConfig( NFile::JoinPath( "..", NFile::DIR_PROFILES, "startup.cfg" ) );
	StoreBuildInfo();

	// Suppress the shell's own error boxes for a missing DLL or an unreadable
	// drive, so the failure goes to the calling process instead. Windows only:
	// there is no equivalent notion off it, and nothing takes its place.
#if BOOST_OS_WINDOWS
	SetErrorMode( SEM_FAILCRITICALERRORS );
#endif
	//
	if ( ProcessCommandLine( arguments ) == false )
	{
		return 0xDEAD;
	}

	std::string szLogFileName, szErrorFileName;
	{
		// NFile::GetNormalizedCurrDir is what the rest of the tree uses in place of
		// GetCurrentDirectory. It hands back forward slashes with one on the end,
		// so the backslash that was concatenated here is gone too, and it fixes
		// the bug the 1024 byte buffer carried: GetCurrentDirectory wrote nothing
		// and returned the size it wanted when the path did not fit, leaving
		// uninitialised stack behind.
		const std::string szDir = NFile::GetNormalizedCurrDir();
		//
		szLogFileName = szDir + "log.txt";
		szErrorFileName = szDir + "error.txt";
		NFile::RemoveFile( szErrorFileName );
		NFile::RemoveFile( szLogFileName );
	}
	if ( IConsoleBuffer *pConsole = Singleton<IConsoleBuffer>() )
	{
		pConsole->SetLogfile( szLogFileName.c_str() );
		SetupPipeDumpToConsole( PIPE_CHAT, CONSOLE_STREAM_CONSOLE );
		csSystem << GetCrashpadStatus().c_str() << endl;
	}

	NGScene::SFLB3_RunResourceLoadingThread();
	// show splash screen during program starting
	//	NWinFrame::ShowSplashScreen( true );
	// load and initialize all dll modules, register and initialize singletons
	NGameX::Initialize();
	//
	CObj<CObjectBase> pSplashScreen =
			is_debugger_present() ? 0 : NSplash::CreateSplashScreen( NMainLoop::GetBaseDir() + "splash.bmp", true );
	//
	CreateAI();
	//
	// initialize win app
#if BOOST_OS_WINDOWS
	// icon.rc compiles main.ico into the executable, and MAKEINTRESOURCE names it
	// by id. That is also what Explorer and the task bar show for the file.
	LPCSTR pIcon = MAKEINTRESOURCE( IDI_MAIN );
#else
	// an ELF has no resource section, so the window class gets no icon here and
	// the same picture is set from a file once the window exists
	LPCSTR pIcon = 0;
#endif
	if ( !NWinFrame::SFLB1_InitApplication( " Blitzkrieg II", "NIVAL_RTS_ENGINE", pIcon ) )
	{
		MessageBox( 0, "InitApplication", "Error", MB_OK );
		return 0xDEAD;
	}
	// main.ico is installed beside the game's other run-time files. Does nothing
	// on Windows, where the resource above already covered it.
	NWinFrame::SetIcon( NMainLoop::GetBaseDir() + "main.ico" );
	// init graphics
	if ( !NGfx::Init3D( NWinFrame::GetWnd() ) )
	{
		ASSERT( 0 ); // DX not found
		MessageBox( 0, "Failed to initialize Direct3D9", "Error", MB_OK );
		return 0xDEAD;
	}
	// init input system
	NInput::InitInput( NWinFrame::GetWnd() );
	//Input()->Init( NWinFrame::GetWnd() );

	// init graphics system
	Singleton<ISFX>()->Init( NWinFrame::GetWnd(), 0, SFX_OUTPUT_DSOUND, 44100, 128 );

	// init profile and read configs
	NMOD::InstantAttachMOD( "", NDb::DATABASE_MODE_GAME );
	NProfile::LoadProfile();
	//
	// Which mod to attach, in the order the three ways of saying it beat each
	// other: the editor's own argument, then --mod, then what the profile
	// remembers from the last time one was picked from the menu.
	//
	// --mod has to be read here rather than left to --setvar, because
	// current_attached_mod is read on the next line and command line variables
	// are not applied until after the configs, several steps further down. A
	// --setvar of it would be a variable nothing goes back to look at.
	//
	// --mod= with nothing after it names no mod, which is how a run says "ignore
	// the profile and start unmodded" - worth having, since attaching one is
	// remembered and would otherwise persist into every later run.
	std::string szMOD2Attach = NStr::ToMBCS( NGlobal::GetVar( "current_attached_mod", "" ) );
	if ( bModFromCommandLine )
	{
		szMOD2Attach = szStartMod;
	}
	if ( !szEditorMap.empty() )
	{
		szMOD2Attach = szEditorMod;
	}
	if ( szEditorMap.empty() && !szMOD2Attach.empty() )
	{
		szMOD2Attach = NFile::JoinPath( NMainLoop::GetBaseDir(), NFile::DIR_MODS, szMOD2Attach );
	}
	if ( NMOD::DoesMODAttached( szMOD2Attach ) == false )
	{
		NMOD::InstantAttachMOD( szMOD2Attach, NDb::DATABASE_MODE_GAME );
	}
	//
	NGlobal::LoadConfig( NFile::JoinPath( "..", NFile::DIR_PROFILES, "autoexec.cfg" ) );
	NGlobal::LoadConfig( NFile::JoinPath( "..", NFile::DIR_PROFILES, "game.cfg" ) );
	//
	NGlobal::SetVar( "code_version_number", REVISION_NUMBER_STR );
	NGlobal::SetVar( "code_build_date_time", BUILD_DATE_TIME_STR );
	// Hash the final startup data once, after profile/mod configs and the version.
	NGameX::InitializeMPDataVersionChecksum();
	//
	pSplashScreen = 0;
	// setup video mode
	if ( !Scene()->SetupMode( SCENE_MODE_WINDOWED, false ) )
	{
		MessageBox( 0, "Can't setup scene mode from config", "Error", MB_OK );
		return 0xDEAD;
	}
	// After the configs, so these win rather than being overwritten by them.
	if ( bReplayVerify )
	{
		NGlobal::SetVar( "Replay.ExitWhenFinished", 1 );
		// Failure until the replay actually plays out, so quitting, a handled
		// crash or ending up in a menu all report it.
		NGlobal::SetVar( "Replay.Result", 1 );
		// Verification compares nothing without this, and a run with no console
		// has no other way to switch it on.
		NGlobal::SetVar( "checksum_verify", 1 );
	}
	// Loading a map is one blocking call, so a run that only wants to measure it
	// has nothing to do once it returns. This is what makes the game a profiling
	// subject: without it the process stays in the mission and the samples taken
	// after the load bury the ones taken during it.
	if ( bExitAfterLoad )
	{
		NGlobal::SetVar( "Mission.ExitAfterLoad", 1 );
		// The phase breakdown is the number this run exists to produce, so it
		// goes to stdout as well as to the log.
		NGlobal::SetVar( "Mission.ReportLoadTime", 1 );
	}
	// Last, so an explicit --setvar overrides the defaults above as well. Routed
	// through the console rather than SetVar so a value is typed exactly as the
	// same line in a .cfg would be.
	for ( size_t i = 0; i < commandLineVars.size(); ++i )
	{
		NGlobal::ProcessCommand( L"setvar " + UTF8ToWide( commandLineVars[i].first ) + L" = " +
		                         UTF8ToWide( commandLineVars[i].second ) );
	}
	// Run the normal mission command only after the game, profile and mod DB
	// are initialized, and after game.cfg: its "main_menu" line unregisters the
	// scenario tracker, so a mission started any earlier is left without one and
	// used to crash in CICMission::PostCreate. Quoting preserves map folders and
	// filenames with spaces.
	//
	// --editor-map= and --mission= name the same thing - a MapInfo record to
	// start now - under the editor's name and the documented one, so one branch
	// serves both and the editor wins if somehow given both.
	const std::string &szMapToStart = !szEditorMap.empty() ? szEditorMap : szStartMission;
	if ( !szMapToStart.empty() )
	{
		NGlobal::ProcessCommand( L"map \"" + UTF8ToWide( szMapToStart ) + L"\"" );
	}
	// The campaign form of the same thing. The fields were validated while
	// parsing; each is quoted separately because a mission can be named by its
	// MapInfo record, whose path may contain spaces, and the command splits on
	// them. Missing fields default to the first chapter, its first mission and
	// the easiest difficulty, which is what the "mission" command does with
	// however many arguments it is given.
	else if ( !szStartCampaign.empty() )
	{
		std::vector<std::string> fields;
		NStr::SplitString( szStartCampaign, &fields, ';' );
		while ( fields.size() < 4 )
		{
			fields.push_back( "0" );
		}
		std::wstring command = L"mission";
		for ( size_t i = 0; i < fields.size(); ++i )
		{
			command += L" \"" + UTF8ToWide( fields[i] ) + L"\"";
		}
		NGlobal::ProcessCommand( command );
	}
	// Same point in startup for the same reason: the replay command reaches
	// CICMission, which needs the database loaded. The file name is resolved by
	// ReplayHistory, so this passes it through unchanged. The player, when
	// given, is the command's second argument, as the replay screen passes it.
	else if ( !szReplayFile.empty() )
	{
		std::wstring command = L"replay \"" + UTF8ToWide( szReplayFile ) + L"\"";
		if ( nReplayPlayer >= 0 )
		{
			command += L" " + std::to_wstring( nReplayPlayer );
		}
		NGlobal::ProcessCommand( command );
	}
	// The mission commands above only queue an interface command; the map is
	// loaded by the first StepApp below, in one blocking call. A profiler that
	// attaches by process id therefore has to be running before this loop starts,
	// and polling for the process cannot win that race: by the time the id exists
	// the load may already be under way, and the samples that matter are the ones
	// it would have missed.
	//
	// So this waits, after printing the id, which is the one piece of information
	// the attaching side needs. Zero by default, so an ordinary run never pauses;
	// set it with --setvar Mission.LoadProfileDelay=10. Seconds rather than
	// milliseconds because what is being waited for is a person or a script
	// starting another program.
	const int nLoadProfileDelay = NGlobal::GetVar( "Mission.LoadProfileDelay", 0 );
	if ( nLoadProfileDelay > 0 )
	{
		fmt::print( "map load: pid {}, waiting {} s before loading so a profiler can attach\n",
		            CurrentProcessId(),
		            nLoadProfileDelay );
		fflush( stdout );
		std::this_thread::sleep_for( std::chrono::seconds( nLoadProfileDelay ) );
	}
	// start
	Cursor()->Acquire( true );
	while ( 1 )
	{
		NWinFrame::PumpMessages();

		CWinToInputMessageConverter convert;
		convert.Do();

		const bool bAppActive = NWinFrame::IsAppActive();
		// Off Windows an animated cursor is a set of frames this loop steps
		// through, so it needs a tick; on Windows it is one handle USER32
		// animates and this does nothing. Only while focused, because that is
		// the only time the cursor being shown is ours. Cheap either way: the
		// shipped cursors run at 6 to 12 fps, so at any playable frame rate the
		// great majority of these calls find nothing to do and return.
		if ( bAppActive )
		{
			Cursor()->Step();
		}
		if ( NWinFrame::IsExit() )
		{
			NWinFrame::ResetExit();
			NMainLoop::Command( CreateICExitGame() ); // generate 'EXIT' command
		}
		if ( bAppActive )
		{
			NGfx::CheckBackBufferSize();
		}
		NGfx::SetGamma( bAppActive ); //bSetGamma );
		if ( !NMainLoop::StepApp( bAppActive ) )
		{
			break;
		}
		if ( !bAppActive )
		{
			std::this_thread::sleep_for( std::chrono::milliseconds( 40 ) );
		}
	}
	//
	// Read before the singletons go, since that is where the variable lives.
	// Zero unless a --replay-verify run set it, so an ordinary run is
	// unaffected.
	const int nExitCode = NGlobal::GetVar( "Replay.Result", 0 );
	//
	NMainLoop::ResetStack();

	ClearHoldQueue();
	Scene()->Clear();
	NProfile::SaveProfile();

	NVFS::SetMainVFS( 0 );
	NVFS::SetMainFileCreator( 0 );

	NInput::DoneInput();
	NGfx::Done3D();
	NDb::SaveChanges();
	NDb::CloseDatabase();
	NSingleton::DoneSingletons();
	return nExitCode;
}

static bool ProcessCommandLine( const std::vector<std::string> &arguments )
{
	for ( std::vector<std::string>::const_iterator it = arguments.begin(); it != arguments.end(); ++it )
	{
		// get string
		std::string szString = *it;
		if ( szString.empty() )
		{
			continue;
		}

		// Keep the old command-line syntax, adding unambiguous named map/mod
		// arguments. WinMain retains quotes; POSIX shells already remove them.
		const std::string mapPrefix = "--editor-map=";
		const std::string modPrefix = "--editor-mod=";
		if ( szString.compare( 0, mapPrefix.size(), mapPrefix ) == 0 ||
		     szString.compare( 0, modPrefix.size(), modPrefix ) == 0 )
		{
			const bool bMap = szString.compare( 0, mapPrefix.size(), mapPrefix ) == 0;
			std::string value = szString.substr( bMap ? mapPrefix.size() : modPrefix.size() );
			NStr::TrimBoth( value, '"' );
			if ( value.find_first_of( "\"\r\n;" ) != std::string::npos || ( bMap && value.empty() ) )
			{
				MessageBox( 0, "Invalid editor map/mod argument.", "Start Mission in Game", MB_OK | MB_ICONERROR );
				return false;
			}
			( bMap ? szEditorMap : szEditorMod ) = value;
			continue;
		}
		// --mission=<MapInfo> starts one map as a single mission, a custom
		// mission or a campaign's own map alike, and
		// --campaign=<Campaign>[;chapter[;mission[;difficulty]]] starts one
		// mission of a campaign with that campaign's state behind it, which is
		// the difference between the two. Both are the console commands the game
		// already registers ("map" and "mission"), run at the one point in
		// startup where starting a mission works; what they add is not having to
		// write a line into Profiles/game.cfg to get there.
		//
		// The semicolon is a separator in the campaign argument and so cannot be
		// rejected there, but the fields it separates are checked one by one
		// below. Quotes and line breaks stay out of both: the command is built as
		// a quoted string, and a value carrying either would end the command and
		// start another one.
		const std::string missionPrefix = "--mission=";
		const std::string campaignPrefix = "--campaign=";
		const bool bMission = szString.compare( 0, missionPrefix.size(), missionPrefix ) == 0;
		const bool bCampaign = szString.compare( 0, campaignPrefix.size(), campaignPrefix ) == 0;
		if ( bMission || bCampaign )
		{
			std::string value = szString.substr( bMission ? missionPrefix.size() : campaignPrefix.size() );
			NStr::TrimBoth( value, '"' );
			bool bValid = !value.empty() && value.find_first_of( "\"\r\n" ) == std::string::npos;
			if ( bMission )
			{
				bValid = bValid && value.find( ';' ) == std::string::npos;
			}
			else
			{
				// campaign;chapter;mission;difficulty, at most four fields, none
				// of them empty. The chapter is an index, the mission an index or
				// a MapInfo name, the difficulty an index or easy/normal/hard.
				std::vector<std::string> fields;
				NStr::SplitString( value, &fields, ';' );
				bValid = bValid && !fields.empty() && fields.size() <= 4;
				for ( size_t i = 0; bValid && i < fields.size(); ++i )
				{
					bValid = !fields[i].empty();
				}
			}
			if ( !bValid )
			{
				MessageBox( 0,
				            bMission ? "Invalid --mission argument, expected the name of a MapInfo record."
				                     : "Invalid --campaign argument, expected "
				                       "<campaign>[;<chapter>[;<mission>[;<difficulty>]]].",
				            "Start mission",
				            MB_OK | MB_ICONERROR );
				return false;
			}
			( bMission ? szStartMission : szStartCampaign ) = value;
			continue;
		}
		// Play back a recorded replay without going through the menus, the same
		// way --editor-map= starts a mission: both turn into the console command
		// that the interface already registers. The semicolon is excluded along
		// with the quotes and newlines because ReplayHistory joins its arguments
		// into a semicolon-separated string.
		// Play the replay out and leave, reporting through the exit status
		// instead of the dialog a watching player would dismiss. Separate from
		// --replay= so a replay can still be watched from the command line.
		if ( szString == "--replay-verify" )
		{
			bReplayVerify = true;
			continue;
		}
		// Leave as soon as the map is in, and report how long each phase of
		// loading it took. For profiling a load: the work is one blocking call, so
		// a run that stays in the mission afterwards goes on sampling a frame loop
		// that has nothing to do with it. Pair with --mission, --campaign or
		// --replay; on its own it reaches the menus and nothing is loaded, so
		// nothing is reported and the game stays up.
		if ( szString == "--exit-after-load" )
		{
			bExitAfterLoad = true;
			continue;
		}
		// --mod=<folder> attaches a mod, naming a folder under MODs the way the
		// profile's own current_attached_mod does. Without this a mod could only
		// be chosen from the menu, because the profile is where the choice lives
		// and --setvar is applied long after the attach has happened.
		//
		// The value is a relative path, so a separator is legitimate in it, but
		// one that climbs out of MODs is not: the attach hands it to the VFS as a
		// resource root, and anything above that folder is not a mod.
		const std::string startModPrefix = "--mod=";
		if ( szString.compare( 0, startModPrefix.size(), startModPrefix ) == 0 )
		{
			std::string value = szString.substr( startModPrefix.size() );
			NStr::TrimBoth( value, '"' );
			if ( value.find_first_of( "\"\r\n;" ) != std::string::npos || value.find( ".." ) != std::string::npos )
			{
				MessageBox(
						0, "Invalid --mod argument, expected a folder name under MODs.", "Attach MOD", MB_OK | MB_ICONERROR );
				return false;
			}
			szStartMod = value;
			bModFromCommandLine = true;
			continue;
		}
		// --setvar name=value or --setvar=name=value, repeatable. Applied after
		// the configs, so it overrides them, which is the only way to reach a
		// variable in a run that never shows a console.
		//
		// Both spellings, because both are in use: the documentation and
		// launch.sh's own options write it as two arguments, launch.sh hands the
		// game one. The second argument is consumed here so that it never reaches
		// the catch-all at the end of this loop, which would make a variable of
		// the whole text.
		const std::string varPrefix = "--setvar=";
		const bool bSetVarSeparate = szString == "--setvar";
		if ( bSetVarSeparate || szString.compare( 0, varPrefix.size(), varPrefix ) == 0 )
		{
			if ( bSetVarSeparate && std::next( it ) == arguments.end() )
			{
				MessageBox( 0, "--setvar needs name=value after it.", "Set variable", MB_OK | MB_ICONERROR );
				return false;
			}
			std::string value = bSetVarSeparate ? *++it : szString.substr( varPrefix.size() );
			NStr::TrimBoth( value, '"' );
			const std::string::size_type nEq = value.find( '=' );
			if ( nEq == std::string::npos || nEq == 0 || value.find_first_of( "\"\r\n;" ) != std::string::npos )
			{
				MessageBox( 0, "Invalid --setvar argument, expected name=value.", "Set variable", MB_OK | MB_ICONERROR );
				return false;
			}
			commandLineVars.push_back( std::make_pair( value.substr( 0, nEq ), value.substr( nEq + 1 ) ) );
			continue;
		}
		const std::string replayPrefix = "--replay=";
		if ( szString.compare( 0, replayPrefix.size(), replayPrefix ) == 0 )
		{
			std::string value = szString.substr( replayPrefix.size() );
			NStr::TrimBoth( value, '"' );
			if ( value.empty() || value.find_first_of( "\"\r\n;" ) != std::string::npos )
			{
				MessageBox( 0, "Invalid --replay argument.", "Replay", MB_OK | MB_ICONERROR );
				return false;
			}
			szReplayFile = value;
			continue;
		}
		// --replay-player=<number> watches the --replay= as that player, which is
		// what the replay screen's team list does. The simulation must play out
		// the same whoever watches, so playing one replay as each player in turn
		// is a determinism check of its own. Whether the replay has that player
		// is only known once it is loaded, and is checked there.
		const std::string replayPlayerPrefix = "--replay-player=";
		if ( szString.compare( 0, replayPlayerPrefix.size(), replayPlayerPrefix ) == 0 )
		{
			const std::string value = szString.substr( replayPlayerPrefix.size() );
			if ( value.empty() || value.size() > 2 || value.find_first_not_of( "0123456789" ) != std::string::npos )
			{
				MessageBox( 0, "Invalid --replay-player argument, expected a player number.", "Replay", MB_OK | MB_ICONERROR );
				return false;
			}
			nReplayPlayer = NStr::ToInt( value );
			continue;
		}
		// check for '-' at the begining
		if ( szString == "-show-version" )
		{
			std::string szVersion =
					fmt::format( "Version: {}\nBuild date/time: {}\n", REVISION_NUMBER_STR, BUILD_DATE_TIME_STR );
			fmt::print( "{}", szVersion );
			return false;
		}
		else if ( szString == "-show-version-mb" )
		{
			std::string szVersion =
					fmt::format( "Version: {}\nBuild date/time: {}\n", REVISION_NUMBER_STR, BUILD_DATE_TIME_STR );
			::MessageBox( 0, szVersion.c_str(), "Build version", MB_OK | MB_ICONEXCLAMATION );
			return false;
		}
		if ( szString[0] == '-' )
		{
			szString.erase( 0, 1 );
			NStr::TrimBoth( szString );
			if ( szString.empty() )
			{
				continue;
			}
		}
		// check for spacial cases - save (.sav) and map (.b2m or .b2x)
		if ( szString.size() > 4 )
		{
			const int nExtPos = szString.rfind( '.' );
			if ( nExtPos != std::string::npos )
			{
				std::string szExt = szString.substr( nExtPos );
				NStr::ToLower( &szExt );
				if ( szExt == ".sav" )
				{
					NGlobal::SetVar( "LoadSavedFile", szString );
					continue;
				}
				else if ( ( szExt == ".b2m" ) || ( szExt == ".b2x" ) )
				{
					NGlobal::SetVar( "StartNewMap", szString );
					continue;
				}
			}
		}
		// check for '\"' tag - string entry
		const int nFirstQuotePos = szString.find( '\"' );
		if ( nFirstQuotePos != std::string::npos )
		{
			const int nLastQuotePos = szString.rfind( '\"' );
			NI_ASSERT( nLastQuotePos != std::string::npos,
			           fmt::format( "Can't read string from cmd line string entry \"{}\"", szString ) );
			const std::string szVarName = szString.substr( 0, nFirstQuotePos );
			const std::string szValue = szString.substr( nFirstQuotePos + 1, nLastQuotePos - nFirstQuotePos );
			NGlobal::SetVar( szVarName, szValue );
			//
			continue;
		}
		// check for number
		const int nNumberPos = szString.find_first_of( "-0123456789" );
		if ( nNumberPos != std::string::npos )
		{
			const std::string szVarName = szString.substr( 0, nNumberPos );
			const std::string szValue = szString.substr( nNumberPos );
			NGlobal::SetVar( szVarName, NStr::ToInt( szValue ) );
			continue;
		}
		// no special preferences for parsing - just store this var as integer '1'
		NGlobal::SetVar( szString, 1 );
	}

	if ( nReplayPlayer >= 0 && szReplayFile.empty() )
	{
		MessageBox( 0, "--replay-player needs --replay= as well.", "Replay", MB_OK | MB_ICONERROR );
		return false;
	}
	return true;
}
