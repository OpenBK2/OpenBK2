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

}  // namespace



SEC_TREECLASS::SEC_TREECLASS() : m_dwTreeCtrlStyleEx(0), m_bStoreSubItemText(FALSE) {
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
    return TreeView_GetItemRect(GetSafeHwnd(), hti, lpRect, nCode != 0);
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

HTREEITEM SEC_TREECLASS::GetFirstSelectedItem() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    // The common control selects one item at a time, so the first selected item
    // is the selected item and there is never a next one. The editor asks for
    // multiple selection (TVXS_MULTISEL) and gets one item instead; that is the
    // largest thing this control cannot do, and it is honest about it here
    // rather than returning items that are not selected.
    return TreeView_GetSelection(GetSafeHwnd());
}

HTREEITEM SEC_TREECLASS::GetNextSelectedItem(HTREEITEM hItem) const {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    // See GetFirstSelectedItem: one selection, so the walk ends immediately.
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetPrevSelectedItem(HTREEITEM hItem) const {
    spdlog::debug("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    // See GetFirstSelectedItem.
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetCaretItem() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return TreeView_GetSelection(GetSafeHwnd());
}

HTREEITEM SEC_TREECLASS::GetSelectedItem() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return TreeView_GetSelection(GetSafeHwnd());
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
    return FALSE;
}

BOOL SEC_TREECLASS::SelectItemRange( HTREEITEM htiFirst, HTREEITEM htiLast, BOOL bSelect) {
    spdlog::debug("{} this={} htiFirst={} htiLast={} bSelect={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(htiFirst), spdlog::fmt_lib::ptr(htiLast), bSelect);
    return FALSE;
}

BOOL SEC_TREECLASS::IsSelected(HTREEITEM hti) const {
    spdlog::debug("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return hti != nullptr && TreeView_GetSelection(GetSafeHwnd()) == hti;
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
    // The control has a normal and a state list; a header list, which is what
    // LVSIL_HEADER asks for, belongs to the columns and is dropped.
    const HIMAGELIST hOldImageList = TreeView_SetImageList(GetSafeHwnd(),
        pImageList != nullptr ? pImageList->GetSafeHandle() : nullptr, nImageListType);
    return CImageList::FromHandle(hOldImageList);
}

void SEC_TREECLASS::EnableHeaderCtrl(BOOL bEnable, BOOL bSortHeader) {
    spdlog::debug("{} this={} bEnable={} bSortHeader={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable, bSortHeader);
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
    return TRUE;
}

BOOL SEC_TREECLASS::ModifyTreeCtrlStyle(DWORD dwRemove, DWORD dwAdd, BOOL bRedraw) {
    spdlog::debug("{} this={} dwRemove={} dwAdd={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemove, dwAdd, bRedraw);
    return ModifyStyle(dwRemove, dwAdd, bRedraw ? SWP_FRAMECHANGED : 0);
}

BOOL SEC_TREECLASS::ModifyTreeCtrlStyleEx(DWORD dwRemoveEx, DWORD dwAddEx, BOOL bRedraw) {
    spdlog::debug("{} this={} dwRemoveEx={} dwAddEx={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemoveEx, dwAddEx, bRedraw);
    m_dwTreeCtrlStyleEx = (m_dwTreeCtrlStyleEx & ~dwRemoveEx) | dwAddEx;
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
    if (!GetItemRect(hti, &rectItem, FALSE)) {
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
    // bUpdate asked the toolkit to repaint. Nothing paints these yet, so there
    // is nothing to repaint; when the custom-draw pass exists it belongs here.
    return TRUE;
}

BOOL SEC_TREECLASS::DeleteColumn( int nCol ) {
    spdlog::debug("{} this={} nCol={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol);
    // Column zero is the item's own text and cannot go: the control owns it.
    if (nCol <= 0 || nCol >= static_cast<int>(m_columns.size())) {
        return FALSE;
    }
    m_columns.erase(m_columns.begin() + nCol);
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
    return FALSE;
}

BOOL SEC_TREECLASS::ModifyListCtrlStyleEx(DWORD dwRemoveEx, DWORD dwAddEx, BOOL bRedraw) {
    spdlog::debug("{} this={} dwRemoveEx={} dwAddEx={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemoveEx, dwAddEx, bRedraw);
    return FALSE;
}

void SEC_TREECLASS::SetColumnHeading( int nCol, const CString& strHeading ) {
    spdlog::debug("{} this={} nCol={} strHeading={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol, strHeading.GetString());
    if (nCol >= 0 && nCol < static_cast<int>(m_columns.size())) {
        m_columns[nCol].strHeading = strHeading;
    }
}

void SEC_TREECLASS::SetColumnFormat( int nCol, int fmt ) {
    spdlog::debug("{} this={} nCol={} fmt={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol, fmt);
    if (nCol >= 0 && nCol < static_cast<int>(m_columns.size())) {
        m_columns[nCol].nFormat = fmt;
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
    }
}

void SEC_TREECLASS::SetColumnImage( int nCol, int nImage ) {
    spdlog::debug("{} this={} nCol={} nImage={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol, nImage);
    if (nCol >= 0 && nCol < static_cast<int>(m_columns.size())) {
        m_columns[nCol].nImage = nImage;
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

BOOL SEC_TREECLASS::SetIconBkColor(COLORREF rgbIconBk) {
    spdlog::debug("{} this={} rgbIconBk={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), rgbIconBk);
    return FALSE;
}

BOOL SEC_TREECLASS::SetSelIconBkColor(COLORREF rgbSelIconBk) {
    spdlog::debug("{} this={} rgbSelIconBk={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), rgbSelIconBk);
    return FALSE;
}

BOOL SEC_TREECLASS::DeselectAllItems(int iExclude) {
    spdlog::debug("{} this={} iExclude={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iExclude);
    // One selection, so clearing it is all there is to deselect and iExclude
    // has nothing to exclude. See GetFirstSelectedItem.
    return TreeView_SelectItem(GetSafeHwnd(), nullptr);
}

UINT SEC_TREECLASS::GetSelectedCount() const {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return TreeView_GetSelection(GetSafeHwnd()) != nullptr ? 1 : 0;
}

void SEC_TREECLASS::RecalcScrollBars() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

CEdit* SEC_TREECLASS::GetEditControl() {
    spdlog::debug("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return reinterpret_cast<CEdit*>(CWnd::FromHandle(TreeView_GetEditControl(GetSafeHwnd())));
}
