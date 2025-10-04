#include "stdafx.h"
#include "mapeditorlib/commandhandlerdefines.h"
#include "mapeditorlib/resourcedefines.h"
#include "scintilla/Platform.h"

#include <crtdbg.h>
#include <Shlwapi.h>

#include "Scintilla/Scintilla.h"
#include "libdb/ResourceManager.h"
#include "libdb/Logger.h"
#include "Misc/StrProc.h"
#include "Misc/HPTimer.h"
#include "System/FileUtils.h"
#include "Main/MainLoop.h"
#include "MapEditorLib/Tools_Registry.h"
#include "Main/MODs.h"

#include "System/SplashScreen.h"

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
#include "ProgressHook.h"
#include "MODContainer.h"

#include "MapeditorLib/MapEditorModule.h"

#include "MapEditorApp.h"
#include "libdb/EditorDb.h"
#include "libdb/DBWatcherClient.h"

#include "System/VFSOperations.h"
#include "System/WinVFS.h"

bool IsRunningOnLocalDrive();


EXTERNVAR CLogger theLogger;

BEGIN_MESSAGE_MAP(CEditorApp, CWinApp)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
END_MESSAGE_MAP() 


// ************************************************************************************************************************ //
// **
// ** map editor app
// **
// **
// **
// ************************************************************************************************************************ //

void DisableFastMemAlloc(); // defined in memory mgr
CEditorApp::CEditorApp() : pMainFrame( 0 )
{
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	const int nBreakId = -1;
	_CrtSetBreakAlloc( nBreakId );
	//
	int *pInitMem = new int;
	delete pInitMem;
	DisableFastMemAlloc();

	//NGlobal::SetVar( "fixrandom", 1 );
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
			CString strVersion;
			strVersion.LoadString( IDS_PROGRAM_VERSION );
			pUserDataContainer->Get()->constUserData.szVersion = strVersion;
		}
		//
		if ( pUserDataContainer->Get()->constUserData.szApplicationTitle.empty() )
		{
			CString strApplicationTitle;
			strApplicationTitle.LoadString( AFX_IDS_APP_TITLE );
			pUserDataContainer->Get()->constUserData.szApplicationTitle = strApplicationTitle;
		}
	}
}


static void StartDBWatcher()
{
	STARTUPINFO startinfo;
	PROCESS_INFORMATION procinfo;
	Zero( startinfo );
	Zero( procinfo );
	startinfo.cb = sizeof( startinfo );
	BOOL bRetVal = CreateProcess( "XDBWatcher.exe", 0, 0, 0, FALSE, 0, 0, NULL, &startinfo, &procinfo );
}

bool CEditorApp::CreateSingletons()
{
	NHPTimer::STime time = 0;
	NHPTimer::GetTime( &time );
	//
	::CoInitialize( 0 );
	HINSTANCE hInstance = ::AfxGetResourceHandle();
	Scintilla_RegisterClasses( hInstance );
	//
	DebugTrace( "EditorApp() Start: %g", NHPTimer::GetTimePassed( &time ) );

	//
	const string &szBaseDir = NMainLoop::GetBaseDir();
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
		theLogger.WriteLog( StrFmt("%s %s", cfg.szApplicationTitle.c_str(), cfg.szVersion.c_str()) );
		theLogger.WriteLog( "" );
		theLogger.WriteLog( StrFmt("StartFolder:             %s", cfg.szStartFolder.c_str()) );
		theLogger.WriteLog( StrFmt("DataStorageFolder:       %s", cfg.szDataStorageFolder.c_str()) );
		theLogger.WriteLog( StrFmt("ExportSourceFolder:      %s", cfg.szExportSourceFolder.c_str()) );
		theLogger.WriteLog( StrFmt("ExportDestinationFolder: %s", cfg.szExportDestinationFolder.c_str()) );
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
		//				StrFmt("Couldn't open game database.\n\n"
		//						"(DB location was specified as : %s)\n\n"
		//						"Most probably it's configuration error.\nRefer to the developers for help.", cfg.szDataStorageFolder.c_str()),
		//				"Error",//StrFmt("%s %s Error", cfg.szApplicationTitle.c_str(), cfg.szVersion.c_str()),
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
		dbconfig.szObjectRecordIDsPath = pUserData->constUserData.szObjectRecordIDsFolder;
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
	NSingleton::RegisterSingleton( new CProgressHook(), IProgressHook::tidTypeID );
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
	const vector<IEditorModule*> &extModules = GetEditorModules();
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
	const vector<IEditorModule*> &extModules = GetEditorModules();
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
	Scintilla_ReleaseResources();
	::CoUninitialize();
	//
	DebugTrace( "EditorApp() Finalize: %g", NHPTimer::GetTimePassed( &time ) );
}


