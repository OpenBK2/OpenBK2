#if !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_MAIN_TREE_CONTROL__)
#define __COMMON_CONTROLS__PROPERTY_CONTROL_MAIN_TREE_CONTROL__
#pragma once

#include "PC_ItemEditor.h"
#include "..\MapEditorLib\Interface_CommandHandler.h"
#include "..\MapEditorLib\DefaultView.h"
#include "..\MapEditorLib\ObjectController.h"
#include "SortTreeControl.h"

// Можно вставлять только в диалоги CDialog и в его производные
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CALLBACK PCMainTreeControlCompareFunc( LPARAM lParam0, LPARAM lParam1, LPARAM lParamSort );
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CPCMainTreeControl : public CSortTreeControl, public ICommandHandler, public CDefaultView
{
	enum ENewElementExpandMode
	{
		NEEM_USER_DEFINED			= 0,
		NEEM_ALWAYS_EXPAND		= 1,
		MEEM_ALWAYS_COLLAPSE	= 2,
	};
	bool bNeedTranslateAccelerators;
	bool bModal;
	//
	void CopySelection();
	void PasteSelection();
	//
	// Функция сортировки
	friend int CALLBACK PCMainTreeControlCompareFunc( LPARAM lParam0, LPARAM lParam1, LPARAM lParamSort );
	int SortItemText( const CString &rstrText0, const CString &rstrText1 );
	//
	bool bCreateControls;
	bool bShowHidden;
	string szGUID;
	//
	CWnd* pwndStatusStringWindow;
	CPtr<IPCItemEditor> pActiveItemEditor;
	CPtr<IPCItemEditor> pMultilineStringEditor;
	//
	// Таймер дла отложенного считывания полей структур из базы
	LONG nCreateTreeTimer;
	bool bCreateTree;
	bool bAsync;
	CPtr<IManipulatorIterator> pCreateTreeManipulatorIterator;
	string szCreateTreeParentName;
	ENewElementExpandMode newElementExpandMode;
	HTREEITEM hCreateTreeParentItem;
  inline UINT GetCreateTreeTimerID() { return 200; }
  inline UINT GetCreateTreeTimerInterval() { return 20; }	// Частота в миллисекундах
	inline UINT GetCreateTreeTimerCount() { return ( bShowHidden ? 100 : 20 ); }		// Количество считываний за один раз
  void SetCreateTreeTimer();
  void KillCreateTreeTimer();
  void OnCreateTreeTimer();
	//
	// hArrayItem - куда вставлять
	// hItem - после которого вставлять (TVI_FIRST для вставки на первое место, TVI_LAST для добавления элемента в конец)
	// nNewIndex - новый ( реальный индекс элемента, не (-1) )
	bool InsertPCNode( HTREEITEM hArrayItem, HTREEITEM hItem, int nNewIndex );
	// hArrayItem - из которого удалять
	// hItem - который удалять (0 для удаления всего)
	// nDeleteIndex - индекс для удаления (при удалении всего игнорируется)
	bool DeletePCNode( HTREEITEM hArrayItem, HTREEITEM hItem, int nDeleteIndex );
	//
	void ExpandPCItem();
	void ExpandAllPCItems();
	void CollapsePCItem();
	void CollapseAllPCItems();
	void SetOptimalWidth();
	void Refresh();
	void AddNode();
	void DeleteAllNodes();
	void InsertNode();
	void DeleteNode();
	void ShowHidden();
	void SelectAll();
	//
	EPCIEType GetTreeItemType( HTREEITEM hItem );
	bool GetTreeItemEditorPlace( HTREEITEM hItem, CTRect<int> *pRect );
	//
	HTREEITEM AddTreeItem( const string &rszName, EPCIEType nType, const SPropertyDesc *pDesc );
	bool ItemMustBeExpand( HTREEITEM hItem );
	HTREEITEM AddTreeItemInternal( HTREEITEM hRootItem, const string &rszAdditionalName, EPCIEType nType, const SPropertyDesc *pDesc );
	//
	void SetPCItemView( HTREEITEM hItem, const SPropertyDesc *pDesc );
	IPCItemEditor* CreatePCItemEditor( HTREEITEM hItem );
	bool UpdateValueFromPCItemEditor( IPCItemEditor *pPCItemEditor );
	void UpdatePCItemEditorPosition( HTREEITEM hItem );
	void ClosePCItemEditor( bool bAcceptChanges );
	//
	bool ForceRelativeParam_ReadOnly( HTREEITEM hItem, bool bDefaultValue );
	string ForceRelativeParam_StringParam( HTREEITEM hItem, const string &rszDefaultValue );
	int ForceRelativeParam_IntParam( HTREEITEM hItem, int nDefaultValue );
	//
	void PickTextColors( LvPaintContext* pPC );
	//
	void UpdateMultilineStringEditor();
	void UpdateStatusStringWindow();
	//
	bool GetValue( const string &rszName, CVariant *pVariant );
	bool AddChangeOperation( const string &rszName, const CVariant &rValue, CObjectBaseController *pObjectController );

protected:
	afx_msg void OnDestroy();
	afx_msg void OnTimer( UINT nIDEvent );
	//
	afx_msg void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg void OnVScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
	afx_msg BOOL OnMouseWheel( UINT nFlags, short zDelta, CPoint pt );
	//
	afx_msg void OnLButtonDown( UINT nFlags, CPoint point );
	afx_msg void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
	//
	afx_msg void OnSize( UINT nType, int cx, int cy );
	//
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnKillFocus( CWnd* pNewWnd );
	//
	afx_msg void OnContextMenu( CWnd *pwnd, CPoint point );
	//
	afx_msg LRESULT OnMessagePCItemChange( WPARAM wParam, LPARAM lParam );
	afx_msg void OnSelchangedTree( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnItemExpanded( NMHDR* pNMHDR, LRESULT* pResult );
	//	
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );
	//void OnPCItemChange( WPARAM wParam, LPARAM lParam );

	// CSortTreeControl
	HTREEITEM GetTreeItem( const string &rszName );
	void GetClipboardPrefix( string *pszClipboardPrefix ) {}
	bool GetTreeItemName( HTREEITEM hItem, string *pszName );
	bool IsIgnoreCase() { return false; }

public:
	void EnableEdit( bool bEnable );

	CPCMainTreeControl( bool bNeedTranslateAccelerators, bool bModal );
	~CPCMainTreeControl();
	//
	void CreateTree( HTREEITEM hParentItem, bool _bCreateTree, bool bAsync );
	bool SelectPCItem( const string &rszItemName );
	//
	bool GetSelectedPCItemName( string *pszItemName );
	bool GetSelectedPCItemDescription( string *pszItemName );
	//
	bool UpdateValue( const string &rszItemName );
	//
	void SetMultilineStringEditor( IPCItemEditor* _pMultilineStringEditor );
	IPCItemEditor* GetMultilineStringEditor() { return pMultilineStringEditor; }
	//
	void SetStatusStringWindow( CWnd* _pwndStatusStringWindow );
	CWnd* GetStatusStringWindow() { return pwndStatusStringWindow; }
	//
	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );
	
	// Создание Undo Operation
	virtual CObjectController* CreateController() { return CDefaultView::CreateController<CObjectController>( static_cast<CObjectController*>( 0 ) ); }
	// CDefaultView
	void SetViewManipulator( IManipulator* _pViewManipulator, const SObjectSet &rObjectSet, const string &rszTemporaryLabel );
	void Undo( IController* pController ); 
	void Redo( IController* pController ); 

	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // !defined(__COMMON_CONTROLS__PROPERTY_CONTROL_MAIN_TREE_CONTROL__)
