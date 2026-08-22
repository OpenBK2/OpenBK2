#pragma once

#include "WindowScrollableContainerBase.h"
#include "UIComponents.h"

class CWindowMSButton;
class CWindowSimple;

// CWindowListHeader

struct IHeaderNotify : public virtual CObjectBase
{
	virtual void ColumnsResized( const std::vector<int> &sizes ) = 0;
	virtual void ColumnResort( IWindowSorter *pSorter ) = 0;
	virtual void SetOptimalWidth( const int nColumn ) = 0;
};

class CWindowListHeader : public CWindow, public IButtonNotify
{
	OBJECT_BASIC_METHODS(CWindowListHeader)
	
	CPtr<NDb::SWindowListHeader> pInstance;
	CDBPtr<NDb::SWindowListHeaderShared> pShared;
	std::vector< CPtr<IWindowSorter> > sorters;
	int nSelectedColumn;										// sorting is performed by this column
	int bResizable;
	
	int nResizingIndex;											// this column is currently being resized

	CObj<CWindow> pSortIconDown;			// is being added to currently selected column
	CObj<CWindow> pSortIconUp;			// is being added to currently selected column
	
	CPtr<IHeaderNotify> pHeaderNotify;
	
	std::vector<int> columnSizes;
	std::vector< CPtr<IWindow> > subitems;

	void AdjustColums();
	void ResizeColumn( const int nPos, const int nResizingIndex );
	// return column index and bool - is that column will be restored
	// from width 0 ( to display different cursors )
	std::pair<int,bool> GetResizeColumnIndex( const CVec2 &vPos ) const;
	void UpdateSortButton();
protected:
	NDb::SWindow* GetInstance() { return pInstance; }
public:
	CWindowListHeader() : bResizable( false ), nResizingIndex( -1 ), nSelectedColumn( -1 ), pHeaderNotify( 0 ) {  }
	int operator&( IBinSaver &saver );
	void Reposition( const CTRect<float> &parentRect );
	void Init();
	void AfterLoad();

	void SetNotify( IHeaderNotify *_pHeaderNotify ) { pHeaderNotify = _pHeaderNotify; }
	void InitByDesc( const struct NDb::SUIDesc *pDesc );
	void MakeInterior();
	const std::vector<int>& GetSizes() const
	{
		return columnSizes;
	}
	const int GetNColumns() const  { return subitems.size(); }
	void Resort( const int nColumn, const bool bAscending );
	void SetSorter( IWindowSorter *pSorter, const int nColumn ) { sorters[nColumn] = pSorter; }
	
	void SetColumnSize( const int nColumn, const int nWidth );

	//{ IButtonNotify
	void Released( class CWindow *pWho );
	void Pushed( class CWindow *pWho ) { }
	void Entered( class CWindow *pWho ) {  }
	void Leaved( class CWindow *pWho ) { }
	void StateChanged( class CWindow *pWho ) {  }
	//}
	bool OnMouseMove( const CVec2 &vPos, const int nButton );
	bool OnButtonDown( const CVec2 &vPos, const int nButton );
	bool OnButtonUp( const CVec2 &vPos, const int nButton );
	bool OnButtonDblClk( const CVec2 &vPos, const int nButton );
};

// CWindowListItem 

class CWindowListItem : public CWindow, public IListControlItem
{
	OBJECT_BASIC_METHODS(CWindowListItem)
	
	CPtr<NDb::SWindowListItem> pInstance;
	CDBPtr<NDb::SWindowListItemShared> pShared;
	
	std::vector< CPtr<IWindow> > subitems;
	CPtr< CObjectBase > pUserData;
protected:
	virtual NDb::SWindow* GetInstance() { return pInstance; }
public:
	virtual int operator&( IBinSaver &saver );
	void MakeInterior( const int nColumns );
	void SetColumnSizes( const std::vector<int> &sizes );
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );

	//{ struct IListControlItem
	IWindow * GetSubItem( const int nSubItem );
	
	CObjectBase* GetUserData() const { return pUserData; }
	void SetUserData( CObjectBase *pData ) { pUserData = pData; }
	//}
};


//	CWindowListCtrl


class CWindowListCtrl :	public CWindowScrollableContainerBase, public IListControl, public IHeaderNotify
{
	OBJECT_BASIC_METHODS(CWindowListCtrl)

	CPtr<NDb::SWindowListCtrl> pInstance;
	CDBPtr<NDb::SWindowListSharedData> pShared;
	CObj<CWindowListHeader> pHeader;
	
	typedef std::list< CObj<IWindow> > CItems;
	CItems items;

	int nSortColumn;
	bool bSelectable;

	CPtr<IDataViewer> pViewer;
	CPtr<ISelectNotify> pSelectNotify;
	CPtr<IFocusNotify> pFocusNotify;
private:
	IListControlItem* AddItemInternal();
protected:
	virtual NDb::SWindow* GetInstance() { return pInstance; }
	
	//{ CWindowScrollableContainerBase
	void Select( IWindow *pElement );
	//}
public:
	CWindowListCtrl() : nSortColumn( 0 ), bSelectable( true ) {  }
	//{ CWindow
	virtual int operator&( IBinSaver &saver );
	virtual void Reposition( const CTRect<float> &parentRect );
	virtual void Init();
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );
	void AfterLoad();
	void SetFocus( const bool bFocus );
	//}

	//{ IListControl
	IListControlItem* AddItem();
	IListControlItem* AddItem( CObjectBase *pData );
	void RemoveItem( IListControlItem *pItem );

	void Resort( const int nColumn = -1, const bool bAscending = true );
	void SetSorter( IWindowSorter *pSorter, const int nColumn );
	
	void SetViewer( IDataViewer *pViewer );
	void RemoveAllElements();

	const int GetNColumns() const  { return pHeader->GetNColumns(); }
	int GetItemCount() const;
	IListControlItem* GetSelectedListItem() const;
	void SelectItem( CObjectBase *pData );

	void Update() { CWindowScrollableContainerBase::Update(); }
	//}
	
	//{ IHeaderNotify
	virtual void ColumnsResized( const std::vector<int> &sizes );
	virtual void ColumnResort( IWindowSorter *pSorter );
	virtual void SetOptimalWidth( const int nColumn );
	//}
	
	void SetSelectNotify( ISelectNotify *_pNotify ) { pSelectNotify = _pNotify; }
	void SetFocusNotify( IFocusNotify *_pNotify ) { pFocusNotify = _pNotify; }

	int GetRowHeight() const;
};


