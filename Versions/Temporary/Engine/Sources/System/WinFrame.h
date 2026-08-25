#pragma once

#include "System_export.h"

// HWND, HINSTANCE and LPCSTR below. On Windows every stdafx.h has already
// pulled this in before anything reaches here; off Windows it is DXVK Native.
#include <windows.h>

#include "Misc/HPTimer.h"
#include "WinCursor.h"

#include <cstdint>

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
		NHPTimer::STime time;
		EMsg msg;
		union
		{
			struct { int x,y; };         // mouse
			struct { int nKey, nRep; };  // key
		};
		uint32_t dwFlags;
	};
	// WinFrame interface
	SYSTEM_EXPORT bool (GetMessage)( SWindowsMsg *pRes );
	SYSTEM_EXPORT bool IsAppActive();
	SYSTEM_EXPORT bool IsExit();
	void Exit();
	SYSTEM_EXPORT void ResetExit(); // b2`s cheat to show movie on exit
	SYSTEM_EXPORT HWND GetWnd();
	SYSTEM_EXPORT void PumpMessages();
	SYSTEM_EXPORT bool SFLB1_InitApplication( HINSTANCE hInstance, const char *pszAppName, const char *pszWndName, LPCSTR nIcon );
	SYSTEM_EXPORT void SetCursor( NWinCursor::TCursor _hCursor );
	void ShowCursor( bool bShow );
	void EnableCursorManagement( bool bEnable );
	SYSTEM_EXPORT void FlashTaskbarIfInactive();
};


