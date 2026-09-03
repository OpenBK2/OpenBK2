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
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
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
    spdlog::debug("{} this={} pParentWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pParentWnd));
    return Create(pParentWnd, "SECControlBar", WS_CHILD | CBRS_TOP, 0, 0, nullptr);
}

BOOL SECControlBar::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, UINT nID, DWORD dwStyle, DWORD dwExStyle, const RECT& rect, CWnd* pParentWnd, CCreateContext* pContext) {
    spdlog::debug("{} this={} lpszClassName={} lpszWindowName={} nID={} dwStyle={} dwExStyle={} "
                  "rect.left={} rect.top={} rect.right={} rect.bottom={} "
                  "pParentWnd={} pContext={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszClassName, lpszWindowName, nID, dwStyle, dwExStyle,
                  rect.left, rect.top, rect.right, rect.bottom, spdlog::fmt_lib::ptr(pParentWnd), spdlog::fmt_lib::ptr(pContext));
    m_dwStyle = dwStyle & CBRS_ALL;
    if (lpszClassName == nullptr) {
        lpszClassName = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW),
                                            reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1));
    }
    return CWnd::Create(lpszClassName, lpszWindowName, (dwStyle & ~CBRS_ALL) | WS_CHILD, rect, pParentWnd, nID, pContext);
}

BOOL SECControlBar::Create(CWnd* pParentWnd, LPCTSTR lpszWindowName, DWORD dwStyle, DWORD dwExStyle, UINT nID, CCreateContext* pContext) {
    spdlog::debug("{} this={} pParentWnd={} lpszWindowName={} dwStyle={} dwExStyle={} nID={} pContext={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pParentWnd), lpszWindowName, dwStyle, dwExStyle, nID,
                      spdlog::fmt_lib::ptr(pContext));
    RECT rect{0, 0, 0, 0};
    LPCTSTR lpszClassName = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1), ::LoadIcon(nullptr, IDI_APPLICATION));
    return Create(lpszClassName, lpszWindowName, nID, dwStyle, dwExStyle, rect, pParentWnd, pContext);
}

