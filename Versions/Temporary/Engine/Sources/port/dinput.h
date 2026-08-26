#pragma once

#include <boost/predef.h>

// DirectInput's vocabulary, and how to reach it from SDL.
//
// This is the physical layer of the keyboard, the one the bindings use.
// Input/Input.cpp's kiKeyInfoList names every control the engine knows -
// "LSHIFT", "NUM_ENTER", "MOUSE_BUTTON2" - and pairs each name with a DIK_ or
// DIMOFS_ code. Those names are the on-disk format: every input.cfg in
// Versions/ and in OBK2_Examples/ is written in them, and a user's own cfg in
// their profile is written in them too. The codes behind the names are not: an
// action id is (device << 24) | code, it is built at startup and never leaves
// the process.
//
// So the codes could have been renumbered off Windows, and this header is the
// argument for not doing it. kiKeyInfoList is a two hundred line table, and a
// second copy of it keyed by SDL_Scancode would be two hundred lines that have
// to be kept in step with the first by hand, in a file where a single wrong row
// silently moves one key's binding. Keeping one table and translating at the
// device boundary is a table of pairs instead, which is checkable by reading it.
//
// The constants are therefore spelled DIK_, DIMOFS_ and DI8DEVTYPE_ rather than
// given neutral names of their own, so that kiKeyInfoList and everything that
// indexes into it carry over unchanged: on Windows they come from the real
// dinput.h and this header adds nothing, and off Windows they are fixed numbers
// that only have to agree with themselves. port/virtualkey.h supplies the
// logical layer the UI reads the same way, and port/window.h the SWP_ and HWND_
// vocabulary.
//
// Nothing here implements DirectInput. Off Windows the device layer is SDL; this
// is only the spelling it reports its results in.
#if BOOST_OS_WINDOWS
#include <dinput.h>
#else

#include <SDL3/SDL.h>

#include <cstdint>

// Device types, as GET_DIDEVICE_TYPE yields them. The values are DirectInput's
// own; nothing outside this process sees them, but there is no reason to differ.
#define DI8DEVTYPE_DEVICE 0x11
#define DI8DEVTYPE_MOUSE 0x12
#define DI8DEVTYPE_KEYBOARD 0x13
#define DI8DEVTYPE_JOYSTICK 0x14
#define DI8DEVTYPE_GAMEPAD 0x15
#define DI8DEVTYPE_DRIVING 0x16

#define GET_DIDEVICE_TYPE( dwDevType ) ( (dwDevType)&0xFF )

// Mouse control offsets, which are the field offsets within DIMOUSESTATE2: three
// axes of four bytes each, then one byte per button.
//
// The button order is DirectInput's, not SDL's, and they disagree about the
// middle button: DirectInput numbers left, right, middle, and SDL numbers left,
// middle, right. Every shipped input.cfg binds the camera rotate to
// MOUSE_BUTTON2, so getting this backwards would put it on the right button.
#define DIMOFS_X 0
#define DIMOFS_Y 4
#define DIMOFS_Z 8
#define DIMOFS_BUTTON0 12
#define DIMOFS_BUTTON1 13
#define DIMOFS_BUTTON2 14
#define DIMOFS_BUTTON3 15
#define DIMOFS_BUTTON4 16
#define DIMOFS_BUTTON5 17
#define DIMOFS_BUTTON6 18
#define DIMOFS_BUTTON7 19

// The size of the mouse state buffer, which is what c_dfDIMouse2.dwDataSize is
// on Windows: eight buttons after the three axes.
#define DIMOUSESTATE2_SIZE 20
// One byte per key, indexed by the DIK_ code itself.
#define DIKEYBOARDSTATE_SIZE 256

