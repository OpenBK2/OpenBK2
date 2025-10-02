#include "StdAfx.h"

#include "Main_export.h"

//#include "..\ui\commandparam.h"
//#include "..\ui\dbuserinterface.h"
#include <wtypes.h>
#include <winbase.h>


#include "../Main/MainLoop.h"

#include "../UI/UI.h"

namespace NMain
{

MAIN_EXPORT bool Initialize()
{
	// game timer
	NSingleton::RegisterSingleton( CreateGameTimer( NGlobal::GetVar( "AI_SEGMENT_DURATION", 50 ) ), IGameTimer::tidTypeID );
	NMainLoop::InitMainLoop();
	// UI consts reciever
	NSingleton::RegisterSingleton( CreateUIInitialization(), IUIInitialization::tidTypeID );
	return true;
}

}


