#include "stdafx.h"

#include <thread>
#include <chrono>

#include <mimalloc-new-delete.h>
#include "resource.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/IconsSet.h"

#include "Misc/StrProc.h"
#include "System/FileUtils.h"

#include "Input/Input.h"
#include "SceneB2/Scene.h"
#include "SceneB2/Cursor.h"
#include "System/GResource.h"

#include "Sound/SFX.h"
#include "3Dmotor/Gfx.h"

#include "Main/Profiles.h"
#include "WinFrame2Input.h"
#include "Main/MainLoopCommands.h"
#include "AILogic/CreateAI.h"
#include "VersionInfo.h"
#include "libdb/Db.h"

#include "System/FilePath.h"
#include "System/Commands.h"

#include "System/VFSOperations.h"

#include "System/SplashScreen.h"
#include "port/messagebox.h"
#include "Main/MODs.h"

#include "port/debugging.h"

#include <exception>
#include <typeinfo>

#if !BOOST_OS_WINDOWS
#include <cxxabi.h>
#endif

#if !BOOST_OS_WINDOWS
// Captured where the exception was thrown rather than where it was caught, which
// is the difference between a stack that names the failure and one that names
// this file. Boost records it by overriding __cxa_throw, so it needs no
// cooperation from whatever threw, and no MSVC equivalent exists.
#include <boost/stacktrace/stacktrace.hpp>
#endif

#include <client/crashpad_client.h>
#include <client/crash_report_database.h>
#include <client/settings.h>

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
	void PostStorageInitialize();
};

static bool ProcessCommandLine( const std::vector<std::string> &arguments );
static int RunGame( const std::vector<std::string> &arguments );
static int RunGameGuarded( const std::vector<std::string> &arguments );
static void StoreBuildInfo()
{
	const std::string szVersion = fmt::format( "Build {}, {} {}", NVersionInfo::nBuild, NVersionInfo::szDate, NVersionInfo::szTime );
	NGlobal::SetVar( "version.info", szVersion );
}

namespace {
	bool InitCrashpad() {
		// base::FilePath holds a wstring on Windows and a string everywhere else,
		// which is what FILE_PATH_LITERAL is for: it adds the L only where one
		// belongs. The handler is a real executable, so its name carries the
		// platform suffix; the other two name directories and do not.
		//
		// All three stay relative, resolved against the working directory, which is
		// where the handler is installed beside the game.
#if BOOST_OS_WINDOWS
		base::FilePath handler( FILE_PATH_LITERAL( "crashpad_handler.exe" ) );
#else
		base::FilePath handler( FILE_PATH_LITERAL( "crashpad_handler" ) );
#endif
		base::FilePath db( FILE_PATH_LITERAL( "crashpad_db" ) );
		base::FilePath metrics( FILE_PATH_LITERAL( "crashpad_metrics" ) );

		auto database = crashpad::CrashReportDatabase::Initialize(db);
		if (!database) {
			return false;
		}
		database->GetSettings()->SetUploadsEnabled(false);

		crashpad::CrashpadClient client;
		return client.StartHandler(handler, db, metrics, "", "", {}, {}, true, false);
	}
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
		ReportFatalError( fmt::format( "{}: {}", CurrentExceptionTypeName(), e.what() ) );
	}
	catch ( ... )
	{
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
	for ( NStr::CStringIterator<char, std::string, NStr::CBracketSeparator<char, NStr::SBracketsQuoteTest<char> > > it( lpCmdLine, ' ' );
	      !it.IsEnd(); it.Next() )
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
	return RunGameGuarded( SplitCommandLine( lpCmdLine ) );
}

#else

int main( int argc, char *argv[] )
{
	// argv[0] is the program itself, which WinMain is not given either
	return RunGameGuarded( std::vector<std::string>( argv + 1, argv + argc ) );
}

#endif

static int RunGame( const std::vector<std::string> &arguments )
{
    // Keep mimalloc in the executable's import table so its redirect DLL can
    // replace the CRT allocator for every game module before initialization.
    (void)mi_version();

	NGlobal::LoadConfig( "../profiles/startup.cfg" );
	StoreBuildInfo();
	// crashpad will generate a crash report and write minidump
	InitCrashpad();

	// Suppress the shell's own error boxes for a missing DLL or an unreadable
	// drive, so the failure goes to the calling process instead. Windows only:
	// there is no equivalent notion off it, and nothing takes its place.
#if BOOST_OS_WINDOWS
	SetErrorMode( SEM_FAILCRITICALERRORS );
#endif
	//
	if ( ProcessCommandLine( arguments ) == false )
		return 0xDEAD;

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
	}

	NGScene::SFLB3_RunResourceLoadingThread();
	// show splash screen during program starting
