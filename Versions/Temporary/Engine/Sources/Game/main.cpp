#include "stdafx.h"

#include <thread>
#include <chrono>

#include <crtdbg.h>
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

bool ProcessCommandLine( LPSTR lpCmdLine );
static void StoreBuildInfo()
{
	const std::string szVersion = fmt::format( "Build {}, {} {}", NVersionInfo::nBuild, NVersionInfo::szDate, NVersionInfo::szTime );
	NGlobal::SetVar( "version.info", szVersion );
}

namespace {
	bool InitCrashpad() {
		base::FilePath handler(L"crashpad_handler.exe");
		base::FilePath db(L"crashpad_db");
		base::FilePath metrics(L"crashpad_metrics");

		auto database = crashpad::CrashReportDatabase::Initialize(db);
		if (!database) {
			return false;
		}
		database->GetSettings()->SetUploadsEnabled(false);

		crashpad::CrashpadClient client;
		return client.StartHandler(handler, db, metrics, "", "", {}, {}, true, false);
	}
}

static std::string szLaunchDirectory;
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    // Keep mimalloc in the executable's import table so its redirect DLL can
    // replace the CRT allocator for every game module before initialization.
    (void)mi_version();

	NGlobal::LoadConfig( "..\\profiles\\startup.cfg" );
	StoreBuildInfo();
	//
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
	const int nLeakId = -1;
	_CrtSetBreakAlloc( nLeakId );

	// crashpad will generate a crash report and write minidump
	InitCrashpad();

	// disable system-critical errors displaying - just send it to calling process
	SetErrorMode( SEM_FAILCRITICALERRORS );
	//
	if ( ProcessCommandLine(lpCmdLine) == false )
		return 0xDEAD;

	std::string szLogFileName, szErrorFileName;
	{
		char buffer[1024];
		GetCurrentDirectory( 1024, buffer );
		szLaunchDirectory = buffer;
		if ( !szLaunchDirectory.empty() ) 
		{
			if ( szLaunchDirectory[szLaunchDirectory.size() - 1] != '\\' ) 
				szLaunchDirectory += '\\';
		}
		//
		szLogFileName = std::string(buffer) + "\\log.txt";
		szErrorFileName = std::string(buffer) + "\\error.txt";
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
//	NWinFrame::ShowSplashScreen( hInstance, true );
	// load and initialize all dll modules, register and initialize singletons
	NGameX::Initialize();
	//
	CObj<CObjectBase> pSplashScreen = is_debugger_present() ? 0 :NSplash::CreateSplashScreen( NMainLoop::GetBaseDir() + "splash.bmp", true );
	//
	CreateAI();
	//
	// initialize win app
	if ( !NWinFrame::SFLB1_InitApplication(hInstance, " Blitzkrieg II", "NIVAL_RTS_ENGINE", MAKEINTRESOURCE(IDI_MAIN)) )
	{
		MessageBox( 0, "InitApplication", "Error", MB_OK );
		return 0xDEAD;
	}
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
	NGlobal::LoadConfig( "..\\profiles\\autoexec.cfg" );
	NGlobal::LoadConfig( "..\\profiles\\game.cfg" );
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

bool ProcessCommandLine( LPSTR lpCmdLine )
{
	for ( NStr::CStringIterator<char, std::string, NStr::CBracketSeparator<char, NStr::SBracketsQuoteTest<char> > > it(lpCmdLine, ' ');
	      !it.IsEnd(); it.Next() )
	{
		// get string
		std::string szString;
		it.Get( &szString );
		NStr::TrimBoth( szString );
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


