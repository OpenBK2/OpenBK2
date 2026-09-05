#include "Toolkit/tbarpage.h"
#include "Toolkit/tbarmgr.h"

#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


namespace {

// Written out rather than std::min/std::max: windows.h defines min and max as
// macros here, which is exactly what breaks the std versions.
int Clamp(int nValue, int nLow, int nHigh) {
    if (nValue < nLow) {
        return nLow;
    }
    return ( nValue > nHigh ) ? nHigh : nValue;
}

// The prompt MFC keeps for a command: "status bar text\ntooltip", in one string
// resource numbered by the command id. Only the first half is a description.
CString CommandPrompt(UINT nID) {
    CString strPrompt;
    if (!strPrompt.LoadString(nID)) {
        return CString();
    }
    const int nSplit = strPrompt.Find(_T('\n'));
    if (nSplit >= 0) {
        strPrompt = strPrompt.Left(nSplit);
    }
    return strPrompt;
}

// The tooltip half of the same string, which is the short name of the command
// and so the best label for a button that has no face.
CString CommandLabel(UINT nID) {
    CString strPrompt;
    if (!strPrompt.LoadString(nID)) {
        return CString();
    }
    const int nSplit = strPrompt.Find(_T('\n'));
    if (nSplit >= 0) {
        return strPrompt.Mid(nSplit + 1);
    }
    return strPrompt;
}

// Every command in a menu and its submenus, in order, with the text the menu
// shows. The accelerator after the tab and the & of the mnemonic are dropped:
// they belong to the menu, not to the command.
void CollectMenuCommands(HMENU hMenu, std::vector<std::pair<UINT, CString>> *pOut) {
    if (hMenu == nullptr || pOut == nullptr) {
        return;
    }
    const int nCount = ::GetMenuItemCount(hMenu);
    for (int i = 0; i < nCount; ++i) {
        if (const HMENU hSub = ::GetSubMenu(hMenu, i)) {
            CollectMenuCommands(hSub, pOut);
            continue;
        }
        const UINT nID = ::GetMenuItemID(hMenu, i);
        if (nID == 0 || nID == static_cast<UINT>(-1)) {
            continue;   // a separator, or a popup GetSubMenu already took
        }
        TCHAR szText[256] = { 0 };
        ::GetMenuString(hMenu, i, szText, _countof(szText), MF_BYPOSITION);
        CString strText(szText);
        const int nTab = strText.Find(_T('\t'));
        if (nTab >= 0) {
            strText = strText.Left(nTab);
        }
        strText.Remove(_T('&'));
        if (!strText.IsEmpty()) {
            pOut->push_back(std::make_pair(nID, strText));
        }
    }
}

}  // namespace


/////////////////////////////////////////////////////////////////////////////
// SECToolBarSheet

// The caption is a string resource in the toolkit's own resources, so it is
// looked up the same way the page templates are: through the module chain
// rather than through whichever module happens to be current. Passing the id
// straight to CPropertySheet would have it call LoadString on the current
// module and come up empty.
SECToolBarSheet::SECToolBarSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
    : CPropertySheet(_T(""), pParentWnd, iSelectPage) {
    spdlog::debug("{} this={} nIDCaption={} pParentWnd={} iSelectPage={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDCaption, spdlog::fmt_lib::ptr(pParentWnd), iSelectPage);
    CString strCaption;
    if (nIDCaption != 0 && strCaption.LoadString(nIDCaption)) {
        SetTitle(strCaption);
    }
}

// OK is the only button that means anything here.
//
// Everything on both pages takes effect the moment it is changed -- a toolbar
// shown, an option toggled -- so there is nothing pending for Apply to do and
// nothing for Cancel to undo; leaving Cancel there would promise an undo that
// does not happen. Help is hidden for the same reason: the toolkit's help file
// is not part of this tree, so the button had nothing to open.
BOOL SECToolBarSheet::OnInitDialog() {
    const BOOL bResult = CPropertySheet::OnInitDialog();
    const UINT anHide[] = { ID_APPLY_NOW, IDCANCEL, IDHELP };
    for (UINT nID : anHide) {
        if (CWnd *pButton = GetDlgItem(nID)) {
            pButton->ShowWindow(SW_HIDE);
        }
    }
    return bResult;
}


/////////////////////////////////////////////////////////////////////////////
// SECCmdButtonGrid

BEGIN_MESSAGE_MAP(SECCmdButtonGrid, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
    ON_WM_VSCROLL()
    ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL SECCmdButtonGrid::CreateGrid(CWnd* pParent, const CRect& rect, UINT nID) {
    // A class of its own so the background is the dialog's, which is what the
    // group box around it is painted on.
    const LPCTSTR lpszClass = AfxRegisterWndClass(
        CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW),
        reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1), nullptr);
    return CWnd::CreateEx(WS_EX_CLIENTEDGE, lpszClass, nullptr,
                          WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP,
                          rect, pParent, nID);
}

