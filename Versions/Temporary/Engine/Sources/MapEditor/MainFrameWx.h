#pragma once

// The editor's main frame in wx, beside CMainFrame rather than instead of it.
//
// Both frames stay in the build while the frame moves across, so the two can
// be run side by side and compared: how they look, how they behave, and how
// their code reads. Which one a session gets is chosen once, at startup, by
// OBK2_WX_FRAME in the environment. The views inside either frame are wx's.
//
// Nothing here names wx, so CEditorApp can ask without knowing.
#include "MapEditor_export.h"

namespace NMainFrameWx
{
	// wx is built in, and OBK2_WX_FRAME is not set to 0.
	// Exported because the executable asks too, before anything else runs, to
	// decide whose message loop the session has.
	MAPEDITOR_EXPORT bool IsWanted();
	// Makes the frame, registers it as the main frame, and makes the MFC
	// application's main window a CWnd over its handle, so AfxGetMainWnd and
	// every MFC dialog owned by the frame keep working. Hidden until Show.
	bool Create();
	// Shows the frame where the last session left it.
	void Show();
}
