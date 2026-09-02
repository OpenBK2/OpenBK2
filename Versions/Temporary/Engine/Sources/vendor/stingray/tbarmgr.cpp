#include "Toolkit/tbarmgr.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


SECToolBarManager::SECToolBarManager() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}
SECToolBarManager::SECToolBarManager(CFrameWnd* pFrameWnd, CFrameWnd* pOwnerFrame) {
    spdlog::trace("{} this={} pFrameWnd={} pOwnerFrame={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pFrameWnd), spdlog::fmt_lib::ptr(pOwnerFrame));
}

void SECToolBarManager::DefineDefaultToolBar(UINT nID, const CString& strTitle, UINT nBtnCount, UINT* lpBtnIDs, DWORD dwAlignment, UINT nDockBarID, UINT nDockNextToID, BOOL bDocked, BOOL bVisible) {
    spdlog::trace("{} this={} nID={} strTitle={} nBtnCount={} lpBtnIDs={} dwAlignment={} nDockBarID={} nDockNextToID={} bDocked={} bVisible={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this),
    nID, strTitle.GetString(), nBtnCount, spdlog::fmt_lib::ptr(lpBtnIDs), dwAlignment, nDockBarID, nDockNextToID, bDocked, bVisible);
}

void SECToolBarManager::DefineDefaultToolBar(UINT nID, const CString& strTitle, UINT nToolbarID, UINT& nRetButtonCount, UINT*& pRetButtonArray, DWORD dwAlignment, UINT nDockBarID, UINT nDockNextToID, BOOL bDocked, BOOL bVisible) {
    spdlog::trace("{} this={} nID={} strTitle={} nToolbarID={} nRetButtonCount={} pRetButtonArray={} dwAlignment={} nDockBarID={} nDockNextToID={} bDocked={} bVisible={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this),
    nID, strTitle.GetString(), nToolbarID, nRetButtonCount, spdlog::fmt_lib::ptr(pRetButtonArray), dwAlignment, nDockBarID, nDockNextToID, bDocked, bVisible);
}

BOOL SECToolBarManager::IsToolBarCommand(CRect& rect) const {
    spdlog::trace("{} this={} rect.left={} rect.top={} rect.right={} rect.bottom={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this),
         rect.left, rect.top, rect.right, rect.bottom);
    return FALSE;
}

SECCustomToolBar* SECToolBarManager::ToolBarUnderRect(const CRect& rect) const {
    spdlog::trace("{} this={} rect.left={} rect.top={} rect.right={} rect.bottom={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this),
        rect.left, rect.top, rect.right, rect.bottom);
    return nullptr;
}

SECCustomToolBar* SECToolBarManager::ToolBarFromID(const UINT nToolBarID) const {
    spdlog::trace("{} this={} nToolBarID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nToolBarID);
    return nullptr;
}

SECCustomToolBar* SECToolBarManager::CreateUserToolBar(LPCTSTR lpszTitle) {
    spdlog::trace("{} this={} lpszTitle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszTitle);
    return nullptr;
}

BOOL SECToolBarManager::LoadToolBarResource(LPCTSTR lpszStdBmpName, LPCTSTR lpszLargeBmpName) {
    spdlog::trace("{} this={} lpszStdBmpName={} lpszLargeBmpName={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszStdBmpName, lpszLargeBmpName);
    return FALSE;
}

BOOL SECToolBarManager::LoadToolBarResource(UINT nIDStdBmp, UINT nIDLargeBmp) {
    spdlog::trace("{} this={} nIDStdBmp={} nIDLargeBmp={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDStdBmp, nIDLargeBmp);
    return FALSE;
}

BOOL SECToolBarManager::LoadToolBarResource() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECToolBarManager::AddToolBarResource(LPCTSTR lpszStdBmpName, LPCTSTR lpszLargeBmpName) {
    spdlog::trace("{} this={} lpszStdBmpName={} lpszLargeBmpName={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(lpszStdBmpName), spdlog::fmt_lib::ptr(lpszLargeBmpName));
    return FALSE;
}

BOOL SECToolBarManager::AddToolBarResource(UINT nIDStdBmp, UINT nIDLargeBmp) {
    spdlog::trace("{} this={} nIDStdBmp={} nIDLargeBmp={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDStdBmp, nIDLargeBmp);
    return FALSE;
}

BOOL SECToolBarManager::AddBitmapResource(LPCTSTR lpszStdBmpName, LPCTSTR lpszLargeBmpName, const UINT* lpIDArray, UINT nIDCount) {
    spdlog::trace("{} this={} lpszStdBmpName={} lpszLargeBmpName={} lpIDArray={} nIDCount={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszStdBmpName, lpszLargeBmpName, spdlog::fmt_lib::ptr(lpIDArray), nIDCount);
    return FALSE;
}

BOOL SECToolBarManager::AddBitmapResource(UINT nIDstdBmpName, UINT nIDLargeBmpName, const UINT* lpIDArray, UINT nIDCount) {
    spdlog::trace("{} this={} nIDstdBmpName={} nIDLargeBmpName={} lpIDArray={} nIDCount={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDstdBmpName, nIDLargeBmpName, spdlog::fmt_lib::ptr(lpIDArray), nIDCount);
    return FALSE;
}

BOOL SECToolBarManager::AddBitmap(HBITMAP hBmpSmall, HBITMAP hBmpLarge, const UINT* lpIDArray, UINT nIDCount) {
    spdlog::trace("{} this={} hBmpSmall={} hBmpLarge={} lpIDArray={} nIDCount={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hBmpSmall), spdlog::fmt_lib::ptr(hBmpLarge), spdlog::fmt_lib::ptr(lpIDArray), nIDCount);
    return FALSE;
}

int SECToolBarManager::ExecViewToolBarsDlg() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

void SECToolBarManager::InformBtns(UINT nID, UINT nCode, void* pData) {
    spdlog::trace("{} this={} nID={} nCode={} pData={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID, nCode, spdlog::fmt_lib::ptr(pData));
}

void SECToolBarManager::EnableToolTips(BOOL bEnable) {
    spdlog::trace("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
}

void SECToolBarManager::EnableFlyBy(BOOL bEnable) {
    spdlog::trace("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
}

BOOL SECToolBarManager::ToolTipsEnabled() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECToolBarManager::FlyByEnabled() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

void SECToolBarManager::EnableLargeBtns(BOOL bEnable) {
    spdlog::trace("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
}

BOOL SECToolBarManager::LargeBtnsEnabled() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

void SECToolBarManager::EnableCoolLook(BOOL bEnable, DWORD dwExCoolLookStyles) {
    spdlog::trace("{} this={} bEnable={} dwExCoolLookStyles={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable, dwExCoolLookStyles);
}

BOOL SECToolBarManager::CoolLookEnabled() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

void SECToolBarManager::SetButtonMap(const SECBtnMapEntry* pMap) {
    spdlog::trace("{} this={} pMap={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pMap));
}

const SECBtnMapEntry* SECToolBarManager::GetButtonMap() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECToolBarManager::SetDefaultDockState() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

BOOL SECToolBarManager::SetMenuInfo(int nCount, UINT nIDMenu, ...) {
    spdlog::trace("{} this={} nCount={} nIDMenu={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCount, nIDMenu);
    return FALSE;
}

void SECToolBarManager::LoadState(const CString & state) {
    spdlog::trace("{} this={} state={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), state.GetString());
}

void SECToolBarManager::SaveState(const CString & state) {
    spdlog::trace("{} this={} state={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), state.GetString());
}
