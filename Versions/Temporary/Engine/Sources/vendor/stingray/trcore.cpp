#include "Toolkit/trcore.h"

#include <windowsx.h>
#include <boost/current_function.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include "logging.h"


SEC_TREECLASS::SEC_TREECLASS() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

int SEC_TREECLASS::GetActiveColumn() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

BOOL SEC_TREECLASS::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) {
    spdlog::trace("{} this={} dwStyle={} "
                  "rect.left={} rect.top={} rect.right={} rect.bottom={} "
                  "pParentWnd={} nID={} pContext={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this),
                  dwStyle, rect.left, rect.top, rect.right, rect.bottom,
                  spdlog::fmt_lib::ptr(pParentWnd), nID, spdlog::fmt_lib::ptr(pContext));
    LPCTSTR lpszClassName = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1), ::LoadIcon(nullptr, IDI_APPLICATION));
    return CWnd::Create(lpszClassName, "SEC_TREECLASS", dwStyle, rect, pParentWnd, nID, pContext);
}

BOOL SEC_TREECLASS::Create(DWORD dwStyle, DWORD dwStyleEx, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) {
    spdlog::trace("{} this={} dwStyle={} dwStyleEx={} "
                  "rect.left={} rect.top={} rect.right={} rect.bottom={} "
                  "pParentWnd={} nID={} pContext={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this),
                  dwStyle, dwStyleEx, rect.left, rect.top, rect.right, rect.bottom,
                  spdlog::fmt_lib::ptr(pParentWnd), nID, spdlog::fmt_lib::ptr(pContext));
    LPCTSTR lpszClassName = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1), ::LoadIcon(nullptr, IDI_APPLICATION));
    return CWnd::CreateEx(dwStyleEx, lpszClassName, "SEC_TREECLASS", dwStyle, rect, pParentWnd, nID, pContext);
}

