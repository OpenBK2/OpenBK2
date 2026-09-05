#include "Toolkit/tbarsdlg.h"
#include "Toolkit/tbarmgr.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


SECToolBarsPage::SECToolBarsPage() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

// Kept, not drawn. This page is meant to list the manager's toolbars with a
// check box each; it has the manager now and no template to draw on. See
// SECToolBarSheet.
void SECToolBarsPage::SetManager(SECToolBarManager* pManager) {
    spdlog::debug("{} this={} pManager={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pManager));
    m_pManager = pManager;
}
