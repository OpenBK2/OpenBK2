#include "Toolkit/tbarpage.h"
#include "Toolkit/tbarmgr.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


SECToolBarSheet::SECToolBarSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage) {
    spdlog::trace("{} this={} nIDCaption={} pParentWnd={} iSelectPage={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDCaption, spdlog::fmt_lib::ptr(pParentWnd), iSelectPage);
}

SECToolBarCmdPage::SECToolBarCmdPage() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

SECToolBarCmdPage::SECToolBarCmdPage(UINT nIDTemplate, UINT nIDCaption) {
    spdlog::trace("{} this={} nIDTemplate={} nIDCaption={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDTemplate, nIDCaption);
}

SECToolBarCmdPage::SECToolBarCmdPage(LPCTSTR lpszTemplate, UINT nIDCaption) {
    spdlog::trace("{} this={} lpszTemplate={} nIDCaption={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszTemplate, nIDCaption);
}

void SECToolBarCmdPage::SetManager(SECToolBarManager* pManager) {
    spdlog::trace("{} this={} pManager={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pManager));
}

void SECToolBarCmdPage::DefineBtnGroup(LPCTSTR lpszTitle, int nBtnCount, UINT* lpBtnIDs) {
    spdlog::trace("{} this={} nIDCaption={} pParentWnd={} iSelectPage={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszTitle, nBtnCount, spdlog::fmt_lib::ptr(lpBtnIDs));
}

void SECToolBarCmdPage::DefineMenuGroup(LPCTSTR lpszTitle) {
    spdlog::trace("{} this={} lpszTitle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszTitle);
}
