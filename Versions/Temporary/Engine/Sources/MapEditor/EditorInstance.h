#pragma once

#include <string>

// One editor at a time, and a way to hand the second one's file to the first.
//
// This replaces CMapEditorSingleton, which did both jobs with Win32 directly:
// a named file mapping whose name was the mutual exclusion, holding the running
// editor's HWND so a second instance could find it, and WM_COPYDATA to carry
// the path across -- the one Win32 message the kernel marshals between
// processes. None of that ports, so it is gone rather than translated.
//
// wxSingleInstanceChecker is the exclusion (a named mutex on Windows, a lock
// file under the user's directory elsewhere) and wxIPC is the transport.
//
// Deliberately toolkit-free: CEditorApp::Initialize asks these questions before
// any frame exists, and MapEditorApp.cpp has no wx in it. See
// MapEditorLib/MessageBoxes.h for why that matters.
namespace NEditorInstance
{
	// What distinguishes this editor from another build of it, as
	// CMapEditorSingletonBase::SetMapFileName named the shared section. Set
	// before anything below is called.
	void SetName( const std::string &rszName );

	// Is another editor of this name already running? When it is not, this
	// process takes the lock and keeps it until Release.
	//
	// Answering false here is what lets the editor go on starting, so it is
	// asked once, early.
	bool IsAnotherRunning();

	// Ask the editor that holds the lock to come to the front, and to open
	// rszFilePath as well when that is not empty. False when it could not be
	// reached -- which happens when it is still starting up and not yet
	// answering, the same gap CreateMapFile left.
	bool AskRunningToOpen( const std::string &rszFilePath );

	// Start answering those asks. The main frame is raised by this side; the
	// handler is given the path, and is not called at all when the other
	// instance only wanted the window brought forward.
	typedef void ( *TOpenHandler )( const std::string &rszFilePath );
	void StartAnswering( TOpenHandler pfnOpen );

	// Stop answering and drop the lock. Safe to call without either.
	void Release();
}