bool CEditorApp::ParseCommandLine( const string &rszCommandLine )
{
	const string szValidRegistryVersion = "1";
	string szRegistryVersion;
	CString strKey;

	CString strPath;
	strPath.LoadString( IDS_REGISTRY_PATH );
	CString strTitle;
	strTitle.LoadString( AFX_IDS_APP_TITLE );
	const string szRegistryKey = StrFmt( "Software\\%s\\%s\\%s",
																			 LPCTSTR( strPath ),
																			 Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str(),
																			 LPCTSTR( strTitle ) );
	//
	{
		CRegistrySection registrySection( HKEY_CURRENT_USER, KEY_READ, szRegistryKey.c_str() );
		strKey.LoadString( IDS_REGISTRY_KEY_VERSION );
		registrySection.LoadString( strKey, &szRegistryVersion, "" );
	}
	//
	if ( ( rszCommandLine.find( "-reg" ) != string::npos ) || ( szRegistryVersion != szValidRegistryVersion ) )
	{
		SHDeleteKey( HKEY_CURRENT_USER, szRegistryKey.c_str() ); 
	}
	else if ( !rszCommandLine.empty() )
	{
		char buffer[2048];
		memset( buffer, 0, 2048 );
		::GetModuleFileName( 0, buffer, 2048 );
		string szAppName = buffer;
		string szFilePath;
		CStringManager::SplitFileName( &szFilePath, 0, 0, szAppName );
		NStr::TrimBoth( szFilePath, "\\/" );
		NFile::SetCurrDir( szFilePath );
	}
	//
	{
		CRegistrySection registrySection( HKEY_CURRENT_USER, KEY_WRITE, szRegistryKey.c_str() );
		strKey.LoadString( IDS_REGISTRY_KEY_VERSION );
		registrySection.SaveString( strKey, szValidRegistryVersion );
	}
	return true;
}


BOOL CEditorApp::InitInstance()
{
	if ( !IsRunningOnLocalDrive() ) 
	{
		return false;
	}
	//
	NGlobal::LoadConfig( "..\\profiles\\startup.cfg" );
	NGlobal::LoadConfig( "..\\profiles\\editor.cfg" );
	//
	InitCommonControls();

	CWinApp::InitInstance();
	RWSetDotNetStyle( false );
	// Получаем командную строку
	string szCommandLine( m_lpCmdLine );
	NStr::TrimBoth( szCommandLine, '\"' );

	// проверяем наличие предыдущего редактора
	CMapEditorSingletonChecker mapEditorSingletonChecker;
	if ( szCommandLine.empty() )
	{
		if ( mapEditorSingletonChecker.BringAppOnTop() )
		{
			return false;
		}
	}
	else
	{
		if ( mapEditorSingletonChecker.OpenFileOnApp( szCommandLine ) )
		{
			return false;
		}
	}

	const bool bShowSplashScreen = ( !IsDebuggerPresent() ) && ( NGlobal::GetVar( "disable_splash_screen", 0 ) == 0 );
	CObj<CObjectBase> pSplashScreen;
	//Показать SplashScreen
	if ( bShowSplashScreen )
		pSplashScreen = NSplash::CreateSplashScreen( "..\\splash.bmp", false );

	// Нет такого же приложения, продолжаем инициализацию
#if defined( _DO_SEH ) && !defined( _DEBUG )
	// set StructuredExceptionHandler 
	SetCrashHandler();
#endif // defined( _DO_SEH ) && !defined( _DEBUG )

	// Загружаем данные пользователя с диска (UserData.xml и ConstUserData.xml)
	CreateUserDataSingleton();

	// Пропарсить все аргументы командной строки
	if ( !ParseCommandLine( szCommandLine ) )
	{
		return false;
	}

	// Установить рабочий раздел Registry
	CString strPath;
	strPath.LoadString( IDS_REGISTRY_PATH );
	const string szRegistryKey = StrFmt( "%s\\%s",
																			 LPCTSTR( strPath ),
																			 Singleton<IUserDataContainer>()->Get()->constUserData.szApplicationTitle.c_str() );
	SetRegistryKey( szRegistryKey.c_str() );
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
	pMainFrame = new CMainFrame();
	m_pMainWnd = dynamic_cast<CWnd*>( pMainFrame );
	//
	DebugTrace( "EditorApp() Create mainFrame: %g", NHPTimer::GetTimePassed( &time ) );
	//	
	if ( !pMainFrame->LoadFrame( IDR_EDITORTYPE ) )
	{
		delete pMainFrame;
		pMainFrame = 0;
		m_pMainWnd = 0;
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
	const vector<IEditorModule*> &extModules = GetEditorModules();
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
	m_nCmdShow = SW_SHOWNORMAL;
	/**
	if ( szCommandLine.find( "-topmost" ) != string::npos )
	{
		pMainFrame->ModifyStyleEx( 0, WS_EX_TOPMOST, 0 );
	}
	/**/
	pMainFrame->ShowWindow( m_nCmdShow );
	pMainFrame->UpdateWindow();
	if ( !szCommandLine.empty() && ( szCommandLine != "-reg" ) )
	{
		pMainFrame->OpenResource( szCommandLine );
	}
	return true;
}


int CEditorApp::ExitInstance() 
{
#if defined( _DO_SEH ) && !defined( _DEBUG )
	// set StructuredExceptionHandler 
	ResetCrashHandler();
#endif // defined( _DO_SEH ) && !defined( _DEBUG )
	//
	// Убираем все созданные в редакторе Singletons
	DestroySingletons();
	//
	return CWinApp::ExitInstance();
}


void CEditorApp::OnAppAbout()
{
	if ( pMainFrame )
	{
		pMainFrame->OnHelpAbout();
	}
}


void CEditorApp::OnHelp() 
{
	if ( pMainFrame != 0 )
  {
		pMainFrame->OnHelpContents();
  }
}


BOOL CEditorApp::SaveAllModified() 
{
	return TRUE;
}

void CEditorApp::SetMapFileName( const string &szMapFileName )
{
	CMapEditorSingletonBase::SetMapFileName( szMapFileName );
}

void CEditorApp::RegisterEditors()
{
//	g_RegisterEditorsSemiAutoMagic.Load(); // сейчас регистрируются только в коде
}

// basement storage  


