#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectabwndbase.htm

struct SECTab {

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
};
