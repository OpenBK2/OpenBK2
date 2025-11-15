#include "stdafx.h"
#include <crtdbg.h>

#include "console.h"
#include "CustomLobby.h"
#include "LadderLobby.h"
#include "ServerApp.h"
#include "Server.h"

#include "System/FileUtils.h"
#include "Misc/StrProc.h"
#include "Scintilla/Platform.h"
#include "Scintilla/Scintilla.h"
#include "Server_Client_Common/Commands.h"

CServerApp theApp;

CServerApp::CServerApp()
{
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	const int nBreakId = -1;
	_CrtSetBreakAlloc( nBreakId );
}


void CALLBACK TimerProc( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime )
{
	theApp.Segment();
}

const string GetBaseDir()
{
	char buffer[1024];
	GetCurrentDirectory( 1024, buffer );
	//
	string szTemp = buffer;
	NI_ASSERT( !szTemp.empty(), "Can't get current directory" );
	if ( szTemp[szTemp.size() - 1] != '\\' )
		szTemp += '\\';
	szTemp += "..\\";

	string szBaseDir;
	NFile::GetFullName( &szBaseDir, szTemp );
	NStr::ToLower( &szBaseDir );

	return szBaseDir;
}

BOOL CServerApp::InitInstance()
{
	CWinApp::InitInstance();

	::CoInitialize( 0 );
	HINSTANCE hInstance = ::AfxGetResourceHandle();
	Scintilla_RegisterClasses( hInstance );

	const string szBaseDir = GetBaseDir();

	pCmds = new CCommands( true );
	pConsole = new CConsole( pCmds, "Server" );
	pGameServer = new CGameServer( pCmds, szBaseDir + "server.xml" );

	string szCfgFile = szBaseDir + "server.xml";

	pGameServer->AddLobby( new CCustomLobby( pGameServer->GetClients(), szCfgFile ) );
	pGameServer->AddLobby( new CLadderLobby( pGameServer->GetClients(), szCfgFile ) );

	m_pMainWnd = pConsole;
	SetTimer( 0, 1, 50, &TimerProc );

	return true;
}

int CServerApp::ExitInstance() 
{
	Scintilla_ReleaseResources();
	::CoUninitialize();

  delete pConsole;
	pGameServer = 0;
	//
	return CWinApp::ExitInstance();
}

void CServerApp::Segment()
{
	Sleep( 20 );
	pGameServer->Segment();
	//pConsole->Segment();
}


