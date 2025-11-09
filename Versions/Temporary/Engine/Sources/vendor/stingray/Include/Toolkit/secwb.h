#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include "swinfrm.h"
#include "swinmdi.h"

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworksheet.htm?Highlight=SECWorksheet

class SECWorksheet : public SECMDIChildWnd {

};


// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkbook.htm?Highlight=SECWorkbook

// SECWorkbook inherits most of its functionality from SECMDIFrameWnd

class SECWorkbook : public SECMDIFrameWnd {
public:
    // Overrideables
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkbook__addsheet.htm
    // Adds a new worksheet to the workbook
    virtual void AddSheet(SECWorksheet* pSheet);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkbook__removesheet.htm
    // Removes the specified worksheet from the workbook
    virtual void RemoveSheet(SECWorksheet* pSheet);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkbook__gettablabel.htm
    // Gets the label associated with the tab
    virtual const TCHAR* GetTabLabel(SECWorksheet* pSheet) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkbook__ondrawtab.htm
    // Draws a blank tab at the position specified
    virtual void OnDrawTab(CDC* pDC, SECWorksheet* pSheet);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkbook__ondrawtabiconandlabel.htm
    // Renders the icon and tab label on top of the blank tab drawn by OnDrawTab()
    virtual void OnDrawTabIconAndLabel(CDC* pDC, SECWorksheet* pSheet);

    // Operations
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkbook__setworkbookmode.htm
    // Toggles between Workbook and Normal modes
    void SetWorkbookMode(BOOL bEnabled = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkbook__setshowicons.htm
    // Sets display of icons
    void SetShowIcons(BOOL bShowIcons = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkbook__gettabicon.htm
    // Returns the icon to be drawn on the tab
    virtual HICON GetTabIcon(SECWorksheet* pSheet) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkbook__settabicon.htm
    // Sets an icon to be drawn on the tab
    virtual BOOL SetTabIcon(SECWorksheet* pSheet,HICON hIcon,BOOL bRedraw=TRUE);

    // Queries
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkbook__lookupsheet.htm
    // Looks up the specified worksheet in the workbook
    BOOL LookupSheet(SECWorksheet* pSheet, int& nIndex);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkbook__getworksheet.htm
    // Returns a pointer to the worksheet specified in the index
    SECWorksheet* GetWorksheet(int nSheet) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/secworkbook__getsheetcount.htm
    // Returns a count of the total number of worksheets
    int GetSheetCount() const;

    UINT m_nIDCurMenuRsrc = 0;
};
