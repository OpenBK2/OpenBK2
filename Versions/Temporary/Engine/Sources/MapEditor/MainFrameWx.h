#pragma once

// The editor's main frame, in wx.
//
// Nothing here names wx, so CEditorApp can make the frame without knowing.
namespace NMainFrameWx
{
	// Makes the frame and registers it as the main frame. Hidden until Show.
	bool Create();
	// Shows the frame where the last session left it.
	void Show();
}
