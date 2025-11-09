#include "Toolkit/swinmdi.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>


void SECMDIChildWnd::SwapMenu(UINT nID) {
    spdlog::trace("{} this={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID);
}

void SECMDIFrameWnd::EnableContextListMode(BOOL bEnable) {

    spdlog::trace("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
}

void SECMDIFrameWnd::DockControlBarEx(CControlBar* pBar, UINT nDockBarID,int nCol, int nRow, float fPctWidth, int nHeight) {
    spdlog::trace("{} this={} pBar={} nDockBarID={} nCol={} nRow={} fPctWidth={} nHeight={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBar), nDockBarID, nCol, nRow, fPctWidth, nHeight);
}

void SECMDIFrameWnd::ReDockControlBar(CControlBar* pBar, CDockBar* pDockBar, LPCRECT lpRect) {
    spdlog::trace("{} this={} pBar={} pDockBar={} lpRect={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBar), spdlog::fmt_lib::ptr(pDockBar), spdlog::fmt_lib::ptr(lpRect));
}

void SECMDIFrameWnd::FloatControlBarInMDIChild(CControlBar* pBar, CPoint point, DWORD dwStyle) {
    spdlog::trace("{} this={} pBar={} point.x={} point.y={} dwStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBar), point.x, point.y, dwStyle);
}

BOOL SECMDIFrameWnd::EnableCustomCaption(BOOL bEnable, BOOL bRedraw) {

    spdlog::trace("{} this={} bEnable={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable, bRedraw);
    return FALSE;
}

void SECMDIFrameWnd::ForceCaptionRedraw() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECMDIFrameWnd::CreateCaptionAppFont(CFont& font) {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECMDIFrameWnd::CreateCaptionDocFont(CFont& font) {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECMDIFrameWnd::DrawCaptionText() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECMDIFrameWnd::SetCaptionTextAlign(AlignCaption ac, BOOL bRedraw) {
    spdlog::trace("{} this={} ac={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), static_cast<int>(ac), bRedraw);
}

void SECMDIFrameWnd::SwapMenu(UINT nID) {
    spdlog::trace("{} this={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID);
}

BOOL SECMDIFrameWnd::LoadAdditionalMenus(UINT nCount, UINT nIDMenu, ...) {

    spdlog::trace("{} this={} nCount={} nIDMenu={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCount, nIDMenu);
    return FALSE;
}

void SECMDIFrameWnd::EnableOleContainmentMode() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

afx_msg LRESULT SECMDIFrameWnd::OnExtendContextMenu(WPARAM wParam, LPARAM lParam) {

    spdlog::trace("{} this={} wParam={} lParam={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), wParam, lParam);
    return 0;
}

SECWorkspaceManagerEx* SECMDIFrameWnd::InitWorkspaceMgrEx(const CString& strAppKey, BOOL bRegistryMode, CRuntimeClass* pWSClass, BOOL bSectionKey) {

    spdlog::trace("{} this={} strAppKey={} bRegistryMode={} pWSClass={} bSectionKey={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strAppKey.GetString(), bRegistryMode, spdlog::fmt_lib::ptr(pWSClass), bSectionKey);
    return nullptr;
}

void SECMDIFrameWnd::EnableBmpMenus() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}
