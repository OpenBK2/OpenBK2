#include "Toolkit/tabwnd3.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


SEC3DTabWnd::SEC3DTabWnd() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

BOOL SEC3DTabWnd::Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID) {
    spdlog::debug("{} this={} pParentWnd={} dwStyle={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pParentWnd), dwStyle, nID);
    RECT rect{0, 0, 0, 0};
    LPCTSTR lpszClassName = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1), ::LoadIcon(nullptr, IDI_APPLICATION));
    return SECTabWndBase::Create(lpszClassName, "SEC3DTabWnd", dwStyle, rect, pParentWnd, nID, nullptr);
}

DWORD SEC3DTabWnd::GetTabStyle() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

DWORD SEC3DTabWnd::SetTabStyle(DWORD dwTabStyle) {
    spdlog::debug("{} this={} dwTabStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwTabStyle);
    return 0;
}

void SEC3DTabWnd::EnableTab(CWnd* pWnd, BOOL bEnable) {
    spdlog::debug("{} this={} pWnd={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd), bEnable);
}

void SEC3DTabWnd::EnableTab(int nIndex, BOOL bEnable) {
    spdlog::debug("{} this={} nIndex={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, bEnable);
}

BOOL SEC3DTabWnd::SetFontActiveTab(CFont* pFont, BOOL bRedraw) {
    spdlog::debug("{} this={} pFont={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pFont), bRedraw);
    return FALSE;
}

BOOL SEC3DTabWnd::SetFontInactiveTab(CFont* pFont, BOOL bRedraw) {
    spdlog::debug("{} this={} pFont={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pFont), bRedraw);
    return FALSE;
}

CFont* SEC3DTabWnd::GetFontActiveTab() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

CFont* SEC3DTabWnd::GetFontInactiveTab() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}


BOOL SEC3DTabWnd::IsTabEnabled(CWnd* pWnd) {
    spdlog::debug("{} this={} pWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd));
    return FALSE;
}

BOOL SEC3DTabWnd::IsTabEnabled(int nIndex) {
    spdlog::debug("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    return FALSE;
}
