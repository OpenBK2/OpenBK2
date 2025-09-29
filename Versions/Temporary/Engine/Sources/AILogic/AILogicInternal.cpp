#include "stdafx.h"

#include "../system/time.h"
#include "../ui/commandparam.h"
#include "../ui/dbuserinterface.h"

#include "AILogicInternal.h"
#include "Commands.h"
#include "UnitsIterators.h"
#include "NewUpdater.h"
#include "StaticObjects.h"
#include "../Common_RTS_AI/AIMap.h"
#include "GroupLogic.h"
#include "Entrenchment.h"
#include "HitsStore.h"
#include "UnitCreation.h"
#include "AntiArtilleryManager.h"
#include "Cheats.h"
#include "GlobalObjects.h"
#include "AckManager.h"
#include "CombatEstimator.h"
#include "Statistics.h"
#include "General.h"
#include "Weather.h"
#include "DifficultyLevel.h"
#include "Graveyard.h"
#include "GridCreation.h"
#include "Artillery.h"
#include "Formation.h"
#include "Shell.h"
#include "Building.h"
#include "ExecutorContainer.h"
#include "TerraAIObserver.h"
#include "UnderConstructionObject.h"
#include "TempBuffer.h"
#include "KeyBuildingBonusSystem.h"
#include "Reinforcement.h"
#include "../Main/GameTimer.h"

#include "../Common_RTS_AI/StaticMapHeights.h"
#include "../Common_RTS_AI/CommonPathFinder.h"
extern CUnderConstructionObject theUnderConstructionObject;
#include "ExecutorRestoreTransparencyQueue.h"
//#include "../Common_RTS_AI/CollisionInternal.h"
#include "../System/CheckSumLog.h"


//#include "../SCeneB2/AIDebugInfo.h"
#include "../UISpecificB2/UISpecificB2.h"

#include "ManuverBuilder.h"
#include "../Misc/StrProc.h"

#include "BalanceTest.h"
//#include "../System/Commands.h"
#include "Bridge.h"
#include "Soldier.h"
#include "Trains.h"
#include "ScenarioTracker.h"
#include "FeedbackSystem.h"
#include "DBAIConsts.h"
#include "PlayerReinforcement.h"
#include "../Common_RTS_AI/CheckSums.h"
#include "../Misc/Win32Helper.h"
#include "../Input/Bind.h"
#include "GlobalWarFog.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define FPS_TEST
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
REGISTER_SAVELOAD_CLASS( 0x1108D441, CAILogic );
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern CFeedBackSystem theFeedBackSystem;
extern CPlayerReinforcementArray theReinfArray;
extern CBridgeHeightRemover theBridgeHeightsRemover;
extern CManuverBuilder theManuverBuilder;
extern CWeather theWeather;
extern CSupremeBeing theSupremeBeing;
extern CCombatEstimator theCombatEstimator;
extern CAckManager theAckManager;
extern CGroupLogic theGroupLogic;
extern CUnits units;
extern CEventUpdater updater;
extern CStaticObjects theStatObjs;
extern CExecutorContainer theExecutorContainer;
NTimer::STime curTime;
extern CHitsStore theHitsStore;
extern CDiplomacy theDipl;
extern CGlobalWarFog theWarFog;
extern CUnitCreation theUnitCreation;
extern CAntiArtilleryManager theAAManager;
extern SCheats theCheats;
extern CStatistics theStatistics;
extern CDifficultyLevel theDifficultyLevel;
extern CGraveyard theGraveyard;
extern CKeyBuildingBonusSystem theBonusSystem;
// for debug
extern CShellsStore theShellsStore;

#ifdef FPS_TEST
EXTERNVAR int nDGCurrentFrame;
static int nSegmentStop = 0;
static int nSegmentCount = -10;
static int nStartFrame = 0;
static DWORD dwStartTickCount = 0;

static const int nSegemntsInTick = 20;
static int nSegmentNextTick = 0;
static DWORD dwTickStartTickCount = 0;
static int nTickStartFrame = 0;

#include <VTuneAPI.h>
#pragma comment( lib, "vtuneapi.lib" )
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CBalanceTest theBalanceTest;

