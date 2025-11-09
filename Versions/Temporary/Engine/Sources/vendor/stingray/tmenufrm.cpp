#include "Toolkit/tmenufrm.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>


SECMenuBar::SECMenuBar() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

SECMenuBar::~SECMenuBar() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

BOOL SECMenuBar::LoadMenu(UINT nIDResource) {
    spdlog::trace("{} this={} nIDResource={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDResource);
    return FALSE;
}

BOOL SECMenuBar::SwitchMenu(UINT nIDResource) {
    spdlog::trace("{} this={} nIDResource={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDResource);
    return FALSE;
}

void SECMenuBar::EnableBitFlag(DWORD dwBit, BOOL bUpdate) {
    spdlog::trace("{} this={} dwBit={} bUpdate={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwBit, bUpdate);
}

void SECMenuBar::OnBarStyleChange(DWORD dwOldStyle, DWORD dwNewStyle) {
    spdlog::trace("{} this={} dwOldStyle={} dwNewStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwOldStyle, dwNewStyle);
}

HFONT SECMenuBar::GetMenuFont() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECMenuBar::ResetMenus(BOOL bNoUpdate) {
    spdlog::trace("{} this={} bNoUpdate={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bNoUpdate);
}

BOOL SECMenuBar::SetMenuInfo(int nCount, UINT nIDMenu, ...) {
    spdlog::trace("{} this={} nCount={} nIDMenu={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCount, nIDMenu);
    return FALSE;
}

UINT SECMenuBar::GetCurMenuID() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}
