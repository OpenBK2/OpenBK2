#include "Toolkit/tbarcust.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


IMPLEMENT_DYNAMIC(SECCustomToolBar, CToolBar)

SECCustomToolBar::SECCustomToolBar() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

SECCustomToolBar::~SECCustomToolBar() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    ClearButtons();
}

void SECCustomToolBar::ClearButtons() {
    for (Button *pBtn : m_btns) {
        delete pBtn;
    }
    m_btns.clear();
}

// Mirror the control's buttons into m_btns, which the editor indexes directly.
//
// Separators are kept in place rather than skipped, because the editor uses the
// index it finds here to ask GetItemRect for that button's rectangle, and the
// control counts separators when numbering.
void SECCustomToolBar::RebuildButtons() {
    ClearButtons();
    if (GetSafeHwnd() == nullptr) {
        return;
    }
    const int nCount = CToolBar::GetCount();
    m_btns.reserve(nCount);
    for (int i = 0; i < nCount; ++i) {
        Button *pBtn = new Button();
        pBtn->m_nID = CToolBar::GetItemID(i);
        m_btns.push_back(pBtn);
    }
}

std::vector<UINT> SECCustomToolBar::CurrentIDs() const {
    std::vector<UINT> ids;
    if (GetSafeHwnd() == nullptr) {
        return ids;
    }
    const int nCount = CToolBar::GetCount();
    ids.reserve(nCount);
    for (int i = 0; i < nCount; ++i) {
        ids.push_back(CToolBar::GetItemID(i));
    }
    return ids;
}

// The class name is dropped: CToolBar registers its own, and the toolkit only
// took one so that a derived bar could supply a different painted window.
//
// dwExStyle is the toolkit's CBRS_EX_ set, the cool look and the gripper, and
// has nothing to do with the WS_EX_ flags CreateWindowEx wants. It is dropped
// for the same reason SECControlBar drops it: none of what it selects is drawn
// here. TBSTYLE_FLAT is passed instead, which is the closest the common control
// gets to the look those flags asked for and costs nothing.
BOOL SECCustomToolBar::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, UINT nID, DWORD dwStyle, DWORD dwExStyle, const RECT& rect, CWnd* pParentWnd, CCreateContext* pContext) {
    spdlog::debug("{} this={} lpszClassName={} lpszWindowName={} nID={} dwStyle={} dwExStyle={} "
                  "rect.left={} rect.top={} rect.right={} rect.bottom={} "
                  "pParentWnd={} pContext={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszClassName, lpszWindowName, nID, dwStyle, dwExStyle,
                  rect.left, rect.top, rect.right, rect.bottom,
                  spdlog::fmt_lib::ptr(pParentWnd), spdlog::fmt_lib::ptr(pContext));
    if (!CToolBar::CreateEx(pParentWnd, TBSTYLE_FLAT, dwStyle, CRect(0, 0, 0, 0), nID)) {
        return FALSE;
    }
    // The title is what a docked bar's gripper menu and the float caption show,
    // and what the window dump identifies the bar by.
    if (lpszWindowName != nullptr) {
        SetWindowText(lpszWindowName);
    }
    return TRUE;
}

