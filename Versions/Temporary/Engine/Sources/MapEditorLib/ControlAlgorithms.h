#pragma once


namespace NCA
{
	enum ESelectionType
	{
		ST_FOCUS						= 0,
		ST_SELECT						= 1,
		ST_COMPLETE					= 2,
		ST_COMPLETE_FOCUS		= 3,
		ST_COMPLETE_SELECT	= 4,
	};
	typedef list<HTREEITEM> CHTREEITEMList;
	static MSG acceleratorMessage;
	//
	bool IsParent( HTREEITEM hItem, HTREEITEM hParentItem, const SECTreeCtrl *pTreeControl );
	void CreateFullName( string *pszFullName, HTREEITEM hItem, const string &rszTreeItemDivider, const SECTreeCtrl *pTreeControl );
	bool IsTopSelection( HTREEITEM hItem, HTREEITEM hItemToSkip, const SECTreeCtrl *pTreeControl );
	int CreateSelection( CHTREEITEMList *pSelection, ESelectionType eSelectionType, HTREEITEM hParentItem, HTREEITEM hItemToSkip, const SECTreeCtrl *pTreeControl );
	void SelectAll( HTREEITEM hParentItem, HTREEITEM hItemToSkip, SECTreeCtrl *pTreeControl );
	void FillWindowsClipboard( const CHTREEITEMList &rSelection,
														 const string &rszClipboardPrefix,
														 const string &rszClipboardPostfix,
														 const string &rszItemPrefix,
														 const string &rszItemPostfix,
														 const string &rszTreeItemDivider,
														 const string &rszColumnDivider,
														 const SECTreeCtrl *pTreeControl );
	bool TranslateAccelerators( bool bModal, unsigned nMessage, WPARAM wParam, LPARAM lParam );
};


