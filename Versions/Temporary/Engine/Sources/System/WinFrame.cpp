#include "stdafx.h"
#include "WinFrame.h"
#include "Commands.h"

#include "port/virtualkey.h"

#include <cstdint>
#include <mutex>

#include <fmt/format.h>

#if BOOST_OS_WINDOWS
#include "Misc/Win32Helper.h"
#else
#include "SdlVideo.h"
#include "WinImageFormats.h"

#include "port/window.h"

#include <SDL3/SDL.h>

#include <fstream>
#include <iterator>
#include <vector>
#endif

using namespace NWinFrame;

static HWND hWnd = 0;                            // window handle
static volatile bool bExit = false;
static volatile bool bActive = true;
static std::mutex msgs;
static std::list< SWindowsMsg > msgList;
static NWinCursor::TCursor hCursor;
static bool bManageCursor = true;

#if BOOST_OS_WINDOWS
using namespace NWin32Helper;

static HWND hWndSplashScreen;
static HINSTANCE hInstance = 0;                  // instance handle
static ATOM atomWndClassName = 0;                // atom window class name identification (assigned during registration)

// Force disable app minimisation - alt + tab is still ok though
static bool s_bMinimizeOnDeactivate = false;
static bool bAppAlwaysActive = true;

struct SSaveCursorClip
{
	~SSaveCursorClip() { ClipCursor(0); }
} doSaveCursorClip;

static void Report( const char *pszText, int nVal = -0x7fffffff )
{
	std::string message;
	if ( nVal != -0x7fffffff )
		message = fmt::format( "{}{}", pszText, nVal );
	else
		message = pszText;
	DebugTrace( "%s", message.c_str() );
}
#endif

