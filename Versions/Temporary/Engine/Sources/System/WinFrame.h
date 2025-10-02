#ifndef __WINFRAME_H__
#define __WINFRAME_H__
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "System_export.h"


#include "..\misc\HPTimer.h"
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
		DWORD dwFlags;
	};
	// WinFrame interface
	SYSTEM_EXPORT bool (GetMessage)( SWindowsMsg *pRes );
	SYSTEM_EXPORT bool IsAppActive();
	SYSTEM_EXPORT bool IsExit();
	void Exit();
	SYSTEM_EXPORT void ResetExit(); // b2`s cheat to show movie on exit
	SYSTEM_EXPORT HWND GetWnd();
	SYSTEM_EXPORT void PumpMessages();
	bool __declspec(dllexport) SFLB1_InitApplication( HINSTANCE hInstance, const char *pszAppName, const char *pszWndName, LPCSTR nIcon );
	SYSTEM_EXPORT void SetCursor( HCURSOR _hCursor );
	void ShowCursor( bool bShow );
	void EnableCursorManagement( bool bEnable );
};

#endif

