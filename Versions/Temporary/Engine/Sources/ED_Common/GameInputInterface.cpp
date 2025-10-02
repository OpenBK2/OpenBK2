#include "StdAfx.h"
#include "GameInputInterface.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool CGameInputInterface::ProcessEvent( const struct SGameMessage &msg )
{
	return NInput::CGMORegContainer::ProcessEvent( msg, this );
}


