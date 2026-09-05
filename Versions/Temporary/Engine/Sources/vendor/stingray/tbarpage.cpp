#include "Toolkit/tbarpage.h"
#include "Toolkit/tbarmgr.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


SECToolBarSheet::SECToolBarSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage) {
    spdlog::debug("{} this={} nIDCaption={} pParentWnd={} iSelectPage={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDCaption, spdlog::fmt_lib::ptr(pParentWnd), iSelectPage);
}

SECToolBarCmdPage::SECToolBarCmdPage() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

SECToolBarCmdPage::SECToolBarCmdPage(UINT nIDTemplate, UINT nIDCaption) {
    spdlog::debug("{} this={} nIDTemplate={} nIDCaption={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDTemplate, nIDCaption);
}

SECToolBarCmdPage::SECToolBarCmdPage(LPCTSTR lpszTemplate, UINT nIDCaption) {
    spdlog::debug("{} this={} lpszTemplate={} nIDCaption={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), SafeString( lpszTemplate ), nIDCaption);
}

// The three below keep what they are given and draw none of it.
//
// CMainFrame::OnToolsCustomize describes the whole dialog before opening it:
// the manager, one button group per toolbar, the custom toolbars from the
// profile, and the menus as a group of their own. That description was being
// dropped, so even once the pages can be shown there would have been nothing to
// put in them. Now the contents are ready and only the showing is missing --
// see SECToolBarSheet for what that needs.
void SECToolBarCmdPage::SetManager(SECToolBarManager* pManager) {
    spdlog::debug("{} this={} pManager={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pManager));
    m_pManager = pManager;
}

void SECToolBarCmdPage::DefineBtnGroup(LPCTSTR lpszTitle, int nBtnCount, UINT* lpBtnIDs) {
    spdlog::debug("{} this={} lpszTitle={} nBtnCount={} lpBtnIDs={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), SafeString( lpszTitle ), nBtnCount, spdlog::fmt_lib::ptr(lpBtnIDs));
    CmdGroup group;
    group.strTitle = ( lpszTitle != nullptr ) ? lpszTitle : _T("");
    group.bMenu = false;
    for ( int i = 0; i < nBtnCount && lpBtnIDs != nullptr; ++i ) {
        // Separators carry no command and are not offered as buttons.
        if ( lpBtnIDs[i] != 0 ) {
            group.btnIDs.push_back( lpBtnIDs[i] );
        }
    }
    m_groups.push_back( group );
}

void SECToolBarCmdPage::DefineMenuGroup(LPCTSTR lpszTitle) {
    spdlog::debug("{} this={} lpszTitle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), SafeString( lpszTitle ));
    CmdGroup group;
    group.strTitle = ( lpszTitle != nullptr ) ? lpszTitle : _T("");
    // The commands come from the manager's menu resources rather than from a
    // list handed in here; see SECToolBarManager::SetMenuInfo.
    group.bMenu = true;
    m_groups.push_back( group );
}
