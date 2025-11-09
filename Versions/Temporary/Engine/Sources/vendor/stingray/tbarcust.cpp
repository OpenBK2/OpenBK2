#include "Toolkit/tbarcust.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>


BOOL SECCustomToolBar::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, UINT nID, DWORD dwStyle, DWORD dwExStyle, const RECT& rect, CWnd* pParentWnd, CCreateContext* pContext) {
    spdlog::trace("{} this={} lpszClassName={} lpszWindowName={} nID={} dwStyle={} dwExStyle={} "
                  "rect.left={} rect.top={} rect.right={} rect.bottom={} "
                  "pParentWnd={} pContext={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszClassName, lpszWindowName, nID, dwStyle, dwExStyle,
                  rect.left, rect.top, rect.right, rect.bottom,
                  spdlog::fmt_lib::ptr(pParentWnd), spdlog::fmt_lib::ptr(pContext));
    return TRUE;
}

BOOL SECCustomToolBar::CreateEx(DWORD dwExStyle, CWnd* pParentWnd, DWORD dwStyle, UINT nID, LPCTSTR lpszTitle) {
    spdlog::trace("{} this={} dwExStyle={} pParentWnd={} dwStyle={} nID={} lpszTitle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwExStyle, spdlog::fmt_lib::ptr(pParentWnd), dwStyle, nID, lpszTitle);
    return FALSE;
}

void SECCustomToolBar::SetBarInfoEx(SECControlBarInfo* pInfo, CFrameWnd* pFrameWnd) {
    spdlog::trace("{} this={} pInfo={} pFrameWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pInfo), spdlog::fmt_lib::ptr(pFrameWnd));
}

void SECCustomToolBar::SetButtonStyle(int nIndex, UINT nStyle) {
    spdlog::trace("{} this={} nIndex={} nStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, nStyle);
}

UINT SECCustomToolBar::GetButtonStyle(int nIndex) const {
    spdlog::trace("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    return 0;
}

BOOL SECCustomToolBar::RemoveButton(int nIndex, BOOL bNoUpdate) {
    spdlog::trace("{} this={} nIndex={} bNoUpdate={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, bNoUpdate);
    return FALSE;
}

void SECCustomToolBar::AddButton(int nIndex, int nID, BOOL bSeparator, BOOL bNoUpdate) {
    spdlog::trace("{} this={} nIndex={} nID={} bSeparator={} bNoUpdate={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex, nID, bSeparator, bNoUpdate);
}

int SECCustomToolBar::GetBtnCount() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

BOOL SECCustomToolBar::InConfigMode() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECCustomToolBar::InAltDragMode() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

int SECCustomToolBar::CommandToIndex(UINT nID) const {
    spdlog::trace("{} this={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID);
    return 0;
}

UINT SECCustomToolBar::GetItemID(int nIndex) const {
    spdlog::trace("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    return 0;
}

int SECCustomToolBar::GetCurBtn() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

int SECCustomToolBar::IDToBmpIndex(UINT nID, HBITMAP* lphBmp) {
    spdlog::trace("{} this={} nID={} lphBmp={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID, spdlog::fmt_lib::ptr(lphBmp));
    return 0;
}

BOOL SECCustomToolBar::LoadToolBar(LPCTSTR lpszResourceName) {
    spdlog::trace("{} this={} lpszResourceName={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszResourceName);
    return FALSE;
}

BOOL SECCustomToolBar::LoadBitmap(UINT nIDResource, const UINT* lpIDArray, int nIDCount) {
    spdlog::trace("{} this={} nIDResource={} lpIDArray={} nIDCount={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDResource, spdlog::fmt_lib::ptr(lpIDArray), nIDCount);
    return FALSE;
}

BOOL SECCustomToolBar::SetButtons(const UINT* lpIDArray, int nIDCount) {
    spdlog::trace("{} this={} lpIDArray={} nIDCount={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(lpIDArray), nIDCount);
    return FALSE;
}

void SECCustomToolBar::GetItemRect(int nIndex, LPRECT lpRect) const {
    spdlog::trace("{} nIndex={} lpRect={}", BOOST_CURRENT_FUNCTION, nIndex, spdlog::fmt_lib::ptr(lpRect));
}

void SECCustomToolBar::InformBtns(UINT nID, UINT nCode, void* pData, BOOL bPass) {
    spdlog::trace("{} this={} nID={} nCode={} pData={} bPass={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nID, nCode, spdlog::fmt_lib::ptr(pData), bPass);
}

void SECCustomToolBar::BalanceWrap(int nRow, Wrapped* pWrap) {
    spdlog::trace("{} this={} nRow={} pWrap={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nRow, spdlog::fmt_lib::ptr(pWrap));
}

BOOL SECCustomToolBar::GetDragMode() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECCustomToolBar::AcceptDrop() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}
