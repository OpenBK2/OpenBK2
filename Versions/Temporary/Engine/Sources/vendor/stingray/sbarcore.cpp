#include "Toolkit/sbarcore.h"
#include "Toolkit/sbarstat.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


// What a docking window gets when nothing has told it how big to be. Wide
// enough to read a tree or a log in, narrow enough not to take the frame.
namespace { const int DEFAULT_THICKNESS = 120; }

BOOL SECControlBar::m_bOptimizedRedrawEnabled = FALSE;

SECControlBar::SECControlBar() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

// Creating the window is only half of creating a control bar.
//
// CControlBar keeps the CBRS_ half of the style in m_dwStyle, separately from
// the window style, and every bar MFC ships sets it in its own Create:
// CDialogBar::Create is m_dwStyle = (nStyle & CBRS_ALL) before it creates
// anything. These stubs never did, so every docking window in the editor had
// m_dwStyle 0: no CBRS_ALIGN_LEFT, no CBRS_SIZE_DYNAMIC, no borders. A bar with
// no alignment is one CFrameWnd::RecalcLayout gives no room to, which is why
// they stayed invisible even once they were docked and had WS_VISIBLE.
//
// The dwExStyle these take is the toolkit's own CBRS_EX_ set, the cool look and
// the gripper, which has nothing to do with the WS_EX_ flags CreateWindowEx
// wants. Passing it through was asking Windows for whatever those bits happen to
// mean there. It is dropped: this library draws none of what it selects.
BOOL SECControlBar::Create(CWnd* pParentWnd) {
    spdlog::trace("{} this={} pParentWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pParentWnd));
    return Create(pParentWnd, "SECControlBar", WS_CHILD | CBRS_TOP, 0, 0, nullptr);
}

BOOL SECControlBar::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, UINT nID, DWORD dwStyle, DWORD dwExStyle, const RECT& rect, CWnd* pParentWnd, CCreateContext* pContext) {
    spdlog::trace("{} this={} lpszClassName={} lpszWindowName={} nID={} dwStyle={} dwExStyle={} "
                  "rect.left={} rect.top={} rect.right={} rect.bottom={} "
                  "pParentWnd={} pContext={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszClassName, lpszWindowName, nID, dwStyle, dwExStyle,
                  rect.left, rect.top, rect.right, rect.bottom, spdlog::fmt_lib::ptr(pParentWnd), spdlog::fmt_lib::ptr(pContext));
    m_dwStyle = dwStyle & CBRS_ALL;
    return CWnd::Create(lpszClassName, lpszWindowName, (dwStyle & ~CBRS_ALL) | WS_CHILD, rect, pParentWnd, nID, pContext);
}

BOOL SECControlBar::Create(CWnd* pParentWnd, LPCTSTR lpszWindowName, DWORD dwStyle, DWORD dwExStyle, UINT nID, CCreateContext* pContext) {
    spdlog::trace("{} this={} pParentWnd={} lpszWindowName={} dwStyle={} dwExStyle={} nID={} pContext={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pParentWnd), lpszWindowName, dwStyle, dwExStyle, nID,
                      spdlog::fmt_lib::ptr(pContext));
    RECT rect{0, 0, 0, 0};
    LPCTSTR lpszClassName = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1), ::LoadIcon(nullptr, IDI_APPLICATION));
    return Create(lpszClassName, lpszWindowName, nID, dwStyle, dwExStyle, rect, pParentWnd, pContext);
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

// Not a stub. Doing nothing here left every docking window in the editor
// created but never placed, which is what an empty main frame looks like.
//
// MFC already does this: CControlBar::EnableDocking records the styles the bar
// will accept and gives it a CDockContext, which is what CFrameWnd::DockControlBar
// then needs. The toolkit's version differs in going through NewDockContext so
// that a derived bar can supply its own; that stays a stub, and MFC makes the
// context itself.
void SECControlBar::EnableDocking(DWORD dwDockStyle) {
    spdlog::trace("{} this={} dwDockStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwDockStyle);
    CControlBar::EnableDocking(dwDockStyle);
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

// How much room the frame should give this bar. Returning nothing is what a
// stub can say and what an invisible docking window looks like.
//
// The toolkit measures a docking window from whatever is docked inside it, and
// this library does not know what that is. What it does know is the size the
// frame asked for when it docked the bar, which DockControlBarEx puts in
// m_nMRUWidth, the member MFC keeps a dynamic bar's size in. So that is the
// thickness, and the bar stretches along the edge it is docked to, which is
// what every dockable bar does.
//
// LM_HORZ says the bar lies along the top or bottom, so its length is cx and
// its thickness cy, and the other way round when it is on the left or right.
CSize SECControlBar::CalcDynamicLayout(int nLength, DWORD dwMode) {
    spdlog::trace("{} this={} nLength={} dwMode={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nLength, dwMode);
    const int nThickness = m_nMRUWidth != 0 ? static_cast<int>(m_nMRUWidth) : DEFAULT_THICKNESS;
    const int nAlong = (dwMode & LM_STRETCH) != 0 ? 32767 : (nLength >= 0 ? nLength : nThickness);
    return (dwMode & LM_HORZ) != 0 ? CSize(nAlong, nThickness) : CSize(nThickness, nAlong);
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
