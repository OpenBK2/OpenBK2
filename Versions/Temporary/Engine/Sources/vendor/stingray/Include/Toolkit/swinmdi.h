#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include "swsmgrex.h"
#include "sbarmgr.h"
#include "tmenufrm.h"

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdichildwnd.htm

class SECMDIChildWnd : public CMDIChildWnd {
public:
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdichildwnd__swapmenu.htm
    // Switches the menu on the menubar when running with a cool look menubar.
    void SwapMenu(UINT nID);
};

enum AlignCaption {
    acLeft,
    acCenter,
    acRight,
};

class SECMDIFrameWnd : public CMDIFrameWnd {
public:
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdiframewnd__enablecontextlistmode.htm
    // Enables or disables the control bar context list mode.
    void EnableContextListMode(BOOL bEnable = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdiframewnd__dockcontrolbarex.htm
    // Similar to DockControlBar but gives greater control over how and where the control bar will
    virtual void DockControlBarEx(CControlBar* pBar, UINT nDockBarID = 0,int nCol = 0, int nRow = 0, float fPctWidth = (float)1.0, int nHeight = 150);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdiframewnd__redockcontrolbar.htm
    // Converts the control bar back to a normal docking or floating control bar.
    void ReDockControlBar(CControlBar* pBar, CDockBar* pDockBar, LPCRECT lpRect = NULL);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdiframewnd__floatcontrolbarinmdichild.htm
    // Converts the control bar to an MDI child window.
    virtual void FloatControlBarInMDIChild(CControlBar* pBar, CPoint point, DWORD dwStyle = CBRS_ALIGN_TOP);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdiframewnd__enablecustomcaption.htm
    // Enables or disables gradient caption rendering.
    BOOL EnableCustomCaption(BOOL bEnable, BOOL bRedraw = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdiframewnd__forcecaptionredraw.htm
    // Forces a redraw of the caption bar.
    void ForceCaptionRedraw();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdiframewnd__createcaptionappfont.htm
    // Creates the caption text font for the application name
    virtual void CreateCaptionAppFont(CFont& font);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdiframewnd__createcaptiondocfont.htm
    // Creates the caption text font for the document name
    virtual void CreateCaptionDocFont(CFont& font);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdiframewnd__drawcaptiontext.htm
    // Draws the text on the caption bar
    virtual void DrawCaptionText();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdiframewnd__setcaptiontextalign.htm
    // Modifies the alignment of the caption text relative to the caption bar.
    void SetCaptionTextAlign(AlignCaption ac, BOOL bRedraw = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdiframewnd__swapmenu.htm
    // Switches the menu on the menubar when running with a cool look menubar.
    void SwapMenu(UINT nID);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdiframewnd__loadadditionalmenus.htm
    // Loads additional menus that are not created by the document templates.
    BOOL LoadAdditionalMenus(UINT nCount, UINT nIDMenu, ...);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdiframewnd__enableolecontainmentmode.htm
    // Sets CBRS_HIDE_INPLACE for all control bars. Call when used with Ole Containers.
    void EnableOleContainmentMode();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdiframewnd__onextendcontextmenu.htm
    // Traps WM_EXTENDCONTEXTMENU message sent by the SECControlBar class after it creates a context menu. Trap this message if you wish to customize the default context menu.
    afx_msg LRESULT OnExtendContextMenu(WPARAM wParam, LPARAM lParam);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secmdiframewnd__initworkspacemgrex.htm
    // Initialize the extended workspace manager.
    virtual SECWorkspaceManagerEx* InitWorkspaceMgrEx(const CString& strAppKey, BOOL bRegistryMode=FALSE, CRuntimeClass* pWSClass= RUNTIME_CLASS(SECWorkspaceManagerEx), BOOL bSectionKey=FALSE);
    void EnableBmpMenus();

    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__m_uitextalign.htm
    UINT m_uiTextAlign = 0;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__m_bhandlecaption.htm
    BOOL m_bHandleCaption = FALSE;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__m_bactive.htm
    BOOL m_bActive = FALSE;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__m_pcontrolbarmanager.htm
    SECControlBarManager * m_pControlBarManager = nullptr;

    // m_pMenuBar is a member of SECFrameWnd so you do not have to create it yourself
    SECMenuBar * m_pMenuBar = nullptr;
};