//	NWinFrame::ShowSplashScreen( true );
	// load and initialize all dll modules, register and initialize singletons
	NGameX::Initialize();
	//
	CObj<CObjectBase> pSplashScreen = is_debugger_present() ? 0 :NSplash::CreateSplashScreen( NMainLoop::GetBaseDir() + "splash.bmp", true );
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
	if ( !NWinFrame::SFLB1_InitApplication(" Blitzkrieg II", "NIVAL_RTS_ENGINE", pIcon) )
	{
		MessageBox( 0, "InitApplication", "Error", MB_OK );
		return 0xDEAD;
	}
	// main.ico is installed beside the game's other run-time files. Does nothing
	// on Windows, where the resource above already covered it.
	NWinFrame::SetIcon( NMainLoop::GetBaseDir() + "main.ico" );
	// init graphics
	if ( !NGfx::Init3D(NWinFrame::GetWnd()) )
	{
		ASSERT(0); // DX not found
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
	std::string szMOD2Attach = NStr::ToMBCS( NGlobal::GetVar("current_attached_mod", "") );
	if ( !szMOD2Attach.empty() )
		szMOD2Attach = NMainLoop::GetBaseDir() + "MODs\\" + szMOD2Attach;
	if ( NMOD::DoesMODAttached(szMOD2Attach) == false )
		NMOD::InstantAttachMOD( szMOD2Attach, NDb::DATABASE_MODE_GAME );
	//
	NGlobal::LoadConfig( "../profiles/autoexec.cfg" );
	NGlobal::LoadConfig( "../profiles/game.cfg" );
	//
	NGlobal::SetVar( "code_version_number", REVISION_NUMBER_STR );
	NGlobal::SetVar( "code_build_date_time", BUILD_DATE_TIME_STR );
	// 
	pSplashScreen = 0;
	// setup video mode
	if ( !Scene()->SetupMode(SCENE_MODE_WINDOWED, false) )
	{
		MessageBox( 0, "Can't setup scene mode from config", "Error", MB_OK );
		return 0xDEAD;
	}
	// start
	Cursor()->Acquire( true );
	while ( 1 ) 
	{
		NWinFrame::PumpMessages();

		CWinToInputMessageConverter convert;
		convert.Do();

		const bool bAppActive = NWinFrame::IsAppActive();
		if ( NWinFrame::IsExit() )
		{
			NWinFrame::ResetExit();
			NMainLoop::Command( CreateICExitGame() );				// generate 'EXIT' command
		}
		if ( bAppActive )
			NGfx::CheckBackBufferSize();
		NGfx::SetGamma( bAppActive );//bSetGamma );
		if ( !NMainLoop::StepApp(bAppActive) )
			break;
		if ( !bAppActive )
			std::this_thread::sleep_for( std::chrono::milliseconds( 40 ) );
	}
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
	return 0;
}

static bool ProcessCommandLine( const std::vector<std::string> &arguments )
{
	for ( std::vector<std::string>::const_iterator it = arguments.begin(); it != arguments.end(); ++it )
	{
		// get string
		std::string szString = *it;
		if ( szString.empty() )
			continue;
		// check for '-' at the begining
		if ( szString == "-show-version" )
		{
			std::string szVersion = fmt::format( "Version: {}\nBuild date/time: {}\n", REVISION_NUMBER_STR, BUILD_DATE_TIME_STR );
			fmt::print( "{}", szVersion );
			return false;
		}
		else if ( szString == "-show-version-mb" )
		{
			std::string szVersion = fmt::format( "Version: {}\nBuild date/time: {}\n", REVISION_NUMBER_STR, BUILD_DATE_TIME_STR );
			::MessageBox( 0, szVersion.c_str(), "Build version", MB_OK | MB_ICONEXCLAMATION );
			return false;
		}
		if ( szString[0] == '-' ) 
		{
			szString.erase( 0, 1 );
			NStr::TrimBoth( szString );
			if ( szString.empty() ) 
				continue;
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
				else if ( (szExt == ".b2m") || (szExt == ".b2x") ) 
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
			NI_ASSERT( nLastQuotePos != string::npos, fmt::format("Can't read string from cmd line string entry \"{}\"", szString) );
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
			NGlobal::SetVar( szVarName, NStr::ToInt(szValue) );
			continue;
		}
		// no special preferences for parsing - just store this var as integer '1'
		NGlobal::SetVar( szString, 1 );
	}
	return true;
}


