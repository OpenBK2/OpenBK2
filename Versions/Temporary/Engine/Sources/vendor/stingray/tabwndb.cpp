#include "Toolkit/tabwndb.h"
#include "Toolkit/ot_dockingwindows.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


namespace {
// The strip is a child of the tab window and nothing else addresses it, so any
// id outside the ranges MFC reserves for control bars and panes will do.
const UINT ID_TAB_STRIP = 0x7A00;
}

BEGIN_MESSAGE_MAP(SECTabWndBase, CWnd)
    ON_WM_SIZE()
    ON_NOTIFY(TCN_SELCHANGE, ID_TAB_STRIP, &SECTabWndBase::OnTabSelChange)
END_MESSAGE_MAP()

SECTabWndBase::~SECTabWndBase() {
    for (SECTab *pTab : m_tabs) {
        delete pTab;
    }
    m_tabs.clear();
}

int SECTabWndBase::IndexOf(const CWnd* pWnd) const {
    for (size_t i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i]->pWnd == pWnd) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// The strip cannot be made until this window exists, and the editor adds tabs
// before that in at least one path, so every entry point that needs it asks
// here rather than assuming the constructor could have done it.
BOOL SECTabWndBase::EnsureTabControl() {
    if (GetSafeHwnd() == nullptr) {
        return FALSE;
    }
    if (m_wndTabs.GetSafeHwnd() != nullptr) {
        return TRUE;
    }
    if (!m_wndTabs.Create(WS_CHILD | WS_VISIBLE | TCS_HOTTRACK | TCS_FOCUSNEVER,
                          CRect(0, 0, 0, 0), this, ID_TAB_STRIP)) {
        spdlog::warn("SECTabWndBase::EnsureTabControl: the tab strip would not create");
        return FALSE;
    }
    m_wndTabs.SetFont(CFont::FromHandle(reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT))));
    return TRUE;
}

// Strip across the top, page filling what is left.
//
// The toolkit puts the tabs on whichever edge the TWS_TABS_ON_* style selects.
// Those constants are placeholders in this port -- consecutive small integers
// rather than the bit flags the toolkit defines -- so they cannot be tested for,
// and the strip always goes on top. The original editor's dump shows a strip 31
// pixels high, which is what a common control comes to at this font anyway.
void SECTabWndBase::LayoutTabs() {
    if (GetSafeHwnd() == nullptr || m_wndTabs.GetSafeHwnd() == nullptr) {
        return;
    }
    CRect rcClient;
    GetClientRect(&rcClient);
    if (rcClient.IsRectEmpty()) {
        return;
    }

    // ItemSize is per tab; the row height plus a small border is what the strip
    // needs. Asking the control what a whole row costs is more reliable than a
    // constant, since it follows the font.
    int nStrip = 0;
    if (m_wndTabs.GetItemCount() > 0) {
        CRect rcItem;
        m_wndTabs.GetItemRect(0, &rcItem);
        nStrip = rcItem.Height() + 4;
    }
    if (nStrip <= 0) {
        nStrip = 24;
    }
    if (nStrip > rcClient.Height()) {
        nStrip = rcClient.Height();
    }

    m_wndTabs.SetWindowPos(nullptr, rcClient.left, rcClient.top, rcClient.Width(), nStrip,
                           SWP_NOZORDER | SWP_NOACTIVATE);
    ShowActivePage();
}

void SECTabWndBase::ShowActivePage() {
    if (GetSafeHwnd() == nullptr) {
        return;
    }
    CRect rcClient;
    GetClientRect(&rcClient);
    CRect rcStrip(0, 0, 0, 0);
    if (m_wndTabs.GetSafeHwnd() != nullptr) {
        m_wndTabs.GetWindowRect(&rcStrip);
        ScreenToClient(&rcStrip);
    }
    const CRect rcPage(rcClient.left, rcStrip.bottom, rcClient.right, rcClient.bottom);

    for (size_t i = 0; i < m_tabs.size(); ++i) {
        CWnd *pWnd = m_tabs[i]->pWnd;
        if (pWnd == nullptr || pWnd->GetSafeHwnd() == nullptr) {
            continue;
        }
        const BOOL bActive = (static_cast<int>(i) == m_nActive);
        if (bActive && !rcPage.IsRectEmpty()) {
            pWnd->SetWindowPos(nullptr, rcPage.left, rcPage.top, rcPage.Width(), rcPage.Height(),
                               SWP_NOZORDER | SWP_NOACTIVATE);
        }
        pWnd->ShowWindow(bActive ? SW_SHOW : SW_HIDE);
    }
}

void SECTabWndBase::OnSize(UINT nType, int cx, int cy) {
    CWnd::OnSize(nType, cx, cy);
    LayoutTabs();
}

// The strip tells us the selection changed; the editor's own tab window wants to
// hear about it as TCM_TABSEL, which is what CDefault3DTabWindow::WindowProc
// watches for in order to run the command bound to the tab.
void SECTabWndBase::OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult) {
    const int nSel = m_wndTabs.GetCurSel();
    if (nSel >= 0 && nSel < static_cast<int>(m_tabs.size())) {
        m_nActive = nSel;
        ShowActivePage();
        SendMessage(TCM_TABSEL, static_cast<WPARAM>(nSel), 0);
    }
    if (pResult != nullptr) {
        *pResult = 0;
    }
}

