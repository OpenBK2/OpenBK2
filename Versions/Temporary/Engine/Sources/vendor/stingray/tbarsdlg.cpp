#include "Toolkit/tbarsdlg.h"
#include "Toolkit/tbarmgr.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


BEGIN_MESSAGE_MAP(SECToolBarsPage, CPropertyPage)
    ON_CONTROL(CLBN_CHKCHANGE, IDC_SEC_LIST, &SECToolBarsPage::OnCheckChange)
    ON_LBN_SELCHANGE(IDC_SEC_LIST, &SECToolBarsPage::OnSelChange)
    ON_BN_CLICKED(IDC_SEC_TOOLTIPS, &SECToolBarsPage::OnTooltips)
    ON_BN_CLICKED(IDC_SEC_COOL_LOOK, &SECToolBarsPage::OnCoolLook)
    ON_BN_CLICKED(IDC_SEC_LARGE_BTNS, &SECToolBarsPage::OnLargeBtns)
    ON_BN_CLICKED(IDC_SEC_RESET, &SECToolBarsPage::OnReset)
END_MESSAGE_MAP()


// The template lives in the toolkit's own resources, which are compiled into
// whichever module included secres.rc -- ED_B2_M1 and the executable, not
// necessarily the one whose resource handle is current when this is
// constructed. AfxFindResourceHandle walks the module chain and finds it
// wherever it landed; without this the page can construct in a module that has
// no such dialog and fail at DoModal with a CResourceException.
SECToolBarsPage::SECToolBarsPage() : CPropertyPage(IDD) {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    m_psp.hInstance = AfxFindResourceHandle(MAKEINTRESOURCE(IDD), RT_DIALOG);
}

void SECToolBarsPage::SetManager(SECToolBarManager* pManager) {
    spdlog::debug("{} this={} pManager={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pManager));
    m_pManager = pManager;
}

void SECToolBarsPage::DoDataExchange(CDataExchange* pDX) {
    CPropertyPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_SEC_LIST, m_wndList);
}

BOOL SECToolBarsPage::OnInitDialog() {
    CPropertyPage::OnInitDialog();

    // The list is LBS_OWNERDRAWFIXED | LBS_HASSTRINGS in the template, which is
    // what CCheckListBox needs; the check style has to be set before anything
    // is added, because that is what fixes the item height.
    m_wndList.SetCheckStyle(BS_AUTOCHECKBOX);

    if (m_pManager != nullptr) {
        for (const SECToolBarManager::ToolBarDef &def : m_pManager->GetToolBarDefs()) {
            const int nItem = m_wndList.AddString(def.strTitle);
            if (nItem < 0) {
                continue;
            }
            m_wndList.SetCheck(nItem, m_pManager->IsToolBarVisible(def.nID) ? BST_CHECKED
                                                                           : BST_UNCHECKED);
            m_barIDs.push_back(def.nID);
        }
        CheckDlgButton(IDC_SEC_TOOLTIPS, m_pManager->ToolTipsEnabled() ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(IDC_SEC_COOL_LOOK, m_pManager->CoolLookEnabled() ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(IDC_SEC_LARGE_BTNS, m_pManager->LargeBtnsEnabled() ? BST_CHECKED : BST_UNCHECKED);
    }

    if (!m_barIDs.empty()) {
        m_wndList.SetCurSel(0);
        OnSelChange();
    }

    // New... makes an empty toolbar, and the only way to put anything on one is
    // to drag commands from the Commands page, which is not implemented. A
    // button that can only produce an empty bar is worse than one that says it
    // cannot, so it says so.
    if (CWnd *pNew = GetDlgItem(IDC_SEC_NEW)) {
        pNew->EnableWindow(FALSE);
    }

    return TRUE;
}

// A check ticked or cleared. CCheckListBox reports which item through
// GetCurSel, which it has already moved to the item whose box was hit.
void SECToolBarsPage::OnCheckChange() {
    const int nItem = m_wndList.GetCurSel();
    if (nItem < 0 || static_cast<size_t>(nItem) >= m_barIDs.size() || m_pManager == nullptr) {
        return;
    }
    const BOOL bShow = m_wndList.GetCheck(nItem) == BST_CHECKED;
    spdlog::debug("SECToolBarsPage::OnCheckChange: toolbar {} -> {}", m_barIDs[nItem], bShow ? "shown" : "hidden");
    m_pManager->ShowToolBar(m_barIDs[nItem], bShow);
}

// The name box follows the selection. It is disabled in the template and stays
// that way: renaming means a user toolbar, and there are none.
void SECToolBarsPage::OnSelChange() {
    const int nItem = m_wndList.GetCurSel();
    CString strName;
    if (nItem >= 0) {
        m_wndList.GetText(nItem, strName);
    }
    SetDlgItemText(IDC_SEC_NAME, strName);
}

void SECToolBarsPage::OnTooltips() {
    if (m_pManager != nullptr) {
        const BOOL bOn = IsDlgButtonChecked(IDC_SEC_TOOLTIPS) == BST_CHECKED;
        m_pManager->EnableToolTips(bOn);
        // Flyby help is the same switch in this dialog, as it was in the
        // toolkit: the tooltip text and the status bar prompt come from the
        // same string and are meant to appear together.
        m_pManager->EnableFlyBy(bOn);
    }
}

void SECToolBarsPage::OnCoolLook() {
    if (m_pManager != nullptr) {
        m_pManager->EnableCoolLook(IsDlgButtonChecked(IDC_SEC_COOL_LOOK) == BST_CHECKED);
    }
}

void SECToolBarsPage::OnLargeBtns() {
    if (m_pManager != nullptr) {
        m_pManager->EnableLargeBtns(IsDlgButtonChecked(IDC_SEC_LARGE_BTNS) == BST_CHECKED);
    }
}

// Reset applies to the selected toolbar, which is what the retail dialog did --
// the button sits under the list, not under the options.
void SECToolBarsPage::OnReset() {
    const int nItem = m_wndList.GetCurSel();
    if (nItem < 0 || static_cast<size_t>(nItem) >= m_barIDs.size() || m_pManager == nullptr) {
        return;
    }
    m_pManager->ResetToolBar(m_barIDs[nItem]);
    m_wndList.SetCheck(nItem, m_pManager->IsToolBarVisible(m_barIDs[nItem]) ? BST_CHECKED
                                                                           : BST_UNCHECKED);
}