IAIScenarioTracker *GetScenarioTracker()
{
	//CRAP{ FOR SAVES COMPATIBILITY
	IAIScenarioTracker * p = dynamic_cast<CAILogic*>(Singleton<IAILogic>())->GetScenarioTracker();
	if ( p )
		return p;
	return Singleton<IAIScenarioTracker>();
	//CRAP}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*******************************************************************
//*		 								   CAILogic																		*
//*******************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::PickedObj( const int nObjID )
{
	if ( CLinkObject::IsLinkObjectExists( nObjID ) )
	{
		CUpdatableObj *pObj = GetObjectByUniqueIdSafe<CUpdatableObj>( nObjID );
		if ( pObj )
		{	
			IDebugSingleton *pDebug = Singleton<IDebugSingleton>();
			if ( pDebug )
			{
				IStatsSystemWindow *pStatsSystemWindow = pDebug->GetStatsWindow();
				if ( pStatsSystemWindow )
					pStatsSystemWindow->UpdateEntry( L"Pick", NStr::ToUnicode(StrFmt( "%d", pObj->GetUniqueId() )), 0xff00ff00 );
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::PickEmpty()
{
	IDebugSingleton *pDebug = Singleton<IDebugSingleton>();
	if ( pDebug )
	{
		IStatsSystemWindow *pStatsSystemWindow = pDebug->GetStatsWindow();
		if ( pStatsSystemWindow )
			pStatsSystemWindow->UpdateEntry( L"Pick", L"none", 0xff00ff00 );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAILogic::CAILogic()
: timeLocalPlayerUnitCheck( 0 ), bLocalPlayerUnitsPresent( false ), timeLastMiniMapUpdateUnits( 0 ), bNeedNewGroupNumber( true )
{
	CScripts::RegisterScriptForSaveLoad();
	bMissionLoaded = false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAILogic::~CAILogic()
{
	ClearAI();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ITerraAIObserver* CAILogic::CreateTerraAIObserver( const int nSizeX, const int nSizeY )
{
	pAIMap = new CAIMap( nSizeX, nSizeY, SAIConsts::TILE_SIZE, SConsts::MAX_UNIT_TILE_RADIUS, SAIConsts::MAX_MAP_SIZE );
	SetAIMap( pAIMap );

	return new CTerraAIObserverInGame( nSizeX, nSizeY );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::ToggleWarFog( const bool bWarFog )
{
	theCheats.SetTurnOffWarFog( !bWarFog );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::ClearAI()
{
	CQueueUnit::Clear();
	
	NGlobalObjects::Clear();
	CAICommand::Clear();
	CExistingObject::Clear();
	
	CLinkObject::Clear();
	updater.Clear();
	scripts.Clear();
	CExistingObjectModifyAI::Clear();
	ConstructorInfo() = 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAILogic::CheckForScenarioTruck( const SMapObjectInfo &object, LinkInfo *linksInfo, const SMechUnitRPGStats **pNewStats ) const
{
	if ( !theDipl.IsNetGame() )
	{
		CDBPtr<SUnitBaseRPGStats> pStats = checked_cast<const SUnitBaseRPGStats*>( object.pObject.GetPtr() );

		// транспорт и с кем-то линкуется
		if ( pStats->IsTransport() && object.link.nLinkWith > 0 )
		{
			CObjectBase *pObj = CLinkObject::GetObjectByLink( object.link.nLinkWith );
			// найден объект, с которым линкуется
			if ( pObj )
			{
				CArtillery *pArtillery = dynamic_cast<CArtillery*>( pObj );
	
				// линкуется с артиллерией
				if ( pArtillery )
				{
					if ( !pArtillery->GetStats()->GetActions()->availExposures.GetData( ACTION_COMMAND_TAKE_ARTILLERY ) )
						return false;

					// артиллерия сценарийная, нужно заменить на подходяцщий грузовик
/*
					if ( pArtillery->GetScenarioUnit() )
					{
						*pNewStats = 0;
						const float fWeight = pArtillery->GetStats()->fWeight;

						for ( CAvailTrucks::const_iterator iter = availableTrucks.begin(); iter != availableTrucks.end(); ++iter )
						{
							const float fTowingForce = (*iter)->fTowingForce;
							if ( fTowingForce >= fWeight && ( *pNewStats == 0 || fTowingForce < (*pNewStats)->fTowingForce ) )
								*pNewStats = *iter;
						}

						NI_ASSERT( *pNewStats != 0, StrFmt( "Truck for artillery %s not found", pArtillery->GetStats()->szKeyName.c_str() ) );
					}
*/
				}
				/*else								// removed - trucks were unable to travel inside other units (i.e. transport ships)
					return false;*/
			}
			// линкуется с несуществующим объектом
			else
				return false;
		}
	}

	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::SendAcknowlegdementForced( CObjectBase *pObj, const EUnitAckType eAck )
{
	if ( pObj && pObj->IsRefValid() )
	{
		CCommonUnit *pUnit = checked_cast<CCommonUnit*>( pObj );
		pUnit->SendAcknowledgement( eAck, true );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CObjectBase* CAILogic::AddObject( const int nUniqueID, const SMapObjectInfo &object, LinkInfo *linksInfo, bool bInitialization, const SHPObjectRPGStats *pPassedStats, EReinforcementType eType )
{
	CUpdatableObj *pResult = 0;

	NI_ASSERT( object.pObject != 0, "Trying to add object with empty stats... ignoring" );
	if ( object.pObject == 0 ) 
		return 0;

	NI_ASSERT( object.pObject->GetTypeID() != NDb::SInfantryRPGStats::typeID, "Trying to add bare infantry w/o squad - ignoring" );
	if ( object.pObject->GetTypeID() == NDb::SInfantryRPGStats::typeID ) 
		return 0;

	{
		const bool bGoodVisObj = object.pObject->pvisualObject != 0 || 
			object.pObject->GetTypeID() == NDb::SSquadRPGStats::typeID ||
			object.pObject->GetTypeID() == NDb::SBridgeRPGStats::typeID ||
			object.pObject->GetTypeID() == NDb::SFenceRPGStats::typeID ||
			object.pObject->GetTypeID() == NDb::SEntrenchmentRPGStats::typeID;
		NI_ASSERT( bGoodVisObj , StrFmt( "object (ID = \"%s\") of type \"%s\" with empty visual part, ignoring", NDb::GetResName(object.pObject), typeid(*object.pObject.GetPtr()).name() ) );
		if ( !bGoodVisObj ) 
			return 0;
	}
	bool bNotAllowedObject = !GetAIMap()->IsPointInside( CVec2( object.vPos.x,object.vPos.y ) ) && object.pObject->GetTypeID() != NDb::SMechUnitRPGStats::typeID &&
														object.pObject->GetTypeID() != NDb::SSquadRPGStats::typeID && object.pObject->GetTypeID() != NDb::SInfantryRPGStats::typeID ;
	// check if object is inside map
	NI_ASSERT( !bNotAllowedObject, "object is outside map, deleting. see next assert about object number in MapInfo" );
	if ( bNotAllowedObject )
		return 0;

	const int nPlayer = object.nPlayer;

	switch ( object.pObject->eGameType )
	{
		case SGVOGT_UNIT:
		{
			if ( theDipl.IsPlayerExist( nPlayer ) )
			{
				const SMechUnitRPGStats *pNewStats = 0;
				if ( CheckForScenarioTruck( object, linksInfo, &pNewStats ) )
				{
					const WORD wDir = object.nDir;
					int id = 0;
					if ( pNewStats != 0 )
					{
						if ( GetAIMap()->IsPointInside( CVec2( object.vPos.x,object.vPos.y ) ) )
						{
							const float z = ( pNewStats->IsAviation() ) ? object.vPos.z  : object.vPos.z + GetHeights()->GetVisZ( object.vPos.x, object.vPos.y );
							const EReinforcementType eTempType = eType == _RT_NONE ? NReinforcement::GetReinforcementTypeByUnitRPGType( pNewStats->etype ) : eType;
							id = theUnitCreation.AddNewUnit( nUniqueID, pNewStats, object.fHP, object.vPos.x, object.vPos.y, z, wDir, nPlayer, true, true, eTempType );
						}
					}
					else if ( pPassedStats != 0 )
					{
						NI_ASSERT( dynamic_cast<const SUnitBaseRPGStats*>(pPassedStats) != 0, StrFmt( "Unit expected, passed object (%s)", pPassedStats->szKeyName.c_str() ) );
						const SUnitBaseRPGStats *pStats = checked_cast<const SUnitBaseRPGStats*>(pPassedStats);
						NI_ASSERT( pStats->IsAviation() || GetAIMap()->IsPointInside( CVec2( object.vPos.x,object.vPos.y ) ), "ground unit is outside map" );
						if ( pStats->IsAviation() || GetAIMap()->IsPointInside( CVec2( object.vPos.x,object.vPos.y ) ) )
						{
							const float z = ( pStats->IsAviation() ) ? object.vPos.z  : object.vPos.z + GetHeights()->GetVisZ( object.vPos.x, object.vPos.y );
							const EReinforcementType eTempType = eType == _RT_NONE ? NReinforcement::GetReinforcementTypeByUnitRPGType( pStats->etype ) : eType;
							id = theUnitCreation.AddNewUnit( nUniqueID, pStats,	object.fHP,	object.vPos.x, object.vPos.y,	z, wDir, nPlayer,	true, true, eTempType );
						}
					}
					else
					{
						const SUnitBaseRPGStats *pStats = checked_cast<const SUnitBaseRPGStats*>( object.pObject.GetPtr() );
						const float z = ( pStats->IsAviation() ) ? object.vPos.z  : object.vPos.z + GetHeights()->GetVisZ( object.vPos.x, object.vPos.y );
						const EReinforcementType eTempType = eType == _RT_NONE ? NReinforcement::GetReinforcementTypeByUnitRPGType( checked_cast<const SUnitBaseRPGStats*>( object.pObject.GetPtr() )->etype ) : eType;
						id = theUnitCreation.AddNewUnit( nUniqueID, pStats, object.fHP, object.vPos.x, object.vPos.y, z, wDir, nPlayer, true, true, eTempType );
					}
					if ( id != 0 )
					{
						pResult = CAIUnit::GetUnitByUniqueID( id );
						scripts.AddObjToScriptGroup( CAIUnit::GetUnitByUniqueID( id ), object.nScriptID );

						if ( !NGlobal::GetVar( "nogeneral", 0 ) &&
								!theDipl.IsNetGame() &&
								!bInitialization && theSupremeBeing.IsMobileReinforcement( theDipl.GetNParty(nPlayer), object.nScriptID ) )
							theSupremeBeing.AddReinforcement( CAIUnit::GetUnitByUniqueID( id ) );
					}
				}
			}

			break;
		}

		case SGVOGT_SQUAD:
		{
			if ( theDipl.IsPlayerExist( object.nPlayer ) )
			{
				CDBPtr<SSquadRPGStats> pStats = checked_cast<const SSquadRPGStats*>(pPassedStats);
				if ( pStats == 0 )
					pStats = checked_cast<const SSquadRPGStats*>( object.pObject.GetPtr() );

				const WORD wDir = object.nDir;
				const int nFormation = object.nFrameIndex == -1 ? 0 : object.nFrameIndex;

				if ( !pStats->members.empty() && pStats->members[0] != 0 )
				{
					//cheat for paratroopers (elite) squads
					const NDb::EReinforcementType eReinfType = eType == NDb::RT_PARATROOPS ? eType : NReinforcement::GetReinforcementTypeByUnitRPGType( pStats->members[0]->etype );
					pResult = theUnitCreation.AddNewFormation( pStats, nFormation, object.fHP, object.vPos.x, 
																										 object.vPos.y, GetHeights()->GetVisZ( object.vPos.x, object.vPos.y ), wDir, nPlayer, true, true, -1, eReinfType );
				}
				else
				{
					NI_ASSERT( !pStats->members.empty() && pStats->members[0] != 0, StrFmt("Invalid squad \"%s\" - empty or invalid members list", NDb::GetResName(pStats)) );
					pResult = 0;
				}
				// pResult can be null in the case of incorrect squad <= defensive programming
				if ( pResult ) 
					scripts.AddObjToScriptGroup( pResult, object.nScriptID );
			}

			break;
		}

		case SGVOGT_BUILDING:
		{
			if ( theCheats.GetLoadObjects() )
			{
				CDBPtr<SBuildingRPGStats> pStats = checked_cast<const SBuildingRPGStats*>(object.pObject.GetPtr());

				pResult = theStatObjs.AddNewBuilding( pStats, object.fHP, object.vPos, object.nDir, object.nFrameIndex, nPlayer, object.link.nLinkID );
				scripts.AddObjToScriptGroup( pResult, object.nScriptID );
				break;
			}
			break;
		}
		
		case SGVOGT_ENTRENCHMENT:
		{
			CDBPtr<SEntrenchmentRPGStats> pStats = checked_cast<const SEntrenchmentRPGStats*>( object.pObject.GetPtr() );

			pResult = theStatObjs.AddNewEntrencmentPart( pStats, object.fHP, object.vPos, object.nDir, object.nFrameIndex, object.nPlayer, false );
			scripts.AddObjToScriptGroup( pResult, object.nScriptID );

			break;
		}

		case SGVOGT_MINE:
		{
			if ( theCheats.GetLoadObjects() )
			{
				CDBPtr<SMineRPGStats> pStats = checked_cast<const SMineRPGStats*>( object.pObject.GetPtr() );
	  		pResult = checked_cast<CUpdatableObj*>( theStatObjs.AddNewMine( pStats, object.fHP, object.vPos, object.nFrameIndex, object.nPlayer ) );
				scripts.AddObjToScriptGroup( pResult, object.nScriptID );
			}

			break;
		}
		
		case SGVOGT_TERRAOBJ:
		{
			if ( theCheats.GetLoadObjects() )
			{
				CDBPtr<SObjectBaseRPGStats> pObjDesc = checked_cast<const SObjectBaseRPGStats*>( object.pObject.GetPtr() );
				
				if ( object.pObject->eVisType == SGVOT_MESH )
					pResult = theStatObjs.AddNewTerraMeshObj( pObjDesc, object.fHP, object.vPos, object.nDir, object.nFrameIndex );
				else
					pResult = theStatObjs.AddNewTerraObj( pObjDesc, object.fHP, object.vPos, object.nDir, object.nFrameIndex );

				scripts.AddObjToScriptGroup( pResult, object.nScriptID );
			}

			break;
		}
		
		case SGVOGT_BRIDGE:
		{
			CDBPtr<SBridgeRPGStats> pObjDesc = static_cast<const SBridgeRPGStats*>( pPassedStats );
			if ( pObjDesc == 0 )
				pObjDesc = checked_cast<const SBridgeRPGStats*>( object.pObject.GetPtr() );
			pResult = theStatObjs.AddNewBridgeSpan( pObjDesc, object.fHP, object.vPos, object.nDir, object.nFrameIndex );
			scripts.AddObjToScriptGroup( pResult, object.nScriptID );

			break;
		}
		
		case SGVOGT_FENCE:
		{
			if ( theCheats.GetLoadObjects() )
			{
				CDBPtr<SFenceRPGStats> pObjDesc = static_cast<const SFenceRPGStats*>( pPassedStats );
				if ( pObjDesc == 0 )
					pObjDesc = checked_cast<const SFenceRPGStats*>( object.pObject.GetPtr() );
//				NI_ASSERT( object.nFrameIndex != -1, StrFmt("Can't add fence (linkID = %d) with frame index -1", object.link.nLinkID) );
				if ( object.nFrameIndex != -1 ) 
				{
					//pResult = theStatObjs.AddNewFenceObject( pObjDesc, object.fHP, object.vPos, object.nDir, object.nPlayer, false );
					pResult = theStatObjs.AddNewFenceObject( pObjDesc, object.fHP, object.vPos, object.nDir, object.nPlayer, object.nFrameIndex );
					scripts.AddObjToScriptGroup( pResult, object.nScriptID );
				}
				else
				{
					NI_ASSERT( object.nFrameIndex != -1, StrFmt("Can't add fence \"%s\" with frame index -1", NDb::GetResName(object.pObject)) );
				}
			}

			break;
		}

		case SGVOGT_FLAG:
		{
			NI_ASSERT( false, StrFmt( "DESIGN: Object has type SGVOGT_FLAG, LinkID %d", object.link.nLinkID ) );

			break;
		}
		case SGVOGT_SOUND:
			return 0;

		default:
		{
			if ( theCheats.GetLoadObjects() )
			{
				NI_ASSERT( dynamic_cast<const SInfantryRPGStats*>( pPassedStats ) == 0,	StrFmt( "Soldier witout squad on map." ) );
				NI_ASSERT( dynamic_cast_ptr<const SInfantryRPGStats*>( object.pObject ) == 0, StrFmt( "Soldier witout squad on map." ) );

				if ( pPassedStats == 0 ) 
					pPassedStats = object.pObject;
				if ( pPassedStats != 0 && dynamic_cast<const SObjectBaseRPGStats*>(pPassedStats) == 0 ) 
				{
					NI_ASSERT( dynamic_cast<const SObjectBaseRPGStats*>(pPassedStats) != 0, StrFmt("Incorrect object \"%s\" of type \"%s\" - trying to treat as simple object", NDb::GetResName(pPassedStats), typeid(*pPassedStats).name()) );
					return 0;
				}
				
				CDBPtr<SObjectBaseRPGStats> pObjDesc = checked_cast<const SObjectBaseRPGStats*>( pPassedStats );
				if ( pObjDesc == 0 )
					pObjDesc = checked_cast<const SObjectBaseRPGStats*>( object.pObject.GetPtr() );
				if ( !pObjDesc )
					return 0;
				pResult = theStatObjs.AddNewStaticObject( pObjDesc, object.fHP, object.vPos, object.nDir, object.nFrameIndex );
				scripts.AddObjToScriptGroup( pResult, object.nScriptID );
			}

			break;
		}
	}

	if ( pResult != 0 )
	{
		NI_ASSERT( dynamic_cast<CLinkObject*>( pResult ) != 0, StrFmt("Wrong object of type \"%s\" created - CLinkObject expected", typeid(*pResult).name()) );
		CLinkObject *pLinkResult = checked_cast<CLinkObject*>( pResult );
		pLinkResult->SetLink( object.link.nLinkID );

		if ( linksInfo != 0 && pLinkResult->GetUniqueId() != 0 )
			(*linksInfo)[pLinkResult->GetUniqueId()] = object.link;
	}

	return pResult;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::LoadUnits( const SMapInfo *pMapInfo, LinkInfo *linksInfo )
{
	list<int> transports;
//	ConstructorInfo() = Singleton<CConstructorInfo>();
	for ( int i = 0; i < pMapInfo->objects.size(); ++i )
	{
		if ( pProgress )
			pProgress->Step();
			
		if ( pMapInfo->objects[i].pObject == 0 || pMapInfo->objects[i].link.nLinkID == -1 ) 
			continue;
#ifndef _FINALRELEASE
		if ( pMapInfo->objects[i].pObject->GetTypeID() == NDb::SMechUnitRPGStats::typeID ) 
		{
			const NDb::EDBUnitRPGType eUnitRPGType = 
				checked_cast_ptr<const NDb::SUnitBaseRPGStats*>(pMapInfo->objects[i].pObject)->eDBtype;
			NI_ASSERT( eUnitRPGType > NDb::DB_RPG_TYPE_OFFICER, StrFmt("Object %d (MechUnit \"%s\") in the map \"%s\" are treated as Infantry", i, NDb::GetResName(pMapInfo->objects[i].pObject), NDb::GetResName(pMapInfo)) );
			if ( eUnitRPGType <= NDb::DB_RPG_TYPE_OFFICER )
				continue;
			if ( !theDipl.IsPlayerExist( pMapInfo->objects[i].nPlayer ) )
				continue;
		}
		else if ( pMapInfo->objects[i].pObject->GetTypeID() == NDb::SInfantryRPGStats::typeID ) 
		{
			const NDb::EDBUnitRPGType eUnitRPGType = 
				checked_cast_ptr<const NDb::SUnitBaseRPGStats*>(pMapInfo->objects[i].pObject)->eDBtype;
			NI_ASSERT( eUnitRPGType <= NDb::DB_RPG_TYPE_OFFICER, StrFmt("Object %d (Infantry \"%s\") in the map \"%s\" are treated as MechUnit", i, NDb::GetResName(pMapInfo->objects[i].pObject), NDb::GetResName(pMapInfo)) );
			if ( eUnitRPGType > NDb::DB_RPG_TYPE_OFFICER )
				continue;
			if ( !theDipl.IsPlayerExist( pMapInfo->objects[i].nPlayer ) )
				continue;
		}
#endif
		// DebugTrace( "mapobj %d", i );
		// в reinforcement
		const int nGroup = pMapInfo->reinforcements.GetGroupById( pMapInfo->objects[i].nScriptID );
		if ( nGroup != -1 )
			scripts.AddUnitToReinforcGroup( pMapInfo->objects[i], nGroup, 0/*, 0 */);
		else
		{
			const SUnitBaseRPGStats *pStats = dynamic_cast_ptr<const SUnitBaseRPGStats*>( pMapInfo->objects[i].pObject );

			if ( !pStats || !pStats->IsTransport() )
			{
				const int nUniqueID = ++SLinkObjDataAutoMagic::pLinkObjData->nCurUniqueID;
			
				CObjectBase *pObj = AddObject( nUniqueID, pMapInfo->objects[i], linksInfo, true, 0 );
				NI_ASSERT( pObj != 0, StrFmt( "Failed to add %d object (\"%s\" : \"%s\") to map \"%s\" at (%g, %g)", i, typeid(*pMapInfo->objects[i].pObject).name(), NDb::GetResName(pMapInfo->objects[i].pObject), NDb::GetResName(pMapInfo), pMapInfo->objects[i].vPos.x, pMapInfo->objects[i].vPos.y) );
			}
			else
				transports.push_back( i );
		}
	}

	for ( list<int>::iterator iter = transports.begin(); iter != transports.end(); ++iter )
	{
		const SMapObjectInfo &mapObj = pMapInfo->objects[*iter];
		const SUnitBaseRPGStats *pStats = dynamic_cast_ptr<const SUnitBaseRPGStats*>( mapObj.pObject );

		const int nUniqueID = ++SLinkObjDataAutoMagic::pLinkObjData->nCurUniqueID;
		CObjectBase *pObj = AddObject( nUniqueID, mapObj, linksInfo, true, 0 );
		NI_ASSERT( pObj != 0, StrFmt( "Failed to add %d object (\"%s\" : \"%s\") to map \"%s\" at (%g, %g)", *iter, typeid(*mapObj.pObject).name(), NDb::GetResName(mapObj.pObject), NDb::GetResName(pMapInfo), mapObj.vPos.x, mapObj.vPos.y) );
	}
		
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::LinkArtilleryWithTransport( CArtillery * pArtillery, CAITransportUnit *pTransport )
{
	// переместить артиллерию к пушке
	pTransport->SetTowedArtillery( pArtillery );
	pArtillery->SetBeingHooked( pTransport );
	CFormation* pCrew = pArtillery->GetCrew();
	pTransport->SetTowedArtilleryCrew( pCrew );
	pArtillery->SetDirection( 65535/2 + pTransport->GetFrontDirection() );
	pArtillery->CallUpdatePlacement();

	const CVec2 vTransportHookPoint = pTransport->GetHookPoint();
	const CVec2 vArtilleryHookPoint = pArtillery->GetHookPoint();
	const CVec2 vShiftFromHookPointToArtillery = pArtillery->GetCenterPlain() - vArtilleryHookPoint;
	const CVec2 vHookPoint = vTransportHookPoint + vShiftFromHookPointToArtillery;

	pArtillery->SetCenter( CVec3( vHookPoint.x, vHookPoint.y, GetHeights()->GetZ( vHookPoint ) ), false );

	if ( IsValid( pCrew ) )
		pCrew->QuickLoadToMechUnit( pTransport );
	
	pArtillery->InstallAction( ACTION_NOTIFY_UNINSTALL_TRANSPORT, true );
	updater.AddUpdate( 0, ACTION_NOTIFY_FINISH_UNINSTALL_TRANSPORT, pArtillery, 0 );
	pArtillery->SetDirection( 65535/2 + pTransport->GetFrontDirection() );
	pArtillery->CallUpdatePlacement();
	pArtillery->UpdateAmmoBoxVisibility( true, false );

	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_BEING_TOWED, pTransport->GetUniqueId() ), pArtillery, false );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AdvanceSegments( CCommonUnit *pUnit )
{

	if ( pUnit->IsFormation() )
	{
		CFormation *pFormation = static_cast<CFormation*>( pUnit );
		pFormation->Segment();
		pFormation->Segment();
		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			(*pFormation)[i]->Segment();
			(*pFormation)[i]->Segment();
		}
	}
	else
	{
		pUnit->Segment();
		pUnit->Segment();
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::InitLinks( LinkInfo &linksInfo )
{
	for ( LinkInfo::iterator iter = linksInfo.begin(); iter != linksInfo.end(); ++iter )
	{
		bool bTrain = false;

		if ( CAIUnit *pTrainUnit = dynamic_cast<CAIUnit*>(CLinkObject::GetObjectByUniqueId( iter->first )) )
		{
			if ( pTrainUnit->GetStats()->IsTrain() )
			{
				bTrain = true;				
				CTrainCar *pTrainCar = dynamic_cast<CTrainCar*>( pTrainUnit );		// Train cars manage the linking
				if ( pTrainCar )
				{
					const SMapObjectInfo::SLinkInfo &link = linksInfo[pTrainUnit->GetUniqueId()];
					if ( link.nLinkWith > 0 )
					{
						CObjectBase *pObj = CLinkObject::GetObjectByLink( link.nLinkWith );
						if ( pObj )
						{
							if ( CMilitaryCar *pMilitaryCar = dynamic_cast<CMilitaryCar*>( pObj ) )
							{
								pTrainCar->LinkToCar( pMilitaryCar );
							}
							else
								NI_ASSERT( false, "Wrong link" );
						}
						else
							NI_ASSERT( false, StrFmt( "Wrong link, linked object does not exist for unit: %s", 
								pTrainUnit->GetStats() ? pTrainUnit->GetStats()->GetDBID().ToString() : "???" ) );
					}
				}
			}
		}

		if ( !bTrain )
		{
			CExistingObject *pObj1 = dynamic_cast<CExistingObject*>( CLinkObject::GetObjectByUniqueId( iter->first ) );
			CExistingObject *pObj2 = dynamic_cast<CExistingObject*>( CLinkObject::GetObjectByLink( iter->second.nLinkWith ) );
			// static objecta are linked (key-building and flag)
			if ( pObj1 && pObj2 && ( theBonusSystem.IsKeyBuilding( iter->second.nLinkID ) || theBonusSystem.IsKeyBuilding( iter->second.nLinkWith ) ) )
			{
				CBuildingSimple *pBld1 = dynamic_cast<CBuildingSimple *>( pObj1 );
				CBuildingSimple *pBld2 = dynamic_cast<CBuildingSimple *>( pObj2 );
				if ( pBld1 )
					pBld1->SetPartyFlag( pObj2 );
				else if ( pBld2 )
					pBld2->SetPartyFlag( pObj1 );
			}

			if ( CCommonUnit *pUnit = dynamic_cast<CCommonUnit*>(CLinkObject::GetObjectByUniqueId( iter->first )) )
			{
				const SMapObjectInfo::SLinkInfo &link = linksInfo[pUnit->GetUniqueId()];
				// с кем-то линкуется и этот кто-то существует
				if ( link.nLinkWith > 0 )
				{
					CObjectBase *pObj = CLinkObject::GetObjectByLink( link.nLinkWith );
					if ( pObj )
					{
						//линковка грузовичка с пушкой
						if ( CAITransportUnit *pTransport = dynamic_cast<CAITransportUnit*>( pObj ) )
						{
							if ( CArtillery *pArtillery = dynamic_cast<CArtillery*>(pUnit) )
								LinkArtilleryWithTransport( pArtillery, pTransport );
							// линкуются солдатики
							else if ( pUnit->IsFormation() )
							{
								//theGroupLogic.UnitCommand( SAIUnitCmd( link.bIntention ? ACTION_COMMAND_LOAD : ACTION_COMMAND_LOAD_NOW, pTransport->GetUniqueId() ), pUnit, false );

								if ( !link.bIntention )
								{
									CFormation *pFormation = static_cast<CFormation*>( pUnit );
									for ( int i = 0; i < pFormation->Size(); ++i )
									{
										theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRANSPORT, pTransport->GetUniqueId() ), (*pFormation)[i], false );
										(*pFormation)[i]->SetInTransport( pTransport );
										pTransport->AddPassenger( (*pFormation)[i] );
									}
									theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRANSPORT, pTransport->GetUniqueId() ), pFormation, false );
								}
								else
								{
									theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_LOAD, pTransport->GetUniqueId() ), pUnit, false );
								}
							}
							else if ( CMilitaryCar * pCar = dynamic_cast<CMilitaryCar*>( pUnit ) )
							{
								if ( CMilitaryCar *pTransport = dynamic_cast<CMilitaryCar*>( pObj ) )
								{
									theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_MECH_ENTER_NOW, pTransport->GetUniqueId() ), pCar, false );
									AdvanceSegments( pCar );
									AdvanceSegments( pTransport );
								}
							}
						}
						else if ( CArtillery *pArtillery = dynamic_cast<CArtillery*>( pObj ) )
						{
							if ( CAITransportUnit *pTransport = dynamic_cast<CAITransportUnit*>(pUnit) )
								LinkArtilleryWithTransport( pArtillery, pTransport );
						}
						// линковка с юнитом
						else if ( CCommonUnit *pWithUnit = dynamic_cast<CCommonUnit*>(pObj) )
						{
							// линкуются солдатики
							if ( pUnit->IsFormation() )
							{
								if ( link.bIntention )
									theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_LOAD, pWithUnit->GetUniqueId() ), pUnit, false );
								else
								{
									theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_LOAD_NOW, pWithUnit->GetUniqueId() ), pUnit, false );
									AdvanceSegments( pUnit );
								}
							}
							else if ( CMilitaryCar * pCar = dynamic_cast<CMilitaryCar*>( pUnit ) )
							{
								if ( CMilitaryCar *pTransport = dynamic_cast<CMilitaryCar*>( pObj ) )
								{
									theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_MECH_ENTER_NOW, pTransport->GetUniqueId() ), pCar, false );
									AdvanceSegments( pCar );
									AdvanceSegments( pTransport );
								}
							}
						}
						// link with static object
						else if ( CStaticObject *pStaticObj = dynamic_cast<CStaticObject*>( pObj ) )
						{
							// линковка с домиком
							if ( pStaticObj->GetObjectType() == ESOT_BUILDING )
							{
								if ( link.bIntention )
									theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_ENTER, pStaticObj->GetUniqueId(), float(0) ), pUnit, false );
								else
								{
									CCommonUnit *pLoadingUnit = checked_cast<CCommonUnit*>(CLinkObject::GetObjectByUniqueId( iter->first ));
									theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_ENTER_BUILDING_NOW, pStaticObj->GetUniqueId() ), pLoadingUnit, false );
								}
							}
							// линковка с окопом
							else if ( pStaticObj->GetObjectType() == ESOT_ENTR_PART )
							{
								if ( link.bIntention )
									theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_ENTER, pStaticObj->GetUniqueId(), float(2) ), pUnit, false );
								else
								{
									CCommonUnit *pLoadingUnit = checked_cast<CCommonUnit*>(CLinkObject::GetObjectByUniqueId( iter->first ));									
									theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_ENTER_ENTREHCMNENT_NOW, pStaticObj->GetUniqueId(), float(2) ), pLoadingUnit, false );
								}
							}
						}
						else
							NI_ASSERT( false, "Wrong link" );
					}
					else
						NI_ASSERT( false, "Wrong link" );
				}
			}
		}
	}
} 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::LoadEntrenchments( const vector<SEntrenchmentInfo> &entrenchments )
{
	// по окопам
	for ( int i = 0; i < entrenchments.size(); ++i )
	{
		// по секциям
		CPtr<CFullEntrenchment> pFullEntrenchment = new CFullEntrenchment();
		for ( int j = 0; j < entrenchments[i].sections.size(); ++j )
		{
			vector<CObjectBase*> segments;
			for ( int k = 0; k < entrenchments[i].sections[j].data.size(); ++k )
			{
				const int nLink = entrenchments[i].sections[j].data[k];
				CLinkObject *pObj = CLinkObject::GetObjectByLink( nLink );
				NI_ASSERT( pObj != 0, StrFmt("Section of entrenchment (link = %d) doesn't exist", nLink) );
				if ( pObj )
					segments.push_back( pObj );
			}

			if ( segments.empty() )
				continue;
			theStatObjs.AddNewEntrencment( &(segments[0]), segments.size(), pFullEntrenchment );
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::LoadBridges( const vector< NDb::SIntArray > &bridgesInfo )
{
	// по мостам
	for ( int i = 0; i < bridgesInfo.size(); ++i )
	{
		list<CPtr<CBridgeSpan> > bridge;
		// по пролётам
		CPtr<CFullBridge> pFullBridge = new CFullBridge;
		for ( int j = 0; j < bridgesInfo[i].data.size(); ++j )
		{
			NI_ASSERT( CLinkObject::GetObjectByLink( bridgesInfo[i].data[j] ) != 0, "Span of a bridge doesn't exist" );
			NI_ASSERT( dynamic_cast<CBridgeSpan*>( CLinkObject::GetObjectByLink( bridgesInfo[i].data[j] ) ) != 0, "Wrong type of bridge span" );
			// add span only in the case it exist (!)
			if ( CBridgeSpan* pSpan = checked_cast<CBridgeSpan*>( CLinkObject::GetObjectByLink( bridgesInfo[i].data[j] ) ) )
			{
				pSpan->SetFullBrige( pFullBridge );
				pFullBridge->AddSpan( pSpan );

				bridge.push_back( pSpan );
			}
		}
		pFullBridge->InitEntireBridge();
		bridges.push_back( bridge );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::LaunchStartCommand( const SAIStartCommand &startCommand, CObjectBase **pUnitsBuffer, const int nSize )
{
	SAIUnitCmd cmd;
	cmd.nCmdType = static_cast<EActionCommand>(startCommand.nCmdType);
	cmd.fNumber = startCommand.fNumber;
	cmd.bFromExplosion = startCommand.bFromExplosion;
	if ( CLinkObject *pObj = CLinkObject::GetObjectByLink( startCommand.nLinkID ) )
		cmd.nObjectID = pObj->GetUniqueId();
	cmd.vPos = startCommand.vPos;
	
	const	WORD wGroup = theGroupLogic.GenerateGroupNumber();
	theGroupLogic.RegisterGroup( pUnitsBuffer, nSize, wGroup );
	theGroupLogic.GroupCommand( cmd, wGroup, true );
	theGroupLogic.UnregisterGroup( wGroup );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::InitStartCommands()
{
	for ( vector< SAIStartCommand >::const_iterator iter = startCmds.begin(); iter != startCmds.end(); ++iter )
	{
		if ( !iter->unitLinkIDs.empty() )
		{
			const int nSize = iter->unitLinkIDs.size();
			vector<CObjectBase*> unitsBuffer( nSize );
			for ( int i = 0; i < nSize; ++i )
				unitsBuffer[i] = CLinkObject::GetObjectByLink( iter->unitLinkIDs[i] );

			LaunchStartCommand( *iter, &(unitsBuffer[0]), nSize );
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::InitStartCommands( const LinkInfo &linksInfo, hash_map<int, int> &old2NewLinks )
{
	for ( vector< SAIStartCommand >::const_iterator iter = startCmds.begin(); iter != startCmds.end(); ++iter )
	{
		if ( !iter->unitLinkIDs.empty() )
		{
			int nActuallyUnits = 0;			
			const int nSize = iter->unitLinkIDs.size();
			vector<CObjectBase*> unitsBuffer( nSize );
			for ( int i = 0; i < nSize; ++i )
			{
				const int nLink = iter->unitLinkIDs[i];
				if ( old2NewLinks.find( nLink ) != old2NewLinks.end() )
				{
					CLinkObject *pUnit = CLinkObject::GetObjectByLink( old2NewLinks[nLink] );
					if ( pUnit != 0 && linksInfo.find( pUnit->GetUniqueId() ) != linksInfo.end() )
						unitsBuffer[nActuallyUnits++] = pUnit;
				}
			}

			if ( nActuallyUnits != 0 )
				LaunchStartCommand( *iter, &(unitsBuffer[0]), nSize );
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::InitReservePositions()
{
	for ( vector< SBattlePosition >::const_iterator iter = reservePositions.begin(); iter != reservePositions.end(); ++iter )
	{
		CLinkObject *pLinkObject = CLinkObject::GetObjectByLink( iter->nArtilleryLinkID );
		if ( pLinkObject && pLinkObject->IsRefValid() && pLinkObject->IsAlive() )
		{
			CAIUnit *pUnit = checked_cast<CAIUnit*>( pLinkObject );
			pUnit->SetBattlePos( iter->vPos );

			// есть грузовичок, который таскает
			CLinkObject *pLinkTruck = CLinkObject::GetObjectByLink( iter->nTruckLinkID );
			if ( pLinkTruck && pLinkTruck->IsRefValid() && pLinkTruck->IsAlive() )
			{
				CAITransportUnit *pTruck = checked_cast<CAITransportUnit*>( pLinkTruck );
				pTruck->SetMustTow( pUnit );
				pUnit->SetTruck( pTruck );
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::InitReservePositions( hash_map<int, int> &old2NewLinks )
{
	for ( vector< SBattlePosition >::const_iterator iter = reservePositions.begin(); iter != reservePositions.end(); ++iter )
	{
		const int nArtilleryLink = iter->nArtilleryLinkID;
		if ( old2NewLinks.find( nArtilleryLink ) != old2NewLinks.end() )
		{
			CLinkObject *pLinkObject = CLinkObject::GetObjectByLink( old2NewLinks[nArtilleryLink] );
			if ( pLinkObject && pLinkObject->IsRefValid() && pLinkObject->IsAlive() )
			{
				CCommonUnit *pUnit = checked_cast<CCommonUnit*>( pLinkObject );
				pUnit->SetBattlePos( iter->vPos );

				// есть грузовичок, который таскает
				const int nTruckLink = iter->nTruckLinkID;
				if ( old2NewLinks.find( nTruckLink ) != old2NewLinks.end() )
				{
					CLinkObject *pLinkTruck = CLinkObject::GetObjectByLink( old2NewLinks[nTruckLink] );
					if ( pLinkTruck && pLinkTruck->IsRefValid() && pLinkTruck->IsAlive() )
					{
						CAIUnit *pTruck = checked_cast<CAIUnit*>( pLinkTruck );
						pUnit->SetTruck( pTruck );
					}
				}
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::CommonInit( const NDb::STerrain *pTerrainInfo )
{
	theCheats.Init();
	theCheats.SetNPartyForWarFog( theDipl.GetMyParty(), true );

	bSuspended = true;
	bFirstTime = true;

	curTime = CAITimer::GetSegmentTime();
	timeLastMiniMapUpdateUnits = curTime;

	theAAManager.Init();
	units.Init();
	theGroupLogic.Init( pCollisionsCollector );

	theStatistics.Init();
	
	theWeather.Init();

	checkSum = adler32( 0L, Z_NULL, 0 );	
	if ( theDipl.IsNetGame() )
	{
		periodToCheckSum = NGlobal::GetVar( "checksumperiod", 1000 );
		nextCheckSumTime = periodToCheckSum;
	}
	else
	{
		periodToCheckSum = nextCheckSumTime = 0;
	}

	theDifficultyLevel.Init();

	theCheats.SetHistoryPlaying( NGlobal::GetVar( "History.Playing", 0 ) != 0 );

	// global executors
	CPtr<CExecutor> pEx = new CExecutorRestoreTransparencyQueue;
	theExecutorContainer.Add( pEx );
	pEx->RegisterOnEvents( &theExecutorContainer );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::SetProgressHook( IProgressHook *_pProgress )
{
	pProgress = _pProgress;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::LogCheckSum( ICheckSumLog *_pCheckSumLog )
{ 
	uLong tmpCheckSum = adler32( 0L, Z_NULL, 0 );	
	CPtr<IBinSaver> pCheckSumSaver = CreateCheckSumSaver( &tmpCheckSum, _pCheckSumLog, curTime );
	this->operator&( *pCheckSumSaver );
}
void ForceLink111();
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::Init( ICheckSumLog *_pCheckSumLog, const SMapInfo* pMapInfo, const NDb::SAIGameConsts *_pConsts, IAIScenarioTracker *_pScenarioTracker )
{
	ForceLink111();
	// set control word for FP co-processor
	// _EM_INVALID | _EM_ZERODIVIDE | _EM_OVERFLOW | _EM_UNDERFLOW | _EM_INEXACT | _EM_DENORMAL | _PC_24
	// 0xa001f
	_control87( _EM_INVALID | _EM_ZERODIVIDE | _EM_OVERFLOW | _EM_UNDERFLOW | _EM_INEXACT | _EM_DENORMAL | _PC_24, 0xfffff );

	pConsts = _pConsts;
	pCheckSumLog = _pCheckSumLog;
	pScenarioTracker = _pScenarioTracker;

	CExistingObject::Init();	
	SConsts::Load();
	SConsts::InitPriorities( pConsts->unitTypesPriorities );

	Singleton<CCommonPathFinder>()->Init();

	NReinforcement::InitReinforcementTypes( _pConsts );
	theUnitCreation.SetConsts( pConsts );
	//theFeedBackSystem.Init( pConsts );

	theDipl.Load( pMapInfo->diplomacies );
	theExecutorContainer.Init();
	
	theStatistics.Init();

	theWarFog.Init( pMapInfo->nNumPatchesX * AI_TILES_IN_PATCH, pMapInfo->nNumPatchesY * AI_TILES_IN_PATCH, pConsts->warFog.nMaxRadius, pConsts->warFog.fUnitHeight );

	theCheats.SetWarFog( ( NGlobal::GetVar( "warfog", "" ) ).GetString().empty() );
	theCheats.SetLoadObjects( ( NGlobal::GetVar( "noobjects", "" ) ).GetString().empty() );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::UpdateUnitsOnBridges()
{
	for ( CGlobalIter it( 0, ANY_PARTY ); !it.IsFinished(); it.Iterate() )	
	{
		CAIUnit *pUnit = *it;
		if ( GetTerrain()->IsBridge( pUnit->GetCenterTile() ) )
			pUnit->SetCenter( CVec3( pUnit->GetCenterPlain(), GetHeights()->GetZ( pUnit->GetCenterPlain() ) ), true );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::InitAfterMapLoad( const struct NDb::SMapInfo *pMapInfo )
{
	NWin32Helper::CRoundingControl roundControl( NWin32Helper::CRoundingControl::RCM_NEAR );

//	ConstructorInfo() = Singleton<CConstructorInfo>();

	if ( !GetScenarioTracker()->GetGameType() == IAIScenarioTracker::EGT_SINGLE )
	{
		CPtr<IRandomSeed> pSeed = NRandom::CreateRandomSeedCopy();
		pSeed->InitByZeroSeed();
		NRandom::SetRandomSeed( pSeed );
	}

	GetTerrain()->StartInitMode();
	pCollisionsCollector = CreateCollisionsCollector();
	if ( pProgress )
		pProgress->SetNumSteps( pMapInfo->objects.size() );
		
	theStatObjs.Init( GetAIMap()->GetSizeX(), GetAIMap()->GetSizeY() );
	updater.Init( GetAIMap()->GetSizeX(), GetAIMap()->GetSizeY() );
	theManuverBuilder.Init( pConsts );

	theUnitCreation.Init( pMapInfo, pCollisionsCollector );
	CommonInit( pMapInfo );

	scripts.Init( pMapInfo );
	if ( pMapInfo->scriptAreas.size() > 0 )
		scripts.InitAreas( &(pMapInfo->scriptAreas[0]), pMapInfo->scriptAreas.size() );

	LinkInfo linksInfo;

	theReinfArray.InitPlayerReinforcementArray( pMapInfo, pConsts );
	// init key-building bonus system
	theBonusSystem.InitBonusSystem( pMapInfo );
	// перед загрузкой юнитов запомнить, кто является мобильным резервом
	// -- порядок не менять
	if ( !NGlobal::GetVar( "balance_test", 0 ) )
		LoadUnits( pMapInfo, &linksInfo );
	theBonusSystem.SendUpdates();

	LoadEntrenchments( pMapInfo->entrenchments );
	LoadBridges( pMapInfo->bridges );
	// проинициализировать все maxes на карте
	GetTerrain()->FinishInitMode();
	theStatObjs.PostAllObjectsInit();

	//
	InitLinks( linksInfo );
	reservePositions = pMapInfo->reservePositionsList;
	InitReservePositions();
	UpdateUnitsOnBridges();


	// init general
	if ( !theDipl.IsNetGame() )
	{
		list<CCommonUnit*> pUnits;
		for ( CGlobalIter iter( 0, ANY_PARTY ); !iter.IsFinished(); iter.Iterate() )
			pUnits.push_back( *iter );

		theSupremeBeing.Init( pMapInfo );
		theSupremeBeing.GiveNewUnitsToGenerals( pUnits );
	}
	else if ( GetScenarioTracker()->GetGameType() == IAIScenarioTracker::EGT_MULTI_FLAG_CONTROL )
	{
		// Call starting units reinforcement for this game mode
		theReinfArray.PlaceInitialUnits();
	}

	// -- конец порядок не менять
	startCmds = pMapInfo->startCommandsList;
	InitStartCommands();

	scripts.Load( pMapInfo->szScriptFileRef, pConsts );
	theHitsStore.Init( GetAIMap()->GetSizeX(), GetAIMap()->GetSizeY() );
	theBalanceTest.InitBalanceTest( pMapInfo );

	CPtr<CTerraAIObserverInGame> pOb = new CTerraAIObserverInGame( true );
	pOb->InitInGame();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::PostMapLoad()
{
	CPtr<ICheckSumLog> pTmpChecksum = pCheckSumLog;
	pCheckSumLog = 0;

	bNetGameStarted = false;
	// run 2 segment to launch all start commands
	const bool bOldSuspended = bSuspended;
	bSuspended = false;
	Segment();
	bSuspended = bOldSuspended;
	pCheckSumLog = pTmpChecksum;
	bMissionLoaded = true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::UpdateCheckSum( bool bSend )
{
	if ( !IsValid( pCheckSumLog ) && curTime < NGlobal::GetVar( "start_replay_time", 0 ) * 1000 )
		return;

	//if ( theCheats.IsHistoryPlaying() )
	//	return;

	if ( NGlobal::GetVar( "light_checksum_count", 0 ) )
	{
		using namespace NCheckSums;
		static SCheckSumBufferStorage	checkSumBuf( 10000 );
		checkSumBuf.nCnt = 0;
		
		for ( CGlobalIter iter( 0, ANY_PARTY ); !iter.IsFinished(); iter.Iterate() )
		{
			CAIUnit *pUnit = *iter;
			const CVec2 vCenter = pUnit->GetCenterPlain();
			const WORD wDir = pUnit->GetFrontDirection();
			const float fHP = pUnit->GetHitPoints();

			CopyToBuf( &checkSumBuf, vCenter );
			CopyToBuf( &checkSumBuf, wDir );	
			CopyToBuf( &checkSumBuf, fHP );
		}

		checkSum = adler32( checkSum, &(checkSumBuf.buf[0]), checkSumBuf.nCnt );
		//theGroupLogic.GetCheckSum( &checkSum );
		
		//CONSOLE_BUFFER_LOG2( 10, StrFmt( "%ul", checkSum ), 0, true );
		theShellsStore.UpdateCheckSum( &checkSum );

		//CPtr<IBinSaver> pCheckSumSaver = CreateCheckSumSaver( &checkSum, pCheckSumLog, curTime );
		//GetTerrain()->operator&( *pCheckSumSaver );
		//GetHeights()->operator&( *pCheckSumSaver );

		pCheckSumLog->AddChecksumLog( curTime, checkSum, 0 );
	}
	else
	{
		if ( pCheckSumLog )
		{
			//DebugTrace( "curTime %i", curTime );
			CPtr<IBinSaver> pCheckSumSaver = CreateCheckSumSaver( &checkSum, pCheckSumLog, curTime );
			this->operator&( *pCheckSumSaver );
		}
	}
	if ( bSend )
	{
		//CONSOLE_BUFFER_LOG2( PIPE_A7_MULTIPLAYER_CHECK, StrFmt( "%ul", checkSum ), 0, false );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::Segment()
{
	// set control word for FP co-processor
	// _EM_INVALID | _EM_ZERODIVIDE | _EM_OVERFLOW | _EM_UNDERFLOW | _EM_INEXACT | _EM_DENORMAL | _PC_24
	// 0xa001f
	_control87( _EM_INVALID | _EM_ZERODIVIDE | _EM_OVERFLOW | _EM_UNDERFLOW | _EM_INEXACT | _EM_DENORMAL | _PC_24, 0xfffff );
	//
	if ( !bSuspended )
	{
		bSegment = true;
		curTime = CAITimer::GetSegmentTime();
		scripts.Segment();

		updater.UpdateTime( curTime );

		theGroupLogic.Segment();
		theExecutorContainer.Segment();

		if ( theCheats.GetWarFog() )
			theWarFog.Segment();

		theGraveyard.Segment();
		theWeather.Segment();

		theHitsStore.Segment();
		CLinkObject::Segment();
		theStatObjs.Segment();
		theBridgeHeightsRemover.Segment();
		theAAManager.Segment();
		garbage.clear();
		updater.ClearPlacementUpdates();
		theUnitCreation.Segment();
		theReinfArray.Segment();
		theCombatEstimator.Segment();
		theSupremeBeing.Segment();
		theFeedBackSystem.Segment();
		if ( theDipl.IsNetGame() )
		{
			//DebugTrace( "======== AILogic Segment time %d", curTime );
			if ( NGlobal::GetVar( "count_checksum", 0 ) && NGlobal::GetVar( "History.Playing", 0 ) == 0 )
			{
				if ( curTime >= nextCheckSumTime )
				{
					UpdateCheckSum( true );
					nextCheckSumTime = curTime + periodToCheckSum;
				}
			}
		}
		bFirstTime = false;

		bSegment = false;

		if ( curTime == SConsts::SHOW_ALL_TIME_COEFF * SConsts::AI_SEGMENT_DURATION )
			updater.AddUpdate( EFB_ASK_FOR_WARFOG );

		theBalanceTest.SegmentBalanceTest();

		if ( curTime >= timeLocalPlayerUnitCheck )
		{
			bool bPresent = false;
			for ( CGlobalIter iter( theDipl.GetMyParty(), EDI_FRIEND ); !iter.IsFinished(); iter.Iterate() )
			{
				CAIUnit *pUnit = *iter;
				if ( pUnit->IsRefValid() && pUnit->IsAlive() && pUnit->GetPlayer() == theDipl.GetMyNumber() )
				{
					bPresent = true;
					break;
				}
			}
			if ( bPresent != bLocalPlayerUnitsPresent )
				updater.AddUpdate( EFB_LOCAL_PLAYER_UNITS_PRESENT, bPresent, 0 );
			bLocalPlayerUnitsPresent = bPresent;
			timeLocalPlayerUnitCheck = curTime + 500 + NRandom::Random( 1000 );
		}

#ifdef FPS_TEST
		++nSegmentCount;
		if ( nSegmentCount == 0 )
		{
			nStartFrame = nDGCurrentFrame;
			dwStartTickCount = ::GetTickCount();

			nSegmentNextTick = nSegmentCount + nSegemntsInTick;
			nTickStartFrame = nDGCurrentFrame;
			dwTickStartTickCount = ::GetTickCount();

			nSegmentStop = NGlobal::GetVar( "FPS_TEST_DURATION", 0 );
			NI_ASSERT( nSegmentStop, "FPS_TEST #define'ed but FPS_TEST_DURATION not" );
			//VTResume();
		}
		if ( nSegmentStop == nSegmentCount )
		{
			DebugTrace( "[FPS Test]: Average FPS %2.3f (segments: %d)", 1000.f*(float)(nDGCurrentFrame - nStartFrame)/(float)( ::GetTickCount() - dwStartTickCount ), nSegmentCount );
			NInput::PostEvent( "exit", 0, 0 );
		}
		if ( nSegmentNextTick == nSegmentCount )
		{
			DebugTrace( "[FPS Test]: Average FPS %2.3f (segments: %d .. %d )", 1000.f*(float)(nDGCurrentFrame - nTickStartFrame)/(float)( ::GetTickCount() - dwTickStartTickCount ), nSegmentCount - nSegemntsInTick, nSegmentCount );
			nSegmentNextTick = nSegmentCount + nSegemntsInTick;
			nTickStartFrame = nDGCurrentFrame;
			dwTickStartTickCount = ::GetTickCount();
			//theWarFog.DumpWarFog();
			//updater.DumpSizes();
		}
		if ( nSegmentCount == 240 )
			VTResume();
		if ( nSegmentCount == 300 )
			VTPause();
#endif
	}

//	CheckAIObjectBase();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::UnitCommand( SAIUnitCmd *pCommand, const WORD wGroupID, const int nPlayer )
{
	curTime = CAITimer::GetSegmentTime();
	switch( pCommand->nCmdType )
	{
	case ACTION_COMMAND_ORDER:
		theReinfArray[nPlayer].AddCallReinforcementCommand( NDb::EReinforcementType( pCommand->nNumber ), pCommand->vPos );
		break;
	case ACTION_COMMAND_USE_SUPER_WEAPON:
		//NSuperWeapon::SuperWeaponUse( nPlayer, pCommand->vPos );
		break;
	case ACTION_COMMAND_PLACE_MARKER:
		if ( theDipl.GetMyParty() == theDipl.GetNParty( nPlayer ) )
		{
			DebugTrace( "+++ Marker at %f,%f", pCommand->vPos.x, pCommand->vPos.y );
			//Send update
			updater.AddUpdate( EFB_PLACE_MARKER, MAKELONG( pCommand->vPos.x, pCommand->vPos.y ) );
		}
		break;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int CAILogic::GenerateGroupNumber()
{
	return theGroupLogic.GenerateGroupNumber();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::RegisterGroup( const vector<int> &vIDs, const int nGroup )
{
	theGroupLogic.RegisterGroup( vIDs, nGroup );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::RegisterGroup( CObjectBase **pUnitsBuffer, const int nLen, const WORD wGroup )
{
	theGroupLogic.RegisterGroup( pUnitsBuffer, nLen, wGroup );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::UnregisterGroup( const int nGroup )
{
	theGroupLogic.UnregisterGroup( nGroup );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::GroupCommand( SAIUnitCmd *pCommand, const WORD wGroup, bool bPlaceInQueue )
{
	theGroupLogic.GroupCommand( *pCommand, wGroup, bPlaceInQueue );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::CallScriptFunction( const char *pszCommand )
{
	scripts.CallScriptFunction( pszCommand );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAILogic::CanShowVisibilities() const
{
	return curTime >= SConsts::AI_SEGMENT_DURATION * SConsts::SHOW_ALL_TIME_COEFF;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const bool CAILogic::GetMiniMapUnitsInfo( vector< SMiniMapUnitInfo > &vUnits )
{
	if ( curTime >= timeLastMiniMapUpdateUnits && ( !theDipl.IsNetGame() || bNetGameStarted ) )
	{
		//DebugTrace( "GameTimer()->GetPauseType() = %d, curTime = %d, timeLastMiniMapUpdateUnits = %d, theDipl.IsNetGame() = %s, bNetGameStarted = %s", GameTimer()->GetPauseType(), curTime, timeLastMiniMapUpdateUnits, theDipl.IsNetGame() ? "true" : "false", bNetGameStarted ? "true" : "false" );
		vUnits.resize( 0 );
		for ( CGlobalIter iter( 0, ANY_PARTY ); !iter.IsFinished(); iter.Iterate() )
		{
			CAIUnit *pUnit = *iter;
			if ( IsValidObj( pUnit ) && pUnit->IsAlive() )
			{
				const CVec2 vCenter( pUnit->GetCenterPlain() );
				const SUnitBaseRPGStats *pStats = pUnit->GetStats();

				if ( pStats && pStats->IsAviation() || GetAIMap()->IsPointInside( vCenter ) && ( false && !pUnit->IsInSolidPlace() || 
					pUnit->GetParty() != theCheats.GetNPartyForWarFog() && pUnit->IsVisible( theCheats.GetNPartyForWarFog() ) ||
					pUnit->IsVisibleByPlayer() ) )
				{
					const EUnitStateNames eName = pUnit->GetState()->GetName();
					if ( eName != EUSN_REST_ON_BOARD && eName != EUSN_MECHUNIT_REST_ON_BOARD )
					{
						float z = pUnit->GetCenter().z;
						vUnits.push_back( SMiniMapUnitInfo( vCenter.x, vCenter.y, z, pUnit->GetPlayer(), pUnit->GetBoundTileRadius() ) );
					}
				}
			}
		}
		timeLastMiniMapUpdateUnits = curTime + SConsts::AI_SEGMENT_DURATION;
		return true;
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAILogic::GetMiniMapWarForInfo( CArray2D<BYTE> **pWarFogInfo, bool bFirstTime )
{
	if ( CanShowVisibilities() && ( !theDipl.IsNetGame() || bNetGameStarted ) )
		return theWarFog.GetWarForInfo( pWarFogInfo, theCheats.GetNPartyForWarFog(), bFirstTime );
	else
		return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CAILogic::GetMiniMapWarFogSizeX() const
{
	return theWarFog.GetSizeX();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CAILogic::GetMiniMapWarFogSizeY() const
{
	return theWarFog.GetSizeY();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::ShowAreas( const vector<int> &units, EActionNotify &eType, bool bShow )
{
	if ( bShow && units.size() > 0 )
	{
		//Generate group number
//		int nGroup = theGroupLogic.GenerateGroupNumber();
//		theGroupLogic.RegisterGroup( units, nGroup );
		updater.UpdateAreasGroup( true, eType );
		theGroupLogic.UpdateAllAreas( units, eType );
	}
	else
		updater.UpdateAreasGroup( false, ACTION_NOTIFY_NONE );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::GetShootAreas( int nUnitID, SShootAreas *pAreas )
{
	SShootAreas unitAreas;
	int nAreas;
	CLinkObject *pLinkObj = CLinkObject::GetObjectByUniqueId( nUnitID );
	if ( !pLinkObj )
		return;

	CAIUnit *pAIUnit = dynamic_cast<CAIUnit *>( pLinkObj );
	if ( pAIUnit )
	{
		pAIUnit->GetShootAreas( &unitAreas, &nAreas );
		for ( list<SShootArea>::iterator it = unitAreas.areas.begin(); it != unitAreas.areas.end(); ++it )
			pAreas->areas.push_back( *it );
	}
	else
	{
		CFormation *pFormation = dynamic_cast<CFormation *>( pLinkObj );
		if ( !pFormation )
			return;

		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			(*pFormation)[i]->GetShootAreas( &unitAreas, &nAreas );
			for ( list<SShootArea>::iterator it = unitAreas.areas.begin(); it != unitAreas.areas.end(); ++it )
				pAreas->areas.push_back( *it );
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::RequestBuildPreview( const EActionCommand eBuildCommand, const CVec2 &vStart, const CVec2 &vEnd, bool bFinished )
{
	theUnderConstructionObject.ShowUnderConstruction( eBuildCommand, vStart, vEnd, bFinished, this );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAILogic::IsCombatSituation()
{
	return theCombatEstimator.IsCombatSituation();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::ToGarbage( CCommonUnit *pUnit )
{
	garbage.push_back( pUnit );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CAILogic::GetUniqueIDOfObject( CObjectBase *pObj )
{
	NI_ASSERT( dynamic_cast<CLinkObject*>(pObj) != 0, StrFmt("Wrong object of type \"%s\" - CLinkObject expected", typeid(*pObj).name()) );

	return checked_cast<CLinkObject*>(pObj)->GetUniqueId();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CObjectBase* CAILogic::GetObjByUniqueID( const int id )
{
	return GetObjectByUniqueIdSafe<CObjectBase>( id );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::SetMyDiplomacyInfo( const int nParty, const int nNumber )
{
	if ( nNumber != -1 )
	{
		theDipl.SetMyNumber( nNumber );
		if ( nParty != -1 )
			theDipl.SetParty( theDipl.GetMyNumber(), nParty );
	}
	
	if ( nParty != -1 )
		theCheats.SetNPartyForWarFog( nParty, true );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::SetNPlayers( const int nPlayers )
{
	theDipl.SetNPlayers( nPlayers );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::SetNetGame( const bool bNetGame )
{
	theDipl.SetNetGame( bNetGame );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAILogic::SubstituteUniqueIDs( const vector<int> &vIDs )
{
	typedef CObjectBase* LPObjectBase;
	LPObjectBase *objects = new LPObjectBase[ vIDs.size() ];
	for ( int i = 0; i < vIDs.size(); ++i )
		objects[i] = (CObjectBase *)CLinkObject::GetObjectByUniqueIdSafe( vIDs[i] );
	bool result = SubstituteUniqueIDs( objects, vIDs.size() );
	delete[] objects;
	return result;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAILogic::SubstituteUniqueIDs( CObjectBase **pUnitsBuffer, const int nLen )
{
	bool bCorrect = true;
	
	for ( int i = 0 ; i < nLen; ++i )
	{
		if ( pUnitsBuffer[i] == 0 || dynamic_cast<CLinkObject*>(pUnitsBuffer[i]) == 0 )
		{
			CONSOLE_BUFFER_LOG2(
				CONSOLE_STREAM_CONSOLE, 
				("Wrong object of type \"%s\" - CLinkObject expected", typeid(*pUnitsBuffer[i]).name()),
				0xffff0000, true );

			pUnitsBuffer[i] = 0;			
			bCorrect = false;
		}
		else
		{
			CLinkObject *pObj = checked_cast<CLinkObject*>( pUnitsBuffer[i] );
			pUnitsBuffer[i] = reinterpret_cast<CObjectBase*>( pObj->GetUniqueId() );
		}
	}

	return bCorrect;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAILogic::UpdateAcknowledgment( SAIAcknowledgment &pAck )
{
	return theAckManager.UpdateAcknowledgment( pAck );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAILogic::UpdateAcknowledgment( SAIBoredAcknowledgement &pAck )
{
	return theAckManager.UpdateAcknowledgment( pAck );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float CAILogic::GetZ( const CVec2 &vPoint ) const
{
	return GetHeights()->GetVisZ( vPoint.x, vPoint.y );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const DWORD CAILogic::GetNormal( const CVec2 &vPoint ) const
{
	return GetHeights()->GetNormal( vPoint.x, vPoint.y );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const bool CAILogic::GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const
{
	return GetHeights()->GetIntersectionWithTerrain( pvResult, vBegin, vEnd );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAILogic::ToggleShow( const int nShowType )
{
	theCheats.SetTurnOffWarFog( !theCheats.GetTurnOffWarFog() );
	return theCheats.GetTurnOffWarFog();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::SetDifficultyLevel( const int nLevel )
{
	if ( !theDipl.IsNetGame() )
		theDifficultyLevel.SetLevel( nLevel );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::SetCheatDifficultyLevel( const int nCheatLevel )
{
	if ( !theDipl.IsNetGame() )
		theDifficultyLevel.SetCheatLevel( nCheatLevel );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::NetGameStarted()
{
	bNetGameStarted = true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAILogic::IsNetGameStarted() const
{
	return bNetGameStarted;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CDifficultyLevel* CAILogic::GetDifficultyLevel() const
{
	return &theDifficultyLevel;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::NeutralizePlayer( const int nPlayer )
{
	if ( theDipl.IsPlayerExist( nPlayer ) )
	{
		int nBestPlayer = theDipl.GetNeutralPlayer();
		float fBestPrice = -1.0f;
		for ( int i = 0; i < theDipl.GetNPlayers(); ++i )
		{
			if ( i != nPlayer && theDipl.IsPlayerExist( i ) && theDipl.GetDiplStatus( i, nPlayer ) == EDI_FRIEND )
			{
				//const float fLocalPrice = theStatistics.GetValue( STMT_PLAYER_EXPERIENCE, nPlayer );
				//if ( fLocalPrice > fBestPrice )
				{
					//fBestPrice = fLocalPrice;
					nBestPlayer = i;
				}
			}
		}
		
		list<CCommonUnit*> playerUnits;
		for ( CGlobalIter iter( theDipl.GetNParty( nPlayer ), EDI_FRIEND ); !iter.IsFinished(); iter.Iterate() )
		{
			CCommonUnit *pUnit = *iter;
			if ( pUnit->GetPlayer() == nPlayer )
				playerUnits.push_back( pUnit );
		}

		for ( list<CCommonUnit*>::iterator iter = playerUnits.begin(); iter != playerUnits.end(); ++iter )
		{
			CCommonUnit *pUnit = *iter;
			theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP ), pUnit, false );

			pUnit->ChangePlayer( nBestPlayer );
		}

		theDipl.SetPlayerNotExist( nPlayer );

		if ( nBestPlayer != theDipl.GetNeutralPlayer() )
	 		updater.AddUpdate( EFB_TROOPS_PASSED, MAKELONG( nPlayer, nBestPlayer ) );
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CObjectBase* CAILogic::GetUnitState( CObjectBase *pObj )
{
	if ( CQueueUnit *pUnit = dynamic_cast<CQueueUnit*>(pObj) )
		return pUnit->GetState();
	else
		return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAILogic::IsFrozen( CObjectBase *pObj ) const
{
	if ( CCommonUnit *pUnit = dynamic_cast<CCommonUnit*>(pObj) )
		return pUnit->CanBeFrozen();
	else
		return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CAILogic::IsFrozenByState( CObjectBase *pObj ) const
{
	if ( CCommonUnit *pUnit = dynamic_cast<CCommonUnit*>(pObj) )
		return pUnit->IsFrozenByState();
	else
		return false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::GetGridUnitsCoordinates( const int nGroup, const CVec2 &vGridCenter, CVec2 **pCoord, int *pnLen )
{
	CGrid grid( vGridCenter, nGroup, CVec2( 1.0f, 0.0f ) );

	*pnLen = grid.GetNUnitsInGrid();
	*pCoord = GetLocalTempBuffer<CVec2>( *pnLen );
	for ( int i = 0; i < *pnLen; ++i )
		(*pCoord)[i] = grid.GetUnitCenter( i ) - vGridCenter;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::Suspend() 
{ 
	bSuspended = true; 
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::Resume() 
{ 
	bSuspended = false; 
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CPtr<SAIBasicUpdate> pUpdate = 0;

CObjectBase* CAILogic::GetUpdate()
{
	pUpdate = updater.GetUpdate();
	return pUpdate;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::PrepareUpdates()
{
	updater.PrepareUpdates();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::SetGlobeScriptHandler( IGlobeScriptHandler *pHandler )
{
	scripts.SetGlobeScriptHandler( pHandler );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int CAILogic::WAR_FOG_FULL_UPDATE() const
{ 
	return SAIConsts::WAR_FOG_FULL_UPDATE; 
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int CAILogic::VIS_POWER() const
{ 
	return SAIConsts::VIS_POWER; 
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CAILogic::GetUniqueID( CObjectBase *pObj ) 
{ 
	if ( CLinkObject *pLinkObj = dynamic_cast<CLinkObject*>(pObj) ) 
		return pLinkObj->GetUniqueId();
	else
		return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CStaticMapHeights* CAILogic::GetHeights() const
{
	return ::GetHeights();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int CAILogic::GetPlayerDiplomacy( const int nPlayer ) const
{
	return theDipl.GetNParty( nPlayer );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const int CAILogic::GetUnitCount( const int nPlayer ) const
{
	int cnt = 0;
	for ( CGlobalIter iter( GetPlayerDiplomacy( nPlayer ), EDI_FRIEND ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( pUnit->IsRefValid() && pUnit->IsAlive() && pUnit->GetPlayer() == nPlayer )
		{
			++cnt;
		}
	}
	return cnt;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const bool CAILogic::HasPlayerNoUnits( const int nPlayer ) const
{
	NI_VERIFY( nPlayer >= 0 && nPlayer < theReinfArray.size(), "Index out of range", return false ); // CRAP - dont't use AILogic directly at Client

	if ( theReinfArray[nPlayer].CanCallNow() )
		return false;

	for ( CGlobalIter iter( GetPlayerDiplomacy( nPlayer ), EDI_FRIEND ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( pUnit->IsRefValid() && pUnit->IsAlive() )
		{
			return false;
		}
	}

	return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool ConverWStingToInt( int *pValue, const wstring &wszString, const int nMin, const int nMax )
{
	string szParam;

	NStr::ToMBCS( &szParam, wszString );
	if ( !NStr::IsDecNumber( szParam ) )
	{
		csSystem << "error: invalid radius, must be integer value" << endl;
		return false;
	}
	*pValue = NStr::ToInt( szParam );
	if ( nMin != nMax && ( *pValue < nMin || *pValue >= nMax ) )
	{
		csSystem << "error: invalid radius, must be integer value between " << nMin << " and " << nMax <<endl;
		return false;
	}
	return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void PathfinderTest( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	const int nMaxX = GetAIMap()->GetSizeX();
	const int nMaxY = GetAIMap()->GetSizeY();
	SVector vStartTile( NRandom::Random( 0, nMaxX ), NRandom::Random( 0, nMaxY ) );
	SVector vFinishTile( NRandom::Random( 0, nMaxX ), NRandom::Random( 0, nMaxY ) );

	for ( int i = 0; i < 500; ++i )
	{
		const int dwTickStart = GetTickCount();

		CCommonPathFinder *pPathFinder = Singleton<CCommonPathFinder>();
		pPathFinder->SetPathParameters( 0, EAC_HUMAN, GetAIMap()->GetPointByTile( vStartTile ), GetAIMap()->GetPointByTile( vFinishTile ), vStartTile, GetAIMap() );
		pPathFinder->DoesPathExist();
    const int dwTickCount = GetTickCount() - dwTickStart;
		if ( dwTickCount > 5 )
		{
			//csSystem << "prolonged (" << dwTickCount << ") path searching from " << vStartTile.x << " x " << vStartTile.y << " to " << vFinishTile.x << " x " << vFinishTile.y << endl;
		}
		else
		{
			SVector vStartTile( NRandom::Random( 0, nMaxX ), NRandom::Random( 0, nMaxY ) );
			SVector vFinishTile( NRandom::Random( 0, nMaxX ), NRandom::Random( 0, nMaxY ) );
		}
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void CommandCheckPath( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.size() != 7 )
	{
		csSystem << "usage: " << szID << "file_name start_tile finish_tile ai_class radius" << endl;
		csSystem << "\tfile_name:\tname for .tga file with maxes (file will be saved in debug_images folder with .tga extension)" << endl;
		csSystem << "\tstart_tile:\tfind path from this tile (x y)" << endl;
		csSystem << "\tfinish_tile:\tfind path to this tile (x y)" << endl;
		csSystem << "\tai_class:\t\tHUMAN, WHEEL, TRACK, SEA, RIVER, TERRAIN, WATER, ANY" << endl;
		csSystem << "\tradius:\t\t\tBound tile radius" << endl;
		return;
	}

	SVector vStartTile( 0, 0 );
	if ( !ConverWStingToInt( &(vStartTile.x), paramsSet[1], 0, GetAIMap()->GetSizeX() ) )
		return;
	if ( !ConverWStingToInt( &(vStartTile.y), paramsSet[2], 0, GetAIMap()->GetSizeY() ) )
		return;

	SVector vFinishTile( 0, 0 );
	if ( !ConverWStingToInt( &(vFinishTile.x), paramsSet[3], 0, GetAIMap()->GetSizeX() ) )
		return;
	if ( !ConverWStingToInt( &(vFinishTile.y), paramsSet[4], 0, GetAIMap()->GetSizeY() ) )
		return;

	string szParam;

	NStr::ToMBCS( &szParam, paramsSet[5] );
	NStr::ToLowerASCII( &szParam );
	EAIClasses aiClass = EAC_NONE;
	if (szParam == "human" )
		aiClass = EAC_HUMAN;
	else if ( szParam == "wheel" )
		aiClass = EAC_WHELL;
	else if ( szParam == "track" )
		aiClass = EAC_TRACK;
	else if ( szParam == "river" )
		aiClass = EAC_RIVER;
	else if ( szParam == "sea" )
		aiClass = EAC_SEA;
	else if ( szParam == "terrain" )
		aiClass = EAC_TERRAIN;
	else if ( szParam == "water" )
		aiClass = EAC_WATER;
	else if ( szParam == "any" )
		aiClass = EAC_ANY;

	int nBoundTileRadius = 0;
	if ( !ConverWStingToInt( &nBoundTileRadius, paramsSet[6], 0, GetAIMap()->GetMaxUnitTileRadius() ) )
		return;

	STerrainModeSetter modeSetter( ELM_STATIC, GetAIMap()->GetTerrain() );
	CCommonPathFinder *pPathFinder = Singleton<CCommonPathFinder>();
	pPathFinder->SetPathParameters( nBoundTileRadius, aiClass, GetAIMap()->GetPointByTile( vStartTile ), GetAIMap()->GetPointByTile( vFinishTile ), vStartTile, GetAIMap() );
	CPtr<IStaticPath> pPath = pPathFinder->CreatePath( true );
	vector<SVector> realPath;
	vector<SVector> smoothPath;

	if ( !pPathFinder->IsPathFound() )
	{
		csSystem << "PATH NOT FOUND !!!" << endl;

		smoothPath.push_back( vStartTile );
		smoothPath.push_back( vFinishTile );
	}
	else
	{
		for ( int i = 0; i < pPath->GetLength(); ++i )
			smoothPath.push_back( pPath->GetTile( i ) );
	}

	pPathFinder->SetPathParameters( nBoundTileRadius, aiClass, GetAIMap()->GetPointByTile( vStartTile ), GetAIMap()->GetPointByTile( vFinishTile ), vStartTile, GetAIMap() );
	pPathFinder->DoesPathExist();
	if ( pPathFinder->GetPathLength() > 1 )
	{
		realPath.resize( pPathFinder->GetPathLength() );
		pPathFinder->GetStopPoints( &(realPath[0]), pPathFinder->GetPathLength() );
		for ( int i = 1; i < pPathFinder->GetPathLength(); ++i )
		{
			if ( !GetAIMap()->IsTileInside( realPath[i] ) )
				realPath[i] = realPath[i-1];
		}
	}


	NStr::ToMBCS( &szParam, paramsSet[0] );
	NStr::ToLowerASCII( &szParam );
	GetAIMap()->GetTerrain()->DumpMaxes( ELM_STATIC, aiClass, "debug_images\\" + szParam + ".tga", realPath, smoothPath, true );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void DumpWarFogHeights( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
	if ( paramsSet.size() != 5 )
	{
		csSystem << "usage: " << szID << "file_name start_point finish_point" << endl;
		csSystem << "\tfile_name:\tname for .tga file with trace result (file will be saved in debug_images\\warfog folder with .tga extension)" << endl;
		csSystem << "\tstart_point:\tstart trace from this point" << endl;
		csSystem << "\tfinish_point:\tfinish trace at this point" << endl;
		return;
	}

	SVector vStartTile( 0, 0 );
	if ( !ConverWStingToInt( &(vStartTile.x), paramsSet[1], 0, GetAIMap()->GetSizeX()*GetAIMap()->GetTileSize() ) )
		return;
	if ( !ConverWStingToInt( &(vStartTile.y), paramsSet[2], 0, GetAIMap()->GetSizeY()*GetAIMap()->GetTileSize() ) )
		return;

	SVector vFinishTile( 0, 0 );
	if ( !ConverWStingToInt( &(vFinishTile.x), paramsSet[3], 0, GetAIMap()->GetSizeX()*GetAIMap()->GetTileSize() ) )
		return;
	if ( !ConverWStingToInt( &(vFinishTile.y), paramsSet[4], 0, GetAIMap()->GetSizeY()*GetAIMap()->GetTileSize() ) )
		return;

	const CVec2 vStartPoint( vStartTile.x, vStartTile.y );
	const CVec2 vFinishPoint( vFinishTile.x, vFinishTile.y );

	string szParam;
	NStr::ToMBCS( &szParam, paramsSet[0] );
	NStr::ToLowerASCII( &szParam );
	//theWarFog.DebugTraceRay( "debug_images\\warfog\\" + szParam + ".tga", vStartPoint, vFinishPoint );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CAILogic::DumpAfterAssinc() const
{
	GetTerrain()->DumpMaxes( ELM_ALL, EAC_WHELL, "EAC_WHELL.tga" );
	GetTerrain()->DumpMaxes( ELM_ALL, EAC_TRACK, "EAC_TRACK.tga" );
	GetTerrain()->DumpMaxes( ELM_ALL, EAC_HUMAN, "EAC_HUMAN.tga" );
	GetTerrain()->DumpMaxes( ELM_ALL, EAC_TERRAIN, "EAC_TERRAIN.tga" );

	GetTerrain()->DumpMaxes( ELM_STATIC, EAC_WHELL, "EAC_WHELL_s.tga" );
	GetTerrain()->DumpMaxes( ELM_STATIC, EAC_TRACK, "EAC_TRACK_s.tga" );
	GetTerrain()->DumpMaxes( ELM_STATIC, EAC_HUMAN, "EAC_HUMAN_s.tga" );
	GetTerrain()->DumpMaxes( ELM_STATIC, EAC_TERRAIN, "EAC_TERRAIN_s.tga" );

	//GetTerrain()->DumpLockInfo();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
START_REGISTER( AICheckers )

REGISTER_CMD( "check_path", CommandCheckPath )
REGISTER_CMD( "pathfinder_test", PathfinderTest );
REGISTER_CMD( "trace_warfog", DumpWarFogHeights )

FINISH_REGISTER