BOOL SEC_TREECLASS::SubclassTreeCtrlId( UINT idc, CWnd *pWndDlg ) {
    spdlog::trace("{} this={} idc={} pWndDlg={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), idc, spdlog::fmt_lib::ptr(pWndDlg));
    if (!SubclassDlgItem(idc, pWndDlg)) {
        return FALSE;
    }
    return TRUE;
}

BOOL SEC_TREECLASS::GetItemRect( HTREEITEM hti, LPRECT lpRect, UINT nCode ) const {
    spdlog::trace("{} this={} hti={} lpRect={} nCode={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti), spdlog::fmt_lib::ptr(lpRect), nCode);
    return FALSE;
}

UINT SEC_TREECLASS::GetIndent() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

void SEC_TREECLASS::SetIndent(UINT nIndent) {
    spdlog::trace("{} this={} nIndent={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndent);
}

UINT SEC_TREECLASS::SetItemHeight(HTREEITEM hti, UINT cyItemHeight) const {
    spdlog::trace("{} this={} hti={} cyItemHeight={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti), cyItemHeight);
    return 0;
}

HTREEITEM SEC_TREECLASS::GetNextItem(HTREEITEM hItem, UINT nCode) const {
    spdlog::trace("{} this={} hItem={} cyItemHeight={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nCode);
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetChildItem(HTREEITEM hItem) const {
    spdlog::trace("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetNextSiblingItem(HTREEITEM hItem) const {
    spdlog::trace("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetPrevSiblingItem(HTREEITEM hItem) const {
    spdlog::trace("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetParentItem(HTREEITEM hItem) const {
    spdlog::trace("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetFirstVisibleItem() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetLastVisibleItem() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetNextVisibleItem(HTREEITEM hItem) const {
    spdlog::trace("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetPrevVisibleItem(HTREEITEM hItem) const {
    spdlog::trace("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetFirstSelectedItem() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetNextSelectedItem(HTREEITEM hItem) const {
    spdlog::trace("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetPrevSelectedItem(HTREEITEM hItem) const {
    spdlog::trace("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetCaretItem() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetSelectedItem() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetDropHilightItem() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetRootItem(HTREEITEM hti) const {
    spdlog::trace("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetNextItemInDisplayOrder(HTREEITEM hti) const {
    spdlog::trace("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetPrevItemInDisplayOrder(HTREEITEM hti) const {
    spdlog::trace("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return nullptr;
}

BOOL SEC_TREECLASS::SetItem(const LV_ITEM* pLVI, BOOL bRedraw) {
    spdlog::trace("{} this={} pLVI={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pLVI), bRedraw);
    return FALSE;
}

BOOL SEC_TREECLASS::SetItem(TV_ITEM* pItem) {
    spdlog::trace("{} this={} pItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pItem));
    return FALSE;
}

BOOL SEC_TREECLASS::SetItem(HTREEITEM hItem, UINT nMask, LPCTSTR lpszItem, int nImage, int nSelectedImage, UINT nState, UINT nStateMask, LPARAM lParam) {
    spdlog::trace("{} this={} hItem={} nMask={} lpszItem={} nImage={} nSelectedImage={} nState={} nStateMask={} lParam={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nMask, lpszItem, nImage, nSelectedImage, nState, nStateMask, lParam);
    return FALSE;
}

BOOL SEC_TREECLASS::SetItemEx(TV_ITEM* pTVI, TV_ITEM_EX* pTVIX) {
    spdlog::trace("{} this={} pTVI={} pTVIX={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pTVI), spdlog::fmt_lib::ptr(pTVIX));
    return FALSE;
}

BOOL SEC_TREECLASS::SetItemEx(LV_ITEM* pLVI, const LV_ITEM_EX* pLVIX) {
    spdlog::trace("{} this={} pTVI={} pTVIX={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pLVI), spdlog::fmt_lib::ptr(pLVIX));
    return FALSE;
}

BOOL SEC_TREECLASS::SetItemImage(HTREEITEM hItem, int nImage, int nSelectedImage) {
    spdlog::trace("{} this={} hItem={} nImage={} nSelectedImage={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nImage, nSelectedImage);
    return FALSE;
}

BOOL SEC_TREECLASS::SetItemState(HTREEITEM hItem, UINT nState, UINT nStateMask) {
    spdlog::trace("{} this={} hItem={} nState={} nStateMask={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nState, nStateMask);
    return FALSE;
}

BOOL SEC_TREECLASS::SetItemData(HTREEITEM hItem, SEC_DWORD dwData) {
    spdlog::trace("{} this={} hItem={} dwData={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), dwData);
    return FALSE;
}

BOOL SEC_TREECLASS::GetItem(TV_ITEM* pItem, BOOL bCopyText, BOOL bGetDispInfo) const {
    spdlog::trace("{} this={} pItem={} bCopyText={} bGetDispInfo={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pItem), bCopyText, bGetDispInfo);
    return FALSE;
}

BOOL SEC_TREECLASS::GetItem(LV_ITEM* pLVI, BOOL bCopyText, BOOL bGetDispInfo) const {
    spdlog::trace("{} this={} pLVI={} bCopyText={} bGetDispInfo={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pLVI), bCopyText, bGetDispInfo);
    return FALSE;
}

BOOL SEC_TREECLASS::GetItemEx(TV_ITEM* pTVI, TV_ITEM_EX* pTVIX, BOOL bGetDispInfoEx) const {
    spdlog::trace("{} this={} pTVI={} pTVIX={} bGetDispInfoEx={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pTVI), spdlog::fmt_lib::ptr(pTVIX), bGetDispInfoEx);
    return FALSE;
}

BOOL SEC_TREECLASS::GetItemEx(LV_ITEM* pLVI, LV_ITEM_EX* pLVIX, BOOL bGetDispInfoEx) const {
    spdlog::trace("{} this={} pLVI={} pLVIX={} bGetDispInfoEx={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pLVI), spdlog::fmt_lib::ptr(pLVIX), bGetDispInfoEx);
    return FALSE;
}

BOOL SEC_TREECLASS::GetItemImage(HTREEITEM hItem, int& nImage, int& nSelectedImage) const {
    spdlog::trace("{} this={} hItem={} nImage={} nSelectedImage={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nImage, nSelectedImage);
    return FALSE;
}

UINT SEC_TREECLASS::GetItemState(HTREEITEM hItem, UINT nStateMask) const {
    spdlog::trace("{} this={} hItem={} nStateMask={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nStateMask);
    return 0;
}

SEC_DWORD SEC_TREECLASS::GetItemData(HTREEITEM hItem) const {
    spdlog::trace("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return 0;
}

Node* SEC_TREECLASS::GetNode( HTREEITEM hti ) const {
    spdlog::trace("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return nullptr;
}

Node* SEC_TREECLASS::GetNode( Item* pItem ) const {
    spdlog::trace("{} this={} pItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pItem));
    return nullptr;
}

Node* SEC_TREECLASS::GetNodeAt( int nIndex ) const {
    spdlog::trace("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    return nullptr;
}

BOOL SEC_TREECLASS::SetNodeParent( SECTreeNode *pNode, SECTreeNode *pNodeParent, BOOL bInvalidate, HTREEITEM hInsertAfter ) {
    spdlog::trace("{} this={} pNode={} pNodeParent={} bInvalidate={} hInsertAfter={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pNode), spdlog::fmt_lib::ptr(pNodeParent), bInvalidate, spdlog::fmt_lib::ptr(hInsertAfter));
    return FALSE;
}

HTREEITEM SEC_TREECLASS::GetItemAt(int nIndex) const {
    spdlog::trace("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    return nullptr;
}

int SEC_TREECLASS::ItemIndex(HTREEITEM, int nStartFrom) const {
    spdlog::trace("{} this={} nStartFrom={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nStartFrom);
    return 0;
}

int SEC_TREECLASS::NodeIndex(Node *pNode, int nStartFrom) const {
    spdlog::trace("{} this={} pNode={} nStartFrom={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pNode), nStartFrom);
    return 0;
}

BOOL SEC_TREECLASS::IsExpanded(HTREEITEM hItem) const {
    spdlog::trace("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return FALSE;
}

BOOL SEC_TREECLASS::IsCallbackItem(int nIndex) const {
    spdlog::trace("{} this={} nIndex={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nIndex);
    return FALSE;
}

void SEC_TREECLASS::StoreSubItemText( BOOL bEnable ) {
    spdlog::trace("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
}

BOOL SEC_TREECLASS::IsStoringSubItemText() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

CString SEC_TREECLASS::GetItemText(HTREEITEM hItem, int iSubItem) const {
    spdlog::trace("{} this={} hItem={} iSubItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), iSubItem);
    return {};
}

BOOL SEC_TREECLASS::SetItemText(HTREEITEM hItem, LPCTSTR lpszItem) {
    spdlog::trace("{} this={} hItem={} lpszItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), lpszItem);
    return FALSE;
}

BOOL SEC_TREECLASS::SetItemText(HTREEITEM hItem, int nSubItem, LPCTSTR lpszItem) {
    spdlog::trace("{} this={} hItem={} nSubItem={} lpszItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nSubItem, lpszItem);
    return FALSE;
}

BOOL SEC_TREECLASS::SetItemString(HTREEITEM hti, int nSubItem, const CString& strItem) {
    spdlog::trace("{} this={} hti={} nSubItem={} strItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti), nSubItem, strItem.GetString());
    return FALSE;
}

BOOL SEC_TREECLASS::GetItemString(HTREEITEM hti, int nSubItem, CString& strItem) {
    spdlog::trace("{} this={} hti={} nSubItem={} strItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti), nSubItem, strItem.GetString());
    return FALSE;
}

BOOL SEC_TREECLASS::SetNoEllipsis(BOOL bNoEllipsis) {
    spdlog::trace("{} this={} bNoEllipsis={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bNoEllipsis);
    return FALSE;
}

BOOL SEC_TREECLASS::InsertBatch(TV_ITEM** ppItems, HTREEITEM hParent, int cItems, BOOL bInvalidate) {
    spdlog::trace("{} this={} ppItems={} hParent={} cItems={} bInvalidate={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(ppItems), spdlog::fmt_lib::ptr(hParent), cItems, bInvalidate);
    return FALSE;
}

HTREEITEM SEC_TREECLASS::InsertItem(LPTV_INSERTSTRUCT lpInsertStruct) {
    spdlog::trace("{} this={} lpInsertStruct={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(lpInsertStruct));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::InsertItem(UINT nMask, LPCTSTR lpszItem, int nImage, int nSelectedImage, UINT nState, UINT nStateMask, LPARAM lParam, HTREEITEM hParent, HTREEITEM hInsertAfter) {
    spdlog::trace("{} this={} nMask={} lpszItem={} nImage={} nSelectedImage={} nState={} nStateMask={} lParam={} hParent={} hInsertAfter={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nMask, lpszItem, nImage, nSelectedImage, nState, nStateMask, lParam, spdlog::fmt_lib::ptr(hParent), spdlog::fmt_lib::ptr(hInsertAfter));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::InsertItem(LPCTSTR lpszItem, HTREEITEM hParent, HTREEITEM hInsertAfter) {
    spdlog::trace("{} this={} lpszItem={} hParent={} hInsertAfter={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszItem, spdlog::fmt_lib::ptr(hParent), spdlog::fmt_lib::ptr(hInsertAfter));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::InsertItem(LPCTSTR lpszItem, int nImage, int nSelectedImage, HTREEITEM hParent, HTREEITEM hInsertAfter) {
    spdlog::trace("{} this={} lpszItem={} nImage={} nSelectedImage={} hParent={} hInsertAfter={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), lpszItem, nImage, nSelectedImage, spdlog::fmt_lib::ptr(hParent), spdlog::fmt_lib::ptr(hInsertAfter));
    return nullptr;
}

BOOL SEC_TREECLASS::DeleteItem(HTREEITEM hItem) {
    spdlog::trace("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return FALSE;
}

BOOL SEC_TREECLASS::DeleteAllItems() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SEC_TREECLASS::Expand(HTREEITEM hItem, UINT nCode, BOOL bRedraw, BOOL bForceExpand) {
    spdlog::trace("{} this={} hItem={} nCode={} bRedraw={} bForceExpand={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nCode, bRedraw, bForceExpand);
    return FALSE;
}

void SEC_TREECLASS::ExpandCompletely(HTREEITEM hItem, BOOL bRedraw) {
    spdlog::trace("{} this={} hItem={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), bRedraw);
}

void SEC_TREECLASS::CollapseCompletely(HTREEITEM hItem, BOOL bRedraw) {
    spdlog::trace("{} this={} hItem={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), bRedraw);
}

BOOL SEC_TREECLASS::Select(HTREEITEM hItem, UINT nCode) {
    spdlog::trace("{} this={} hItem={} nCode={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nCode);
    return FALSE;
}

BOOL SEC_TREECLASS::SelectItem(HTREEITEM hItem) {
    spdlog::trace("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return FALSE;
}

BOOL SEC_TREECLASS::SelectDropTarget(HTREEITEM hItem, BOOL bAutoScroll) {
    spdlog::trace("{} this={} hItem={} bAutoScroll={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), bAutoScroll);
    return FALSE;
}

BOOL SEC_TREECLASS::SelectSetFirstVisible(HTREEITEM hItem) {
    spdlog::trace("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return FALSE;
}

BOOL SEC_TREECLASS::SetFirstVisible(HTREEITEM hti) {
    spdlog::trace("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return FALSE;
}

HTREEITEM SEC_TREECLASS::HitTest(CPoint pt, UINT* pFlags) {
    spdlog::trace("{} this={} pt.x={} pt.y={} pFlags={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), pt.x, pt.y, spdlog::fmt_lib::ptr(pFlags));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::HitTest(TV_HITTESTINFO* pHitTestInfo) {
    spdlog::trace("{} this={} pHitTestInfo={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pHitTestInfo));
    return nullptr;
}

CImageList* SEC_TREECLASS::CreateDragImage(HTREEITEM hItem) {
    spdlog::trace("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return nullptr;
}

CImageList* SEC_TREECLASS::CreateDragImageEx(HTREEITEM hItem, CPoint& ptOffset) {
    spdlog::trace("{} this={} hItem={} ptOffset.x={} ptOffset.y={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), ptOffset.x, ptOffset.y);
    return nullptr;
}

BOOL SEC_TREECLASS::EnsureVisible(HTREEITEM hItem, BOOL bParentVisible) {
    spdlog::trace("{} this={} hItem={} bParentVisible={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), bParentVisible);
    return FALSE;
}

int SEC_TREECLASS::CompareItem(Node *pNode1, Node *pNode2) {
    spdlog::trace("{} this={} pNode1={} pNode2={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pNode1), spdlog::fmt_lib::ptr(pNode2));
    return 0;
}

BOOL SEC_TREECLASS::SortChildren(HTREEITEM hItem, BOOL bRecursive, BOOL bAscending) {
    spdlog::trace("{} this={} hItem={} bRecursive={} bAscending={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), bRecursive, bAscending);
    return FALSE;
}

BOOL SEC_TREECLASS::SortChildrenCB(LPTV_SORTCB pSort, BOOL bRecursive, BOOL bAscending) {
    spdlog::trace("{} this={} pSort={} bRecursive={} bAscending={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pSort), bRecursive, bAscending);
    return FALSE;
}

CEdit* SEC_TREECLASS::EditLabel(HTREEITEM hItem, int nCol) {
    spdlog::trace("{} this={} hItem={} nCol={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem), nCol);
    return nullptr;
}

UINT SEC_TREECLASS::GetChildCount(HTREEITEM hti, BOOL bRecursive, BOOL bExpandedOnly) {
    spdlog::trace("{} this={} hti={} bRecursive={} bExpandedOnly={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti), bRecursive, bExpandedOnly);
    return 0;
}

BOOL SEC_TREECLASS::SelectAllVisibleChildren(HTREEITEM hti) {
    spdlog::trace("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return FALSE;
}

BOOL SEC_TREECLASS::SelectItemRange( HTREEITEM htiFirst, HTREEITEM htiLast, BOOL bSelect) {
    spdlog::trace("{} this={} htiFirst={} htiLast={} bSelect={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(htiFirst), spdlog::fmt_lib::ptr(htiLast), bSelect);
    return FALSE;
}

BOOL SEC_TREECLASS::IsSelected(HTREEITEM hti) const {
    spdlog::trace("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return FALSE;
}

BOOL SEC_TREECLASS::HideItem( HTREEITEM hti, BOOL bHide ) {
    spdlog::trace("{} this={} hti={} bHide={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti), bHide);
    return FALSE;
}

BOOL SEC_TREECLASS::IsHidden( HTREEITEM hti ) const {
    spdlog::trace("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return FALSE;
}

HTREEITEM SEC_TREECLASS::GetFirstHiddenItem() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetNextHiddenItem(HTREEITEM hti) const {
    spdlog::trace("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return nullptr;
}

BOOL SEC_TREECLASS::UnHideAllItems() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

BOOL SEC_TREECLASS::DisableAllItems( BOOL bDisable ) {
    spdlog::trace("{} this={} bDisable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bDisable);
    return FALSE;
}


BOOL SEC_TREECLASS::DisableItem( HTREEITEM hti, BOOL bDisable ) {
    spdlog::trace("{} this={} hti={} bDisable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti), bDisable);
    return FALSE;
}

BOOL SEC_TREECLASS::IsDisabled( HTREEITEM hti) const {
    spdlog::trace("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return FALSE;
}

HTREEITEM SEC_TREECLASS::GetFirstDisabledItem() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}

HTREEITEM SEC_TREECLASS::GetNextDisabledItem(HTREEITEM hti) const {
    spdlog::trace("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return nullptr;
}

void SEC_TREECLASS::HideDisabledItems(BOOL bHide) {
    spdlog::trace("{} this={} bHide={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bHide);
}

BOOL SEC_TREECLASS::IsHideDisabledItems() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return FALSE;
}

UINT SEC_TREECLASS::GetCount() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

BOOL SEC_TREECLASS::ItemHasChildrenOnDemand(HTREEITEM hItem) const {
    spdlog::trace("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return FALSE;
}

BOOL SEC_TREECLASS::ItemHasChildren(HTREEITEM hItem) const {
    spdlog::trace("{} this={} hItem={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hItem));
    return FALSE;
}

BOOL SEC_TREECLASS::ItemExists(HTREEITEM hti) const {
    spdlog::trace("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return FALSE;
}

void SEC_TREECLASS::EnableScrollOnExpand(BOOL bEnable) {
    spdlog::trace("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
}

CImageList* SEC_TREECLASS::GetImageList(UINT nImageList) const {
    spdlog::trace("{} this={} nImageList={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nImageList);
    return nullptr;
}

CImageList* SEC_TREECLASS::SetImageList(CImageList* pImageList, int nImageListType) {
    spdlog::trace("{} this={} pImageList={} nImageListType={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pImageList), nImageListType);
    return nullptr;
}

void SEC_TREECLASS::EnableHeaderCtrl(BOOL bEnable, BOOL bSortHeader) {
    spdlog::trace("{} this={} bEnable={} bSortHeader={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable, bSortHeader);
}

void SEC_TREECLASS::EnableWordWrap(BOOL bEnable) {
    spdlog::trace("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
}

void SEC_TREECLASS::EnableToolTips(BOOL bEnable) {
    spdlog::trace("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
}

void SEC_TREECLASS::EnableMultiSelect(BOOL bEnable) {
    spdlog::trace("{} this={} bEnable={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), bEnable);
}

BOOL SEC_TREECLASS::GetTreeCtrlStyles(DWORD& dwStyle, DWORD& dwStyleEx) const {
    spdlog::trace("{} this={} dwStyle={} dwStyleEx={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwStyle, dwStyleEx);
    return FALSE;
}

BOOL SEC_TREECLASS::SetTreeCtrlStyles(DWORD dwStyle, DWORD dwStyleEx, BOOL bRedraw) {
    spdlog::trace("{} this={} dwStyle={} dwStyleEx={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwStyle, dwStyleEx, bRedraw);
    return FALSE;
}

DWORD SEC_TREECLASS::GetTreeCtrlStyle() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

DWORD SEC_TREECLASS::GetTreeCtrlStyleEx() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

BOOL SEC_TREECLASS::SetTreeCtrlStyle(DWORD dwStyle, BOOL bRedraw) {
    spdlog::trace("{} this={} dwStyle={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwStyle, bRedraw);
    return FALSE;
}

BOOL SEC_TREECLASS::SetTreeCtrlStyleEx(DWORD dwStyleEx, BOOL bRedraw) {
    spdlog::trace("{} this={} dwStyle={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwStyleEx, bRedraw);
    return FALSE;
}

BOOL SEC_TREECLASS::ModifyTreeCtrlStyle(DWORD dwRemove, DWORD dwAdd, BOOL bRedraw) {
    spdlog::trace("{} this={} dwRemove={} dwAdd={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemove, dwAdd, bRedraw);
    return FALSE;
}

BOOL SEC_TREECLASS::ModifyTreeCtrlStyleEx(DWORD dwRemoveEx, DWORD dwAddEx, BOOL bRedraw) {
    spdlog::trace("{} this={} dwRemoveEx={} dwAddEx={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemoveEx, dwAddEx, bRedraw);
    return FALSE;
}

BOOL SEC_TREECLASS::ModifyTreeCtrlStyles(DWORD dwRemove, DWORD dwAdd, DWORD dwRemoveEx, DWORD dwAddEx, BOOL bRedraw) {
    spdlog::trace("{} this={} dwRemove={} dwAdd={} dwRemoveEx={} dwAddEx={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemove, dwAdd, dwRemoveEx, dwAddEx, bRedraw);
    return FALSE;
}

void SEC_TREECLASS::SetFilterLevel(WORD wLevel) {
    spdlog::trace("{} this={} wLevel={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), wLevel);
}

WORD SEC_TREECLASS::GetFilterLevel() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

BOOL SEC_TREECLASS::Update( HTREEITEM hti, BOOL bLabelOnly, BOOL bEraseBkgnd, BOOL bUpdateBelow, BOOL bUpdateNow ) {
    spdlog::trace("{} this={} hti={} bLabelOnly={} bEraseBkgnd={} bUpdateBelow={} bUpdateNow={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti), bLabelOnly, bEraseBkgnd, bUpdateBelow, bUpdateNow);
    return FALSE;
}

inline BOOL SEC_TREECLASS::InvalidateItem(HTREEITEM hti) {
    spdlog::trace("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
    return FALSE;
}

void SEC_TREECLASS::ReMeasureItem( HTREEITEM hti ) {
    spdlog::trace("{} this={} hti={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(hti));
}

void SEC_TREECLASS::SetAutoExpandDelay( UINT nDelay ) {
    spdlog::trace("{} this={} nDelay={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nDelay);
}

void SEC_TREECLASS::SetMaxAnimations( int nMaxAnimations ) {
    spdlog::trace("{} this={} nMaxAnimations={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nMaxAnimations);
}

BOOL SEC_TREECLASS::InsertColumn( int nCol, const CString& strHeader, int nFormat, int wWidth, int iSubItem, int iImage, BOOL bUpdate ) {
    spdlog::trace("{} this={} nCol={} strHeader={} nFormat={} nWidth={} iSubItem={} iImage={} bUpdate={}",
        BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol, strHeader.GetString(), nFormat, wWidth, iSubItem, iImage, bUpdate);
    return FALSE;
}

BOOL SEC_TREECLASS::DeleteColumn( int nCol ) {
    spdlog::trace("{} this={} nCol={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol);
    return FALSE;
}

BOOL SEC_TREECLASS::DeleteColumn( const CString& strColumnHeading ) {
    spdlog::trace("{} this={} strColumnHeading={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), strColumnHeading.GetString());
    return FALSE;
}

UINT SEC_TREECLASS::GetColumnCount() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

BOOL SEC_TREECLASS::ModifyListCtrlStyle(DWORD dwRemove, DWORD dwAdd, BOOL bRedraw) {
    spdlog::trace("{} this={} dwRemove={} dwAdd={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemove, dwAdd, bRedraw);
    return FALSE;
}

BOOL SEC_TREECLASS::ModifyListCtrlStyleEx(DWORD dwRemoveEx, DWORD dwAddEx, BOOL bRedraw) {
    spdlog::trace("{} this={} dwRemoveEx={} dwAddEx={} bRedraw={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), dwRemoveEx, dwAddEx, bRedraw);
    return FALSE;
}

void SEC_TREECLASS::SetColumnHeading( int nCol, const CString& strHeading ) {
    spdlog::trace("{} this={} nCol={} strHeading={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol, strHeading.GetString());
}

void SEC_TREECLASS::SetColumnFormat( int nCol, int fmt ) {
    spdlog::trace("{} this={} nCol={} fmt={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol, fmt);
}

int SEC_TREECLASS::GetColumnWidth( int nCol ) const {
    spdlog::trace("{} this={} nCol={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol);
    return 0;
}

void SEC_TREECLASS::SetColumnWidth( int nCol, int width ) {
    spdlog::trace("{} this={} nCol={} width={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol, width);
}

void SEC_TREECLASS::SetColumnImage( int nCol, int nImage ) {
    spdlog::trace("{} this={} nCol={} nImage={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), nCol, nImage);
}

void SEC_TREECLASS::PickTextColors(LvPaintContext* pPC) {
    spdlog::trace("{} this={} pPC={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), spdlog::fmt_lib::ptr(pPC));
}

BOOL SEC_TREECLASS::SetBkColor(COLORREF rgbBk) {
    spdlog::trace("{} this={} rgbBk={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), rgbBk);
    return FALSE;
}

BOOL SEC_TREECLASS::SetIconBkColor(COLORREF rgbIconBk) {
    spdlog::trace("{} this={} rgbIconBk={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), rgbIconBk);
    return FALSE;
}

BOOL SEC_TREECLASS::SetSelIconBkColor(COLORREF rgbSelIconBk) {
    spdlog::trace("{} this={} rgbSelIconBk={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), rgbSelIconBk);
    return FALSE;
}

BOOL SEC_TREECLASS::DeselectAllItems(int iExclude) {
    spdlog::trace("{} this={} iExclude={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this), iExclude);
    return FALSE;
}

UINT SEC_TREECLASS::GetSelectedCount() const {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return 0;
}

void SEC_TREECLASS::RecalcScrollBars() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
}

CEdit* SEC_TREECLASS::GetEditControl() {
    spdlog::trace("{} this={}", BOOST_CURRENT_FUNCTION, spdlog::fmt_lib::ptr(this));
    return nullptr;
}
