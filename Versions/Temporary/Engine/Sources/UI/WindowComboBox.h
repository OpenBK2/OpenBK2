#ifndef __WINDOW_COMBO_BOX_H__
#define __WINDOW_COMBO_BOX_H__

#pragma ONCE

#include "Window.h"
#include "UIComponents.h"

class CWindowListItem;
class CWindowMSButton;
class CWindowListCtrl;

class CWindowComboBox : public CWindow, public IComboBox, 
	public IButtonNotify, public ISelectNotify, public IClickNotify, public IFocusNotify
{
	OBJECT_BASIC_METHODS(CWindowComboBox);

	typedef vector< CPtr<IListControlItem> > CItems;
	
	ZDATA_(CWindow)
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
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CWindow*)this); f.Add(2,&pShared); f.Add(3,&pInstance); f.Add(4,&pLine); f.Add(5,&pIcon); f.Add(6,&pList); f.Add(7,&vListPos); f.Add(8,&pViewer); f.Add(9,&items); f.Add(10,&nSelected); f.Add(11,&pLineData); f.Add(12,&nMaxVisibleRows); f.Add(13,&nListBaseHeight); f.Add(14,&nRowHeight); return 0; }
	
	bool bSuppressPopupList;
private:
	void ResizeList();
protected:
	//{ CWindow
	NDb::SWindow* GetInstance() { return pInstance; }
	bool IsRelatedFocus( IWindow *pWindow ) const;
	//}

	bool OnEscape( const SGameMessage &msg );
	
	// Returns true for visible "modal" drop-down list, false otherwise
	bool IsModalList() const;
	void ShowList( bool bShow );
public:
	CWindowComboBox();

	//{ CWindow
	void InitByDesc( const struct NDb::SUIDesc *pDesc );
	
	void Init();
	
	void Reposition( const CTRect<float> &parentRect );

	bool OnButtonDown( const CVec2 &vPos, const int nButton );
	//}

	//{ IComboBox
	void AddItem( CObjectBase *pData );
	void RemoveItem( CObjectBase *pData );
	void RemoveAllItems();
	int GetItemCount() const { return items.size(); }
	IListControlItem* GetItem( int nIndex ) const;
	int GetSelectedIndex() const { return nSelected; }
	void Select( int nIndex );

	void SetViewer( IDataViewer *pViewer );
	void SetLine( CObjectBase *pData );
	//}

	//{ IButtonNotify
	void Released( class CWindow *pWho ) {}
	void Pushed( class CWindow *pWho );
	void Entered( class CWindow *pWho ) {}
	void Leaved( class CWindow *pWho ) {}
	void StateChanged( class CWindow *pWho ) {}
	//}

	//{ ISelectNotify
	void OnSelectData( CObjectBase *pData );
	//}

	//{ IClickNotify
	void Clicked( interface IWindow *pWho, int nButton );
	void DoubleClicked( interface IWindow *pWho, int nButton ) {}
	//}

	//{ IFocusNotify
	void OnFocus( const bool bFocus );
	//}
};

#endif //__WINDOW_COMBO_BOX_H__
