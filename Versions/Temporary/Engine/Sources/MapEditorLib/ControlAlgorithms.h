#pragma once

#include "MapEditorLib_export.h"

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
	typedef std::list<HTREEITEM> CHTREEITEMList;
	static MSG acceleratorMessage;
	//
	bool IsParent( HTREEITEM hItem, HTREEITEM hParentItem, const SECTreeCtrl *pTreeControl );
	void CreateFullName( std::string *pszFullName, HTREEITEM hItem, const std::string &rszTreeItemDivider, const SECTreeCtrl *pTreeControl );
	bool IsTopSelection( HTREEITEM hItem, HTREEITEM hItemToSkip, const SECTreeCtrl *pTreeControl );
	MAPEDITORLIB_EXPORT int CreateSelection( CHTREEITEMList *pSelection, ESelectionType eSelectionType, HTREEITEM hParentItem, HTREEITEM hItemToSkip, const SECTreeCtrl *pTreeControl );
	MAPEDITORLIB_EXPORT void SelectAll( HTREEITEM hParentItem, HTREEITEM hItemToSkip, SECTreeCtrl *pTreeControl );
	MAPEDITORLIB_EXPORT void FillWindowsClipboard( const CHTREEITEMList &rSelection,
														 const std::string &rszClipboardPrefix,
														 const std::string &rszClipboardPostfix,
														 const std::string &rszItemPrefix,
														 const std::string &rszItemPostfix,
														 const std::string &rszTreeItemDivider,
														 const std::string &rszColumnDivider,
														 const SECTreeCtrl *pTreeControl );
	MAPEDITORLIB_EXPORT bool TranslateAccelerators( bool bModal, unsigned nMessage, WPARAM wParam, LPARAM lParam );
};


