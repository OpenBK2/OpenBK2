#include "Toolkit/sbarcore.h"
#include "Toolkit/sbarstat.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


BOOL SECControlBar::m_bOptimizedRedrawEnabled = FALSE;

SECControlBar::SECControlBar() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

BOOL SECControlBar::Create(CWnd* pParentWnd) {
    spdlog::trace("{} this={} pParentWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pParentWnd));
    RECT rect{ 0, 0, 0, 0 };
    LPCTSTR lpszClassName = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1), ::LoadIcon(nullptr, IDI_APPLICATION));
    return CControlBar::Create(lpszClassName, "SECControlBar", 0, rect, pParentWnd, 0, nullptr);
}

BOOL SECControlBar::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, UINT nID, DWORD dwStyle, DWORD dwExStyle, const RECT& rect, CWnd* pParentWnd, CCreateContext* pContext) {
    spdlog::trace("{} this={} lpszClassName={} lpszWindowName={} nID={} dwStyle={} dwExStyle={} "
                  "rect.left={} rect.top={} rect.right={} rect.bottom={} "
                  "pParentWnd={} pContext={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszClassName, lpszWindowName, nID, dwStyle, dwExStyle,
                  rect.left, rect.top, rect.right, rect.bottom, spdlog::fmt_lib::ptr(pParentWnd), spdlog::fmt_lib::ptr(pContext));
    return CControlBar::CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

BOOL SECControlBar::Create(CWnd* pParentWnd, LPCTSTR lpszWindowName, DWORD dwStyle, DWORD dwExStyle, UINT nID, CCreateContext* pContext) {
    spdlog::trace("{} this={} pParentWnd={} lpszWindowName={} dwStyle={} dwExStyle={} nID={} pContext={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pParentWnd), lpszWindowName, dwStyle, dwExStyle, nID,
                      spdlog::fmt_lib::ptr(pContext));
    RECT rect{0, 0, 0, 0};
    LPCTSTR lpszClassName = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1), ::LoadIcon(nullptr, IDI_APPLICATION));
    return CControlBar::CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

void SECControlBar::GetInsideRect(CRect& rectInside) const {
    spdlog::trace("{} this={} rectInside.left={} rectInside.top={} rectInside.right={} rectInside.bottom={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), rectInside.left, rectInside.top, rectInside.right, rectInside.bottom);
}

BOOL SECControlBar::IsMDIChild() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECControlBar::GetOptimizeRedrawEnabled() {
    spdlog::trace("{}", BOOST_CURRENT_FUNCTION);
    return m_bOptimizedRedrawEnabled;
}

void SECControlBar::SetOptimizedRedrawEnabled(BOOL bOptimize) {
    spdlog::trace("{} bOptimize={}", BOOST_CURRENT_FUNCTION, bOptimize);
    m_bOptimizedRedrawEnabled = bOptimize;
}

BOOL SECControlBar::GetBarSizePos(int& nRow,int& nCol) {
    spdlog::trace("{} this={} nRow={} nCol={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nRow, nCol);
    return FALSE;
}

BOOL SECControlBar::GetBarSizePos(int& nRow,int& nCol,int& nDockbarID) {
    spdlog::trace("{} this={} nRow={} nCol={} nDockbarID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nRow, nCol, nDockbarID);
    return FALSE;
}

BOOL SECControlBar::GetBarSizePos(int& nRow,int& nCol,int& nDockbarID,float& fPctWidth,int& nHeight) {
    spdlog::trace("{} this={} nRow={} nCol={} nDockbarID={} fPctWidth={} nHeight={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nRow, nCol, nDockbarID, fPctWidth, nHeight);
    return FALSE;
}

void SECControlBar::EnableDocking(DWORD dwDockStyle) {
    spdlog::trace("{} this={} dwDockStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwDockStyle);
}

SECDockContext * SECControlBar::NewDockContext() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECControlBar::SetExBarStyle(DWORD dwExStyle, BOOL bAutoUpdate) {
    spdlog::trace("{} this={} dwExStyle={} bAutoUpdate={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwExStyle, bAutoUpdate);
}

void SECControlBar::ModifyBarStyleEx(DWORD dwRemove, DWORD dwAdd, BOOL bAutoUpdate) {
    spdlog::trace("{} this={} dwRemove={} dwAdd={} bAutoUpdate={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemove, dwAdd, bAutoUpdate);
}

// Not a stub. A control bar's id is what CFrameWnd::GetControlBar looks it up
// by, what LoadBarState and SaveBarState name it by, and what ShowControlBar
// addresses it with. Returning 0 gave every docking window in the editor the
// same id, and 0 is the one value GetControlBar refuses outright, so no bar
// could be found by any of them and LoadBarState asserted on the first one it
// read back.
//
// The first id at or above nBaseID that no control bar on this frame is using.
// Stable across runs given the same creation order, which is what a saved
// layout needs in order to still mean something the next time.
UINT SECControlBar::GetUniqueBarID(CFrameWnd* pMainWnd, UINT nBaseID) {
    spdlog::trace("{} pMainWnd={} nBaseID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(pMainWnd), nBaseID);
    if (pMainWnd == nullptr) {
        return nBaseID;
    }
    UINT nID = nBaseID;
    while (pMainWnd->GetControlBar(nID) != nullptr) {
        ++nID;
    }
    return nID;
}

// Still a stub. Answering it means walking every control bar on the frame, and
// CFrameWnd::m_listControlBars is not public; GetControlBar only looks one up
// by id, which is the question already answered above. Nothing in this tree
// calls it.
BOOL SECControlBar::VerifyUniqueBarIds(CFrameWnd* pFrameWnd) {
    spdlog::trace("{} pFrameWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(pFrameWnd));
    return FALSE;
}

// Also not a stub, and the same question GetUniqueBarID answers by counting.
BOOL SECControlBar::VerifyUniqueSpecificBarID(CFrameWnd* pFrameWnd, UINT nBarID) {
    spdlog::trace("{} pFrameWnd={} nBarID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(pFrameWnd), nBarID);
    return pFrameWnd != nullptr && pFrameWnd->GetControlBar(nBarID) == nullptr;
}

CSize SECControlBar::CalcDynamicLayout(int nLength, DWORD dwMode) {
    spdlog::trace("{} this={} nLength={} dwMode={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nLength, dwMode);
    return CSize(0, 0);
}

void SECControlBar::OnBarBeginDock() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECControlBar::OnBarEndDock() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECControlBar::OnBarBeginFloat() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECControlBar::OnBarEndFloat() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECControlBar::OnBarBeginMDIFloat() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECControlBar::OnBarEndMDIFloat() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

BOOL SECControlBar::OnGripperClose() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECControlBar::OnGripperExpand() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

void SECControlBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler) {
    spdlog::trace("{} this={} pTarget={} bDisableIfNoHndler={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pTarget), bDisableIfNoHndler);
}

BOOL SECStatusBar::SetIndicators(const UINT * indicators, int size) {
    spdlog::trace("{} this={} indicators={} size={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(indicators), size);
    return TRUE;
}

int SECStatusBar::CommandToIndex(UINT nID) {
    spdlog::trace("{} this={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID);
    return 0;
}

void SECStatusBar::SetPaneInfo(int index, UINT nID, UINT status, UINT size) {
    spdlog::trace("{} this={} index={} nID={} status={} size={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), index, nID, status, size);
}

void SECStatusBar::SetPaneText(int index, const CString & text) {
    spdlog::trace("{} this={} index={} text={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), index, text.GetString());
}
