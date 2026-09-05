#include "Toolkit/trcore.h"

#include <windowsx.h>
#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"

// The toolkit draws its own tree, with columns, multiple selection, hidden and
// disabled items and an animated expand. This library has none of that drawing,
// so the window it creates is the common control, SysTreeView32, and every call
// that the common control also has is forwarded to it as a message. What the
// control has no counterpart for is still a stub that logs and answers nothing:
// the columns, the toolkit's node and item indices, hidden and disabled items,
// the filter level and the item colours. Where a forward had to give a poorer
// answer than the toolkit would have, the reason is written at that function.
//
// A call made before Create or after the window is gone sends to a null window,
// which returns zero rather than faulting, so none of the forwards below need to
// guard for it.

namespace {

// Every style bit the common control itself defines. "Set the style" then means
// replacing exactly these and leaving WS_ alone.
const DWORD TREE_STYLE_MASK = TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT
    | TVS_EDITLABELS | TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS | TVS_RTLREADING
    | TVS_NOTOOLTIPS | TVS_CHECKBOXES | TVS_TRACKSELECT | TVS_SINGLEEXPAND
    | TVS_INFOTIP | TVS_FULLROWSELECT | TVS_NOSCROLL | TVS_NONEVENHEIGHT
    | TVS_NOHSCROLL;

// The extended style word the toolkit takes is a mixture: the caller puts
// WS_EX_CLIENTEDGE in it alongside TVXS_ and LVXS_ bits. The toolkit's own bits
// live in the high half (see trcore.h), so the low half is what Windows gets.
const DWORD TREE_STYLE_EX_WINDOW_MASK = 0x0000FFFFu;

// The common control has no recursive expand, so the toolkit's "completely"
// variants walk the subtree here. A null item is the tree's own root, which has
// no expanded state of its own and is only walked through.
void ExpandSubtree( HWND hWnd, HTREEITEM hItem, UINT nCode )
{
    if ( hItem != nullptr ) {
        TreeView_Expand( hWnd, hItem, nCode );
    }
    for ( HTREEITEM hChild = TreeView_GetChild( hWnd, hItem ); hChild != nullptr;
          hChild = TreeView_GetNextSibling( hWnd, hChild ) ) {
        ExpandSubtree( hWnd, hChild, nCode );
    }
}

// Same for a recursive sort: the control sorts one item's children per message.
void SortSubtree( HWND hWnd, HTREEITEM hItem )
{
    TreeView_SortChildren( hWnd, hItem, FALSE );
    for ( HTREEITEM hChild = TreeView_GetChild( hWnd, hItem ); hChild != nullptr;
          hChild = TreeView_GetNextSibling( hWnd, hChild ) ) {
        SortSubtree( hWnd, hChild );
    }
}

// The header's control id. It is a sibling of the tree, so this shares a
// namespace with whatever dialog the tree is on, which is why it is well above
// the ids those use. Nothing looks the header up by it -- MFC reflects a
// notification to the window it came from, not to the id -- so two trees on one
// dialog giving their headers the same id costs nothing.
const UINT ID_TREE_HEADER = 0x7FF0;

// A column format is an LVCFMT_ value, of which only the alignment is anything
// this can act on. LVCFMT_LEFT, _RIGHT and _CENTER happen to be 0, 1 and 2, and
// so do HDF_LEFT, _RIGHT and _CENTER, but that is a coincidence rather than a
// guarantee, so it is spelled out both ways round.
int HeaderFormatFromColumnFormat( int nFormat )
{
    switch ( nFormat & LVCFMT_JUSTIFYMASK ) {
    case LVCFMT_RIGHT:  return HDF_RIGHT;
    case LVCFMT_CENTER: return HDF_CENTER;
    default:            return HDF_LEFT;
    }
}

UINT DrawTextFormatFromColumnFormat( int nFormat )
{
    switch ( nFormat & LVCFMT_JUSTIFYMASK ) {
    case LVCFMT_RIGHT:  return DT_RIGHT;
    case LVCFMT_CENTER: return DT_CENTER;
    default:            return DT_LEFT;
    }
}

// CLR_NONE is what the control answers when no colour was set, and it is not a
// colour anything can be painted in.
COLORREF ColorOrDefault( COLORREF rgb, int nSysColor )
{
    return rgb == static_cast< COLORREF >( CLR_NONE ) ? ::GetSysColor( nSysColor ) : rgb;
}

}  // namespace


BEGIN_MESSAGE_MAP( SECTreeHeaderCtrl, CHeaderCtrl )
    // Both spellings: which one a header sends depends on the character width
    // the caller was built with, and this library has been compiled both ways.
    ON_NOTIFY_REFLECT( HDN_ITEMCHANGEDA, OnItemChanged )
    ON_NOTIFY_REFLECT( HDN_ITEMCHANGEDW, OnItemChanged )
END_MESSAGE_MAP()


SECTreeHeaderCtrl::SECTreeHeaderCtrl() : m_pTree(nullptr) {
}

void SECTreeHeaderCtrl::OnItemChanged( NMHDR *pNMHDR, LRESULT *pResult ) {
    *pResult = 0;
    const NMHEADER *const pHeader = reinterpret_cast< NMHEADER * >( pNMHDR );
    if (m_pTree == nullptr || pHeader->pitem == nullptr
        || ( pHeader->pitem->mask & HDI_WIDTH ) == 0) {
        return;
    }
    m_pTree->SetColumnWidthFromHeader( pHeader->iItem, pHeader->pitem->cxy );
}


BEGIN_MESSAGE_MAP( SEC_TREECLASS, CWnd )
    ON_WM_NCCALCSIZE()
    ON_WM_WINDOWPOSCHANGED()
    ON_WM_NCDESTROY()
    ON_WM_HSCROLL()
    ON_WM_LBUTTONDOWN()
    // The control sends its custom draw to its parent, and MFC reflects it back
    // here, which is the same route TVN_SELCHANGED already takes to the editor's
    // own tree classes. Nothing between here and the control handles it, so the
    // reflection is not competing with anyone.
    ON_NOTIFY_REFLECT( NM_CUSTOMDRAW, OnCustomDraw )
    // TVN_SELCHANGED and TVN_DELETEITEM are wanted too and are deliberately not
    // here: see OnChildNotify for why a map entry would never have run.
END_MESSAGE_MAP()


SEC_TREECLASS::SEC_TREECLASS() : m_dwTreeCtrlStyleEx(0), m_bStoreSubItemText(FALSE),
    m_bHeaderEnabled(FALSE), m_nHeaderHeight(0), m_bHeaderInset(false),
    m_dwListCtrlStyle(0), m_dwListCtrlStyleEx(0),
    m_pHeaderImageList(nullptr), m_bSyncingHeader(false),
    m_rectHeaderPlaced(0, 0, 0, 0), m_rectHeaderVisible(0, 0, 0, 0) {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    // Column zero exists before anyone inserts anything: the editor sets its
    // heading, format, width and image first and only then inserts the rest.
    // The width is what SetColumnWidth is about to overwrite; it is here so
    // that a tree nobody configured still answers a usable number rather than
    // zero, which is what PC_MainTreeControl sizes its in-place editor from.
    SColumn first;
    first.nFormat = LVCFMT_LEFT;
    first.nWidth = 100;
    first.nSubItem = 0;
    first.nImage = -1;
    m_columns.push_back(first);
}

int SEC_TREECLASS::GetActiveColumn() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    // The toolkit tracked which column the user was working in, for in-place
    // editing and for the sort arrow. Nothing here tracks that yet, so the
    // first column is the answer, and it is the right one for a tree that is
    // not drawing the others.
    return 0;
}

void SEC_TREECLASS::ForgetSubItemText( HTREEITEM hItem ) {
    // A null item or TVI_ROOT means the whole tree, which is also what
    // TreeView_DeleteItem takes them to mean.
    if (hItem == nullptr || hItem == TVI_ROOT) {
        m_subItemText.clear();
        return;
    }
    // Deleting an item deletes its children with it, and their handles go with
    // them, so the subtree has to be walked before the control forgets it.
    for (HTREEITEM hChild = TreeView_GetChild(GetSafeHwnd(), hItem); hChild != nullptr;
         hChild = TreeView_GetNextSibling(GetSafeHwnd(), hChild)) {
        ForgetSubItemText(hChild);
    }
    m_subItemText.erase(hItem);
}

BOOL SEC_TREECLASS::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) {
    spdlog::debug("{} this={} dwStyle={} "
                  "rect.left={} rect.top={} rect.right={} rect.bottom={} "
                  "pParentWnd={} nID={} pContext={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this),
                  dwStyle, rect.left, rect.top, rect.right, rect.bottom,
                  spdlog::fmt_lib::ptr(pParentWnd), nID, spdlog::fmt_lib::ptr(pContext));
    // A real SysTreeView32, not a class of our own. MFC hooks the creation, so
    // the derived class keeps its message map and gets its reflected
    // notifications exactly as it would from CTreeCtrl.
    return CWnd::Create(WC_TREEVIEW, nullptr, dwStyle, rect, pParentWnd, nID, pContext);
}

BOOL SEC_TREECLASS::Create(DWORD dwStyle, DWORD dwStyleEx, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) {
    spdlog::debug("{} this={} dwStyle={} dwStyleEx={} "
                  "rect.left={} rect.top={} rect.right={} rect.bottom={} "
                  "pParentWnd={} nID={} pContext={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this),
                  dwStyle, dwStyleEx, rect.left, rect.top, rect.right, rect.bottom,
                  spdlog::fmt_lib::ptr(pParentWnd), nID, spdlog::fmt_lib::ptr(pContext));
    // dwStyleEx is the toolkit's word, not Windows': the caller mixes
    // WS_EX_CLIENTEDGE into the same value as TVXS_ and LVXS_ bits. Only the
    // window half may reach CreateEx; passing the whole word, as this used to,
    // turns TVXS_MULTISEL and friends into whatever WS_EX_ bit they collide
    // with. The toolkit half is kept so the style getters can answer with it.
    m_dwTreeCtrlStyleEx = dwStyleEx & ~TREE_STYLE_EX_WINDOW_MASK;
    // A multi-select tree needs TVS_SHOWSELALWAYS: only one of its selected
    // items is the control's caret, and the rest are drawn selected only while
    // the tree has the focus without it. Added here as well as in
    // UpdateMultiSelectStyle because at this point there is no window to
    // modify the style of, and ComboBox_GDBBrowser asks for TVXS_MULTISEL
    // right here, in the creation word.
    if ((m_dwTreeCtrlStyleEx & TVXS_MULTISEL) != 0) {
        dwStyle |= TVS_SHOWSELALWAYS;
    }
    return CWnd::CreateEx(dwStyleEx & TREE_STYLE_EX_WINDOW_MASK, WC_TREEVIEW, nullptr, dwStyle, rect, pParentWnd, nID, pContext);
}

