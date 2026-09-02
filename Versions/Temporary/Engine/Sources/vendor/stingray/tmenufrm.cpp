#include "Toolkit/tmenufrm.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"

#include <cstdarg>


SECMenuBar::SECMenuBar() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

SECMenuBar::~SECMenuBar() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

// Load the menus this bar is to offer, and keep them.
//
// Loaded here rather than in SwitchMenu because here is where the caller has
// the right module selected: CEditorAppSpecific::CreateMenus wraps its call in
// AfxSetResourceHandle for ED_B2_M1, which is the DLL IDM_MAIN, IDM_MAPINFO
// and IDM_MODEL live in. By the time an editor asks to switch to one the
// handle is the executable's again, where those menus are not.
BOOL SECMenuBar::SetMenus(const std::vector<UINT> &menus) {
    for (const UINT nID : menus) {
        if (nID == 0) {
            continue;
        }
        const HMENU hMenu = ::LoadMenu(AfxGetResourceHandle(), MAKEINTRESOURCE(nID));
        spdlog::debug("SECMenuBar::SetMenus: menu {} loaded as {}", nID, spdlog::fmt_lib::ptr(hMenu));
        if (hMenu != nullptr) {
            m_menus.emplace_back(nID, hMenu);
        }
    }
    return !m_menus.empty();
}

// This bar draws nothing, so it takes no room. The menu goes on the frame.
CSize SECMenuBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz) {
    spdlog::debug("{} this={} bStretch={} bHorz={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bStretch, bHorz);
    return CSize(0, 0);
}

CSize SECMenuBar::CalcDynamicLayout(int nLength, DWORD dwMode) {
    spdlog::debug("{} this={} nLength={} dwMode={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nLength, dwMode);
    return CSize(0, 0);
}

BOOL SECMenuBar::LoadMenu(UINT nIDResource) {
    spdlog::debug("{} this={} nIDResource={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDResource);
    return FALSE;
}

// Show one of the menus SetMenuInfo named.
//
// The toolkit draws its own menu bar, which is what makes it customizable and
// what this library does not do. The menu goes on the frame instead, through
// Windows' own menu bar: the same menus in a plainer frame.
//
// MDISetMenu rather than SetMenu, because MFC keeps an MDI frame's menu and
// its Window menu separately and fills the second in with the open documents.
// Null for the second leaves it alone.
BOOL SECMenuBar::SwitchMenu(UINT nIDResource) {
    spdlog::debug("{} this={} nIDResource={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDResource);

    HMENU hMenu = nullptr;
    for (const auto &entry : m_menus) {
        if (entry.first == nIDResource) {
            hMenu = entry.second;
            break;
        }
    }
    if (hMenu == nullptr) {
        // Not one it was told about. Worth trying, for a menu that does live in
        // whichever module is current, and worth saying out loud either way.
        hMenu = ::LoadMenu(AfxGetResourceHandle(), MAKEINTRESOURCE(nIDResource));
        if (hMenu == nullptr) {
            spdlog::warn("SECMenuBar::SwitchMenu: no menu {} here or in SetMenuInfo", nIDResource);
            return FALSE;
        }
        m_menus.emplace_back(nIDResource, hMenu);
    }

    CWnd *pMainWnd = AfxGetMainWnd();
    if (pMainWnd == nullptr || pMainWnd->GetSafeHwnd() == nullptr) {
        return FALSE;
    }
    m_nCurMenuID = nIDResource;
    if (CMDIFrameWnd *pFrame = DYNAMIC_DOWNCAST(CMDIFrameWnd, pMainWnd)) {
        pFrame->MDISetMenu(CMenu::FromHandle(hMenu), nullptr);
        pFrame->DrawMenuBar();
        return TRUE;
    }
    pMainWnd->SetMenu(CMenu::FromHandle(hMenu));
    pMainWnd->DrawMenuBar();
    return TRUE;
}

void SECMenuBar::EnableBitFlag(DWORD dwBit, BOOL bUpdate) {
    spdlog::debug("{} this={} dwBit={} bUpdate={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwBit, bUpdate);
}

void SECMenuBar::OnBarStyleChange(DWORD dwOldStyle, DWORD dwNewStyle) {
    spdlog::debug("{} this={} dwOldStyle={} dwNewStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwOldStyle, dwNewStyle);
}

HFONT SECMenuBar::GetMenuFont() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECMenuBar::ResetMenus(BOOL bNoUpdate) {
    spdlog::debug("{} this={} bNoUpdate={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bNoUpdate);
}

BOOL SECMenuBar::SetMenuInfo(int nCount, UINT nIDMenu, ...) {
    spdlog::debug("{} this={} nCount={} nIDMenu={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCount, nIDMenu);
    std::vector<UINT> menus;
    if (nCount > 0) {
        menus.push_back(nIDMenu);
        va_list args;
        va_start(args, nIDMenu);
        for (int i = 1; i < nCount; ++i) {
            menus.push_back(va_arg(args, UINT));
        }
        va_end(args);
    }
    return SetMenus(menus);
}

UINT SECMenuBar::GetCurMenuID() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return m_nCurMenuID;
}
