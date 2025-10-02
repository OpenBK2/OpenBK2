#include "stdafx.h"

#include "GlobalObjects.h"
#include "StaticMembers.h"
#include "AILogicInternal.h"
#include "Reinforcement.h"
#include "../Misc/Progress.h"
#include "ScenarioTracker.h"

extern NTimer::STime curTime;

int CAILogic::operator&( IBinSaver &saver )
{
	NGlobalObjects::Serialize( 1, saver );

	CStaticMembers staticMembers;
	saver.Add( 2, &staticMembers );

	saver.Add( 3, &bSuspended );
	saver.Add( 5, &garbage );
	saver.Add( 7, &scripts );
	saver.Add( 9, &bridges );
	//saver.Add( 10, &eTypeOfAreasToShow );
	saver.Add( 12, &bFirstTime );
	saver.Add( 13, &startCmds );
	if ( !saver.IsChecksum() )
	{
		saver.Add( 14, &nextCheckSumTime );
		saver.Add( 15, &periodToCheckSum );
		saver.Add( 16, &checkSum );
		saver.Add( 17, &availableTrucks );
		saver.Add( 18, &bNetGameStarted );
	}
	saver.Add( 19, &reservePositions );
	saver.Add( 20, &pConsts );
	saver.Add( 21, &pCheckSumLog );
	saver.Add( 22, &pProgress );
	if ( saver.IsReading() && pConsts )
	{
		NReinforcement::InitReinforcementTypes( pConsts );
	}

	saver.Add( 25, &curTime );
	saver.Add( 27, &bMissionLoaded );
	if ( !saver.IsChecksum() )
	{
		saver.Add( 29, &pAIMap );
		saver.Add( 30, &pCollisionsCollector );
	}

	SetAIMap( pAIMap );
	if ( !saver.IsChecksum() )
	{
		//saver.Add( 31, &bDrawMarkers );
		saver.Add( 32, &bSegment );
		//saver.Add( 33, &lastPassabilityUpdate );
		saver.Add( 34, &pScenarioTracker );
	}
	saver.Add( 35, &timeLocalPlayerUnitCheck );
	if ( !saver.IsChecksum() )
	{
		saver.Add( 36, &bLocalPlayerUnitsPresent );
	}
	if ( saver.IsReading() )
		timeLastMiniMapUpdateUnits = curTime;

	SetNeedNewGroupNumber();
		
	return 0;
}


