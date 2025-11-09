#include "Toolkit/swinfrm.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>


SECFrameWnd::SECFrameWnd() : m_pControlBarManager(nullptr) {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

BOOL SECFrameWnd::GetActiveState() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return m_bActive;
}

void SECFrameWnd::EnableDocking(DWORD dwDockStyle, DWORD dwDockStyleEx) {
    spdlog::trace("{} this={} dwDockStyle={} dwDockStyleEx={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwDockStyle, dwDockStyleEx);
}

CDockBar* SECFrameWnd::CreateNewDockBar() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECFrameWnd::EnableContextListMode(BOOL bEnable) {
    spdlog::trace("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
}

void SECFrameWnd::FloatControlBar(CControlBar* pBar, CPoint point, DWORD dwStyle) {
    spdlog::trace("{} this={} pBar={} point.x={} point.y={} dwStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBar), point.x, point.y, dwStyle);
}

void SECFrameWnd::DockControlBarEx(CControlBar* pBar, UINT nDockBarID, int nCol, int nRow, float fPctWidth, int nHeight) {
    spdlog::trace("{} this={} pBar={} nDockBarID+{} nCol={} nRow={} fPctWith={} nHeight={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBar), nDockBarID, nCol, nRow, fPctWidth, nHeight);
}

void SECFrameWnd::CreateCaptionAppFont(CFont& font) {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECFrameWnd::CreateCaptionDocFont(CFont& font) {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECFrameWnd::DrawCaptionText() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECFrameWnd::SwapMenu(UINT nID) {
    spdlog::trace("{} this={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID);
}

BOOL SECFrameWnd::HasMenuBar() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

SECMenuBar* SECFrameWnd::GetMenuBar() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECFrameWnd::SetMenuBar(SECMenuBar* pMenuBar) {
    spdlog::trace("{} this={} pMenuBar={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pMenuBar));
}

CMenu* SECFrameWnd::GetMenu() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

SECWorkspaceManagerEx* SECFrameWnd::InitWorkspaceMgrEx(const CString& strAppKey, BOOL bRegistryMode, CRuntimeClass* pWSClass, BOOL bSectionKey) {
    spdlog::trace("{} this={} strAppKey={} bRegistryMode={}, pWSClass={}, bSectionKey={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strAppKey.GetString(), bRegistryMode, spdlog::fmt_lib::ptr(pWSClass), bSectionKey);
    return nullptr;
}

void SECFrameWnd::EnableBmpMenus() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}
