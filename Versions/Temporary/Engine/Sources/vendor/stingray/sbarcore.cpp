#include "Toolkit/sbarcore.h"
#include "Toolkit/sbarstat.h"

// For the AFX_IDW_DOCKBAR_ ids, which name the frame's four dock bars, and for
// CDockContext, which is what drags a bar out of one.
#include <afxpriv.h>
#include <algorithm>


BEGIN_MESSAGE_MAP( SECControlBar, CControlBar )
    ON_WM_SIZE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_SETCURSOR()
    ON_WM_CAPTURECHANGED()
END_MESSAGE_MAP()

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


namespace
{
const int DEFAULT_THICKNESS = 120;
const int MIN_PANE_SIZE = 64;
const int RESIZE_STRIP = 5;
const int MAX_PANE_SIZE = 16384;
}

BOOL SECControlBar::m_bOptimizedRedrawEnabled = FALSE;

SECControlBar::SECControlBar()
    : m_szDockHorz(240, DEFAULT_THICKNESS), m_ptDockHorz(0, 0),
      m_szDockVert(DEFAULT_THICKNESS, 240), m_szFloat(265, 400) {
    m_fDockedPctWidth = m_fPctWidth = 1.0f;
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

// Creating the window is only half of creating a control bar.
//
// CControlBar keeps the CBRS_ half of the style in m_dwStyle, separately from
// the window style, and every bar MFC ships sets it in its own Create:
// CDialogBar::Create is m_dwStyle = (nStyle & CBRS_ALL) before it creates
// anything. These stubs never did, so every docking window in the editor had
// m_dwStyle 0: no CBRS_ALIGN_LEFT, no CBRS_SIZE_DYNAMIC, no borders. A bar with
// no alignment is one CFrameWnd::RecalcLayout gives no room to, which is why
// they stayed invisible even once they were docked and had WS_VISIBLE.
//
// The dwExStyle these take is the toolkit's own CBRS_EX_ set, the cool look and
// the gripper, which has nothing to do with the WS_EX_ flags CreateWindowEx
// wants. Passing it through was asking Windows for whatever those bits happen to
// mean there. Keep it as toolkit metadata instead of a Win32 extended style.
BOOL SECControlBar::Create(CWnd* pParentWnd) {
    spdlog::debug("{} this={} pParentWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pParentWnd));
    return Create(pParentWnd, "SECControlBar", WS_CHILD | CBRS_TOP, 0, 0, nullptr);
}

BOOL SECControlBar::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, UINT nID, DWORD dwStyle, DWORD dwExStyle, const RECT& rect, CWnd* pParentWnd, CCreateContext* pContext) {
    spdlog::debug("{} this={} lpszClassName={} lpszWindowName={} nID={} dwStyle={} dwExStyle={} "
                  "rect.left={} rect.top={} rect.right={} rect.bottom={} "
                  "pParentWnd={} pContext={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), SafeString( lpszClassName ), SafeString( lpszWindowName ), nID, dwStyle, dwExStyle,
                  rect.left, rect.top, rect.right, rect.bottom, spdlog::fmt_lib::ptr(pParentWnd), spdlog::fmt_lib::ptr(pContext));
    m_dwStyle = dwStyle & CBRS_ALL;
    m_dwExStyle = dwExStyle;
    if (lpszClassName == nullptr) {
        lpszClassName = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW),
                                            reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1));
    }
    return CWnd::Create(lpszClassName, lpszWindowName, (dwStyle & ~CBRS_ALL) | WS_CHILD, rect, pParentWnd, nID, pContext);
}

BOOL SECControlBar::Create(CWnd* pParentWnd, LPCTSTR lpszWindowName, DWORD dwStyle, DWORD dwExStyle, UINT nID, CCreateContext* pContext) {
    spdlog::debug("{} this={} pParentWnd={} lpszWindowName={} dwStyle={} dwExStyle={} nID={} pContext={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pParentWnd), SafeString( lpszWindowName ), dwStyle, dwExStyle, nID,
                      spdlog::fmt_lib::ptr(pContext));
    RECT rect{0, 0, 0, 0};
    LPCTSTR lpszClassName = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1), ::LoadIcon(nullptr, IDI_APPLICATION));
    return Create(lpszClassName, lpszWindowName, nID, dwStyle, dwExStyle, rect, pParentWnd, pContext);
}