// Scan codes, from the keyboard's own numbering rather than from the layout, so
// that a key keeps its binding when the layout changes. Values are DirectInput's.
#define DIK_ESCAPE 0x01
#define DIK_1 0x02
#define DIK_2 0x03
#define DIK_3 0x04
#define DIK_4 0x05
#define DIK_5 0x06
#define DIK_6 0x07
#define DIK_7 0x08
#define DIK_8 0x09
#define DIK_9 0x0A
#define DIK_0 0x0B
#define DIK_MINUS 0x0C
#define DIK_EQUALS 0x0D
#define DIK_BACK 0x0E
#define DIK_TAB 0x0F
#define DIK_Q 0x10
#define DIK_W 0x11
#define DIK_E 0x12
#define DIK_R 0x13
#define DIK_T 0x14
#define DIK_Y 0x15
#define DIK_U 0x16
#define DIK_I 0x17
#define DIK_O 0x18
#define DIK_P 0x19
#define DIK_LBRACKET 0x1A
#define DIK_RBRACKET 0x1B
#define DIK_RETURN 0x1C
#define DIK_LCONTROL 0x1D
#define DIK_A 0x1E
#define DIK_S 0x1F
#define DIK_D 0x20
#define DIK_F 0x21
#define DIK_G 0x22
#define DIK_H 0x23
#define DIK_J 0x24
#define DIK_K 0x25
#define DIK_L 0x26
#define DIK_SEMICOLON 0x27
#define DIK_APOSTROPHE 0x28
#define DIK_GRAVE 0x29
#define DIK_LSHIFT 0x2A
#define DIK_BACKSLASH 0x2B
#define DIK_Z 0x2C
#define DIK_X 0x2D
#define DIK_C 0x2E
#define DIK_V 0x2F
#define DIK_B 0x30
#define DIK_N 0x31
#define DIK_M 0x32
#define DIK_COMMA 0x33
#define DIK_PERIOD 0x34
#define DIK_SLASH 0x35
#define DIK_RSHIFT 0x36
#define DIK_MULTIPLY 0x37
#define DIK_LMENU 0x38
#define DIK_SPACE 0x39
#define DIK_CAPITAL 0x3A
#define DIK_F1 0x3B
#define DIK_F2 0x3C
#define DIK_F3 0x3D
#define DIK_F4 0x3E
#define DIK_F5 0x3F
#define DIK_F6 0x40
#define DIK_F7 0x41
#define DIK_F8 0x42
#define DIK_F9 0x43
#define DIK_F10 0x44
#define DIK_NUMLOCK 0x45
#define DIK_SCROLL 0x46
#define DIK_NUMPAD7 0x47
#define DIK_NUMPAD8 0x48
#define DIK_NUMPAD9 0x49
#define DIK_SUBTRACT 0x4A
#define DIK_NUMPAD4 0x4B
#define DIK_NUMPAD5 0x4C
#define DIK_NUMPAD6 0x4D
#define DIK_ADD 0x4E
#define DIK_NUMPAD1 0x4F
#define DIK_NUMPAD2 0x50
#define DIK_NUMPAD3 0x51
#define DIK_NUMPAD0 0x52
#define DIK_DECIMAL 0x53
#define DIK_OEM_102 0x56
#define DIK_F11 0x57
#define DIK_F12 0x58
#define DIK_F13 0x64
#define DIK_F14 0x65
#define DIK_F15 0x66
#define DIK_KANA 0x70
#define DIK_ABNT_C1 0x73
#define DIK_CONVERT 0x79
#define DIK_NOCONVERT 0x7B
#define DIK_YEN 0x7D
#define DIK_ABNT_C2 0x7E
#define DIK_NUMPADEQUALS 0x8D
#define DIK_PREVTRACK 0x90
#define DIK_AT 0x91
#define DIK_COLON 0x92
#define DIK_UNDERLINE 0x93
#define DIK_KANJI 0x94
#define DIK_STOP 0x95
#define DIK_AX 0x96
#define DIK_UNLABELED 0x97
#define DIK_NEXTTRACK 0x99
#define DIK_NUMPADENTER 0x9C
#define DIK_RCONTROL 0x9D
#define DIK_MUTE 0xA0
#define DIK_CALCULATOR 0xA1
#define DIK_PLAYPAUSE 0xA2
#define DIK_MEDIASTOP 0xA4
#define DIK_VOLUMEDOWN 0xAE
#define DIK_VOLUMEUP 0xB0
#define DIK_WEBHOME 0xB2
#define DIK_NUMPADCOMMA 0xB3
#define DIK_DIVIDE 0xB5
#define DIK_SYSRQ 0xB7
#define DIK_RMENU 0xB8
#define DIK_PAUSE 0xC5
#define DIK_HOME 0xC7
#define DIK_UP 0xC8
#define DIK_PRIOR 0xC9
#define DIK_LEFT 0xCB
#define DIK_RIGHT 0xCD
#define DIK_END 0xCF
#define DIK_DOWN 0xD0
#define DIK_NEXT 0xD1
#define DIK_INSERT 0xD2
#define DIK_DELETE 0xD3
#define DIK_LWIN 0xDB
#define DIK_RWIN 0xDC
#define DIK_APPS 0xDD
#define DIK_POWER 0xDE
#define DIK_SLEEP 0xDF
#define DIK_WAKE 0xE3
#define DIK_WEBSEARCH 0xE5
#define DIK_WEBFAVORITES 0xE6
#define DIK_WEBREFRESH 0xE7
#define DIK_WEBSTOP 0xE8
#define DIK_WEBFORWARD 0xE9
#define DIK_WEBBACK 0xEA
#define DIK_MYCOMPUTER 0xEB
#define DIK_MAIL 0xEC
#define DIK_MEDIASELECT 0xED

