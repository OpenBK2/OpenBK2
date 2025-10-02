#pragma once

#include "../misc/HashFuncs.h"


// Нельзя использовать GetItemData() и SetItemData() !
// Вставлять элемент в дерево только функцией InsertTreeItem
// Удалять элемент из дерева только функцией DeleteTreeItem
// Удалять все элементы из дерева функцией DeleteAllTreeItems
class CSortTreeControl : public SECTreeCtrl
{
protected:
	typedef hash_map<string, HTREEITEM> CTreeItemMap;
	typedef hash_map<HTREEITEM, COLORREF, SDefaultPtrHash> CTreeItemColorMap;
	typedef hash_map<HTREEITEM, bool, SDefaultPtrHash> CTreeItemBoolMap;
	
private:
	CTreeItemMap treeItemMap;
	CTreeItemColorMap treeItemColorMap;
	CTreeItemBoolMap treeItemReadOnlyMap;
	CTreeItemMap clipboardTreeItemMap;
	bool bClipboardCut;
	bool bEnableEdit;

protected:	
	CSortTreeControl() : bClipboardCut( false ), bEnableEdit( true ) {}

	void ClearTreeItemMap();
	void RemoveTreeItemFromMaps( HTREEITEM hItem );
	//
	HTREEITEM InsertTreeItem( LPCTSTR lpszItem, int nImage, int nSelectedImage, HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST );
	bool DeleteAllTreeItems();
	bool DeleteTreeItem( HTREEITEM hItem );
	bool RenameTreeItem( const string &rszDestination, const string &rszSource );
	//
	bool IsTopSelection( HTREEITEM hItem, HTREEITEM hItemToSkip = 0 );
	//
	// Clipboard methods
	void FillClipboard( bool _bClipboardCut );
	void ClearClipboard() { clipboardTreeItemMap.clear(); }
	//
	bool IsClipboardEmpty() const { return clipboardTreeItemMap.empty(); };
	bool IsClipboardItem( HTREEITEM hItem ) const;
	bool IsClipboardCut() const { return bClipboardCut; }
	//
	void FillWindowsClipboard( HTREEITEM hItem, string *pszWindowClipboardText );
	//
	const CTreeItemMap& GetClipboard() const { return clipboardTreeItemMap; }
	//	
	void SetTreeItemColor( HTREEITEM hItem, COLORREF color ); 
	bool GetTreeItemColor( HTREEITEM hItem, COLORREF *pColor ); 
	void RemoveTreeItemColorFromCache( HTREEITEM hItem ); 
	//
	void SetTreeItemReadOnly( HTREEITEM hItem, bool bReadOnly ); 
	bool GetTreeItemReadOnly( HTREEITEM hItem, bool *pbReadOnly ); 
	void RemoveTreeItemReadOnlyFromCache( HTREEITEM hItem ); 

	// Name Cache
	void ClearNameCache( const string &rszName );
	void SetNameCache( const string &rszName, HTREEITEM hItem );

	//CSortTreeControl
	virtual void SortTree( HTREEITEM hParentItem, PFNTVCOMPARE pfnCompare, LPARAM lParam );
	virtual HTREEITEM GetTreeItem( const string &rszName );
	//
	virtual void GetClipboardPrefix( string *pszClipboardPrefix ) = 0;
	virtual bool GetTreeItemName( HTREEITEM hItem, string *pszName ) = 0;
	virtual bool IsIgnoreCase() = 0;
public:

	virtual void EnableEdit( bool bEnable ) { bEnableEdit = bEnable; }
	bool IsEditEnabled() const { return bEnableEdit; }

	DECLARE_MESSAGE_MAP()
};


