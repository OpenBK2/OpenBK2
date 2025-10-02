#include "stdafx.h"

#include "GlobalObjects.h"

#include "GroupLogic.h"
#include "Units.h"
#include "NewUpdater.h"
#include "StaticObjects.h"
#include "Shell.h"
#include "Diplomacy.h"
#include "GlobalWarFog.h"
#include "UnitCreation.h"
#include "AntiArtilleryManager.h"
#include "Cheats.h"
#include "HitsStore.h"
#include "AckManager.h"
#include "CombatEstimator.h"
#include "Statistics.h"
#include "GeneralInternal.h"
#include "Weather.h"
#include "DifficultyLevel.h"
#include "Graveyard.h"
#include "AAFeedBacks.h"
#include "ExecutorContainer.h"
#include "ManuverBuilder.h"
#include "UnderConstructionObject.h"
#include "Bridge.h"
#include "KeyBuildingBonusSystem.h"
#include "CommonStates.h"
#include "PlayerReinforcement.h"
#include "RailRoads.h"
#include "BalanceTest.h"
#include "PlanesFormation.h"
#include "FeedbackSystem.h"
#include "CommandRegistratorForScript.h"

extern CAAFeedBacks theAAFeedBacks;
extern CSupremeBeing theSupremeBeing;
extern CCombatEstimator theCombatEstimator;
extern CAckManager theAckManager;
extern CGroupLogic theGroupLogic;
extern CUnits units;
extern CEventUpdater updater;
extern CStaticObjects theStatObjs;
extern NTimer::STime curTime;
extern CHitsStore theHitsStore;
extern CDiplomacy theDipl;
extern CGlobalWarFog theWarFog;
extern CUnitCreation theUnitCreation;
extern CAntiArtilleryManager theAAManager;
extern SCheats theCheats;
extern CShellsStore theShellsStore;
extern CStatistics theStatistics;
extern CWeather theWeather;
extern CDifficultyLevel theDifficultyLevel;
extern CGraveyard theGraveyard;
extern CExecutorContainer theExecutorContainer;
extern CManuverBuilder theManuverBuilder;
extern CUnderConstructionObject theUnderConstructionObject;
extern CBridgeHeightRemover theBridgeHeightsRemover;
extern CKeyBuildingBonusSystem theBonusSystem;
extern SPatrolWaypoints thePatrolWaypoints;
extern CPlayerReinforcementArray theReinfArray;
extern SRailRoadSystem theRailRoadSystem;
extern CBalanceTest theBalanceTest;
extern CFeedBackSystem theFeedBackSystem;
extern CCommandRegistratorForScript theCommandTrackerForScript;

void NGlobalObjects::Clear()
{
	theGraveyard.Clear();

	theWarFog.Clear();

	theGroupLogic.Clear();
	units.Clear();
	updater.Clear();
	theStatObjs.Clear();
	theHitsStore.Clear();
	theDipl.Clear();
	theUnitCreation.Clear();
	theAAManager.Clear();
	theCheats.Clear();
	theShellsStore.Clear();
	theAckManager.Clear();
	theWeather.Clear();

	theCombatEstimator.Clear();
	theSupremeBeing.Clear();
	theWeather.Clear();
	theGraveyard.Clear();
	theDifficultyLevel.Clear();

	theAAFeedBacks.Clear();
	theExecutorContainer.Clear();
	
	theManuverBuilder.Clear();
	theUnderConstructionObject.Clear();
	theBridgeHeightsRemover.Clear();
	theBonusSystem.Clear();

	thePatrolWaypoints.Clear();
	theReinfArray.clear();

	theRailRoadSystem.Clear();
	theBalanceTest.Clear();
	CPlanesFormation::Clear();
	theFeedBackSystem.Clear();
	theCommandTrackerForScript.Clear();
}

class CGlobalSerializer
{
public: 
	int operator&( IBinSaver &saver ); 
private:
};

int CGlobalSerializer::operator&( IBinSaver &saver )
{
	if ( saver.IsReading() )
		SConsts::Load();

	saver.Add( 4, &theGroupLogic );
	saver.Add( 5, &units );
	saver.Add( 6, &updater );
	saver.Add( 7, &theStatObjs );
	saver.Add( 8, &theHitsStore );
	saver.Add( 9, &theDipl );
	saver.Add( 10, &theUnitCreation );
	saver.Add( 11, &theAAManager );
	saver.Add( 12, &theShellsStore );

	if ( !saver.IsChecksum() )
	{
		saver.Add( 13, &theWarFog );
		saver.Add( 14, &theCheats );
		saver.Add( 15, &theAckManager );
		saver.Add( 16, &theCombatEstimator );
		saver.Add( 17, &theStatistics );
		saver.Add( 18, &theSupremeBeing );
	}
	saver.Add( 21, &theWeather );
	
	saver.Add( 22, &theDifficultyLevel );
	saver.Add( 23, &theGraveyard );
	saver.Add( 24, &theAAFeedBacks );
	saver.Add( 25, &theExecutorContainer );

	saver.Add( 29, &theManuverBuilder );
	
	if ( !saver.IsChecksum() )
		saver.Add( 30, &theUnderConstructionObject );
	
	saver.Add( 31, &theBridgeHeightsRemover );
	saver.Add( 32, &theBonusSystem );
	saver.Add( 33, &thePatrolWaypoints );
	saver.Add( 35, &theReinfArray );
	saver.Add( 36, &theRailRoadSystem );
	saver.Add( 37, &theFeedBackSystem );
	saver.Add( 38, &theCommandTrackerForScript );
	return 0;
}

void NGlobalObjects::Serialize( int idChunk, IBinSaver &saver )
{
	CGlobalSerializer globalSerializer;
	saver.Add( idChunk, &globalSerializer );
}