// One buffered device event, in the four fields the reader uses. The real
// DIDEVICEOBJECTDATA carries two more that nothing here reads.
//
// dwOfs is the control's offset, which for a keyboard is the DIK_ code itself.
// dwTimeStamp is on the clock GetCurrentTimeMilliseconds returns, because the
// accumulators in Input/BindInternal.h integrate power over the difference
// between an event's stamp and that clock. dwSequence orders events across
// devices, so one counter has to serve all of them.
struct DIDEVICEOBJECTDATA
{
	uint32_t dwOfs;
	uint32_t dwData;
	uint32_t dwTimeStamp;
	uint32_t dwSequence;
};

//! A DIK_ code and the SDL scan code for the same physical key.
struct SDirectInputKey
{
	int nKey;
	SDL_Scancode scancode;
};

// The single source for both directions of the translation, so that they cannot
// disagree.
//
// Nine of kiKeyInfoList's keys are absent, because SDL 3 has no scan code for
// them: DIK_AT, DIK_COLON, DIK_UNDERLINE, DIK_AX and DIK_UNLABELED are keys of
// the Japanese and AX layouts, DIK_ABNT_C2 is the Brazilian keypad separator
// that SDL reports as SDL_SCANCODE_KP_COMMA and DIK_NUMPADCOMMA already claims,
// and DIK_CALCULATOR, DIK_MYCOMPUTER and DIK_MAIL are launch keys SDL 3 dropped.
// They keep their names and their action ids; they simply never fire. No cfg
// anywhere in the tree binds one.
constexpr SDirectInputKey directInputKeys[] = {
	{ DIK_ESCAPE, SDL_SCANCODE_ESCAPE },
	{ DIK_1, SDL_SCANCODE_1 },
	{ DIK_2, SDL_SCANCODE_2 },
	{ DIK_3, SDL_SCANCODE_3 },
	{ DIK_4, SDL_SCANCODE_4 },
	{ DIK_5, SDL_SCANCODE_5 },
	{ DIK_6, SDL_SCANCODE_6 },
	{ DIK_7, SDL_SCANCODE_7 },
	{ DIK_8, SDL_SCANCODE_8 },
	{ DIK_9, SDL_SCANCODE_9 },
	{ DIK_0, SDL_SCANCODE_0 },
	{ DIK_MINUS, SDL_SCANCODE_MINUS },
	{ DIK_EQUALS, SDL_SCANCODE_EQUALS },
	{ DIK_BACK, SDL_SCANCODE_BACKSPACE },
	{ DIK_TAB, SDL_SCANCODE_TAB },
	{ DIK_Q, SDL_SCANCODE_Q },
	{ DIK_W, SDL_SCANCODE_W },
	{ DIK_E, SDL_SCANCODE_E },
	{ DIK_R, SDL_SCANCODE_R },
	{ DIK_T, SDL_SCANCODE_T },
	{ DIK_Y, SDL_SCANCODE_Y },
	{ DIK_U, SDL_SCANCODE_U },
	{ DIK_I, SDL_SCANCODE_I },
	{ DIK_O, SDL_SCANCODE_O },
	{ DIK_P, SDL_SCANCODE_P },
	{ DIK_LBRACKET, SDL_SCANCODE_LEFTBRACKET },
	{ DIK_RBRACKET, SDL_SCANCODE_RIGHTBRACKET },
	{ DIK_RETURN, SDL_SCANCODE_RETURN },
	{ DIK_LCONTROL, SDL_SCANCODE_LCTRL },
	{ DIK_A, SDL_SCANCODE_A },
	{ DIK_S, SDL_SCANCODE_S },
	{ DIK_D, SDL_SCANCODE_D },
	{ DIK_F, SDL_SCANCODE_F },
	{ DIK_G, SDL_SCANCODE_G },
	{ DIK_H, SDL_SCANCODE_H },
	{ DIK_J, SDL_SCANCODE_J },
	{ DIK_K, SDL_SCANCODE_K },
	{ DIK_L, SDL_SCANCODE_L },
	{ DIK_SEMICOLON, SDL_SCANCODE_SEMICOLON },
	{ DIK_APOSTROPHE, SDL_SCANCODE_APOSTROPHE },
	{ DIK_GRAVE, SDL_SCANCODE_GRAVE },
	{ DIK_LSHIFT, SDL_SCANCODE_LSHIFT },
	{ DIK_BACKSLASH, SDL_SCANCODE_BACKSLASH },
	{ DIK_Z, SDL_SCANCODE_Z },
	{ DIK_X, SDL_SCANCODE_X },
	{ DIK_C, SDL_SCANCODE_C },
	{ DIK_V, SDL_SCANCODE_V },
	{ DIK_B, SDL_SCANCODE_B },
	{ DIK_N, SDL_SCANCODE_N },
	{ DIK_M, SDL_SCANCODE_M },
	{ DIK_COMMA, SDL_SCANCODE_COMMA },
	{ DIK_PERIOD, SDL_SCANCODE_PERIOD },
	{ DIK_SLASH, SDL_SCANCODE_SLASH },
	{ DIK_RSHIFT, SDL_SCANCODE_RSHIFT },
	{ DIK_MULTIPLY, SDL_SCANCODE_KP_MULTIPLY },
	{ DIK_LMENU, SDL_SCANCODE_LALT },
	{ DIK_SPACE, SDL_SCANCODE_SPACE },
	{ DIK_CAPITAL, SDL_SCANCODE_CAPSLOCK },
	{ DIK_F1, SDL_SCANCODE_F1 },
	{ DIK_F2, SDL_SCANCODE_F2 },
	{ DIK_F3, SDL_SCANCODE_F3 },
	{ DIK_F4, SDL_SCANCODE_F4 },
	{ DIK_F5, SDL_SCANCODE_F5 },
	{ DIK_F6, SDL_SCANCODE_F6 },
	{ DIK_F7, SDL_SCANCODE_F7 },
	{ DIK_F8, SDL_SCANCODE_F8 },
	{ DIK_F9, SDL_SCANCODE_F9 },
	{ DIK_F10, SDL_SCANCODE_F10 },
	{ DIK_NUMLOCK, SDL_SCANCODE_NUMLOCKCLEAR },
	{ DIK_SCROLL, SDL_SCANCODE_SCROLLLOCK },
	{ DIK_NUMPAD7, SDL_SCANCODE_KP_7 },
	{ DIK_NUMPAD8, SDL_SCANCODE_KP_8 },
	{ DIK_NUMPAD9, SDL_SCANCODE_KP_9 },
	{ DIK_SUBTRACT, SDL_SCANCODE_KP_MINUS },
	{ DIK_NUMPAD4, SDL_SCANCODE_KP_4 },
	{ DIK_NUMPAD5, SDL_SCANCODE_KP_5 },
	{ DIK_NUMPAD6, SDL_SCANCODE_KP_6 },
	{ DIK_ADD, SDL_SCANCODE_KP_PLUS },
	{ DIK_NUMPAD1, SDL_SCANCODE_KP_1 },
	{ DIK_NUMPAD2, SDL_SCANCODE_KP_2 },
	{ DIK_NUMPAD3, SDL_SCANCODE_KP_3 },
	{ DIK_NUMPAD0, SDL_SCANCODE_KP_0 },
	{ DIK_DECIMAL, SDL_SCANCODE_KP_PERIOD },
	{ DIK_OEM_102, SDL_SCANCODE_NONUSBACKSLASH },
	{ DIK_F11, SDL_SCANCODE_F11 },
	{ DIK_F12, SDL_SCANCODE_F12 },
	{ DIK_F13, SDL_SCANCODE_F13 },
	{ DIK_F14, SDL_SCANCODE_F14 },
	{ DIK_F15, SDL_SCANCODE_F15 },
	// the Japanese layout's own keys, where SDL numbers by USB usage and
	// DirectInput by position; these five are the pairs that agree
	{ DIK_KANA, SDL_SCANCODE_INTERNATIONAL2 },
	{ DIK_ABNT_C1, SDL_SCANCODE_INTERNATIONAL1 },
	{ DIK_CONVERT, SDL_SCANCODE_INTERNATIONAL4 },
	{ DIK_NOCONVERT, SDL_SCANCODE_INTERNATIONAL5 },
	{ DIK_YEN, SDL_SCANCODE_INTERNATIONAL3 },
	{ DIK_KANJI, SDL_SCANCODE_LANG5 },
	{ DIK_NUMPADEQUALS, SDL_SCANCODE_KP_EQUALS },
	{ DIK_PREVTRACK, SDL_SCANCODE_MEDIA_PREVIOUS_TRACK },
	{ DIK_STOP, SDL_SCANCODE_STOP },
	{ DIK_NEXTTRACK, SDL_SCANCODE_MEDIA_NEXT_TRACK },
	{ DIK_NUMPADENTER, SDL_SCANCODE_KP_ENTER },
	{ DIK_RCONTROL, SDL_SCANCODE_RCTRL },
	{ DIK_MUTE, SDL_SCANCODE_MUTE },
	{ DIK_PLAYPAUSE, SDL_SCANCODE_MEDIA_PLAY_PAUSE },
	{ DIK_MEDIASTOP, SDL_SCANCODE_MEDIA_STOP },
	{ DIK_VOLUMEDOWN, SDL_SCANCODE_VOLUMEDOWN },
	{ DIK_VOLUMEUP, SDL_SCANCODE_VOLUMEUP },
	{ DIK_WEBHOME, SDL_SCANCODE_AC_HOME },
	{ DIK_NUMPADCOMMA, SDL_SCANCODE_KP_COMMA },
	{ DIK_DIVIDE, SDL_SCANCODE_KP_DIVIDE },
	// DIK_SYSRQ is the key labelled Print Screen; SysRq is what it sends with
	// Alt held. Every shipped input.cfg binds the screenshot to it by that name.
	{ DIK_SYSRQ, SDL_SCANCODE_PRINTSCREEN },
	{ DIK_RMENU, SDL_SCANCODE_RALT },
	{ DIK_PAUSE, SDL_SCANCODE_PAUSE },
	{ DIK_HOME, SDL_SCANCODE_HOME },
	{ DIK_UP, SDL_SCANCODE_UP },
	{ DIK_PRIOR, SDL_SCANCODE_PAGEUP },
	{ DIK_LEFT, SDL_SCANCODE_LEFT },
	{ DIK_RIGHT, SDL_SCANCODE_RIGHT },
	{ DIK_END, SDL_SCANCODE_END },
	{ DIK_DOWN, SDL_SCANCODE_DOWN },
	{ DIK_NEXT, SDL_SCANCODE_PAGEDOWN },
	{ DIK_INSERT, SDL_SCANCODE_INSERT },
	{ DIK_DELETE, SDL_SCANCODE_DELETE },
	{ DIK_LWIN, SDL_SCANCODE_LGUI },
	{ DIK_RWIN, SDL_SCANCODE_RGUI },
	{ DIK_APPS, SDL_SCANCODE_APPLICATION },
	{ DIK_POWER, SDL_SCANCODE_POWER },
	{ DIK_SLEEP, SDL_SCANCODE_SLEEP },
	{ DIK_WAKE, SDL_SCANCODE_WAKE },
	{ DIK_WEBSEARCH, SDL_SCANCODE_AC_SEARCH },
	{ DIK_WEBFAVORITES, SDL_SCANCODE_AC_BOOKMARKS },
	{ DIK_WEBREFRESH, SDL_SCANCODE_AC_REFRESH },
	{ DIK_WEBSTOP, SDL_SCANCODE_AC_STOP },
	{ DIK_WEBFORWARD, SDL_SCANCODE_AC_FORWARD },
	{ DIK_WEBBACK, SDL_SCANCODE_AC_BACK },
	{ DIK_MEDIASELECT, SDL_SCANCODE_MEDIA_SELECT },
};