// Where a docking window may put what it contains: this bar's client area with
// its borders taken off.
//
// Leaving the caller's rectangle untouched, which is what this did, is not a
// harmless nothing. CDWLog::OnSize, CDWPropertyBrowser::OnSize and the GDB
// browser all ask for it and then SetWindowPos their contents into whatever
// came back, so the Scintilla control in the log window came out 0 by 0 inside
// a bar that was 265 by 265.
//
// CalcInsideRect is MFC's own answer to the same question, and it is what
// CControlBar's painting uses, so the contents land exactly inside the borders
// the bar draws.
void SECControlBar::GetInsideRect(CRect& rectInside) const {
    // rectInside is an output parameter and may not have been initialized.
    if (GetSafeHwnd() == nullptr) {
        rectInside.SetRectEmpty();
        return;
    }
    GetPanelRect(rectInside);
    // The gripper comes out of the same rectangle the contents are given, which
    // is what makes it cost nothing elsewhere: CDefaultDockingWindow::OnSize
    // asks for this and positions its contents in it, so the pane's contents
    // move down by themselves and no editor code changes.
    rectInside.top += GetGripperHeight();
    if (rectInside.top > rectInside.bottom) {
        rectInside.top = rectInside.bottom;
    }
}

// How tall the gripper strip is, and zero when there is not one.
//
// A floating bar has a real caption of its own from its mini frame, so a second
// one inside it would be two title bars stacked.
int SECControlBar::GetGripperHeight() const {
    if (!m_bShowGripper || GetSafeHwnd() == nullptr || (m_dwStyle & CBRS_FLOATING) != 0) {
        return 0;
    }
    const int nHeight = ::GetSystemMetrics(SM_CYSMCAPTION);
    return (nHeight > 0) ? nHeight : 16;
}

// Where the gripper is, and where its close button is inside it.
BOOL SECControlBar::GetGripperRects(CRect *pRectGripper, CRect *pRectClose) const {
    const int nHeight = GetGripperHeight();
    if (nHeight <= 0) {
        return FALSE;
    }
    CRect rect;
    GetPanelRect(rect);
    rect.bottom = (std::min)(rect.bottom, rect.top + nHeight);
    if (rect.IsRectEmpty()) {
        return FALSE;
    }
    if (pRectGripper != nullptr) {
        *pRectGripper = rect;
    }
    if (pRectClose != nullptr) {
        // Square, at the right hand end, inset a little so it is not touching
        // the border.
        const int nButton = rect.Height() - 2;
        *pRectClose = CRect(rect.right - nButton - 1, rect.top + 1, rect.right - 1, rect.top + 1 + nButton);
    }
    return TRUE;
}

// Draw the gripper: the pane's own title, and a close button.
//
// The toolkit drew a gripper with a close and an expand button and kept the
// rectangles in m_rcGripperCloseButton and m_rcGripperExpandButton. This draws
// the close half of that, because closing is what the editor overrides
// OnGripperClose for; there is nothing behind expand in this editor.
void SECControlBar::DoPaint(CDC *pDC) {
    if (pDC == nullptr) return;
    CControlBar::DoPaint(pDC);
    CRect thickness, divider;
    GetResizeRects(thickness, divider);
    if (!thickness.IsRectEmpty())
        pDC->Draw3dRect(thickness, ::GetSysColor(COLOR_3DHILIGHT), ::GetSysColor(COLOR_3DSHADOW));
    if (!divider.IsRectEmpty())
        pDC->Draw3dRect(divider, ::GetSysColor(COLOR_3DHILIGHT), ::GetSysColor(COLOR_3DSHADOW));
    CRect rectGripper;
    if (!GetGripperRects(&rectGripper, &m_rcGripperCloseButton)) {
        return;
    }
    pDC->FillSolidRect(&rectGripper, ::GetSysColor(COLOR_3DFACE));
    // A line under it, so the strip reads as a caption rather than as part of
    // whatever the pane is showing.
    pDC->FillSolidRect(rectGripper.left, rectGripper.bottom - 1, rectGripper.Width(), 1,
        ::GetSysColor(COLOR_3DSHADOW));

    CString strTitle;
    GetWindowText(strTitle);
    if (!strTitle.IsEmpty()) {
        CRect rectText(rectGripper);
        rectText.left += 4;
        rectText.right = m_rcGripperCloseButton.left - 2;
        const int nOldMode = pDC->SetBkMode(TRANSPARENT);
        const COLORREF rgbOld = pDC->SetTextColor(::GetSysColor(COLOR_BTNTEXT));
        CFont *const pOldFont = pDC->SelectObject(
            CFont::FromHandle(reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT))));
        pDC->DrawText(strTitle, &rectText, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        pDC->SelectObject(pOldFont);
        pDC->SetTextColor(rgbOld);
        pDC->SetBkMode(nOldMode);
    }
    // DFCS_CAPTIONCLOSE is the same X the system draws on a small caption, so
    // it follows the user's theme without this having to know anything about it.
    pDC->DrawFrameControl(&m_rcGripperCloseButton, DFC_CAPTION,
        DFCS_CAPTIONCLOSE | (m_bGripperCloseDown ? DFCS_PUSHED : 0));
}

