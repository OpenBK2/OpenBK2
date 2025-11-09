#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include "tabwndb.h"

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec3dtabwnd.htm

enum {
    // Creates the window with tabs on the bottom.
    TWS_TABS_ON_BOTTOM,
    // Creates the window with tabs on the top.
    TWS_TABS_ON_TOP,
    // Creates the window with tabs on the left.
    TWS_TABS_ON_LEFT,
    // Creates the window with tabs on the right.
    TWS_TABS_ON_RIGHT,
    // When set, the active tab will not be drawn with emphasis.
    TWS_NOACTIVE_TAB_ENLARGED,
    // Provides tabs in the style similar to that in Visual Studio.
    TWS_DRAW_STUDIO_LIKE,
    // Provides a normal 3D border for the client area.
    TWS_DRAW_3D_NORMAL,
    // Allows drag-and-drop rearrangements of the tabs.
    TWS_DYNAMIC_ARRANGE_TABS,
};

class SEC3DTabWnd : public SECTabWndBase {
public:
    // Initialization
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec3dtabwnd__sec3dtabwnd.htm
    SEC3DTabWnd();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec3dtabwnd__create.htm
    virtual BOOL Create(CWnd* pParentWnd, DWORD dwStyle = WS_CHILD | WS_VISIBLE | TWS_TABS_ON_BOTTOM, UINT nID = AFX_IDW_PANE_FIRST);

    // Operations
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec3dtabwnd__gettabstyle.htm
    // Gets tab style.
    DWORD GetTabStyle() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec3dtabwnd__settabstyle.htm
    // Sets the tab style.
    DWORD SetTabStyle(DWORD dwTabStyle);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec3dtabwnd__enabletab.htm
    // Enables/disables tab.
    void EnableTab(CWnd* pWnd, BOOL bEnable = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec3dtabwnd__enabletab.htm
    // Enables/disables tab.
    void EnableTab(int nIndex, BOOL bEnable = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec3dtabwnd__setfontactivetab.htm
    // Sets an active tab's currrent font.
    BOOL SetFontActiveTab(CFont* pFont, BOOL bRedraw=TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec3dtabwnd__setfontinactivetab.htm
    // Sets an inactive tab's currrent font.
    BOOL SetFontInactiveTab(CFont* pFont, BOOL bRedraw=TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec3dtabwnd__getfontactivetab.htm
    // Gets the current font of an active tab.
    CFont* GetFontActiveTab();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec3dtabwnd__getfontinactivetab.htm
    // Gets the current font of an inactive tab.
    CFont* GetFontInactiveTab();

    // Attributes
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec3dtabwnd__istabenabled.htm
    // Determines if tab is enabled or disabled.
    BOOL IsTabEnabled(CWnd* pWnd);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec3dtabwnd__istabenabled.htm
    // Determines if tab is enabled or disabled.
    BOOL IsTabEnabled(int nIndex);
};
