#pragma once

#include <boost/predef.h>

// Windows virtual key codes, and how to reach them from an SDL scancode.
//
// This is the logical layer of the keyboard, the one the UI reads. WinFrame
// posts a key code in SWindowsMsg::nKey and UI/WindowEditLine, UI/WindowConsole
// and UI/UIScreen compare it against VK_ constants. On Windows that code is the
// wParam of WM_KEYDOWN and nothing has to happen. Off Windows it has to be made,
// and the only honest source is the scancode: a keycode already has the layout
// and the modifiers folded in, and VK codes do not.
//
// Note this is not the layer the bindings use. Those are physical, keyed by name
// through Input/Input.cpp's kiKeyInfoList, and never meet a VK code.
//
// The constants are spelled VK_ rather than given a neutral name of their own so
// that the UI comparisons carry over unchanged: on Windows they come from
// windows.h and this header adds nothing, and off Windows they are fixed numbers
// that only have to agree with themselves. port/window.h supplies the SWP_ and
// HWND_ vocabulary the same way.
#if BOOST_OS_WINDOWS
#include <windows.h>
#else
#include <SDL3/SDL.h>
#endif

#if !BOOST_OS_WINDOWS

#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_CLEAR 0x0C
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_PAUSE 0x13
#define VK_CAPITAL 0x14
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_SNAPSHOT 0x2C
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_LWIN 0x5B
#define VK_RWIN 0x5C
#define VK_APPS 0x5D
#define VK_NUMPAD0 0x60
#define VK_NUMPAD1 0x61
#define VK_NUMPAD2 0x62
#define VK_NUMPAD3 0x63
#define VK_NUMPAD4 0x64
#define VK_NUMPAD5 0x65
#define VK_NUMPAD6 0x66
#define VK_NUMPAD7 0x67
#define VK_NUMPAD8 0x68
#define VK_NUMPAD9 0x69
#define VK_MULTIPLY 0x6A
#define VK_ADD 0x6B
#define VK_SEPARATOR 0x6C
#define VK_SUBTRACT 0x6D
#define VK_DECIMAL 0x6E
#define VK_DIVIDE 0x6F
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_NUMLOCK 0x90
#define VK_SCROLL 0x91
#define VK_LSHIFT 0xA0
#define VK_RSHIFT 0xA1
#define VK_LCONTROL 0xA2
#define VK_RCONTROL 0xA3
#define VK_LMENU 0xA4
#define VK_RMENU 0xA5
// The OEM keys are the ones whose engraving moves with the layout. The number is
// the position, and the character named beside it is what a US keyboard prints
// there, which is the only layout these codes are defined against.
#define VK_OEM_1 0xBA        // ; :
#define VK_OEM_PLUS 0xBB     // = +
#define VK_OEM_COMMA 0xBC    // , <
#define VK_OEM_MINUS 0xBD    // - _
#define VK_OEM_PERIOD 0xBE   // . >
#define VK_OEM_2 0xBF        // / ?
#define VK_OEM_3 0xC0        // ` ~
#define VK_OEM_4 0xDB        // [ {
#define VK_OEM_5 0xDC        // backslash |
#define VK_OEM_6 0xDD        // ] }
#define VK_OEM_7 0xDE        // ' "
#define VK_OEM_102 0xE2      // the extra key on a 102 key board