BOOL SECCustomToolBar::CreateEx(DWORD dwExStyle, CWnd* pParentWnd, DWORD dwStyle, UINT nID, LPCTSTR lpszTitle) {
    spdlog::debug("{} this={} dwExStyle={} pParentWnd={} dwStyle={} nID={} lpszTitle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwExStyle, spdlog::fmt_lib::ptr(pParentWnd), dwStyle, nID, lpszTitle);
    return Create(nullptr, lpszTitle, nID, dwStyle, dwExStyle, CRect(0, 0, 0, 0), pParentWnd, nullptr);
}

// Still a stub. Saving a customized bar's layout means saving which buttons the
// user moved where, and this library does not let them be moved.
void SECCustomToolBar::SetBarInfoEx(SECControlBarInfo* pInfo, CFrameWnd* pFrameWnd) {
    spdlog::debug("{} this={} pInfo={} pFrameWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pInfo), spdlog::fmt_lib::ptr(pFrameWnd));
}

void SECCustomToolBar::SetButtonStyle(int nIndex, UINT nStyle) {
    spdlog::debug("{} this={} nIndex={} nStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, nStyle);
    if (GetSafeHwnd() != nullptr && nIndex >= 0 && nIndex < CToolBar::GetCount()) {
        CToolBar::SetButtonStyle(nIndex, nStyle);
    }
}

UINT SECCustomToolBar::GetButtonStyle(int nIndex) const {
    spdlog::debug("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    if (GetSafeHwnd() == nullptr || nIndex < 0 || nIndex >= CToolBar::GetCount()) {
        return 0;
    }
    return CToolBar::GetButtonStyle(nIndex);
}

// CToolBar has no insert or delete, only SetButtons for the whole bar, so both
// of these read the current ids back, change the one entry and set them again.
BOOL SECCustomToolBar::RemoveButton(int nIndex, BOOL bNoUpdate) {
    spdlog::debug("{} this={} nIndex={} bNoUpdate={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, bNoUpdate);
    std::vector<UINT> ids = CurrentIDs();
    if (nIndex < 0 || nIndex >= static_cast<int>(ids.size())) {
        return FALSE;
    }
    ids.erase(ids.begin() + nIndex);
    return SetButtons(ids.empty() ? nullptr : ids.data(), static_cast<int>(ids.size()));
}

void SECCustomToolBar::AddButton(int nIndex, int nID, BOOL bSeparator, BOOL bNoUpdate) {
    spdlog::debug("{} this={} nIndex={} nID={} bSeparator={} bNoUpdate={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, nID, bSeparator, bNoUpdate);
    std::vector<UINT> ids = CurrentIDs();
    // A zero id is how SetButtons is told to leave a gap; afxres.h spells the
    // same value ID_SEPARATOR, which is not included here.
    const UINT nNew = bSeparator ? 0u : static_cast<UINT>(nID);
    if (nIndex < 0 || nIndex > static_cast<int>(ids.size())) {
        ids.push_back(nNew);
    } else {
        ids.insert(ids.begin() + nIndex, nNew);
    }
    SetButtons(ids.data(), static_cast<int>(ids.size()));
}

int SECCustomToolBar::GetBtnCount() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    if (GetSafeHwnd() == nullptr) {
        return 0;
    }
    return CToolBar::GetCount();
}

// Still stubs, both of them, and honestly so: there is no customize mode and no
// alt-drag, so the bar is never in either.
BOOL SECCustomToolBar::InConfigMode() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECCustomToolBar::InAltDragMode() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

// -1, not 0, for a command that is not on this bar. Zero is a valid index and
// saying it here is what made the status bar's panes collide.
int SECCustomToolBar::CommandToIndex(UINT nID) const {
    spdlog::debug("{} this={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID);
    if (GetSafeHwnd() == nullptr) {
        return -1;
    }
    return CToolBar::CommandToIndex(nID);
}

UINT SECCustomToolBar::GetItemID(int nIndex) const {
    spdlog::debug("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    if (GetSafeHwnd() == nullptr || nIndex < 0 || nIndex >= CToolBar::GetCount()) {
        return 0;
    }
    return CToolBar::GetItemID(nIndex);
}

// Still a stub, and -1 rather than 0 for the same reason as above: no button is
// active, and button 0 is not the way to say that.
int SECCustomToolBar::GetCurBtn() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return -1;
}

// Still a stub. This maps a command id to its frame in the manager's shared
// bitmap, which only the customize dialog needs in order to draw a button that
// is not on a bar yet.
int SECCustomToolBar::IDToBmpIndex(UINT nID, HBITMAP* lphBmp) {
    spdlog::debug("{} this={} nID={} lphBmp={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID, spdlog::fmt_lib::ptr(lphBmp));
    if (lphBmp != nullptr) {
        *lphBmp = nullptr;
    }
    return -1;
}

// A toolbar resource carries the bitmap and the button ids together, so this one
// call is enough to fill a bar.
BOOL SECCustomToolBar::LoadToolBar(LPCTSTR lpszResourceName) {
    spdlog::debug("{} this={} lpszResourceName={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(lpszResourceName));
    if (!CToolBar::LoadToolBar(lpszResourceName)) {
        return FALSE;
    }
    RebuildButtons();
    return TRUE;
}

// The toolkit takes the id array here as well, because its bitmap is shared
// between every bar and the array says which frames of it this bar uses.
// CToolBar owns its bitmap outright and numbers the frames in button order, so
// the array is applied as the button list instead, which comes to the same
// arrangement for a bar whose bitmap was built for it.
BOOL SECCustomToolBar::LoadBitmap(UINT nIDResource, const UINT* lpIDArray, int nIDCount) {
    spdlog::debug("{} this={} nIDResource={} lpIDArray={} nIDCount={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDResource, spdlog::fmt_lib::ptr(lpIDArray), nIDCount);
    if (!CToolBar::LoadBitmap(nIDResource)) {
        return FALSE;
    }
    if (lpIDArray != nullptr && nIDCount > 0) {
        return SetButtons(lpIDArray, nIDCount);
    }
    RebuildButtons();
    return TRUE;
}

BOOL SECCustomToolBar::SetButtons(const UINT* lpIDArray, int nIDCount) {
    spdlog::debug("{} this={} lpIDArray={} nIDCount={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(lpIDArray), nIDCount);
    if (GetSafeHwnd() == nullptr || nIDCount < 1) {
        return FALSE;
    }
    if (!CToolBar::SetButtons(lpIDArray, nIDCount)) {
        return FALSE;
    }
    RebuildButtons();
    return TRUE;
}

void SECCustomToolBar::GetItemRect(int nIndex, LPRECT lpRect) const {
    spdlog::debug("{} this={} nIndex={} lpRect={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, spdlog::fmt_lib::ptr(lpRect));
    if (lpRect == nullptr) {
        return;
    }
    if (GetSafeHwnd() == nullptr || nIndex < 0 || nIndex >= CToolBar::GetCount()) {
        ::SetRectEmpty(lpRect);
        return;
    }
    CToolBar::GetItemRect(nIndex, lpRect);
}

// Still a stub. The toolkit's buttons are objects that can be told things; these
// are entries in a common control and there is nobody to inform.
void SECCustomToolBar::InformBtns(UINT nID, UINT nCode, void* pData, BOOL bPass) {
    spdlog::debug("{} this={} nID={} nCode={} pData={} bPass={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID, nCode, spdlog::fmt_lib::ptr(pData), bPass);
}

// Still a stub. The control wraps a docked bar itself.
void SECCustomToolBar::BalanceWrap(int nRow, Wrapped* pWrap) {
    spdlog::debug("{} this={} nRow={} pWrap={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nRow, spdlog::fmt_lib::ptr(pWrap));
}

// Both answer for customize mode, which does not exist here: nothing can be
// dragged onto a bar and nothing accepts a drop.
BOOL SECCustomToolBar::GetDragMode() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECCustomToolBar::AcceptDrop() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}
