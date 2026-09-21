#pragma once

#include "MapEditorLib_export.h"

#include <string>

// What build of the editor this is, as a block of "name : value" lines.
//
// This existed twice and the two copies said different things. The About box
// (MapEditor/AboutViewWx.cpp) collected the version, the revision, the build,
// the configuration, the compiler, wx, the OS and the code page, for bug
// reports; the log window got two lines at startup naming the Granny header and
// the Granny DLL, and nothing else. So a report pasted out of the log named a
// library and not the build, and one pasted out of the About box named the
// build and not the libraries.
//
// One function now, shown by the About box and logged at startup, so that the
// two cannot drift apart again, and neither list is missing what the other has.
//
// Declared without naming a toolkit and implemented in BuildDetailsWx.cpp, for
// the reason MessageBoxes.h spells out: a translation unit that includes a wx
// header loses windows.h's A/W macros and stops linking against
// NDb::GetObjectA.
namespace NBuildDetails
{
	// One "name : value" per line, every line newline-terminated. UTF-8.
	//
	// Nothing is cached: the MOD, the display scale and the git revision of a
	// dirty tree can all differ between two calls.
	MAPEDITORLIB_EXPORT std::string Collect();

	// The same lines, into the editor's log window, under a heading.
	//
	// Here rather than at the call site so that the one thing in the block that
	// is not just information -- a Granny version mismatch -- is logged as the
	// error it is, and is so in every front end that calls this.
	MAPEDITORLIB_EXPORT void Log();
}