BOOL SEC_TREECLASS::SubclassTreeCtrlId( UINT idc, CWnd *pWndDlg ) {
    spdlog::debug("{} this={} idc={} pWndDlg={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), idc, spdlog::fmt_lib::ptr(pWndDlg));
    if (!SubclassDlgItem(idc, pWndDlg)) {
        return FALSE;
    }
    return TRUE;
}

BOOL SEC_TREECLASS::GetItemRect( HTREEITEM hti, LPRECT lpRect, UINT nCode ) const {
    spdlog::debug("{} this={} hti={} lpRect={} nCode={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti), spdlog::fmt_lib::ptr(lpRect), nCode);
    // The control's only rectangle choice is text-only against whole-row, which
    // is what a caller passing a non-zero code is after.
    if (!TreeView_GetItemRect(GetSafeHwnd(), hti, lpRect, nCode != 0)) {
        return FALSE;
    }
    // "The whole line the item occupies" is the whole of the tree's own column,
    // not the whole of every column: the toolkit drew the columns after the
    // first itself, and the line it reported ended where they began.
    // CPCMainTreeControl reads it that way -- GetTreeItemEditorPlace does
    // itemRect.left = itemRect.right and then adds GetColumnWidth( 1 ), which
    // puts the in-place editor on column one only if that right edge is column
    // zero's. Against the control's answer, which runs to the client edge, the
    // editor started at the far right and was clamped to nothing.
    if (nCode == 0 && m_columns.size() > 1) {
        const int nColumnRight = GetColumnLeft( 1 );
        if (nColumnRight < lpRect->right) {
            lpRect->right = nColumnRight;
        }
    }
    return TRUE;
}

UINT SEC_TREECLASS::GetIndent() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return static_cast<UINT>(TreeView_GetIndent(GetSafeHwnd()));
}

void SEC_TREECLASS::SetIndent(UINT nIndent) {
    spdlog::debug("{} this={} nIndent={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndent);
    TreeView_SetIndent(GetSafeHwnd(), nIndent);
}

UINT SEC_TREECLASS::SetItemHeight(HTREEITEM hti, UINT cyItemHeight) const {
    spdlog::debug("{} this={} hti={} cyItemHeight={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti), cyItemHeight);
    // The common control keeps one item height for the whole tree, so hti has
    // nowhere to go: a per item height is part of the drawing the toolkit does
    // and this library does not.
    return static_cast<UINT>(TreeView_SetItemHeight(GetSafeHwnd(), static_cast<SHORT>(cyItemHeight)));
}

HTREEITEM SEC_TREECLASS::GetNextItem(HTREEITEM hItem, UINT nCode) const {
    spdlog::debug("{} this={} hItem={} cyItemHeight={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nCode);
    return TreeView_GetNextItem(GetSafeHwnd(), hItem, nCode);
}

HTREEITEM SEC_TREECLASS::GetChildItem(HTREEITEM hItem) const {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return TreeView_GetChild(GetSafeHwnd(), hItem);
}

HTREEITEM SEC_TREECLASS::GetNextSiblingItem(HTREEITEM hItem) const {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return TreeView_GetNextSibling(GetSafeHwnd(), hItem);
}

HTREEITEM SEC_TREECLASS::GetPrevSiblingItem(HTREEITEM hItem) const {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return TreeView_GetPrevSibling(GetSafeHwnd(), hItem);
}

HTREEITEM SEC_TREECLASS::GetParentItem(HTREEITEM hItem) const {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return TreeView_GetParent(GetSafeHwnd(), hItem);
}

HTREEITEM SEC_TREECLASS::GetFirstVisibleItem() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return TreeView_GetFirstVisible(GetSafeHwnd());
}

HTREEITEM SEC_TREECLASS::GetLastVisibleItem() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return TreeView_GetLastVisible(GetSafeHwnd());
}

HTREEITEM SEC_TREECLASS::GetNextVisibleItem(HTREEITEM hItem) const {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return TreeView_GetNextVisible(GetSafeHwnd(), hItem);
}

HTREEITEM SEC_TREECLASS::GetPrevVisibleItem(HTREEITEM hItem) const {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return TreeView_GetPrevVisible(GetSafeHwnd(), hItem);
}

// The three below are the walk the editor does over a selection:
//
//     HTREEITEM h = GetFirstSelectedItem();
//     while ( h ) { ...; h = GetNextSelectedItem( h ); }
//
// With one selection there was never a next one and the loop ran once. They
// answer from m_selection now, which is in display order, so the walk visits
// the selected items top to bottom. Without TVXS_MULTISEL nothing is kept and
// the control's own single selection is still the answer.
HTREEITEM SEC_TREECLASS::GetFirstSelectedItem() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    if (!IsMultiSelect()) {
        return TreeView_GetSelection(GetSafeHwnd());
    }
    return m_selection.empty() ? nullptr : m_selection.front();
}

HTREEITEM SEC_TREECLASS::GetNextSelectedItem(HTREEITEM hItem) const {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    if (!IsMultiSelect()) {
        return nullptr;
    }
    for (size_t i = 0; i + 1 < m_selection.size(); ++i) {
        if (m_selection[i] == hItem) {
            return m_selection[i + 1];
        }
    }
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetPrevSelectedItem(HTREEITEM hItem) const {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    if (!IsMultiSelect()) {
        return nullptr;
    }
    for (size_t i = 1; i < m_selection.size(); ++i) {
        if (m_selection[i] == hItem) {
            return m_selection[i - 1];
        }
    }
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetCaretItem() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return TreeView_GetSelection(GetSafeHwnd());
}

// The one selected item, for the callers that only want one.
//
// The caret, which is the ordinary answer and the same one this always gave.
// A Ctrl+click that deselects the caret item is the exception: the caret moves
// to another selected item, and only when nothing is left does this fall back
// to the set, which is then empty too.
HTREEITEM SEC_TREECLASS::GetSelectedItem() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    const HTREEITEM hCaret = TreeView_GetSelection(GetSafeHwnd());
    if (hCaret != nullptr || !IsMultiSelect()) {
        return hCaret;
    }
    return m_selection.empty() ? nullptr : m_selection.front();
}

HTREEITEM SEC_TREECLASS::GetDropHilightItem() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return TreeView_GetDropHilight(GetSafeHwnd());
}

