#pragma once

#include <Windows.h>
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

#include <map>
#include <vector>

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__settreectrlstyleex.htm?Highlight=TVXS_MULTISEL

// https://help.perforce.com/stingray/2024.1/Stingray_Studio_HTML_User_Guide/Content/otugTOC/Tree_Control_Tree_View_S.htm

enum {
    // Prevents the tree view control from sending TVN_DRAGDROP notification messages.
#ifndef TVS_DISABLEDRAGDROP
    TVS_DISABLEDRAGDROP,
#endif
    // Allows the user to edit the labels of tree view items.
#ifndef TVS_EDITLABELS
    TVS_EDITLABELS,
#endif
    // Displays plus (+) and minus (-) buttons next to parent items. The user clicks the buttons to expand or collapse a parent item's list of child items. To include buttons with items at the root of the tree view, you need to specify TVS_LINESATROOT.
#ifndef TVS_HASBUTTONS
    TVS_HASBUTTONS,
#endif
    // Uses lines to show the hierarchy of items.
#ifndef TVS_HASLINES
    TVS_HASLINES,
#endif
    // Uses lines to link items at the root of the tree view control. This value is ignored if TVS_HASLINES is not also specified.
#ifndef TVS_LINESATROOT
    TVS_LINESATROOT,
#endif
    // Causes a selected item to remain selected when the tree view control loses focus.
#ifndef TVS_SHOWSELALWAYS
    TVS_SHOWSELALWAYS,
#endif
    // The toolkit's own extended styles share one word with Windows' WS_EX_ ones:
    // callers pass WS_EX_CLIENTEDGE | TVXS_MULTISEL | ... as a single value. So
    // these are given the high half of the word and WS_EX_ keeps the low half,
    // which is where every WS_EX_ bit the editor asks for lives. SEC_TREECLASS
    // splits the word on that boundary.
    //
    // Displays the column header. This style removes the LVS_NOCOLUMNHEADER style so that all the columns display headers.
    TVXS_COLUMNHEADER = 0x00010000,
    // Enable tooltips.
    TVXS_FLYBYTOOLTIPS = 0x00020000,
    // Enables the user to select multiple items.
    TVXS_MULTISEL = 0x00040000,
    // Enables word wrapping of text if the first column is narrow. Specifying this style automatically enables the LVXS_WORDWRAP style, affecting all columns.
    TVXS_WORDWRAP = 0x00080000,
    TVXS_ANIMATE = 0x00100000,
    // Disables multiple selection of items.
#ifndef LVS_SINGLESEL
    LVS_SINGLESEL,
#endif
    // The item column fills the width not occupied by subitem columns.
    LVXS_FITCOLUMNSONSIZE = 0x00200000,
    // Enable tooltips for additional columns. The TVXS_FLYBYTOOLTIPS style automatically enables this style.
    LVXS_FLYBYTOOLTIPS = 0x00400000,
    LVXS_HILIGHTSUBITEMS = 0x00800000,
    // Paints vertical lines between columns.
    LVXS_LINESBETWEENCOLUMNS = 0x01000000,
    // Paints horizontal lines between items.
    LVXS_LINESBETWEENITEMS = 0x02000000,
    // Prevents automatic resizing of column 0 when a column is deleted.
    LVXS_NOGROWCOLUMNONDELETE = 0x04000000,
    // Specifies that additional columns do not display column headers. This style is automatically removed by specifying the TVXS_COLUMNHEADER style, which causes all columns to display headers.
#ifndef LVS_NOCOLUMNHEADER
    LVS_NOCOLUMNHEADER,
#endif
    // Enables word wrapping of item text if the column is narrow. The TVXS_WORDWRAP style automatically enables this style.
    LVXS_WORDWRAP = 0x08000000,
    // Reserved.
    LVXS_OWNERDRAWVARIABLE = 0x10000000,
};

// https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass.htm

struct TV_ITEM_EX {

};

struct LV_ITEM_EX {

};

struct Node {

};

struct Item {

};

struct SECTreeNode {

};

enum {
    TVIF_EX_STATEEX,
    TVHT_COLUMNHEADING,
    TVHT_COLUMNSEP,
};

struct LvPaintContext {
    LV_ITEM lvi;
    COLORREF rgbText;
    COLORREF rgbTextBkgnd;
    COLORREF rgbIconBkgnd;
    COLORREF rgbItemBkgnd;
    virtual ~LvPaintContext() = default;
};

