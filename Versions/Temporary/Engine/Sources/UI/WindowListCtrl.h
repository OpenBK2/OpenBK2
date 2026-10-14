#pragma once

#include "UIComponents.h"
#include "WindowScrollableContainerBase.h"

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
	OBJECT_BASIC_METHODS( CWindowListHeader )

	CPtr<NDb::SWindowListHeader> pInstance;
	CDBPtr<NDb::SWindowListHeaderShared> pShared;
	std::vector<CPtr<IWindowSorter>> sorters;
	int nSelectedColumn; // sorting is performed by this column
	int bResizable;

	int nResizingIndex; // this column is currently being resized

	CObj<CWindow> pSortIconDown; // is being added to currently selected column
	CObj<CWindow> pSortIconUp;   // is being added to currently selected column

	CPtr<IHeaderNotify> pHeaderNotify;

	std::vector<int> columnSizes;
	std::vector<CPtr<IWindow>> subitems;

	void AdjustColums();
	void ResizeColumn( const int nPos, const int nResizingIndex );
	// return column index and bool - is that column will be restored
	// from width 0 ( to display different cursors )
	std::pair<int, bool> GetResizeColumnIndex( const CVec2 &vPos ) const;
	void UpdateSortButton();

protected:
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

public:
	CWindowListHeader()
		: bResizable( false )
		, nResizingIndex( -1 )
		, nSelectedColumn( -1 )
		, pHeaderNotify( 0 )
	{
	}

	int operator&( IBinSaver &saver ) override;
	void Reposition( const CTRect<float> &parentRect ) override;
	void Init() override;
	void AfterLoad() override;

	void SetNotify( IHeaderNotify *_pHeaderNotify )
	{
		pHeaderNotify = _pHeaderNotify;
	}

	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;
	void MakeInterior();

	const std::vector<int> &GetSizes() const
	{
		return columnSizes;
	}

	const int GetNColumns() const
	{
		return subitems.size();
	}

	void Resort( const int nColumn, const bool bAscending );

	void SetSorter( IWindowSorter *pSorter, const int nColumn )
	{
		sorters[nColumn] = pSorter;
	}

	void SetColumnSize( const int nColumn, const int nWidth );

	//{ IButtonNotify
	void Released( class CWindow *pWho ) override;

	void Pushed( class CWindow *pWho ) override {}

	void Entered( class CWindow *pWho ) override {}

	void Leaved( class CWindow *pWho ) override {}

	void StateChanged( class CWindow *pWho ) override {}

	//}
	bool OnMouseMove( const CVec2 &vPos, const int nButton ) override;
	bool OnButtonDown( const CVec2 &vPos, const int nButton ) override;
	bool OnButtonUp( const CVec2 &vPos, const int nButton ) override;
	bool OnButtonDblClk( const CVec2 &vPos, const int nButton ) override;
};

// CWindowListItem

class CWindowListItem : public CWindow, public IListControlItem
{
	OBJECT_BASIC_METHODS( CWindowListItem )

	CPtr<NDb::SWindowListItem> pInstance;
	CDBPtr<NDb::SWindowListItemShared> pShared;

	std::vector<CPtr<IWindow>> subitems;
	CPtr<CObjectBase> pUserData;

protected:
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

public:
	int operator&( IBinSaver &saver ) override;
	void MakeInterior( const int nColumns );
	void SetColumnSizes( const std::vector<int> &sizes );
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;

	//{ struct IListControlItem
	IWindow *GetSubItem( const int nSubItem ) override;

	CObjectBase *GetUserData() const override
	{
		return pUserData;
	}

	void SetUserData( CObjectBase *pData ) override
	{
		pUserData = pData;
	}

	//}
};

//	CWindowListCtrl

class CWindowListCtrl : public CWindowScrollableContainerBase, public IListControl, public IHeaderNotify
{
	OBJECT_BASIC_METHODS( CWindowListCtrl )

	CPtr<NDb::SWindowListCtrl> pInstance;
	CDBPtr<NDb::SWindowListSharedData> pShared;
	CObj<CWindowListHeader> pHeader;

	typedef std::list<CObj<IWindow>> CItems;
	CItems items;

	int nSortColumn;
	bool bSelectable;

	CPtr<IDataViewer> pViewer;
	CPtr<ISelectNotify> pSelectNotify;
	CPtr<IFocusNotify> pFocusNotify;

private:
	IListControlItem *AddItemInternal();

protected:
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

	//{ CWindowScrollableContainerBase
	void Select( IWindow *pElement ) override;
	//}
public:
	CWindowListCtrl()
		: nSortColumn( 0 )
		, bSelectable( true )
	{
	}

	//{ CWindow
	int operator&( IBinSaver &saver ) override;
	void Reposition( const CTRect<float> &parentRect ) override;
	void Init() override;
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;
	void AfterLoad() override;
	void SetFocus( const bool bFocus ) override;
	//}

	//{ IListControl
	IListControlItem *AddItem() override;
	IListControlItem *AddItem( CObjectBase *pData ) override;
	void RemoveItem( IListControlItem *pItem ) override;

	void Resort( const int nColumn = -1, const bool bAscending = true ) override;
	void SetSorter( IWindowSorter *pSorter, const int nColumn ) override;

	void SetViewer( IDataViewer *pViewer ) override;
	void RemoveAllElements() override;

	const int GetNColumns() const override
	{
		return pHeader->GetNColumns();
	}

	int GetItemCount() const override;
	IListControlItem *GetSelectedListItem() const override;
	void SelectItem( CObjectBase *pData ) override;

	void Update() override
	{
		CWindowScrollableContainerBase::Update();
	}

	//}

	//{ IHeaderNotify
	void ColumnsResized( const std::vector<int> &sizes ) override;
	void ColumnResort( IWindowSorter *pSorter ) override;
	void SetOptimalWidth( const int nColumn ) override;

	//}

	void SetSelectNotify( ISelectNotify *_pNotify )
	{
		pSelectNotify = _pNotify;
	}

	void SetFocusNotify( IFocusNotify *_pNotify )
	{
		pFocusNotify = _pNotify;
	}

	int GetRowHeight() const;
};
