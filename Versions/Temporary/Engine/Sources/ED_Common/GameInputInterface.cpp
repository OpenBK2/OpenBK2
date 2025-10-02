#include "StdAfx.h"
#include "GameInputInterface.h"

bool CGameInputInterface::ProcessEvent( const struct SGameMessage &msg )
{
	return NInput::CGMORegContainer::ProcessEvent( msg, this );
}


