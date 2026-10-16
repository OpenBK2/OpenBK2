#pragma once
#include "WindowScrollableContainerBase.h"

class CWindowMSButton;

// 1 - level tree.
// parent items are buttons, child items (subitems) are any type of window
class CWindow1LvlTreeControl : public CWindowScrollableContainerBase, public IButtonNotify, public I1LvlTreeControl
{
	OBJECT_BASIC_METHODS( CWindow1LvlTreeControl )

	//{ dynamic data
	struct SItem
	{
		CObj<CWindowMSButton> pItem;
		typedef std::list<CObj<IWindow>> CSubItems;
		CSubItems subitems;
		// Defensive member default; explicit constructor values still take precedence.
		bool bCollapsed = false;

		SItem() {}

		SItem( CWindowMSButton *_pItem )
			: pItem( _pItem )
		{
		}

		int operator&( IBinSaver &saver )
		{
			saver.Add( 1, &pItem );
			saver.Add( 2, &subitems );
			saver.Add( 3, &bCollapsed );
			return 0;
		}
	};

	std::vector<SItem> items;
	//} dinamic data
	CPtr<NDb::SWindow1LvlTreeControl> pInstance;
	CDBPtr<NDb::SWindow1LvlTreeControlShared> pShared;

	SItem *GetContainer( IWindow *pCont );
	void Expand( SItem *pContainer );
	void Collapse( SItem *pContainer );

protected:
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

public:
	IWindow *AddItem() override;
	IWindow *AddSubItem( IWindow *pItem ) override;
	void Init() override;
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;

	// IButtonNotify
	void Released( class CWindow *pWho ) override;

	void Pushed( class CWindow *pWho ) override {}

	void Entered( class CWindow *pWho ) override {}

	void Leaved( class CWindow *pWho ) override {}

	void StateChanged( class CWindow *pWho ) override {}

	int operator&( IBinSaver &saver ) override;
};