SECTab* SECTabWndBase::InsertTab(CWnd* pWnd, int nIndex, LPCTSTR lpszLabel) {
    spdlog::debug("{} this={} pWnd={} nIndex={} lpszLabel={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd), nIndex, lpszLabel);
    if (pWnd == nullptr) {
        return nullptr;
    }
    if (nIndex < 0 || nIndex > static_cast<int>(m_tabs.size())) {
        nIndex = static_cast<int>(m_tabs.size());
    }

    SECTab *pTab = new SECTab();
    pTab->pWnd = pWnd;
    pTab->strLabel = (lpszLabel != nullptr) ? lpszLabel : _T("");
    m_tabs.insert(m_tabs.begin() + nIndex, pTab);

    // The page is a window the caller made, usually a dialog, and it belongs
    // inside this one from here on.
    if (pWnd->GetSafeHwnd() != nullptr && GetSafeHwnd() != nullptr &&
        pWnd->GetParent() != this) {
        pWnd->SetParent(this);
    }
    if (pWnd->GetSafeHwnd() != nullptr) {
        pWnd->ShowWindow(SW_HIDE);
    }

    if (EnsureTabControl()) {
        TCITEM item = { 0 };
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(pTab->strLabel));
        m_wndTabs.InsertItem(nIndex, &item);
    }
    if (m_nActive < 0) {
        ActivateTab(nIndex);
    } else {
        if (nIndex <= m_nActive) {
            ++m_nActive;
        }
        LayoutTabs();
    }
    return pTab;
}

