
#pragma once

#include "../Input/GameMessage.h"
#include "../System/Time.h"
#include "InterfaceCommandDeclare.h"

class CRunModeInterfaceBase : public IInterfaceBase, protected NInput::CGMORegContainer
{
public:	
	// lifecycle
	CRunModeInterfaceBase( CTimeCounter * pTimer = 0 );
	virtual ~CRunModeInterfaceBase();
	
	// IInterfaceBase
	virtual void OnGetFocus( bool bFocus );
	virtual void Step( bool bAppActive );
	virtual bool ProcessEvent( const struct SGameMessage &msg );

	// methods
	DWORD GetTime() const { return dwTime; }

	// members
protected:
	CTimeCounter * pTimer;
	DWORD dwTime;
};


