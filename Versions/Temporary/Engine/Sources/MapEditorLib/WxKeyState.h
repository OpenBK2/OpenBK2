#pragma once

// Is a key held down right now?
//
// GetAsyncKeyState( nKey ) & 0x8000 asked this, and it is the one question the
// editor asks outside an event: the camera reads the arrow keys while it is
// moving, and a few states check whether Ctrl, Shift or Alt is held at the
// moment they act rather than when the event arrived.
//
// wxGetKeyState answers it on every platform, but of a wxKeyCode rather than a
// Win32 virtual key code, so the codes the editor actually tests are mapped
// here. Only those: a code nothing asks about is better missing and asserted
// than guessed, because a wrong mapping would read as a key that is never
// down.
//
// Header-only, like the other Wx*.h here; see WxWidget.h for why these stay
// out of MapEditorLib's own translation units.
//
// wxGetKeyState asks GetAsyncKeyState on MSW, which reads the physical keyboard
// on the input desktop, so this cannot be exercised from the probe desktop:
// SendInput is refused there and SetKeyboardState moves GetKeyState, not
// GetAsyncKeyState. Anything resting on it wants a real keyboard to check.

#include "Misc/Asserts.h"
#include "port/vkcodes.h"

#include <wx/defs.h>
#include <wx/utils.h>

namespace NWxKey
{
	inline bool IsDown( unsigned nVirtualKey )
	{
		switch ( nVirtualKey )
		{
			case VK_SHIFT:		return wxGetKeyState( WXK_SHIFT );
			case VK_CONTROL:	return wxGetKeyState( WXK_CONTROL );
			// VK_MENU is the Alt key, whatever its name suggests.
			case VK_MENU:			return wxGetKeyState( WXK_ALT );
			case VK_LEFT:			return wxGetKeyState( WXK_LEFT );
			case VK_RIGHT:		return wxGetKeyState( WXK_RIGHT );
			case VK_UP:				return wxGetKeyState( WXK_UP );
			case VK_DOWN:			return wxGetKeyState( WXK_DOWN );
		}
		NI_ASSERT( false, "NWxKey::IsDown(): no wx key code for this virtual key" );
		return false;
	}
}
