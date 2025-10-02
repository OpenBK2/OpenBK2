#include "stdafx.h"

#include "AILogicInternal.h"
#include "CreateAI.h"
#include "GlobeUpdater.h"

void CreateAI()
{
	CPtr<CAILogic> pAI = new CAILogic();
	NSingleton::RegisterSingleton( pAI, IAILogic::tidTypeID );
	NSingleton::RegisterSingleton( pAI, ICommonB2M1AI::tidTypeID );

	NSingleton::UnRegisterSingleton( CUpdates2Globe::tidTypeID );
	NSingleton::RegisterSingleton( new CUpdates2Globe(), CUpdates2Globe::tidTypeID );
}

IAILogic* CreateAI4Globe()
{
	return new CAILogic();
}


