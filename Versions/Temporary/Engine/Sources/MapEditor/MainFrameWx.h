#pragma once

// The editor's main frame in wx, beside CMainFrame rather than instead of it.
//
// Both frames stay in the build while the frame moves across, so the two can
// be run side by side and compared: how they look, how they behave, and how
// their code reads. Which one a session gets is chosen once, at startup, by
// OBK2_WX_FRAME in the environment -- not by OBK2_WX_DIALOGS, which picks the
// toolkit of the views inside whichever frame is up.
//
// Nothing here names wx, so CEditorApp can ask without knowing. In a build
// without BUILD_WX_EDITOR, IsWanted answers false and the MFC frame is used.
namespace NMainFrameWx
{
	// OBK2_WX_FRAME is set to something other than 0, and wx is built in.
	bool IsWanted();
	// Makes the frame, registers it as the main frame, and makes the MFC
	// application's main window a CWnd over its handle, so AfxGetMainWnd and
	// every MFC dialog owned by the frame keep working. Hidden until Show.
	bool Create();
	// Shows the frame where the last session left it.
	void Show();
}
