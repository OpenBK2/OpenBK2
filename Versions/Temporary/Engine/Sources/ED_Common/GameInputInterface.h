#pragma once

#include "Input/GameMessage.h"
#include "InterfaceCommandDeclare.h"

#include "ED_Common_export.h"

class ED_COMMON_EXPORT CGameInputInterface : public IInterfaceBase, protected NInput::CGMORegContainer
{
public:
	~CGameInputInterface() override {}

	// IInterfaceBase
	void OnGetFocus( bool bFocus ) override {}

	void Step( bool bAppActive ) override {}

	bool ProcessEvent( const struct SGameMessage &msg ) override;
};
