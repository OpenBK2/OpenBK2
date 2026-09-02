#include "Toolkit/progress.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


SECProgressCtrl::SECProgressCtrl() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

BOOL SECProgressCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, DWORD dwExStyle) {
    spdlog::debug("{} this={} dwStyle={} rect.left={} rect.top={} rect.right={} rect.bottom={} pParentWnd={} nID={} dwExStyle={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwStyle, rect.left, rect.top, rect.right, rect.bottom, spdlog::fmt_lib::ptr(pParentWnd), nID, dwExStyle);
    return TRUE;
}

BOOL SECProgressCtrl::AttachProgress(int nCtlID, CWnd* pParentWnd) {
    spdlog::debug("{} this={} nCtlID={} pParentWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCtlID, spdlog::fmt_lib::ptr(pParentWnd));
    return TRUE;
}

void SECProgressCtrl::SetRange(ULONG ulLower, ULONG ulUpper) {
    spdlog::debug("{} this={} ulLower={} ulUpper={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), ulLower, ulUpper);
}

ULONG SECProgressCtrl::SetPos(ULONG ulPos, BOOL bYield) {
    spdlog::debug("{} this={} ulPos={} bYield={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), ulPos, bYield);
    return 0;
}

ULONG SECProgressCtrl::OffsetPos(ULONG ulPos, BOOL bYield) {
    spdlog::debug("{} this={} ulPos={} bYield={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), ulPos, bYield);
    return 0;
}

ULONG SECProgressCtrl::SetStep(ULONG ulStep) {
    spdlog::debug("{} this={} ulStep={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), ulStep);
    return 0;
}

void SECProgressCtrl::SetColors(COLORREF fgnd, COLORREF bgnd) {
    spdlog::debug("{} this={} fgnd={} bgnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), fgnd, bgnd);
}
void SECProgressCtrl::SetFont(CFont* pFont, BOOL bRedraw) {
    spdlog::debug("{} this={} pFont={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pFont), bRedraw);
}

void SECProgressCtrl::SetWindowText(LPCTSTR lpszNewText) {
    spdlog::debug("{} this={} lpszNewText={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszNewText);
}

void SECProgressCtrl::GetWindowText(CString& strText) {
    spdlog::debug("{} this={} strText={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strText.GetString());
}

void SECProgressCtrl::SetExStyle(DWORD dwExNewStyle) {
    spdlog::debug("{} this={} dwExNewStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwExNewStyle);
}

DWORD SECProgressCtrl::GetExStyle() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

ULONG SECProgressCtrl::StepIt(BOOL bYield) {
    spdlog::debug("{} this={} bYield={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bYield);
    return 0;
}

void SECProgressCtrl::ResetProgress() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

BOOL SECProgressCtrl::StepOnTimer(UINT nInterval) {
    spdlog::debug("{} this={} nInterval={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nInterval);
    return FALSE;
}

BOOL SECProgressCtrl::OnInitProgress() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

void SECProgressCtrl::OnPaintBarFill(CDC* pDC, CRect rectFill) {
    spdlog::debug("{} this={} pDC={} rectFill.left={} rectFill.top={} rectFill.right={} rectFill.bottom={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pDC), rectFill.left, rectFill.top, rectFill.right, rectFill.bottom);
}

void SECProgressCtrl::OnPaintBarEmpty(CDC* pDC, CRect rectEmpty) {
    spdlog::debug("{} this={} pDC={} rectEmpty.left={} rectEmpty.top={} rectEmpty.right={} rectEmpty.bottom={}",
            BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pDC), rectEmpty.left, rectEmpty.top, rectEmpty.right, rectEmpty.bottom);
}

void SECProgressCtrl::OnPaintBarText(CDC* pDC,float fPctComplete, CRect rectEmpty, CRect rectFilled) {
    spdlog::debug("{} this={} pDC={} fPctComplete={} "
                  "rectEmpty.left={} rectEmpty.top={} rectEmpty.right={} rectEmpty.bottom={} "
                  "rectFilled.left={} rectFilled.top={} rectFilled.right={} rectFilled.bottom={}",
            BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pDC), fPctComplete,
            rectEmpty.left, rectEmpty.top, rectEmpty.right, rectEmpty.bottom,
            rectFilled.left, rectFilled.top, rectFilled.right, rectFilled.bottom);
}

void SECProgressCtrl::OnDisplayStr(CString& strToDisplay) {
    spdlog::debug("{} this={} strToDisplay={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strToDisplay.GetString());
}

void SECProgressCtrl::DoPaint(CPaintDC* pdc) {
    spdlog::debug("{} this={} pdc={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pdc));
}

BOOL SECProgressCtrl::CalcProgressRects(float fPct, CRect& rectFilled, CRect& rectEmpty) {
    spdlog::debug("{} this={} fPct={} "
                      "rectEmpty.left={} rectEmpty.top={} rectEmpty.right={} rectEmpty.bottom={} "
                      "rectFilled.left={} rectFilled.top={} rectFilled.right={} rectFilled.bottom={}",
                BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), fPct,
                rectEmpty.left, rectEmpty.top, rectEmpty.right, rectEmpty.bottom,
                rectFilled.left, rectFilled.top, rectFilled.right, rectFilled.bottom);
    return FALSE;
}

BOOL SECProgressCtrl::PaintProgressBarAndText(float fPctComplete, CRect rectFilled, CRect rectEmpty,CDC* pdc) {
    spdlog::debug("{} this={} "
                      "rectEmpty.left={} rectEmpty.top={} rectEmpty.right={} rectEmpty.bottom={} "
                      "rectFilled.left={} rectFilled.top={} rectFilled.right={} rectFilled.bottom={} "
                      "pdc={}",
                BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this),
                rectEmpty.left, rectEmpty.top, rectEmpty.right, rectEmpty.bottom,
                rectFilled.left, rectFilled.top, rectFilled.right, rectFilled.bottom,
                spdlog::fmt_lib::ptr(pdc));
    return FALSE;
}

float SECProgressCtrl::CalcPercentComplete() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0.0f;
}
