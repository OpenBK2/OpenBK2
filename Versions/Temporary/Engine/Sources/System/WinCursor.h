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
	// What a loaded cursor is on this platform, and what one displayable frame of
	// it is.
	//
	// On Windows the two are the same thing: LoadCursorFromFile hands back one
	// handle for the whole file and USER32 runs any animation behind it, so
	// there is never a frame to pick. Off Windows SDL manages cursors but has no
	// animated one, so a loaded cursor is the frames plus their timing and a
	// frame is a single SDL cursor which SDL owns until DestroyCursor is called.
	#if BOOST_OS_WINDOWS
	typedef HCURSOR TCursor;
	typedef HCURSOR TFrame;
	#else
	struct SCursor;
	typedef SCursor *TCursor;
	typedef SDL_Cursor *TFrame;
	#endif

	//! Load a cursor from a .cur or .ani in the VFS. 0 if it cannot be read.
	//! The parentheses keep windows.h's LoadCursorA/W macro off the name.
	SYSTEM_EXPORT TCursor (LoadCursor)( const std::string &szFileName );
	//! Release what LoadCursor returned. Does nothing for 0.
	SYSTEM_EXPORT void DestroyCursor( TCursor hCursor );

	//! How many steps the animation has, 1 for a still cursor and 0 for no
	//! cursor. Always 1 on Windows, whatever the file holds, because there the
	//! animation is not ours to run.
	SYSTEM_EXPORT int GetStepCount( TCursor hCursor );
	//! How long step nStep is shown, in milliseconds. At least one jiffy for a
	//! step that exists, so a caller may advance a deadline by it without the
	//! risk of standing still. 0 on Windows, where nothing steps.
	SYSTEM_EXPORT int GetStepDelay( TCursor hCursor, int nStep );
	//! What to display at step nStep, to be handed to NWinFrame::SetCursor.
	SYSTEM_EXPORT TFrame GetStepFrame( TCursor hCursor, int nStep );
}
