#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include <vector>

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase.htm

// One tab: the window it shows and the label above it. The toolkit returns this
// from AddTab and InsertTab as an opaque handle, so it is handed back as a
// pointer into the tab window's own list and stays valid until that tab is
// removed. Nothing in this editor keeps one.
struct SECTab {
    CWnd* pWnd = nullptr;
    CString strLabel;
    BOOL bEnabled = TRUE;
    void* pExtra = nullptr;
};

struct SECTabControlBase {

};

enum {
    SEC_TAB_DEFICON_CX,
    SEC_TAB_DEFICON_CY,
};

class SECTabWndBase : public CWnd {
public:
    // Operations
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__inserttab.htm
    // // Inserts a new tab with the given label before the currently active tab.
    virtual SECTab* InsertTab(CWnd* pWnd, int nIndex, LPCTSTR lpszLabel);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__inserttab.htm
    // // Inserts a new tab with the given label before the currently active tab.
    virtual SECTab* InsertTab(CRuntimeClass* pViewClass, int nIndex, LPCTSTR lpszLabel, CCreateContext* pContext = NULL, UINT nID = -1);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__addtab.htm
    // Appends a new tab to the end of the existing array of tabs.
    virtual SECTab* AddTab(CWnd* pWnd, LPCTSTR lpszLabel);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__addtab.htm
    // Appends a new tab to the end of the existing array of tabs.
    virtual SECTab* AddTab(CRuntimeClass* pViewClass, LPCTSTR lpszLabel, CCreateContext* pContext = NULL, UINT nID = -1);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__settabicon.htm
    // // Specifies the icon for a given tab.
    virtual void SetTabIcon(int nIndex, HICON hIcon);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__settabicon.htm
    // Specifies the icon for a given tab.
    virtual void SetTabIcon(int nIndex, UINT nIDIcon, int cx=SEC_TAB_DEFICON_CX, int cy=SEC_TAB_DEFICON_CY);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__settabicon.htm
    // Specifies the icon for a given tab.
    virtual void SetTabIcon(int nIndex, LPCTSTR lpszResourceName, int cx=SEC_TAB_DEFICON_CX, int cy=SEC_TAB_DEFICON_CY);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__removetab.htm
    // Removes the tab at the specifed index.
    virtual void RemoveTab(CWnd* pWnd);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__removetab.htm
    // Removes the tab at the specifed index.
    virtual void RemoveTab(int nIndex);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__renametab.htm
    // Gives a new name to the tab at the specified index.
    virtual void RenameTab(CWnd* pWnd, LPCTSTR lpszLabel);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__renametab.htm
    // Gives a new name to the tab at the specified index.
    virtual void RenameTab(int nIndex, LPCTSTR lpszLabel);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__activatetab.htm
    // Causes the specified tab to become the active tab.
    virtual BOOL ActivateTab(CWnd* pWnd, int nIndex);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__activatetab.htm
    // Causes the specified tab to become the active tab.
    virtual BOOL ActivateTab(CWnd* pWnd);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__activatetab.htm
    // Causes the specified tab to become the active tab.
    virtual BOOL ActivateTab(int nIndex);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__clearselection.htm
    // All tabs are marked as unselected.
    virtual void ClearSelection();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__scrolltotab.htm
    // The specified tab is scrolled into view.
    virtual void ScrollToTab(CWnd* pWnd);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__scrolltotab.htm
    // The specified tab is scrolled into view.
    virtual void ScrollToTab(int nIndex);

    // Queries
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__gettabcount.htm
    // Returns the number of tabs
    int GetTabCount() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__gettabinfo.htm
    // Returns information about the tab with the supplied index.
    BOOL GetTabInfo(int nIndex, LPCTSTR& lpszLabel, BOOL& bSelected, CWnd*& pWnd, void*& pExtra) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__getactivetab.htm
    // Returns the index of the tab which is associated with the specified window.
    BOOL FindTab(const CWnd* const pWnd, int& nTab) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__getactivetab.htm
    // If any tab is active, return its associated window.
    BOOL GetActiveTab(CWnd*& pWnd) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__getactivetab.htm
    // If any tab is active, return its index.
    BOOL GetActiveTab(int& nIndex) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__tabexists.htm
    // Checks for the existence of the tab associated with the specified window.
    BOOL TabExists(CWnd* pClient) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__tabexists.htm
    // Checks for the existence of the tab with the supplied index.
    BOOL TabExists(int nTab) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase__gettabcontrol.htm
    // Returns a pointer to the SECTabControlBase object associated with the tab window.
    const SECTabControlBase* GetTabControl() const;

    virtual ~SECTabWndBase();

protected:
    // The tab strip. The toolkit draws its own, which is what SEC3DTabWnd's
    // styles select between and what makes the tabs draggable; this holds a
    // common control instead and lays the pages out under it. That is the one
    // place this class gives a poorer answer than the toolkit would.
    CTabCtrl m_wndTabs;
    // Owned. Pointers rather than values so a SECTab handed back by AddTab
    // survives the list growing.
    std::vector<SECTab*> m_tabs;
    int m_nActive = -1;

    // Create the strip once this window has one to be a child of.
    BOOL EnsureTabControl();
    // Put the strip across the top and the active page under it.
    void LayoutTabs();
    // Show only the active tab's window, sized to the display area.
    void ShowActivePage();
    int IndexOf(const CWnd* pWnd) const;

    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()
};
