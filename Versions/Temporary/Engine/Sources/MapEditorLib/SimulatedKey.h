#pragma once

// A key press delivered with its modifiers stated, for the probe tooling.
//
// The editor runs its probes on a second Win32 desktop, where SendInput is
// refused and only posted messages arrive. A posted WM_KEYDOWN carries no
// modifier state: CLuaEditor asks GetAsyncKeyState whether Ctrl is down, wx
// asks GetKeyState, and neither can be set from outside. So Ctrl+F in the
// script editor could not be tested at all.
//
// A window that has keyboard shortcuts worth testing handles this registered
// message and sends it into the same function its key handler calls, with the
// modifiers taken from the message rather than from the keyboard. What is
// skipped is only the operating system's answer to "is Ctrl down"; the
// dispatch behind it, and everything the shortcut opens, is the real code.
// scripts/port/simkey.py is the other end.
//
//   wParam OP_KEY               lParam MAKELPARAM( virtual key, EModifier bits )
//                               answers 1 when the window handled the message
//   wParam OP_SELECTION_START   answers the editor's selection start, in the
//   wParam OP_SELECTION_END     editor's own positions -- how a probe sees where
//                               Find Next went
//
// Registered, not WM_APP + n, so it cannot collide with a message some control
// already uses, and a probe finds it by name. Needs <windows.h>, which every
// project here has from its stdafx.h.
//
// **Windows only, and not a thing to port.** This is test scaffolding of ours,
// not the editor's own code, and it exists to work around a problem that only
// Windows has: the probe desktop. A second Win32 desktop refuses SendInput, so
// a probe can only post messages, and a posted WM_KEYDOWN says nothing about
// Ctrl -- hence a message that states the modifiers instead. Nothing off
// Windows has that shape. A Linux probe would drive the editor some other way
// and would want its own answer, which may well not be a window message at
// all, so the right move then is to write that rather than translate this.
//
// The one user, TextEditorViewWx.cpp, already guards its MSWWindowProc with
// __WXMSW__. The body is guarded here too so that the header is safe for
// anyone who includes it without thinking about the platform; off Windows it
// is empty and any use of it will not compile, which is the intended answer.
//
// BOOST_OS_WINDOWS rather than __WXMSW__ because nothing here includes wx.
#include <boost/predef.h>

#if BOOST_OS_WINDOWS

namespace NSimulatedKey
{
	inline unsigned Message()
	{
		static const unsigned nMessage = ::RegisterWindowMessageW( L"OBK2.SimulatedKey" );
		return nMessage;
	}

	enum EOperation
	{
		OP_KEY = 1,
		OP_SELECTION_START = 2,
		OP_SELECTION_END = 3,
	};

	enum EModifier
	{
		MODIFIER_CONTROL = 0x0001,
		MODIFIER_SHIFT = 0x0002,
	};

	inline unsigned KeyOf( intptr_t nLParam )
	{
		return static_cast<unsigned>( nLParam & 0xFFFF );
	}

	inline bool HasModifier( intptr_t nLParam, EModifier eModifier )
	{
		return ( ( nLParam >> 16 ) & eModifier ) != 0;
	}
}

#endif // BOOST_OS_WINDOWS
