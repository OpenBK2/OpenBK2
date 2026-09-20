#pragma once

#include "MapEditorLib_export.h"

#include <string>

// The editor's message boxes.
//
// ::MessageBox( MainWindowHandle(), text, title, flags ) at twenty-three call
// sites, in five shapes, nearly always titled with the application name and
// owned by the main frame. wx does all of that portably; these wrappers exist
// so the shapes are named rather than spelled out in flags each time, and so
// the title and the owner are decided once.
//
// The owner is the main frame, as MainWindowHandle() was: a message box with no
// owner can fall behind the editor, and wx disables its parent's top-level
// window while it is up, which is what MB_APPLMODAL did. Two call sites used to
// pass no owner at all (the acks Excel reader and the MechUnit exporter); they
// get one now.
//
// Text is UTF-8, as every narrow string in the tree is. A few of these strings
// carry \r\n, kept as the original wrote them; that is fine on Windows, but GTK
// draws the CR, so they want rewriting when the editor is run there.
//
// Declared without naming a toolkit, and implemented once in MessageBoxesWx.cpp,
// so that no caller has to include wx. That is not tidiness: a translation unit
// that includes a wx header loses windows.h's A/W macros, and the tree exports
// NDb::GetObjectA. Such a TU emits a call to NDb::GetObject and does not link.
// Three of the callers below reach NDb::Get<> or CDBPtr, whose lazy load calls
// GetObject from a template in System/DB.h, so the fault would not even be
// visible at the call site. See MapEditorLib/MainWindow.h for the same trap.
namespace NMessage
{
	// MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2: the question the editor asks
	// before throwing work away, with No as the default so that hitting return
	// discards nothing.
	MAPEDITORLIB_EXPORT bool AskYesNo( const std::string &rszText );

	// The same with a third way out, and No still the default: MB_DEFBUTTON2 put
	// it on No, and on the prompt this mostly serves -- save the changes before
	// closing? -- that is what decides what the return key does.
	enum EAnswer { ANSWER_YES, ANSWER_NO, ANSWER_CANCEL };

	MAPEDITORLIB_EXPORT EAnswer AskYesNoCancel( const std::string &rszText );

	MAPEDITORLIB_EXPORT void Error( const std::string &rszText );
	MAPEDITORLIB_EXPORT void Warning( const std::string &rszText );
	MAPEDITORLIB_EXPORT void Information( const std::string &rszText );

	// For the few that name the job rather than the application: an export, an
	// import or a registration says which one it was.
	MAPEDITORLIB_EXPORT void Error( const std::string &rszText, const std::string &rszTitle );
	MAPEDITORLIB_EXPORT void Information( const std::string &rszText, const std::string &rszTitle );
}
