#pragma once

#include "MapEditorLib/Tools_MnemonicsCollector.h"

#include "Tree_GDBBrowser_InputState.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/DefaultView.h"
#include "MapEditorLib/FolderController.h"
#include "SortTreeControl.h"

#include <cstdint>

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
	static const unsigned		TABGDBB_TREE_COLUMN_NAME  [TABGDBB_TREE_COLUMN_COUNT];
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
		EGDBOType Get( const std::string &rszGDBOMnemonic );
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
	typedef std::list<STreeOperation> CTreeOperationList;
	//typedef std::list<SGDBOperation> SGDBOperationList;
	//
	CTreeGDBBrowserInputState dragAndDropState;

	int nGDBBrowserID;
	bool bCreateControls;
	unsigned nPCDialogCommandHandlerID;
	//
	HTREEITEM hLabelEditItem;
	std::string szItemTextFromBeginLabelEdit;
	CGDBOMnemonics typeMnemonics;
	//
	bool bStrongSelection; // selection in open / resent list operations
	//
	// Таймер дла отложенного считывания полей структур из базы
	int32_t nCreateTreeTimer;
	bool bCreateTreeSelectionChanged;
	std::string szIgnoreSelectionName;
	CPtr<IManipulatorIterator> pCreateTreeManipulatorIterator;
  inline unsigned GetCreateTreeTimerID() { return 100; }
  inline unsigned GetCreateTreeTimerInterval() { return 100; }	// Частота в миллисекундах
  inline unsigned GetCreateTreeTimerCount() { return 1000000; }		// Количество считываний за один раз
  void SetCreateTreeTimer();
  void KillCreateTreeTimer();
  void OnCreateTreeTimer();

  //Таймер для отложенного обновления поля поcле редактирования метки
	int32_t nLabelEditSortTimer;
	HTREEITEM hLabelEditSortTimerItem;
  inline unsigned GetLabelEditSortTimerID() { return 101; }
  inline unsigned GetLabelEditSortTimerInterval() { return 10; }
  void SetLabelEditSortTimer();
  void KillLabelEditSortTimer();
  void OnLabelEditSortTimer();
	//
	HTREEITEM AddTreeItem( HTREEITEM hRootItem, const std::string &rszAdditionalName, EGDBOType nType, const SIteratorDesc *pDesc );
	//HTREEITEM CopyTreeItems( HTREEITEM hParentItem, HTREEITEM hSourceItem, bool bClearState );
	//void DeleteTreeItems( HTREEITEM hItem );
	//	
	bool IsNotEditLabel();
	//
	void SaveHeaderWidthInternal();
	//
	void GetUniqueName( HTREEITEM hParentItem, const std::string &rszName, EGDBOType nType, std::string *pszName );
	HTREEITEM FindName( HTREEITEM hParentItem, const std::string &rszName, EGDBOType nType, bool bCheckType, HTREEITEM hItemToSkip );
	HTREEITEM FindPlaceToInsert( HTREEITEM hParentItem, const std::string &rszName, EGDBOType nType );
	void PickTextColors( LvPaintContext* pPC );
	//
	bool ExecuteTreeOperation( const STreeOperation &rTreeOperation );
	bool ExecuteTreeOperations( const CTreeOperationList &rTreeOperationList );
	//
	HTREEITEM FindFirstItem( const std::string &rszSearch, HTREEITEM hStartItem );
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
	afx_msg void OnTimer( UINT_PTR nIDEvent );
	//
	afx_msg void OnMouseMove( unsigned nFlags, CPoint point );
	afx_msg void OnLButtonDown( unsigned nFlags, CPoint point );
	afx_msg void OnLButtonUp( unsigned nFlags, CPoint point );
	afx_msg void OnRButtonDown( unsigned nFlags, CPoint point );
	afx_msg void OnLButtonDblClk( unsigned nFlags, CPoint point );
	//
	afx_msg void OnKeyDown( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
	afx_msg void OnKeyUp( unsigned nChar, unsigned nRepCnt, unsigned nFlags );
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
	virtual LRESULT WindowProc( unsigned message, WPARAM wParam, LPARAM lParam );
	//
	EGDBOType GetTreeItemType( HTREEITEM hItem );
	
	void InternalUndo( IController* pController ); 
	void InternalRedo( IController* pController ); 
	
	// CSortTreeControl
	HTREEITEM GetTreeItem( const std::string &rszName );
	void GetClipboardPrefix( std::string *pszClipboardPrefix ) { if ( pszClipboardPrefix ) ( *pszClipboardPrefix ) = GetObjectSet().szObjectTypeName; }
	bool GetTreeItemName( HTREEITEM hItem, std::string *pszName );
	bool IsIgnoreCase() { return true; }

public:
	CTreeGDBBrowserBase( bool _bNeedTranslateAccelerators, bool _bModal, int _nGDBBrowserID );
	virtual ~CTreeGDBBrowserBase();

	inline void SetStrongSelection() { bStrongSelection = true; }
	inline bool GetStrongSelection() { bool bSelection = bStrongSelection; bStrongSelection = false; return bSelection; }

	bool IsTreeCreated() { return GetViewManipulator() != 0; }
	void CreateTree();
	// pszName = 0 необходимо самостоятельно получать имя
	void SetTreeItemView( HTREEITEM hItem, const std::string *pszName );

	// Для работы классов потомков
	bool GetCurrentTreeItemName( std::string *pszName );
	bool SetCurrentTreeItemName( const std::string &rszName, bool bUpdateSelection = true );
	//
	void SetPCDialogCommandHandlerID( unsigned _nPCDialogCommandHandlerID, bool bUpdate );
	//
	bool GetCurrentObjectSet( SObjectSet *pObjectSet );
	bool GetCurrentSelectionSet( SSelectionSet *pSelectionSet );

	void UpdateSelectionManipulator( bool bUpdate );
	
	// ICommandHandler
	bool HandleCommand( unsigned nCommandID, uint32_t dwData );
	bool UpdateCommand( unsigned nCommandID, bool *pbEnable, bool *pbCheck );

	// Создание Undo Operation
	virtual CFolderController* CreateController() { return CDefaultView::CreateController<CFolderController>( static_cast<CFolderController*>( 0 ) ); }
	// CDefaultView
	void SetViewManipulator( IManipulator* _pViewManipulator, const SObjectSet &rObjectSet, const std::string &rszTemporaryLabel );
	void Undo( IController* pController );
	void Redo( IController* pController );
	//void Update( IController* pController );
	
	// CTreeGDBBrowserBase
	virtual void Load() = 0;
	virtual bool CanLoad() = 0;
	virtual	bool CanAutoLoadAfterBuildingObject() = 0;
	virtual bool GetLoadContextMenuLabel( std::string *pszLabel ) = 0;
	virtual bool GetSaveHeaderWidthLabel( std::string *pszLabel ) = 0;
	virtual void LoadHeaderWidth() = 0;
	virtual void SaveHeaderWidth() = 0;
	DECLARE_MESSAGE_MAP()
};