void SECControlBar::OnSize(UINT nType, int cx, int cy) {
    CControlBar::OnSize(nType, cx, cy);
    // MFC invalidates only the changed borders when a bar is resized. Our close
    // button moves with the caption's right edge, so copied client pixels can
    // leave old X buttons behind. Repaint the entire caption at its new size.
    CRect caption;
    if (GetGripperRects(&caption, nullptr))
        InvalidateRect(&caption, FALSE);
}

void SECControlBar::OnLButtonDown(UINT nFlags, CPoint point) {
    CRect thickness, divider;
    GetResizeRects(thickness, divider);
    if (thickness.PtInRect(point) || divider.PtInRect(point)) {
        TrackDockResize(!thickness.PtInRect(point), point);
        return;
    }
    CRect rectGripper;
    CRect rectClose;
    if (!GetGripperRects(&rectGripper, &rectClose) || !rectGripper.PtInRect(point)) {
        CControlBar::OnLButtonDown(nFlags, point);
        return;
    }
    if (rectClose.PtInRect(point)) {
        m_bGripperCloseDown = TRUE;
        InvalidateRect(&rectClose);
        SetCapture();
        return;
    }
    // Anywhere else on the strip is the handle: this is what a gripper is for
    // besides closing, and MFC already knows how to drag a docked bar once it
    // is told where the drag began.
    if (m_pDockContext != nullptr) {
        ClientToScreen(&point);
        m_pDockContext->StartDrag(point);
    }
}

void SECControlBar::OnLButtonUp(UINT nFlags, CPoint point) {
    if (!m_bGripperCloseDown) {
        CControlBar::OnLButtonUp(nFlags, point);
        return;
    }
    m_bGripperCloseDown = FALSE;
    if (GetCapture() == this) ::ReleaseCapture();
    CRect rectClose;
    if (!GetGripperRects(nullptr, &rectClose)) return;
    InvalidateRect(&rectClose);
    // Released somewhere else means the click was taken back, the way any
    // button behaves.
    if (!rectClose.PtInRect(point)) {
        return;
    }
    // The editor's own override gets to refuse: CDWGDBBrowser has one, and it
    // has never been asked until now because nothing drew a button to ask from.
    if (!OnGripperClose()) {
        return;
    }
    if (CFrameWnd *const pFrame = GetDockingFrame()) {
        pFrame->ShowControlBar(this, FALSE, FALSE);
    }
}

// m_arrBars also contains small integer placeholders for floating bars. Test
// the full pointer-sized value before casting; LOWORD(pointer) is unsafe on x64.
std::vector<SECControlBar*> SECControlBar::GetRowBars() const {
    std::vector<SECControlBar*> row;
    if (m_pDockBar == nullptr || IsFloating()) return row;
    bool found = false;
    for (INT_PTR i = 0; i < m_pDockBar->m_arrBars.GetSize(); ++i) {
        void* entry = m_pDockBar->m_arrBars[i];
        if (entry == nullptr) {
            if (found) break;
            row.clear();
        } else if (reinterpret_cast<UINT_PTR>(entry) > 0xffff) {
            auto* pane = dynamic_cast<SECControlBar*>(static_cast<CControlBar*>(entry));
            if (pane != nullptr && pane->IsVisible()) {
                row.push_back(pane);
                if (pane == this) found = true;
            }
        }
    }
    if (!found) row.clear();
    return row;
}

