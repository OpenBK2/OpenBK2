#include "stdafx.h"

// CoInitialize and CoUninitialize, which came in through Shlwapi.h until the
// profile stopped needing SHDeleteKey.
#include <objbase.h>
#include "MapEditorLib/Resources.h"
#include <fmt/format.h>
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/ResourceDefines.h"

#include "libdb/ResourceManager.h"
#include "libdb/Logger.h"
#include "Misc/StrProc.h"
#include "Misc/HPTimer.h"
#include "System/FileUtils.h"
#include "Main/MainLoop.h"
#include "Main/MODs.h"

#include "System/SplashScreen.h"

#include "libdb_export.h"

// Singletons
#include "UserDataContainer.h"
#include "MainFrameContainer.h"
#include "CommandHandlerContainer.h"
#include "ObjectCollector.h"
#include "ControllerContainer.h"
#include "ViewContainer.h"
#include "ChildFrameContainer.h"
#include "EditorContainer.h"
#include "BuilderContainer.h"
#include "ExporterContainer.h"
#include "FolderCallback.h"
#include "MODContainer.h"

#include "MapEditorLib/MapEditorModule.h"

#include "MapEditorApp.h"
#include "AppProfile.h"
#include "MainFrameWx.h"
#include "MainFrameShared.h"
#include "EditorInstance.h"
#include "libdb/EditorDb.h"
#include "libdb/DBWatcherClient.h"

#include "System/VFSOperations.h"
#include "System/WinVFS.h"

#include "port/debugging.h"
#include "port/process.h"


EXTERNVAR LIBDB_EXPORT CLogger theLogger;

CEditorApp *CEditorApp::pInstance = nullptr;


// ************************************************************************************************************************ //
// **
// ** map editor app
// **
// **
// **
// ************************************************************************************************************************ //

CEditorApp::CEditorApp()
{
	// The CRT debug heap calls that stood here are gone: _CrtSetDbgFlag with
	// _CRTDBG_LEAK_CHECK_DF, a _CrtSetBreakAlloc( -1 ) that meant never break,
	// and a new/delete pair that forced the debug heap to initialise before the
	// leak tracking started.
	//
	// Two of them did nothing in any configuration, and the flag only did
	// anything in a Debug build, where it dumped the leaks to the debugger at
	// exit. Nothing was reading that, and every tool worth using for the
	// question -- ASan, Valgrind, heaptrack -- works on the running program
	// without the CRT's help and says more when it does.
	//NGlobal::SetVar( "fixrandom", 1 );
	pInstance = this;
}


CEditorApp::~CEditorApp()
{
	if ( pInstance == this )
	{
		pInstance = nullptr;
	}
}


void CEditorApp::CreateUserDataSingleton()
{
	GameXInitialize();
	//
	NSingleton::RegisterSingleton( new CUserDataContainer(), IUserDataContainer::tidTypeID );
	if ( IUserDataContainer *pUserDataContainer = Singleton<IUserDataContainer>() )
	{
		pUserDataContainer->Load();
		//
		if ( pUserDataContainer->Get()->constUserData.szVersion.empty() )
		{
			std::string strVersion = NResources::GetString( IDS_PROGRAM_VERSION );
			pUserDataContainer->Get()->constUserData.szVersion = strVersion;
		}
		//
		if ( pUserDataContainer->Get()->constUserData.szApplicationTitle.empty() )
		{
			std::string strApplicationTitle = NResources::GetString( AFX_IDS_APP_TITLE );
			pUserDataContainer->Get()->constUserData.szApplicationTitle = strApplicationTitle;
		}
	}
}


// Whatever XDBWatcher.exe was.
//
// Ported as it stood, not repaired: CreateProcess becomes LaunchDetachedIn so
// the file compiles off Windows, and the result is still ignored, as the
// unused BOOL it was assigned to ignored it.
//
// **There is no XDBWatcher.exe.** No CMakeLists builds one, it is in no
// install, and the only trace of it in the tree is XDBWatcherClient.tlh and
// .tli, the COM wrappers generated from its type library. So this has been
// failing silently on every start for as long as the CMake build has existed.
// Worth deciding about rather than keeping: the client half,
// NDBWatcherClient::RegisterSingleton just below, is live and presumably
// copes with nothing being there to talk to.
static void StartDBWatcher()
{
	LaunchDetachedIn( "XDBWatcher.exe", "", "" );
}

