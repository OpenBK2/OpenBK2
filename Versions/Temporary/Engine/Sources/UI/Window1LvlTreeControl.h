#pragma once
#include "windowscrollablecontainerbase.h"



class CWindowMSButton;

// 1 - level tree.
// parent items are buttons, child items (subitems) are any type of window
class CWindow1LvlTreeControl : public CWindowScrollableContainerBase, public IButtonNotify, public I1LvlTreeControl
{
	OBJECT_BASIC_METHODS(CWindow1LvlTreeControl)

	//{ dynamic data
	struct SItem
	{
		CObj<CWindowMSButton> pItem;
		typedef list<CObj<IWindow> > CSubItems;
		CSubItems subitems;
		bool bCollapsed;
		SItem() { }
		SItem( CWindowMSButton *_pItem ) : pItem( _pItem ) {  }

		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &pItem );
			saver.Add( 2, &subitems );
			saver.Add( 3, &bCollapsed );
			return 0;
		}
	};
	vector<SItem> items;
	//} dinamic data
	CPtr<NDb::SWindow1LvlTreeControl> pInstance;
	CDBPtr<NDb::SWindow1LvlTreeControlShared> pShared;

	SItem *GetContainer( IWindow* pCont );
	void Expand( SItem* pContainer );
	void Collapse( SItem* pContainer );
protected:
	NDb::SWindow* GetInstance() { return pInstance; }

public:
	IWindow* AddItem();
	IWindow* AddSubItem( IWindow *pItem );
	void Init();
	void InitByDesc( const struct NDb::SUIDesc *pDesc );
	
	// IButtonNotify
	void Released( class CWindow *pWho );
	void Pushed( class CWindow *pWho ) {  }
	void Entered( class CWindow *pWho ) {  }
	void Leaved( class CWindow *pWho ) {  }
	void StateChanged( class CWindow *pWho ) {  }

	int operator&( IBinSaver &saver );
};

