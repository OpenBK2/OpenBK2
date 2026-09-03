#include "Toolkit/olbar.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


namespace {
// A button per pane, addressed by id. The range is private to this window and
// clear of the control bar and pane ranges MFC reserves.
const UINT ID_BAR_BUTTON_FIRST = 0x7B00;
const UINT ID_BAR_BUTTON_LAST = 0x7BFF;
// How tall one pane button is. The toolkit measures its own font; this asks for
// a button that fits the shell's.
const int BAR_BUTTON_HEIGHT = 24;
}

BEGIN_MESSAGE_MAP(SECShortcutBar, CWnd)
    ON_WM_SIZE()
    ON_COMMAND_RANGE(ID_BAR_BUTTON_FIRST, ID_BAR_BUTTON_LAST, &SECShortcutBar::OnBarButton)
END_MESSAGE_MAP()

SECShortcutBar::SECShortcutBar() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

SECShortcutBar::~SECShortcutBar() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    for (SECBar *pBar : m_bars) {
        delete pBar->pButton;
        delete pBar;
    }
    m_bars.clear();
}

// The button is made only once this window exists, since the editor adds panes
// during module creation and the bar is not always created by then.
BOOL SECShortcutBar::EnsureButton(int iIndex) {
    if (GetSafeHwnd() == nullptr || iIndex < 0 || iIndex >= static_cast<int>(m_bars.size())) {
        return FALSE;
    }
    SECBar *pBar = m_bars[iIndex];
    if (pBar->pButton != nullptr) {
        return TRUE;
    }
    if (ID_BAR_BUTTON_FIRST + iIndex > ID_BAR_BUTTON_LAST) {
        spdlog::warn("SECShortcutBar::EnsureButton: more panes than the button id range holds ({})", iIndex);
        return FALSE;
    }
    pBar->pButton = new CButton();
    if (!pBar->pButton->Create(pBar->strLabel, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                              CRect(0, 0, 0, 0), this,
                              ID_BAR_BUTTON_FIRST + static_cast<UINT>(iIndex))) {
        spdlog::warn("SECShortcutBar::EnsureButton: pane button {} would not create", iIndex);
        delete pBar->pButton;
        pBar->pButton = nullptr;
        return FALSE;
    }
    pBar->pButton->SetFont(CFont::FromHandle(reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT))));
    return TRUE;
}

void SECShortcutBar::DestroyButton(int iIndex) {
    if (iIndex < 0 || iIndex >= static_cast<int>(m_bars.size())) {
        return;
    }
    SECBar *pBar = m_bars[iIndex];
    if (pBar->pButton != nullptr) {
        if (pBar->pButton->GetSafeHwnd() != nullptr) {
            pBar->pButton->DestroyWindow();
        }
        delete pBar->pButton;
        pBar->pButton = nullptr;
    }
}

