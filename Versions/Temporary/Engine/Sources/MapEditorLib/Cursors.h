#pragma once

#include "MapEditorLib_export.h"

#include "Misc/Geom.h"

// The mouse cursor's shape, for the editor states that change it while they
// work.
//
// This was ::SetCursor( ::LoadCursor( 0, IDC_* ) ), with a null module handle,
// so every one of them was a stock system cursor rather than a resource. wx's
// wxSetCursor is the same thing: on MSW it ends in ::SetCursor with the
// cursor's handle, so Windows behaves exactly as before, and wxGTK has its own
// implementation. Both set the cursor globally and transiently, which is what
// Win32 did and what the callers expect -- they set it again on the next mouse
// move rather than once per window.
//
// Declared without naming a toolkit, and implemented once in CursorsWx.cpp,
// for the reason MessageBoxes.h gives: a translation unit that includes a wx
// header loses windows.h's A/W macros and stops linking against
// NDb::GetObjectA. MapObjectState, which uses all of this, is one of the files
// that must never see one.
namespace NCursor
{
	enum EShape
	{
		// What the editor puts back when it is not saying anything: IDC_ARROW.
		SHAPE_ARROW,
		// "This is a valid target", over a link the mouse is about to make.
		// IDC_UPARROW, which **wx has no stock cursor for**: there is no
		// wxCURSOR_UP_ARROW, wxCURSOR_BASED_ARROW_UP is X11 only, and nothing in
		// wx's MSW stock table maps to it. So this one is an asset of ours,
		// res/uparrow.cur, drawn by scripts/port/mkcursor.py and embedded in
		// CursorsWx.cpp. It is a placeholder: anyone who wants to draw a better
		// one should, and only that file changes.
		SHAPE_UP_ARROW,
		// "Not here", which the drag over a rejected target shows: IDC_NO.
		SHAPE_NO_ENTRY,
	};

	MAPEDITORLIB_EXPORT void Set( EShape eShape );

	// Where the mouse is now, in screen coordinates: ::GetCursorPos, which is
	// wxGetMousePosition. Asked for outside an event, which is why it is this
	// and not the position a wxMouseEvent carries.
	MAPEDITORLIB_EXPORT CTPoint<int> GetPosition();
}
