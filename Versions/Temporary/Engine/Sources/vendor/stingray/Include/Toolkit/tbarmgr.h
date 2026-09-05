#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include "sbarmgr.h"
#include "tbarcust.h"

#include <vector>

struct SECBtnMapEntry {
    int a, b, c, d;
};

#define BEGIN_BUTTON_MAP(m) static const SECBtnMapEntry m[] = {
#define END_BUTTON_MAP() };
#define TWOPART_BUTTON(a, b, c ,d) { (a), (b), (c), (d) },


// https://help.perforce.com/stingray/11/html/otug/8-9.html
// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__m_dwexstyle.htm?Highlight=CBRS_EX_COOLBORDERS

enum {
    CBRS_EX_STDCONTEXTMENU,
    // Control bar is given the standard context menu (i.e., Allow docking, and Show/Hide control bar menu items).
    CBRS_EX_STRETCH_ON_SIZE,
    // When the control bar is resized, all child windows are stretched and repositioned so as to preserve the proportions of the control bar. If you require another form of resize handling, be sure to omit this extended style from the Create call and override SECControlBar::OnSize.
    CBRS_EX_DRAWBORDERS,
    // Draw a border around the bar.
    CBRS_EX_BORDERSPACE,
    // Leave border space for ease of dragging.
    CBRS_EX_ALLOW_MDI_FLOAT,
    // Control bar can be re-parented by an MDI child window.
    CBRS_EX_SIZE_TO_FIT,
    // Size the (single) child to fit.
    CBRS_EX_UNIDIRECTIONAL,
    // The control bar can be sized in one dimension at a time (no diagonal sizing allowed). In addition, a change in height dictates a new width and vice versa. A toolbar is an example of a unidirectional control bar.
    CBRS_EX_COOLBORDERS,
    // Floating buttons, no border.
    CBRS_EX_GRIPPER,
    // Draw the dragging gripper.
    CBRS_EX_GRIPPER_CLOSE,
    // Draw the close button on gripper.
    CBRS_EX_GRIPPER_EXPAND,
    // Expand/contract control bar button.
    CBRS_EX_COOL = CBRS_EX_COOLBORDERS | CBRS_EX_GRIPPER | CBRS_EX_GRIPPER_CLOSE | CBRS_EX_GRIPPER_EXPAND,
    // Control bar will have the “cool look” – a flat, painted look similar to the control bars seen in Microsoft Developer Studio. These extended style options allow you to customize the “cool look” for your application. By default, customizable toolbars are CBRS_EX_COOLBORDERS or CBRS_EX_GRIPPER; all other control bars are CBRS_EX_COOL. NOTE: As of Objective Toolkit 5.0, gripper requires coolborders, and close requires gripper. Also, the gripper drawing code has been virtualized, so you can easily plug in your own gripper (or modify the existing gripper) with just one or two overrides.
};

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager.htm?Highlight=SECToolBarManager

class SECToolBarManager : public SECControlBarManager {
public:
    // Construction/Initialization
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__sectoolbarmanager.htm
    SECToolBarManager();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__sectoolbarmanager.htm

