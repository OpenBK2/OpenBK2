#include "Toolkit/swinmdi.h"
#include "dockex.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"

#include <cstdarg>
#include <vector>


void SECMDIChildWnd::SwapMenu(UINT nID) {
    spdlog::debug("{} this={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID);
}

void SECMDIFrameWnd::EnableContextListMode(BOOL bEnable) {

    spdlog::debug("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
}

// The other half of the docking, and the one the editor actually calls:
// CMainFrame is an MDI frame, so this runs and SECFrameWnd's copy never does.
//
// The Ex is placement the toolkit adds over MFC's DockControlBar: which row and
// column of the dock bar to land in, and how much of the row to take. This used
// to keep nHeight, as the bar's m_nMRUWidth, and drop the other three, which
// left every bar starting a row of its own and sized square. See dockex.h for
// what the four mean now and how they are honoured.
void SECMDIFrameWnd::DockControlBarEx(CControlBar* pBar, UINT nDockBarID,int nCol, int nRow, float fPctWidth, int nHeight) {
    spdlog::debug("{} this={} pBar={} nDockBarID={} nCol={} nRow={} fPctWidth={} nHeight={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBar), nDockBarID, nCol, nRow, fPctWidth, nHeight);
    NDockEx::DockControlBarEx( this, pBar, nDockBarID, nCol, nRow, fPctWidth, nHeight );
}

void SECMDIFrameWnd::ReDockControlBar(CControlBar* pBar, CDockBar* pDockBar, LPCRECT lpRect) {
    spdlog::debug("{} this={} pBar={} pDockBar={} lpRect={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBar), spdlog::fmt_lib::ptr(pDockBar), spdlog::fmt_lib::ptr(lpRect));
}

void SECMDIFrameWnd::FloatControlBarInMDIChild(CControlBar* pBar, CPoint point, DWORD dwStyle) {
    spdlog::debug("{} this={} pBar={} point.x={} point.y={} dwStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBar), point.x, point.y, dwStyle);
}

BOOL SECMDIFrameWnd::EnableCustomCaption(BOOL bEnable, BOOL bRedraw) {

    spdlog::debug("{} this={} bEnable={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable, bRedraw);
    return FALSE;
}

void SECMDIFrameWnd::ForceCaptionRedraw() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECMDIFrameWnd::CreateCaptionAppFont(CFont& font) {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECMDIFrameWnd::CreateCaptionDocFont(CFont& font) {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECMDIFrameWnd::DrawCaptionText() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECMDIFrameWnd::SetCaptionTextAlign(AlignCaption ac, BOOL bRedraw) {
    spdlog::debug("{} this={} ac={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), static_cast<int>(ac), bRedraw);
}

void SECMDIFrameWnd::SwapMenu(UINT nID) {
    spdlog::debug("{} this={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID);
}

// Where the frame is told which menus the application has, and the only place
// it is told, so it is where the menu bar learns them.
//
// The menu bar also has to become a window here. CMainFrame::ShowMenu will not
// ask it to switch menus unless ::IsWindow(m_pMenuBar->m_hWnd), and nothing
// else creates it: the toolkit creates its own, and this library had left
// m_pMenuBar an object with no window, so every ShowMenu in the editor was
// skipped and the frame kept the menu LoadFrame gave it. It reports a size of
// zero, so having it costs the frame no room.
BOOL SECMDIFrameWnd::LoadAdditionalMenus(UINT nCount, UINT nIDMenu, ...) {
    spdlog::debug("{} this={} nCount={} nIDMenu={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCount, nIDMenu);
    if (m_pMenuBar == nullptr) {
        return FALSE;
    }
    // AFX_IDW_CONTROLBAR_LAST, which is the id the original editor's menu bar
    // carries: its window tree shows it as 59647.
    //
    // Asking GetUniqueBarID for the first id free from AFX_IDW_TOOLBAR was
    // wrong, and only became visibly wrong once the toolbars were real. It
    // answered 59392, because at this point in startup no bar holds that id
    // yet: SECToolBarManager has been given all six definitions but does not
    // build them until later, and 59392 is the first one it will use. The menu
    // bar and the File Toolbar then both had 59392, which is the id
    // GetControlBar, ShowControlBar and LoadBarState address a bar by.
    //
    // The end of the control bar range cannot collide with them, since that is
    // where the toolbar ids count up from.
    if (m_pMenuBar->GetSafeHwnd() == nullptr
        && !m_pMenuBar->CreateEx(0, this, WS_CHILD | CBRS_TOP,
                                 AFX_IDW_CONTROLBAR_LAST,
                                 "SECMenuBar")) {
        spdlog::warn("SECMDIFrameWnd::LoadAdditionalMenus: the menu bar has no window");
        return FALSE;
    }
    std::vector<UINT> menus;
    if (nCount > 0) {
        menus.push_back(nIDMenu);
        va_list args;
        va_start(args, nIDMenu);
        for (UINT i = 1; i < nCount; ++i) {
            menus.push_back(va_arg(args, UINT));
        }
        va_end(args);
    }
    return m_pMenuBar->SetMenus(menus);
}

void SECMDIFrameWnd::EnableOleContainmentMode() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

afx_msg LRESULT SECMDIFrameWnd::OnExtendContextMenu(WPARAM wParam, LPARAM lParam) {

    spdlog::debug("{} this={} wParam={} lParam={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), wParam, lParam);
    return 0;
}

SECWorkspaceManagerEx* SECMDIFrameWnd::InitWorkspaceMgrEx(const CString& strAppKey, BOOL bRegistryMode, CRuntimeClass* pWSClass, BOOL bSectionKey) {

    spdlog::debug("{} this={} strAppKey={} bRegistryMode={} pWSClass={} bSectionKey={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strAppKey.GetString(), bRegistryMode, spdlog::fmt_lib::ptr(pWSClass), bSectionKey);
    return nullptr;
}

void SECMDIFrameWnd::EnableBmpMenus() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}
