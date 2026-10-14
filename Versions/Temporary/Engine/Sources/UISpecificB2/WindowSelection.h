#pragma once
#include "DBUISpecificB2.h"
#include "Input/GameMessage.h"
#include "Misc/2Darray.h"
#include "UI/Background.h"
#include "UI/CommandParam.h"
#include "UI/DBUserInterface.h"
#include "UI/UI.h"
#include "UI/UIFactory.h"
#include "UI/Window.h"
#include "UISpecificB2.h"

#include <zconf.h>

class CWindowSelection : public CWindow, public ISelection
{
	OBJECT_BASIC_METHODS( CWindowSelection )
	CPtr<NDb::SWindowSelection> pInstance;
	bool bSelectorVisible;
	CVec2 vSelectionFirstPoint;

protected:
	NDb::SWindow *GetInstance() override
	{
		return pInstance;
	}

	CPtr<CBackground> pSelectorRect;

public:
	void RegisterObservers() override;

	bool MsgUpdateSelection( const SGameMessage &msg );
	bool MsgStartSelection( const SGameMessage &msg );
	bool MsgEndSelection( const SGameMessage &msg );
	bool MsgCancelSelection( const SGameMessage &msg );

	int operator&( IBinSaver &saver ) override;
	void Visit( struct IUIVisitor *pVisitor ) override;
	void InitByDesc( const struct NDb::SUIDesc *pDesc ) override;
};