// Buttons for the panes up to and including the open one stack at the top,
// buttons for the rest sit at the bottom, and the open pane's window fills
// everything between. That is the arrangement an Outlook bar has, and it is what
// the editor's four panes -- Terrain, Objects, Gameplay, Script -- expect.
//
// Only the open pane is shown; the others are hidden rather than merely covered,
// so a pane that is not on screen cannot take clicks meant for the one that is.
void SECShortcutBar::LayoutBars() {
    if (GetSafeHwnd() == nullptr) {
        return;
    }
    CRect rcClient;
    GetClientRect(&rcClient);
    if (rcClient.IsRectEmpty()) {
        return;
    }

    const int nCount = static_cast<int>(m_bars.size());
    int nTop = rcClient.top;
    int nBottom = rcClient.bottom;

    for (int i = 0; i < nCount; ++i) {
        EnsureButton(i);
        SECBar *pBar = m_bars[i];
        if (pBar->pButton == nullptr || pBar->pButton->GetSafeHwnd() == nullptr) {
            continue;
        }
        if (i <= m_nActiveBar || m_nActiveBar < 0) {
            pBar->pButton->SetWindowPos(nullptr, rcClient.left, nTop, rcClient.Width(),
                                        BAR_BUTTON_HEIGHT, SWP_NOZORDER | SWP_NOACTIVATE);
            nTop += BAR_BUTTON_HEIGHT;
        }
    }
    for (int i = nCount - 1; i > m_nActiveBar && m_nActiveBar >= 0; --i) {
        SECBar *pBar = m_bars[i];
        if (pBar->pButton == nullptr || pBar->pButton->GetSafeHwnd() == nullptr) {
            continue;
        }
        nBottom -= BAR_BUTTON_HEIGHT;
        pBar->pButton->SetWindowPos(nullptr, rcClient.left, nBottom, rcClient.Width(),
                                    BAR_BUTTON_HEIGHT, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    for (int i = 0; i < nCount; ++i) {
        CWnd *pWnd = m_bars[i]->pWnd;
        if (pWnd == nullptr || pWnd->GetSafeHwnd() == nullptr) {
            continue;
        }
        const BOOL bActive = (i == m_nActiveBar);
        if (bActive && nBottom > nTop) {
            pWnd->SetWindowPos(nullptr, rcClient.left, nTop, rcClient.Width(), nBottom - nTop,
                               SWP_NOZORDER | SWP_NOACTIVATE);
        }
        pWnd->ShowWindow(bActive ? SW_SHOW : SW_HIDE);
    }
}

void SECShortcutBar::OnSize(UINT nType, int cx, int cy) {
    CWnd::OnSize(nType, cx, cy);
    LayoutBars();
}

void SECShortcutBar::OnBarButton(UINT nID) {
    const int iIndex = static_cast<int>(nID - ID_BAR_BUTTON_FIRST);
    spdlog::debug("SECShortcutBar::OnBarButton this={} nID={} iIndex={}", spdlog::fmt_lib::ptr(this), nID, iIndex);
    ActivateBar(iIndex);
}

BOOL SECShortcutBar::Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID) {
    spdlog::debug("{} this={} dwStyle={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwStyle, nID);
    LPCTSTR lpszClassName = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1), ::LoadIcon(nullptr, IDI_APPLICATION));
    RECT rect{0, 0, 0, 0};
    return CWnd::Create(lpszClassName, "SECShortcutBar", dwStyle, rect, pParentWnd, nID, nullptr);
}

