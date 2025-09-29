#include "StdAfx.h"
#include "resource.h"
#include "ELK.h"

#include <crtdbg.h>
#include "MainFrm.h"
#include "AboutDialog.h"
#include "../System/FileUtils.h"
#include "../MapEditorLib/Tools_Registry.h"
#include "../Image/Image.h"
#include "../Misc/StrProc.h"
#include "../ED_Common/Initialize.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CELKApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_HELP_CONTENTS, OnHelpContents)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DisableFastMemAlloc(); // defined in memory mgr
CELKApp::CELKApp() : pMainFrame( 0 )
{
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	const int nBreakId = -1;
	_CrtSetBreakAlloc( nBreakId );
	//
	int *pInitMem = new int;
	delete pInitMem;
	DisableFastMemAlloc();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CELKApp theApp;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CELKApp::InitInstance()
{
	ManualLoadEDCommonLibrary();
	//
	InitCommonControls();
	CWinApp::InitInstance();
	RWSetDotNetStyle( false );

	{
		CString strProgramTitle;
		strProgramTitle.LoadString( AFX_IDS_APP_TITLE );

		string szMessage;
		char pBuffer[0xFFF + 1];
		::GetCurrentDirectory( 0xFFF, pBuffer );
		string szZIPToolPath = string( pBuffer ) + string( "\\" ) + string( CELK::ZIP_EXE );
		if ( !NFile::DoesFileExist( szZIPToolPath.c_str() ) )
		{
			szMessage = StrFmt( _T( "Can't find file \"%s\" in ELK work directory: %s\\." ), CELK::ZIP_EXE, pBuffer );
		}
		if ( !szMessage.empty() )
		{
			::MessageBox( ::GetDesktopWindow(), szMessage.c_str(), strProgramTitle, MB_OK | MB_ICONSTOP );
			return false;
		}

		//TODO{INITIALIZE
		//регистрируем IImageProcessor
		/**
		HMODULE hImage = LoadLibrary( ( string( pBuffer ) + _T( "\\image.dll" ) ).c_str() );
		if ( hImage )
		{
			GETMODULEDESCRIPTOR pfnGetModuleDescriptor = reinterpret_cast<GETMODULEDESCRIPTOR>( GetProcAddress( hImage, _T( "GetModuleDescriptor" ) ) );
			if ( pfnGetModuleDescriptor )
			{
				const SModuleDescriptor *pDesc = ( *pfnGetModuleDescriptor )();
				if ( pDesc && pDesc->pFactory )
				{
					IImageProcessor *pIP = CreateObject<IImageProcessor>( pDesc->pFactory, IImageProcessor::tidTypeID );
					if ( pIP )
					{
						RegisterSingleton( IImageProcessor::tidTypeID, pIP );
					}
				}
			}
		}
		/**/
	}

#if defined( _DO_SEH ) && !defined( _DEBUG )
	// set StructuredExceptionHandler 
	SetCrashHandler();
#endif // defined( _DO_SEH ) && !defined( _DEBUG )

	CString strRegistryPathName;
	strRegistryPathName.LoadString( IDS_REGISTRY_PATH );
	SetRegistryKey( strRegistryPathName );

	pMainFrame = new CMainFrame();
	if ( !pMainFrame )
	{
		return false;
	}
	//если введен ключ, включаем расширенную функциональность	
	{
		string szCommandLine( m_lpCmdLine );
		NStr::ToLower( &szCommandLine );
		pMainFrame->bShortApperence = ( szCommandLine != string( _T( "-advanced") ) );
	}
	//если проинсталлирована игра, включаем поддержку игры
	{
		string szGameFolder;
		CRegistrySection registrySection( HKEY_LOCAL_MACHINE, KEY_READ, CELK::GAME_REGISTRY_FOLDER );
		registrySection.LoadString( CELK::GAME_REGISTRY_KEY, &szGameFolder, "" );
		pMainFrame->bGameExists = ( !szGameFolder.empty() );
	}	
	
	m_pMainWnd = dynamic_cast<CWnd*>( pMainFrame );
	if ( pMainFrame->bShortApperence )
	{
		if ( !pMainFrame->LoadFrame( IDR_SHORT_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, 0, 0 ) )
		{
			//NI_ASSERT( 0, StrFmt( _T( "CELKApp::InitInstance, can't create main frame" ) ) ); // Unable to load frame
			return false;
		}
	}
	else
	{
		if ( !pMainFrame->LoadFrame( IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, 0, 0 ) )
		{
			//NI_ASSERT( 0, StrFmt( _T( "CELKApp::InitInstance, can't create main frame" ) ) ); // Unable to load frame
			return false;
		}
	}
	pMainFrame->UpdateWindow();
	if ( pMainFrame->params.bFullScreen )
	{
		pMainFrame->ShowWindow( SW_MAXIMIZE );
	}
	else
	{
		pMainFrame->ShowWindow( SW_SHOW );
	}
	pMainFrame->UpdateWindow();

	return true;
}

#include "MLParser.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CELKApp::OnAppAbout()
{
/**
	wstring wszText;
	File2String( &wszText, "c:\\b2\\editor\\unicode_test.txt", true );
	NML::CMLText mlText;
	const int nTextCount = NML::Parse( &mlText, wszText, true );
/**/
	CAboutDialog wndAboutDialog;
	wndAboutDialog.DoModal();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CELKApp::ExitInstance() 
{
	return CWinApp::ExitInstance();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CELKApp::OnHelpContents() 
{
	if ( m_pMainWnd != 0 )
  {
    CMainFrame *pFrame = static_cast<CMainFrame*>( m_pMainWnd );
		if ( NFile::DoesFileExist( pFrame->params.szHelpFilePath.c_str() ) )
		{
			pFrame->RunExternalHelpFile( pFrame->params.szHelpFilePath );
		}
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
