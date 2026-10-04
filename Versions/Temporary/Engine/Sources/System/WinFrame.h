#pragma once

#include "System_export.h"

// HWND, HINSTANCE and LPCSTR below. On Windows every stdafx.h has already
// pulled this in before anything reaches here; off Windows it is DXVK Native.
#include <windows.h>

#include "Misc/HPTimer.h"
#include "WinCursor.h"

#include <cstdint>
#include <string>

//
namespace NWinFrame
{
	struct SWindowsMsg
	{
		enum EMsg
		{
			MOUSE_WHEEL,
			MOUSE_MOVE,
			RB_DOWN,
			LB_DOWN,
			RB_UP,
			LB_UP,
			RB_DBLCLK,
			LB_DBLCLK,
			KEY_DOWN,
			KEY_UP,
			CHAR,
			TIME,
			CLOSE,
		};
		NHPTimer::STime time = 0;
		EMsg msg{};
		union
		{
			struct { int x,y; };         // mouse
			struct { int nKey, nRep; };  // key
		};
		uint32_t dwFlags = 0;

		// Initialize one anonymous-struct union arm; the key fields alias this storage.
		SWindowsMsg() : x( 0 ), y( 0 ) {}
	};
	// WinFrame interface
	SYSTEM_EXPORT bool (GetMessage)( SWindowsMsg *pRes );
	SYSTEM_EXPORT bool IsAppActive();
	SYSTEM_EXPORT bool IsExit();
	void Exit();
	SYSTEM_EXPORT void ResetExit(); // b2`s cheat to show movie on exit
	SYSTEM_EXPORT HWND GetWnd();
	// Only the standalone game owns its window; editor viewports keep their layout.
	SYSTEM_EXPORT bool IsGameWindow( HWND hWindow );
	SYSTEM_EXPORT void FitGameWindowToMonitor();
	SYSTEM_EXPORT void SetRenderSize( int width, int height );
	SYSTEM_EXPORT void SetMouseBounds( int left, int top, int right, int bottom, bool bAcquire );
	SYSTEM_EXPORT void GetMousePos( float *px, float *py );
	SYSTEM_EXPORT void SetMousePos( float x, float y );
	SYSTEM_EXPORT void PumpMessages();
	//! Create the game window.
	//!
	//! Took an HINSTANCE until the entry point stopped being WinMain everywhere.
	//! Off Windows there is no such thing, and on Windows the only value ever
	//! passed was WinMain's own, which is what GetModuleHandle(0) returns, so the
	//! Windows implementation asks for it rather than being handed it.
	SYSTEM_EXPORT bool SFLB1_InitApplication( const char *pszAppName, const char *pszWndName, LPCSTR nIcon );
	//! Give the window an icon, read from a .ico on the filesystem.
	//!
	//! Does nothing on Windows: SFLB1_InitApplication already put the icon out
	//! of the executable's resource section into the window class, which is also
	//! what Explorer shows for the file itself. ELF has no resource section, so
	//! off Windows the same picture arrives as a file.
	SYSTEM_EXPORT void SetIcon( const std::string &szFileName );
	SYSTEM_EXPORT void SetCursor( NWinCursor::TFrame _hCursor );
	void ShowCursor( bool bShow );
	void EnableCursorManagement( bool bEnable );
	SYSTEM_EXPORT void FlashTaskbarIfInactive();
};


