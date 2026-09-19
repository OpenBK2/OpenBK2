#pragma once

// The main window's native handle, for what still speaks Win32 to it: message
// boxes, a popup menu. It was MainFrameWnd()->GetSafeHwnd(), a CWnd attached
// over the wx frame's handle so that MFC code could find it; that CWnd is gone.
//
// A widget's GetNativeWidget is its window handle in this front-end (see
// WxWidget.h). No wx here, on purpose: wx's headers undefine windows.h's A/W
// macros, and a translation unit that includes them sees NDb::GetObject where
// the rest of the tree sees NDb::GetObjectA, which does not link.

#include "Interface_MainFrame.h"

inline HWND MainWindowHandle()
{
	IMainFrameContainer *const pContainer = Singleton<IMainFrameContainer>();
	IWidget *const pWindow = ( pContainer != nullptr ) ? pContainer->GetMainWindow() : nullptr;
	return ( pWindow != nullptr ) ? static_cast<HWND>( pWindow->GetNativeWidget() ) : 0;
}