struct TvPaintContext : LvPaintContext {
    TV_ITEM tvi;
};

using SEC_DWORD = DWORD;

//! What the item data pair carries.
//!
//! The toolkit spelled these SEC_DWORD, which is DWORD, which is 32 bits on
//! both targets. The control's TVITEM::lParam is pointer sized, and the one
//! caller in this editor -- CSortTreeControl::InsertTreeItem -- stores an
//! HTREEITEM in it, so on x64 the toolkit's own width silently threw half the
//! handle away. Nothing reads it back today, which is the only reason that has
//! not shown up as a fault; widening it here means it cannot.
using SEC_ITEMDATA = LPARAM;

// The image list slot the toolkit kept for the column headings. Spelled out
// here as well as in ot_dockingwindows.h, and to the same value, because
// SetImageList has to recognise it and that lives in this header's own
// implementation.
#ifndef LVSIL_HEADER
#define LVSIL_HEADER 3
#endif

class SEC_TREECLASS;

//! A tree's column header.
//!
//! A child of the tree's parent and not of the tree, because a child window
//! is confined to its parent's client area, and the tree's client area is
//! exactly where the items are: a header inside it would sit on top of the
//! first row, which is one row of every tree permanently out of sight. The
//! tree gives up a strip of its frame instead, through WM_NCCALCSIZE, and
//! this stands in it.
//!
//! Being a sibling, its notifications go to the tree's parent, which is
//! where MFC reflects any control's back to the control -- and it reflects
//! before it dispatches, so a parent with notification handlers of its own
//! does not intercept these.
class SECTreeHeaderCtrl : public CHeaderCtrl {
public:
    SECTreeHeaderCtrl();

    //! The tree this heads. Not the parent window, so it has to be kept.
    SEC_TREECLASS *m_pTree;

protected:
    afx_msg void OnItemChanged( NMHDR *pNMHDR, LRESULT *pResult );

    DECLARE_MESSAGE_MAP()
};

class SEC_TREECLASS : public CWnd {
public:
    // Construction/Initialization
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__sec_treeclass.htm
    // Construction
    SEC_TREECLASS();