void SECControlBar::GetResizeRects(CRect& thickness, CRect& divider) const {
    thickness.SetRectEmpty();
    divider.SetRectEmpty();
    if (GetSafeHwnd() == nullptr || m_pDockBar == nullptr || IsFloating()) return;
    CRect rect;
    GetClientRect(rect);
    CalcInsideRect(rect, (m_dwStyle & CBRS_ORIENT_HORZ) != 0);
    if (rect.Width() < RESIZE_STRIP * 2 || rect.Height() < RESIZE_STRIP * 2) return;
    thickness = rect;
    switch (m_dwStyle & CBRS_ALIGN_ANY) {
    case CBRS_ALIGN_LEFT: thickness.left = thickness.right - RESIZE_STRIP; break;
    case CBRS_ALIGN_RIGHT: thickness.right = thickness.left + RESIZE_STRIP; break;
    case CBRS_ALIGN_TOP: thickness.top = thickness.bottom - RESIZE_STRIP; break;
    case CBRS_ALIGN_BOTTOM: thickness.bottom = thickness.top + RESIZE_STRIP; break;
    default: thickness.SetRectEmpty(); return;
    }
    const auto row = GetRowBars();
    const auto it = std::find(row.begin(), row.end(), this);
    if (it != row.end() && it + 1 != row.end()) {
        divider = rect;
        if (m_dwStyle & CBRS_ORIENT_HORZ) divider.left = divider.right - RESIZE_STRIP;
        else divider.top = divider.bottom - RESIZE_STRIP;
    }
}

void SECControlBar::GetPanelRect(CRect& rect) const {
    GetClientRect(rect);
    CalcInsideRect(rect, (m_dwStyle & CBRS_ORIENT_HORZ) != 0);
    CRect thickness, divider;
    GetResizeRects(thickness, divider);
    if (!thickness.IsRectEmpty()) {
        switch (m_dwStyle & CBRS_ALIGN_ANY) {
        case CBRS_ALIGN_LEFT: rect.right -= RESIZE_STRIP; break;
        case CBRS_ALIGN_RIGHT: rect.left += RESIZE_STRIP; break;
        case CBRS_ALIGN_TOP: rect.bottom -= RESIZE_STRIP; break;
        case CBRS_ALIGN_BOTTOM: rect.top += RESIZE_STRIP; break;
        }
    }
    if (!divider.IsRectEmpty()) {
        if (m_dwStyle & CBRS_ORIENT_HORZ) rect.right -= RESIZE_STRIP;
        else rect.bottom -= RESIZE_STRIP;
    }
    rect.right = (std::max)(rect.left, rect.right);
    rect.bottom = (std::max)(rect.top, rect.bottom);
}

BOOL SECControlBar::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) {
    if (pWnd == this && nHitTest == HTCLIENT) {
        CPoint point;
        ::GetCursorPos(&point);
        ScreenToClient(&point);
        CRect thickness, divider;
        GetResizeRects(thickness, divider);
        const bool across = thickness.PtInRect(point) != FALSE;
        if (across || divider.PtInRect(point)) {
            const bool horz = (m_dwStyle & CBRS_ORIENT_HORZ) != 0;
            ::SetCursor(::LoadCursor(nullptr, across == horz ? IDC_SIZENS : IDC_SIZEWE));
            return TRUE;
        }
    }
    return CControlBar::OnSetCursor(pWnd, nHitTest, message);
}

void SECControlBar::OnCaptureChanged(CWnd* pWnd) {
    if (m_bGripperCloseDown) {
        m_bGripperCloseDown = FALSE;
        Invalidate(FALSE);
    }
    CControlBar::OnCaptureChanged(pWnd);
}

void SECControlBar::OnLButtonDblClk(UINT nFlags, CPoint point) {
    CRect gripper, close;
    if (m_pDockContext != nullptr && GetGripperRects(&gripper, &close)
        && gripper.PtInRect(point) && !close.PtInRect(point)) {
        m_pDockContext->ToggleDocking();
        return;
    }
    CWnd::OnLButtonDblClk(nFlags, point);
}