bool NWinFrame::GetMessage( SWindowsMsg *pRes )
{
	std::lock_guard lock( msgs );
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

bool NWinFrame::IsExit()
{
	return bExit;
}

void NWinFrame::ResetExit()
{
	bExit = false;
}

HWND NWinFrame::GetWnd()
{
	return hWnd;
}

static void AddMsg( SWindowsMsg::EMsg msg, int x, int y, uint32_t dwFlags )
{
	NHPTimer::STime time;
	NHPTimer::GetTime( &time );
	std::lock_guard lock( msgs );
	SWindowsMsg &m = msgList.emplace_back();
	m.time = time;
	m.msg = msg;
	m.x = x;
	m.y = y;
	m.dwFlags = dwFlags;
}

void NWinFrame::EnableCursorManagement( bool bEnable )
{
	bManageCursor = bEnable;
}

#if BOOST_OS_WINDOWS

static void SetActive( bool _bActive )
{
	return; // no mimising/sleep allowed!

 	if ( bAppAlwaysActive )
		return;

	bActive = _bActive;        // activation flag 
	if ( s_bMinimizeOnDeactivate )
	{
		if ( !bActive )
			ShowWindow( hWnd, SW_MINIMIZE );
	}
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

static LRESULT CALLBACK WndProc( HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam );
bool SFLB2_CreateWin( LPCSTR pszApp, LPCSTR pszWnd, unsigned dwWidth, unsigned dwHeight, LPCSTR nIcon )
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
  uint32_t dwWinStyle = WS_POPUP|WS_SYSMENU|WS_VISIBLE;//WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_VISIBLE;

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

static LRESULT CALLBACK WndProc( HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam )
{
	//Report( "WndProc_", uMsg );
	//
	bool bCallDefWindowProc = false;
	NGlobal::CValue always_active = NGlobal::GetVar("force_app_always_active");
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
			if (always_active.GetFloat() != 0)
			{
				SetActive(true);
				break;
			}
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
						if (always_active.GetFloat() != 0)
							break;
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

bool NWinFrame::SFLB1_InitApplication( const char *pszAppName, const char *pszWndName, LPCSTR nIcon )
{
	int nXSize = 10000;
	int nYSize = 10000;
	// The module handle of the executable, which is what WinMain was handed back
	// when it was the only entry point this game had.
	::hInstance = GetModuleHandle( 0 );
	if ( !SFLB2_CreateWin( pszAppName, pszWndName, nXSize, nYSize, nIcon ) )
		return false;
	return true;
}

void NWinFrame::SetIcon( const std::string & )
{
	// nothing to do: SFLB2_CreateWin took the icon from the resource section
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

void NWinFrame::FlashTaskbarIfInactive()
{
	HWND hwnd = NWinFrame::GetWnd();
	if (!IsWindowVisible(hwnd) || GetForegroundWindow() != hwnd)
	{
		FLASHWINFO fi = { 0 };
		fi.cbSize = sizeof(FLASHWINFO);
		fi.hwnd = hwnd;
		fi.dwFlags = FLASHW_TRAY;

		FlashWindowEx(&fi);
	}
}

#else
namespace
{
//! The SDL window behind the handle, which is what an HWND holds off Windows.
SDL_Window *Window()
{
	return AsSdlWindow( hWnd );
}

void AddMouseButton( const SDL_MouseButtonEvent &rEvent )
{
	// SDL counts the clicks rather than sending a separate double-click message,
	// which is what CS_DBLCLKS made Windows do.
	const bool bDouble = rEvent.clicks == 2;
	SWindowsMsg::EMsg msg;
	if ( rEvent.button == SDL_BUTTON_LEFT )
	{
		msg = rEvent.down ? ( bDouble ? SWindowsMsg::LB_DBLCLK : SWindowsMsg::LB_DOWN ) : SWindowsMsg::LB_UP;
	}
	else if ( rEvent.button == SDL_BUTTON_RIGHT )
	{
		msg = rEvent.down ? ( bDouble ? SWindowsMsg::RB_DBLCLK : SWindowsMsg::RB_DOWN ) : SWindowsMsg::RB_UP;
	}
	else
	{
		// The middle and extra buttons have no window message here either: the
		// game reads them through the bindings, which go at the mouse directly.
		return;
	}
	AddMsg( msg, static_cast<int>( rEvent.x ), static_cast<int>( rEvent.y ), 0 );
}

void AddText( const char *pszText )
{
	// One event, several characters. Windows sends a WM_CHAR per character and
	// the conversion downstream is one to one; SDL hands over a UTF-8 string,
	// which a dead key or an input method can make longer than the keystroke
	// that produced it. Each code point becomes its own CHAR message, so what
	// reaches the UI is shaped the way it has always been.
	size_t nLeft = SDL_strlen( pszText );
	while ( nLeft > 0 )
	{
		const Uint32 nCodePoint = SDL_StepUTF8( &pszText, &nLeft );
		if ( nCodePoint == 0 || nCodePoint == SDL_INVALID_UNICODE_CODEPOINT )
		{
			continue;
		}
		AddMsg( SWindowsMsg::CHAR, static_cast<int>( nCodePoint ), 1, 0 );
	}
}
}

void NWinFrame::Exit()
{
	bExit = true;
}

bool NWinFrame::SFLB1_InitApplication( const char *pszAppName, const char *, LPCSTR )
{
	if ( !NSdl::EnsureVideo() )
	{
		return false;
	}
	// Sized to the desktop rather than to the 10000 by 10000 the Windows path
	// asks for. The size is provisional either way, since Gfx resizes the window
	// to the back buffer on the first SetMode, and a window larger than every
	// monitor is a Win32 way of saying borderless that SDL says directly.
	int nWidth = 1024, nHeight = 768;
	const SDL_DisplayMode *pDesktop = SDL_GetDesktopDisplayMode( SDL_GetPrimaryDisplay() );
	if ( pDesktop != 0 )
	{
		nWidth = pDesktop->w;
		nHeight = pDesktop->h;
	}
	// SDL_WINDOW_VULKAN is not optional here even though nothing in this file
	// touches Vulkan. D3D9 is DXVK off Windows, and its SDL3 backend presents by
	// calling SDL_Vulkan_CreateSurface on this window; SDL rejects that outright
	// for a window created without the flag, and DXVK reports the refusal as
	// VK_ERROR_OUT_OF_HOST_MEMORY, so CreateDevice fails with a message that
	// points at memory rather than at the window.
	SDL_Window *pWindow = SDL_CreateWindow( pszAppName != 0 ? pszAppName : "", nWidth, nHeight,
	                                        SDL_WINDOW_BORDERLESS | SDL_WINDOW_VULKAN );
	if ( pWindow == 0 )
	{
		csSystem << CC_RED << "Cannot create the game window: " << SDL_GetError() << endl;
		return false;
	}
	hWnd = pWindow;
	// text arrives only while this is on, and the console and the edit line want it
	SDL_StartTextInput( pWindow );
	return true;
}

void NWinFrame::SetIcon( const std::string &szFileName )
{
	if ( hWnd == 0 )
	{
		return;
	}
	// a filesystem path, not a VFS one, the same as the splash image: this file
	// is installed beside the executable's data rather than packed
	std::ifstream file( szFileName.c_str(), std::ios::binary );
	const std::vector<uint8_t> buffer( ( std::istreambuf_iterator<char>( file ) ),
	                                   std::istreambuf_iterator<char>() );
	std::vector<NWinImage::SImageInfo> infos;
	if ( buffer.empty() || !NWinImage::GetImages( &infos, &buffer[0], buffer.size(), 0 ) )
	{
		csSystem << CC_RED << "Cannot read the window icon " << szFileName.c_str() << endl;
		return;
	}
	// Every size the file carries is handed over, not just one. SDL keeps them
	// as alternates and lets the desktop take whichever fits the slot it is
	// filling, which is the reason the container holds more than one to begin
	// with: a task bar button and an alt-tab panel are not the same size.
	//
	// The largest is the one passed to SDL_SetWindowIcon, because that is the
	// one SDL falls back to scaling when no alternate matches, and scaling down
	// keeps more than scaling up.
	std::vector<NWinImage::SImage> images;
	std::vector<SDL_Surface *> surfaces;
	for ( int i = 0; i < infos.size(); ++i )
	{
		NWinImage::SImage image;
		if ( !NWinImage::DecodeImage( &image, &buffer[0], buffer.size(), 0, i ) )
		{
			continue;
		}
		images.push_back( image );
	}
	// the pixels have to outlive every surface, so no surface is made until the
	// vector holding them has stopped growing and moving them about
	for ( int i = 0; i < images.size(); ++i )
	{
		SDL_Surface *pSurface = SDL_CreateSurfaceFrom( images[i].nWidth, images[i].nHeight,
			SDL_PIXELFORMAT_ARGB8888, &images[i].pixels[0],
			images[i].nWidth * static_cast<int>( sizeof( uint32_t ) ) );
		if ( pSurface != 0 )
		{
			surfaces.push_back( pSurface );
		}
	}
	if ( surfaces.empty() )
	{
		csSystem << CC_RED << "No usable image in the window icon " << szFileName.c_str() << endl;
		return;
	}
	int nLargest = 0;
	for ( int i = 1; i < surfaces.size(); ++i )
	{
		if ( surfaces[i]->w > surfaces[nLargest]->w )
		{
			nLargest = i;
		}
	}
	for ( int i = 0; i < surfaces.size(); ++i )
	{
		if ( i != nLargest )
		{
			SDL_AddSurfaceAlternateImage( surfaces[nLargest], surfaces[i] );
		}
	}
	// On X11 this becomes _NET_WM_ICON and every window manager reads it. On
	// Wayland it needs xdg_toplevel_icon_v1, which not every compositor has;
	// where it is missing SDL says so and the icon comes from the desktop file
	// matched to the app id NSdl::InitVideo sets instead.
	if ( !SDL_SetWindowIcon( AsSdlWindow( hWnd ), surfaces[nLargest] ) )
	{
		csSystem << CC_RED << "Cannot set the window icon: " << SDL_GetError() << endl;
	}
	// SDL_SetWindowIcon copies what it is given, so all of this can go now
	for ( int i = 0; i < surfaces.size(); ++i )
	{
		SDL_DestroySurface( surfaces[i] );
	}
}

void NWinFrame::PumpMessages()
{
	SDL_Event event;
	while ( SDL_PollEvent( &event ) )
	{
		switch ( event.type )
		{
		case SDL_EVENT_QUIT:
			bExit = true;
			break;

		case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
			AddMsg( SWindowsMsg::CLOSE, 0, 0, 0 );
			break;

		case SDL_EVENT_MOUSE_MOTION:
			AddMsg( SWindowsMsg::MOUSE_MOVE, static_cast<int>( event.motion.x ),
			        static_cast<int>( event.motion.y ), 0 );
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
			AddMouseButton( event.button );
			break;

		case SDL_EVENT_MOUSE_WHEEL:
			// Posted for the same reason the Windows path posts it, and read by
			// as much: ConvertMessage has no case for MOUSE_WHEEL, so it is
			// dropped. The wheel the game acts on is the bindings' MOUSE_AXIS_Z.
			AddMsg( SWindowsMsg::MOUSE_WHEEL, static_cast<int>( event.wheel.mouse_x ),
			        static_cast<int>( event.wheel.mouse_y ), 0 );
			break;

		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
		{
			// From the scancode. port/virtualkey.h says why not from the keycode.
			const int nKey = SdlScancodeToVirtualKey( event.key.scancode );
			if ( nKey != 0 )
			{
				// Windows folds auto-repeat into a count that the readers loop
				// over; SDL sends one event per repeat, so the count is one.
				AddMsg( event.key.down ? SWindowsMsg::KEY_DOWN : SWindowsMsg::KEY_UP, nKey, 1,
				        event.key.scancode );
			}
			break;
		}

		case SDL_EVENT_TEXT_INPUT:
			AddText( event.text.text );
			break;

		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
			// This has no counterpart on Windows, where a window is the size it
			// was told to be. A compositor may answer a size request with a
			// different size, leaving the back buffer no longer matching the
			// window. Gfx::CheckBackBufferSize already watches for exactly that,
			// so all this has to do is not treat it as impossible the way
			// WM_ENTERSIZEMOVE does.
			break;

		default:
			break;
		}
	}
}

void NWinFrame::SetCursor( NWinCursor::TCursor _hCursor )
{
	if ( !bManageCursor )
	{
		return;
	}
	if ( hCursor == _hCursor )
	{
		return;
	}
	hCursor = _hCursor;
	if ( hCursor != 0 )
	{
		SDL_SetCursor( hCursor );
	}
}

void NWinFrame::ShowCursor( bool bShow )
{
	if ( !bManageCursor )
	{
		return;
	}
	static bool bIsShown = (bool)3;
	if ( bIsShown == bShow )
	{
		return;
	}
	bIsShown = bShow;
	if ( bShow )
	{
		SDL_ShowCursor();
		if ( hCursor != 0 )
		{
			SDL_SetCursor( hCursor );
		}
	}
	else
	{
		SDL_HideCursor();
	}
}

void NWinFrame::FlashTaskbarIfInactive()
{
	SDL_Window *pWindow = Window();
	if ( pWindow == 0 )
	{
		return;
	}
	if ( ( SDL_GetWindowFlags( pWindow ) & SDL_WINDOW_INPUT_FOCUS ) == 0 )
	{
		SDL_FlashWindow( pWindow, SDL_FLASH_BRIEFLY );
	}
}
#endif

// no sleep or mimizing!!!
// START_REGISTER(WinFrame)
// REGISTER_VAR_EX( "minimize_on_deactivate", NGlobal::VarBoolHandler, &s_bMinimizeOnDeactivate, true, STORAGE_NONE );
// REGISTER_VAR_EX( "app_always_active", NGlobal::VarBoolHandler, &bAppAlwaysActive, false, STORAGE_USER );
// FINISH_REGISTER
