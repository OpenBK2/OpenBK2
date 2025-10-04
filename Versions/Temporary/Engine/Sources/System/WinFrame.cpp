#include "stdafx.h"
#include "WinFrame.h"
#include "Misc/Win32Helper.h"
#include "Commands.h"

using namespace NWinFrame;
using namespace NWin32Helper;

static HWND hWnd = 0;                            // window handle
static HWND hWndSplashScreen;
static HINSTANCE hInstance = 0;                  // instance handle
static ATOM atomWndClassName = 0;                // atom window class name identification (assigned during registration)
static volatile bool bExit = false;
static volatile bool bActive = true;
static CCriticalSection msgs;
static list< SWindowsMsg > msgList;
static HCURSOR hCursor;
static bool bManageCursor = true;
static bool s_bMinimizeOnDeactivate = true;
static bool bAppAlwaysActive = false;

struct SSaveCursorClip
{
	~SSaveCursorClip() { ClipCursor(0); }
} doSaveCursorClip;

static void Report( const char *pszText, int nVal = -0x7fffffff )
{
	const char *pszMsg;
	if ( nVal != -0x7fffffff )
		pszMsg = StrFmt( "%s%d\n", pszText, nVal );
	else
		pszMsg = StrFmt( "%s\n", pszText );
	OutputDebugString( pszMsg );
}

bool NWinFrame::GetMessage( SWindowsMsg *pRes )
{
	CCriticalSectionLock lock( msgs );
	if ( !msgList.empty() )
	{
		*pRes = msgList.front();
		msgList.pop_front();
		return true;
	}
	pRes->msg = SWindowsMsg::TIME;
	NHPTimer::GetTime( &pRes->time );
	return false;
}

bool NWinFrame::IsAppActive()
{
	return bActive;
}

static void SetActive( bool _bActive )
{
 	if ( bAppAlwaysActive )
		return;

	bActive = _bActive;        // activation flag 
	if ( s_bMinimizeOnDeactivate )
	{
		if ( !bActive )
			ShowWindow( hWnd, SW_MINIMIZE );
	}
}

bool NWinFrame::IsExit()
{
	return bExit;
}

void NWinFrame::ResetExit()
{
	bExit = false;
}

void NWinFrame::PumpMessages()
{
  // Now we are ready to recieve and process Windows messages.
  MSG msg;
	while ( PeekMessage( &msg, 0, 0, 0, PM_NOREMOVE ) )
	{
		if ( ::GetMessage( &msg, 0, 0, 0 ) )
		{
			if ( msg.message == WM_ACTIVATEAPP )
			{
				SetActive( msg.wParam != 0 );
				//Report( "MainMsgProcess::WM_activateapp ", msg.wParam );
			}
			TranslateMessage( &msg );
			DispatchMessage( &msg );
			//Report( "...finish msg process", msg.message );
		}
		else
			bExit = true;
	}
}

HWND NWinFrame::GetWnd()
{
	return hWnd;
}

namespace NWinFrame
{
void SetEditorWnd( HWND _hWnd )
{
	hWnd = _hWnd;
}
}

void NWinFrame::Exit()
{
	PostQuitMessage(0);
	//bClientExitReq = true;
}

static void AddMsg( SWindowsMsg::EMsg msg, int x, int y, DWORD dwFlags )
{
	NHPTimer::STime time;
	NHPTimer::GetTime( &time );
	CCriticalSectionLock lock( msgs );
	SWindowsMsg &m = *msgList.insert( msgList.end() );
	m.time = time;
	m.msg = msg;
	m.x = x;
	m.y = y;
	m.dwFlags = dwFlags;
}