void SECCmdButtonGrid::SetCells(std::vector<Cell> cells) {
    m_cells = std::move(cells);
    m_nSel = -1;
    m_nScroll = 0;
    UpdateScrollBar();
    if (GetSafeHwnd() != nullptr) {
        Invalidate();
    }
}

UINT SECCmdButtonGrid::GetSelectedID() const {
    if (m_nSel < 0 || static_cast<size_t>(m_nSel) >= m_cells.size()) {
        return 0;
    }
    return m_cells[m_nSel].nID;
}

// As many columns as fit at the minimum width, and then the width shared out
// between them so the row is filled. A fixed cell width left a strip of unused
// box at the right and clipped "Open resource..." for no reason.
int SECCmdButtonGrid::Columns() const {
    CRect rcClient;
    GetClientRect(&rcClient);
    return Clamp(rcClient.Width() / nMinCellWidth, 1, 64);
}

int SECCmdButtonGrid::CellWidth() const {
    CRect rcClient;
    GetClientRect(&rcClient);
    return Clamp(rcClient.Width() / Columns(), nMinCellWidth, 4096);
}

// Where one cell sits, in client coordinates, with the scroll applied. A cell
// above or below the window still gets a rectangle; the paint loop skips it.
CRect SECCmdButtonGrid::CellRect(int nIndex) const {
    const int nCols = Columns();
    const int nWidth = CellWidth();
    const int nRow = nIndex / nCols;
    const int nCol = nIndex % nCols;
    const int x = nCol * nWidth;
    const int y = (nRow - m_nScroll) * nCellHeight;
    return CRect(x, y, x + nWidth, y + nCellHeight);
}

void SECCmdButtonGrid::UpdateScrollBar() {
    if (GetSafeHwnd() == nullptr) {
        return;
    }
    CRect rcClient;
    GetClientRect(&rcClient);
    const int nCols = Columns();
    const int nRows = (static_cast<int>(m_cells.size()) + nCols - 1) / nCols;
    const int nPage = Clamp(rcClient.Height() / nCellHeight, 1, 1024);

    SCROLLINFO si = { 0 };
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = ( nRows > 0 ) ? ( nRows - 1 ) : 0;
    si.nPage = nPage;
    m_nScroll = Clamp(m_nScroll, 0, ( nRows > nPage ) ? ( nRows - nPage ) : 0);
    si.nPos = m_nScroll;
    SetScrollInfo(SB_VERT, &si, TRUE);
}

void SECCmdButtonGrid::OnSize(UINT nType, int cx, int cy) {
    CWnd::OnSize(nType, cx, cy);
    UpdateScrollBar();
}

BOOL SECCmdButtonGrid::OnEraseBkgnd(CDC* pDC) {
    CRect rcClient;
    GetClientRect(&rcClient);
    pDC->FillSolidRect(rcClient, ::GetSysColor(COLOR_BTNFACE));
    return TRUE;
}

void SECCmdButtonGrid::OnPaint() {
    CPaintDC dc(this);
    CRect rcClient;
    GetClientRect(&rcClient);

    // The dialog's font, not the window's: this window was created in code and
    // never got one, and the stock system font is the Windows 95 look the rest
    // of the editor has been moved off.
    CFont *pOldFont = nullptr;
    if (CWnd *pParent = GetParent()) {
        if (CFont *pFont = pParent->GetFont()) {
            pOldFont = dc.SelectObject(pFont);
        }
    }
    dc.SetBkMode(TRANSPARENT);

    for (size_t i = 0; i < m_cells.size(); ++i) {
        CRect rcCell = CellRect(static_cast<int>(i));
        if (rcCell.bottom <= rcClient.top || rcCell.top >= rcClient.bottom) {
            continue;
        }
        const Cell &cell = m_cells[i];
        const bool bSelected = ( static_cast<int>(i) == m_nSel );
        if (bSelected) {
            dc.FillSolidRect(rcCell, ::GetSysColor(COLOR_HIGHLIGHT));
        }
        dc.DrawEdge(&rcCell, bSelected ? BDR_SUNKENOUTER : BDR_RAISEDINNER, BF_RECT);

        CRect rcText = rcCell;
        rcText.DeflateRect(3, 2);
        if (cell.hImages != nullptr && cell.nImage >= 0) {
            ::ImageList_Draw(cell.hImages, cell.nImage, dc.GetSafeHdc(),
                             rcText.left, rcText.top + 1, ILD_TRANSPARENT);
            rcText.left += 20;
        }
        dc.SetTextColor(::GetSysColor(bSelected ? COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT));
        dc.DrawText(cell.strText, &rcText,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    }

    if (pOldFont != nullptr) {
        dc.SelectObject(pOldFont);
    }
}

void SECCmdButtonGrid::OnLButtonDown(UINT nFlags, CPoint point) {
    CWnd::OnLButtonDown(nFlags, point);
    const int nCols = Columns();
    const int nRow = m_nScroll + point.y / nCellHeight;
    const int nCol = point.x / CellWidth();
    const int nIndex = ( nCol < nCols ) ? ( nRow * nCols + nCol ) : -1;
    m_nSel = ( nIndex >= 0 && static_cast<size_t>(nIndex) < m_cells.size() ) ? nIndex : -1;
    Invalidate();
    // Reported as a plain BN_CLICKED so the page can handle it with
    // ON_BN_CLICKED on this window's id, like any other control.
    if (CWnd *pParent = GetParent()) {
        pParent->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED),
                             reinterpret_cast<LPARAM>(GetSafeHwnd()));
    }
}

