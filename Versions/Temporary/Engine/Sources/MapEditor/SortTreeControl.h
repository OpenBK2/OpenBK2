#pragma once

#include "Misc/HashFuncs.h"


// Нельзя использовать GetItemData() и SetItemData() !
// Вставлять элемент в дерево только функцией InsertTreeItem
// Удалять элемент из дерева только функцией DeleteTreeItem
// Удалять все элементы из дерева функцией DeleteAllTreeItems
class CSortTreeControl : public SECTreeCtrl
{
protected:
	typedef std::unordered_map<std::string, HTREEITEM> CTreeItemMap;
	typedef std::unordered_map<HTREEITEM, COLORREF> CTreeItemColorMap;
	typedef std::unordered_map<HTREEITEM, bool> CTreeItemBoolMap;
	
private:
	CTreeItemMap treeItemMap;
	CTreeItemColorMap treeItemColorMap;
	CTreeItemBoolMap treeItemReadOnlyMap;
	CTreeItemMap clipboardTreeItemMap;
	bool bClipboardCut;
	bool bEnableEdit;
	unsigned nUpdateDepth = 0;
	bool bRestoreRedraw = false;

protected:	

	// Native tree controls recalculate layout on every insert unless redraw is
	// disabled. Nesting lets synchronous property-tree batches share one update.
	class CTreeUpdateLock
	{
		CSortTreeControl &tree;
		bool bLocked = true;
	public:
		explicit CTreeUpdateLock( CSortTreeControl &value ) : tree(value)
		{
			if ( tree.nUpdateDepth++ == 0 )
			{
				tree.bRestoreRedraw = ( tree.GetStyle() & WS_VISIBLE ) != 0;
				if ( tree.bRestoreRedraw )
					tree.SetRedraw( FALSE );
			}
		}
		~CTreeUpdateLock() { Unlock(); }
		// Finish native layout before scrolling, selecting or positioning editors.
		void Unlock()
		{
			if ( !bLocked )
				return;
			bLocked = false;
			if ( --tree.nUpdateDepth == 0 && tree.bRestoreRedraw )
			{
				tree.SetRedraw( TRUE );
				tree.RedrawWindow( nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN );
			}
		}
		CTreeUpdateLock( const CTreeUpdateLock & ) = delete;
		CTreeUpdateLock &operator=( const CTreeUpdateLock & ) = delete;
	};

	CSortTreeControl() : bClipboardCut( false ), bEnableEdit( true ) {}

	void ClearTreeItemMap();
	void RemoveTreeItemFromMaps( HTREEITEM hItem );
	//
	HTREEITEM InsertTreeItem( LPCTSTR lpszItem, int nImage, int nSelectedImage, HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST );
	bool DeleteAllTreeItems();
	bool DeleteTreeItem( HTREEITEM hItem );
	bool RenameTreeItem( const std::string &rszDestination, const std::string &rszSource );
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
	void FillWindowsClipboard( HTREEITEM hItem, std::string *pszWindowClipboardText );
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
	void ClearNameCache( const std::string &rszName );
	void SetNameCache( const std::string &rszName, HTREEITEM hItem );

	//CSortTreeControl
	virtual void SortTree( HTREEITEM hParentItem, PFNTVCOMPARE pfnCompare, LPARAM lParam );
	virtual HTREEITEM GetTreeItem( const std::string &rszName );
	//
	virtual void GetClipboardPrefix( std::string *pszClipboardPrefix ) = 0;
	virtual bool GetTreeItemName( HTREEITEM hItem, std::string *pszName ) = 0;
	virtual bool IsIgnoreCase() = 0;
public:

	virtual void EnableEdit( bool bEnable ) { bEnableEdit = bEnable; }
	bool IsEditEnabled() const { return bEnableEdit; }

	DECLARE_MESSAGE_MAP()
};


