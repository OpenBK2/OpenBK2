#include "stdafx.h"

#include "Pathfinders.h"
#include "CommonPathFinder.h"

void RegisterPathfinderSingleton()
{
	NSingleton::RegisterSingleton( new CCommonPathFinder(), CCommonPathFinder::tidTypeID );
}

