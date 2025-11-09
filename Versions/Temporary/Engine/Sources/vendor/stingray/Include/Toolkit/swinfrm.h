#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include "swsmgrex.h"
#include "sbarmgr.h"
#include "tmenufrm.h"

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd.htm
// The SECFrameWnd class derives from CFrameWnd

class SECFrameWnd : public CFrameWnd {
public:
    // Construction
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__secframewnd.htm
    // Constructor for the SECFrameWnd class.
    SECFrameWnd();

    // Attributes
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__getactivestate.htm
    // Returns the activation state for the window.
    BOOL GetActiveState();

    // Operations
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__enabledocking.htm
    // Enables docking for the frame.
    void EnableDocking(DWORD dwDockStyle, DWORD dwDockStyleEx = 0);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__createnewdockbar.htm
    // Create a new dockbar
    virtual CDockBar* CreateNewDockBar();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__enablecontextlistmode.htm
    // Enables or disables the control bar context list mode.
    void EnableContextListMode(BOOL bEnable = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__floatcontrolbar.htm
    // Remove a control bar from it's dockbar and float it, or move a floating bar to
    virtual void FloatControlBar(CControlBar* pBar, CPoint point, DWORD dwStyle = CBRS_ALIGN_TOP);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__dockcontrolbarex.htm
    // Dock a control bar with extended information.
    virtual void DockControlBarEx(CControlBar* pBar, UINT nDockBarID = 0,int nCol = 0, int nRow = 0, float fPctWidth = (float)1.0, int nHeight = 150);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__createcaptionappfont.htm
    // Creates the caption text font for the application name
    virtual void CreateCaptionAppFont(CFont& font);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__createcaptiondocfont.htm
    // Creates the caption text font for the document name
    virtual void CreateCaptionDocFont(CFont& font);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__drawcaptiontext.htm
    // Draws the text on the caption bar
    virtual void DrawCaptionText();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__swapmenu.htm
    // Switches the menu on the menubar when running with a cool look menubar.
    void SwapMenu(UINT nID);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__hasmenubar.htm
    // Returns TRUE if a menubar is present, FALSE if CMenu.
    BOOL HasMenuBar();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__getmenubar.htm
    // Gets a pointer to the menu bar.
    SECMenuBar* GetMenuBar();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__setmenubar.htm
    // Set the MenuBar for this frame.
    void SetMenuBar(SECMenuBar* pMenuBar);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__getmenu.htm
    // Returns a pointer to the currently active menu.
    CMenu* GetMenu() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__initworkspacemgrex.htm
    // Initialize the extended workspace manager.
    virtual SECWorkspaceManagerEx* InitWorkspaceMgrEx(const CString& strAppKey, BOOL bRegistryMode=FALSE, CRuntimeClass* pWSClass= RUNTIME_CLASS(SECWorkspaceManagerEx), BOOL bSectionKey=FALSE);
    void EnableBmpMenus();
protected:
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__m_uitextalign.htm
    // DrawText alignment flags for caption
    UINT m_uiTextAlign = 0;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__m_bhandlecaption.htm
    // If custom caption drawing is enabled, this will be the caption handle.
    BOOL m_bHandleCaption = FALSE;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__m_bactive.htm
    BOOL m_bActive = FALSE;
    // Activation status (WM_ACTIVATE status)
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secframewnd__m_pcontrolbarmanager.htm
    // Pointer to the ControlBarManager
    SECControlBarManager * m_pControlBarManager = nullptr;

    // m_pMenuBar is a member of SECFrameWnd so you do not have to create it yourself
    SECMenuBar * m_pMenuBar = nullptr;
};