void SECShortcutBar::SetBarClass(CRuntimeClass* const pBarClass) {
    spdlog::debug("{} this={} pBarClass={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBarClass));
}

CRuntimeClass* SECShortcutBar::GetBarClass() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECShortcutBar::SetListBarClass( CRuntimeClass* const pBarClass ) {
    spdlog::debug("{} this={} pBarClass={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBarClass));
}

CRuntimeClass* SECShortcutBar::GetListBarClass() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECShortcutBar::SetListCtrlClass( CRuntimeClass* const pBarClass ) {
    spdlog::debug("{} this={} pBarClass={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBarClass));
}

CRuntimeClass* SECShortcutBar::GetListCtrlClass() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECShortcutBar::SetFontPointSize( const int& iFontPointSize )  {
    spdlog::debug("{} this={} iFontPointSize={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iFontPointSize);
}

int SECShortcutBar::GetFontPointSize() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

void SECShortcutBar::SetFontName( const CString& sFontName ) {
    spdlog::debug("{} this={} sFontName={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), sFontName.GetString());
    m_strFontName = sFontName;
}

// This returned a reference to a temporary: `return "";` builds a CString on the
// spot and hands back a reference to it, which is dead before the caller sees
// it. The compiler has been saying so as C4172 on this line. The name is now
// kept in a member, which has a lifetime. It is still not applied to anything.
const CString& SECShortcutBar::GetFontName() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return m_strFontName;
}

void SECShortcutBar::SetAnimationSpeed( const int& iAnimationSpeed ) {
    spdlog::debug("{} this={} iAnimationSpeed={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iAnimationSpeed);
}

int SECShortcutBar::GetAnimationSpeed() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

void SECShortcutBar::SetAnimationStep( const int& iAnimationStep ) {
    spdlog::debug("{} this={} iAnimationStep={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iAnimationStep);
}

int SECShortcutBar::GetAnimationStep() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

void SECShortcutBar::SetBarMenu( HMENU hMenu, int iIndex ){
    spdlog::debug("{} this={} hMenu={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hMenu), iIndex);
}

void SECShortcutBar::SetBarMenu( CMenu* pSubMenu, int iIndex, int iLevel ){
    spdlog::debug("{} this={} pSubMenu={} iIndex={} iLevel={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pSubMenu), iIndex, iLevel);
}

void SECShortcutBar::SetBarFont( CFont* pFont, int iIndex ) {
    spdlog::debug("{} this={} pFont={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pFont), iIndex);
}

void SECShortcutBar::SetBarFont( HFONT hFont, int iIndex ) {
    spdlog::debug("{} this={} hFont={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hFont), iIndex);
}

void SECShortcutBar::SetBackFillColor( COLORREF color ) {
    spdlog::debug("{} this={} color={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), color);
}

void SECShortcutBar::SetBackFillColor( CBrush* pBackFillBrush ) {
    spdlog::debug("{} this={} pBackFillBrush={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pBackFillBrush));
}

void SECShortcutBar::SetFocusRectColor( COLORREF color, int iIndex ) {
    spdlog::debug("{} this={} color={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), color, iIndex);
}

COLORREF SECShortcutBar::GetFocusRectColor( int iIndex ) const {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return 0;
}

void SECShortcutBar::SetTextColor( COLORREF color, int iIndex ) {
    spdlog::debug("{} this={} color={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), color, iIndex);
}

COLORREF SECShortcutBar::GetTextColor(int iIndex) const {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return 0;
}

void SECShortcutBar::SetBkColor( COLORREF color, int iIndex ) {
    spdlog::debug("{} this={} color={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), color, iIndex);
}

COLORREF SECShortcutBar::GetBkColor(int iIndex) const {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return 0;
}

void SECShortcutBar::SetPaneBkColor( COLORREF color, int iIndex ) {
    spdlog::debug("{} this={} color={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), color, iIndex);
}

COLORREF SECShortcutBar::GetPaneBkColor(int iIndex) const {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return 0;
}

int SECShortcutBar::GetBarCount() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return static_cast<int>(m_bars.size());
}

CWnd* SECShortcutBar::GetBarWnd( int iIndex ) const {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    if (iIndex < 0 || iIndex >= static_cast<int>(m_bars.size())) {
        return nullptr;
    }
    return m_bars[iIndex]->pWnd;
}

void SECShortcutBar::SetAlignStyle( DWORD dwAlign ) {
    spdlog::debug("{} this={} dwAlign={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwAlign);
}

DWORD SECShortcutBar::GetAlignStyle() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

void SECShortcutBar::ModifyBarStyle( DWORD dwRemove, DWORD dwAdd, BOOL bRecalcRedraw ) {
    spdlog::debug("{} this={} dwRemove={} dwAdd={} bRecalcRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemove, dwAdd, bRecalcRedraw);
}

DWORD SECShortcutBar::GetBarStyle() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

SECBar& SECShortcutBar::GetActiveBar() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    static SECBar bar;
    return bar;
}

BOOL SECShortcutBar::HasActiveBar() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return m_nActiveBar >= 0 && m_nActiveBar < static_cast<int>(m_bars.size());
}

// -1 when no pane is open, not 0. CDefaultShortcutBar::OnNotifyChangeTab packs
// this into the command parameter it sends on, so naming pane zero when none is
// open is the same mistake the status bar's CommandToIndex was making.
int SECShortcutBar::GetActiveIndex() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return HasActiveBar() ? m_nActiveBar : -1;
}

BOOL SECShortcutBar::IsVertAlign() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECShortcutBar::IsHorzAlign() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SECShortcutBar::IsStyleSet( DWORD dwStyle ) const {
    spdlog::debug("{} this={} dwStyle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwStyle);
    return FALSE;
}

SECIterator<SECBar*>* SECShortcutBar::CreateBarIterator() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

// Returns a reference, so an out-of-range index has nothing honest to answer
// with. The empty static stays as the fallback for that case only, and says so
// in the log rather than handing back someone else's pane silently.
SECBar& SECShortcutBar::GetBar( int iIndex ) const {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    static SECBar bar;
    if (iIndex < 0 || iIndex >= static_cast<int>(m_bars.size())) {
        spdlog::warn("SECShortcutBar::GetBar: no pane {} of {}", iIndex, m_bars.size());
        return bar;
    }
    return *m_bars[iIndex];
}

SECBar* SECShortcutBar::GetBarPtr( int iIndex ) const {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    if (iIndex < 0 || iIndex >= static_cast<int>(m_bars.size())) {
        return nullptr;
    }
    return m_bars[iIndex];
}

// Which pane button is under that point, or -1 for none. In client coordinates,
// as the toolkit's is.
int SECShortcutBar::HitBar( const CPoint& pt ) {
    spdlog::debug("{} this={} pt.x={} pt.y={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), pt.x, pt.y);
    for (size_t i = 0; i < m_bars.size(); ++i) {
        const CButton *pButton = m_bars[i]->pButton;
        if (pButton == nullptr || pButton->GetSafeHwnd() == nullptr) {
            continue;
        }
        CRect rc;
        pButton->GetWindowRect(&rc);
        ScreenToClient(&rc);
        if (rc.PtInRect(pt)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void SECShortcutBar::SetFlatStyleMode( BOOL bEnabled ) {
    spdlog::debug("{} this={} bEnabled={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnabled);
}


auto SECShortcutBar::AddBar(CWnd *pWnd, LPCTSTR lpszLabel, BOOL bRecalc) -> SECBar * {
    spdlog::debug("{} this={} pWnd={} lpszLabel={} bRecalc={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pWnd), lpszLabel, bRecalc);
    return InsertBar(static_cast<int>(m_bars.size()), pWnd, lpszLabel, bRecalc);
}

SECBar* SECShortcutBar::AddBar(CRuntimeClass* pViewClass, LPCTSTR lpszLabel, CCreateContext* pContext, BOOL bRecalc, UINT nID) {
    spdlog::debug("{} this={} pViewClass={} lpszLabel={} pContext={} bRecalc={} nID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pViewClass), lpszLabel, spdlog::fmt_lib::ptr(pContext), bRecalc, nID);
    return nullptr;
}

SECListBar* SECShortcutBar::AddBar( LPCTSTR lpszLabel, BOOL bRecalc ) {
    spdlog::debug("{} this={} lpszLabel={} bRecalc={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszLabel, bRecalc);
    return nullptr;
}

SECBar* SECShortcutBar::InsertBar( int iIndex, CWnd* pWnd, LPCTSTR lpszLabel, BOOL bRecalc) {
    spdlog::debug("{} this={} iIndex={} pWnd={} lpszLabel={} bRecalc={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex, spdlog::fmt_lib::ptr(pWnd), lpszLabel, bRecalc);
    if (pWnd == nullptr) {
        return nullptr;
    }
    if (iIndex < 0 || iIndex > static_cast<int>(m_bars.size())) {
        iIndex = static_cast<int>(m_bars.size());
    }

    SECBar *pBar = new SECBar();
    pBar->pWnd = pWnd;
    pBar->strLabel = (lpszLabel != nullptr) ? lpszLabel : _T("");
    m_bars.insert(m_bars.begin() + iIndex, pBar);

    // The pane is a window the caller built, and it belongs inside this one now.
    if (pWnd->GetSafeHwnd() != nullptr && GetSafeHwnd() != nullptr && pWnd->GetParent() != this) {
        pWnd->SetParent(this);
    }
    if (pWnd->GetSafeHwnd() != nullptr) {
        pWnd->ShowWindow(SW_HIDE);
    }

    // The buttons carry their index in their command id, so every button after
    // the insertion point now has the wrong one. Rebuilding them is simpler than
    // renumbering, and only happens when a pane is added.
    for (int i = iIndex; i < static_cast<int>(m_bars.size()); ++i) {
        DestroyButton(i);
    }

    if (m_nActiveBar < 0) {
        ActivateBar(iIndex);
    } else {
        if (iIndex <= m_nActiveBar) {
            ++m_nActiveBar;
        }
        LayoutBars();
    }
    return pBar;
}

SECBar* SECShortcutBar::InsertBar( int iIndex, CRuntimeClass* pViewClass, LPCTSTR lpszLabel, CCreateContext* pContext, BOOL bRecalc, UINT uID ) {
    spdlog::debug("{} this={} iIndex={} pViewClass={} lpszLabel={} pContext={} bRecalc={} uID={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex, spdlog::fmt_lib::ptr(pViewClass), lpszLabel, spdlog::fmt_lib::ptr(pContext), bRecalc, uID);
    return nullptr;
}

SECListBar* SECShortcutBar::InsertBar( int iIndex, LPCTSTR lpszLabel, BOOL bRecalc) {
    spdlog::debug("{} this={} iIndex={} lpszLabel={} bRecalc={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex, lpszLabel, bRecalc);
    return nullptr;
}

// The pane window is not destroyed: the editor made it, keeps its own list and
// destroys it itself. CDefaultShortcutBar::RemoveAllShortcuts does exactly that.
void SECShortcutBar::RemoveBar( int iIndex ) {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    if (iIndex < 0 || iIndex >= static_cast<int>(m_bars.size()) || !OnRemoveBar(iIndex)) {
        return;
    }
    if (m_bars[iIndex]->pWnd != nullptr && m_bars[iIndex]->pWnd->GetSafeHwnd() != nullptr) {
        m_bars[iIndex]->pWnd->ShowWindow(SW_HIDE);
    }
    DestroyButton(iIndex);
    delete m_bars[iIndex];
    m_bars.erase(m_bars.begin() + iIndex);

    // As in InsertBar: the ids below the hole are now wrong.
    for (int i = iIndex; i < static_cast<int>(m_bars.size()); ++i) {
        DestroyButton(i);
    }

    if (m_bars.empty()) {
        m_nActiveBar = -1;
    } else if (m_nActiveBar >= static_cast<int>(m_bars.size())) {
        m_nActiveBar = static_cast<int>(m_bars.size()) - 1;
    } else if (iIndex < m_nActiveBar) {
        --m_nActiveBar;
    }
    LayoutBars();
}

void SECShortcutBar::RenameBar( int iIndex, LPCTSTR lpszLabel ) {
    spdlog::debug("{} this={} iIndex={} lpszLabel={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex, lpszLabel);
    if (iIndex < 0 || iIndex >= static_cast<int>(m_bars.size()) || lpszLabel == nullptr) {
        return;
    }
    m_bars[iIndex]->strLabel = lpszLabel;
    if (m_bars[iIndex]->pButton != nullptr && m_bars[iIndex]->pButton->GetSafeHwnd() != nullptr) {
        m_bars[iIndex]->pButton->SetWindowText(lpszLabel);
    }
}

// Open one pane. OnChangeBar is asked first, which is how a derived bar refuses
// or reacts: the editor's overrides it to run the command bound to that pane.
void SECShortcutBar::ActivateBar(int nIndex) {
    spdlog::debug("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    if (nIndex < 0 || nIndex >= static_cast<int>(m_bars.size())) {
        return;
    }
    if (!m_bars[nIndex]->bEnabled) {
        return;
    }
    if (!OnChangeBar(nIndex)) {
        return;
    }
    m_nActiveBar = nIndex;
    LayoutBars();
}

void SECShortcutBar::DisableBar( int iIndex ) {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    if (iIndex < 0 || iIndex >= static_cast<int>(m_bars.size()) || !OnDisableBar(iIndex)) {
        return;
    }
    m_bars[iIndex]->bEnabled = FALSE;
    if (m_bars[iIndex]->pButton != nullptr && m_bars[iIndex]->pButton->GetSafeHwnd() != nullptr) {
        m_bars[iIndex]->pButton->EnableWindow(FALSE);
    }
}

void SECShortcutBar::EnableBar( int iIndex ) {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    if (iIndex < 0 || iIndex >= static_cast<int>(m_bars.size()) || !OnEnableBar(iIndex)) {
        return;
    }
    m_bars[iIndex]->bEnabled = TRUE;
    if (m_bars[iIndex]->pButton != nullptr && m_bars[iIndex]->pButton->GetSafeHwnd() != nullptr) {
        m_bars[iIndex]->pButton->EnableWindow(TRUE);
    }
}

void SECShortcutBar::OnStyleChange( DWORD dwRemovedStyles, DWORD dwAddedStyles ) {
    spdlog::debug("{} this={} dwRemovedStyles={} dwAddedStyles={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemovedStyles, dwAddedStyles);
}

// TRUE, where this used to say FALSE. These four are veto hooks: the toolkit
// calls them before doing the thing and a derived bar returns FALSE to stop it.
// Refusing by default means the base class can never open, remove, disable or
// enable a pane, which is not a neutral answer.
BOOL SECShortcutBar::OnChangeBar( int iIndex ) {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return TRUE;
}

BOOL SECShortcutBar::OnRemoveBar( int iIndex ) {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return TRUE;
}

BOOL SECShortcutBar::OnDisableBar( int iIndex ) {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return TRUE;
}

BOOL SECShortcutBar::OnEnableBar( int iIndex ) {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    return TRUE;
}

BOOL SECShortcutBar::OnCreatePaneWnd( CWnd* pwnd ) {
    spdlog::debug("{} this={} pwnd={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pwnd));
    return FALSE;
}

BOOL SECShortcutBar::OnCreateBar( SECBar* pbar ) {
    spdlog::debug("{} this={} pbar={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pbar));
    return FALSE;
}

void SECShortcutBar::ConstructGDIObjects() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

void SECShortcutBar::DestructGDIObjects() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

SECBar* SECShortcutBar::CreateNewBar() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

SECListBar* SECShortcutBar::CreateNewListBar() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

void SECShortcutBar::SelectPane( int iIndex ) {
    spdlog::debug("{} this={} iIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iIndex);
    ActivateBar(iIndex);
}

void RWSetDotNetStyle(bool enable) {
    spdlog::debug("{} enable={}", BOOST_CURRENT_FUNCTION, enable);
}
