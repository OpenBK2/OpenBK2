#include "Toolkit/swinfrm.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


SECFrameWnd::SECFrameWnd() : m_pControlBarManager(nullptr) {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

BOOL SECFrameWnd::GetActiveState() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return m_bActive;
}

void SECFrameWnd::EnableDocking(DWORD dwDockStyle, DWORD dwDockStyleEx) {
    spdlog::debug("{} this={} dwDockStyle={} dwDockStyleEx={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwDockStyle, dwDockStyleEx);
}

CDockBar* SECFrameWnd::CreateNewDockBar() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECFrameWnd::EnableContextListMode(BOOL bEnable) {
    spdlog::debug("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
}

void SECFrameWnd::FloatControlBar(CControlBar* pBar, CPoint point, DWORD dwStyle) {
    spdlog::debug("{} this={} pBar={} point.x={} point.y={} dwStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBar), point.x, point.y, dwStyle);
}

void SECFrameWnd::DockControlBarEx(CControlBar* pBar, UINT nDockBarID, int nCol, int nRow, float fPctWidth, int nHeight) {
    spdlog::debug("{} this={} pBar={} nDockBarID+{} nCol={} nRow={} fPctWith={} nHeight={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBar), nDockBarID, nCol, nRow, fPctWidth, nHeight);
}

void SECFrameWnd::CreateCaptionAppFont(CFont& font) {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECFrameWnd::CreateCaptionDocFont(CFont& font) {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECFrameWnd::DrawCaptionText() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECFrameWnd::SwapMenu(UINT nID) {
    spdlog::debug("{} this={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID);
}

BOOL SECFrameWnd::HasMenuBar() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

SECMenuBar* SECFrameWnd::GetMenuBar() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECFrameWnd::SetMenuBar(SECMenuBar* pMenuBar) {
    spdlog::debug("{} this={} pMenuBar={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pMenuBar));
}

CMenu* SECFrameWnd::GetMenu() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

SECWorkspaceManagerEx* SECFrameWnd::InitWorkspaceMgrEx(const CString& strAppKey, BOOL bRegistryMode, CRuntimeClass* pWSClass, BOOL bSectionKey) {
    spdlog::debug("{} this={} strAppKey={} bRegistryMode={}, pWSClass={}, bSectionKey={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strAppKey.GetString(), bRegistryMode, spdlog::fmt_lib::ptr(pWSClass), bSectionKey);
    return nullptr;
}

void SECFrameWnd::EnableBmpMenus() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}
