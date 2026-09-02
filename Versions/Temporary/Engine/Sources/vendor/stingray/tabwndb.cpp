#include "Toolkit/tabwndb.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


SECTab* SECTabWndBase::InsertTab(CWnd* pWnd, int nIndex, LPCTSTR lpszLabel) {
    spdlog::debug("{} this={} pWnd={} nIndex={} lpszLabel={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd), nIndex, lpszLabel);

    return nullptr;
}

SECTab* SECTabWndBase::InsertTab(CRuntimeClass* pViewClass, int nIndex, LPCTSTR lpszLabel, CCreateContext* pContext, UINT nID) {
    spdlog::debug("{} this={} pViewClass={} lpszLabel={} pContext={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pViewClass), lpszLabel, spdlog::fmt_lib::ptr(pContext), nID);
    return nullptr;
}

SECTab* SECTabWndBase::AddTab(CWnd* pWnd, LPCTSTR lpszLabel) {
    spdlog::debug("{} this={} pWnd={} lpszLabel={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd), lpszLabel);
    return nullptr;
}

SECTab* SECTabWndBase::AddTab(CRuntimeClass* pViewClass, LPCTSTR lpszLabel, CCreateContext* pContext, UINT nID) {
    spdlog::debug("{} this={} pViewClass={} lpszLabel={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pViewClass), lpszLabel, nID);
    return nullptr;
}

void SECTabWndBase::SetTabIcon(int nIndex, HICON hIcon) {
    spdlog::debug("{} this={} nIndex={} hIcon={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, spdlog::fmt_lib::ptr(hIcon));
}

void SECTabWndBase::SetTabIcon(int nIndex, UINT nIDIcon, int cx, int cy) {
    spdlog::debug("{} this={} nIndex={} nIDIcon={} cx={} cy={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, nIDIcon, cx, cy);
}

void SECTabWndBase::SetTabIcon(int nIndex, LPCTSTR lpszResourceName, int cx, int cy) {
    spdlog::debug("{} this={} nIndex={} lpszResourceName={} cx={} cy={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, lpszResourceName, cx, cy);
}

void SECTabWndBase::RemoveTab(CWnd* pWnd) {
    spdlog::debug("{} this={} pWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd));
}

void SECTabWndBase::RemoveTab(int nIndex) {
    spdlog::debug("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
}

void SECTabWndBase::RenameTab(CWnd* pWnd, LPCTSTR lpszLabel) {
    spdlog::debug("{} this={} pWnd={} lpszLabel={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd), lpszLabel);
}

void SECTabWndBase::RenameTab(int nIndex, LPCTSTR lpszLabel) {
    spdlog::debug("{} this={} nIndex={} lpszLabel={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, lpszLabel);
}

BOOL SECTabWndBase::ActivateTab(CWnd* pWnd, int nIndex) {
    spdlog::debug("{} this={} pWnd={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd), nIndex);
    return FALSE;
}

BOOL SECTabWndBase::ActivateTab(CWnd* pWnd) {
    spdlog::debug("{} this={} pWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd));
    return FALSE;
}

BOOL SECTabWndBase::ActivateTab(int nIndex) {
    spdlog::debug("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    return FALSE;
}

void SECTabWndBase::ClearSelection() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECTabWndBase::ScrollToTab(CWnd* pWnd) {
    spdlog::debug("{} this={} pWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd));
}

void SECTabWndBase::ScrollToTab(int nIndex) {
    spdlog::debug("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
}

int SECTabWndBase::GetTabCount() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

BOOL SECTabWndBase::GetTabInfo(int nIndex, LPCTSTR& lpszLabel, BOOL& bSelected, CWnd*& pWnd, void*& pExtra) const {
    spdlog::debug("{} this={} nIndex={} lpszLabel={} bSelected={} pWnd={} pExtra={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, lpszLabel, bSelected, spdlog::fmt_lib::ptr(pWnd), spdlog::fmt_lib::ptr(pExtra));
    return FALSE;
}

BOOL SECTabWndBase::FindTab(const CWnd* const pWnd, int& nTab) const {
    spdlog::debug("{} this={} pWnd={} nTab={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd), nTab);
    return FALSE;
}

BOOL SECTabWndBase::GetActiveTab(CWnd*& pWnd) const {
    spdlog::debug("{} this={} pWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd));
    return FALSE;
}

BOOL SECTabWndBase::GetActiveTab(int& nIndex) const {
    spdlog::debug("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    return FALSE;
}
BOOL SECTabWndBase::TabExists(CWnd* pClient) const {
    spdlog::debug("{} this={} pClient={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pClient));
    return FALSE;
}

BOOL SECTabWndBase::TabExists(int nTab) const {
    spdlog::debug("{} this={} nTab={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nTab);
    return FALSE;
}

const SECTabControlBase* SECTabWndBase::GetTabControl() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}