constexpr int DIRECT_INPUT_KEY_COUNT = sizeof( directInputKeys ) / sizeof( directInputKeys[0] );

//! The DIK_ code for an SDL scan code, or zero for a key the table does not
//! carry. Zero is not a DIK_ code, so it doubles as "no such key".
//!
//! A scan of a hundred and forty rows, rather than a lookup table built at
//! startup, because this runs once per key event: a hundred a second while
//! someone is typing quickly, against a frame that has a scene in it.
inline int SdlScancodeToDirectInputKey( SDL_Scancode scancode )
{
	for ( int i = 0; i < DIRECT_INPUT_KEY_COUNT; ++i )
	{
		if ( directInputKeys[i].scancode == scancode )
		{
			return directInputKeys[i].nKey;
		}
	}
	return 0;
}

//! The SDL scan code for a DIK_ code, or SDL_SCANCODE_UNKNOWN for one of the
//! nine the table does not carry.
inline SDL_Scancode DirectInputKeyToSdlScancode( int nKey )
{
	for ( int i = 0; i < DIRECT_INPUT_KEY_COUNT; ++i )
	{
		if ( directInputKeys[i].nKey == nKey )
		{
			return directInputKeys[i].scancode;
		}
	}
	return SDL_SCANCODE_UNKNOWN;
}

//! The DirectInput mouse button offset for an SDL button number, or -1 for a
//! button beyond the five DIMOUSESTATE2 names. Not zero: zero is DIMOFS_X.
//!
//! SDL numbers left, middle, right, X1, X2 from one. DirectInput numbers left,
//! right, middle, X1, X2 from zero, so the middle and right buttons swap.
inline int SdlMouseButtonToOffset( int nButton )
{
	switch ( nButton )
	{
	case SDL_BUTTON_LEFT: return DIMOFS_BUTTON0;
	case SDL_BUTTON_RIGHT: return DIMOFS_BUTTON1;
	case SDL_BUTTON_MIDDLE: return DIMOFS_BUTTON2;
	case SDL_BUTTON_X1: return DIMOFS_BUTTON3;
	case SDL_BUTTON_X2: return DIMOFS_BUTTON4;
	// SDL numbers anything past X2 straight on, and DIMOUSESTATE2 has three more
	// bytes for them. kiKeyInfoList names all three, so a cfg can bind them.
	case 6: return DIMOFS_BUTTON5;
	case 7: return DIMOFS_BUTTON6;
	case 8: return DIMOFS_BUTTON7;
	default: return -1;
	}
}

#endif
