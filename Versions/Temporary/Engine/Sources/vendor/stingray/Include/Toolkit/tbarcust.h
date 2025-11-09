#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include "sbarcore.h"

#include <vector>

struct Wrapped {

};

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar.htm

// this class inherits from SECControlBar

class SECCustomToolBar : public SECControlBar {
public:
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__create.htm
    // Creates the child window for the customizable toolbar and attaches it to an SECCustomToolBar object.
    virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, UINT nID,DWORD dwStyle, DWORD dwExStyle, const RECT& rect, CWnd* pParentWnd = NULL,CCreateContext* pContext = NULL);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__createex.htm
    // Creates a customizable toolbar with extended style attributes.
    virtual BOOL CreateEx(DWORD dwExStyle, CWnd* pParentWnd, DWORD dwStyle = WS_VISIBLE | WS_CHILD | CBRS_TOP, UINT nID = AFX_IDW_TOOLBAR, LPCTSTR lpszTitle = NULL);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__setbarinfoex.htm
    // Saves the toolbar configuration
    virtual void SetBarInfoEx(SECControlBarInfo* pInfo, CFrameWnd* pFrameWnd);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__setbuttonstyle.htm
    // Sets the style of a button.
    void SetButtonStyle(int nIndex, UINT nStyle);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__getbuttonstyle.htm
    // Gets the style of a button.
    UINT GetButtonStyle(int nIndex) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__removebutton.htm
    // Removes a button from the toolbar
    virtual BOOL RemoveButton(int nIndex, BOOL bNoUpdate = FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__addbutton.htm
    // Adds a button to the toolbar.
    virtual void AddButton(int nIndex, int nID, BOOL bSeparator = FALSE,BOOL bNoUpdate = FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__getbtncount.htm
    // Returns number of buttons on toolbar.
    int GetBtnCount() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__inconfigmode.htm
    // Returns TRUE if toolbar is in customize mode.
    BOOL InConfigMode() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__inaltdragmode.htm
    // Returns TRUE when dragging with ALT key down (when not in customize mode).
    BOOL InAltDragMode() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__commandtoindex.htm
    // Returns the command ID for a given button index.
    int CommandToIndex(UINT nID) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__getitemid.htm
    // Returns the command ID for a given button index.
    UINT GetItemID(int nIndex) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__getcurbtn.htm
    // Returns the index of the currently active button.
    int GetCurBtn() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__idtobmpindex.htm
    // Converts a command ID to an index in the bitmap button.
    virtual int IDToBmpIndex(UINT nID, HBITMAP* lphBmp);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__loadtoolbar.htm
    // Loads a toolbar bitmap resource.
    BOOL LoadToolBar(LPCTSTR lpszResourceName);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__loadbitmap.htm
    // Loads a toolbar bitmap resource.
    BOOL LoadBitmap(UINT nIDResource, const UINT* lpIDArray, int nIDCount);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__setbuttons.htm
    // Sets the buttons on the current toolbar.
    virtual BOOL SetButtons(const UINT* lpIDArray, int nIDCount);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__getitemrect.htm
    // Returns the rect of the given button index.
    void GetItemRect(int nIndex, LPRECT lpRect) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__informbtns.htm
    // Passes notification through to all buttons of nID.
    void InformBtns(UINT nID, UINT nCode, void* pData, BOOL bPass = FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__balancewrap.htm
    // Balances the wrapping of the toolbar.
    virtual void BalanceWrap(int nRow, Wrapped* pWrap);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__getdragmode.htm
    // Returns TRUE if the default drag mode is add.
    virtual BOOL GetDragMode() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/seccustomtoolbar__acceptdrop.htm
    // Returns TRUE if the toolbar accepts dropped buttons.
    virtual BOOL AcceptDrop() const;

    struct Button {
        UINT m_nID;
    };
    std::vector<Button*> m_btns;
};