    // Operations
    // retrieve the active Column
    int GetActiveColumn();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__create.htm
    // Create an SECTreeCtrl
    virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__create.htm
    // Create an SECTreeCtrl
    virtual BOOL Create(DWORD dwStyle, DWORD dwStyleEx, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__subclasstreectrlid.htm
    // Create a SECTreeCtrl on a dialog template.
    BOOL SubclassTreeCtrlId( UINT idc, CWnd *pWndDlg );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getitemrect.htm
    // Retrieves the bounding rectangle for an item and determines whether it is visible or not.
    BOOL GetItemRect( HTREEITEM hti, LPRECT lpRect, UINT nCode ) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getindent.htm
    // Retrieves the amount, in pixels, that child items are indented relative to their parents.
    UINT GetIndent() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setindent.htm
    // Sets the width of indentation for a tree view control and redraws the control to reflect the new width.
    void SetIndent(UINT nIndent);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setitemheight.htm
    // Sets the height of a single tree item
    UINT SetItemHeight(HTREEITEM hti, UINT cyItemHeight) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getnextitem.htm
    // Retrieves the next tree view item.
    HTREEITEM GetNextItem(HTREEITEM hItem, UINT nCode) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getchilditem.htm
    // Retrieves the tree view item that is the child of the item specified.
    HTREEITEM GetChildItem(HTREEITEM hItem) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getnextsiblingitem.htm
    // Retrieves the next sibling of the item specified.
    HTREEITEM GetNextSiblingItem(HTREEITEM hItem) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getprevsiblingitem.htm
    // Retrieves the previous sibling of the item specified.
    HTREEITEM GetPrevSiblingItem(HTREEITEM hItem) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getparentitem.htm
    // Retrieves the parent of the item specified.
    HTREEITEM GetParentItem(HTREEITEM hItem) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getfirstvisibleitem.htm
    // Retrieves the first visible item of the tree view control.
    HTREEITEM GetFirstVisibleItem() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getlastvisibleitem.htm
    // Retrieves the last visible item of the tree view control.
    HTREEITEM GetLastVisibleItem() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getnextvisibleitem.htm
    // Retrieves the next visible item of the item specified.
    HTREEITEM GetNextVisibleItem(HTREEITEM hItem) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getprevvisibleitem.htm
    // Retrieves the previous visible item of the item specified.
    HTREEITEM GetPrevVisibleItem(HTREEITEM hItem) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getfirstselecteditem.htm
    // Retrieves the first item currently selected in the tree view control.
    HTREEITEM GetFirstSelectedItem() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getnextselecteditem.htm
    // Retrieves the next item currently selected in the tree view control.
    HTREEITEM GetNextSelectedItem(HTREEITEM hItem) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getprevvisibleitem.htm
    // Retrieves the previously selected item in the tree view control.
    HTREEITEM GetPrevSelectedItem(HTREEITEM hItem) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getcaretitem.htm
    // Retrieves the item that currently has the caret (i.e., keyboard focus).
    HTREEITEM GetCaretItem() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getselecteditem.htm
    // Retrieves a currently selected item of the tree view control.
    HTREEITEM GetSelectedItem() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getdrophilightitem.htm
    // Retrieves the item that is the target of a drag-and-drop operation.
    HTREEITEM GetDropHilightItem() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getrootitem.htm
    // Retrieves the root item of the tree view control.
    HTREEITEM GetRootItem(HTREEITEM hti = NULL) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getnextitemindisplayorder.htm
    // Gets the next displayed item.
    HTREEITEM GetNextItemInDisplayOrder(HTREEITEM hti) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getprevitemindisplayorder.htm
    // Gets the previous displayed item.
    HTREEITEM GetPrevItemInDisplayOrder(HTREEITEM hti) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setitem.htm
    // Sets the attributes of the specified item.
    virtual BOOL SetItem(const LV_ITEM* pLVI, BOOL bRedraw = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setitem.htm
    // Sets the attributes of the specified item.
    BOOL SetItem(TV_ITEM* pItem);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setitem.htm
    // Sets the attributes of the specified item.
    BOOL SetItem(HTREEITEM hItem, UINT nMask, LPCTSTR lpszItem, int nImage, int nSelectedImage, UINT nState, UINT nStateMask, LPARAM lParam);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setitemex.htm
    // Sets the extended attributes of the specified item.
    virtual BOOL SetItemEx(TV_ITEM* pTVI, TV_ITEM_EX* pTVIX);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setitemex.htm
    // Sets the extended attributes of the specified item.
    virtual BOOL SetItemEx(LV_ITEM* pLVI, const LV_ITEM_EX* pLVIX);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setitemimage.htm
    // Sets the index of the item’s image and its selected image within the tree view control’s image list.
    BOOL SetItemImage(HTREEITEM hItem, int nImage, int nSelectedImage);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setitemstate.htm
    // Sets the state of the item specified.
    BOOL SetItemState(HTREEITEM hItem, UINT nState, UINT nStateMask);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setitemdata.htm
    // Sets the 32-bit application-specific value associated with the item specified.
    BOOL SetItemData(HTREEITEM hItem, SEC_ITEMDATA dwData);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getitem.htm
    // Retrieves the attributes of the specified item.
    virtual BOOL GetItem(TV_ITEM* pItem, BOOL bCopyText = TRUE, BOOL bGetDispInfo = FALSE) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getitem.htm
    // Retrieves the attributes of the specified item.
    virtual BOOL GetItem(LV_ITEM* pLVI, BOOL bCopyText = TRUE, BOOL bGetDispInfo = FALSE) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getitemex.htm
    // Retrieves the extended attributes of the specified item.
    virtual BOOL GetItemEx(TV_ITEM* pTVI, TV_ITEM_EX* pTVIX, BOOL bGetDispInfoEx = FALSE) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getitemex.htm
    // Retrieves the extended attributes of the specified item.
    virtual BOOL GetItemEx(LV_ITEM* pLVI, LV_ITEM_EX* pLVIX, BOOL bGetDispInfoEx = FALSE) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getitemimage.htm
    // Retrieves the index of the item’s image and its selected image within the tree view control’s image list.
    BOOL GetItemImage(HTREEITEM hItem, int& nImage, int& nSelectedImage) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getitemstate.htm
    // Returns the state of the item specified.
    UINT GetItemState(HTREEITEM hItem, UINT nStateMask) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getitemdata.htm
    // Gets the 32-bit application-specific value associated with the item specified.
    SEC_ITEMDATA GetItemData(HTREEITEM hItem) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getnode.htm
    // Gets the stored pointer to a Node from an HTREEITEM.
    Node* GetNode( HTREEITEM hti ) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getnode.htm
    // Gets the stored pointer to a node from a list Item pointer.
    Node* GetNode( Item* pItem ) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getnodeat.htm
    // Gets the stored pointer of a node based on an item index.
    Node* GetNodeAt( int nIndex ) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setnodeparent.htm
    // Changes the parent of an existing node.
    BOOL SetNodeParent( SECTreeNode *pNode, SECTreeNode *pNodeParent, BOOL bInvalidate=TRUE, HTREEITEM hInsertAfter=TVI_LAST );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getitemat.htm
    // Gets an HTREEITEM based on a list index.
    HTREEITEM GetItemAt(int nIndex) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__itemindex.htm
    // Returns the list index of an HTREEITEM.
    int ItemIndex(HTREEITEM, int nStartFrom = 0) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__nodeindex.htm
    // Returns the list index of a Node.
    int NodeIndex(Node *, int nStartFrom = 0) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__isexpanded.htm
    // Determines whether a given item is expanded (i.e., displaying the list of child items, if any, associated with it).
    BOOL IsExpanded(HTREEITEM hItem) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__iscallbackitem.htm
    // Determines whether specified item uses text or image callbacks.
    virtual BOOL IsCallbackItem(int nIndex) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__storesubitemtext.htm
    // Enables or disables sub-item text storage for multi-column trees.
    virtual void StoreSubItemText( BOOL bEnable = TRUE );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__isstoringsubitemtext.htm
    // Returns the state of sub-item text storage.
    virtual BOOL IsStoringSubItemText() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getitemtext.htm
    // Returns the text of the specified item or sub-item.
    CString GetItemText(HTREEITEM hItem, int iSubItem = 0) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setitemtext.htm
    // Sets the text of the specified item.
    virtual BOOL SetItemText(HTREEITEM hItem, LPCTSTR lpszItem);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setitemtext.htm
    // Sets the text of the specified item or sub-item.
    virtual BOOL SetItemText(HTREEITEM hItem, int nSubItem, LPCTSTR lpszItem);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setitemstring.htm
    // Sets the text of the specified item or sub-item.
    BOOL SetItemString(HTREEITEM hti, int nSubItem, const CString& strItem);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getitemstring.htm
    // Returns the text of the specified item or sub-item.
    BOOL GetItemString(HTREEITEM hti, int nSubItem, CString& strItem);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setnoellipsis.htm
    // Allows scrolling till the end of the text instead of using ellipsis.
    BOOL SetNoEllipsis(BOOL bNoEllipsis = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__insertbatch.htm
    // Insert a group of items under a common parent.
    BOOL InsertBatch(TV_ITEM** ppItems, HTREEITEM hParent, int cItems, BOOL bInvalidate=TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__insertitem.htm
    // Inserts a new item in a tree view control.
    HTREEITEM InsertItem(LPTV_INSERTSTRUCT lpInsertStruct);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__insertitem.htm
    // Inserts a new item in a tree view control.
    HTREEITEM InsertItem(UINT nMask, LPCTSTR lpszItem, int nImage, int nSelectedImage, UINT nState, UINT nStateMask, LPARAM lParam, HTREEITEM hParent, HTREEITEM hInsertAfter);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__insertitem.htm
    // Inserts a new item in a tree view control.
    HTREEITEM InsertItem(LPCTSTR lpszItem, HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__insertitem.htm
    // Inserts a new item in a tree view control.
    HTREEITEM InsertItem(LPCTSTR lpszItem, int nImage, int nSelectedImage, HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__deleteitem.htm
    // Deletes an item from the tree view control.
    BOOL DeleteItem(HTREEITEM hItem);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__deleteallitems.htm
    // Deletes all items from the tree view control.
    BOOL DeleteAllItems();
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__expand.htm
    // Expands or collapses the list of child items, if any, associated with the given parent item.
    virtual BOOL Expand(HTREEITEM hItem, UINT nCode, BOOL bRedraw = TRUE, BOOL bForceExpand = FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__expandcompletely.htm
    // Recursively expands an item and all of its children.
    void ExpandCompletely(HTREEITEM hItem, BOOL bRedraw = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__collapsecompletely.htm
    // Recursively collapse an item and all of its children
    void CollapseCompletely(HTREEITEM hItem, BOOL bRedraw = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__select.htm
    // Selects the given tree view item, scrolls the item into view, or redraws the item in the style used to indicate the target of a drag-and-drop operation.
    BOOL Select(HTREEITEM hItem, UINT nCode);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__selectitem.htm
    // Selects the given tree view item.
    BOOL SelectItem(HTREEITEM hItem);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__selectdroptarget.htm
    // Redraws the item in the style used to indicate the target of a drag-and-drop operation.
    BOOL SelectDropTarget(HTREEITEM hItem, BOOL bAutoScroll=TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__selectsetfirstvisible.htm
    // Scrolls the tree view vertically so that the given item is the first visible item and gives it the focus and selection.
    BOOL SelectSetFirstVisible(HTREEITEM hItem);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setfirstvisible.htm
    // Determines whether a given item is set as the first visible item in the tree view control.
    BOOL SetFirstVisible(HTREEITEM hti);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__hittest.htm
    // Determines the location of the specified point relative to the client area of a tree view control.
    HTREEITEM HitTest(CPoint pt, UINT* pFlags = NULL);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__hittest.htm
    // Determines the location of the specified point relative to the client area of a tree view control.
    HTREEITEM HitTest(TV_HITTESTINFO* pHitTestInfo);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__createdragimage.htm
    // Creates a dragging bitmap for the given item in a tree view control, creates an image list for the bitmap, and adds the bitmap to the image list. An application uses the image-list functions to display the image when an item is being dragged.
    CImageList* CreateDragImage(HTREEITEM hItem);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__createdragimageex.htm
    // Extends CreateDragImage to create multiple selection images
    CImageList* CreateDragImageEx(HTREEITEM hItem, CPoint& ptOffset);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__ensurevisible.htm
    // Ensures that a tree view item is visible. If necessary, expands the parent item or scrolls the tree view control so that the item is visible.
    BOOL EnsureVisible(HTREEITEM hItem, BOOL bParentVisible = FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__compareitem.htm
    // Compares the text of two items.
    virtual int CompareItem(Node *pNode1, Node *pNode2);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__sortchildren.htm
    // Sorts the child items of the given parent item in a tree view control.
    BOOL SortChildren(HTREEITEM hItem, BOOL bRecursive = TRUE, BOOL bAscending = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__sortchildrencb.htm
    // Sorts tree view items using an application-defined callback function that compares the items.
    BOOL SortChildrenCB(LPTV_SORTCB pSort, BOOL bRecursive = TRUE, BOOL bAscending = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__editlabel.htm
    // Begins in-place editing of the specified item’s text.
    CEdit* EditLabel(HTREEITEM hItem, int nCol = 0);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getchildcount.htm
    // Returns the number of children an item has.
    UINT GetChildCount(HTREEITEM hti, BOOL bRecursive = TRUE, BOOL bExpandedOnly = FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__selectallvisiblechildren.htm
    // Selects all child nodes visible beneath a parent node.
    BOOL SelectAllVisibleChildren(HTREEITEM hti);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__selectitemrange.htm
    // Selects/deselects a range of items.
    BOOL SelectItemRange( HTREEITEM htiFirst, HTREEITEM htiLast, BOOL bSelect);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__isselected.htm
    // Indicates whether an item is selected.
    BOOL IsSelected(HTREEITEM hti) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__hideitem.htm
    // Hides/Shows an item.
    BOOL HideItem( HTREEITEM hti, BOOL bHide );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__ishidden.htm
    // Indicates whether or not an item is hidden.
    BOOL IsHidden( HTREEITEM hti ) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getfirsthiddenitem.htm
    // Gets first hidden item.
    HTREEITEM GetFirstHiddenItem(void) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getnexthiddenitem.htm
    // Gets next hidden item.
    HTREEITEM GetNextHiddenItem(HTREEITEM) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__unhideallitems.htm
    // Makes all hidden items visible.
    BOOL UnHideAllItems(void);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__disableallitems.htm
    // Disables all items (shown in disabled text color), or enables disabled items.
    BOOL DisableAllItems( BOOL bDisable );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__disableitem.htm
    // Disables/enables a particular item.
    BOOL DisableItem( HTREEITEM hti, BOOL bDisable );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__isdisabled.htm
    // Indicates whether or not an item is disabled.
    BOOL IsDisabled( HTREEITEM ) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getfirstdisableditem.htm
    // Gets first disabled item.
    HTREEITEM GetFirstDisabledItem(void) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getnextdisableditem.htm
    // Gets next disabled item.
    HTREEITEM GetNextDisabledItem(HTREEITEM) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__hidedisableditems.htm
    // Hides items that are disabled.
    void HideDisabledItems(BOOL bHide);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__ishidedisableditems.htm
    // Indicates whether or not disabled items are hidden.
    BOOL IsHideDisabledItems() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getcount.htm
    // Retrieves a count of the items in a tree view control.
    UINT GetCount() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__itemhaschildrenondemand.htm
    // Determines whether the tree item specified has child items on demand.
    BOOL ItemHasChildrenOnDemand(HTREEITEM hItem) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__itemhaschildren.htm
    // Determines whether the tree item specified has child items.
    BOOL ItemHasChildren(HTREEITEM hItem) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__itemexists.htm
    // Indicates whether or not an item exists in the tree view control.
    BOOL ItemExists(HTREEITEM hti) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__enablescrollonexpand.htm
    // Enables/disables scrolling after an item is expanded, to make the last child visible.
    void EnableScrollOnExpand(BOOL bEnable=TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getimagelist.htm
    // Returns a pointer to the requested image list.
    virtual CImageList* GetImageList(UINT nImageList) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setimagelist.htm
    // Sets the normal or state image list.
    virtual CImageList* SetImageList(CImageList* pImageList, int nImageListType);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__enableheaderctrl.htm
    // Enables/disables the column header control.
    virtual void EnableHeaderCtrl(BOOL bEnable = TRUE, BOOL bSortHeader = FALSE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__enablewordwrap.htm
    // Enables/disables wrapping of item text.
    virtual void EnableWordWrap(BOOL bEnable = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__enabletooltips.htm
    // Enables/disables tooltips.
    virtual void EnableToolTips(BOOL bEnable = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__enablemultiselect.htm
    // Enables/disables multiple selection.
    virtual void EnableMultiSelect(BOOL bEnable = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__gettreectrlstyles.htm
    // Retrieves values for the style and extended style of a tree view control.
    virtual BOOL GetTreeCtrlStyles(DWORD& dwStyle, DWORD& dwStyleEx) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__settreectrlstyles.htm
    // Sets values for the style and extended style of a tree view control.
    virtual BOOL SetTreeCtrlStyles(DWORD dwStyle, DWORD dwStyleEx, BOOL bRedraw = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__gettreectrlstyle.htm
    // Retrieves the style of a tree view control.
    virtual DWORD GetTreeCtrlStyle() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__gettreectrlstyleex.htm
    // Retrieves the extended style of a tree view control.
    virtual DWORD GetTreeCtrlStyleEx() const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__settreectrlstyle.htm
    // Sets value for the style of a tree view control.
    virtual BOOL SetTreeCtrlStyle(DWORD dwStyle, BOOL bRedraw = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__settreectrlstyleex.htm
    // Modifies a tree view control’s style by removal and addition of specified styles.
    virtual BOOL SetTreeCtrlStyleEx(DWORD dwStyleEx, BOOL bRedraw = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__modifytreectrlstyle.htm
    // Modifies a tree view control’s style by removal and addition of specified styles.
    virtual BOOL ModifyTreeCtrlStyle(DWORD dwRemove, DWORD dwAdd, BOOL bRedraw = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__modifytreectrlstyleex.htm
    // Modifies a tree view control’s extended style by removal and addition of specified extended styles.
    virtual BOOL ModifyTreeCtrlStyleEx(DWORD dwRemoveEx, DWORD dwAddEx, BOOL bRedraw = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__modifytreectrlstyles.htm
    // Modifies a tree view control’s style ans extended style by removal and addition of specified styles and extended styles.
    virtual BOOL ModifyTreeCtrlStyles(DWORD dwRemove, DWORD dwAdd, DWORD dwRemoveEx, DWORD dwAddEx, BOOL bRedraw = TRUE);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setfilterlevel.htm
    // Filters out a complete level of the tree from visibility.
    void SetFilterLevel(WORD wLevel);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__getfilterlevel.htm
    // Retrieves level of the tree isolated by the filter set by SetFilterLevel.
    WORD GetFilterLevel(void) const;
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__update.htm
    // Updates the drawing of an item.
    virtual BOOL Update( HTREEITEM hti, BOOL bLabelOnly = FALSE, BOOL bEraseBkgnd = TRUE, BOOL bUpdateBelow = FALSE, BOOL bUpdateNow = FALSE );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__invalidateitem.htm
    // Invalidates an item's rectangle.
    inline BOOL InvalidateItem(HTREEITEM hti);
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__remeasureitem.htm
    // Causes the item's width and height to be measured. The item will then be invalidated.
    void ReMeasureItem( HTREEITEM hti );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setautoexpanddelay.htm
    // sets the hover time required to auto expand a drop target. Must have TVXS_AUTOEXPAND.
    void SetAutoExpandDelay( UINT nDelay );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__setmaxanimations.htm
    // sets the maximum numbret of animations when expanding or collapsing, for TVXS_ANIMATE style
    void SetMaxAnimations( int nMaxAnimations );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__insertcolumn.htm
    // Inserts a column.
    virtual BOOL InsertColumn( int nCol, const CString& strHeader, int nFormat=LVCFMT_LEFT, int wWidth=-1, int iSubItem = -1, int iImage = -1, BOOL bUpdate = TRUE );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__deletecolumn.htm
    // Deletes a column.
    virtual BOOL DeleteColumn( int nCol );
    // https://help.perforce.com/stingray/2023.2/Stingray_Studio_API_Documentation/Content/Toolkit/sec_treeclass__deletecolumn.htm
    // Deletes a column.
    virtual BOOL DeleteColumn( const CString& strColumnHeading );

    UINT GetColumnCount() const;
    virtual BOOL ModifyListCtrlStyle(DWORD dwRemove, DWORD dwAdd, BOOL bRedraw = TRUE);
    virtual BOOL ModifyListCtrlStyleEx(DWORD dwRemoveEx, DWORD dwAddEx, BOOL bRedraw = TRUE);
    virtual void SetColumnHeading( int nCol, const CString& strHeading );
    virtual void SetColumnFormat( int nCol, int fmt );
    virtual int GetColumnWidth( int nCol ) const;
    virtual void SetColumnWidth( int nCol, int width );
    void SetColumnImage( int nCol, int nImage );
    virtual void PickTextColors(LvPaintContext* pPC);
    virtual BOOL SetBkColor(COLORREF rgbBk);
    virtual BOOL SetIconBkColor(COLORREF rgbIconBk);
    virtual BOOL SetSelIconBkColor(COLORREF rgbSelIconBk);
    virtual BOOL DeselectAllItems(int iExclude = -1);
    virtual UINT GetSelectedCount() const;
    virtual void RecalcScrollBars();
    virtual CEdit* GetEditControl();

protected:
    // The toolkit's own extended styles (the TVXS_ and LVXS_ half of the word
    // above). None of them is a window style, so they are kept here rather than
    // read back off the window, and nothing acts on them yet.
    DWORD m_dwTreeCtrlStyleEx;

    //! One of the tree's columns.
    //!
    //! The toolkit drew the tree itself and so had columns; SysTreeView32 has
    //! none and drops every call about them. They are kept here instead, which
    //! is what lets the calls answer honestly. Nothing paints them yet: that
    //! needs a header control and a custom-draw pass, and it needs this first.
    struct SColumn {
        CString strHeading;
        int nFormat;
        int nWidth;
        int nSubItem;
        int nImage;
    };

    //! Never empty. The editor calls SetColumnHeading( 0, ... ) before it
    //! inserts anything -- "первая колонка уже существует" in
    //! ComboBox_GDBBrowser.h -- so column zero exists from construction, and
    //! InsertColumn adds the ones after it.
    std::vector<SColumn> m_columns;

    //! Whether SetItemText keeps the text for columns past the first.
    //!
    //! Off until asked, which is the toolkit's model and what every caller in
    //! this editor does: all four trees that use subitems call
    //! StoreSubItemText( true ) as they are built. With it off, SetItemText for
    //! a subitem answers FALSE because it really did not store it, which is a
    //! different thing from the constant FALSE it used to answer.
    BOOL m_bStoreSubItemText;

    //! Text for columns past the first, per item.
    //!
    //! Column zero's text belongs to the control and is read back from it;
    //! the rest has nowhere else to live. Keyed by HTREEITEM, so it has to be
    //! forgotten when an item goes away -- the control reuses handles, and a
    //! new item inheriting a dead one's text would be worse than having none.
    std::map<HTREEITEM, std::vector<CString> > m_subItemText;

    //! Drop the stored text for an item and everything under it.
    void ForgetSubItemText( HTREEITEM hItem );

    //! So the header can hand a dragged width back to the columns.
    friend class SECTreeHeaderCtrl;

    // ---- the columns on screen ------------------------------------------
    //
    // The toolkit drew the tree itself, so its columns were simply part of
    // that drawing. SysTreeView32 draws one column and knows nothing of any
    // other, so the headings are a real header control put over the tree and
    // the text for the columns past the first is painted in a custom-draw
    // pass. Neither is something the control offers; both are additions on
    // top of it, and each is commented where it gives a poorer answer than
    // the toolkit's own drawing did.

    //! The column heading control, created on the first EnableHeaderCtrl.
    //! See SECTreeHeaderCtrl for why it is not a child of the tree.
    SECTreeHeaderCtrl m_wndHeader;

    //! The height of the strip the header stands in, and whether the tree
    //! is currently giving it up. Asked for by OnNcCalcSize, which is called
    //! often enough that measuring the header each time would be waste.
    int m_nHeaderHeight;
    bool m_bHeaderInset;

    //! What EnableHeaderCtrl last said.
    //!
    //! Show and hide, not create and destroy: CPCMainTreeControl calls
    //! EnableHeaderCtrl( GetCount() != 0 ) every time its contents change,
    //! so the header goes whenever the tree empties and is wanted back with
    //! the next item.
    BOOL m_bHeaderEnabled;

    //! The list styles, which the header is the only thing here that acts on.
    //!
    //! LVS_NOSORTHEADER decides whether the headings are buttons, and
    //! LVXS_HILIGHTSUBITEMS whether a selected row is drawn selected across
    //! all of its columns. The rest is kept only so that what was set is
    //! what is read back.
    DWORD m_dwListCtrlStyle;
    DWORD m_dwListCtrlStyleEx;

    //! The image list for the column headings, LVSIL_HEADER.
    //!
    //! The tree control has a normal list and a state list and no third
    //! slot, so this used to be forwarded under a type number the control
    //! does not define, and dropped. It belongs to the header, and
    //! SetColumnImage picks out of it.
    CImageList *m_pHeaderImageList;

    //! Set while SyncHeaderColumns writes the header, so the changes it
    //! causes are not read back as the user dragging a divider.
    bool m_bSyncingHeader;

    //! The colour behind an item's icon, and behind a selected item's.
    //!
    //! CLR_NONE until asked for, which is the image list's own "no
    //! background, draw masked" and so is also the right starting value.
    //! The two are kept apart because the toolkit had both, even though an
    //! image list can only carry one -- see ApplyIconBkColor.
    COLORREF m_rgbIconBk = CLR_NONE;
    COLORREF m_rgbSelIconBk = CLR_NONE;

    //! Push the icon background onto the image list, if there is one yet.
    BOOL ApplyIconBkColor();

    //! Where LayoutHeader last put the header and how much of it it last
    //! showed, so a repaint does not move or re-clip a window that is
    //! already right.
    CRect m_rectHeaderPlaced;
    CRect m_rectHeaderVisible;

    //! A column's width as something to lay out with, so never negative.
    //! InsertColumn's default is -1, which the toolkit read as "measure it";
    //! nothing here measures, so it lays out as nothing.
    int GetLayoutColumnWidth( int nCol ) const;
    //! Left edge of a column in client coordinates, horizontal scroll included.
    int GetColumnLeft( int nCol ) const;
    //! Create the header if it is wanted and missing, then match it to the
    //! columns and to whether it should be on screen.
    void UpdateHeaderCtrl();
    //! Replace the header's items with m_columns.
    void SyncHeaderColumns();
    //! Put the header across the top of the client area, scrolled with the tree.
    void LayoutHeader();
    //! Paint one item's columns past the first.
    void DrawSubItems( CDC *pDC, HTREEITEM hItem );
    //! Make the frame agree with whether there is a header to stand in it.
    void UpdateHeaderInset();
    //! A column width the user dragged, on its way back to m_columns.
    void SetColumnWidthFromHeader( int nCol, int nWidth );

    afx_msg void OnNcCalcSize( BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR *lpncsp );
    afx_msg void OnWindowPosChanged( WINDOWPOS *lpwndpos );
    afx_msg void OnNcDestroy();
    afx_msg void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar *pScrollBar );
    afx_msg void OnCustomDraw( NMHDR *pNMHDR, LRESULT *pResult );

    DECLARE_MESSAGE_MAP()
};
