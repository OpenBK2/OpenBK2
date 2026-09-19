#pragma once

// The editor's main frame, in wx.
//
// Nothing here names wx, so CEditorApp can make the frame without knowing.
namespace NMainFrameWx
{
	// Makes the frame, registers it as the main frame, and makes the MFC
	// application's main window a CWnd over its handle, so AfxGetMainWnd and
	// every MFC dialog owned by the frame keep working. Hidden until Show.
	bool Create();
	// Shows the frame where the last session left it.
	void Show();
}