void SECCmdButtonGrid::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) {
    SCROLLINFO si = { 0 };
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    GetScrollInfo(SB_VERT, &si);
    const int nMaxPos = Clamp(si.nMax - static_cast<int>(si.nPage) + 1, 0, si.nMax);
    int nNew = m_nScroll;
    switch (nSBCode) {
        case SB_LINEUP:       nNew -= 1; break;
        case SB_LINEDOWN:     nNew += 1; break;
        case SB_PAGEUP:       nNew -= static_cast<int>(si.nPage); break;
        case SB_PAGEDOWN:     nNew += static_cast<int>(si.nPage); break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION: nNew = si.nTrackPos; break;
        case SB_TOP:          nNew = 0; break;
        case SB_BOTTOM:       nNew = nMaxPos; break;
        default: break;
    }
    nNew = Clamp(nNew, 0, nMaxPos);
    if (nNew != m_nScroll) {
        m_nScroll = nNew;
        SetScrollPos(SB_VERT, m_nScroll, TRUE);
        Invalidate();
    }
    CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}


/////////////////////////////////////////////////////////////////////////////
// SECToolBarCmdPage

BEGIN_MESSAGE_MAP(SECToolBarCmdPage, CPropertyPage)
    ON_LBN_SELCHANGE(IDC_SEC_LIST, &SECToolBarCmdPage::OnSelChangeCategory)
    ON_BN_CLICKED(IDC_SEC_BUTTON_GRID, &SECToolBarCmdPage::OnGridSelect)
END_MESSAGE_MAP()

// See SECToolBarsPage for why the resource handle is looked up rather than
// taken from whichever module is current.
SECToolBarCmdPage::SECToolBarCmdPage() : CPropertyPage(IDD) {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    m_psp.hInstance = AfxFindResourceHandle(MAKEINTRESOURCE(IDD), RT_DIALOG);
}

