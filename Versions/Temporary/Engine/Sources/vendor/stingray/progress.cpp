#include "Toolkit/progress.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


// The toolkit drew this control itself, which is where its text, its colours,
// its vertical mode and its four painting overridables come from. Underneath
// all of that it is a progress bar, and this class already derives from MFC's
// CProgressCtrl, so the part the editor uses -- attach, set a range, step --
// is the common control's and is forwarded to it.
//
// What is not the common control's is the text inside the bar. comctl32's
// progress bar has none, so SetWindowText keeps the string and GetWindowText
// answers it, and nothing draws it. That is noted where those two are.

namespace
{

// Let the bar repaint. The toolkit's bYield ran a message pump, so a caller
// stepping the bar inside a long loop saw it move; without something here the
// control is only invalidated and nothing repaints until the loop ends, which
// is the whole point of a progress bar.
//
// UpdateWindow rather than a pump: it repaints now, and it does not deliver
// input or timers to an editor in the middle of an operation that has not
// finished. So the bar moves and nothing re-enters. A caller that wants the
// rest of the window to keep up has to pump for itself.
void RepaintNow( CWnd *pWnd )
{
    if ( pWnd != nullptr && pWnd->GetSafeHwnd() != nullptr )
    {
        pWnd->UpdateWindow();
    }
}

}

SECProgressCtrl::SECProgressCtrl() : m_dwExStyle( SEC_EX_PROGRESS_DEFAULTS ) {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

BOOL SECProgressCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, DWORD dwExStyle) {
    spdlog::debug("{} this={} dwStyle={} rect.left={} rect.top={} rect.right={} rect.bottom={} pParentWnd={} nID={} dwExStyle={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwStyle, rect.left, rect.top, rect.right, rect.bottom, spdlog::fmt_lib::ptr(pParentWnd), nID, dwExStyle);
    m_dwExStyle = dwExStyle;
    if (!CProgressCtrl::Create(dwStyle, rect, pParentWnd, nID)) {
        return FALSE;
    }
    OnInitProgress();
    return TRUE;
}

BOOL SECProgressCtrl::AttachProgress(int nCtlID, CWnd* pParentWnd) {
    spdlog::debug("{} this={} nCtlID={} pParentWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCtlID, spdlog::fmt_lib::ptr(pParentWnd));
    // The control is already on the dialog template; attaching means taking it
    // over. Answering TRUE without doing that, which is what this did, left
    // every later call talking to no window at all -- so the bar existed, was
    // never wrong, and never moved.
    if (pParentWnd == nullptr || GetSafeHwnd() != nullptr) {
        return FALSE;
    }
    if (!SubclassDlgItem(nCtlID, pParentWnd)) {
        spdlog::warn("SECProgressCtrl::AttachProgress: no control {} on the parent", nCtlID);
        return FALSE;
    }
    OnInitProgress();
    return TRUE;
}

void SECProgressCtrl::SetRange(ULONG ulLower, ULONG ulUpper) {
    spdlog::debug("{} this={} ulLower={} ulUpper={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), ulLower, ulUpper);
    // SetRange32, not SetRange: the common control's 16 bit form would fold a
    // range of more than 32767 steps round on itself, and the callers here
    // count files and database records.
    CProgressCtrl::SetRange32(static_cast<int>(ulLower), static_cast<int>(ulUpper));
}

ULONG SECProgressCtrl::SetPos(ULONG ulPos, BOOL bYield) {
    spdlog::debug("{} this={} ulPos={} bYield={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), ulPos, bYield);
    const int nPrev = CProgressCtrl::SetPos(static_cast<int>(ulPos));
    if (bYield) {
        RepaintNow(this);
    }
    return static_cast<ULONG>(nPrev);
}

ULONG SECProgressCtrl::OffsetPos(ULONG ulPos, BOOL bYield) {
    spdlog::debug("{} this={} ulPos={} bYield={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), ulPos, bYield);
    const int nPrev = CProgressCtrl::OffsetPos(static_cast<int>(ulPos));
    if (bYield) {
        RepaintNow(this);
    }
    return static_cast<ULONG>(nPrev);
}

ULONG SECProgressCtrl::SetStep(ULONG ulStep) {
    spdlog::debug("{} this={} ulStep={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), ulStep);
    return static_cast<ULONG>(CProgressCtrl::SetStep(static_cast<int>(ulStep)));
}

void SECProgressCtrl::SetColors(COLORREF fgnd, COLORREF bgnd) {
    spdlog::debug("{} this={} fgnd={} bgnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), fgnd, bgnd);
    // Both are honoured by the common control, but only while it is not drawing
    // itself through a visual style: comctl32 v6 ignores a bar colour for a
    // themed control, and this editor asks for v6 in its manifest. Passed on
    // anyway, since that is a decision for the theme rather than for this.
    CProgressCtrl::SetBarColor(fgnd);
    CProgressCtrl::SetBkColor(bgnd);
}

void SECProgressCtrl::SetFont(CFont* pFont, BOOL bRedraw) {
    spdlog::debug("{} this={} pFont={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pFont), bRedraw);
    // The font was for the text the toolkit drew inside the bar. Kept for the
    // control anyway, so a caller that sets one is not silently ignored.
    CWnd::SetFont(pFont, bRedraw);
}

void SECProgressCtrl::SetWindowText(LPCTSTR lpszNewText) {
    spdlog::debug("{} this={} lpszNewText={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), SafeString( lpszNewText ));
    // Kept, not shown. The toolkit painted this string inside the bar and the
    // common control has nowhere to put it, so this is where the substitution
    // is poorer than what it replaces: GetWindowText answers honestly and
    // nothing draws it. A caller that needs the text on screen wants a static
    // beside the bar, which is what CProgressBarWindow already does.
    m_strText = ( lpszNewText != nullptr ) ? lpszNewText : _T("");
}

void SECProgressCtrl::GetWindowText(CString& strText) {
    spdlog::debug("{} this={} strText={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strText.GetString());
    strText = m_strText;
}

void SECProgressCtrl::SetExStyle(DWORD dwExNewStyle) {
    spdlog::debug("{} this={} dwExNewStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwExNewStyle);
    // Kept and answered. Nothing acts on it: every one of these bits describes
    // drawing the toolkit did itself -- vertical, right to left, the text and
    // its alignment -- and the common control offers none of it.
    m_dwExStyle = dwExNewStyle;
}

DWORD SECProgressCtrl::GetExStyle() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return m_dwExStyle;
}

ULONG SECProgressCtrl::StepIt(BOOL bYield) {
    spdlog::debug("{} this={} bYield={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bYield);
    const int nPrev = CProgressCtrl::StepIt();
    if (bYield) {
        RepaintNow(this);
    }
    return static_cast<ULONG>(nPrev);
}

void SECProgressCtrl::ResetProgress() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    // Back to the start of the range, which is not always zero.
    int nLower = 0;
    int nUpper = 0;
    CProgressCtrl::GetRange(nLower, nUpper);
    CProgressCtrl::SetPos(nLower);
    RepaintNow(this);
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
    // Answerable from the control even though nothing here paints with it: the
    // range and the position are both the common control's to give back.
    int nLower = 0;
    int nUpper = 0;
    CProgressCtrl::GetRange(nLower, nUpper);
    if (nUpper <= nLower) {
        return 0.0f;
    }
    const int nPos = CProgressCtrl::GetPos();
    return ( 100.0f * ( nPos - nLower ) ) / ( nUpper - nLower );
}
