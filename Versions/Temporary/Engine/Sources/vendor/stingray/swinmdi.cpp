#include "Toolkit/swinmdi.h"

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

// The other half of the docking, and the other reason the frame came up empty.
//
// The Ex is placement the toolkit adds over MFC's DockControlBar: which row and
// column of the dock bar to land in, and how much of the row to take. MFC has no
// equivalent, so those three are dropped and the bar goes where MFC puts it.
//
// nHeight does map onto something. The editor creates these bars
// CBRS_SIZE_DYNAMIC, and for a dynamic bar MFC keeps the docked size in
// m_nMRUWidth, which is the same member LoadBarState restores from a saved
// layout. So the one hint of the four that has a home is given one.
void SECMDIFrameWnd::DockControlBarEx(CControlBar* pBar, UINT nDockBarID,int nCol, int nRow, float fPctWidth, int nHeight) {
    spdlog::debug("{} this={} pBar={} nDockBarID={} nCol={} nRow={} fPctWidth={} nHeight={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBar), nDockBarID, nCol, nRow, fPctWidth, nHeight);
    if (pBar == nullptr) {
        return;
    }
    if (nHeight > 0) {
        pBar->m_nMRUWidth = static_cast<UINT>(nHeight);
    }
    DockControlBar(pBar, nDockBarID);
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
    if (m_pMenuBar->GetSafeHwnd() == nullptr
        && !m_pMenuBar->CreateEx(0, this, WS_CHILD | CBRS_TOP,
                                 SECControlBar::GetUniqueBarID(this, AFX_IDW_TOOLBAR),
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
