#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

enum {
    IDS_TOOLBAR_CUSTOMIZE,
    IDD_TOOLBAR_CUSTOMIZE,
};

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarsheet.htm

class SECToolBarSheet: public CPropertySheet {
public:
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarsheet__sectoolbarsheet.htm
    SECToolBarSheet(UINT nIDCaption = IDS_TOOLBAR_CUSTOMIZE, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
};

class SECToolBarManager;

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarcmdpage.htm

class SECToolBarCmdPage : public CPropertyPage {
public:
    enum { IDD = IDD_TOOLBAR_CUSTOMIZE };
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarcmdpage__sectoolbarcmdpage.htm
    // Constructs an SECToolBarCmdPage object.
    SECToolBarCmdPage();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarcmdpage__sectoolbarcmdpage.htm
    // Constructs an SECToolBarCmdPage object.
    SECToolBarCmdPage(UINT nIDTemplate, UINT nIDCaption = 0);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarcmdpage__sectoolbarcmdpage.htm
    // Constructs an SECToolBarCmdPage object.
    SECToolBarCmdPage(LPCTSTR lpszTemplate, UINT nIDCaption = 0);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarcmdpage__setmanager.htm
    // Defines the toolbar manager.
    void SetManager(SECToolBarManager* pManager);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarcmdpage__definebtngroup.htm
    // Defines a button group, consisting of a title and an array of button IDs
    void DefineBtnGroup(LPCTSTR lpszTitle, int nBtnCount, UINT* lpBtnIDs);
    void DefineMenuGroup(LPCTSTR lpszTitle);
};
