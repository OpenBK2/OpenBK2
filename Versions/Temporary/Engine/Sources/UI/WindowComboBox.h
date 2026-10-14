
#pragma once

#include "UIComponents.h"
#include "Window.h"

class CWindowListItem;
class CWindowMSButton;
class CWindowListCtrl;

class CWindowComboBox : public CWindow,
												public IComboBox,
												public IButtonNotify,
												public ISelectNotify,
												public IClickNotify,
												public IFocusNotify
{
	OBJECT_BASIC_METHODS( CWindowComboBox );

	typedef std::vector<CPtr<IListControlItem>> CItems;

	ZDATA_( CWindow )
	CDBPtr<NDb::SWindowComboBoxShared> pShared;
	CPtr<NDb::SWindowComboBox> pInstance;
	CObj<CWindowListItem> pLine;
	CObj<CWindowMSButton> pIcon;
	CObj<CWindowListCtrl> pList;
	CTPoint<int> vListPos;
	CPtr<IDataViewer> pViewer;
	CItems items;
	int nSelected;
	CPtr<CObjectBase> pLineData;
	int nMaxVisibleRows; // use 0 for fixed list size
	int nListBaseHeight;
	int nRowHeight;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CWindow *)this );
		f.Add( 2, &pShared );
		f.Add( 3, &pInstance );
		f.Add( 4, &pLine );
		f.Add( 5, &pIcon );
		f.Add( 6, &pList );
		f.Add( 7, &vListPos );
		f.Add( 8, &pViewer );
		f.Add( 9, &items );
		f.Add( 10, &nSelected );
		f.Add( 11, &pLineData );
		f.Add( 12, &nMaxVisibleRows );
		f.Add( 13, &nListBaseHeight );
		f.Add( 14, &nRowHeight );
		return 0;
	}

	bool bSuppressPopupList;

private:
	void ResizeList();

protected:
	//{ CWindow
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

	bool IsRelatedFocus( IWindow *pWindow ) const override;
	//}

	bool OnEscape( const SGameMessage &msg );

	// Returns true for visible "modal" drop-down list, false otherwise
	bool IsModalList() const;
	void ShowList( bool bShow );

public:
	CWindowComboBox();

	//{ CWindow
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;

	void Init() override;

	void Reposition( const CTRect<float> &parentRect ) override;

	bool OnButtonDown( const CVec2 &vPos, const int nButton ) override;
	//}

	//{ IComboBox
	void AddItem( CObjectBase *pData ) override;
	void RemoveItem( CObjectBase *pData ) override;
	void RemoveAllItems() override;

	int GetItemCount() const override
	{
		return items.size();
	}

	IListControlItem *GetItem( int nIndex ) const override;

	int GetSelectedIndex() const override
	{
		return nSelected;
	}

	void Select( int nIndex ) override;

	void SetViewer( IDataViewer *pViewer ) override;
	void SetLine( CObjectBase *pData ) override;

	//}

	//{ IButtonNotify
	void Released( class CWindow *pWho ) override {}

	void Pushed( class CWindow *pWho ) override;

	void Entered( class CWindow *pWho ) override {}

	void Leaved( class CWindow *pWho ) override {}

	void StateChanged( class CWindow *pWho ) override {}

	//}

	//{ ISelectNotify
	void OnSelectData( CObjectBase *pData ) override;
	//}

	//{ IClickNotify
	void Clicked( struct IWindow *pWho, int nButton ) override;

	void DoubleClicked( struct IWindow *pWho, int nButton ) override {}

	//}

	//{ IFocusNotify
	void OnFocus( const bool bFocus ) override;
	//}
};
