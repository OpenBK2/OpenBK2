#if !defined(__TREE__GDB_BROWSER_BASE__)
#define __TREE__GDB_BROWSER_BASE__


#include "..\MapEditorLib\Tools_MnemonicsCollector.h"

#include "Tree_GDBBrowser_InputState.h"
#include "..\MapEditorLib\Interface_CommandHandler.h"
#include "..\MapEditorLib\DefaultView.h"
#include "..\MapEditorLib\FolderController.h"
#include "SortTreeControl.h"

#define TABGDBB_TREE_COLUMN_COUNT (1)

int CALLBACK TreeGDBBrowserBaseCompareFunc( LPARAM lParam0, LPARAM lParam1, LPARAM lParamSort );

class CGDBOMnemonics;
class CTreeGDBBrowserBase : public CSortTreeControl, public ICommandHandler, public CDefaultView
{
	// Input State
	friend class CTreeGDBBrowserInputState;

public:
	enum EGDBOType
	{
		GDBO_UNKNOWN					= 0,
		GDBO_OBJECT						= 1,
		GDBO_FOLDER						= 2,
		GDBO_COUNT						= 3,
	};
	//
	bool bNeedTranslateAccelerators;
	bool bModal;
	//
	static const UINT		TABGDBB_TREE_COLUMN_NAME  [TABGDBB_TREE_COLUMN_COUNT];
	static const int		TABGDBB_TREE_COLUMN_FORMAT[TABGDBB_TREE_COLUMN_COUNT];
	static const int		TABGDBB_TREE_COLUMN_WIDTH [TABGDBB_TREE_COLUMN_COUNT];

	// Функция сортировки
	friend int CALLBACK TreeGDBBrowserBaseCompareFunc( LPARAM lParam0, LPARAM lParam1, LPARAM lParamSort );
	int SortItemText( const CString &rstrText0, EGDBOType nType0, const CString &rstrText1, EGDBOType nType1 );

private:
	class CGDBOMnemonics : public CMnemonicsCollector<int>
	{
		public:
		CGDBOMnemonics();
		EGDBOType Get( const string &rszGDBOMnemonic );
	};
	//
	struct STreeOperation
	{
		enum EType
		{
			TYPE_INSERT					= 0, // несуществующая команда
			TYPE_REMOVE					= 1, // szDestination - to delete item
			TYPE_COPY						= 2, // szDestinaton - parent item, hSource - to copy item ( with subitems )
			TYPE_RENAME					= 3, // szDestination - parent item, hSource - to move item ( with subitems )
			TYPE_COLOR					= 4, // установить цвет обьекта
			TYPE_CHECK					= 5, // проверить объект на правильное заполнение данных
			TYPE_EXPORT					= 6, // конвертировать объект
			TYPE_EXPORT_FORCE		= 7, // конвертировать объект безусловно
		};
		//
		HTREEITEM hDestination;
		HTREEITEM hSource;
		COLORREF color;
		//
		EType nType;
		bool bExportReferences;
	};
	typedef list<STreeOperation> CTreeOperationList;
	//typedef list<SGDBOperation> SGDBOperationList;
	//
	CTreeGDBBrowserInputState dragAndDropState;

	int nGDBBrowserID;
	bool bCreateControls;
	UINT nPCDialogCommandHandlerID;
	//
	HTREEITEM hLabelEditItem;
	string szItemTextFromBeginLabelEdit;
	CGDBOMnemonics typeMnemonics;
	//
	bool bStrongSelection; // selection in open / resent list operations
	//
	// Таймер дла отложенного считывания полей структур из базы
	LONG nCreateTreeTimer;
	bool bCreateTreeSelectionChanged;
	string szIgnoreSelectionName;
	CPtr<IManipulatorIterator> pCreateTreeManipulatorIterator;
  inline UINT GetCreateTreeTimerID() { return 100; }
  inline UINT GetCreateTreeTimerInterval() { return 100; }	// Частота в миллисекундах
  inline UINT GetCreateTreeTimerCount() { return 1000000; }		// Количество считываний за один раз
  void SetCreateTreeTimer();
  void KillCreateTreeTimer();
  void OnCreateTreeTimer();

  //Таймер для отложенного обновления поля поcле редактирования метки
	LONG nLabelEditSortTimer;
	HTREEITEM hLabelEditSortTimerItem;
  inline UINT GetLabelEditSortTimerID() { return 101; }
  inline UINT GetLabelEditSortTimerInterval() { return 10; }
  void SetLabelEditSortTimer();
  void KillLabelEditSortTimer();
  void OnLabelEditSortTimer();
	//
	HTREEITEM AddTreeItem( HTREEITEM hRootItem, const string &rszAdditionalName, EGDBOType nType, const SIteratorDesc *pDesc );
	//HTREEITEM CopyTreeItems( HTREEITEM hParentItem, HTREEITEM hSourceItem, bool bClearState );
	//void DeleteTreeItems( HTREEITEM hItem );
	//	
	bool IsNotEditLabel();
	//
	void SaveHeaderWidthInternal();
	//
	void GetUniqueName( HTREEITEM hParentItem, const string &rszName, EGDBOType nType, string *pszName );
	HTREEITEM FindName( HTREEITEM hParentItem, const string &rszName, EGDBOType nType, bool bCheckType, HTREEITEM hItemToSkip );
	HTREEITEM FindPlaceToInsert( HTREEITEM hParentItem, const string &rszName, EGDBOType nType );
	void PickTextColors( LvPaintContext* pPC );
	//
	bool ExecuteTreeOperation( const STreeOperation &rTreeOperation );
	bool ExecuteTreeOperations( const CTreeOperationList &rTreeOperationList );
	//
	HTREEITEM CTreeGDBBrowserBase::FindFirstItem( const string &rszSearch, HTREEITEM hStartItem );
	//
	void ShowContextMenu( const CTPoint<int> &rPoint );
	void NewFolder( HTREEITEM hParentItem );
	void New( HTREEITEM hParentItem );

