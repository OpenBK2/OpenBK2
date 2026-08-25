#pragma once

#include <boost/predef.h>

// The few Win32 window calls the renderer makes, over an SDL window.
//
// The window is co-owned. System/WinFrame creates it, and 3Dmotor/Gfx.cpp
// resizes it to the back buffer after every device reset and reads its client
// size back to detect rescaling. Rather than rewrite those call sites, this
// carries them the way port/socket.h carries Net's.
//
// An HWND here holds an SDL_Window*, but DXVK types it as HANDLE, which is
// void*, so the conversion has to be written out. That is the whole reason
// these are functions rather than macros: the cast belongs in one place, and
// nothing outside this header should be treating a window handle as a pointer
// to anything in particular.
//
// RECT, POINT and HWND come from DXVK's windows.h shim. What it has no reason to
// carry is the SetWindowPos vocabulary, so that is here.
#if BOOST_OS_WINDOWS
#include <windows.h>
#else
#include <windows.h>

#include <SDL3/SDL.h>
#endif

#if !BOOST_OS_WINDOWS

// Only the flags this tree passes are honoured; the rest are here so a reader
// can pass what they expect and get what Win32 would do.
#define SWP_NOSIZE 0x0001
#define SWP_NOMOVE 0x0002
#define SWP_NOZORDER 0x0004
#define SWP_NOACTIVATE 0x0010
#define SWP_SHOWWINDOW 0x0040
#define SWP_HIDEWINDOW 0x0080

// Z-order is the compositor's to decide, so these exist only to be passed and
// ignored, which is also what they amount to on a fullscreen window.
#define HWND_TOP ( (HWND)0 )
#define HWND_BOTTOM ( (HWND)1 )
#define HWND_TOPMOST ( (HWND)-1 )
#define HWND_NOTOPMOST ( (HWND)-2 )

//! The SDL window an HWND stands for. DXVK types HWND as HANDLE, so this is the
//! one place that knows what is actually behind it.
inline SDL_Window *AsSdlWindow( HWND hWnd )
{
	return static_cast<SDL_Window *>( hWnd );
}

//! The client area, as a rectangle whose origin is always zero. Reported in
//! pixels rather than in logical units, because the caller sizes a back buffer
//! with it and a scaled display would otherwise render at the wrong resolution.
inline BOOL GetClientRect( HWND hWnd, RECT *pRect )
{
	if ( hWnd == 0 || pRect == 0 )
	{
		return FALSE;
	}
	int nWidth = 0, nHeight = 0;
	if ( !SDL_GetWindowSizeInPixels( AsSdlWindow( hWnd ), &nWidth, &nHeight ) )
	{
		return FALSE;
	}
	pRect->left = 0;
	pRect->top = 0;
	pRect->right = nWidth;
	pRect->bottom = nHeight;
	return TRUE;
}

//! Whether the window is mapped. This is the WS_VISIBLE question that Win32
//! answers, not whether anything can actually be seen: a window this reports as
//! visible may still be occluded, on another workspace, or minimised.
inline BOOL IsWindowVisible( HWND hWnd )
{
	if ( hWnd == 0 )
	{
		return FALSE;
	}
	return ( SDL_GetWindowFlags( AsSdlWindow( hWnd ) ) & SDL_WINDOW_HIDDEN ) == 0 ? TRUE : FALSE;
}

//! Move, resize and show in one call, as Win32 spells it.
//!
//! The position is a request. A Wayland compositor gives clients no way to place
//! their own windows and some X11 managers place them regardless, so a window may
//! not end up where this asks. Size and visibility are honoured.
inline BOOL SetWindowPos( HWND hWnd, HWND hInsertAfter, int nX, int nY, int nCX, int nCY, unsigned nFlags )
{
	(void)hInsertAfter;
	if ( hWnd == 0 )
	{
		return FALSE;
	}
	if ( ( nFlags & SWP_NOSIZE ) == 0 && nCX > 0 && nCY > 0 )
	{
		SDL_SetWindowSize( AsSdlWindow( hWnd ), nCX, nCY );
	}
	if ( ( nFlags & SWP_NOMOVE ) == 0 )
	{
		SDL_SetWindowPosition( AsSdlWindow( hWnd ), nX, nY );
	}
	if ( ( nFlags & SWP_SHOWWINDOW ) != 0 )
	{
		SDL_ShowWindow( AsSdlWindow( hWnd ) );
	}
	else if ( ( nFlags & SWP_HIDEWINDOW ) != 0 )
	{
		SDL_HideWindow( AsSdlWindow( hWnd ) );
	}
	return TRUE;
}

#endif
