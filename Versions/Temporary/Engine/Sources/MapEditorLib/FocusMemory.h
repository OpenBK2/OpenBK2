#pragma once

#include "MapEditorLib_export.h"

#include <memory>

// Remembering which window has the keyboard focus, so it can be given back.
//
// The progress dialog does this around its own lifetime: whatever had the
// focus when it opened has it again when it closes. The window may well be
// gone by then, since the command showing the progress is free to destroy the
// view that started it, and that is what the Win32 version was guarding
// against:
//
//   hwndPreviousFocus = ::GetFocus();
//   ...
//   if ( ::IsWindow( hwndPreviousFocus ) ) { ::SetFocus( hwndPreviousFocus ); }
//
// IsWindow is a weak test and always was. A handle belongs to a window until
// that window is destroyed and then belongs to nothing -- but handles are
// reused, so a window created in between can be given one that a stale handle
// still names, and the focus then goes somewhere unrelated rather than
// nowhere. The window this is built on nulls itself when its window is
// destroyed, which is the test the code wanted.
//
// Declared without naming a toolkit and implemented once in FocusMemoryWx.cpp,
// for the reason MessageBoxes.h gives: a translation unit that includes a wx
// header loses windows.h's A/W macros and stops linking against
// NDb::GetObjectA. MainFrameShared.cpp, which is where this is used, names no
// toolkit at all on purpose -- both frames share it.
class MAPEDITORLIB_EXPORT CFocusMemory
{
	struct SImpl;
	// Out of line, and the destructor with it: unique_ptr needs SImpl complete
	// to destroy it, and it is not complete here.
	std::unique_ptr<SImpl> pImpl;

public:
	CFocusMemory();
	~CFocusMemory();

	// The window with the focus now, if any. Replaces whatever was remembered.
	void Remember();
	// The focus back to that window, if it is still there. Nothing otherwise,
	// including when nothing was remembered.
	void Restore();
};