HTREEITEM SEC_TREECLASS::GetRootItem(HTREEITEM hti) const {
    spdlog::debug("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    // With no item this is the tree's first root; with one it is the root of the
    // branch that item sits in, which is a walk up the parents.
    if (hti == nullptr) {
        return TreeView_GetRoot(GetSafeHwnd());
    }
    HTREEITEM hRoot = hti;
    for (HTREEITEM hParent = TreeView_GetParent(GetSafeHwnd(), hRoot); hParent != nullptr;
         hParent = TreeView_GetParent(GetSafeHwnd(), hRoot)) {
        hRoot = hParent;
    }
    return hRoot;
}

HTREEITEM SEC_TREECLASS::GetNextItemInDisplayOrder(HTREEITEM hti) const {
    spdlog::debug("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    // Display order is the order the control draws the expanded items in.
    return TreeView_GetNextVisible(GetSafeHwnd(), hti);
}

HTREEITEM SEC_TREECLASS::GetPrevItemInDisplayOrder(HTREEITEM hti) const {
    spdlog::debug("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return TreeView_GetPrevVisible(GetSafeHwnd(), hti);
}

BOOL SEC_TREECLASS::SetItem(const LV_ITEM* pLVI, BOOL bRedraw) {
    spdlog::debug("{} this={} pLVI={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pLVI), bRedraw);
    // The list side of the toolkit's item, which is its columns. Those exist
    // now and SetItemText fills them, but this overload cannot reach them: an
    // LV_ITEM names its row by iItem, a list index, and this tree addresses
    // items by HTREEITEM and has no stable row number to map one onto. The
    // toolkit could do it because it drew the rows and so knew their order.
    // Nothing in the editor calls this; if something starts to, it needs an
    // index the tree does not currently keep.
    return FALSE;
}

BOOL SEC_TREECLASS::SetItem(TV_ITEM* pItem) {
    spdlog::debug("{} this={} pItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pItem));
    return TreeView_SetItem(GetSafeHwnd(), pItem);
}

BOOL SEC_TREECLASS::SetItem(HTREEITEM hItem, UINT nMask, LPCTSTR lpszItem, int nImage, int nSelectedImage, UINT nState, UINT nStateMask, LPARAM lParam) {
    spdlog::debug("{} this={} hItem={} nMask={} lpszItem={} nImage={} nSelectedImage={} nState={} nStateMask={} lParam={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nMask, SafeString( lpszItem ), nImage, nSelectedImage, nState, nStateMask, lParam);
    TVITEM item = { 0 };
    item.hItem = hItem;
    item.mask = nMask;
    item.pszText = const_cast<LPTSTR>(lpszItem);
    item.iImage = nImage;
    item.iSelectedImage = nSelectedImage;
    item.state = nState;
    item.stateMask = nStateMask;
    item.lParam = lParam;
    return TreeView_SetItem(GetSafeHwnd(), &item);
}

BOOL SEC_TREECLASS::SetItemEx(TV_ITEM* pTVI, TV_ITEM_EX* pTVIX) {
    spdlog::debug("{} this={} pTVI={} pTVIX={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pTVI), spdlog::fmt_lib::ptr(pTVIX));
    return FALSE;
}

BOOL SEC_TREECLASS::SetItemEx(LV_ITEM* pLVI, const LV_ITEM_EX* pLVIX) {
    spdlog::debug("{} this={} pTVI={} pTVIX={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pLVI), spdlog::fmt_lib::ptr(pLVIX));
    return FALSE;
}

BOOL SEC_TREECLASS::SetItemImage(HTREEITEM hItem, int nImage, int nSelectedImage) {
    spdlog::debug("{} this={} hItem={} nImage={} nSelectedImage={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nImage, nSelectedImage);
    return SetItem(hItem, TVIF_IMAGE | TVIF_SELECTEDIMAGE, nullptr, nImage, nSelectedImage, 0, 0, 0);
}

BOOL SEC_TREECLASS::SetItemState(HTREEITEM hItem, UINT nState, UINT nStateMask) {
    spdlog::debug("{} this={} hItem={} nState={} nStateMask={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nState, nStateMask);
    return SetItem(hItem, TVIF_STATE, nullptr, 0, 0, nState, nStateMask, 0);
}

BOOL SEC_TREECLASS::SetItemData(HTREEITEM hItem, SEC_ITEMDATA dwData) {
    spdlog::debug("{} this={} hItem={} dwData={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), dwData);
    // This took SEC_DWORD, the toolkit's DWORD, which is 32 bit while the
    // control's lParam is pointer sized. The comment here used to say the
    // editor's tree classes do not use item data, so nothing was losing a
    // pointer. That was wrong: CSortTreeControl::InsertTreeItem stores the
    // item's own HTREEITEM in it, and half of it was going over the side on
    // x64. Nothing reads it back yet, which is the only reason it never
    // faulted. See SEC_ITEMDATA in trcore.h.
    return SetItem(hItem, TVIF_PARAM, nullptr, 0, 0, 0, 0, dwData);
}

BOOL SEC_TREECLASS::GetItem(TV_ITEM* pItem, BOOL bCopyText, BOOL bGetDispInfo) const {
    spdlog::debug("{} this={} pItem={} bCopyText={} bGetDispInfo={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pItem), bCopyText, bGetDispInfo);
    // The caller fills in the mask and the text buffer, as it would for the
    // control itself; bCopyText and bGetDispInfo are the toolkit's own cache,
    // which is not here to bypass.
    return TreeView_GetItem(GetSafeHwnd(), pItem);
}

BOOL SEC_TREECLASS::GetItem(LV_ITEM* pLVI, BOOL bCopyText, BOOL bGetDispInfo) const {
    spdlog::debug("{} this={} pLVI={} bCopyText={} bGetDispInfo={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pLVI), bCopyText, bGetDispInfo);
    // The list side of the toolkit's item, which is its columns.
    return FALSE;
}

BOOL SEC_TREECLASS::GetItemEx(TV_ITEM* pTVI, TV_ITEM_EX* pTVIX, BOOL bGetDispInfoEx) const {
    spdlog::debug("{} this={} pTVI={} pTVIX={} bGetDispInfoEx={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pTVI), spdlog::fmt_lib::ptr(pTVIX), bGetDispInfoEx);
    return FALSE;
}

BOOL SEC_TREECLASS::GetItemEx(LV_ITEM* pLVI, LV_ITEM_EX* pLVIX, BOOL bGetDispInfoEx) const {
    spdlog::debug("{} this={} pLVI={} pLVIX={} bGetDispInfoEx={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pLVI), spdlog::fmt_lib::ptr(pLVIX), bGetDispInfoEx);
    return FALSE;
}

BOOL SEC_TREECLASS::GetItemImage(HTREEITEM hItem, int& nImage, int& nSelectedImage) const {
    spdlog::debug("{} this={} hItem={} nImage={} nSelectedImage={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nImage, nSelectedImage);
    TVITEM item = { 0 };
    item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    item.hItem = hItem;
    if (!TreeView_GetItem(GetSafeHwnd(), &item)) {
        return FALSE;
    }
    nImage = item.iImage;
    nSelectedImage = item.iSelectedImage;
    return TRUE;
}

UINT SEC_TREECLASS::GetItemState(HTREEITEM hItem, UINT nStateMask) const {
    spdlog::debug("{} this={} hItem={} nStateMask={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nStateMask);
    return TreeView_GetItemState(GetSafeHwnd(), hItem, nStateMask);
}

SEC_ITEMDATA SEC_TREECLASS::GetItemData(HTREEITEM hItem) const {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    TVITEM item = { 0 };
    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    if (!TreeView_GetItem(GetSafeHwnd(), &item)) {
        return 0;
    }
    return item.lParam;
}

Node* SEC_TREECLASS::GetNode( HTREEITEM hti ) const {
    spdlog::debug("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return nullptr;
}

Node* SEC_TREECLASS::GetNode( Item* pItem ) const {
    spdlog::debug("{} this={} pItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pItem));
    return nullptr;
}

Node* SEC_TREECLASS::GetNodeAt( int nIndex ) const {
    spdlog::debug("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    return nullptr;
}

BOOL SEC_TREECLASS::SetNodeParent( SECTreeNode *pNode, SECTreeNode *pNodeParent, BOOL bInvalidate, HTREEITEM hInsertAfter ) {
    spdlog::debug("{} this={} pNode={} pNodeParent={} bInvalidate={} hInsertAfter={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pNode), spdlog::fmt_lib::ptr(pNodeParent), bInvalidate, spdlog::fmt_lib::ptr(hInsertAfter));
    return FALSE;
}

HTREEITEM SEC_TREECLASS::GetItemAt(int nIndex) const {
    spdlog::debug("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    return nullptr;
}

int SEC_TREECLASS::ItemIndex(HTREEITEM, int nStartFrom) const {
    spdlog::debug("{} this={} nStartFrom={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nStartFrom);
    return 0;
}

int SEC_TREECLASS::NodeIndex(Node *pNode, int nStartFrom) const {
    spdlog::debug("{} this={} pNode={} nStartFrom={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pNode), nStartFrom);
    return 0;
}

BOOL SEC_TREECLASS::IsExpanded(HTREEITEM hItem) const {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return (TreeView_GetItemState(GetSafeHwnd(), hItem, TVIS_EXPANDED) & TVIS_EXPANDED) != 0;
}

BOOL SEC_TREECLASS::IsCallbackItem(int nIndex) const {
    spdlog::debug("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    return FALSE;
}

void SEC_TREECLASS::StoreSubItemText( BOOL bEnable ) {
    spdlog::debug("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
    m_bStoreSubItemText = bEnable;
    if (!bEnable) {
        m_subItemText.clear();
    }
}

BOOL SEC_TREECLASS::IsStoringSubItemText() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return m_bStoreSubItemText;
}

CString SEC_TREECLASS::GetItemText(HTREEITEM hItem, int iSubItem) const {
    spdlog::debug("{} this={} hItem={} iSubItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), iSubItem);
    if (iSubItem > 0) {
        // Column zero's text is the control's; the rest is only ever what was
        // handed to SetItemText, and only when the tree was asked to keep it.
        const std::map<HTREEITEM, std::vector<CString> >::const_iterator it = m_subItemText.find(hItem);
        if (it == m_subItemText.end() || iSubItem >= static_cast<int>(it->second.size())) {
            return CString();
        }
        return it->second[iSubItem];
    }
    TCHAR szText[512] = { 0 };
    TVITEM item = { 0 };
    item.mask = TVIF_TEXT;
    item.hItem = hItem;
    item.pszText = szText;
    item.cchTextMax = _countof(szText);
    if (!TreeView_GetItem(GetSafeHwnd(), &item)) {
        return CString();
    }
    return CString(szText);
}

BOOL SEC_TREECLASS::SetItemText(HTREEITEM hItem, LPCTSTR lpszItem) {
    spdlog::debug("{} this={} hItem={} lpszItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), SafeString( lpszItem ));
    return SetItem(hItem, TVIF_TEXT, lpszItem, 0, 0, 0, 0, 0);
}

BOOL SEC_TREECLASS::SetItemText(HTREEITEM hItem, int nSubItem, LPCTSTR lpszItem) {
    spdlog::debug("{} this={} hItem={} nSubItem={} lpszItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nSubItem, SafeString( lpszItem ));
    if (nSubItem > 0) {
        // FALSE here is now an answer rather than a refusal: it says the text
        // was not kept, which is true exactly when nobody asked for it to be.
        if (!m_bStoreSubItemText || hItem == nullptr) {
            return FALSE;
        }
        std::vector<CString> &text = m_subItemText[hItem];
        if (nSubItem >= static_cast<int>(text.size())) {
            text.resize(nSubItem + 1);
        }
        text[nSubItem] = lpszItem != nullptr ? lpszItem : _T("");
        return TRUE;
    }
    return SetItemText(hItem, lpszItem);
}

BOOL SEC_TREECLASS::SetItemString(HTREEITEM hti, int nSubItem, const CString& strItem) {
    spdlog::debug("{} this={} hti={} nSubItem={} strItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti), nSubItem, strItem.GetString());
    return SetItemText(hti, nSubItem, strItem);
}

BOOL SEC_TREECLASS::GetItemString(HTREEITEM hti, int nSubItem, CString& strItem) {
    spdlog::debug("{} this={} hti={} nSubItem={} strItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti), nSubItem, strItem.GetString());
    // The pair of this is SetItemString, so it answers from the same place:
    // the control for column zero, the stored text for the rest.
    strItem = GetItemText(hti, nSubItem);
    return TRUE;
}

BOOL SEC_TREECLASS::SetNoEllipsis(BOOL bNoEllipsis) {
    spdlog::debug("{} this={} bNoEllipsis={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bNoEllipsis);
    return FALSE;
}

BOOL SEC_TREECLASS::InsertBatch(TV_ITEM** ppItems, HTREEITEM hParent, int cItems, BOOL bInvalidate) {
    spdlog::debug("{} this={} ppItems={} hParent={} cItems={} bInvalidate={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(ppItems), spdlog::fmt_lib::ptr(hParent), cItems, bInvalidate);
    // The control inserts one item per message; the batch is only the toolkit
    // saving itself repaints, which bInvalidate would have controlled.
    for (int i = 0; i < cItems; ++i) {
        if (ppItems[i] == nullptr) {
            continue;
        }
        TVINSERTSTRUCT insert = { 0 };
        insert.hParent = hParent;
        insert.hInsertAfter = TVI_LAST;
        insert.item = *ppItems[i];
        if (TreeView_InsertItem(GetSafeHwnd(), &insert) == nullptr) {
            return FALSE;
        }
    }
    return TRUE;
}

HTREEITEM SEC_TREECLASS::InsertItem(LPTV_INSERTSTRUCT lpInsertStruct) {
    spdlog::debug("{} this={} lpInsertStruct={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(lpInsertStruct));
    return TreeView_InsertItem(GetSafeHwnd(), lpInsertStruct);
}

HTREEITEM SEC_TREECLASS::InsertItem(UINT nMask, LPCTSTR lpszItem, int nImage, int nSelectedImage, UINT nState, UINT nStateMask, LPARAM lParam, HTREEITEM hParent, HTREEITEM hInsertAfter) {
    spdlog::debug("{} this={} nMask={} lpszItem={} nImage={} nSelectedImage={} nState={} nStateMask={} lParam={} hParent={} hInsertAfter={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nMask, SafeString( lpszItem ), nImage, nSelectedImage, nState, nStateMask, lParam, spdlog::fmt_lib::ptr(hParent), spdlog::fmt_lib::ptr(hInsertAfter));
    TVINSERTSTRUCT insert = { 0 };
    insert.hParent = hParent;
    insert.hInsertAfter = hInsertAfter;
    insert.item.mask = nMask;
    insert.item.pszText = const_cast<LPTSTR>(lpszItem);
    insert.item.iImage = nImage;
    insert.item.iSelectedImage = nSelectedImage;
    insert.item.state = nState;
    insert.item.stateMask = nStateMask;
    insert.item.lParam = lParam;
    return TreeView_InsertItem(GetSafeHwnd(), &insert);
}

HTREEITEM SEC_TREECLASS::InsertItem(LPCTSTR lpszItem, HTREEITEM hParent, HTREEITEM hInsertAfter) {
    spdlog::debug("{} this={} lpszItem={} hParent={} hInsertAfter={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), SafeString( lpszItem ), spdlog::fmt_lib::ptr(hParent), spdlog::fmt_lib::ptr(hInsertAfter));
    return InsertItem(TVIF_TEXT, lpszItem, 0, 0, 0, 0, 0, hParent, hInsertAfter);
}

HTREEITEM SEC_TREECLASS::InsertItem(LPCTSTR lpszItem, int nImage, int nSelectedImage, HTREEITEM hParent, HTREEITEM hInsertAfter) {
    spdlog::debug("{} this={} lpszItem={} nImage={} nSelectedImage={} hParent={} hInsertAfter={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), SafeString( lpszItem ), nImage, nSelectedImage, spdlog::fmt_lib::ptr(hParent), spdlog::fmt_lib::ptr(hInsertAfter));
    return InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE, lpszItem, nImage, nSelectedImage, 0, 0, 0, hParent, hInsertAfter);
}

BOOL SEC_TREECLASS::DeleteItem(HTREEITEM hItem) {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    // Before the control deletes it: the walk needs the children to still be
    // there, and afterwards they are not.
    ForgetSubItemText(hItem);
    return TreeView_DeleteItem(GetSafeHwnd(), hItem);
}

BOOL SEC_TREECLASS::DeleteAllItems() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    m_subItemText.clear();
    return TreeView_DeleteAllItems(GetSafeHwnd());
}

BOOL SEC_TREECLASS::Expand(HTREEITEM hItem, UINT nCode, BOOL bRedraw, BOOL bForceExpand) {
    spdlog::debug("{} this={} hItem={} nCode={} bRedraw={} bForceExpand={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nCode, bRedraw, bForceExpand);
    // bForceExpand is the toolkit expanding an item that reports no children
    // yet; the control decides that from the item's own child count.
    return TreeView_Expand(GetSafeHwnd(), hItem, nCode);
}

void SEC_TREECLASS::ExpandCompletely(HTREEITEM hItem, BOOL bRedraw) {
    spdlog::debug("{} this={} hItem={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), bRedraw);
    ExpandSubtree(GetSafeHwnd(), hItem, TVE_EXPAND);
}

void SEC_TREECLASS::CollapseCompletely(HTREEITEM hItem, BOOL bRedraw) {
    spdlog::debug("{} this={} hItem={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), bRedraw);
    ExpandSubtree(GetSafeHwnd(), hItem, TVE_COLLAPSE);
}

BOOL SEC_TREECLASS::Select(HTREEITEM hItem, UINT nCode) {
    spdlog::debug("{} this={} hItem={} nCode={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nCode);
    return TreeView_Select(GetSafeHwnd(), hItem, nCode);
}

BOOL SEC_TREECLASS::SelectItem(HTREEITEM hItem) {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return TreeView_SelectItem(GetSafeHwnd(), hItem);
}

BOOL SEC_TREECLASS::SelectDropTarget(HTREEITEM hItem, BOOL bAutoScroll) {
    spdlog::debug("{} this={} hItem={} bAutoScroll={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), bAutoScroll);
    return TreeView_SelectDropTarget(GetSafeHwnd(), hItem);
}

BOOL SEC_TREECLASS::SelectSetFirstVisible(HTREEITEM hItem) {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return TreeView_SelectSetFirstVisible(GetSafeHwnd(), hItem);
}

BOOL SEC_TREECLASS::SetFirstVisible(HTREEITEM hti) {
    spdlog::debug("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return TreeView_SelectSetFirstVisible(GetSafeHwnd(), hti);
}

HTREEITEM SEC_TREECLASS::HitTest(CPoint pt, UINT* pFlags) {
    spdlog::debug("{} this={} pt.x={} pt.y={} pFlags={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), pt.x, pt.y, spdlog::fmt_lib::ptr(pFlags));
    TVHITTESTINFO hitTest = { 0 };
    hitTest.pt = pt;
    const HTREEITEM hItem = TreeView_HitTest(GetSafeHwnd(), &hitTest);
    if (pFlags != nullptr) {
        *pFlags = hitTest.flags;
    }
    return hItem;
}

HTREEITEM SEC_TREECLASS::HitTest(TV_HITTESTINFO* pHitTestInfo) {
    spdlog::debug("{} this={} pHitTestInfo={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pHitTestInfo));
    return TreeView_HitTest(GetSafeHwnd(), pHitTestInfo);
}

CImageList* SEC_TREECLASS::CreateDragImage(HTREEITEM hItem) {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return CImageList::FromHandle(TreeView_CreateDragImage(GetSafeHwnd(), hItem));
}

CImageList* SEC_TREECLASS::CreateDragImageEx(HTREEITEM hItem, CPoint& ptOffset) {
    spdlog::debug("{} this={} hItem={} ptOffset.x={} ptOffset.y={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), ptOffset.x, ptOffset.y);
    return nullptr;
}

BOOL SEC_TREECLASS::EnsureVisible(HTREEITEM hItem, BOOL bParentVisible) {
    spdlog::debug("{} this={} hItem={} bParentVisible={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), bParentVisible);
    // The control always expands the parents on the way, which is what
    // bParentVisible would otherwise have asked for.
    return TreeView_EnsureVisible(GetSafeHwnd(), hItem);
}

int SEC_TREECLASS::CompareItem(Node *pNode1, Node *pNode2) {
    spdlog::debug("{} this={} pNode1={} pNode2={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pNode1), spdlog::fmt_lib::ptr(pNode2));
    return 0;
}

BOOL SEC_TREECLASS::SortChildren(HTREEITEM hItem, BOOL bRecursive, BOOL bAscending) {
    spdlog::debug("{} this={} hItem={} bRecursive={} bAscending={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), bRecursive, bAscending);
    // The control sorts by label, ascending. Descending is the toolkit's own
    // comparison, so say when it is asked for rather than sort the other way.
    if (!bAscending) {
        spdlog::warn("SEC_TREECLASS::SortChildren: descending sort is not implemented, sorting ascending");
    }
    if (bRecursive) {
        SortSubtree(GetSafeHwnd(), hItem);
        return TRUE;
    }
    return TreeView_SortChildren(GetSafeHwnd(), hItem, FALSE);
}

BOOL SEC_TREECLASS::SortChildrenCB(LPTV_SORTCB pSort, BOOL bRecursive, BOOL bAscending) {
    spdlog::debug("{} this={} pSort={} bRecursive={} bAscending={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pSort), bRecursive, bAscending);
    if (bRecursive) {
        spdlog::warn("SEC_TREECLASS::SortChildrenCB: recursive sort is not implemented, sorting one level");
    }
    if (!bAscending) {
        spdlog::warn("SEC_TREECLASS::SortChildrenCB: descending sort is left to the callback");
    }
    return TreeView_SortChildrenCB(GetSafeHwnd(), pSort, FALSE);
}

CEdit* SEC_TREECLASS::EditLabel(HTREEITEM hItem, int nCol) {
    spdlog::debug("{} this={} hItem={} nCol={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nCol);
    if (nCol > 0) {
        // Editing a subitem needs the columns.
        return nullptr;
    }
    return reinterpret_cast<CEdit*>(CWnd::FromHandle(TreeView_EditLabel(GetSafeHwnd(), hItem)));
}

UINT SEC_TREECLASS::GetChildCount(HTREEITEM hti, BOOL bRecursive, BOOL bExpandedOnly) {
    spdlog::debug("{} this={} hti={} bRecursive={} bExpandedOnly={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti), bRecursive, bExpandedOnly);
    UINT nCount = 0;
    for (HTREEITEM hChild = TreeView_GetChild(GetSafeHwnd(), hti); hChild != nullptr;
         hChild = TreeView_GetNextSibling(GetSafeHwnd(), hChild)) {
        ++nCount;
        if (!bRecursive) {
            continue;
        }
        if (bExpandedOnly
            && (TreeView_GetItemState(GetSafeHwnd(), hChild, TVIS_EXPANDED) & TVIS_EXPANDED) == 0) {
            continue;
        }
        nCount += GetChildCount(hChild, bRecursive, bExpandedOnly);
    }
    return nCount;
}

BOOL SEC_TREECLASS::SelectAllVisibleChildren(HTREEITEM hti) {
    spdlog::debug("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    if (!IsMultiSelect() || hti == nullptr) {
        return FALSE;
    }
    AddVisibleChildren(hti);
    SortSelection();
    ApplySelection();
    return TRUE;
}

BOOL SEC_TREECLASS::SelectItemRange( HTREEITEM htiFirst, HTREEITEM htiLast, BOOL bSelect) {
    spdlog::debug("{} this={} htiFirst={} htiLast={} bSelect={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(htiFirst), spdlog::fmt_lib::ptr(htiLast), bSelect);
    if (!IsMultiSelect()) {
        return FALSE;
    }
    SelectRange(htiFirst, htiLast, bSelect != FALSE);
    ApplySelection();
    return TRUE;
}

BOOL SEC_TREECLASS::IsSelected(HTREEITEM hti) const {
    spdlog::debug("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    if (hti == nullptr) {
        return FALSE;
    }
    if (IsMultiSelect()) {
        return InSelection(hti) ? TRUE : FALSE;
    }
    return TreeView_GetSelection(GetSafeHwnd()) == hti;
}

BOOL SEC_TREECLASS::HideItem( HTREEITEM hti, BOOL bHide ) {
    spdlog::debug("{} this={} hti={} bHide={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti), bHide);
    return FALSE;
}

BOOL SEC_TREECLASS::IsHidden( HTREEITEM hti ) const {
    spdlog::debug("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return FALSE;
}

HTREEITEM SEC_TREECLASS::GetFirstHiddenItem() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetNextHiddenItem(HTREEITEM hti) const {
    spdlog::debug("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return nullptr;
}

BOOL SEC_TREECLASS::UnHideAllItems() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SEC_TREECLASS::DisableAllItems( BOOL bDisable ) {
    spdlog::debug("{} this={} bDisable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bDisable);
    return FALSE;
}


BOOL SEC_TREECLASS::DisableItem( HTREEITEM hti, BOOL bDisable ) {
    spdlog::debug("{} this={} hti={} bDisable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti), bDisable);
    return FALSE;
}

BOOL SEC_TREECLASS::IsDisabled( HTREEITEM hti) const {
    spdlog::debug("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return FALSE;
}

HTREEITEM SEC_TREECLASS::GetFirstDisabledItem() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetNextDisabledItem(HTREEITEM hti) const {
    spdlog::debug("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return nullptr;
}

void SEC_TREECLASS::HideDisabledItems(BOOL bHide) {
    spdlog::debug("{} this={} bHide={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bHide);
}

BOOL SEC_TREECLASS::IsHideDisabledItems() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

UINT SEC_TREECLASS::GetCount() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return TreeView_GetCount(GetSafeHwnd());
}

BOOL SEC_TREECLASS::ItemHasChildrenOnDemand(HTREEITEM hItem) const {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    TVITEM item = { 0 };
    item.mask = TVIF_CHILDREN;
    item.hItem = hItem;
    if (!TreeView_GetItem(GetSafeHwnd(), &item)) {
        return FALSE;
    }
    return item.cChildren == I_CHILDRENCALLBACK;
}

BOOL SEC_TREECLASS::ItemHasChildren(HTREEITEM hItem) const {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    // The item's own count rather than a look for a child, so an item that has
    // said it will produce children later still answers yes.
    TVITEM item = { 0 };
    item.mask = TVIF_CHILDREN;
    item.hItem = hItem;
    if (!TreeView_GetItem(GetSafeHwnd(), &item)) {
        return FALSE;
    }
    return item.cChildren != 0;
}

BOOL SEC_TREECLASS::ItemExists(HTREEITEM hti) const {
    spdlog::debug("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    // There is no message asking whether a handle is still live, but asking for
    // an item that has been deleted fails, which is the same answer.
    if (hti == nullptr) {
        return FALSE;
    }
    TVITEM item = { 0 };
    item.mask = TVIF_HANDLE;
    item.hItem = hti;
    return TreeView_GetItem(GetSafeHwnd(), &item);
}

void SEC_TREECLASS::EnableScrollOnExpand(BOOL bEnable) {
    spdlog::debug("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
    // The control states the opposite: TVS_NOSCROLL is scrolling turned off.
    ModifyStyle(bEnable ? TVS_NOSCROLL : 0, bEnable ? 0 : TVS_NOSCROLL);
}

CImageList* SEC_TREECLASS::GetImageList(UINT nImageList) const {
    spdlog::debug("{} this={} nImageList={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nImageList);
    return CImageList::FromHandle(TreeView_GetImageList(GetSafeHwnd(), nImageList));
}

CImageList* SEC_TREECLASS::SetImageList(CImageList* pImageList, int nImageListType) {
    spdlog::debug("{} this={} pImageList={} nImageListType={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pImageList), nImageListType);
    // The control has a normal list and a state list and no third slot, so
    // LVSIL_HEADER -- which is 3, a type number the control does not define --
    // used to be forwarded and dropped. It is the column headings' list, so it
    // goes to the header instead, which is what SetColumnImage indexes into.
    if (nImageListType == LVSIL_HEADER) {
        CImageList *const pOldImageList = m_pHeaderImageList;
        m_pHeaderImageList = pImageList;
        if (m_wndHeader.GetSafeHwnd() != nullptr) {
            m_wndHeader.SetImageList(pImageList);
            SyncHeaderColumns();
        }
        return pOldImageList;
    }
    const HIMAGELIST hOldImageList = TreeView_SetImageList(GetSafeHwnd(),
        pImageList != nullptr ? pImageList->GetSafeHandle() : nullptr, nImageListType);
    if (nImageListType == TVSIL_NORMAL && m_rgbIconBk != CLR_NONE) {
        // A colour asked for before there was a list to put it on.
        ApplyIconBkColor();
    }
    return CImageList::FromHandle(hOldImageList);
}

void SEC_TREECLASS::EnableHeaderCtrl(BOOL bEnable, BOOL bSortHeader) {
    spdlog::debug("{} this={} bEnable={} bSortHeader={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable, bSortHeader);
    // bSortHeader asked the toolkit to sort the tree when a heading was
    // clicked. Nothing here sorts on a click, so it is not acted on; the
    // headings are still buttons unless LVS_NOSORTHEADER says otherwise, which
    // is the style the editor actually sets.
    m_bHeaderEnabled = bEnable;
    UpdateHeaderCtrl();
}

void SEC_TREECLASS::EnableWordWrap(BOOL bEnable) {
    spdlog::debug("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
}

void SEC_TREECLASS::EnableToolTips(BOOL bEnable) {
    spdlog::debug("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
    ModifyStyle(bEnable ? TVS_NOTOOLTIPS : 0, bEnable ? 0 : TVS_NOTOOLTIPS);
}

void SEC_TREECLASS::EnableMultiSelect(BOOL bEnable) {
    spdlog::debug("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
    // The same switch the style word is, so both routes end in the same place.
    // The editor uses the style: ModifyTreeCtrlStyleEx( 0, TVXS_MULTISEL ) in
    // PC_Dialog and its two siblings, and the whole word at creation in
    // ComboBox_GDBBrowser.
    ModifyTreeCtrlStyleEx(bEnable ? 0 : TVXS_MULTISEL, bEnable ? TVXS_MULTISEL : 0);
}

BOOL SEC_TREECLASS::GetTreeCtrlStyles(DWORD& dwStyle, DWORD& dwStyleEx) const {
    spdlog::debug("{} this={} dwStyle={} dwStyleEx={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwStyle, dwStyleEx);
    dwStyle = GetStyle();
    dwStyleEx = m_dwTreeCtrlStyleEx;
    return TRUE;
}

BOOL SEC_TREECLASS::SetTreeCtrlStyles(DWORD dwStyle, DWORD dwStyleEx, BOOL bRedraw) {
    spdlog::debug("{} this={} dwStyle={} dwStyleEx={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwStyle, dwStyleEx, bRedraw);
    return SetTreeCtrlStyle(dwStyle, bRedraw) && SetTreeCtrlStyleEx(dwStyleEx, bRedraw);
}

DWORD SEC_TREECLASS::GetTreeCtrlStyle() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return GetStyle();
}

DWORD SEC_TREECLASS::GetTreeCtrlStyleEx() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    // Kept rather than read back: none of these bits is a window style, and
    // nothing in this library acts on them yet.
    return m_dwTreeCtrlStyleEx;
}

BOOL SEC_TREECLASS::SetTreeCtrlStyle(DWORD dwStyle, BOOL bRedraw) {
    spdlog::debug("{} this={} dwStyle={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwStyle, bRedraw);
    return ModifyStyle(TREE_STYLE_MASK, dwStyle & TREE_STYLE_MASK, bRedraw ? SWP_FRAMECHANGED : 0);
}

BOOL SEC_TREECLASS::SetTreeCtrlStyleEx(DWORD dwStyleEx, BOOL bRedraw) {
    spdlog::debug("{} this={} dwStyle={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwStyleEx, bRedraw);
    m_dwTreeCtrlStyleEx = dwStyleEx;
    UpdateMultiSelectStyle();
    return TRUE;
}

BOOL SEC_TREECLASS::ModifyTreeCtrlStyle(DWORD dwRemove, DWORD dwAdd, BOOL bRedraw) {
    spdlog::debug("{} this={} dwRemove={} dwAdd={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemove, dwAdd, bRedraw);
    return ModifyStyle(dwRemove, dwAdd, bRedraw ? SWP_FRAMECHANGED : 0);
}

BOOL SEC_TREECLASS::ModifyTreeCtrlStyleEx(DWORD dwRemoveEx, DWORD dwAddEx, BOOL bRedraw) {
    spdlog::debug("{} this={} dwRemoveEx={} dwAddEx={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemoveEx, dwAddEx, bRedraw);
    m_dwTreeCtrlStyleEx = (m_dwTreeCtrlStyleEx & ~dwRemoveEx) | dwAddEx;
    UpdateMultiSelectStyle();
    return TRUE;
}

BOOL SEC_TREECLASS::ModifyTreeCtrlStyles(DWORD dwRemove, DWORD dwAdd, DWORD dwRemoveEx, DWORD dwAddEx, BOOL bRedraw) {
    spdlog::debug("{} this={} dwRemove={} dwAdd={} dwRemoveEx={} dwAddEx={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemove, dwAdd, dwRemoveEx, dwAddEx, bRedraw);
    return ModifyTreeCtrlStyle(dwRemove, dwAdd, bRedraw)
        && ModifyTreeCtrlStyleEx(dwRemoveEx, dwAddEx, bRedraw);
}

void SEC_TREECLASS::SetFilterLevel(WORD wLevel) {
    spdlog::debug("{} this={} wLevel={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), wLevel);
}

WORD SEC_TREECLASS::GetFilterLevel() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

BOOL SEC_TREECLASS::Update( HTREEITEM hti, BOOL bLabelOnly, BOOL bEraseBkgnd, BOOL bUpdateBelow, BOOL bUpdateNow ) {
    spdlog::debug("{} this={} hti={} bLabelOnly={} bEraseBkgnd={} bUpdateBelow={} bUpdateNow={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti), bLabelOnly, bEraseBkgnd, bUpdateBelow, bUpdateNow);
    // bLabelOnly, bEraseBkgnd and bUpdateBelow are the toolkit's own painting;
    // the whole item's rectangle is repainted instead.
    if (!InvalidateItem(hti)) {
        return FALSE;
    }
    if (bUpdateNow) {
        UpdateWindow();
    }
    return TRUE;
}

inline BOOL SEC_TREECLASS::InvalidateItem(HTREEITEM hti) {
    spdlog::debug("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    CRect rectItem;
    // Not through GetItemRect: this wants the row across every column, since
    // the columns past the first are painted over it, and GetItemRect now
    // answers with column zero's share of it.
    if (!TreeView_GetItemRect(GetSafeHwnd(), hti, &rectItem, FALSE)) {
        return FALSE;
    }
    InvalidateRect(&rectItem);
    return TRUE;
}

void SEC_TREECLASS::ReMeasureItem( HTREEITEM hti ) {
    spdlog::debug("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
}

void SEC_TREECLASS::SetAutoExpandDelay( UINT nDelay ) {
    spdlog::debug("{} this={} nDelay={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nDelay);
}

void SEC_TREECLASS::SetMaxAnimations( int nMaxAnimations ) {
    spdlog::debug("{} this={} nMaxAnimations={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nMaxAnimations);
}

BOOL SEC_TREECLASS::InsertColumn( int nCol, const CString& strHeader, int nFormat, int wWidth, int iSubItem, int iImage, BOOL bUpdate ) {
    spdlog::debug("{} this={} nCol={} strHeader={} nFormat={} nWidth={} iSubItem={} iImage={} bUpdate={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol, strHeader.GetString(), nFormat, wWidth, iSubItem, iImage, bUpdate);
    if (nCol < 0 || nCol > static_cast<int>(m_columns.size())) {
        return FALSE;
    }
    SColumn column;
    column.strHeading = strHeader;
    column.nFormat = nFormat;
    column.nWidth = wWidth;
    column.nSubItem = iSubItem;
    column.nImage = iImage;
    m_columns.insert(m_columns.begin() + nCol, column);
    UpdateHeaderCtrl();
    // bUpdate asked the toolkit to repaint. The header has just been rebuilt
    // either way, because a heading that disagrees with the text under it is
    // worse than a repaint nobody asked for; this only decides whether the
    // items are redrawn with it.
    if (bUpdate) {
        Invalidate();
    }
    return TRUE;
}

BOOL SEC_TREECLASS::DeleteColumn( int nCol ) {
    spdlog::debug("{} this={} nCol={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol);
    // Column zero is the item's own text and cannot go: the control owns it.
    if (nCol <= 0 || nCol >= static_cast<int>(m_columns.size())) {
        return FALSE;
    }
    m_columns.erase(m_columns.begin() + nCol);
    UpdateHeaderCtrl();
    Invalidate();
    return TRUE;
}

BOOL SEC_TREECLASS::DeleteColumn( const CString& strColumnHeading ) {
    spdlog::debug("{} this={} strColumnHeading={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strColumnHeading.GetString());
    for (size_t nCol = 0; nCol < m_columns.size(); ++nCol) {
        if (m_columns[nCol].strHeading == strColumnHeading) {
            return DeleteColumn(static_cast<int>(nCol));
        }
    }
    return FALSE;
}

UINT SEC_TREECLASS::GetColumnCount() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return static_cast<UINT>(m_columns.size());
}

BOOL SEC_TREECLASS::ModifyListCtrlStyle(DWORD dwRemove, DWORD dwAdd, BOOL bRedraw) {
    spdlog::debug("{} this={} dwRemove={} dwAdd={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemove, dwAdd, bRedraw);
    // These are the list half of the toolkit's styles, and they described the
    // column drawing it did itself. The header is the only piece of that which
    // exists here, so LVS_NOSORTHEADER is the only bit acted on; the rest is
    // kept so that what was set is what comes back.
    const DWORD dwStyle = ( m_dwListCtrlStyle & ~dwRemove ) | dwAdd;
    if (dwStyle == m_dwListCtrlStyle) {
        return FALSE;
    }
    m_dwListCtrlStyle = dwStyle;
    if (m_wndHeader.GetSafeHwnd() != nullptr) {
        const BOOL bNoSort = ( m_dwListCtrlStyle & LVS_NOSORTHEADER ) != 0;
        m_wndHeader.ModifyStyle( bNoSort ? HDS_BUTTONS : 0, bNoSort ? 0 : HDS_BUTTONS );
    }
    if (bRedraw) {
        Invalidate();
    }
    return TRUE;
}

BOOL SEC_TREECLASS::ModifyListCtrlStyleEx(DWORD dwRemoveEx, DWORD dwAddEx, BOOL bRedraw) {
    spdlog::debug("{} this={} dwRemoveEx={} dwAddEx={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemoveEx, dwAddEx, bRedraw);
    // As above: LVXS_HILIGHTSUBITEMS is the one DrawSubItems reads, and the
    // grid lines, word wrap and column fitting have nobody to act on them.
    const DWORD dwStyleEx = ( m_dwListCtrlStyleEx & ~dwRemoveEx ) | dwAddEx;
    if (dwStyleEx == m_dwListCtrlStyleEx) {
        return FALSE;
    }
    m_dwListCtrlStyleEx = dwStyleEx;
    if (bRedraw) {
        Invalidate();
    }
    return TRUE;
}

void SEC_TREECLASS::SetColumnHeading( int nCol, const CString& strHeading ) {
    spdlog::debug("{} this={} nCol={} strHeading={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol, strHeading.GetString());
    if (nCol >= 0 && nCol < static_cast<int>(m_columns.size())) {
        m_columns[nCol].strHeading = strHeading;
        UpdateHeaderCtrl();
    }
}

void SEC_TREECLASS::SetColumnFormat( int nCol, int fmt ) {
    spdlog::debug("{} this={} nCol={} fmt={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol, fmt);
    if (nCol >= 0 && nCol < static_cast<int>(m_columns.size())) {
        m_columns[nCol].nFormat = fmt;
        UpdateHeaderCtrl();
    }
}

int SEC_TREECLASS::GetColumnWidth( int nCol ) const {
    spdlog::debug("{} this={} nCol={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol);
    // Zero used to be the answer for every column, and it is read back for two
    // things that need a real number: the header widths the editor persists to
    // the profile, and PC_MainTreeControl sizing its in-place editor to
    // GetColumnWidth( 1 ) + 1, which made that editor one pixel wide.
    if (nCol < 0 || nCol >= static_cast<int>(m_columns.size())) {
        return 0;
    }
    return m_columns[nCol].nWidth;
}

void SEC_TREECLASS::SetColumnWidth( int nCol, int width ) {
    spdlog::debug("{} this={} nCol={} width={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol, width);
    if (nCol >= 0 && nCol < static_cast<int>(m_columns.size())) {
        m_columns[nCol].nWidth = width;
        UpdateHeaderCtrl();
        Invalidate();
    }
}

void SEC_TREECLASS::SetColumnImage( int nCol, int nImage ) {
    spdlog::debug("{} this={} nCol={} nImage={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol, nImage);
    if (nCol >= 0 && nCol < static_cast<int>(m_columns.size())) {
        m_columns[nCol].nImage = nImage;
        UpdateHeaderCtrl();
    }
}

void SEC_TREECLASS::PickTextColors(LvPaintContext* pPC) {
    spdlog::debug("{} this={} pPC={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pPC));
}

BOOL SEC_TREECLASS::SetBkColor(COLORREF rgbBk) {
    spdlog::debug("{} this={} rgbBk={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), rgbBk);
    TreeView_SetBkColor(GetSafeHwnd(), rgbBk);
    return TRUE;
}

// The colour behind an item's icon.
//
// The toolkit drew the tree and so could paint whatever it liked behind an
// icon. The common control's equivalent is the image list's own background: an
// image drawn from a list whose background is CLR_NONE is masked and shows
// whatever is under it, and one with a colour set is filled with that colour
// first. So this is the same idea reached through the image list.
//
// The one caller is CPCMainTreeControl::EnableEdit, which greys the whole
// property tree when editing is off -- SetBkColor, then these two with the same
// colour -- so that the icons do not sit on white squares in a grey tree.
BOOL SEC_TREECLASS::SetIconBkColor(COLORREF rgbIconBk) {
    spdlog::debug("{} this={} rgbIconBk={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), rgbIconBk);
    m_rgbIconBk = rgbIconBk;
    return ApplyIconBkColor();
}

BOOL SEC_TREECLASS::SetSelIconBkColor(COLORREF rgbSelIconBk) {
    spdlog::debug("{} this={} rgbSelIconBk={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), rgbSelIconBk);
    m_rgbSelIconBk = rgbSelIconBk;
    return ApplyIconBkColor();
}

// An image list has one background colour, not one for selected items and
// another for the rest, so the two can only both be honoured when they agree --
// which is the only way this editor sets them. When they differ the unselected
// one wins and the selected one is remembered but not drawn, because a selected
// row is already painted by the control and repainting the icon under it would
// mean drawing the icon again as well.
BOOL SEC_TREECLASS::ApplyIconBkColor() {
    const HIMAGELIST hImageList = TreeView_GetImageList(GetSafeHwnd(), TVSIL_NORMAL);
    if (hImageList == nullptr) {
        // Remembered for the image list that has not arrived yet: SetImageList
        // applies it when one does.
        return FALSE;
    }
    if (m_rgbSelIconBk != CLR_NONE && m_rgbSelIconBk != m_rgbIconBk) {
        spdlog::debug("SEC_TREECLASS::ApplyIconBkColor: selected and unselected icon backgrounds differ, "
                      "and an image list has only one; using the unselected one");
    }
    // Note this is the *image list's* colour, and the editor hands the same
    // list to several trees, so a tree that greys itself greys the icons in
    // its siblings too. The toolkit drew per tree and did not have that
    // problem; nothing in this editor greys one tree and not another.
    return ImageList_SetBkColor(hImageList, m_rgbIconBk) != CLR_NONE || m_rgbIconBk == CLR_NONE;
}

BOOL SEC_TREECLASS::DeselectAllItems(int iExclude) {
    spdlog::debug("{} this={} iExclude={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iExclude);
    // iExclude is an index into the flat item list the toolkit kept, which this
    // control does not have and this class does not build. Every caller in the
    // editor takes the default of -1, so it has never been asked for; say so if
    // one ever does rather than silently deselecting the item it wanted kept.
    if (iExclude >= 0) {
        spdlog::warn("SEC_TREECLASS::DeselectAllItems: iExclude={} ignored, items here have no index", iExclude);
    }
    m_selection.clear();
    m_hSelAnchor = nullptr;
    ApplySelection();
    m_bOwnSelection = true;
    const BOOL bResult = TreeView_SelectItem(GetSafeHwnd(), nullptr);
    m_bOwnSelection = false;
    return bResult;
}

UINT SEC_TREECLASS::GetSelectedCount() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    if (IsMultiSelect()) {
        return static_cast<UINT>(m_selection.size());
    }
    return TreeView_GetSelection(GetSafeHwnd()) != nullptr ? 1 : 0;
}

// Nothing to do, and that is the answer rather than an omission.
//
// The toolkit drew the tree and so owned its scroll bars, and had to be told
// when the contents changed. SysTreeView32 recalculates its own on every insert,
// delete and expand, so there is no work here and doing any would be fighting
// it. Counted as a stub by anything that looks for an empty body; it is an
// honest empty body.
void SEC_TREECLASS::RecalcScrollBars() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

CEdit* SEC_TREECLASS::GetEditControl() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return reinterpret_cast<CEdit*>(CWnd::FromHandle(TreeView_GetEditControl(GetSafeHwnd())));
}


// ---- multiple selection ----------------------------------------------------
//
// See trcore.h for why the set is kept here rather than asked of the control.
// Nothing below is traced: these run inside painting and mouse handling, and a
// line per item would bury the log.

bool SEC_TREECLASS::IsMultiSelect() const {
    return ( m_dwTreeCtrlStyleEx & TVXS_MULTISEL ) != 0;
}

bool SEC_TREECLASS::InSelection( HTREEITEM hItem ) const {
    for ( HTREEITEM hSelected : m_selection ) {
        if ( hSelected == hItem ) {
            return true;
        }
    }
    return false;
}

// Appends, so the order is whatever the caller added in. SortSelection puts it
// right, and everything that changes the set calls it before anyone reads.
void SEC_TREECLASS::SetInSelection( HTREEITEM hItem, bool bSelected ) {
    if ( hItem == nullptr ) {
        return;
    }
    for ( size_t i = 0; i < m_selection.size(); ++i ) {
        if ( m_selection[i] == hItem ) {
            if ( !bSelected ) {
                m_selection.erase( m_selection.begin() + i );
            }
            return;
        }
    }
    if ( bSelected ) {
        m_selection.push_back( hItem );
    }
}

// The order the tree lists its items in, which is the order it would display
// them in if everything were expanded. Collapsed children are walked too: an
// item can be selected and then have its parent collapsed, and it is still
// selected.
HTREEITEM SEC_TREECLASS::NextInDocumentOrder( HTREEITEM hItem ) const {
    const HWND hWnd = GetSafeHwnd();
    if ( hWnd == nullptr ) {
        return nullptr;
    }
    if ( hItem == nullptr ) {
        return TreeView_GetRoot( hWnd );
    }
    if ( const HTREEITEM hChild = TreeView_GetChild( hWnd, hItem ) ) {
        return hChild;
    }
    while ( hItem != nullptr ) {
        if ( const HTREEITEM hSibling = TreeView_GetNextSibling( hWnd, hItem ) ) {
            return hSibling;
        }
        hItem = TreeView_GetParent( hWnd, hItem );
    }
    return nullptr;
}

// One walk of the tree, keeping the selected items in the order it meets them.
// An item that has been deleted is not met, so this is also what drops a handle
// that no longer exists -- OnDeleteItem catches the ordinary case, and this
// catches anything that removed items without one, such as a tree rebuilt from
// under this class.
void SEC_TREECLASS::SortSelection() {
    if ( m_selection.empty() ) {
        return;
    }
    std::vector<HTREEITEM> ordered;
    ordered.reserve( m_selection.size() );
    for ( HTREEITEM hItem = NextInDocumentOrder( nullptr );
          hItem != nullptr && ordered.size() < m_selection.size();
          hItem = NextInDocumentOrder( hItem ) ) {
        if ( InSelection( hItem ) ) {
            ordered.push_back( hItem );
        }
    }
    m_selection.swap( ordered );
}

// Collected into a vector first rather than walked with a flag, because either
// end of the range may come first and a single item is both ends at once.
void SEC_TREECLASS::SelectRange( HTREEITEM hFirst, HTREEITEM hLast, bool bSelect ) {
    if ( hFirst == nullptr || hLast == nullptr ) {
        return;
    }
    std::vector<HTREEITEM> order;
    for ( HTREEITEM hItem = NextInDocumentOrder( nullptr ); hItem != nullptr;
          hItem = NextInDocumentOrder( hItem ) ) {
        order.push_back( hItem );
    }
    int nFrom = -1;
    int nTo = -1;
    for ( size_t i = 0; i < order.size(); ++i ) {
        if ( order[i] == hFirst ) {
            nFrom = static_cast<int>( i );
        }
        if ( order[i] == hLast ) {
            nTo = static_cast<int>( i );
        }
    }
    if ( nFrom < 0 || nTo < 0 ) {
        return;
    }
    if ( nFrom > nTo ) {
        const int nSwap = nFrom;
        nFrom = nTo;
        nTo = nSwap;
    }
    for ( int i = nFrom; i <= nTo; ++i ) {
        SetInSelection( order[i], bSelect );
    }
    SortSelection();
}

void SEC_TREECLASS::AddVisibleChildren( HTREEITEM hParent ) {
    const HWND hWnd = GetSafeHwnd();
    for ( HTREEITEM hChild = TreeView_GetChild( hWnd, hParent ); hChild != nullptr;
          hChild = TreeView_GetNextSibling( hWnd, hChild ) ) {
        SetInSelection( hChild, true );
        // Visible means displayed, so the walk stops at anything collapsed.
        if ( ( TreeView_GetItemState( hWnd, hChild, TVIS_EXPANDED ) & TVIS_EXPANDED ) != 0 ) {
            AddVisibleChildren( hChild );
        }
    }
}

// Make the control's TVIS_SELECTED bits say exactly what m_selection says.
//
// Setting the state through TVM_SETITEM does not move the control's caret and
// does not send a selection notification, which is what makes this safe to call
// from inside handling one.
void SEC_TREECLASS::ApplySelection() {
    const HWND hWnd = GetSafeHwnd();
    if ( hWnd == nullptr ) {
        return;
    }
    const bool bWasOwn = m_bOwnSelection;
    m_bOwnSelection = true;
    for ( HTREEITEM hItem : m_selectionDrawn ) {
        if ( !InSelection( hItem ) ) {
            TreeView_SetItemState( hWnd, hItem, 0, TVIS_SELECTED );
        }
    }
    // The control selects its own caret, so an item that has stopped being
    // selected while staying the caret is not in m_selectionDrawn and still
    // has to be cleared.
    if ( const HTREEITEM hCaret = TreeView_GetSelection( hWnd ) ) {
        if ( !InSelection( hCaret ) ) {
            TreeView_SetItemState( hWnd, hCaret, 0, TVIS_SELECTED );
        }
    }
    for ( HTREEITEM hItem : m_selection ) {
        TreeView_SetItemState( hWnd, hItem, TVIS_SELECTED, TVIS_SELECTED );
    }
    m_selectionDrawn = m_selection;
    m_bOwnSelection = bWasOwn;
}

void SEC_TREECLASS::MoveCaretIntoSelection() {
    const HWND hWnd = GetSafeHwnd();
    const HTREEITEM hCaret = TreeView_GetSelection( hWnd );
    if ( hCaret != nullptr && InSelection( hCaret ) ) {
        return;
    }
    // The nearest selected item, not the first: TreeView_SelectItem scrolls to
    // what it selects, and taking m_selection.front() would jump the tree back
    // to the top of the selection every time one item was Ctrl+clicked off.
    HTREEITEM hTarget = nullptr;
    if ( hCaret != nullptr ) {
        bool bPassedCaret = false;
        for ( HTREEITEM hItem = NextInDocumentOrder( nullptr ); hItem != nullptr;
              hItem = NextInDocumentOrder( hItem ) ) {
            if ( hItem == hCaret ) {
                bPassedCaret = true;
                if ( hTarget != nullptr ) {
                    break;      // the last selected item above it will do
                }
                continue;
            }
            if ( InSelection( hItem ) ) {
                hTarget = hItem;
                if ( bPassedCaret ) {
                    break;      // nothing above it, so the first one below
                }
            }
        }
    }
    if ( hTarget == nullptr && !m_selection.empty() ) {
        hTarget = m_selection.front();
    }
    // Null when nothing is selected, which clears the caret, and is what
    // DeselectAllItems does too. The editor still hears about it: this sends
    // TVN_SELCHANGED, and OnChildNotify passes it on.
    m_bOwnSelection = true;
    TreeView_SelectItem( hWnd, hTarget );
    m_bOwnSelection = false;
}

void SEC_TREECLASS::UpdateMultiSelectStyle() {
    if ( GetSafeHwnd() == nullptr ) {
        return;
    }
    if ( !IsMultiSelect() ) {
        if ( !m_selection.empty() ) {
            m_selection.clear();
            m_hSelAnchor = nullptr;
            ApplySelection();
        }
        return;
    }
    ModifyStyle( 0, TVS_SHOWSELALWAYS );
    // Whatever the control already had selected is the selection, so turning
    // the style on mid-run does not lose the current item.
    if ( m_selection.empty() ) {
        if ( const HTREEITEM hCaret = TreeView_GetSelection( GetSafeHwnd() ) ) {
            m_selection.assign( 1, hCaret );
            m_hSelAnchor = hCaret;
            m_selectionDrawn = m_selection;
        }
    }
}

// Where the modifier keys mean what they mean everywhere else: Ctrl adds one,
// Shift takes a run, and a plain click starts again.
//
// The control has the click first. It takes the focus, moves the caret, decides
// whether this is the start of a drag or a label edit, and tells the editor
// through TVN_SELCHANGED -- none of which should be reimplemented here. What it
// also does is select exactly one item, and everything after the call puts the
// real selection back over that.
void SEC_TREECLASS::OnLButtonDown( UINT nFlags, CPoint point ) {
    if ( !IsMultiSelect() ) {
        CWnd::OnLButtonDown( nFlags, point );
        return;
    }

    TVHITTESTINFO hitTest = { 0 };
    hitTest.pt = point;
    const HTREEITEM hItem = TreeView_HitTest( GetSafeHwnd(), &hitTest );
    // The expand button, the indent and the space past the last item belong to
    // the control and change no selection.
    if ( hItem == nullptr
         || ( hitTest.flags & ( TVHT_ONITEM | TVHT_ONITEMRIGHT ) ) == 0 ) {
        CWnd::OnLButtonDown( nFlags, point );
        return;
    }

    const bool bCtrl = ( nFlags & MK_CONTROL ) != 0;
    const bool bShift = ( nFlags & MK_SHIFT ) != 0;

    m_bOwnSelection = true;
    CWnd::OnLButtonDown( nFlags, point );
    m_bOwnSelection = false;

    if ( bShift && m_hSelAnchor != nullptr ) {
        // The anchor stays where it was, so dragging the shifted end back and
        // forth grows and shrinks one run rather than leaving a trail.
        m_selection.clear();
        SelectRange( m_hSelAnchor, hItem, true );
    } else if ( bCtrl ) {
        const bool bWasSelected = InSelection( hItem );
        SetInSelection( hItem, !bWasSelected );
        SortSelection();
        m_hSelAnchor = hItem;
        if ( bWasSelected ) {
            MoveCaretIntoSelection();
        }
    } else {
        m_selection.assign( 1, hItem );
        m_hSelAnchor = hItem;
    }
    ApplySelection();
}

// Where the two notifications this needs are caught, rather than in the message
// map, and the reason is not a preference.
//
// A reflected notification is dispatched through the *most derived* class's
// message map first, and both trees this matters for install their own:
// CPCMainTreeControl and CTreeGDBBrowserBase each map TVN_SELCHANGED with a
// plain ON_NOTIFY_REFLECT, which consumes it. An entry in this class's map
// would never have run for exactly the two trees that ask for TVXS_MULTISEL.
//
// OnChildNotify is the virtual that dispatch goes through, so this sees the
// notification whatever the derived map does with it afterwards. The set is
// updated before the base is called, so the editor's own handler -- which is
// what calls GetFirstSelectedItem -- runs with the selection already right.
BOOL SEC_TREECLASS::OnChildNotify( UINT message, WPARAM wParam, LPARAM lParam, LRESULT *pLResult ) {
    if ( message == WM_NOTIFY && lParam != 0 ) {
        const NMHDR *const pHdr = reinterpret_cast< NMHDR * >( lParam );
        // Both notifications come in an ANSI and a Unicode form, and the
        // control sends whichever suits the window it was created as.
        if ( pHdr->code == TVN_SELCHANGEDA || pHdr->code == TVN_SELCHANGEDW ) {
            OnTreeSelChanged( pHdr );
        } else if ( pHdr->code == TVN_DELETEITEMA || pHdr->code == TVN_DELETEITEMW ) {
            OnTreeDeleteItem( pHdr );
        }
    }
    return CWnd::OnChildNotify( message, wParam, lParam, pLResult );
}

// A caret move this class did not make: the keyboard, or the editor calling
// SelectItem. Shift extends from the anchor, which is what Shift with an arrow
// key means; anything else starts the selection again at the item the caret
// landed on, which is what a plain arrow key means.
void SEC_TREECLASS::OnTreeSelChanged( const NMHDR *pNMHDR ) {
    if ( !IsMultiSelect() || m_bOwnSelection ) {
        return;
    }
    const NMTREEVIEW *const pNotify = reinterpret_cast< const NMTREEVIEW * >( pNMHDR );
    const HTREEITEM hItem = pNotify->itemNew.hItem;
    if ( hItem == nullptr ) {
        return;
    }
    if ( ( ::GetKeyState( VK_SHIFT ) & 0x8000 ) != 0 && m_hSelAnchor != nullptr ) {
        m_selection.clear();
        SelectRange( m_hSelAnchor, hItem, true );
    } else {
        m_selection.assign( 1, hItem );
        m_hSelAnchor = hItem;
    }
    ApplySelection();
}

// The handle is about to stop existing and ApplySelection would write to it.
void SEC_TREECLASS::OnTreeDeleteItem( const NMHDR *pNMHDR ) {
    const NMTREEVIEW *const pNotify = reinterpret_cast< const NMTREEVIEW * >( pNMHDR );
    const HTREEITEM hItem = pNotify->itemOld.hItem;
    SetInSelection( hItem, false );
    for ( size_t i = 0; i < m_selectionDrawn.size(); ++i ) {
        if ( m_selectionDrawn[i] == hItem ) {
            m_selectionDrawn.erase( m_selectionDrawn.begin() + i );
            break;
        }
    }
    if ( hItem == m_hSelAnchor ) {
        m_hSelAnchor = nullptr;
    }
}


// ---- the columns on screen -------------------------------------------------
//
// Everything below exists because SysTreeView32 has one column. The headings
// are a header control laid over the top of the tree, and the text for the
// columns past the first is painted after the control has drawn each item. See
// trcore.h for what that costs against the toolkit's own drawing.

int SEC_TREECLASS::GetLayoutColumnWidth( int nCol ) const {
    if (nCol < 0 || nCol >= static_cast<int>(m_columns.size())) {
        return 0;
    }
    // InsertColumn's default width is -1, which meant "measure the contents" to
    // the toolkit. Nothing here measures, so such a column lays out as nothing
    // rather than as a negative width that would drag the ones after it left.
    return m_columns[nCol].nWidth > 0 ? m_columns[nCol].nWidth : 0;
}

int SEC_TREECLASS::GetColumnLeft( int nCol ) const {
    // The tree scrolls its contents horizontally in pixels, and the columns
    // have to scroll with them, so the origin is the scroll position negated.
    int nLeft = -GetScrollPos( SB_HORZ );
    for (int i = 0; i < nCol; ++i) {
        nLeft += GetLayoutColumnWidth( i );
    }
    return nLeft;
}

void SEC_TREECLASS::UpdateHeaderCtrl() {
    if (GetSafeHwnd() == nullptr) {
        // Called before the window exists -- PC_Dialog sets its columns up
        // around a SubclassTreeCtrlId, and ComboBox_GDBBrowser before the tab
        // is shown. The columns are already kept, so the header is built out of
        // them the first time this runs with a window to hang it on.
        return;
    }
    if (m_wndHeader.GetSafeHwnd() == nullptr) {
        if (!m_bHeaderEnabled) {
            return;
        }
        CWnd *const pParent = GetParent();
        if (pParent == nullptr) {
            return;
        }
        DWORD dwStyle = WS_CHILD | HDS_HORZ;
        if (( m_dwListCtrlStyle & LVS_NOSORTHEADER ) == 0) {
            dwStyle |= HDS_BUTTONS;
        }
        const CRect rectEmpty( 0, 0, 0, 0 );
        if (!m_wndHeader.Create( dwStyle, rectEmpty, pParent, ID_TREE_HEADER )) {
            spdlog::debug("{} this={} header creation failed", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
            return;
        }
        m_wndHeader.m_pTree = this;
        // The header would otherwise come up in the system font while the tree
        // is in whatever font its dialog gave it, which is visibly two
        // different controls rather than one.
        if (GetFont() != nullptr) {
            m_wndHeader.SetFont( GetFont() );
        }
        // The tree's own painting stops at the strip the header stands in,
        // rather than being drawn and then covered.
        ModifyStyle( 0, WS_CLIPSIBLINGS );
        if (m_pHeaderImageList != nullptr) {
            m_wndHeader.SetImageList( m_pHeaderImageList );
        }
        // How tall the strip has to be. Measured once: it depends on the
        // header's font and not on how wide it is laid out.
        CRect rectProbe( 0, 0, 100, 100 );
        WINDOWPOS wp = { 0 };
        HDLAYOUT layout;
        layout.prc = &rectProbe;
        layout.pwpos = &wp;
        m_nHeaderHeight = m_wndHeader.Layout( &layout ) ? wp.cy : 0;
    }
    SyncHeaderColumns();
    UpdateHeaderInset();
    LayoutHeader();
}

void SEC_TREECLASS::UpdateHeaderInset() {
    const bool bInset = m_bHeaderEnabled && m_wndHeader.GetSafeHwnd() != nullptr
        && m_nHeaderHeight > 0;
    if (bInset == m_bHeaderInset) {
        return;
    }
    m_bHeaderInset = bInset;
    // The frame is what OnNcCalcSize is asked about, and it is only asked when
    // something says the frame changed.
    SetWindowPos( nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED );
}

void SEC_TREECLASS::SetColumnWidthFromHeader( int nCol, int nWidth ) {
    if (m_bSyncingHeader || nCol < 0 || nCol >= static_cast<int>( m_columns.size() )) {
        return;
    }
    // The user dragged a divider. The width has to come back to m_columns
    // because that is what GetColumnWidth answers, and the editor writes that
    // into the profile as the layout to restore next run.
    m_columns[nCol].nWidth = nWidth;
    LayoutHeader();
    Invalidate();
}

void SEC_TREECLASS::SyncHeaderColumns() {
    if (m_wndHeader.GetSafeHwnd() == nullptr) {
        return;
    }
    // Rebuilt rather than reconciled: there are a handful of columns and this
    // runs only when one is added, removed or relabelled.
    m_bSyncingHeader = true;
    while (m_wndHeader.GetItemCount() > 0) {
        m_wndHeader.DeleteItem( 0 );
    }
    for (size_t nCol = 0; nCol < m_columns.size(); ++nCol) {
        CString strHeading = m_columns[nCol].strHeading;
        HDITEM item = { 0 };
        item.mask = HDI_TEXT | HDI_WIDTH | HDI_FORMAT;
        item.pszText = strHeading.GetBuffer( 0 );
        item.cchTextMax = strHeading.GetLength();
        item.cxy = GetLayoutColumnWidth( static_cast<int>( nCol ) );
        item.fmt = HDF_STRING | HeaderFormatFromColumnFormat( m_columns[nCol].nFormat );
        if (m_columns[nCol].nImage >= 0 && m_pHeaderImageList != nullptr) {
            item.mask |= HDI_IMAGE;
            item.iImage = m_columns[nCol].nImage;
            item.fmt |= HDF_IMAGE;
        }
        m_wndHeader.InsertItem( static_cast<int>( nCol ), &item );
        strHeading.ReleaseBuffer();
    }
    m_bSyncingHeader = false;
}

void SEC_TREECLASS::LayoutHeader() {
    if (m_wndHeader.GetSafeHwnd() == nullptr) {
        return;
    }
    CWnd *const pParent = GetParent();
    if (pParent == nullptr) {
        return;
    }
    if (!m_bHeaderInset || !IsWindowVisible()) {
        // A header on a tree nobody can see would be a stray strip of headings
        // in the dialog, since it is not the tree's child and does not go with
        // it.
        if (m_wndHeader.IsWindowVisible()) {
            m_wndHeader.ShowWindow( SW_HIDE );
        }
        return;
    }
    // The tree's client area in the parent's coordinates. Taken this way round
    // rather than from the window rect because the client rect already excludes
    // both the border and the strip, so there is no frame arithmetic to get
    // wrong.
    CRect rectClient;
    GetClientRect( &rectClient );
    ClientToScreen( &rectClient );
    pParent->ScreenToClient( &rectClient );

    const int nScroll = GetScrollPos( SB_HORZ );
    int nWidth = GetColumnLeft( static_cast<int>( m_columns.size() ) ) + nScroll;
    if (nWidth < rectClient.Width()) {
        nWidth = rectClient.Width();
    }
    // Laid out scrolled with the tree, and then clipped back to the tree's own
    // width: a sibling has no parent client area to be clipped by, so columns
    // scrolled off the left or running off the right would be drawn over the
    // dialog beside the tree.
    const CRect rectHeader( rectClient.left - nScroll, rectClient.top - m_nHeaderHeight,
        rectClient.left - nScroll + nWidth, rectClient.top );
    const CRect rectVisible( nScroll, 0, nScroll + rectClient.Width(), m_nHeaderHeight );

    // Called from the custom-draw pass, which runs inside the tree's own
    // painting, so it has to be silent when there is nothing to move.
    if (rectHeader != m_rectHeaderPlaced) {
        m_rectHeaderPlaced = rectHeader;
        m_wndHeader.SetWindowPos( &wndTop, rectHeader.left, rectHeader.top,
            rectHeader.Width(), rectHeader.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW );
    } else if (!m_wndHeader.IsWindowVisible()) {
        m_wndHeader.ShowWindow( SW_SHOWNOACTIVATE );
    }
    if (rectVisible != m_rectHeaderVisible) {
        m_rectHeaderVisible = rectVisible;
        CRgn rgnVisible;
        rgnVisible.CreateRectRgnIndirect( &rectVisible );
        // SetWindowRgn takes the region over, so it is detached rather than
        // deleted by CRgn going out of scope.
        m_wndHeader.SetWindowRgn( static_cast< HRGN >( rgnVisible.Detach() ), TRUE );
    }
}

void SEC_TREECLASS::OnNcCalcSize( BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR *lpncsp ) {
    // The base fills the rectangle in, by way of DefWindowProc, and this then
    // takes the strip out of what it filled in. That order is MFC's own, in
    // CMFCControlBarImpl and CMFCCaptionBar.
    CWnd::OnNcCalcSize( bCalcValidRects, lpncsp );
    if (lpncsp == nullptr) {
        return;
    }
    // Only rgrc[0], and that is not an accident. WM_NCCALCSIZE carries two
    // shapes: with wParam TRUE the lParam is an NCCALCSIZE_PARAMS, and with
    // wParam FALSE it is a single RECT. MFC's dispatcher casts to
    // NCCALCSIZE_PARAMS either way, and that is safe only because the struct
    // begins with RECT rgrc[3], so rgrc[0] is the same memory as the lone RECT
    // of the second shape. rgrc[1] and rgrc[2] would be a read off the end of
    // it, which is why bCalcValidRects has to be tested before touching those
    // and does not have to be for this.
    //
    // The strip the header stands in. Taken out of the client area rather than
    // laid over it, so the control's first item starts below the headings
    // instead of behind them.
    if (m_bHeaderInset && lpncsp->rgrc[0].top + m_nHeaderHeight < lpncsp->rgrc[0].bottom) {
        lpncsp->rgrc[0].top += m_nHeaderHeight;
    }
}

void SEC_TREECLASS::OnWindowPosChanged( WINDOWPOS *lpwndpos ) {
    CWnd::OnWindowPosChanged( lpwndpos );
    // Moved, resized, shown or hidden: the header is a sibling and follows none
    // of that on its own.
    LayoutHeader();
}

void SEC_TREECLASS::OnNcDestroy() {
    // The header belongs to the tree's parent, so it outlives the tree unless
    // it is taken down here.
    if (m_wndHeader.GetSafeHwnd() != nullptr) {
        m_wndHeader.DestroyWindow();
    }
    CWnd::OnNcDestroy();
}

void SEC_TREECLASS::DrawSubItems( CDC *pDC, HTREEITEM hItem ) {
    if (pDC == nullptr || hItem == nullptr || m_columns.size() < 2) {
        return;
    }
    CRect rectRow;
    // The whole line the item occupies, which is what gives the top and bottom
    // of every column. False here fails for an item that is not on screen,
    // which the control does not ask to be drawn anyway.
    if (!TreeView_GetItemRect( GetSafeHwnd(), hItem, &rectRow, FALSE )) {
        return;
    }
    CRect rectClient;
    GetClientRect( &rectClient );
    const std::map<HTREEITEM, std::vector<CString> >::const_iterator it = m_subItemText.find( hItem );

    COLORREF rgbBk = ColorOrDefault( TreeView_GetBkColor( GetSafeHwnd() ), COLOR_WINDOW );
    COLORREF rgbText = ColorOrDefault( TreeView_GetTextColor( GetSafeHwnd() ), COLOR_WINDOWTEXT );
    // LVXS_HILIGHTSUBITEMS is the toolkit's word for "a selected row is
    // selected all the way across", and PC_Dialog asks for it. Without it, and
    // without TVS_FULLROWSELECT, the control highlights the label alone and
    // these columns are drawn unselected to match -- painting them highlighted
    // would leave a gap of ordinary background between the label and column one.
    const bool bHilightSubItems = ( m_dwListCtrlStyleEx & LVXS_HILIGHTSUBITEMS ) != 0
        || ( GetStyle() & TVS_FULLROWSELECT ) != 0;
    if (bHilightSubItems
        && ( TreeView_GetItemState( GetSafeHwnd(), hItem, TVIS_SELECTED ) & TVIS_SELECTED ) != 0) {
        rgbBk = ::GetSysColor( COLOR_HIGHLIGHT );
        rgbText = ::GetSysColor( COLOR_HIGHLIGHTTEXT );
    }

    const int nOldBkMode = pDC->SetBkMode( TRANSPARENT );
    const COLORREF rgbOldText = pDC->SetTextColor( rgbText );
    // The font is the one the control selected to draw the item with, so it is
    // left alone: the columns are the same row and read as the same row.
    for (size_t nCol = 1; nCol < m_columns.size(); ++nCol) {
        const int nWidth = GetLayoutColumnWidth( static_cast<int>( nCol ) );
        if (nWidth <= 0) {
            continue;
        }
        CRect rectColumn( GetColumnLeft( static_cast<int>( nCol ) ), rectRow.top, 0, rectRow.bottom );
        rectColumn.right = rectColumn.left + nWidth;
        if (rectColumn.right <= rectClient.left || rectColumn.left >= rectClient.right) {
            continue;
        }
        // Filled before anything is written into it, and not only for the sake
        // of the background: the control drew the item's own label as wide as
        // its text, so a long name runs on past column zero, and this is what
        // clips it back.
        pDC->FillSolidRect( &rectColumn, rgbBk );
        // Read straight out of the store rather than through GetItemText: that
        // logs a line per call, and this runs for every column of every row of
        // every repaint.
        const CString strText = ( it != m_subItemText.end() && nCol < it->second.size() )
            ? it->second[nCol] : CString();
        if (strText.IsEmpty()) {
            continue;
        }
        CRect rectText( rectColumn );
        rectText.DeflateRect( 2, 0 );
        pDC->DrawText( strText, &rectText, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS
            | DT_NOPREFIX | DrawTextFormatFromColumnFormat( m_columns[nCol].nFormat ) );
    }
    pDC->SetTextColor( rgbOldText );
    pDC->SetBkMode( nOldBkMode );
}

void SEC_TREECLASS::OnCustomDraw( NMHDR *pNMHDR, LRESULT *pResult ) {
    const NMTVCUSTOMDRAW *const pDraw = reinterpret_cast<NMTVCUSTOMDRAW *>( pNMHDR );
    *pResult = CDRF_DODEFAULT;
    switch (pDraw->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        // Every repaint is also when the header's position is checked. The tree
        // scrolls sideways for reasons that never reach OnHScroll -- an
        // EnsureVisible, a keyboard move, an item widening the content -- and
        // this is the one place that sees all of them.
        LayoutHeader();
        if (m_columns.size() > 1) {
            *pResult = CDRF_NOTIFYITEMDRAW;
        }
        break;
    case CDDS_ITEMPREPAINT:
        // Let the control draw the item, then add the columns over the top of
        // what it drew.
        *pResult = CDRF_NOTIFYPOSTPAINT;
        break;
    case CDDS_ITEMPOSTPAINT:
        DrawSubItems( CDC::FromHandle( pDraw->nmcd.hdc ),
            reinterpret_cast<HTREEITEM>( pDraw->nmcd.dwItemSpec ) );
        break;
    default:
        break;
    }
}

void SEC_TREECLASS::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar *pScrollBar ) {
    // The control does the scrolling itself, in the default handler; this only
    // has to move the header afterwards so the headings stay over their columns.
    CWnd::OnHScroll( nSBCode, nPos, pScrollBar );
    LayoutHeader();
}
