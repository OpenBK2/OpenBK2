#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include <vector>

#include "secres.h"

class SECToolBarsBase {

};

class SECToolBarManager;

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarspage.htm

//! Page one of Tools -> Customize: which of the frame's toolbars are shown,
//! and the three display options that apply to all of them.
//!
//! The template is IDD_SEC_TOOLBARS_PAGE, recovered from the retail editor --
//! see secres.rc. Until it existed this page had no template at all, so
//! CPropertyPage threw CResourceException and the whole sheet came up as "A
//! required resource was unavailable".
class SECToolBarsPage : public CPropertyPage, public SECToolBarsBase {
public:
    enum { IDD = IDD_SEC_TOOLBARS_PAGE };

    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sectoolbarspage__sectoolbarspage.htm
    // Constructs a SECToolBarsPage.
    SECToolBarsPage();

    void SetManager(SECToolBarManager* pManager);

protected:
    BOOL OnInitDialog() override;
    void DoDataExchange(CDataExchange* pDX) override;

    //! A check ticked or cleared in the toolbar list: show or hide that bar.
    afx_msg void OnCheckChange();
    //! A different toolbar selected: the name box follows it.
    afx_msg void OnSelChange();
    afx_msg void OnTooltips();
    afx_msg void OnCoolLook();
    afx_msg void OnLargeBtns();
    afx_msg void OnReset();

    DECLARE_MESSAGE_MAP()

    //! The manager whose toolbars this page lists.
    SECToolBarManager* m_pManager = nullptr;

    CCheckListBox m_wndList;

    //! Toolbar ids, in the order they were put in the list, so a selection can
    //! be turned back into a bar.
    std::vector<UINT> m_barIDs;
};
