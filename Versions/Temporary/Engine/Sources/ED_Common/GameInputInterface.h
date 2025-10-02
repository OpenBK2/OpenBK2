#pragma once

#include "Input/GameMessage.h"
#include "InterfaceCommandDeclare.h"

class CGameInputInterface : public IInterfaceBase, protected NInput::CGMORegContainer
{
public:	
	virtual ~CGameInputInterface() {}
	
	// IInterfaceBase
	virtual void OnGetFocus( bool bFocus ) {}
	virtual void Step( bool bAppActive ) {}
	virtual bool ProcessEvent( const struct SGameMessage &msg );
};


