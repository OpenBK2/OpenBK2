#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include "sbarmgr.h"

struct SECGripperInfo {

};

struct SECDockContext {

};

// https://help.perforce.com/stingray/11/html/otug/8-7.html#871
// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar.htm

class SECControlBar : public CControlBar {
public:
    // Construction
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__seccontrolbar.htm
    // Constructs an SECControlBar object.
    SECControlBar();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__create.htm
    // Creates a control bar and attaches it to an SECControlBar object.
    virtual BOOL Create(CWnd* pParentWnd);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__create.htm
    // Creates a control bar and attaches it to an SECControlBar object.
    virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, UINT nID, DWORD dwStyle, DWORD dwExStyle, const RECT& rect, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__create.htm
    // Creates a control bar and attaches it to an SECControlBar object.
    virtual BOOL Create(CWnd* pParentWnd, LPCTSTR lpszWindowName, DWORD dwStyle, DWORD dwExStyle, UINT nID, CCreateContext* pContext = NULL);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__getinsiderect.htm
    // Retrieves attributes associated with control bar’s inside rectangle.
    virtual void GetInsideRect(CRect& rectInside) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__ismdichild.htm
    // Determines whether control bar has been re-parented by an MDI child window.
    BOOL IsMDIChild() const;

    // Attributes
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__m_boptimizedredrawenabled.htm
    // TRUE if optimized redraw is in effect.
    static BOOL m_bOptimizedRedrawEnabled;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__getoptimizeredrawenabled.htm
    // Cross-dll accessor to get m_bOptimizedRedrawEnabled state.
    static BOOL GetOptimizeRedrawEnabled();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__setoptimizedredrawenabled.htm
    // Cross-dll accessor to set m_bOptimizedRedrawEnabled state.
    static void SetOptimizedRedrawEnabled(BOOL bOptimize);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__getbarsizepos.htm
    // Access function to get the position of a bar.
    BOOL GetBarSizePos(int& nRow,int& nCol);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__getbarsizepos.htm
    // Access function to get the position of a bar with the ID.
    BOOL GetBarSizePos(int& nRow,int& nCol,int& nDockbarID);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__getbarsizepos.htm
    // Access function to get the position of a bar with the ID, height, and percentage width.
    BOOL GetBarSizePos(int& nRow,int& nCol,int& nDockbarID,float& fPctWidth,int& nHeight);

    // Public data members
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__m_szdockhorz.htm
    // Dimensions when docked horizontally
    CSize m_szDockHorz;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__m_ptdockhorz.htm
    // Location when docked horizontally.
    CPoint m_ptDockHorz;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__m_szdockvert.htm
    // Dimensions when docked vertically.
    CSize m_szDockVert;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__m_szfloat.htm
    // Dimensions when floating.
    CSize m_szFloat;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__m_dwmrudockingstate.htm
    // Remember docking state when “Allow Docking” unchecked.
    DWORD m_dwMRUDockingState = 0;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__m_fpctwidth.htm
    // The percentage of the dock bar’s width this control bar occupies.
    float m_fPctWidth = 0.0f;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__m_fdockedpctwidth.htm
    // The percentage of the dock bar’s width this control bar occupies.
    float m_fDockedPctWidth = 0.0f;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__m_dwexstyle.htm
    // Extended style bits.
    DWORD m_dwExStyle = 0;
protected:
    // Protected data members
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__m_rcborderspace.htm
    // White space around bar used for dragging.
    CRect m_rcBorderSpace;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__m_pmanager.htm
    // Control bar manager.
    SECControlBarManager* m_pManager = nullptr;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__m_gripperinfo.htm
    // Gripper information.
    SECGripperInfo m_GripperInfo;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__m_rcgripperclosebutton.htm
    // Gripper close button.
    CRect m_rcGripperCloseButton;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__m_rcgripperexpandbutton.htm
    // Expand button
    CRect m_rcGripperExpandButton;
public:
    // Operations
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__enabledocking.htm
    // Enables control bar docking.
    void EnableDocking(DWORD dwDockStyle);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__newdockcontext.htm
    // This member is called by EnableDocking() to instantiate a derivative of SECDockContext
    virtual SECDockContext * NewDockContext();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__setexbarstyle.htm
    // Set the extended bar style.
    virtual void SetExBarStyle(DWORD dwExStyle, BOOL bAutoUpdate=FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__modifybarstyleex.htm
    // Called to give control bars derived from SECControlBar a chance to modify
    virtual void ModifyBarStyleEx(DWORD dwRemove, DWORD dwAdd, BOOL bAutoUpdate=FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__getuniquebarid.htm
    // Static utility function to derive a unique controlbar id
    static UINT GetUniqueBarID(CFrameWnd* pMainWnd, UINT nBaseID=0x100);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__verifyuniquebarids.htm
    // Static utility function to verify all existing controlbars have unique ids.
    static BOOL VerifyUniqueBarIds(CFrameWnd* pFrameWnd);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__verifyuniquespecificbarid.htm
    // Static utility function to verify a specific controlbar id is unique
    static BOOL VerifyUniqueSpecificBarID(CFrameWnd* pFrameWnd, UINT nBarID);

    // Overridable
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__calcdynamiclayout.htm
    // Return the size of a dockable bar
    virtual CSize CalcDynamicLayout(int nLength, DWORD dwMode);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__onbarbegindock.htm
    // Virtual function that can be overridden to handle special requirements before a bar is docked.
    virtual void OnBarBeginDock();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__onbarenddock.htm
    // Virtual function that can be overridden to handle special requirements after a bar is docked.
    virtual void OnBarEndDock();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__onbarbeginfloat.htm
    // Virtual function that can be overridden to handle special requirements before a bar is floated.
    virtual void OnBarBeginFloat();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__onbarendfloat.htm
    // Virtual function that can be overridden to handle special requirements after a bar is floated.
    virtual void OnBarEndFloat();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__onbarbeginmdifloat.htm
    // Virtual function that can be overridden to handle special requirements before a bar is floated as a MDI child window.
    virtual void OnBarBeginMDIFloat();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__onbarendmdifloat.htm
    // Virtual function that can be overridden to handle special requirements after a bar is floated as a MDI child window.
    virtual void OnBarEndMDIFloat();

    // Implementation
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__ongripperclose.htm
    // Gripper button callback. Return FALSE to abort closing.
    virtual BOOL OnGripperClose();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccontrolbar__ongripperexpand.htm
    // Gripper button callbacks. Return FALSE to abort expansion.
    virtual BOOL OnGripperExpand();

    void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
};