// Still a stub. Creating the page from a CRuntimeClass is the toolkit's other
// way in, and it needs a CCreateContext and a document to hang the view off.
// This editor always makes the window itself and passes it to the overload
// above.
SECTab* SECTabWndBase::InsertTab(CRuntimeClass* pViewClass, int nIndex, LPCTSTR lpszLabel, CCreateContext* pContext, UINT nID) {
    spdlog::debug("{} this={} pViewClass={} lpszLabel={} pContext={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pViewClass), lpszLabel, spdlog::fmt_lib::ptr(pContext), nID);
    return nullptr;
}

SECTab* SECTabWndBase::AddTab(CWnd* pWnd, LPCTSTR lpszLabel) {
    spdlog::debug("{} this={} pWnd={} lpszLabel={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd), lpszLabel);
    return InsertTab(pWnd, static_cast<int>(m_tabs.size()), lpszLabel);
}

// Still a stub, for the reason given on the InsertTab that takes a class.
SECTab* SECTabWndBase::AddTab(CRuntimeClass* pViewClass, LPCTSTR lpszLabel, CCreateContext* pContext, UINT nID) {
    spdlog::debug("{} this={} pViewClass={} lpszLabel={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pViewClass), lpszLabel, nID);
    return nullptr;
}

// The three SetTabIcon overloads remain unimplemented. A common control shows
// tab images from an image list the strip owns, and nothing in this editor sets
// a tab icon, so there is no list to build and no caller to satisfy.
void SECTabWndBase::SetTabIcon(int nIndex, HICON hIcon) {
    spdlog::debug("{} this={} nIndex={} hIcon={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, spdlog::fmt_lib::ptr(hIcon));
}

void SECTabWndBase::SetTabIcon(int nIndex, UINT nIDIcon, int cx, int cy) {
    spdlog::debug("{} this={} nIndex={} nIDIcon={} cx={} cy={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, nIDIcon, cx, cy);
}

void SECTabWndBase::SetTabIcon(int nIndex, LPCTSTR lpszResourceName, int cx, int cy) {
    spdlog::debug("{} this={} nIndex={} lpszResourceName={} cx={} cy={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, spdlog::fmt_lib::ptr(lpszResourceName), cx, cy);
}

void SECTabWndBase::RemoveTab(CWnd* pWnd) {
    spdlog::debug("{} this={} pWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd));
    RemoveTab(IndexOf(pWnd));
}

// The page window is not destroyed: the editor made it, keeps its own list of
// them and deletes them itself. CDefault3DTabWindow::RemoveAllTabs does exactly
// that, calling RemoveTab for every tab and then destroying the windows.
void SECTabWndBase::RemoveTab(int nIndex) {
    spdlog::debug("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    if (nIndex < 0 || nIndex >= static_cast<int>(m_tabs.size())) {
        return;
    }
    if (m_tabs[nIndex]->pWnd != nullptr && m_tabs[nIndex]->pWnd->GetSafeHwnd() != nullptr) {
        m_tabs[nIndex]->pWnd->ShowWindow(SW_HIDE);
    }
    delete m_tabs[nIndex];
    m_tabs.erase(m_tabs.begin() + nIndex);

    if (m_wndTabs.GetSafeHwnd() != nullptr) {
        m_wndTabs.DeleteItem(nIndex);
    }
    if (m_tabs.empty()) {
        m_nActive = -1;
    } else if (m_nActive >= static_cast<int>(m_tabs.size())) {
        ActivateTab(static_cast<int>(m_tabs.size()) - 1);
    } else if (nIndex < m_nActive) {
        --m_nActive;
    }
    LayoutTabs();
}

void SECTabWndBase::RenameTab(CWnd* pWnd, LPCTSTR lpszLabel) {
    spdlog::debug("{} this={} pWnd={} lpszLabel={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd), lpszLabel);
    RenameTab(IndexOf(pWnd), lpszLabel);
}

void SECTabWndBase::RenameTab(int nIndex, LPCTSTR lpszLabel) {
    spdlog::debug("{} this={} nIndex={} lpszLabel={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, lpszLabel);
    if (nIndex < 0 || nIndex >= static_cast<int>(m_tabs.size()) || lpszLabel == nullptr) {
        return;
    }
    m_tabs[nIndex]->strLabel = lpszLabel;
    if (m_wndTabs.GetSafeHwnd() != nullptr) {
        TCITEM item = { 0 };
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<LPTSTR>(lpszLabel);
        m_wndTabs.SetItem(nIndex, &item);
    }
    LayoutTabs();
}

BOOL SECTabWndBase::ActivateTab(CWnd* pWnd, int nIndex) {
    spdlog::debug("{} this={} pWnd={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd), nIndex);
    const int nFound = IndexOf(pWnd);
    return ActivateTab(nFound >= 0 ? nFound : nIndex);
}

BOOL SECTabWndBase::ActivateTab(CWnd* pWnd) {
    spdlog::debug("{} this={} pWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd));
    return ActivateTab(IndexOf(pWnd));
}

BOOL SECTabWndBase::ActivateTab(int nIndex) {
    spdlog::debug("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    if (nIndex < 0 || nIndex >= static_cast<int>(m_tabs.size())) {
        return FALSE;
    }
    // SEC3DTabWnd::EnableTab can mark a tab unusable. The strip cannot show that,
    // so refusing to select it is the whole of what disabling means here.
    if (!m_tabs[nIndex]->bEnabled) {
        return FALSE;
    }
    m_nActive = nIndex;
    if (m_wndTabs.GetSafeHwnd() != nullptr) {
        m_wndTabs.SetCurSel(nIndex);
    }
    LayoutTabs();
    return TRUE;
}

// Leaves every tab unselected, which for a common control means no page shown.
void SECTabWndBase::ClearSelection() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    m_nActive = -1;
    if (m_wndTabs.GetSafeHwnd() != nullptr) {
        m_wndTabs.SetCurSel(-1);
    }
    ShowActivePage();
}

// Both remain unimplemented, and honestly: a common control scrolls its own
// strip when the tabs do not fit, so there is nothing to bring into view.
void SECTabWndBase::ScrollToTab(CWnd* pWnd) {
    spdlog::debug("{} this={} pWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd));
}

void SECTabWndBase::ScrollToTab(int nIndex) {
    spdlog::debug("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
}

int SECTabWndBase::GetTabCount() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return static_cast<int>(m_tabs.size());
}

BOOL SECTabWndBase::GetTabInfo(int nIndex, LPCTSTR& lpszLabel, BOOL& bSelected, CWnd*& pWnd, void*& pExtra) const {
    spdlog::debug("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    if (nIndex < 0 || nIndex >= static_cast<int>(m_tabs.size())) {
        return FALSE;
    }
    const SECTab *pTab = m_tabs[nIndex];
    lpszLabel = pTab->strLabel;
    bSelected = (nIndex == m_nActive);
    pWnd = pTab->pWnd;
    pExtra = pTab->pExtra;
    return TRUE;
}

BOOL SECTabWndBase::FindTab(const CWnd* const pWnd, int& nTab) const {
    spdlog::debug("{} this={} pWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd));
    const int nFound = IndexOf(pWnd);
    if (nFound < 0) {
        return FALSE;
    }
    nTab = nFound;
    return TRUE;
}

BOOL SECTabWndBase::GetActiveTab(CWnd*& pWnd) const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    if (m_nActive < 0 || m_nActive >= static_cast<int>(m_tabs.size())) {
        return FALSE;
    }
    pWnd = m_tabs[m_nActive]->pWnd;
    return TRUE;
}

BOOL SECTabWndBase::GetActiveTab(int& nIndex) const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    if (m_nActive < 0) {
        return FALSE;
    }
    nIndex = m_nActive;
    return TRUE;
}

BOOL SECTabWndBase::TabExists(CWnd* pClient) const {
    spdlog::debug("{} this={} pClient={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pClient));
    return IndexOf(pClient) >= 0;
}

BOOL SECTabWndBase::TabExists(int nTab) const {
    spdlog::debug("{} this={} nTab={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nTab);
    return nTab >= 0 && nTab < static_cast<int>(m_tabs.size());
}

// Still a stub. SECTabControlBase is the toolkit's own strip object, and this
// class holds a CTabCtrl instead, which is not one.
const SECTabControlBase* SECTabWndBase::GetTabControl() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}
