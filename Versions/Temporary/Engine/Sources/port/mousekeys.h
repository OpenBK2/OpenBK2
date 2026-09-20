#pragma once

#include <boost/predef.h>

// The MK_ flags a Win32 mouse message carries in its wParam: which buttons and
// which modifier keys were down at the moment of the event.
//
// These are not virtual key codes and the two are easy to confuse. A VK_ code
// names one key and is compared with ==; an MK_ flag is one bit of a mask of
// what is held down right now, and the editor's input states test it with &.
// The two even overlap in spelling -- MK_CONTROL and VK_CONTROL -- while
// meaning different things and numbering differently.
//
// The editor's states read these because ED_Common/SceneSurfaceWx.cpp hands
// them the wParam of the mouse message untouched, so IInputState's nFlags is
// the Win32 mask itself. That is a decision with an end date rather than a
// design: see the comment in SceneSurfaceWx.cpp for what a portable wx surface
// would have to reproduce, of which these flags are one part. wxMouseEvent
// answers the same questions with LeftIsDown, ShiftDown and their siblings,
// and when the viewport moves the states can be asked to speak that instead.
//
// Until then the numbers have to exist off Windows for the states to compile.
// On Windows they come from windows.h and this header adds nothing.
#if BOOST_OS_WINDOWS
#include <windows.h>
#else

#define MK_LBUTTON 0x0001
#define MK_RBUTTON 0x0002
#define MK_SHIFT 0x0004
#define MK_CONTROL 0x0008
#define MK_MBUTTON 0x0010
#define MK_XBUTTON1 0x0020
#define MK_XBUTTON2 0x0040

#endif
