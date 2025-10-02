#pragma once

#include "Misc/HashFuncs.h"

class CWindowMSButton;
struct IWindow;

class CButtonGroup : public CObjectBase
{
	OBJECT_BASIC_METHODS( CButtonGroup )

	typedef hash_set< CPtr<CWindowMSButton>, SDefaultPtrHash > CButtons;
	CButtons buttons;
	CPtr<IWindow> pPressed;

	IWindow * ChooseDefault();
public:
	// button must notify its group before state changing.
	// must not change state if false is returned.
	bool TrySwitchState( IWindow *pButton );
	void Add( IWindow * pButton );
	void Init();
	void SetActive( IWindow *pButton );
	void Remove_All();
	IWindow* GetPressed() const; 
	IWindow* GetButton( int i );
};


