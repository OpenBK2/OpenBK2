#include "stdafx.h"

#include "CommonPathFinder.h"
#include "Pathfinders.h"

void RegisterPathfinderSingleton()
{
	NSingleton::RegisterSingleton( new CCommonPathFinder(), CCommonPathFinder::tidTypeID );
}
