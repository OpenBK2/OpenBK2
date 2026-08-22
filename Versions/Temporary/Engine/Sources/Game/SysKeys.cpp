#include "stdafx.h"

#include "SysKeys.h"

#include <cstdint>

//#define LLKHF_EXTENDED       0x00000001
#define LLKHF_INJECTED       0x00000010
//#define LLKHF_ALTDOWN        0x00000020
//#define LLKHF_UP             0x00000080

#define LLMHF_INJECTED       0x00000001

#define WH_KEYBOARD_LL     13
#define WH_MOUSE_LL        14

namespace NSysKeys
{

struct KBDLLHOOKSTRUCT
{
  uint32_t vkCode;
  uint32_t scanCode;
  uint32_t flags;
  uint32_t time;
  uint32_t dwExtraInfo;
};

// keyboard low-level hook to disable fast task switching
static HHOOK hHook = 0;
// previous state of the SPI_SETSCREENSAVERRUNNING
static unsigned nPreviousState = 0;
// current enable state
static bool bCurrEnable = true;

LRESULT CALLBACK LowLevelKeyboardProc( INT nCode, WPARAM wParam, LPARAM lParam )
{
  // By returning a non-zero value from the hook procedure, the
  // message does not get passed to the target window
  KBDLLHOOKSTRUCT *pkbhs = (KBDLLHOOKSTRUCT *)lParam;
  bool bControlKeyDown = 0;

  switch ( nCode )
  {
    case HC_ACTION:
      // Check to see if the CTRL key is pressed
      bControlKeyDown = GetAsyncKeyState( VK_CONTROL ) >> ( (sizeof(SHORT) * 8) - 1 );
      // Disable CTRL+ESC
      if ( (pkbhs->vkCode == VK_ESCAPE) && bControlKeyDown )
        return 1;
      // Disable ALT+ESC
      if ( (pkbhs->vkCode == VK_ESCAPE) && (pkbhs->flags & LLKHF_ALTDOWN) )
        return 1;
			// Disable 'Windows' and 'Application' keys
			if ( (pkbhs->vkCode == VK_LWIN) || (pkbhs->vkCode == VK_RWIN) || (pkbhs->vkCode == VK_APPS) ) 
        return 1;
      break;
  }
  return CallNextHookEx( hHook, nCode, wParam, lParam );
}

void EnableSystemKeys( bool bEnable, HINSTANCE hInstance )
{
	if ( (bCurrEnable == bEnable) /*|| is_debugger_present()*/ ) 
		return;
	const uint32_t dwOSVersion = GetVersion();
	if ( dwOSVersion & 0x80000000 ) 
	{
		if ( bEnable )
			SystemParametersInfo( SPI_SETSCREENSAVERRUNNING, nPreviousState, &nPreviousState, 0 );
		else
			SystemParametersInfo( SPI_SETSCREENSAVERRUNNING, TRUE, &nPreviousState, 0 );
	}
	else if ( (dwOSVersion & 0xff) >= 5 ) 
	{
		if ( bEnable && (NSysKeys::hHook != 0) ) 
			UnhookWindowsHookEx( NSysKeys::hHook );
		else
			NSysKeys::hHook = SetWindowsHookEx( WH_KEYBOARD_LL, NSysKeys::LowLevelKeyboardProc, hInstance, 0 );
	}
	bCurrEnable = bEnable;
}

};