static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
bool __declspec(dllexport) SFLB2_CreateWin( LPCSTR pszApp, LPCSTR pszWnd, unsigned dwWidth, unsigned dwHeight, LPCSTR nIcon )
{
  // create and register class style
        // Register the windows class
  WNDCLASS wndClass = { 0, WndProc, 0, 0, hInstance,
                        LoadIcon( hInstance, nIcon ),
                        0,//LoadCursor( NULL, IDC_ARROW ), 
                        (HBRUSH)GetStockObject(NULL_BRUSH), // NULL_BRUSH // WHITE_BRUSH
                        NULL, pszWnd };
	wndClass.style |= CS_DBLCLKS;
 atomWndClassName = RegisterClass( &wndClass );

  // Set the window's initial style
  DWORD dwWinStyle = WS_POPUP|WS_SYSMENU|WS_VISIBLE;//WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_VISIBLE;

  // Create the render window
  hWnd = CreateWindow( pszWnd, pszApp, dwWinStyle,
                         0, 0, dwWidth, dwHeight, 0L,
                         0,//LoadMenu( hInstance, MAKEINTRESOURCE(IDR_MENU) ), 
                         hInstance, 0L );

  if ( !hWnd )
	{
		//ThrowException( "Can't create main app window\n" );
		return false;
	}
  // show & update window
  ShowWindow( hWnd, SW_SHOW );
  UpdateWindow( hWnd );
  // eliminate cursor once for this widow
	::SetCursor( hCursor );

  return true;
}

static void SetClipCursorRect( HWND _hWnd )
{
	RECT r;
	if ( !GetWindowRect( _hWnd, &r ) )
		return;
	if ( r.left < -100 )
		ClipCursor(0);
	else
	{
		ClipCursor( &r );
		POINT p;
		GetCursorPos( &p );
		SetCursorPos( p.x, p.y );
	}
}

// did not know how to return NCHitTest
static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	//Report( "WndProc_", uMsg );
	//
	bool bCallDefWindowProc = false;
	switch ( uMsg )
	{
		case WM_PAINT:
			/*RECT rect;
			if ( GetUpdateRect(hWnd, &rect, FALSE) )
				ValidateRect( hWnd, &rect );*/
			break;
    case WM_GETMINMAXINFO:
      ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 100;
      ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 100;
      break;
    case WM_ENTERSIZEMOVE:
    // Halt frame movement while the app is sizing or moving
			ASSERT(0);
      break;
    case WM_EXITSIZEMOVE:
      break;
		case WM_SETCURSOR:
			::SetCursor( hCursor );
			break;
    case WM_NCHITTEST:
      // Prevent the user from selecting the menu in fullscreen mode
      //if( !m_bWindowed )
      return HTCLIENT;
      break;
    case WM_POWERBROADCAST:
      switch( wParam )
      {
        case PBT_APMQUERYSUSPEND:
          // At this point, the app should save any data for open
          // network connections, files, etc., and prepare to go into
          // a suspended mode.
          return TRUE;

        case PBT_APMRESUMESUSPEND:
          // At this point, the app should recover any data, network
          // connections, files, etc., and resume running from when
          // the app was suspended.
          return TRUE;
      }
      break;
    case WM_SYSCOMMAND:
      // Prevent moving/sizing and power loss in fullscreen mode
      switch( wParam )
      {
        case SC_MOVE:
        case SC_SIZE:
        case SC_MAXIMIZE:
        case SC_KEYMENU:
        case SC_MONITORPOWER:
				case SC_SCREENSAVE:
          //if( FALSE == m_bWindowed )
          return 1; // in both modes is prevented
          break;
/*				case SC_RESTORE:
					ShowWindow( hWnd, SW_RESTORE );
					break;*/
      }
      break;
/*
    case WM_CLOSE:
			NInput::PostEvent( "try_exit_windows", 0, 0 );
      //PostQuitMessage(0);
      return 0;
*/
		case WM_ACTIVATEAPP:
			//Report( "WndProc::WM_activateapp ", wParam );
			SetActive( wParam != 0 );
			break;
		case WM_SIZE:
		{
			WINDOWINFO info;
			memset( &info, 0, sizeof( WINDOWINFO ) );
			info.cbSize = sizeof( WINDOWINFO );
			if ( hWnd && GetWindowInfo( hWnd, &info ) && info.dwWindowStatus == WS_ACTIVECAPTION )
				SetClipCursorRect( hWnd );
			break;
		}
		case WM_ACTIVATE:
			//if ( !(HIWORD(wParam)) )          // if window is not minimized
			{
				switch ( LOWORD(wParam) )
				{
					case WA_CLICKACTIVE:					// activate window
					case WA_ACTIVE:
						if ( (HIWORD(wParam)) == 0 )
						{
							if ( hWnd )
								SetClipCursorRect( hWnd );
						}
						break;
					case WA_INACTIVE:						// deactivate window
						SetActive( false );
						ClipCursor(0);
						//Report( "WndProc::WM_activate, WA_INACTIVE ", wParam );
						break;
				}
			}
			break;

		case WM_MOUSEMOVE:
			AddMsg( SWindowsMsg::MOUSE_MOVE, lParam & 0xFFFF, (lParam >> 16) & 0xFFFF, wParam );
			break;
		case WM_RBUTTONDOWN:
			AddMsg( SWindowsMsg::RB_DOWN, lParam & 0xFFFF, (lParam >> 16) & 0xFFFF, wParam );
			break;
		case WM_RBUTTONUP:
			AddMsg( SWindowsMsg::RB_UP, lParam & 0xFFFF, (lParam >> 16) & 0xFFFF, wParam );
			break;
		case WM_RBUTTONDBLCLK:
			AddMsg( SWindowsMsg::RB_DBLCLK, lParam & 0xFFFF, (lParam >> 16) & 0xFFFF, wParam );
			break;
		case WM_LBUTTONDOWN:
			AddMsg( SWindowsMsg::LB_DOWN, lParam & 0xFFFF, (lParam >> 16) & 0xFFFF, wParam );
			break;
		case WM_LBUTTONUP:
			AddMsg( SWindowsMsg::LB_UP, lParam & 0xFFFF, (lParam >> 16) & 0xFFFF, wParam );
			break;
		case WM_LBUTTONDBLCLK:
			AddMsg( SWindowsMsg::LB_DBLCLK, lParam & 0xFFFF, (lParam >> 16) & 0xFFFF, wParam );
			break;
		case WM_MOUSEWHEEL:
			AddMsg( SWindowsMsg::MOUSE_WHEEL, lParam & 0xFFFF, (lParam >> 16) & 0xFFFF, wParam );
			break;
		case WM_KEYDOWN:
			AddMsg( SWindowsMsg::KEY_DOWN, wParam, lParam & 0xFFFF, (lParam >> 16) & 0xFFFF );
			break;
		case WM_KEYUP:
			AddMsg( SWindowsMsg::KEY_UP, wParam, lParam & 0xFFFF, (lParam >> 16) & 0xFFFF );
			break;
		case WM_CHAR:
			AddMsg( SWindowsMsg::CHAR, wParam, lParam & 0xFFFF, (lParam >> 16) & 0xFFFF );
			break;
		case WM_CLOSE:
			AddMsg( SWindowsMsg::CLOSE, wParam, lParam & 0xFFFF, (lParam >> 16) & 0xFFFF );
			return 0;

	}
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

