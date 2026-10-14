#pragma once

#include "Input/GameMessage.h"
#include "InterfaceCommandDeclare.h"
#include "System/Time.h"

#include <cstdint>

class CRunModeInterfaceBase : public IInterfaceBase, protected NInput::CGMORegContainer
{
public:
	// lifecycle
	CRunModeInterfaceBase( CTimeCounter *pTimer = 0 );
	~CRunModeInterfaceBase() override;

	// IInterfaceBase
	void OnGetFocus( bool bFocus ) override;
	void Step( bool bAppActive ) override;
	bool ProcessEvent( const struct SGameMessage &msg ) override;

	// methods
	uint32_t GetTime() const
	{
		return dwTime;
	}

	// members
protected:
	CTimeCounter *pTimer;
	uint32_t dwTime;
};