bool CEditorApp::CreateSingletons()
{
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	::CoInitialize( 0 );
	//
	DebugTrace( "EditorApp() Start: %g", NHPTimer::GetTimePassed( &time ) );

	//
	const std::string &szBaseDir = NMainLoop::GetBaseDir();
	// Create logging stream
	{
		CDataStream *pStream = new CFileStream( szBaseDir + "\\MapEditor.log", CFileStream::WIN_CREATE );
		if ( pStream->IsOk() )
			theLogger.SetLogStream( pStream );
		else
			delete pStream;
	}

	if ( IUserDataContainer *pUserDataContainer = Singleton<IUserDataContainer>() )
	{
		const SUserData::SConstUserData &cfg = pUserDataContainer->Get()->constUserData;
		theLogger.WriteLog( fmt::format("{} {}", cfg.szApplicationTitle.c_str(), cfg.szVersion.c_str()) );
		theLogger.WriteLog( "" );
		theLogger.WriteLog( fmt::format("StartFolder:             {}", cfg.szStartFolder.c_str()) );
		theLogger.WriteLog( fmt::format("DataStorageFolder:       {}", cfg.szDataStorageFolder.c_str()) );
		theLogger.WriteLog( fmt::format("ExportSourceFolder:      {}", cfg.szExportSourceFolder.c_str()) );
		theLogger.WriteLog( fmt::format("ExportDestinationFolder: {}", cfg.szExportDestinationFolder.c_str()) );
		theLogger.WriteLog( "" );

		NHPTimer::GetTimePassed( &time );
		IResourceManager::InitSingleton();
		DebugTrace( "EditorApp() IResourceManager::InitSingleton(): %g", NHPTimer::GetTimePassed( &time ) );

		NMOD::InstantAttachMOD( pUserDataContainer->Get()->szOpenedMODFolder, NDb::DATABASE_MODE_EDITOR );

		//pMainVFS = NVFS::CreateWinVFS( cfg.szDataStorageFolder );
		//NVFS::SetMainVFS( pMainVFS );
		//pMainFileCreator = NVFS::CreateWinFileCreator( cfg.szDataStorageFolder );
		//NVFS::SetMainFileCreator( pMainFileCreator );
		//{
		//	DebugTrace( "EditorApp() OpenStorage(): %g", NHPTimer::GetTimePassed( &time ) );
		//	if ( NDb::OpenDatabase( pMainVFS, pMainFileCreator, NDb::DATABASE_MODE_EDITOR ) )
		//	{
		//		Singleton<IResourceManager>()->SetDataDir( cfg.szDataStorageFolder );
		//		DebugTrace( "EditorApp() OpenDatabase(): %g", NHPTimer::GetTimePassed( &time ) );
		//	}
		//	else
		//	{
		//		theLogger.WriteLog( "ERROR: Failed to open game database from $DataStorageFolder" );
		//		::MessageBox( ::GetDesktopWindow(),
		//				fmt::format("Couldn't open game database.\n\n"
		//						"(DB location was specified as : %s)\n\n"
		//						"Most probably it's configuration error.\nRefer to the developers for help.", cfg.szDataStorageFolder.c_str()),
		//				"Error",//fmt::format("{} {} Error", cfg.szApplicationTitle.c_str(), cfg.szVersion.c_str()),
		//				MB_OK  | MB_ICONERROR
		//				);
		//		return false;
		//	}
		//}
	}
	else
	{
		theLogger.WriteLog( "Failed to create UserDataContainer singleton" );
		return false;
	}

	//
	//register editors from file
	RegisterEditors();
	//
	{
		const SUserData *pUserData = Singleton<IUserDataContainer>()->Get();
		IResourceManager::SDBConfig dbconfig;
		dbconfig.szSrcFilePath = pUserData->constUserData.szExportSourceFolder;
		dbconfig.szDstFilePath = pUserData->constUserData.szExportDestinationFolder;
		Singleton<IResourceManager>()->SetConfig( dbconfig );
	}
	//
	// load map editor project-specific module
	LoadMapEditorModule( "" );
	DebugTrace( "EditorApp() LoadMapEditorModule(): %g", NHPTimer::GetTimePassed( &time ) );

	StartDBWatcher();
	NDBWatcherClient::RegisterSingleton();

	NSingleton::RegisterSingleton( new CMainFrameContainer(), IMainFrameContainer::tidTypeID );
	NSingleton::RegisterSingleton( new CCommandHandlerContainer(), ICommandHandlerContainer::tidTypeID );
	NSingleton::RegisterSingleton( new CObjectFilterCollector(), IObjectFilterCollector::tidTypeID );
	NSingleton::RegisterSingleton( new CObjectCollector(), IObjectCollector::tidTypeID );
	NSingleton::RegisterSingleton( new CControllerContainer(), IControllerContainer::tidTypeID );
	NSingleton::RegisterSingleton( new CViewContainer(), IViewContainer::tidTypeID );
	NSingleton::RegisterSingleton( new CChildFrameContainer(), IChildFrameContainer::tidTypeID );
	NSingleton::RegisterSingleton( new CEditorContainer(), IEditorContainer::tidTypeID );
	NSingleton::RegisterSingleton( new CBuilderContainer(), IBuilderContainer::tidTypeID );
	NSingleton::RegisterSingleton( new CExporterContainer(), IExporterContainer::tidTypeID );
	NSingleton::RegisterSingleton( new CFolderCallback(), IFolderCallback::tidTypeID );
	NSingleton::RegisterSingleton( new CMODContainer(), IMODContainer::tidTypeID );
	//
	DebugTrace( "EditorApp() Create singletons: %g", NHPTimer::GetTimePassed( &time ) );
	//
	if ( ICommandHandlerContainer *pCommandHandlerContainer = Singleton<ICommandHandlerContainer>() )
	{
		pCommandHandlerContainer->Register( CHID_MAIN, ID_MAIN_FIRST_COMMAND_ID, ID_MAIN_LAST_COMMAND_ID );
		pCommandHandlerContainer->Register( CHID_SELECTION, ID_SELECTION_FIRST_COMMAND_ID, ID_SELECTION_LAST_COMMAND_ID );
		pCommandHandlerContainer->Register( CHID_PROPERTY_CONTROL, ID_PC_FIRST_COMMAND_ID, ID_PC_LAST_COMMAND_ID );
		pCommandHandlerContainer->Register( CHID_OBJECT, ID_OBJECT_FIRST_COMMAND_ID, ID_OBJECT_LAST_COMMAND_ID );
		pCommandHandlerContainer->Register( CHID_CONTROLLER_CONTAINER, ID_CC_FIRST_COMMAND_ID, ID_CC_LAST_COMMAND_ID );
		pCommandHandlerContainer->Register( CHID_LOG, ID_LOG_FIRST_COMMAND_ID, ID_LOG_LAST_COMMAND_ID );
		pCommandHandlerContainer->Register( CHID_VIEW, ID_VIEW_FIRST_COMMAND_ID, ID_VIEW_LAST_COMMAND_ID );
	}
	//
	const std::vector<IEditorModule*> &extModules = GetEditorModules();
	for ( int i = 0; i < extModules.size(); ++i )
	{
		extModules[i]->ModuleCreate();
	}
	//
	DebugTrace( "EditorApp() ModuleCreate(): %g", NHPTimer::GetTimePassed( &time ) );
	//
	return true;
}