bool __declspec(dllexport) NWinFrame::SFLB1_InitApplication( HINSTANCE hInstance, const char *pszAppName, const char *pszWndName, LPCSTR nIcon )
{
	int nXSize = 10000;
	int nYSize = 10000;
	::hInstance = hInstance;
	if ( !SFLB2_CreateWin( pszAppName, pszWndName, nXSize, nYSize, nIcon ) )
		return false;
	return true;
}

void NWinFrame::SetCursor( HCURSOR _hCursor )
{
	if ( !bManageCursor )
		return;

	if ( hCursor == _hCursor )
		return;
	hCursor = _hCursor;
	::SetCursor( hCursor );
}

void NWinFrame::ShowCursor( bool bShow )
{
	if ( !bManageCursor )
		return;
	static bool bIsShown = (bool)3;
	if ( bIsShown == bShow )
		return;
	bIsShown = bShow;
	if ( bShow )
	{
		::ShowCursor( TRUE );
		::SetCursor( hCursor );
	}
	else
		::ShowCursor( FALSE );
}

void NWinFrame::EnableCursorManagement( bool bEnable )
{
	bManageCursor = bEnable;
}

START_REGISTER(WinFrame)
REGISTER_VAR_EX( "minimize_on_deactivate", NGlobal::VarBoolHandler, &s_bMinimizeOnDeactivate, true, STORAGE_NONE );
REGISTER_VAR_EX( "app_always_active", NGlobal::VarBoolHandler, &bAppAlwaysActive, false, STORAGE_USER );
FINISH_REGISTER