    SECToolBarManager(CFrameWnd* pFrameWnd, CFrameWnd* pOwnerFrame=NULL);
    // Operations
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__definedefaulttoolbar.htm
    // Define a customizable toolbar default state.
    void DefineDefaultToolBar(UINT nID, const CString& strTitle, UINT nBtnCount, UINT* lpBtnIDs, DWORD dwAlignment = CBRS_ALIGN_ANY, UINT nDockBarID = AFX_IDW_DOCKBAR_TOP, UINT nDockNextToID = NULL, BOOL bDocked = TRUE, BOOL bVisible = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__definedefaulttoolbar.htm
    // Define a customizable toolbar default state based on a toolbar resource.
    void DefineDefaultToolBar(UINT nID, const CString& strTitle, UINT nToolbarID, UINT& nRetButtonCount, UINT*& pRetButtonArray, DWORD dwAlignment = CBRS_ALIGN_ANY, UINT nDockBarID = AFX_IDW_DOCKBAR_TOP, UINT nDockNextToID = NULL, BOOL bDocked = TRUE, BOOL bVisible = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__istoolbarcommand.htm
    // Returns TRUE (with the btn rect. in question) if we are currently processing a button command.
    BOOL IsToolBarCommand(CRect& rect) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__toolbarunderrect.htm
    // Returns the toolbar under the given window rect.
    virtual SECCustomToolBar* ToolBarUnderRect(const CRect& rect) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__toolbarfromid.htm
    // Returns the toolbar pointer for a given default toolbar ID.
    virtual SECCustomToolBar* ToolBarFromID(const UINT nToolBarID) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__createusertoolbar.htm
    // Creates a new 'user' toolbar
    SECCustomToolBar* CreateUserToolBar(LPCTSTR lpszTitle = NULL);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__loadtoolbarresource.htm
    // Loads the toolbar resource (to be shared by all toolbars)
    BOOL LoadToolBarResource(LPCTSTR lpszStdBmpName, LPCTSTR lpszLargeBmpName = NULL);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__loadtoolbarresource.htm
    // Loads the toolbar resource (to be shared by all toolbars)
    BOOL LoadToolBarResource(UINT nIDStdBmp, UINT nIDLargeBmp = 0);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__loadtoolbarresource.htm
    // Loads the toolbar resource (to be shared by all toolbars)
    BOOL LoadToolBarResource();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__addtoolbarresource.htm
    // Appends a toolbar resource to the current LoadToolBarResource image
    BOOL AddToolBarResource(LPCTSTR lpszStdBmpName, LPCTSTR lpszLargeBmpName = NULL);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__addtoolbarresource.htm
    // Appends a toolbar resource to the current LoadToolBarResource image
    BOOL AddToolBarResource(UINT nIDStdBmp, UINT nIDLargeBmp = 0);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__addbitmapresource.htm
    // Append a new bitmap resource for use with the available custom toolbar buttons
    BOOL AddBitmapResource(LPCTSTR lpszStdBmpName, LPCTSTR lpszLargeBmpName, const UINT* lpIDArray, UINT nIDCount);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__addbitmapresource.htm
    // Append a new bitmap resource for use with the available custom toolbar buttons
    BOOL AddBitmapResource(UINT nIDstdBmpName, UINT nIDLargeBmpName, const UINT* lpIDArray, UINT nIDCount);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__addbitmap.htm
    // Append a new bitmap for use with the available custom toolbar buttons
    BOOL AddBitmap(HBITMAP hBmpSmall, HBITMAP hBmpLarge, const UINT* lpIDArray, UINT nIDCount);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__execviewtoolbarsdlg.htm
    // Runs the view toolbars dialog.
    int ExecViewToolBarsDlg();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__informbtns.htm
    // Passes notification through to all buttons of nID
    void InformBtns(UINT nID, UINT nCode, void* pData);

    // State Configuration
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__enabletooltips.htm
    // Enable tooltips for all custom toolbars
    void EnableToolTips(BOOL bEnable=TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__enableflyby.htm
    // Enable flyby help for all custom toolbars
    void EnableFlyBy(BOOL bEnable=TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__tooltipsenabled.htm
    // Return current tooltip state
    BOOL ToolTipsEnabled() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__flybyenabled.htm
    // Return current flyby help state
    BOOL FlyByEnabled() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__enablelargebtns.htm
    // Enable large button mode
    void EnableLargeBtns(BOOL bEnable=TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__largebtnsenabled.htm
    // Return current large button mode status
    BOOL LargeBtnsEnabled() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__enablecoollook.htm
    // Enable "cool look" mode
    void EnableCoolLook(BOOL bEnable=TRUE,DWORD dwExCoolLookStyles=CBRS_EX_COOLBORDERS|CBRS_EX_GRIPPER);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__coollookenabled.htm
    // Return current "cool look" mode
    BOOL CoolLookEnabled() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__setbuttonmap.htm
    // Intialize the toolbar button map information across all toolbars
    //! The toolbar image for a command, for anything that wants to draw
    //! a button's face somewhere else -- the menus, through
    //! SECMDIFrameWnd::EnableBmpMenus. False when the command is on no
    //! toolbar, or on one that has not been built yet.
    BOOL GetButtonImage(UINT nID, HIMAGELIST *phImageList, int *pnImage) const;
    //! The menu resources SetMenuInfo was given, in the order it got them.
    //! They are what the Customize dialog offers as draggable commands.
    const std::vector<UINT>& GetMenuIDs() const;
    void SetButtonMap(const SECBtnMapEntry* pMap);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__getbuttonmap.htm
    // Get the button map information
    const SECBtnMapEntry* GetButtonMap() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarmanager__setdefaultdockstate.htm
    // Load a default toolbar configuration state.
    virtual void SetDefaultDockState();
    BOOL SetMenuInfo(int nCount, UINT nIDMenu, ...);
    void LoadState(const CString &);
    void SaveState(const CString &);

    ~SECToolBarManager() override;

    // What DefineDefaultToolBar was told about one toolbar, held until there is
    // somewhere to put it. See CreateBars for why it cannot be built at once.
    struct ToolBarDef {
        UINT nID = 0;
        CString strTitle;
        std::vector<UINT> btnIDs;
        DWORD dwAlignment = CBRS_ALIGN_ANY;
        UINT nDockBarID = AFX_IDW_DOCKBAR_TOP;
        UINT nDockNextToID = 0;
        BOOL bDocked = TRUE;
        BOOL bVisible = TRUE;
        // The bitmap this bar's button faces come from, paired by order of
        // definition with the resources AddToolBarResource was handed.
        UINT nBitmapID = 0;
        SECCustomToolBar* pBar = nullptr;
    };

private:
    // Build a bar per definition, dock it and show or hide it. Called late,
    // from LoadState and SetDefaultDockState, and idempotent.
    void CreateBars();
    ToolBarDef* FindDef(UINT nID);

    std::vector<ToolBarDef> m_defs;

    //! Menu resource ids from SetMenuInfo. Kept, not acted on: the only
    //! thing that would act on them is the Customize dialog.
    std::vector<UINT> m_menuIDs;
    // Bitmap resource ids, in the order they were loaded.
    std::vector<UINT> m_bitmaps;

    const SECBtnMapEntry* m_pButtonMap = nullptr;
    BOOL m_bToolTips = FALSE;
    BOOL m_bFlyBy = FALSE;
    BOOL m_bLargeBtns = FALSE;
    BOOL m_bCoolLook = FALSE;
    DWORD m_dwCoolLookStyles = 0;
    BOOL m_bBarsCreated = FALSE;
};