void CEditorApp::DestroySingletons()
{
	//
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	const std::vector<IEditorModule*> &extModules = GetEditorModules();
	for ( int i = 0; i < extModules.size(); ++i )
	{
		extModules[i]->ModuleDestroy();
	}
	//
	//NDb::SaveChangedIndex();
	//
	DebugTrace( "EditorApp() ModuleDestroy(): %g", NHPTimer::GetTimePassed( &time ) );
	//
	if ( ICommandHandlerContainer *pCommandHandlerContainer = Singleton<ICommandHandlerContainer>() )
	{
		pCommandHandlerContainer->UnRegister( CHID_MAIN );
		pCommandHandlerContainer->UnRegister( CHID_SELECTION );
		pCommandHandlerContainer->UnRegister( CHID_PROPERTY_CONTROL );
		pCommandHandlerContainer->UnRegister( CHID_OBJECT );
		pCommandHandlerContainer->UnRegister( CHID_CONTROLLER_CONTAINER );
		pCommandHandlerContainer->UnRegister( CHID_LOG );
		pCommandHandlerContainer->UnRegister( CHID_VIEW );
	}
	//
	NSingleton::UnRegisterSingleton( IMODContainer::tidTypeID );
	NSingleton::UnRegisterSingleton( NDBWatcherClient::IDBWatcherClient::tidTypeID );
	NSingleton::UnRegisterSingleton( IFolderCallback::tidTypeID );
	NSingleton::UnRegisterSingleton( IExporterContainer::tidTypeID );
	NSingleton::UnRegisterSingleton( IBuilderContainer::tidTypeID );
	NSingleton::UnRegisterSingleton( IEditorContainer::tidTypeID );
	NSingleton::UnRegisterSingleton( IChildFrameContainer::tidTypeID );
	NSingleton::UnRegisterSingleton( IViewContainer::tidTypeID );
	NSingleton::UnRegisterSingleton( IControllerContainer::tidTypeID );
	NSingleton::UnRegisterSingleton( IObjectCollector::tidTypeID );
	NSingleton::UnRegisterSingleton( IObjectFilterCollector::tidTypeID );
	NSingleton::UnRegisterSingleton( ICommandHandlerContainer::tidTypeID );
	NSingleton::UnRegisterSingleton( IMainFrameContainer::tidTypeID );
	//
	DebugTrace( "EditorApp() close singletons: %g", NHPTimer::GetTimePassed( &time ) );
	//
	// unload project-specific module
	UnloadMapEditorModule();
	//
	DebugTrace( "EditorApp() UnloadMapEditorModule(): %g", NHPTimer::GetTimePassed( &time ) );
	//
	if ( IUserDataContainer *pUserDataContainer = Singleton<IUserDataContainer>() )
	{
		pUserDataContainer->Save();
	}
	NSingleton::UnRegisterSingleton( IUserDataContainer::tidTypeID );
	//
	IResourceManager::UninitSingleton();
	//
	DebugTrace( "EditorApp() close other singletons: %g", NHPTimer::GetTimePassed( &time ) );
	//
	pMainVFS = 0;
	pMainFileCreator = 0;
	//
	DebugTrace( "EditorApp() CloseStorage(): %g", NHPTimer::GetTimePassed( &time ) );
	//
	NDb::CloseDatabase();
	//
	DebugTrace( "EditorApp() RemoveAllResources(): %g", NHPTimer::GetTimePassed( &time ) );
	//
	NSingleton::DoneSingletons();
	//
	::CoUninitialize();
	//
	DebugTrace( "EditorApp() Finalize: %g", NHPTimer::GetTimePassed( &time ) );
}


