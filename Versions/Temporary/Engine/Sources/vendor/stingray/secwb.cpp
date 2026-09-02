#include "Toolkit/secwb.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


void SECWorkbook::AddSheet(SECWorksheet* pSheet) {
    spdlog::debug("{} this={} pSheet={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pSheet));
}

void SECWorkbook::RemoveSheet(SECWorksheet* pSheet) {
    spdlog::debug("{} this={} pSheet={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pSheet));
}

const TCHAR* SECWorkbook::GetTabLabel(SECWorksheet* pSheet) const {
    spdlog::debug("{} this={} pSheet={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pSheet));
    return nullptr;
}

void SECWorkbook::OnDrawTab(CDC* pDC, SECWorksheet* pSheet) {
    spdlog::debug("{} this={} pDC={} pSheet={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pDC), spdlog::fmt_lib::ptr(pSheet));
}

void SECWorkbook::OnDrawTabIconAndLabel(CDC* pDC, SECWorksheet* pSheet) {
    spdlog::debug("{} this={} pDC={} pSheet={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pDC), spdlog::fmt_lib::ptr(pSheet));
}

void SECWorkbook::SetWorkbookMode(BOOL bEnabled) {
    spdlog::debug("{} this={} bEnabled={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnabled);
}

void SECWorkbook::SetShowIcons(BOOL bShowIcons) {
    spdlog::debug("{} this={} bShowIcons={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bShowIcons);
}

HICON SECWorkbook::GetTabIcon(SECWorksheet* pSheet) const {
    spdlog::debug("{} this={} pSheet={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pSheet));
    return nullptr;
}

BOOL SECWorkbook::SetTabIcon(SECWorksheet* pSheet, HICON hIcon, BOOL bRedraw) {
    spdlog::debug("{} this={} pSheet={} hIcon={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pSheet), spdlog::fmt_lib::ptr(hIcon), bRedraw);
    return FALSE;
}

BOOL SECWorkbook::LookupSheet(SECWorksheet* pSheet, int& nIndex) {
    spdlog::debug("{} this={} pSheet={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pSheet), nIndex);
    return FALSE;
}

SECWorksheet* SECWorkbook::GetWorksheet(int nSheet) const {
    spdlog::debug("{} this={} nSheet={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nSheet);
    return nullptr;
}

int SECWorkbook::GetSheetCount() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}