// Where a docking window may put what it contains: this bar's client area with
// its borders taken off.
//
// Leaving the caller's rectangle untouched, which is what this did, is not a
// harmless nothing. CDWLog::OnSize, CDWPropertyBrowser::OnSize and the GDB
// browser all ask for it and then SetWindowPos their contents into whatever
// came back, so the Scintilla control in the log window came out 0 by 0 inside
// a bar that was 265 by 265.
//
// CalcInsideRect is MFC's own answer to the same question, and it is what
// CControlBar's painting uses, so the contents land exactly inside the borders
// the bar draws.
void SECControlBar::GetInsideRect(CRect& rectInside) const {
    spdlog::debug("{} this={} rectInside.left={} rectInside.top={} rectInside.right={} rectInside.bottom={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), rectInside.left, rectInside.top, rectInside.right, rectInside.bottom);
    if (GetSafeHwnd() == nullptr) {
        rectInside.SetRectEmpty();
        return;
    }
    GetClientRect(&rectInside);
    CalcInsideRect(rectInside, (m_dwStyle & CBRS_ORIENT_HORZ) != 0);
}

BOOL SECControlBar::IsMDIChild() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECControlBar::GetOptimizeRedrawEnabled() {
    spdlog::debug("{}", BOOST_CURRENT_FUNCTION);
    return m_bOptimizedRedrawEnabled;
}

void SECControlBar::SetOptimizedRedrawEnabled(BOOL bOptimize) {
    spdlog::debug("{} bOptimize={}", BOOST_CURRENT_FUNCTION, bOptimize);
    m_bOptimizedRedrawEnabled = bOptimize;
}

BOOL SECControlBar::GetBarSizePos(int& nRow,int& nCol) {
    spdlog::debug("{} this={} nRow={} nCol={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nRow, nCol);
    return FALSE;
}

BOOL SECControlBar::GetBarSizePos(int& nRow,int& nCol,int& nDockbarID) {
    spdlog::debug("{} this={} nRow={} nCol={} nDockbarID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nRow, nCol, nDockbarID);
    return FALSE;
}

BOOL SECControlBar::GetBarSizePos(int& nRow,int& nCol,int& nDockbarID,float& fPctWidth,int& nHeight) {
    spdlog::debug("{} this={} nRow={} nCol={} nDockbarID={} fPctWidth={} nHeight={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nRow, nCol, nDockbarID, fPctWidth, nHeight);
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
    spdlog::debug("{} this={} dwDockStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwDockStyle);
    CControlBar::EnableDocking(dwDockStyle);
}

SECDockContext * SECControlBar::NewDockContext() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECControlBar::SetExBarStyle(DWORD dwExStyle, BOOL bAutoUpdate) {
    spdlog::debug("{} this={} dwExStyle={} bAutoUpdate={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwExStyle, bAutoUpdate);
}

void SECControlBar::ModifyBarStyleEx(DWORD dwRemove, DWORD dwAdd, BOOL bAutoUpdate) {
    spdlog::debug("{} this={} dwRemove={} dwAdd={} bAutoUpdate={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemove, dwAdd, bAutoUpdate);
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
    spdlog::debug("{} pMainWnd={} nBaseID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(pMainWnd), nBaseID);
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
    spdlog::debug("{} pFrameWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(pFrameWnd));
    return FALSE;
}

// Also not a stub, and the same question GetUniqueBarID answers by counting.
BOOL SECControlBar::VerifyUniqueSpecificBarID(CFrameWnd* pFrameWnd, UINT nBarID) {
    spdlog::debug("{} pFrameWnd={} nBarID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(pFrameWnd), nBarID);
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
// A bar that is not CBRS_SIZE_DYNAMIC is measured through this one instead, and
// CControlBar::CalcFixedLayout answers 0 for anything that has not overridden
// it. The editor's shortcut bars are docked that way and came out 0 wide.
CSize SECControlBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz) {
    spdlog::debug("{} this={} bStretch={} bHorz={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bStretch, bHorz);
    return CalcDynamicLayout(-1, (bStretch ? LM_STRETCH : 0) | (bHorz ? LM_HORZ : 0));
}

CSize SECControlBar::CalcDynamicLayout(int nLength, DWORD dwMode) {
    spdlog::debug("{} this={} nLength={} dwMode={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nLength, dwMode);
    const int nThickness = m_nMRUWidth != 0 ? static_cast<int>(m_nMRUWidth) : DEFAULT_THICKNESS;
    const int nAlong = (dwMode & LM_STRETCH) != 0 ? 32767 : (nLength >= 0 ? nLength : nThickness);
    return (dwMode & LM_HORZ) != 0 ? CSize(nAlong, nThickness) : CSize(nThickness, nAlong);
}

void SECControlBar::OnBarBeginDock() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECControlBar::OnBarEndDock() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECControlBar::OnBarBeginFloat() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECControlBar::OnBarEndFloat() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECControlBar::OnBarBeginMDIFloat() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECControlBar::OnBarEndMDIFloat() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

BOOL SECControlBar::OnGripperClose() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECControlBar::OnGripperExpand() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

// MFC calls this on every bar on every idle, and a bar that holds a window
// rather than buttons has nothing to update. It was 1,764 of the 1,946 lines
// in a trace of one startup, so it stays a level below everything else here
// and OBK2_STINGRAY_LOG=trace is what asks for it.
void SECControlBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler) {
    spdlog::trace("{} this={} pTarget={} bDisableIfNoHndler={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pTarget), bDisableIfNoHndler);
}

// CommandToIndex, SetPaneInfo and SetPaneText are gone from this file: with
// CStatusBar underneath, MFC's own are correct and were only being shadowed.
//
// Returning a constant from CommandToIndex was not the harmless placeholder it
// looked like. CMainFrame::OnCreate asks it where panes 140 and 141 are and then
// sizes what it is told: both answers were 0, so both SetPaneInfo calls landed on
// pane 0 with widths 500 and 200 and the second overwrote the first. Searching
// the panes for the id, which is all CStatusBar::CommandToIndex does, answers 1
// and 2.
//
// SetIndicators still needs an override, because MFC's reports failure that this
// editor cannot survive.
//
// MFC loads a string resource per indicator, to size each pane from the width of
// its text. The editor's two pane ids have no string resource -- not in this
// build, and not in the shipped 2005 binary either, where LoadString for 140 and
// 141 comes back empty as well. Since that editor ran with a working status bar,
// the toolkit's SetIndicators plainly never looked a string up. These panes get
// their text at runtime from CMainFrame::SetStatusBarText and their widths from
// the SetPaneInfo calls that follow, so the lookup has nothing to contribute.
//
// Handing MFC the id array anyway is worse than a wasted lookup, because its loop
// breaks out on the first string it cannot load. With { ID_SEPARATOR, 140, 141 }
// it assigns pane 1 its id, fails to load 140's string and stops, so pane 2 keeps
// the id 0 that AllocElements zeroed it to. CommandToIndex( 141 ) then finds no
// such pane and answers -1, and CMainFrame::OnCreate feeds that straight to
// SetPaneInfo, which asserts on the index.
//
// So the panes are allocated by passing no id array at all, which is the argument
// the string lookup hangs off: with nullptr MFC skips that loop entirely. Each
// pane is then given its id directly, which is all CommandToIndex searches for.
//
// The one thing MFC's skipped loop also does is make an id-less first pane the
// stretchy one, and that is worth keeping: it is the pane the editor writes its
// messages into, and it has to take whatever width the two sized panes leave.
BOOL SECStatusBar::SetIndicators(const UINT * indicators, int size) {
    spdlog::debug("{} this={} indicators={} size={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(indicators), size);
    if (!CStatusBar::SetIndicators(nullptr, size)) {
        return FALSE;
    }
    if (indicators == nullptr) {
        return TRUE;
    }
    for (int i = 0; i < size; ++i) {
        const UINT nStyle = (indicators[i] == 0 && i == 0) ? (SBPS_STRETCH | SBPS_NOBORDERS)
                                                           : SBPS_NORMAL;
        SetPaneInfo(i, indicators[i], nStyle, 0);
    }
    return TRUE;
}