// Resize the entire row's thickness, or trade length with the next pane. Work
// from the original rectangles on every move so live layout cannot accumulate
// rounding error. Escape, right-click and lost capture restore the original.
void SECControlBar::TrackDockResize(bool bDivider, CPoint point) {
    CFrameWnd* frame = m_pDockSite;
    auto row = GetRowBars();
    auto it = std::find(row.begin(), row.end(), this);
    if (frame == nullptr || it == row.end() || GetCapture() != nullptr) return;
    const size_t index = it - row.begin();
    if (bDivider && index + 1 == row.size()) return;
    const bool horz = (m_dwStyle & CBRS_ORIENT_HORZ) != 0;
    struct SavedSize { CSize horizontal, vertical; float weight; };
    std::vector<SavedSize> saved;
    for (auto* pane : row)
        saved.push_back({pane->m_szDockHorz, pane->m_szDockVert, pane->m_fDockedPctWidth});
    CRect original, next(0, 0, 0, 0);
    GetWindowRect(original);
    if (bDivider) row[index + 1]->GetWindowRect(next);
    const int firstLength = horz ? original.Width() : original.Height();
    const int pairLength = firstLength + (horz ? next.Width() : next.Height());
    if (bDivider && pairLength < 2) return;
    const int originalThickness = horz ? original.Height() : original.Width();
    CRect frameRect;
    frame->GetClientRect(frameRect);
    int maxThickness = (horz ? frameRect.Height() : frameRect.Width()) - 96;
    // Limit growth to the space actually left in the document area.
    CRect document;
    frame->RepositionBars(0, 0xffff, AFX_IDW_PANE_FIRST, CWnd::reposQuery, &document);
    maxThickness = (std::min)(maxThickness,
        originalThickness + (horz ? document.Height() : document.Width()) - 96);
    maxThickness = (std::max)(MIN_PANE_SIZE, maxThickness);
    ClientToScreen(&point);
    SetCapture();
    bool accepted = false;
    bool closePending = false;
    MSG msg = {};
    while (GetCapture() == this) {
        const BOOL result = ::GetMessage(&msg, nullptr, 0, 0);
        if (result <= 0) {
            if (result == 0) ::PostQuitMessage((int)msg.wParam);
            break;
        }
        // Delay a queued close until the tracked windows are no longer in use.
        if (msg.message == WM_CLOSE) { closePending = true; break; }
        if (msg.message == WM_LBUTTONUP) { accepted = true; break; }
        if (msg.message == WM_CANCELMODE || msg.message == WM_RBUTTONDOWN
            || (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)) break;
        if (msg.message == WM_MOUSEMOVE) {
            CPoint current;
            ::GetCursorPos(&current);
            if (bDivider) {
                const int delta = horz ? current.x - point.x : current.y - point.y;
                const int minimum = (std::min)(MIN_PANE_SIZE, pairLength / 2);
                const int length = (std::clamp)(firstLength + delta, minimum, pairLength - minimum);
                const float weight = saved[index].weight + saved[index + 1].weight;
                row[index]->m_fDockedPctWidth = weight * length / pairLength;
                row[index + 1]->m_fDockedPctWidth = weight * (pairLength - length) / pairLength;
            } else {
                int delta = horz ? current.y - point.y : current.x - point.x;
                if (m_dwStyle & (CBRS_ALIGN_RIGHT | CBRS_ALIGN_BOTTOM)) delta = -delta;
                const int thickness = (std::clamp)(originalThickness + delta, MIN_PANE_SIZE, maxThickness);
                for (auto* pane : row) {
                    if (horz) pane->m_szDockHorz.cy = thickness;
                    else pane->m_szDockVert.cx = thickness;
                }
            }
            frame->RecalcLayout();
        } else if (msg.message < WM_KEYFIRST || msg.message > WM_KEYLAST) {
            ::DispatchMessage(&msg);
        }
    }
    if (GetCapture() == this) ::ReleaseCapture();
    if (!accepted) {
        for (size_t i = 0; i < row.size(); ++i) {
            row[i]->m_szDockHorz = saved[i].horizontal;
            row[i]->m_szDockVert = saved[i].vertical;
            row[i]->m_fDockedPctWidth = saved[i].weight;
        }
    }
    for (auto* pane : row) pane->m_fPctWidth = pane->m_fDockedPctWidth;
    frame->RecalcLayout();
    if (closePending) ::PostMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
}