bool CEditorApp::ParseCommandLine( const std::string &rszFileToOpen, bool bResetProfile )
{
	const std::string szValidRegistryVersion = "1";
	std::string szRegistryVersion;
	std::string strKey;

	// At the profile's own level rather than in a section, which is where
	// CWinApp's profile calls put it.
	strKey = NResources::GetString( IDS_REGISTRY_KEY_VERSION );
	szRegistryVersion = NAppProfile::GetString( "", strKey, "" );
	//
	if ( bResetProfile || ( szRegistryVersion != szValidRegistryVersion ) )
	{
		NAppProfile::DeleteAll();
	}
	else if ( !rszFileToOpen.empty() )
	{
		char buffer[2048];
		memset( buffer, 0, 2048 );
		::GetModuleFileName( 0, buffer, 2048 );
		std::string szAppName = buffer;
		std::string szFilePath;
		CStringManager::SplitFileName( &szFilePath, 0, 0, szAppName );
		NStr::TrimBoth( szFilePath, "\\/" );
		NFile::SetCurrDir( szFilePath );
	}
	//
	NAppProfile::WriteString( "", NResources::GetString( IDS_REGISTRY_KEY_VERSION ), szValidRegistryVersion );
	return true;
}


bool CEditorApp::Initialize( const std::vector<std::string> &rArgs )
{
	//
	NGlobal::LoadConfig( "..\\profiles\\startup.cfg" );
	NGlobal::LoadConfig( "..\\profiles\\editor.cfg" );
	//
	// The InitCommonControls that stood here is gone, and it was redundant
	// rather than probably redundant: wxApp::Initialize calls it (src/msw/app.cpp),
	// and that runs inside wxEntryStart, before OnInit -- which is what calls
	// this. Since wxIMPLEMENT_APP took over WinMain, the common controls have
	// always been up by the time the editor starts.

	// Получаем командную строку
	//
	// The whole vocabulary: one file to open, and -reg. This used to be the tail
	// of the command line as a single string, with the surrounding quotes taken
	// off by hand; the arguments arrive split and unquoted now, so neither the
	// trim nor the hand-written program-name skip in WxHost.cpp is needed.
	std::string szFileToOpen;
	bool bResetProfile = false;
	for ( const std::string &rszArg : rArgs )
	{
		if ( rszArg == "-reg" )
		{
			bResetProfile = true;
		}
		else if ( szFileToOpen.empty() )
		{
			szFileToOpen = rszArg;
		}
	}

	// проверяем наличие предыдущего редактора
	//
	// Asked of wxSingleInstanceChecker rather than of a named file mapping, and
	// asked first: the old code looked for the running editor's window and took
	// finding one as the answer, so an editor that was still starting and had
	// not yet published its handle was invisible. The lock exists from the
	// moment the first instance asks.
	if ( NEditorInstance::IsAnotherRunning() )
	{
		// It may still be starting and not yet answering, in which case there is
		// nothing useful this instance can do but leave: two editors over one
		// database is what the exclusion is for.
		NEditorInstance::AskRunningToOpen( szFileToOpen );
		return false;
	}

	const bool bShowSplashScreen = ( !is_debugger_present() ) && ( NGlobal::GetVar( "disable_splash_screen", 0 ) == 0 );
	CObj<CObjectBase> pSplashScreen;
	//Показать SplashScreen
	if ( bShowSplashScreen )
		pSplashScreen = NSplash::CreateSplashScreen( "..\\splash.bmp", false );

	// Нет такого же приложения, продолжаем инициализацию

	// Загружаем данные пользователя с диска (UserData.xml и ConstUserData.xml)
	CreateUserDataSingleton();

	// Пропарсить все аргументы командной строки
	if ( !ParseCommandLine( szFileToOpen, bResetProfile ) )
	{
		return false;
	}

	// The registry profile needed SetRegistryKey here; NAppProfile works its
	// key out from the same strings when asked.
	// Создаем все необходимые структуры данных
	if ( !CreateSingletons() )
	{
		DestroySingletons();
		return false;
	}
	// Создаем главное окно
	//
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	// Before the editors make their controls. MFC is not told about it:
	// m_pMainWnd stays null, since the frame is wx's.
	if ( !NMainFrameWx::Create() )
	{
		return false;
	}
	//
	DebugTrace( "EditorApp() Load mainFrame: %g", NHPTimer::GetTimePassed( &time ) );
	//	
	// после создания MainFrame
	GameXPostStorageInitialize();
	//
	DebugTrace( "EditorApp() GameXPostStorageInitialize(): %g", NHPTimer::GetTimePassed( &time ) );
	//	
	// after main frame initialization
	const std::vector<IEditorModule*> &extModules = GetEditorModules();
	for ( int i = 0; i < extModules.size(); ++i )
	{
		extModules[i]->ModulePostCreateMainFrame();
	}
	//
	DebugTrace( "EditorApp() ModulePostCreateMainFrame(): %g", NHPTimer::GetTimePassed( &time ) );
	//	
	// Окно создано, закрываем Splash Screen
	if ( bShowSplashScreen )
		pSplashScreen = 0;
	//Singleton<IResourceManager>()->ResetCache();
	//
	//DebugTrace( "EditorApp() ResetCache(): %g", NHPTimer::GetTimePassed( &time ) );
	//	
	// И показываем основное окно
	/**
	if ( szCommandLine.find( "-topmost" ) != std::string::npos )
	{
		pMainFrame->ModifyStyleEx( 0, WS_EX_TOPMOST, 0 );
	}
	/**/
	NMainFrameWx::Show();
	// The "-reg" this used to exclude by hand is a switch now and never reaches
	// szFileToOpen, which is the argument that is not one.
	if ( !szFileToOpen.empty() )
	{
		NMainFrameShared::OpenResource( szFileToOpen );
	}
	return true;
}


void CEditorApp::Shutdown()
{
	// Убираем все созданные в редакторе Singletons
	DestroySingletons();
}

void CEditorApp::SetMapFileName( const std::string &szMapFileName )
{
	NEditorInstance::SetName( szMapFileName );
}

void CEditorApp::RegisterEditors()
{
//	g_RegisterEditorsSemiAutoMagic.Load(); // сейчас регистрируются только в коде
}

// basement storage  


