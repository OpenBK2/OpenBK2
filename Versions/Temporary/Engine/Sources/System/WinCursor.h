#pragma once

#include "System_export.h"

#include <boost/predef.h>

#include <string>

#if BOOST_OS_WINDOWS
#include <windows.h>
#else
struct SDL_Cursor;
#endif


namespace NWinCursor
{
	// What a loaded cursor is on this platform: the Win32 handle, or an SDL
	// cursor which SDL owns until DestroyCursor is called.
	#if BOOST_OS_WINDOWS
	typedef HCURSOR TCursor;
	#else
	typedef SDL_Cursor *TCursor;
	#endif

	//! Load a cursor from a .cur or .ani in the VFS. 0 if it cannot be read.
	//! The parentheses keep windows.h's LoadCursorA/W macro off the name.
	SYSTEM_EXPORT TCursor (LoadCursor)( const std::string &szFileName );
	//! Release what LoadCursor returned. Does nothing for 0.
	SYSTEM_EXPORT void DestroyCursor( TCursor hCursor );
}