void SECControlBar::LoadPanelState(LPCTSTR profile) {
    CString section;
    section.Format(_T("%s-Pane-%u"), profile, (UINT)GetDlgCtrlID());
    CWinApp* app = AfxGetApp();
    if (app->GetProfileInt(section, _T("Version"), 0) != 1) return;
    auto dimension = [&](LPCTSTR key, int fallback) {
        const UINT value = app->GetProfileInt(section, key, fallback);
        return value >= MIN_PANE_SIZE && value <= MAX_PANE_SIZE ? (int)value : fallback;
    };
    m_szDockHorz.cx = dimension(_T("HorzLength"), m_szDockHorz.cx);
    m_szDockHorz.cy = dimension(_T("HorzThickness"), m_szDockHorz.cy);
    m_szDockVert.cx = dimension(_T("VertThickness"), m_szDockVert.cx);
    m_szDockVert.cy = dimension(_T("VertLength"), m_szDockVert.cy);
    m_szFloat.cx = dimension(_T("FloatWidth"), m_szFloat.cx);
    m_szFloat.cy = dimension(_T("FloatHeight"), m_szFloat.cy);
    const UINT weight = app->GetProfileInt(section, _T("Weight"), 0);
    if (weight >= 1 && weight <= 1000000)
        m_fDockedPctWidth = m_fPctWidth = weight / 1000000.0f;
}

void SECControlBar::SavePanelState(LPCTSTR profile) const {
    CString section;
    section.Format(_T("%s-Pane-%u"), profile, (UINT)GetDlgCtrlID());
    CWinApp* app = AfxGetApp();
    app->WriteProfileInt(section, _T("HorzLength"), m_szDockHorz.cx);
    app->WriteProfileInt(section, _T("HorzThickness"), m_szDockHorz.cy);
    app->WriteProfileInt(section, _T("VertThickness"), m_szDockVert.cx);
    app->WriteProfileInt(section, _T("VertLength"), m_szDockVert.cy);
    app->WriteProfileInt(section, _T("FloatWidth"), m_szFloat.cx);
    app->WriteProfileInt(section, _T("FloatHeight"), m_szFloat.cy);
    app->WriteProfileInt(section, _T("Weight"), (int)(m_fDockedPctWidth * 1000000.0f + 0.5f));
    app->WriteProfileInt(section, _T("Version"), 1);
}

BOOL SECControlBar::IsMDIChild() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECControlBar::GetOptimizeRedrawEnabled() {
    spdlog::debug("{}", BOOST_CURRENT_FUNCTION);
    return m_bOptimizedRedrawEnabled;
}

void SECControlBar::SetOptimizedRedrawEnabled(BOOL bOptimize) {
    spdlog::debug("{} bOptimize={}", BOOST_CURRENT_FUNCTION, bOptimize);
    m_bOptimizedRedrawEnabled = bOptimize;
}