//! The virtual key code for a physical key, or 0 for one that has none.
//!
//! Taken from the scancode rather than from the event's keycode. A VK code names
//! a position on a US layout, which is what a scancode also names; a keycode
//! names a character after the layout and any modifiers have been applied, so
//! deriving one from the other would make the UI's key handling change meaning
//! when the user switches layout.
inline int SdlScancodeToVirtualKey( SDL_Scancode nScancode )
{
	// letters and digits are their own uppercase ASCII, which is the one place
	// the VK numbering is not arbitrary
	if ( nScancode >= SDL_SCANCODE_A && nScancode <= SDL_SCANCODE_Z )
	{
		return 'A' + ( nScancode - SDL_SCANCODE_A );
	}
	if ( nScancode >= SDL_SCANCODE_1 && nScancode <= SDL_SCANCODE_9 )
	{
		return '1' + ( nScancode - SDL_SCANCODE_1 );
	}
	if ( nScancode >= SDL_SCANCODE_F1 && nScancode <= SDL_SCANCODE_F12 )
	{
		return VK_F1 + ( nScancode - SDL_SCANCODE_F1 );
	}

	switch ( nScancode )
	{
	case SDL_SCANCODE_0: return '0';

	case SDL_SCANCODE_RETURN: return VK_RETURN;
	case SDL_SCANCODE_ESCAPE: return VK_ESCAPE;
	case SDL_SCANCODE_BACKSPACE: return VK_BACK;
	case SDL_SCANCODE_TAB: return VK_TAB;
	case SDL_SCANCODE_SPACE: return VK_SPACE;
	case SDL_SCANCODE_CAPSLOCK: return VK_CAPITAL;

	case SDL_SCANCODE_PRINTSCREEN: return VK_SNAPSHOT;
	case SDL_SCANCODE_SCROLLLOCK: return VK_SCROLL;
	case SDL_SCANCODE_PAUSE: return VK_PAUSE;
	case SDL_SCANCODE_INSERT: return VK_INSERT;
	case SDL_SCANCODE_HOME: return VK_HOME;
	case SDL_SCANCODE_PAGEUP: return VK_PRIOR;
	case SDL_SCANCODE_DELETE: return VK_DELETE;
	case SDL_SCANCODE_END: return VK_END;
	case SDL_SCANCODE_PAGEDOWN: return VK_NEXT;
	case SDL_SCANCODE_RIGHT: return VK_RIGHT;
	case SDL_SCANCODE_LEFT: return VK_LEFT;
	case SDL_SCANCODE_DOWN: return VK_DOWN;
	case SDL_SCANCODE_UP: return VK_UP;

	// the keypad keeps its own codes, as it does on Windows, so a binding to
	// keypad 1 stays distinct from a binding to the 1 above the letters
	case SDL_SCANCODE_NUMLOCKCLEAR: return VK_NUMLOCK;
	case SDL_SCANCODE_KP_DIVIDE: return VK_DIVIDE;
	case SDL_SCANCODE_KP_MULTIPLY: return VK_MULTIPLY;
	case SDL_SCANCODE_KP_MINUS: return VK_SUBTRACT;
	case SDL_SCANCODE_KP_PLUS: return VK_ADD;
	// Windows has no virtual key for the keypad's Enter: it reports VK_RETURN
	// and distinguishes the two by an extended-key bit this path does not carry
	case SDL_SCANCODE_KP_ENTER: return VK_RETURN;
	case SDL_SCANCODE_KP_1: return VK_NUMPAD1;
	case SDL_SCANCODE_KP_2: return VK_NUMPAD2;
	case SDL_SCANCODE_KP_3: return VK_NUMPAD3;
	case SDL_SCANCODE_KP_4: return VK_NUMPAD4;
	case SDL_SCANCODE_KP_5: return VK_NUMPAD5;
	case SDL_SCANCODE_KP_6: return VK_NUMPAD6;
	case SDL_SCANCODE_KP_7: return VK_NUMPAD7;
	case SDL_SCANCODE_KP_8: return VK_NUMPAD8;
	case SDL_SCANCODE_KP_9: return VK_NUMPAD9;
	case SDL_SCANCODE_KP_0: return VK_NUMPAD0;
	case SDL_SCANCODE_KP_PERIOD: return VK_DECIMAL;

	// per-side modifiers. Windows also has the merged VK_SHIFT, VK_CONTROL and
	// VK_MENU, which a key event never carries: those are what GetKeyState
	// answers, and the message itself names the side.
	case SDL_SCANCODE_LCTRL: return VK_LCONTROL;
	case SDL_SCANCODE_LSHIFT: return VK_LSHIFT;
	case SDL_SCANCODE_LALT: return VK_LMENU;
	case SDL_SCANCODE_LGUI: return VK_LWIN;
	case SDL_SCANCODE_RCTRL: return VK_RCONTROL;
	case SDL_SCANCODE_RSHIFT: return VK_RSHIFT;
	case SDL_SCANCODE_RALT: return VK_RMENU;
	case SDL_SCANCODE_RGUI: return VK_RWIN;
	case SDL_SCANCODE_APPLICATION: return VK_APPS;

	// the engraved keys, by position rather than by what they print
	case SDL_SCANCODE_MINUS: return VK_OEM_MINUS;
	case SDL_SCANCODE_EQUALS: return VK_OEM_PLUS;
	case SDL_SCANCODE_LEFTBRACKET: return VK_OEM_4;
	case SDL_SCANCODE_RIGHTBRACKET: return VK_OEM_6;
	case SDL_SCANCODE_BACKSLASH: return VK_OEM_5;
	case SDL_SCANCODE_SEMICOLON: return VK_OEM_1;
	case SDL_SCANCODE_APOSTROPHE: return VK_OEM_7;
	case SDL_SCANCODE_GRAVE: return VK_OEM_3;
	case SDL_SCANCODE_COMMA: return VK_OEM_COMMA;
	case SDL_SCANCODE_PERIOD: return VK_OEM_PERIOD;
	case SDL_SCANCODE_SLASH: return VK_OEM_2;
	case SDL_SCANCODE_NONUSBACKSLASH: return VK_OEM_102;

	default: return 0;
	}
}

#endif
