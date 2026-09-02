#pragma once

// SECControlBar
#include "sbarcore.h"

// SECControlBarManager
#include "sbarmgr.h"

// SECToolBarManager
#include "tbarmgr.h"

// SECWorkspaceManagerEx
#include "swsmgrex.h"

// SECFrameWnd
#include "swinfrm.h"

// SECWorkbook
#include "secwb.h"

// SECCustomToolBar
#include "tbarcust.h"

// SECToolBarSheet
#include "tbarpage.h"

// SECToolBarsPage
#include "tbarsdlg.h"

// SEC3DTabWnd
#include "tabwnd3.h"

// SECShortcutBar
#include "olbar.h"

// SECTreeCtrl
#include "trctlx.h"

// SECShortcutBar
#include "olbar.h"

// https://help.perforce.com/stingray/11/html/otug/13-4.html
#ifndef TCM_TABSEL
#define TCM_TABSEL (WM_USER + 1000)
#endif

#ifndef TCM_TABDBLCLK
#define TCM_TABDBLCLK (WM_USER + 1001)
#endif

#ifndef TCM_TABSELCLR
#define TCM_TABSELCLR (WM_USER + 1002)
#endif

#ifndef TCM_TABREACTIVATE
#define TCM_TABREACTIVATE (WM_USER + 1003)
#endif

// Windows CE?
#ifndef LVSIL_HEADER
#define LVSIL_HEADER 3
#endif

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_HTML_User_Guide/Content/otugTOC/Tabbed_Window_Styles.htm

enum {
    // Only the left and right scroll buttons are shown.
    // The user can only scroll the tabs to the left and right.
    // There are no buttons for jumping to the first tab or to the last tab.
    // This style is only valid for SECTabWnd.
    TWS_LEFTRIGHTSCROLL,
    // All four of the scroll buttons are shown in the lower left corner of the tabbed window.
    // These four scroll buttons allow the user to scroll the tabs in the tabbed window to the first tab,
    // to the last tab, a few pixels to the left, and a few pixels to the right.
    // If you do not require the <scroll to first> and <scroll to last> buttons,
    // use the TWS_LEFTRIGHTSCROLL style instead. This style is only valid for SECTabWnd.
    TWS_FULLSCROLL,
};
