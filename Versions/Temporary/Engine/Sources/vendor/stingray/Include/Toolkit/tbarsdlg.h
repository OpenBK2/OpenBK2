#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

class SECToolBarsBase {

};

class SECToolBarManager;

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarspage.htm

class SECToolBarsPage : public CPropertyPage, public SECToolBarsBase {
public:
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarspage__sectoolbarspage.htm
    // Constructs a SECToolBarsPage.
    SECToolBarsPage();

    void SetManager(SECToolBarManager* pManager);
};