BOOL SECControlBar::GetBarSizePos(int& nRow,int& nCol) {
    spdlog::debug("{} this={} nRow={} nCol={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nRow, nCol);
    return FALSE;
}

BOOL SECControlBar::GetBarSizePos(int& nRow,int& nCol,int& nDockbarID) {
    spdlog::debug("{} this={} nRow={} nCol={} nDockbarID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nRow, nCol, nDockbarID);
    return FALSE;
}

BOOL SECControlBar::GetBarSizePos(int& nRow,int& nCol,int& nDockbarID,float& fPctWidth,int& nHeight) {
    spdlog::debug("{} this={} nRow={} nCol={} nDockbarID={} fPctWidth={} nHeight={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nRow, nCol, nDockbarID, fPctWidth, nHeight);
    return FALSE;
}

// Create our context before the bar is first docked. Deleting a dock context
// removes its bar from the dock array, so never replace one on a live docked bar.
void SECControlBar::EnableDocking(DWORD dwDockStyle) {
    if (m_pDockContext == nullptr) {
        m_pDockSite = GetParentFrame();
        m_pDockContext = NewDockContext();
    }
    CControlBar::EnableDocking(dwDockStyle);
}

SECDockContext* SECControlBar::NewDockContext() {
    return new SECDockContext(this);
}

void SECDockContext::StartResize(int nHitTest, CPoint pt) {
    auto* pane = static_cast<SECControlBar*>(m_pBar);
    pane->m_bFloatSizing = true;
    CDockContext::StartResize(nHitTest, pt);
    pane->m_bFloatSizing = false;
}

void SECControlBar::SetExBarStyle(DWORD dwExStyle, BOOL bAutoUpdate) {
    spdlog::debug("{} this={} dwExStyle={} bAutoUpdate={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwExStyle, bAutoUpdate);
}

void SECControlBar::ModifyBarStyleEx(DWORD dwRemove, DWORD dwAdd, BOOL bAutoUpdate) {
    spdlog::debug("{} this={} dwRemove={} dwAdd={} bAutoUpdate={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemove, dwAdd, bAutoUpdate);
}

// Not a stub. A control bar's id is what CFrameWnd::GetControlBar looks it up
// by, what LoadBarState and SaveBarState name it by, and what ShowControlBar
// addresses it with. Returning 0 gave every docking window in the editor the
// same id, and 0 is the one value GetControlBar refuses outright, so no bar
// could be found by any of them and LoadBarState asserted on the first one it
// read back.
//
// The first id at or above nBaseID that no control bar on this frame is using.
// Stable across runs given the same creation order, which is what a saved
// layout needs in order to still mean something the next time.
UINT SECControlBar::GetUniqueBarID(CFrameWnd* pMainWnd, UINT nBaseID) {
    spdlog::debug("{} pMainWnd={} nBaseID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(pMainWnd), nBaseID);
    if (pMainWnd == nullptr) {
        return nBaseID;
    }
    UINT nID = nBaseID;
    while (pMainWnd->GetControlBar(nID) != nullptr) {
        ++nID;
    }
    return nID;
}

// Still a stub. Answering it means walking every control bar on the frame, and
// CFrameWnd::m_listControlBars is not public; GetControlBar only looks one up
// by id, which is the question already answered above. Nothing in this tree
// calls it.
BOOL SECControlBar::VerifyUniqueBarIds(CFrameWnd* pFrameWnd) {
    spdlog::debug("{} pFrameWnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(pFrameWnd));
    return FALSE;
}

// Also not a stub, and the same question GetUniqueBarID answers by counting.
BOOL SECControlBar::VerifyUniqueSpecificBarID(CFrameWnd* pFrameWnd, UINT nBarID) {
    spdlog::debug("{} pFrameWnd={} nBarID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(pFrameWnd), nBarID);
    return pFrameWnd != nullptr && pFrameWnd->GetControlBar(nBarID) == nullptr;
}

// Docked dimensions and floating dimensions must be independent: MFC's MRU
// width belongs to floating toolbars and cannot also store a pane's thickness.
CSize SECControlBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz) {
    return CalcDynamicLayout(-1, (bStretch ? LM_STRETCH : 0) |
        (bHorz ? LM_HORZ | LM_HORZDOCK : LM_VERTDOCK));
}

void SECControlBar::SetDockedLayout(int nThickness, float fPctLength) {
    if (nThickness > 0) {
        m_szDockHorz.cy = m_szDockVert.cx =
            (std::clamp)(nThickness, MIN_PANE_SIZE, MAX_PANE_SIZE);
    }
    m_fDockedPctWidth = m_fPctWidth =
        fPctLength > 0.0f && fPctLength <= 1.0f ? fPctLength : 1.0f;
}

CSize SECControlBar::CalcDynamicLayout(int nLength, DWORD dwMode) {
    if (dwMode & LM_HORZDOCK) return m_szDockHorz;
    if (dwMode & LM_VERTDOCK) return m_szDockVert;
    if (dwMode & LM_MRUWIDTH) return m_szFloat;

    if (m_bFloatSizing && (dwMode & LM_COMMIT)) {
        // CDockContext::EndResize passes width only, even for a height drag.
        // Its accepted rectangle contains both dimensions (also screen-clamped).
        m_szFloat = m_pDockContext->m_rectDragVert.Size();
        m_nMRUWidth = m_szFloat.cx;
        return m_szFloat;
    }
    if (m_bFloatSizing && nLength >= 0) {
        CSize size = m_szFloat;
        if (dwMode & LM_LENGTHY) size.cy = (std::clamp)(nLength, MIN_PANE_SIZE, MAX_PANE_SIZE);
        else size.cx = (std::clamp)(nLength, MIN_PANE_SIZE, MAX_PANE_SIZE);
        return size;
    }
    return IsFloating() ? m_szFloat : ((dwMode & LM_HORZ) ? m_szDockHorz : m_szDockVert);
}

void SECControlBar::OnBarBeginDock() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECControlBar::OnBarEndDock() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECControlBar::OnBarBeginFloat() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECControlBar::OnBarEndFloat() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECControlBar::OnBarBeginMDIFloat() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECControlBar::OnBarEndMDIFloat() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

BOOL SECControlBar::OnGripperClose() {
    // Hide the pane; the editor's View commands can show the same window again.
    return TRUE;
}

BOOL SECControlBar::OnGripperExpand() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

// MFC calls this on every bar on every idle, and a bar that holds a window
// rather than buttons has nothing to update. It was 1,764 of the 1,946 lines
// in a trace of one startup, so it stays a level below everything else here
// and OBK2_STINGRAY_LOG=trace is what asks for it.
void SECControlBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler) {
    spdlog::trace("{} this={} pTarget={} bDisableIfNoHndler={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pTarget), bDisableIfNoHndler);
}

// CommandToIndex, SetPaneInfo and SetPaneText are gone from this file: with
// CStatusBar underneath, MFC's own are correct and were only being shadowed.
//
// Returning a constant from CommandToIndex was not the harmless placeholder it
// looked like. CMainFrame::OnCreate asks it where panes 140 and 141 are and then
// sizes what it is told: both answers were 0, so both SetPaneInfo calls landed on
// pane 0 with widths 500 and 200 and the second overwrote the first. Searching
// the panes for the id, which is all CStatusBar::CommandToIndex does, answers 1
// and 2.
//
// SetIndicators still needs an override, because MFC's reports failure that this
// editor cannot survive.
//
// MFC loads a string resource per indicator, to size each pane from the width of
// its text. The editor's two pane ids have no string resource -- not in this
// build, and not in the shipped 2005 binary either, where LoadString for 140 and
// 141 comes back empty as well. Since that editor ran with a working status bar,
// the toolkit's SetIndicators plainly never looked a string up. These panes get
// their text at runtime from CMainFrame::SetStatusBarText and their widths from
// the SetPaneInfo calls that follow, so the lookup has nothing to contribute.
//
// Handing MFC the id array anyway is worse than a wasted lookup, because its loop
// breaks out on the first string it cannot load. With { ID_SEPARATOR, 140, 141 }
// it assigns pane 1 its id, fails to load 140's string and stops, so pane 2 keeps
// the id 0 that AllocElements zeroed it to. CommandToIndex( 141 ) then finds no
// such pane and answers -1, and CMainFrame::OnCreate feeds that straight to
// SetPaneInfo, which asserts on the index.
//
// So the panes are allocated by passing no id array at all, which is the argument
// the string lookup hangs off: with nullptr MFC skips that loop entirely. Each
// pane is then given its id directly, which is all CommandToIndex searches for.
//
// The one thing MFC's skipped loop also does is make an id-less first pane the
// stretchy one, and that is worth keeping: it is the pane the editor writes its
// messages into, and it has to take whatever width the two sized panes leave.
BOOL SECStatusBar::SetIndicators(const UINT * indicators, int size) {
    spdlog::debug("{} this={} indicators={} size={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(indicators), size);
    if (!CStatusBar::SetIndicators(nullptr, size)) {
        return FALSE;
    }
    if (indicators == nullptr) {
        return TRUE;
    }
    for (int i = 0; i < size; ++i) {
        const UINT nStyle = (indicators[i] == 0 && i == 0) ? (SBPS_STRETCH | SBPS_NOBORDERS)
                                                           : SBPS_NORMAL;
        SetPaneInfo(i, indicators[i], nStyle, 0);
    }
    return TRUE;
}
