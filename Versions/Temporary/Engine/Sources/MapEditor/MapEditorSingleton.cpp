#include "stdafx.h"
#include "MapEditorSingleton.h"

#include <cstdint>

#include <cstring>

string CMapEditorSingletonBase::MAP_FILE_NAME = "CMapEditorSingletonBase_B2MapEditor_1.0";
const uint32_t CMapEditorSingletonBase::MAP_FILE_MAX_SIZE = 0xFFF;

void CMapEditorSingletonBase::SetMapFileName( const string &szMapFileName )
{
	MAP_FILE_NAME = szMapFileName;
}

bool CMapEditorSingletonBase::SendCommand( HWND hWndDst, HWND hWndSrc, uint32_t dwCommand, uint32_t dwDataLength, const void* pData ) const
{
  if ( hWndDst == 0 )
	{
		return false;
	}

  COPYDATASTRUCT cd;
  cd.dwData = dwCommand;
  cd.cbData = dwDataLength;
  cd.lpData = const_cast<void*>( pData );

  return SendMessage( hWndDst, WM_COPYDATA, reinterpret_cast<WPARAM>( hWndSrc ), reinterpret_cast<LPARAM>( &cd ) );
}


CMapEditorSingletonApp::CMapEditorSingletonApp()
	: hMapFileHandle( 0 ), pMapFileData( 0 ) {}


CMapEditorSingletonApp::~CMapEditorSingletonApp()
{
  RemoveMapFile();
}


void CMapEditorSingletonApp::RemoveMapFile()
{
  if ( pMapFileData != 0 )
	{
		::UnmapViewOfFile( pMapFileData );
		pMapFileData = 0;
	}
  if ( hMapFileHandle != 0 ) 
	{
		::CloseHandle( hMapFileHandle );
		hMapFileHandle = 0;
	}
}


bool CMapEditorSingletonApp::CreateMapFile( HWND hWndApp )
{
//	return true;
  if ( hWndApp == 0 )
	{
		return false;
	}
	RemoveMapFile();
	hMapFileHandle = ::CreateFileMapping( static_cast<HANDLE>( INVALID_HANDLE_VALUE ),
																				0,
																				PAGE_READWRITE,
																				0,
																				MAP_FILE_MAX_SIZE,
																				MAP_FILE_NAME.c_str() );
	if ( hMapFileHandle == 0 )
	{
		return false;
	}
  pMapFileData = ::MapViewOfFile( hMapFileHandle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0 );
  if ( pMapFileData == 0 )
  {
    RemoveMapFile();
    return false;
  }

  memset( pMapFileData, 0, MAP_FILE_MAX_SIZE );
  *( static_cast<HWND*>( pMapFileData ) ) = hWndApp;
  
	return true;
}



HWND CMapEditorSingletonChecker::GetAppHwnd() const
{
//	return 0;
  HWND hWndApp = 0;

  HANDLE hFileHandle = ::OpenFileMapping( FILE_MAP_READ, 0, MAP_FILE_NAME.c_str() );
  if ( hFileHandle == 0 )
	{
		return 0;
	}
  void *pFileData = ::MapViewOfFile( hFileHandle, FILE_MAP_READ, 0, 0, 0 );
  if ( pFileData != 0 )
	{
		hWndApp = *( static_cast<HWND*>( pFileData ) );
		::UnmapViewOfFile( pFileData );
		if ( !::IsWindow( hWndApp ) )
		{
			hWndApp = 0;
		}
	}
  ::CloseHandle( hFileHandle );
	
	return hWndApp;
}


bool CMapEditorSingletonChecker::BringAppOnTop() const
{
  const HWND hWndApp = GetAppHwnd();
	if ( hWndApp != 0 )
	{
		WINDOWPLACEMENT windowPlacement;
		::GetWindowPlacement( hWndApp, &windowPlacement );
		if ( windowPlacement.showCmd == SW_SHOWMINIMIZED )
		{
			::ShowWindow( hWndApp, SW_SHOW );
		}
		::SetForegroundWindow( hWndApp );
		return true;
	}
	else
	{
		return false;
	}
}


bool CMapEditorSingletonChecker::OpenFileOnApp( const string &rszFilePath ) const
{
  const HWND hWndApp = GetAppHwnd();
	if ( hWndApp != 0 )
	{
		WINDOWPLACEMENT windowPlacement;
		::GetWindowPlacement( hWndApp, &windowPlacement );
		if ( windowPlacement.showCmd == SW_SHOWMINIMIZED )
		{
			::ShowWindow( hWndApp, SW_SHOW );
		}
		::SetForegroundWindow( hWndApp );
		SendCommand( hWndApp, 0, OPEN_FILE, rszFilePath.size() + 1, rszFilePath.c_str() );
		return true;
	}
	else
	{
		return false;
	}
}

// basement storage  