	void NewFolder();
	void NewFolderAtRoot();
	void Cut();
	void Copy();
	void Paste();
	void Rename();
	void Delete();
	void Color();
	void Find();
	void GotoID();
	void LookupReferences();
	//
	void New();
	void NewAtRoot();
	bool CanNew();
	//
	void Check( bool bCheckReferences );
	void Export( bool bForce, bool bExportReferences );
	bool CanExport( bool bForce );

protected:
	afx_msg int OnCreate( LPCREATESTRUCT pCreateStruct );
	afx_msg void OnDestroy();
	afx_msg void OnTimer( UINT nIDEvent );
	//
	afx_msg void OnMouseMove( UINT nFlags, CPoint point );
	afx_msg void OnLButtonDown( UINT nFlags, CPoint point );
	afx_msg void OnLButtonUp( UINT nFlags, CPoint point );
	afx_msg void OnRButtonDown( UINT nFlags, CPoint point );
	afx_msg void OnLButtonDblClk( UINT nFlags, CPoint point );
	//
	afx_msg void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
	afx_msg void OnKeyUp( UINT nChar, UINT nRepCnt, UINT nFlags );
	//
	afx_msg void OnContextMenu( CWnd *pwnd, CPoint point );
	//
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnKillFocus( CWnd* pNewWnd );
	//
	afx_msg void OnBeginLabelEdit( NMHDR *pNotifyStruct, LRESULT *pResult );
	afx_msg void OnEndLabelEdit( NMHDR *pNotifyStruct, LRESULT *pResult );
	afx_msg void OnBeginLabelEditList( NMHDR *pNotifyStruct, LRESULT *pResult );
	afx_msg void OnEndLabelEditList( NMHDR *pNotifyStruct, LRESULT *pResult );
	//
	afx_msg void OnSelChanged( NMHDR *pNotifyStruct, LRESULT *pResult );
	afx_msg void OnItemExpanded( NMHDR* pNMHDR, LRESULT* pResult );
	//
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );
	//
	EGDBOType GetTreeItemType( HTREEITEM hItem );
	
	void InternalUndo( IController* pController ); 
	void InternalRedo( IController* pController ); 
	
	// CSortTreeControl
	HTREEITEM GetTreeItem( const string &rszName );
	void GetClipboardPrefix( string *pszClipboardPrefix ) { if ( pszClipboardPrefix ) ( *pszClipboardPrefix ) = GetObjectSet().szObjectTypeName; }
	bool GetTreeItemName( HTREEITEM hItem, string *pszName );
	bool IsIgnoreCase() { return true; }

public:
	CTreeGDBBrowserBase( bool _bNeedTranslateAccelerators, bool _bModal, int _nGDBBrowserID );
	virtual ~CTreeGDBBrowserBase();

	inline void SetStrongSelection() { bStrongSelection = true; }
	inline bool GetStrongSelection() { bool bSelection = bStrongSelection; bStrongSelection = false; return bSelection; }

	bool IsTreeCreated() { return GetViewManipulator() != 0; }
	void CreateTree();
	// pszName = 0 необходимо самостоятельно получать имя
	void SetTreeItemView( HTREEITEM hItem, const string *pszName );

	// Для работы классов потомков
	bool GetCurrentTreeItemName( string *pszName );
	bool SetCurrentTreeItemName( const string &rszName, bool bUpdateSelection = true );
	//
	void SetPCDialogCommandHandlerID( UINT _nPCDialogCommandHandlerID, bool bUpdate );
	//
	bool GetCurrentObjectSet( SObjectSet *pObjectSet );
	bool GetCurrentSelectionSet( SSelectionSet *pSelectionSet );

	void UpdateSelectionManipulator( bool bUpdate );
	
	// ICommandHandler
	bool HandleCommand( UINT nCommandID, DWORD dwData );
	bool UpdateCommand( UINT nCommandID, bool *pbEnable, bool *pbCheck );

	// Создание Undo Operation
	virtual CFolderController* CreateController() { return CDefaultView::CreateController<CFolderController>( static_cast<CFolderController*>( 0 ) ); }
	// CDefaultView
	void SetViewManipulator( IManipulator* _pViewManipulator, const SObjectSet &rObjectSet, const string &rszTemporaryLabel );
	void Undo( IController* pController );
	void Redo( IController* pController );
	//void Update( IController* pController );
	
	// CTreeGDBBrowserBase
	virtual void Load() = 0;
	virtual bool CanLoad() = 0;
	virtual	bool CanAutoLoadAfterBuildingObject() = 0;
	virtual bool GetLoadContextMenuLabel( string *pszLabel ) = 0;
	virtual bool GetSaveHeaderWidthLabel( string *pszLabel ) = 0;
	virtual void LoadHeaderWidth() = 0;
	virtual void SaveHeaderWidth() = 0;
	DECLARE_MESSAGE_MAP()
};

#endif // !defined(__TREE__GDB_BROWSER_BASE__)

