#pragma once
#include "../ui/commandparam.h"
#include "../ui/dbuserinterface.h"
#include "../ui/ui.h"
#include "../input/gamemessage.h"
#include "../ui/uifactory.h"
#include "../misc/2darray.h"
#include "../zlib/zconf.h"
#include "../ui/window.h"
#include "UISpecificB2.h"
#include "../UI/Background.h"
#include "DBUISpecificB2.h"


class CWindowSelection : public CWindow, public ISelection
{
	OBJECT_BASIC_METHODS(CWindowSelection)
	CPtr<NDb::SWindowSelection> pInstance;
	bool bSelectorVisible;
	CVec2 vSelectionFirstPoint;

protected:
	virtual NDb::SWindow* GetInstance() { return pInstance; }
	CPtr<CBackground> pSelectorRect;

public:
	void RegisterObservers();

	bool MsgUpdateSelection( const SGameMessage &msg );
	bool MsgStartSelection( const SGameMessage &msg );
	bool MsgEndSelection( const SGameMessage &msg );
	bool MsgCancelSelection( const SGameMessage &msg );

	virtual int operator&( IBinSaver &saver );
	virtual void Visit( struct IUIVisitor *pVisitor );
	virtual void InitByDesc( const struct NDb::SUIDesc *pDesc );
};


