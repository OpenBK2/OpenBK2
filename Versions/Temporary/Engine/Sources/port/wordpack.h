#pragma once

#include <boost/predef.h>

// The windef.h macros that split a 32 bit value into halves and put it back.
//
// These are not spelling. AILogic packs a pair of coordinates with MAKELONG into
// the update stream, and GameX/WorldClient.cpp takes them apart again with
// LOWORD and HIWORD, so the two have to agree bit for bit across a build of the
// game and across the wire between two of them. They are reproduced exactly as
// Windows writes them rather than expressed as functions, for two reasons.
//
// The result type carries meaning. LOWORD and HIWORD yield WORD, which is
// unsigned, so an unpacked half is 0 to 65535 and never negative. A helper
// returning int would hand negative coordinates to a CVec2 for any value with
// bit 15 set.
//
// And the argument conversion carries meaning too. AILogic calls MAKELONG with
// the float members of a CVec2, so the cast in the macro is what truncates them.
// A function taking int would convert at the call instead, which is a different
// conversion with different results at the edges.
//
// DWORD_PTR is spelled ULONG_PTR, which is what it is defined as on Windows and
// what DXVK's shim carries.
#if BOOST_OS_WINDOWS
#include <windows.h>
#else
#include <windows.h>

#define LOWORD( l ) ( (WORD)( ( (ULONG_PTR)( l ) ) & 0xffff ) )
#define HIWORD( l ) ( (WORD)( ( ( (ULONG_PTR)( l ) ) >> 16 ) & 0xffff ) )
#define MAKEWORD( a, b ) \
	( (WORD)( ( (BYTE)( ( (ULONG_PTR)( a ) ) & 0xff ) ) | ( (WORD)( (BYTE)( ( (ULONG_PTR)( b ) ) & 0xff ) ) ) << 8 ) )
#define MAKELONG( a, b ) \
	( (LONG)( ( (WORD)( ( (ULONG_PTR)( a ) ) & 0xffff ) ) | ( (DWORD)( (WORD)( ( (ULONG_PTR)( b ) ) & 0xffff ) ) ) << 16 ) )

#endif