// The editor passes SECToolBarCmdPage::IDD here, so the template it names is
// this one; the caption id names a string in the *editor's* resources ("Command"
// at IDS_TOOLBAR_CUSTOMIZE_COMMAND) and is looked up by CPropertyPage the
// ordinary way.
SECToolBarCmdPage::SECToolBarCmdPage(UINT nIDTemplate, UINT nIDCaption)
    : CPropertyPage(nIDTemplate, nIDCaption) {
    spdlog::debug("{} this={} nIDTemplate={} nIDCaption={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIDTemplate, nIDCaption);
    m_psp.hInstance = AfxFindResourceHandle(MAKEINTRESOURCE(nIDTemplate), RT_DIALOG);
}

SECToolBarCmdPage::SECToolBarCmdPage(LPCTSTR lpszTemplate, UINT nIDCaption)
    : CPropertyPage(lpszTemplate, nIDCaption) {
    spdlog::debug("{} this={} lpszTemplate={} nIDCaption={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), SafeString( lpszTemplate ), nIDCaption);
    m_psp.hInstance = AfxFindResourceHandle(lpszTemplate, RT_DIALOG);
}

void SECToolBarCmdPage::SetManager(SECToolBarManager* pManager) {
    spdlog::debug("{} this={} pManager={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pManager));
    m_pManager = pManager;
}

void SECToolBarCmdPage::DefineBtnGroup(LPCTSTR lpszTitle, int nBtnCount, UINT* lpBtnIDs) {
    spdlog::debug("{} this={} lpszTitle={} nBtnCount={} lpBtnIDs={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), SafeString( lpszTitle ), nBtnCount, spdlog::fmt_lib::ptr(lpBtnIDs));
    CmdGroup group;
    group.strTitle = ( lpszTitle != nullptr ) ? lpszTitle : _T("");
    group.bMenu = false;
    for ( int i = 0; i < nBtnCount && lpBtnIDs != nullptr; ++i ) {
        // Separators carry no command and are not offered as buttons.
        if ( lpBtnIDs[i] != 0 ) {
            group.btnIDs.push_back( lpBtnIDs[i] );
        }
    }
    m_groups.push_back( group );
}

void SECToolBarCmdPage::DefineMenuGroup(LPCTSTR lpszTitle) {
    spdlog::debug("{} this={} lpszTitle={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), SafeString( lpszTitle ));
    CmdGroup group;
    group.strTitle = ( lpszTitle != nullptr ) ? lpszTitle : _T("");
    // The commands come from the manager's menu resources rather than from a
    // list handed in here; see CellsForGroup.
    group.bMenu = true;
    m_groups.push_back( group );
}

// One cell per command, with its toolbar face where the manager has one.
//
// A menu group's commands are the items of every menu resource SetMenuInfo was
// given; most of those appear on no toolbar, which is why a cell falls back to
// the command's own text.
std::vector<SECCmdButtonGrid::Cell> SECToolBarCmdPage::CellsForGroup(const CmdGroup& group) const {
    std::vector<std::pair<UINT, CString>> commands;
    if (group.bMenu) {
        if (m_pManager != nullptr) {
            for (UINT nMenuID : m_pManager->GetMenuIDs()) {
                const HINSTANCE hInst = AfxFindResourceHandle(MAKEINTRESOURCE(nMenuID), RT_MENU);
                const HMENU hMenu = ::LoadMenu(hInst, MAKEINTRESOURCE(nMenuID));
                if (hMenu == nullptr) {
                    spdlog::warn("SECToolBarCmdPage: menu resource {} not found", nMenuID);
                    continue;
                }
                CollectMenuCommands(hMenu, &commands);
                ::DestroyMenu(hMenu);
            }
        }
    } else {
        for (UINT nID : group.btnIDs) {
            commands.push_back(std::make_pair(nID, CommandLabel(nID)));
        }
    }

    std::vector<SECCmdButtonGrid::Cell> cells;
    cells.reserve(commands.size());
    for (const std::pair<UINT, CString> &cmd : commands) {
        SECCmdButtonGrid::Cell cell;
        cell.nID = cmd.first;
        cell.strText = cmd.second;
        if (cell.strText.IsEmpty()) {
            cell.strText.Format(_T("%u"), cmd.first);
        }
        if (m_pManager != nullptr) {
            m_pManager->GetButtonImage(cell.nID, &cell.hImages, &cell.nImage);
        }
        cells.push_back(cell);
    }
    return cells;
}

BOOL SECToolBarCmdPage::OnInitDialog() {
    CPropertyPage::OnInitDialog();

    m_wndCategories.SubclassDlgItem(IDC_SEC_LIST, this);
    for (const CmdGroup &group : m_groups) {
        m_wndCategories.AddString(group.strTitle);
    }

    // Inside the "Buttons" group box, clear of its frame and its title. The
    // group box stays: it is what draws the frame and the caption.
    CRect rcBox(0, 0, 0, 0);
    if (CWnd *pBox = GetDlgItem(IDC_SEC_BUTTONS_BOX)) {
        pBox->GetWindowRect(&rcBox);
        ScreenToClient(&rcBox);
        rcBox.DeflateRect(4, 12, 4, 4);
    }
    m_wndGrid.CreateGrid(this, rcBox, IDC_SEC_BUTTON_GRID);

    if (!m_groups.empty()) {
        m_wndCategories.SetCurSel(0);
        OnSelChangeCategory();
    }

    return TRUE;
}

void SECToolBarCmdPage::OnSelChangeCategory() {
    const int nSel = m_wndCategories.GetCurSel();
    if (nSel < 0 || static_cast<size_t>(nSel) >= m_groups.size()) {
        return;
    }
    m_wndGrid.SetCells(CellsForGroup(m_groups[nSel]));
    SetDlgItemText(IDC_SEC_DESCRIPTION, _T(""));
}

void SECToolBarCmdPage::OnGridSelect() {
    const UINT nID = m_wndGrid.GetSelectedID();
    SetDlgItemText(IDC_SEC_DESCRIPTION, ( nID != 0 ) ? CommandPrompt(nID) : CString());
}
