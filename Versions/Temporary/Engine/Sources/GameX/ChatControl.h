#pragma once

#include "UI/UI.h"

// ChatControlWrapper - a wrapper for a ScrollableContainer (of a specific form) that makes it act like a chat

class CChatControlWrapper : public CObjectBase
{
	OBJECT_BASIC_METHODS( CChatControlWrapper );

	ZDATA
	CPtr<IScrollableContainer> pList;
	CPtr<IWindow> pItem;
	int nMaxItems;
	int nItems;
	std::list< CPtr<IWindow> > items;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pList); f.Add(3,&pItem); f.Add(4,&nMaxItems); f.Add(5,&nItems); f.Add(6,&items); return 0; }
private:

public:
	CChatControlWrapper() : nMaxItems(1), nItems(0) {}
	CChatControlWrapper( IScrollableContainer *_pList, int _nMaxItems );

	void AddItem( const std::wstring &wszText );
};
