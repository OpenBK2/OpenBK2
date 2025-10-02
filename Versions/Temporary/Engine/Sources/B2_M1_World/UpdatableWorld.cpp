#include "StdAfx.h"

#include "AllAnimationsPlayer.h"
#include "ClientAckManager.h"
#include "CommonB2M1AI.h"
#include "MOBridge.h"
#include "MOBuilding.h"
#include "MOEntrenchment.h"
#include "MOFence.h"
#include "MOObject.h"
#include "MOProjectile.h"
#include "MOSquad.h"
#include "MOUnitInfantry.h"
#include "MOUnitHelicopter.h"
#include "MOUnitMechanical.h"
#include "UpdatableWorld.h"

#include "../DebugTools/DebugInfoManager.h"
#include "../Main/GameTimer.h"
#include "../Stats_B2_M1/AbilityActions.h"
#include "../Stats_B2_M1/AIAckTypes.h"
#include "../Stats_B2_M1/FeedBackUpdates.h"
#include "../Sound/SoundScene.h"
#include "../Stats_B2_m1/ReinfUpdates.h"
#include "../Stats_B2_M1/M1UnitSpecific.h"
#include "../Stats_B2_M1/DBAnimB2.h"
#include "../Sound/MusicSystem.h"
#include "../SceneB2/CameraScriptMutators.h"
#include "../SceneB2/DBSceneConsts.h"

#include "../3DLib/GMemBuilder.h"
#include "../3DLib/MemObject.h"
#include "../system/FastMath.h"
#include "../SceneB2/LaserMark.h"

#include "../Sound/DBMusicSystem.h"

#include "../Stats_B2_M1/StatusUpdates.h"
#include "../Stats_B2_M1/SuperWeaponUpdates.h"

//#include "../Stats_B2_M1/DBMapInfo.h"

//CRAP{ for profiling
#ifdef _PROFILER
#include <VTuneAPI.h>
#pragma comment( lib, "vtuneapi.lib" )
#endif // _PROFILER
//CRAP}


#define REGISTER_UPDATE( RPGName, Name ) newFuncs[RPGName::typeID] = Name##::New##Name

#define SELECTION_DELTA 90.0f

const int CLIENT_UNIQUE_ID_UNDER_CONSTRUCTION_LIST = -10; // look for other CLIENT_UNIQUE_ID_xxx (криво, но менять поздно)

static list< CPtr<IClientUpdatableProcess> > processesToUpdate;

// ************************************************************************************************************************ //
// **
// ** functions to extract unique ID from update
// **
// **
// **
// ************************************************************************************************************************ //

template <class TUpdate>
inline const int GetIDFromInfo( const SAIBasicUpdate *_pUpdate )
{
	const TUpdate *pUpdate = checked_cast<const TUpdate*>( _pUpdate );
	return pUpdate->info.nObjUniqueID;
}
template <class TUpdate>
inline const int GetID2( const SAIBasicUpdate *_pUpdate )
{
	const TUpdate *pUpdate = checked_cast<const TUpdate*>( _pUpdate );
	return pUpdate->nObjUniqueID;
}
template <>
inline const int GetID2<SAISpecialAbilityUpdate>( const SAIBasicUpdate *pUpdate )
{
	return GetIDFromInfo<SAISpecialAbilityUpdate>( pUpdate );
}
template <>
inline const int GetID2<SAIDiplomacyUpdate>( const SAIBasicUpdate *pUpdate )
{
	return GetIDFromInfo<SAIDiplomacyUpdate>( pUpdate );
}
template <>
inline const int GetID2<SAIMechShotUpdate>( const SAIBasicUpdate *pUpdate )
{
	return GetIDFromInfo<SAIMechShotUpdate>( pUpdate );
}
template <>
inline const int GetID2<SAIInfantryShotUpdate>( const SAIBasicUpdate *pUpdate )
{
	return GetIDFromInfo<SAIInfantryShotUpdate>( pUpdate );
}
template <>
inline const int GetID2<SAITurretUpdate>( const SAIBasicUpdate *pUpdate )
{
	return GetIDFromInfo<SAITurretUpdate>( pUpdate );
}
template <>
inline const int GetID2<SAIRPGUpdate>( const SAIBasicUpdate *pUpdate )
{
	return GetIDFromInfo<SAIRPGUpdate>( pUpdate );
}
template <>
inline const int GetID2<SAIPlacementUpdateBase>( const SAIBasicUpdate *pUpdate )
{
	return GetIDFromInfo<SAIPlacementUpdateBase>( pUpdate );
}
template <>
inline const int GetID2<SAINewUnitUpdate>( const SAIBasicUpdate *pUpdate )
{
	return GetIDFromInfo<SAINewUnitUpdate>( pUpdate );
}

// ************************************************************************************************************************ //
// **
// ** updatable world itself
// **
// **
// **
// ************************************************************************************************************************ //

CCombatMusic::CCombatMusic()
: eState( ESSS_IDLE ), timeLastCombatNotify( 0 )
{
	combatPlayWONotify = NGlobal::GetVar( "Scene.Sound.StreamingSounds.CombatMusicPlayWONotify", 30000);
}

void CCombatMusic::Update( bool bCombatNotify )
{
	if ( 1 >= Singleton<IMusicSystem>()->GetNPlayLists() )
		return;

	const NTimer::STime curTime = Singleton<IGameTimer>()->GetGameTime();
	
	if ( bCombatNotify )
		timeLastCombatNotify = curTime;

	switch ( eState )
	{
	case ESSS_IDLE: 
		if ( bCombatNotify )
		{
			timeLastCombatNotify = curTime;
			eState = ESSS_TO_COMBAT;
			Singleton<IMusicSystem>()->ChangePlayList( 1 );
		}

		break;
	case ESSS_TO_COMBAT:
		if ( 1 == Singleton<IMusicSystem>()->GetPlayList() )
			eState = ESSS_COMBAT;

		break;
	case ESSS_COMBAT:
		if ( curTime - timeLastCombatNotify > combatPlayWONotify )
		{
			eState = ESSS_TO_IDLE;
			Singleton<IMusicSystem>()->ChangePlayList( 0 );
		}

		break;
	case ESSS_TO_IDLE:
		if ( 0 == Singleton<IMusicSystem>()->GetPlayList() )
			eState = ESSS_IDLE;
		else if ( curTime - timeLastCombatNotify < combatPlayWONotify && Singleton<IMusicSystem>()->CanChangePlayList() )
		{
			timeLastCombatNotify = curTime;
			eState = ESSS_TO_COMBAT;
			Singleton<IMusicSystem>()->ChangePlayList( 1 );
		}

		break;
	}

	bCombatNotify = false;
}

// ************************************************************************************************************************ //
// **
// ** updatable world itself
// **
// **
// **
// ************************************************************************************************************************ //

CUpdatableWorld::CUpdatableWorld()
{
	InitPrivate();
}

CUpdatableWorld::CUpdatableWorld( IVisualNotifications *_pNotifications, ICommonB2M1AI *_pAI )
{
	InitPrivate();
	pAI = _pAI;
	pNotifications = _pNotifications;
	bReinfEnabled = false;
	reinfTimeRecycleStart = 0;
	reinfTimeRecycleEnd = 0;
	fReinfRecycleProgress = 0.0f;
	nReinfCallsLeft = 0;
}

CUpdatableWorld::~CUpdatableWorld()
{
	if ( !bEditor )
	{
		Scene()->ClearScene( SCENE_MISSION );
	}
	pNotifications = 0;
	processesToUpdate.clear();
}

void CUpdatableWorld::InitPrivate()
{
	eSeason = NDb::SEASON_SUMMER;
	processesToUpdate.clear();

	REGISTER_UPDATE( NDb::SBridgeRPGStats, CMOBridge );
	REGISTER_UPDATE( NDb::SBuildingRPGStats, CMOBuilding );
	REGISTER_UPDATE( NDb::SObjectRPGStats, CMOObject );
	REGISTER_UPDATE( NDb::SMineRPGStats, CMOObject );
	REGISTER_UPDATE( NDb::SMechUnitRPGStats, CMOUnitMechanical );
	REGISTER_UPDATE( NDb::SInfantryRPGStats, CMOUnitInfantry );
	REGISTER_UPDATE( NDb::SSquadRPGStats, CMOSquad );
	REGISTER_UPDATE( NDb::SEntrenchmentRPGStats, CMOEntrenchmentPart );
	REGISTER_UPDATE( NDb::SFenceRPGStats, CMOFence );

	// 
	InitEverySegmentFunctions();
	InitUpdateTypeFunctions();
	AckManager()->Init();
	nLastRangeAreasTime = 0;
	eDayTime = NDb::DAY_DAY;
	bEditor = false;
}

void CUpdatableWorld::InitEverySegmentFunctions()
{
	everySegment.push_back( &CUpdatableWorld::UpdateBasicUpdates );
	everySegment.push_back( &CUpdatableWorld::AIUpdateCombatSituationInfo );
	everySegment.push_back( &CUpdatableWorld::UpdateAcknowledgemets );
	everySegment.push_back( &CUpdatableWorld::UpdateClientData );
}

void CUpdatableWorld::UpdateAcknowledgemets()
{
	const bool bNoVisual = NGlobal::GetVar( "temp.no_visual_updates", 0 );
	SAIAcknowledgment ack;
	while ( pAI->UpdateAcknowledgment( ack ) )
	{
		if ( !bNoVisual )
		{
			const int nID = ack.nObjUniqueID;
			CMapObj *pMO = GetMapObj( nID );
			if ( pMO )
			{
				CDynamicCast<IMOUnit> pUnit = pMO;
				if ( pUnit )
					pUnit->AIUpdateAcknowledgement( NDb::EUnitAckType( ack.nAck ), AckManager(), ack.nSet );
				else
				{
					CDynamicCast<IMOSquad> pSquad = pMO;
					if ( pSquad )
					{
						vector<CMOSelectable*> passangers;
						pSquad->GetPassangers( &passangers );
						if ( !passangers.empty() )
						{
							CDynamicCast<IMOUnit> pUnit = passangers[0];
							if ( pUnit )
								pUnit->AIUpdateAcknowledgement( NDb::EUnitAckType( ack.nAck ), AckManager(), ack.nSet );
						}
					}
				}
			}
		}
	}
	SAIBoredAcknowledgement boredAck;
	while ( pAI->UpdateAcknowledgment( boredAck ) )
	{
		if ( !bNoVisual )
		{
			const int nID = boredAck.nObjUniqueID;
			CMapObj *pMO = GetMapObj( nID );
			if ( pMO )
			{
				IMOUnit *pUnit = checked_cast<IMOUnit*>( pMO );
				pUnit->AIUpdateBoredAcknowledgement( boredAck, AckManager() );
			}
		}
	}
}

void CUpdatableWorld::AIUpdateCombatSituationInfo()
{
	combatMusic.Update( pAI->IsCombatSituation() );
}

void CUpdatableWorld::UpdateBasicUpdates()
{
	const bool bNoVisual = NGlobal::GetVar( "temp.no_visual_updates", 0 );
	pAI->PrepareUpdates();
	while ( CPtr<CObjectBase> pRawUpdate = pAI->GetUpdate() ) 
	{
		if ( !bNoVisual )
		{
			CDynamicCast<SAIBasicUpdate> pUpdate = pRawUpdate;
		
			if ( pUpdate )
			{
				CUpdateType::iterator it = updateType.find( pUpdate->eUpdateType );
				// for compatibility with old updates
				if ( it != updateType.end() )
					(this->*(it->second))( pUpdate);
				else
					pUpdate->ProcessClient( this );
			}
		}
	}
}

void CUpdatableWorld::PerformUpdate( CObjectBase * pRawUpdate )
{
	const bool bNoVisual = NGlobal::GetVar( "temp.no_visual_updates", 0 );
	if ( !bNoVisual )
	{
		CPtr<SAIBasicUpdate> pUpdate = dynamic_cast<SAIBasicUpdate*>( pRawUpdate );

		CUpdateType::iterator it = updateType.find( pUpdate->eUpdateType );
		// for compatibility with old updates
		if ( it != updateType.end() )
			(this->*(it->second))( pUpdate );
		else
			pUpdate->ProcessClient( this );
	}
}

void CUpdatableWorld::UpdateClientData()
{
	const bool bNoVisual = NGlobal::GetVar( "no_visual_updates", 0 );
	if ( !bNoVisual )
	{
		for ( list< CPtr<IClientUpdatableProcess> >::iterator it = processesToUpdate.begin(); it != processesToUpdate.end(); )
		{
			CPtr<IClientUpdatableProcess> pProcess = *it;
			if ( IsValid(pProcess) == false || pProcess->Update(currTime) == false )
				it = processesToUpdate.erase( it );
			else
				++it;
		}
	}
}

void CUpdatableWorld::RegisterProcess( IClientUpdatableProcess *pProcess )
{
	NUpdatableProcess::Register( pProcess );
}

void CUpdatableWorld::InitUpdateTypeFunctions()
{
	updateType[ACTION_NOTIFY_NEW_UNIT]					= &CUpdatableWorld::UpdateNewUnits;
	updateType[ACTION_NOTIFY_NEW_ST_OBJ]				= &CUpdatableWorld::UpdateNewUnits;
	updateType[ACTION_NOTIFY_NEW_BRIDGE_SPAN]		= &CUpdatableWorld::UpdateNewBridgeSpan;
	updateType[ACTION_NOTIFY_NEW_ENTRENCHMENT]	= &CUpdatableWorld::UpdateNewEntrenchment;
	updateType[ACTION_NOTIFY_PLACEMENT]					= &CUpdatableWorld::UpdatePlacement;
	updateType[ACTION_NOTIFY_RPG_CHANGED]				= &CUpdatableWorld::UpdateRPGChanged;
	updateType[ACTION_NOTIFY_CHANGE_SELECTION]	= &CUpdatableWorld::UpdateSelection;
	updateType[ACTION_NOTIFY_SIDE_CHANGED]			= &CUpdatableWorld::UpdateSideChanged;
	updateType[ACTION_NOTIFY_NEW_FORMATION]			= &CUpdatableWorld::UpdateNewFormation;
	updateType[ACTION_NOTIFY_ENTRANCE_STATE]		= &CUpdatableWorld::UpdateEntranceState;
	updateType[ACTION_NOTIFY_TURRET_HOR_TURN]		= &CUpdatableWorld::UpdateTurretTurn;
	updateType[ACTION_NOTIFY_TURRET_VERT_TURN]	= &CUpdatableWorld::UpdateTurretTurn;
	updateType[ACTION_NOTIFY_MECH_SHOOT]				= &CUpdatableWorld::UpdateMechShoot;
	updateType[ACTION_NOTIFY_INFANTRY_SHOOT]		= &CUpdatableWorld::UpdateInfantryShoot;
	updateType[ACTION_NOTIFY_UPDATE_DIPLOMACY]	= &CUpdatableWorld::UpdateDiplomacy;
	updateType[ACTION_NOTIFY_SHOOT_AREA]	      = &CUpdatableWorld::UpdateShootAreas;
	updateType[ACTION_NOTIFY_RANGE_AREA]	      = &CUpdatableWorld::UpdateRangeAreas;	

	updateType[ACTION_NOTIFY_SHOOT_BUILDING]				= &CUpdatableWorld::UpdateShootBuilding;
	updateType[ACTION_NOTIFY_THROW_BUILDING]				= &CUpdatableWorld::UpdateThrowbuilding;
	updateType[ACTION_NOTIFY_HIT]										= &CUpdatableWorld::UpdateHit;
	updateType[ACTION_NOTIFY_DELETED_ST_OBJ]				= &CUpdatableWorld::UpdateDeleteObject;
	updateType[ACTION_NOTIFY_SILENT_DEATH]				  = &CUpdatableWorld::UpdateSilentDeath;
	updateType[ACTION_NOTIFY_DISSAPEAR_OBJ]					= &CUpdatableWorld::UpdateDissapearObj;
	updateType[ACTION_NOTIFY_REVEAL_ARTILLERY]			= &CUpdatableWorld::UpdateRevealArtillery;
	updateType[ACTION_NOTIFY_SET_CAMOUFLAGE]				= &CUpdatableWorld::UpdateSetCamouflage;
	updateType[ACTION_NOTIFY_REMOVE_CAMOUFLAGE]			= &CUpdatableWorld::UpdateRemoveCamouflage;
	updateType[ACTION_NOTIFY_SET_AMBUSH]						= &CUpdatableWorld::UpdateSetAmbush;
	updateType[ACTION_NOTIFY_REMOVE_AMBUSH]					= &CUpdatableWorld::UpdateRemoveAmbush;
	updateType[ACTION_NOTIFY_BREAK_TRACK]						= &CUpdatableWorld::UpdateBreakTrack;
	updateType[ACTION_NOTIFY_REPAIR_TRACK]					= &CUpdatableWorld::UpdateRepairTrack;
	updateType[ACTION_NOTIFY_REPAIR_STATE_BEGIN]		= &CUpdatableWorld::UpdateRepairStateBegin;
	updateType[ACTION_NOTIFY_REPAIR_STATE_END]			= &CUpdatableWorld::UpdateRepairStateEnd;
	updateType[ACTION_NOTIFY_RESUPPLY_STATE_BEGIN]	= &CUpdatableWorld::UpdateResuplyStateBegin;
	updateType[ACTION_NOTIFY_RESUPPLY_STATE_END]		= &CUpdatableWorld::UpdateResuplyStateEnd;
	updateType[ACTION_NOTIFY_CHANGE_VISIBILITY]			= &CUpdatableWorld::UpdateChangeVisibility;
	updateType[ACTION_NOTIFY_STATE_CHANGED]					= &CUpdatableWorld::UpdateStateChanged;
	updateType[ACTION_NOTIFY_SERVED_ARTILLERY]			= &CUpdatableWorld::UpdateServedArtillery;

	updateType[ACTION_SET_SELECTION_GROUP]					= &CUpdatableWorld::UpdateSelectionGroup;
	updateType[ACTION_NOTIFY_FEEDBACK]							= &CUpdatableWorld::UpdateNotifyFeedback;

	updateType[ACTION_NOTIFY_DEAD_UNIT]							= &CUpdatableWorld::UpdateDeadUnit;
	updateType[ACTION_NOTIFY_SPECIAL_ABLITY]				= &CUpdatableWorld::UpdateSpecialAbility;

	updateType[ACTION_NOTIFY_SELECTABLE_CHANGED]    = &CUpdatableWorld::UpdateSelectable;

	updateType[ACTION_NOTIFY_TREE_BROKEN]				= &CUpdatableWorld::UpdateTreeBroken;

	updateType[ACTION_NOTIFY_DELAYED_SHOOT]					= &CUpdatableWorld::UpdateDelayedShoot;
	updateType[ACTION_NOTIFY_STOP]									= &CUpdatableWorld::UpdateStop;
	updateType[ACTION_NOTIFY_MOVE]									= &CUpdatableWorld::UpdateMove;
	updateType[ACTION_NOTIFY_IDLE_TRENCH]						= &CUpdatableWorld::UpdateIdleTrench;

	updateType[ACTION_NOTIFY_AVAIL_REINF]						= &CUpdatableWorld::UpdateReinfTypeAvail;
	updateType[ACTION_NOTIFY_REINF_RECYCLE]					= &CUpdatableWorld::UpdateReinfRecycle;
	updateType[ACTION_NOTIFY_REINF_POINT]						= &CUpdatableWorld::UpdateReinforcmentPoint;
	updateType[ACTION_NOTIFY_ANIMATION_CHANGED]			= &CUpdatableWorld::UpdateAnimationChanged;

	updateType[ACTION_NOTIFY_HEADLIGHT]							= &CUpdatableWorld::UpdateSwitchLightFX;
	
	updateType[ACTION_NOTIFY_MODIFY_ENTRANCE_STATE]		= &CUpdatableWorld::UpdateModifyEntranceState;

	updateType[ACTION_NOTIFY_INSTALL_ROTATE]				= &CUpdatableWorld::UpdateAction;
	updateType[ACTION_NOTIFY_UNINSTALL_ROTATE]			= &CUpdatableWorld::UpdateAction;
	updateType[ACTION_NOTIFY_INSTALL_TRANSPORT]			= &CUpdatableWorld::UpdateAction;
	updateType[ACTION_NOTIFY_UNINSTALL_TRANSPORT]		= &CUpdatableWorld::UpdateAction;
	updateType[ACTION_NOTIFY_INSTALL_MOVE]			    = &CUpdatableWorld::UpdateAction;
	updateType[ACTION_NOTIFY_UNINSTALL_MOVE]		    = &CUpdatableWorld::UpdateAction;
	updateType[ACTION_NOTIFY_FINISH_INSTALL_ROTATE]				= &CUpdatableWorld::UpdateAction;
	updateType[ACTION_NOTIFY_FINISH_UNINSTALL_ROTATE]			= &CUpdatableWorld::UpdateAction;
	updateType[ACTION_NOTIFY_FINISH_INSTALL_TRANSPORT]		= &CUpdatableWorld::UpdateAction;
	updateType[ACTION_NOTIFY_FINISH_UNINSTALL_TRANSPORT]	= &CUpdatableWorld::UpdateAction;
	updateType[ACTION_NOTIFY_FINISH_INSTALL_MOVE]			    = &CUpdatableWorld::UpdateAction;
	updateType[ACTION_NOTIFY_FINISH_UNINSTALL_MOVE]		    = &CUpdatableWorld::UpdateAction;
	updateType[ACTION_NOTIFY_OBJECTS_UNDER_CONSTRUCTION]  = &CUpdatableWorld::UpdateObjectsUnderConstruction;
	updateType[ACTION_NOTIFY_KEY_BUILDING_CAPTURED]  = &CUpdatableWorld::UpdateKeyBuildingCaptured;
	updateType[ACTION_NOTIFY_KEY_BUILDING_LOST]  = &CUpdatableWorld::UpdateKeyBuildingLost;
	updateType[ACTION_NOTIFY_KEY_CAPTURE_PROGRESS]  = &CUpdatableWorld::UpdateKeyBuildingCaptureProgress;
	updateType[ACTION_NOTIFY_NEW_KEY_BUILDING]  = &CUpdatableWorld::UpdateNewKeyBuilding;
	updateType[ACTION_NOTIFY_DEADPLANE] = &CUpdatableWorld::UpdateDeadPlane;
	updateType[ACTION_NOTIFY_CHANGE_DBID] = &CUpdatableWorld::UpdateChageDBID;
	updateType[ACTION_NOTIFY_DEAD_PROJECTILE] = &CUpdatableWorld::UpdateDeadProjectile;

	updateType[ACTION_NOTIFY_PARADROP_STARTED] = &CUpdatableWorld::UpdateStartFinishParadrop;

	updateType[ACTION_NOTIFY_DAMAGE] = &CUpdatableWorld::UpdateDamage;

	updateType[ACTION_NOTIFY_SCAMERA_RUN] = &CUpdatableWorld::UpdateScriptCameraRun;
	updateType[ACTION_NOTIFY_SCAMERA_RESET] = &CUpdatableWorld::UpdateScriptCameraReset;

	updateType[ACTION_NOTIFY_SC_START_MOVIE] = &CUpdatableWorld::UpdateScriptCameraStartMovie;
	updateType[ACTION_NOTIFY_SC_STOP_MOVIE] = &CUpdatableWorld::UpdateScriptCameraStopMovie;

	updateType[ACTION_NOTIFY_WEATHER_CHANGED] = &CUpdatableWorld::UpdateWeatherChanged;
	updateType[ACTION_NOTIFY_PLANE_RETURNS_DUE_WEATHER] = &CUpdatableWorld::UpdatePlaneReturns;

	updateType[ACTION_NOTIFY_DISABLE_ACTION] = &CUpdatableWorld::UpdateNotifyDisableAction;
	updateType[ACTION_NOTIFY_ENABLE_ACTION] = &CUpdatableWorld::UpdateNotifyEnableAction;
	updateType[ACTION_NOTIFY_PARENT_OF_ATOM_OBJ] = &CUpdatableWorld::UpdateParentOfAtomObj;
	
	updateType[ACTION_NOTIFY_PLAY_EFFECT] = &CUpdatableWorld::UpdatePlayEffect;

	updateType[ACTION_NOTIFY_UPDATE_STATUS] = &CUpdatableWorld::UpdateStatus;
	updateType[ACTION_NOTIFY_SUPERWEAPON_CONTROL] = &CUpdatableWorld::UpdateSuperWeaponControl;
	updateType[ACTION_NOTIFY_SUPERWEAPON_RECYCLE] = &CUpdatableWorld::UpdateSuperWeaponRecycle;
	updateType[ACTION_NOTIFY_DIVEBOMBER_DIVE] = &CUpdatableWorld::UpdateDiveBomberDive;

	updateType[ACTION_NOTIFY_PLAY_ATTACHED_EFFECT]			= &CUpdatableWorld::ObjectPlayAttachedEffect;
	updateType[ACTION_NOTIFY_STOP_ATTACHED_EFFECT]			= &CUpdatableWorld::ObjectStopAttachedEffect;
}

void CUpdatableWorld::UpdateDiveBomberDive( const SAIBasicUpdate * _pUpdate )
{
	const SAIActionUpdate *pUpdate = checked_cast<const SAIActionUpdate*>( _pUpdate );
	if ( CMOUnitMechanical *pMO = dynamic_cast<CMOUnitMechanical *>( GetMapObj(pUpdate->nObjUniqueID) ) )
	{
		pMO->SetDiveSound( pUpdate->nParam );
	}
}

void CUpdatableWorld::UpdateDeadPlane( const SAIBasicUpdate * _pUpdate )
{
	const SAIActionUpdate *pUpdate = checked_cast<const SAIActionUpdate*>( _pUpdate );
	if ( CMOUnitMechanical *pMO = dynamic_cast<CMOUnitMechanical *>( GetMapObj(pUpdate->nObjUniqueID) ) )
		pMO->AIUpdateDeadPlane( pUpdate );
	else if ( CMOUnitHelicopter *pMOHelicopter = dynamic_cast<CMOUnitHelicopter *>( GetMapObj(pUpdate->nObjUniqueID) ) )
		pMOHelicopter->AIUpdateDeadPlane( pUpdate, GetSeason() );
}

void CUpdatableWorld::UpdateReinfTypeAvail( const SAIBasicUpdate * _pUpdate )
{
	const SAIAvailableReinfUpdate *pUpdate( checked_cast<const SAIAvailableReinfUpdate*>( _pUpdate ) );

	if ( pUpdate->pReinf )
	{
		if ( pUpdate->bEnabled )
		{
			enabledReinforcements.push_back( pUpdate->pReinf );
		}
		else
		{
			for ( CEnabledReinforcements::iterator it = enabledReinforcements.begin(); it != enabledReinforcements.end(); ++it )
			{
				if ( *it == pUpdate->pReinf )
				{
					it = enabledReinforcements.erase( it );
					break;
				}
			}
		}
	}
	nReinfCallsLeft = pUpdate->nReinforcementCallsLeft;
	NInput::PostEvent( "new_update_reinf_avail", 0, 0 );
}

void CUpdatableWorld::UpdateReinfRecycle( const SAIBasicUpdate * _pUpdate )
{
	const SAIReinfRecycleUpdate *pUpdate( checked_cast<const SAIReinfRecycleUpdate*>( _pUpdate ) );

	bReinfEnabled = pUpdate->bEnabled;
	reinfTimeRecycleStart = 0;
	fReinfRecycleProgress = pUpdate->fProgress;
	reinfTimeRecycleEnd = pUpdate->timeRecycleEnd;
	NInput::PostEvent( "new_update_reinf_avail", 0, 0 );
}

void CUpdatableWorld::UpdateReinforcmentPoint( const SAIBasicUpdate * _pUpdate )
{
	const SAIReinfPointUpdate *pUpdate( checked_cast<const SAIReinfPointUpdate*>( _pUpdate ) );

	bool bWasEmpty = reinforcementPositions.empty();
	if ( pUpdate->bEnable )
		reinforcementPositions[pUpdate->nPointID] = pUpdate->position;
	else
		reinforcementPositions.erase( pUpdate->nPointID );
	bool bIsEmpty = reinforcementPositions.empty();
	if ( bWasEmpty != bIsEmpty )
		NInput::PostEvent( "new_update_reinf_point", !bIsEmpty, 0 );
}

void CUpdatableWorld::UpdateChageDBID( const SAIBasicUpdate * _pUpdate )
{
	const SAIChangeDBIDUpdate *pUpdate( checked_cast<const SAIChangeDBIDUpdate*>( _pUpdate ) );
	const int nID = pUpdate->nObjUniqueID;
	CMapObj *pMO = checked_cast<CMapObj *>(GetMapObj( nID ));

	if ( pMO && dynamic_cast<CMOUnitInfantry*>( pMO ) )
		checked_cast<CMOUnitInfantry*>( pMO )->ChangeRPGStats( *pUpdate, GetSeason() );
}

void CUpdatableWorld::SendUpdateReinfPoints( int nFactoryID )
{
	/*
	CReinforcementFactories::iterator pos = reinforcmentFactories.find( nFactoryID );
	if ( pos != reinforcmentFactories.end() )
	{
		CReinforcmentPoints & reinforcmentPoints = pos->second;		
		bool bEnabled = false;
		float fRecycle = 0.0f;
		for ( CReinforcmentPoints::iterator it = reinforcmentPoints.begin(); it != reinforcmentPoints.end(); ++it )
		{
			CReinforcmentPoint *pPoint = it->second;
			
			fRecycle = max( fRecycle, pPoint->GetRecycle() );
			if ( pPoint->IsEnabled() && pPoint->IsEnabledReinf() )
			{
				bEnabled = true;
				break;
			}
		}
		NInput::PostEvent( "update_reinforcement_points", checked_cast<int>( (fRecycle + 0.0005f) * 1000.0f ), bEnabled );
	}*/
}

void CUpdatableWorld::UpdateMove( const SAIBasicUpdate * _pUpdate )
{
	const SAIActionUpdate *pUpdate( checked_cast<const SAIActionUpdate*>( _pUpdate ) );
	const int nID = pUpdate->nObjUniqueID;
	CMapObj *pMO = checked_cast<CMapObj *>(GetMapObj( nID ));
	if ( pMO == 0 )
		return;
	//
	const NTimer::STime time = Min( pUpdate->nUpdateTime, GameTimer()->GetGameTime() );
	CPtr<IClientUpdatableProcess> pProcess = pMO->AIUpdateMovement( time, true, Scene(), SoundScene() );
	if ( pProcess != 0 )
		RegisterProcess( pProcess );
}

void CUpdatableWorld::UpdateIdleTrench( const SAIBasicUpdate * _pUpdate )
{
	typedef SAIEntranceUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );

	const int nInfantryUniqueID = pUpdate->info.nInfantryUniqueID;

	CMapObj * pMapObj = GetMapObj( nInfantryUniqueID );
	if ( !pMapObj )
		return;

	const int nTargetUniqueID = pUpdate->info.nTargetUniqueID;

	Scene()->ShowObject( nInfantryUniqueID, true );
}

void CUpdatableWorld::UpdateStop( const SAIBasicUpdate * _pUpdate )
{
	const SAIActionUpdate *pUpdate( checked_cast<const SAIActionUpdate*>( _pUpdate ) );
	const int nID = pUpdate->nObjUniqueID;
	CMapObj *pMO = checked_cast<CMapObj *>(GetMapObj( nID ));
	if ( pMO == 0 )
		return;
	const NTimer::STime time = Min( pUpdate->nUpdateTime, GameTimer()->GetGameTime() );
	CPtr<IClientUpdatableProcess> pProcess = pMO->AIUpdateMovement( time, false, Scene(), SoundScene() );
	if ( pProcess != 0 )
		RegisterProcess( pProcess );
}

void CUpdatableWorld::UpdateDelayedShoot( const SAIBasicUpdate * _pUpdate )
{
	const SAIActionUpdate *pUpdate( checked_cast<const SAIActionUpdate*>( _pUpdate ) );
	const int nID = pUpdate->nObjUniqueID;
	CMapObj *pMO = checked_cast<CMapObj *>( GetMapObj(nID) );
	if ( pMO == 0 )
		return;
	pMO->AIUpdateAction( pUpdate, eSeason );
}

void CUpdatableWorld::UpdateSelectable( const SAIBasicUpdate * _pUpdate )
{
	const SAIActionUpdate *pUpdate( checked_cast<const SAIActionUpdate*>( _pUpdate ) );
	CMOSelectable *pMO = checked_cast<CMOSelectable *>( GetMapObj(pUpdate->nObjUniqueID) );
	if ( pMO == 0 )
		return;
	//		(GetMapObj( checked_cast<const CUpdatableObj*>( pUpdate->pObj.GetPtr() )->GetUniqueId() ));
	const bool bCanSelect = pUpdate->nParam;
	if ( pMO->CanSelect() != bCanSelect ) 
	{
		if ( !bCanSelect )
		{
			DeSelect( pMO );
			HideFromSelectionGroup( pMO );
			pMO->SetCanSelect( bCanSelect );
		}
		else
		{
			pMO->SetCanSelect( bCanSelect );
			UnHideFromSelectionGroup( pMO );
		}
		UpdateSpecialSelection( pUpdate->nObjUniqueID, bCanSelect ? pMO : 0 );
	}
}

void CUpdatableWorld::UpdateDeadProjectile( const SAIBasicUpdate * _pUpdate )
{
	const SAIDissapearObjUpdate *pUpdate( checked_cast<const SAIDissapearObjUpdate*>( _pUpdate ) );
	const int nUniqueID = pUpdate->nDissapearObjID;
	Scene()->RemoveAllAttached( nUniqueID, ESSOT_PROJECTILE );
	RemoveMapObj( nUniqueID, true );
}

void CUpdatableWorld::UpdateDeadUnit( const SAIBasicUpdate * _pUpdate )
{																					 
	//SAIDeadUnitUpdate
	const SAIDeadUnitUpdate *pUpdate( checked_cast<const SAIDeadUnitUpdate*>( _pUpdate ) );

	const int nUniqueID = pUpdate->nDeadObj;
	if ( IMOUnit *pUnit = checked_cast<IMOUnit *>( GetMapObj( nUniqueID ) ) )
	{
		pUnit->AIUpdatePlacement( pUpdate->placement, Scene(), SoundScene(), eSeason );
		DeSelectDead( pUnit );
		RemoveFromSelectionGroup( pUnit );
		pUnit->AIUpdateDeadUnit( pUpdate, eSeason, ( eDayTime == NDb::DAY_NIGHT ), SoundScene(), AckManager() );

		if ( pUnit->GetTypeID() == NDb::SInfantryRPGStats::typeID )
		{
			// Add to graveyard
			graveyard[ nUniqueID ] = GameTimer()->GetGameTime() + NGlobal::GetVar( "infantry_corpses_existence", 10 ) * 1000;	
		}
	}

	OnDeadOrRemoveMapObj( nUniqueID );
}	

void CUpdatableWorld::UpdateSpecialAbility( const SAIBasicUpdate * _pUpdate )
{
	typedef SAISpecialAbilityUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );

	const int nID = GetID2<T>( _pUpdate );
	CMOSelectable *pMO = checked_cast<CMOSelectable *>( GetMapObj( nID ) );

	if ( pMO && pMO->AIUpdateSpecialAbility( *pUpdate ) )
		DoUpdateSpecialAbility( pMO );
}

void CUpdatableWorld::UpdateTreeBroken( const SAIBasicUpdate * _pUpdate )
{
	const SAITreeBrokenUpdate *pUpdate = checked_cast<const SAITreeBrokenUpdate*>( _pUpdate );
	const int nUniqueID = pUpdate->nObjUniqueID;

	CPtr<CMOObject> pTree = checked_cast<CMOObject*>( GetMapObj( nUniqueID ) );
	NI_ASSERT( pTree, StrFmt( "Trying to trample (as a tree) object # %d", nUniqueID ) );
	if ( pTree != 0 )
		pTree->AIUpdateFall( pUpdate );
}

void CUpdatableWorld::UpdateNewEntrenchment( const SAIBasicUpdate * _pUpdate )
{
	CDynamicCast<const SAITrenchUpdate> pUpdate = _pUpdate;
	NI_ASSERT( pUpdate, "Programmers: wrong update type (CUpdatableWorld::UpdateNewEntrenchment)" );
	if ( !pUpdate )
		return;

	CDynamicCast<CMOEntrenchmentPart> pMOPart = GetMapObj( pUpdate->info.nSegmentID );
	if ( !pMOPart )
		return;

	const int nUniqueID = pUpdate->info.nEntrenchID;
	CDynamicCast<CMOEntrenchment> pMOEntrenchment = GetMapObj( nUniqueID );
	if ( !pMOEntrenchment )
	{
		//DebugTrace( "New entrenchment: %d", nUniqueID );
		pMOEntrenchment = new CMOEntrenchment();
		pMOEntrenchment->Create( nUniqueID, pUpdate, eSeason, eDayTime, false );
		AddMapObj( nUniqueID, pMOEntrenchment );
		pMOEntrenchment->SetEntrenchmentStats( NDb::Get<NDb::SEntrenchmentRPGStats>( pMOPart->GetDBID() ) );
	}

	//DebugTrace( "Linking part to entrenchment: %d -> %d", pMOPart->GetID(), pMOEntrenchment->GetID() );
	pMOEntrenchment->AddPart( pMOPart, pUpdate->bLast, pUpdate->bDigBySegment );
}

void CUpdatableWorld::UpdateAnimationChanged( const SAIBasicUpdate * _pUpdate )
{
	const SAIActionUpdate *pUpdate( checked_cast<const SAIActionUpdate*>( _pUpdate ) );
	const int nUniqueID = pUpdate->nObjUniqueID;

	if ( CMapObj *pMO = GetMapObj( nUniqueID ) )
	{
		const NDb::SAnimB2 *pAnim = pMO->GetAnimation( pUpdate->nParam );
		pMO->AIUpdateAnimationChanged( pAnim, pUpdate->nUpdateTime );
	}
}

void CUpdatableWorld::ObjectPlayAttachedEffect( const SAIBasicUpdate *_pUpdate )
{
	const SAIActionUpdate *pUpdate( checked_cast<const SAIActionUpdate*>( _pUpdate ) );
	const int nUniqueID = pUpdate->nObjUniqueID;

	if ( CMapObj *pMO = GetMapObj( nUniqueID ) )
	{
		pMO->ForceSwitchLightFX( pUpdate->nParam, true, false );
	}
}

void CUpdatableWorld::ObjectStopAttachedEffect( const SAIBasicUpdate *_pUpdate )
{
	const SAIActionUpdate *pUpdate( checked_cast<const SAIActionUpdate*>( _pUpdate ) );
	const int nUniqueID = pUpdate->nObjUniqueID;

	if ( CMapObj *pMO = GetMapObj( nUniqueID ) )
	{
		pMO->ForceSwitchLightFX( pUpdate->nParam, false, false );
	}
}

void CUpdatableWorld::UpdateModifyEntranceState( const SAIBasicUpdate *_pUpdate )
{
	const SAIModifyEntranceStateUpdate *pUpdate = checked_cast<const SAIModifyEntranceStateUpdate*>( _pUpdate );
	if ( pUpdate == 0 )
		return;
	
	CMapObj *pMO = GetMapObj( pUpdate->nObjUniqueID );
	IMOContainer *pMOCont = dynamic_cast<IMOContainer*>( pMO );
	if ( pMOCont )
		pMOCont->AIUpdateModifyEntranceState( pUpdate->bOpen );
	if ( pMO )
		DoUpdateObjectStats( pMO );
}

void CUpdatableWorld::UpdateShootBuilding( const SAIBasicUpdate * _pUpdate )
{
}

void CUpdatableWorld::UpdateThrowbuilding( const SAIBasicUpdate * _pUpdate )
{

}

void CUpdatableWorld::UpdateHit( const SAIBasicUpdate * _pUpdate )
{
	const SAIHitUpdate *pUpdate = checked_cast<const SAIHitUpdate*>( _pUpdate );
	const NDb::SComplexEffect *pComplexEffect = 0;
	int nVictimUniqueID = -1;

	switch( pUpdate->info.eHitType ) 
	{
	case SAINotifyHitInfo::EHitType::EHT_NONE:
		//		DebugTrace( "EHT_NONE" );
		break;
	case SAINotifyHitInfo::EHitType::EHT_HIT:
		pComplexEffect = pUpdate->info.pWeapon->shells[pUpdate->info.wShell].pEffectHitDirect;
		nVictimUniqueID = pUpdate->info.nVictimUniqueID;
		//		DebugTrace( "EHT_HIT" );
		break;
	case SAINotifyHitInfo::EHitType::EHT_MISS:
		pComplexEffect = pUpdate->info.pWeapon->shells[pUpdate->info.wShell].pEffectHitMiss;
		nVictimUniqueID = pUpdate->info.nVictimUniqueID;
		//		DebugTrace( "EHT_MISS" );
		break;
	case SAINotifyHitInfo::EHitType::EHT_REFLECT:
		pComplexEffect = pUpdate->info.pWeapon->shells[pUpdate->info.wShell].pEffectHitReflect;
		nVictimUniqueID = pUpdate->info.nVictimUniqueID;
		//		DebugTrace( "EHT_REFLECT" );
		break;
	case SAINotifyHitInfo::EHitType::EHT_GROUND:
		{
			const NDb::SWeaponRPGStats::SShell &shell = pUpdate->info.pWeapon->shells[pUpdate->info.wShell];
			pComplexEffect = shell.pEffectHitGround;
			if ( shell.pCraters != 0 )
				PlaceCrater( shell.pCraters, eSeason, CVec2( pUpdate->info.explCoord.x, pUpdate->info.explCoord.y ) );
		}
		//		DebugTrace( "EHT_GROUND" );
		break;
	case SAINotifyHitInfo::EHitType::EHT_WATER:
		pComplexEffect = pUpdate->info.pWeapon->shells[pUpdate->info.wShell].pEffectHitWater;
		//		DebugTrace( "EHT_WATER" );
		break;
	case SAINotifyHitInfo::EHitType::EHT_AIR:
		pComplexEffect = pUpdate->info.pWeapon->shells[pUpdate->info.wShell].pEffectHitAir;
		//		DebugTrace( "EHT_AIR" );
		break;
	}
	if ( pComplexEffect != 0 )
	{
		if ( CMapObj *pUnit = checked_cast<CMapObj*>( GetMapObj( nVictimUniqueID ) ) )
		{
			if ( pUnit->GetTypeID() == NDb::SBridgeRPGStats::typeID )				
				// For bridges, show explosion on top
				PlayComplexEffect( OBJECT_ID_FORGET, pComplexEffect, pUpdate->nUpdateTime, pUpdate->info.explCoord );
			else
				pUnit->AIUpdateHit( pComplexEffect, pUpdate->info.wDir, pUpdate->nUpdateTime );
		}
		else
			PlayComplexEffect( OBJECT_ID_FORGET, pComplexEffect, pUpdate->nUpdateTime, pUpdate->info.explCoord );
		// add earthquake
		if ( pUpdate->info.pWeapon->shells[pUpdate->info.wShell].fDetonationPower > 0 )
		{
			CVec3 vPos( pUpdate->info.explCoord.x, pUpdate->info.explCoord.y, 0 );
			AI2Vis( &vPos );
			Camera()->AddEarthquake( vPos, pUpdate->info.pWeapon->shells[pUpdate->info.wShell].fDetonationPower );
		}
	}
}

void CUpdatableWorld::UpdateDeleteObject( const SAIBasicUpdate * _pUpdate )
{
	typedef SAIDissapearObjUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );

	const int nUniqueID = pUpdate->nDissapearObjID;
	CMapObj *pUnit = GetMapObj( nUniqueID );

	if ( pUnit != 0 )
	{
		if ( nUniqueID != -1 )
		{
			const NDb::SStaticObjectRPGStats *pStats = dynamic_cast<const NDb::SStaticObjectRPGStats*>( pUnit->GetStats() );
			if ( pStats != 0 && !pUnit->IsSilentlyDead() )
			{
				CVec3 vPos, vScale;
				CQuat qRot;
				pUnit->GetPlacement( &vPos, &qRot, &vScale );
				if ( pUnit->GetHP() <= 0 && pStats->pEffectExplosion != 0 )
				{
					CDynamicCast<const NDb::SObjectBaseRPGStats> pObjectStats = pStats;
					if ( pObjectStats && pObjectStats->pSeasonedEffectExplosion )
						PlayComplexSeasonedEffect( OBJECT_ID_FORGET, pObjectStats->pSeasonedEffectExplosion, pUpdate->nUpdateTime, vPos, eSeason );
					else
						PlayComplexEffect( OBJECT_ID_FORGET, pStats->pEffectExplosion, pUpdate->nUpdateTime, vPos );
				}
			}
		}
		Scene()->RemoveAllAttached( pUnit->GetID(), ESSOT_LIGHT );
		DeSelectDead( pUnit );
		RemoveFromSelectionGroup( pUnit );
		RemoveMapObj( nUniqueID, true );
	}
}

void CUpdatableWorld::UpdateSilentDeath( const SAIBasicUpdate * _pUpdate )
{
	const SAIActionUpdate *pUpdate = checked_cast<const SAIActionUpdate*>( _pUpdate );
	const int nUniqueID = pUpdate->nObjUniqueID;
	//	if ( checked_cast<const CUpdatableObj*>( pUpdate->pObj.GetPtr() )->GetObjectID() != -1 )
	if ( CMapObj *pMO = checked_cast<CMapObj *>(GetMapObj( nUniqueID )) ) 
	{
		Scene()->RemoveAllAttached( pMO->GetID(), ESSOT_DEATH_EFFECTS );
		const NDb::SObjectRPGStats *pStats = dynamic_cast<const NDb::SObjectRPGStats *>( pMO->GetStats() );
		if ( pStats != 0 )
		{
			CVec3 vPos, vScale;
			CQuat qRot;
			pMO->GetPlacement( &vPos, &qRot, &vScale );

			if ( pStats->pSeasonedEffectDeath )
				PlayComplexSeasonedEffect( OBJECT_ID_FORGET, pStats->pSeasonedEffectDeath, pUpdate->nUpdateTime, vPos, eSeason );
			else if ( pStats->pEffectDeath != 0 )
				PlayComplexEffect( OBJECT_ID_FORGET, pStats->pEffectDeath, pUpdate->nUpdateTime, vPos );			

			// change model to death without animation and death effect
			if ( IMOUnit *pUnit = dynamic_cast<IMOUnit *>( pMO ) )
			{
				CPtr<SAIDeadUnitUpdate> pDeathUpdate = new SAIDeadUnitUpdate();
				// don't play death animation and effect
				pDeathUpdate->dieAnimation.nParam = -1;
				pDeathUpdate->dieTime = 0;
				pDeathUpdate->dieAction = ACTION_NOTIFY_NONE;
				pDeathUpdate->nFatality = -1;
				pDeathUpdate->nDeadObj = nUniqueID;

				UpdateDeadUnit( pDeathUpdate );
			}
			else
			{
				CPtr<SAIRPGUpdate> pRPGUpdate = new SAIRPGUpdate();
				pRPGUpdate->info.fHitPoints = 0.0f;
				pRPGUpdate->info.nObjUniqueID = nUniqueID;

				UpdateRPGChanged( pRPGUpdate );
			}
		}
		pMO->SetSilentlyDead();
	}
}

void CUpdatableWorld::UpdateDissapearObj( const SAIBasicUpdate * _pUpdate )
{
	typedef SAIDissapearObjUpdate T;
	const T *pUpdate = checked_cast<const T*>( _pUpdate );
	const int nUniqueID = pUpdate->nDissapearObjID;

	if ( CMapObj *pMO = GetMapObj( nUniqueID ) )
	{
		DeSelectDead( pMO );
		RemoveFromSelectionGroup( pMO );
		pMO->AIUpdateDissapear( pUpdate, SoundScene(), AckManager() );
	}

	RemoveMapObj( nUniqueID, true );
}

void CUpdatableWorld::UpdateRevealArtillery( const SAIBasicUpdate * _pUpdate )
{
	typedef SAICircleUpdate T;
	const T *pUpdate = checked_cast<const T*>( _pUpdate );

	if ( pUpdate )
	{
		if ( pUpdate->nParam > 0 )
			pNotifications->Notify( EVNT_ENEMY_AA_SEEN, 0, pUpdate->info.center );
		else
			pNotifications->Notify( EVNT_ENEMY_ARTILLERY_SEEN, 0, pUpdate->info.center );
	}
}

void CUpdatableWorld::UpdateSetCamouflage( const SAIBasicUpdate * _pUpdate )
{

}

void CUpdatableWorld::UpdateRemoveCamouflage( const SAIBasicUpdate * _pUpdate )
{

}

void CUpdatableWorld::UpdateSetAmbush( const SAIBasicUpdate * _pUpdate )
{

}

void CUpdatableWorld::UpdateRemoveAmbush( const SAIBasicUpdate * _pUpdate )
{

}

void CUpdatableWorld::UpdateBreakTrack( const SAIBasicUpdate * _pUpdate )
{
	const SAIActionUpdate *pUpdate = checked_cast<const SAIActionUpdate*>( _pUpdate );
	if ( pUpdate == 0 )
		return;

	CMapObj *pMO = GetMapObj( pUpdate->nObjUniqueID );
	CMOUnitMechanical *pMOUnit = dynamic_cast<CMOUnitMechanical*>( pMO );
	if ( pMO && pMOUnit )
	{
		pMOUnit->SetTrackBroken( true );
	}
}

void CUpdatableWorld::UpdateRepairTrack( const SAIBasicUpdate * _pUpdate )
{
	const SAIActionUpdate *pUpdate = checked_cast<const SAIActionUpdate*>( _pUpdate );
	if ( pUpdate == 0 )
		return;

	CMapObj *pMO = GetMapObj( pUpdate->nObjUniqueID );
	CMOUnitMechanical *pMOUnit = dynamic_cast<CMOUnitMechanical*>( pMO );
	if ( pMO && pMOUnit )
	{
		pMOUnit->SetTrackBroken( false );
	}
}

void CUpdatableWorld::UpdateRepairStateBegin( const SAIBasicUpdate * _pUpdate )
{

}

void CUpdatableWorld::UpdateRepairStateEnd( const SAIBasicUpdate * _pUpdate )
{

}

void CUpdatableWorld::UpdateResuplyStateBegin( const SAIBasicUpdate * _pUpdate )
{

}

void CUpdatableWorld::UpdateResuplyStateEnd( const SAIBasicUpdate * _pUpdate )
{

}

void CUpdatableWorld::UpdateChangeVisibility( const SAIBasicUpdate * _pUpdate )
{
	const SAIChangeVisibilityUpdate *pUpdate( dynamic_cast<const SAIChangeVisibilityUpdate*>( _pUpdate ) );
	if ( pUpdate )
	{
		const int nID = pUpdate->info.nObjUniqueID;
		CMapObj *pMO = checked_cast<CMapObj *>(GetMapObj( nID ));
		if ( pMO && ( pMO->IsVisible() != pUpdate->bVisible ) )
		{
			pMO->AIUpdatePlacement( pUpdate->info, Scene(), SoundScene(), eSeason );
			pMO->SetVisible( pUpdate->bVisible, eSeason, ( eDayTime == NDb::DAY_NIGHT ) );
		}
	}
	else
	{
		const SAIActionUpdate *pOldUpdate( checked_cast<const SAIActionUpdate*>( _pUpdate ) );
		const int nID = pOldUpdate->nObjUniqueID;
		CMapObj *pMO = checked_cast<CMapObj *>(GetMapObj( nID ));
		bool bNewVisibility = pOldUpdate->nParam;
		if ( pMO && ( pMO->IsVisible() != bNewVisibility ) )
			pMO->SetVisible( pOldUpdate->nParam, eSeason, ( eDayTime == NDb::DAY_NIGHT ) );
	}
}

void CUpdatableWorld::UpdateStateChanged( const SAIBasicUpdate * _pUpdate )
{
	typedef SAIActionUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );
	const int nID = pUpdate->nObjUniqueID;

	CMapObj *pMO = GetMapObj( nID );
	if ( pMO == 0 )
		return;
	pMO->AIUpdateState( pUpdate->nParam );
}

void CUpdatableWorld::UpdateServedArtillery( const SAIBasicUpdate * _pUpdate )
{
	typedef SAIActionUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );
	const int nID = pUpdate->nObjUniqueID;

	if ( CDynamicCast<CMOSquad> pMOSquad = GetMapObj( nID ) )
	{
		pMOSquad->AIUpdateServedArtillery( dynamic_cast<IMOUnit*>( GetMapObj( pUpdate->nParam ) ) );
		DoUpdateSpecialAbility( pMOSquad );
	}
}

void CUpdatableWorld::UpdateSelectionGroup( const SAIBasicUpdate * _pUpdate )
{
	typedef SAIActionUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );

	const int nID = pUpdate->nObjUniqueID;
	CMOSelectable *pMO = checked_cast<CMOSelectable *>( GetMapObj( nID ) );
	CMOSelectable *pMOPattern = checked_cast<CMOSelectable *>( GetMapObj( pUpdate->nParam ) );
	if ( pMO == 0 || pMOPattern == 0 )
		return;

	OnReplaceSelectionGroup( pMOPattern, pMO );
}

void CUpdatableWorld::UpdateNotifyFeedback( const SAIBasicUpdate * _pUpdate )
{
	typedef SAIFeedbackUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );
	
	OnUpdateNotifyFeedback( pUpdate );
}

void CUpdatableWorld::UpdateDiplomacy( const SAIBasicUpdate * _pUpdate )
{
	typedef SAIDiplomacyUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );

	const int nID = GetID2<T>( _pUpdate );
	CMOSelectable *pMO = checked_cast<CMOSelectable *>( GetMapObj( nID ) );
	if ( pMO == 0 )
		return;
	if ( !pMO->AIUpdateDiplomacy( pUpdate->info ) )
	{
		DeSelect( pMO );
		RemoveFromSelectionGroup( pMO );
	}
	OnUpdateDiplomacy( pMO, pUpdate->info.nPlayer );
}

void CUpdatableWorld::UpdateKeyBuildingCaptured( const SAIBasicUpdate * _pUpdate )
{
	typedef SAIKeyBuildingUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );

	const int nID = pUpdate->info.nObjUniqueID;
	CMapObj *pMO = GetMapObj( nID );
	if ( pMO )
	{
		pMO->AIUpdateKeyObject( pUpdate->info );
		CVec3 vCenter = pMO->GetCenter();
		pNotifications->Notify( EVNT_CAPTURE_KEY_OBJECT, -1, CVec2( vCenter.x, vCenter.y ) );
		pNotifications->Notify( EVNT_KEY_OBJECT_STATE, pMO );
		OnUpdateDiplomacy( pMO, pUpdate->info.nPlayer );
	}
}

void CUpdatableWorld::UpdateKeyBuildingLost( const SAIBasicUpdate * _pUpdate )
{
	typedef SAIKeyBuildingUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );

	const int nID = pUpdate->info.nObjUniqueID;
	CMapObj *pMO = GetMapObj( nID );
	if ( pMO )
	{
		pMO->AIUpdateKeyObject( pUpdate->info );
		CVec3 vCenter = pMO->GetCenter();
		if ( pUpdate->info.bFriendLost )
			pNotifications->Notify( EVNT_LOSS_KEY_OBJECT, -1, CVec2( vCenter.x, vCenter.y ) );
		pNotifications->Notify( EVNT_KEY_OBJECT_STATE, pMO );
		OnUpdateDiplomacy( pMO, pUpdate->info.nPlayer );
	}
}

void CUpdatableWorld::UpdateKeyBuildingCaptureProgress( const SAIBasicUpdate * _pUpdate )
{
	typedef SAIKeyBuildingCaptureUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );

	const int nID = pUpdate->nObjUniqueID;
	CMapObj *pMO = GetMapObj( nID );
	if ( pMO )
	{
		OnUpdateKeyObjectProgress( pMO, pUpdate->fProgress, pUpdate->nNewSide );
	}
}

void CUpdatableWorld::UpdateNewKeyBuilding( const SAIBasicUpdate * _pUpdate )
{
	typedef SAIKeyBuildingUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );

	const int nID = pUpdate->info.nObjUniqueID;
	CMapObj *pMO = GetMapObj( nID );
	if ( pMO )
	{
		keyBuildings[nID] = pMO;
		pMO->AIUpdateKeyObject( pUpdate->info );
		CVec3 vCenter = pMO->GetCenter();
		pNotifications->Notify( EVNT_KEY_OBJECT_STATE, pMO );
		OnUpdateDiplomacy( pMO, pUpdate->info.nPlayer );
	}
}

void CUpdatableWorld::UpdateShootAreas( const SAIBasicUpdate *_pUpdate )
{
	const SAIShootAreaUpdate *pUpdate = checked_cast<const SAIShootAreaUpdate*>( _pUpdate );
	CMOUnit *pMO = checked_cast<CMOUnit *>( GetMapObj( pUpdate->nObjID ) );
	if ( pMO )
		pMO->AIUpdateShootAreas( pUpdate );
}

void CUpdatableWorld::UpdateRangeAreas( const SAIBasicUpdate *_pUpdate )
{
	const SAIShootAreaUpdate *pUpdate = checked_cast<const SAIShootAreaUpdate*>( _pUpdate );
	if ( pUpdate->nUpdateTime - nLastRangeAreasTime < 1000 )
		return;
	nLastRangeAreasTime = pUpdate->nUpdateTime;
	for ( int i = 0; i < pUpdate->info.size(); ++i )
	{
		const SShootAreas &areas = pUpdate->info[i];
		for ( list<SShootArea>::const_iterator it = areas.areas.begin(); it != areas.areas.end(); ++it )
		{
			const SShootArea &area = *it;
			DWORD dwColor = area.GetColor();
			CVec3 vColor( ( dwColor & 0x00ff0000 ) >> 16, ( dwColor & 0x0000ff00 ) >> 8, dwColor & 0x000000ff );
			vColor = vColor / 256.0f;
			CVec2 vCenter( area.vCenter3D.x, area.vCenter3D.y );
			const float fStartAngle = AI2VisRad(area.wStartAngle) + FP_PI2;
			const float fEndAngle = AI2VisRad(area.wFinishAngle) + FP_PI2;
			Scene()->AddShootArea( -1, fStartAngle, fEndAngle, area.fMinR, area.fMaxR, vColor, vCenter );
		}
	}
}

void CUpdatableWorld::UpdateMechShoot( const SAIBasicUpdate * _pUpdate )
{
	const SAIMechShotUpdate *pUpdate( checked_cast<const SAIMechShotUpdate*>( _pUpdate ) );

	const int nID = GetID2<SAIMechShotUpdate>( _pUpdate );
	CMapObj *pMO = GetMapObj( nID );
	if ( pMO )
		checked_cast<IMOContainer*>(pMO)->AIUpdateShot( pUpdate->info, pUpdate->info.time, Scene(), eSeason );
}

void CUpdatableWorld::UpdateInfantryShoot( const SAIBasicUpdate * _pUpdate )
{
	const SAIInfantryShotUpdate *pUpdate( checked_cast<const SAIInfantryShotUpdate*>( _pUpdate ) );

	const int nID = GetID2<SAIInfantryShotUpdate>( _pUpdate );
	CMapObj *pMO = GetMapObj( nID );
	if ( pMO == 0 )
		return;
	checked_cast<IMOContainer*>(pMO)->AIUpdateShot( pUpdate->info, pUpdate->info.time, Scene(), eSeason );
}

void CUpdatableWorld::UpdateTurretTurn( const SAIBasicUpdate * _pUpdate )
{
	typedef SAITurretUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );
	const int nID = GetID2<T>( _pUpdate );
	if ( CMapObj *pMO = GetMapObj( nID ) )
		pMO->AIUpdateTurretTurn( pUpdate->info, pUpdate->nUpdateTime, Scene(), pUpdate->eUpdateType == ACTION_NOTIFY_TURRET_HOR_TURN );
}

void CUpdatableWorld::UpdateEntranceState( const SAIBasicUpdate * _pUpdate )
{
	typedef SAIEntranceUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );

	const int nInfantryUniqueID = pUpdate->info.nInfantryUniqueID;
	
	CMapObj * pMapObj = GetMapObj( nInfantryUniqueID );
	if ( !pMapObj )
		return;

	const int nTargetUniqueID = pUpdate->info.nTargetUniqueID;
	IMOContainer *pTarget = dynamic_cast<IMOContainer *>(GetMapObj( nTargetUniqueID ));	
	CDynamicCast<CMOUnitInfantry> pInfantry = pMapObj;
	if ( pTarget )
	{
		CDynamicCast<CMOUnitMechanical> pMechUnit( pMapObj );
		if ( pMechUnit )
		{
			if ( pUpdate->info.bEnter )
			{
				pTarget->Load( pMechUnit, true );
				DeSelect( pMechUnit );
				RemoveFromSelectionGroup( pMechUnit );
				pMechUnit->SetTransport( pTarget );
			}
			else
			{
				pTarget->Load( pMechUnit, false );
				pMechUnit->SetTransport( 0 );
			}
		}
		else if ( pInfantry )
		{
			if ( pUpdate->info.bEnter )
			{
				pTarget->LoadSquad( pInfantry->GetSquad(), true );
				if ( NGlobal::GetVar( "m1", 0 ) != 1 )
				{
					DeSelect( pInfantry );
				}
				else
				{
					CDynamicCast<CMOBuilding> pBuilding = pTarget;
					if ( !pBuilding )
						DeSelect( pInfantry );
					// Units should keep selection then entering buildings...
				}
				RemoveFromSelectionGroup( pInfantry );
//				Scene()->ShowObject( pInfantry->GetID(), false );
				pInfantry->SetTransport( pTarget );				
			}
			else
			{
				//pInfantry->CreateSceneObject( pInfantry->GetID(), 0, eSeason );
				pTarget->LoadSquad( pInfantry->GetSquad(), false );
//				if ( pInfantry->IsVisible() )
//					Scene()->ShowObject( pInfantry->GetID(), true );
				pInfantry->SetTransport( 0 );			
			}
			DoUpdateObjectStats( pTarget );
		}
	}
	else			//Can be an entrenchment - technically, CMOEntrenchment should be inherited from IMOContainer
	{
		CMOEntrenchment *pEntrenchment = dynamic_cast<CMOEntrenchment *>(GetMapObj( nTargetUniqueID ));
		if ( pEntrenchment && pInfantry )			//It *IS* an entrenchment, and the unit is infantry
		{
			if ( pUpdate->info.bEnter )
			{
				DeSelect( pInfantry );
				pInfantry->SetEntrench( true );
			}
			else
			{
				pInfantry->SetTransport( 0 );
				pInfantry->SetEntrench( false );
			}
		}
	}
}

void CUpdatableWorld::UpdateNewFormation( const SAIBasicUpdate * _pUpdate )
{
	typedef SAIFormationUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );

	const int nSoldierUniqueID = pUpdate->info.nSoldierID;
	CMOUnitInfantry *pSoilder = checked_cast<CMOUnitInfantry *>(GetMapObj( nSoldierUniqueID ));

	const int nSquadUniqudID = pUpdate->info.nFormationID;
	CMOSquad *pSquad = checked_cast<CMOSquad *>(GetMapObj( nSquadUniqudID ));

	if ( pSoilder == 0 || pSquad == 0 )
		return;

	pSquad->Load( pSoilder, true );
	if ( pSquad->IsSelected() )
		Select( pSoilder );
}

void CUpdatableWorld::UpdateSideChanged( const SAIBasicUpdate *_pUpdate ) 
{
	// don't use ScenarioTracker when uncomment this code because tracker doesn't exist in B2_M1_World (shared part )
	// better to make virtual function and implement it in GameX\WorldClient (specific part)
	// 
	//	const SAIActionUpdate *pUpdate( checked_cast<const SAIActionUpdate*>( _pUpdate ) );
	// FOR FLAGS ONLY
	/*
	//CRAP{ 
	for ( CPtr<IPlayerScenarioInfoIterator> pIt = GetSingleton<IScenarioTracker>()->CreatePlayerScenarioInfoIterator();
	!pIt->IsEnd(); pIt->Next() )
	{
	IPlayerScenarioInfo *pPlayer = pIt->Get();
	if ( pPlayer->GetDiplomacySide() == action.nParam ) 
	{
	IPlayerScenarioInfo *pUserPlayer = GetSingleton<IScenarioTracker>()->GetUserPlayer();
	string szFlagName = pPlayer->GetGeneralSide();
	NStr::ToLower( szFlagName );
	if ( IUIMiniMap *pUIMiniMap = checked_cast<IUIMiniMap*>( Scene()->GetMissionScreen()->GetChildByID( 20000 ) ) )
	{
	WORD wCircleColor;
	string szSoundName;
	if ( action.nParam == pUserPlayer->GetDiplomacySide() )
	{
	// we got the flag
	wCircleColor = 0xF0F0;
	szSoundName = "Int_completed";
	}
	else if ( action.nParam == 2 )
	{
	wCircleColor = 0xF00F;
	szSoundName = IsFriend() ? "Int_failed" : "Int_information";
	}
	else
	{
	// enemy got the flag
	wCircleColor = 0xFF00;
	szSoundName = IsNeutral() ? "Int_failed" : "Int_flag_captured";
	}
	CVec3 vPos;
	WORD wDir;
	GetPlacement( &vPos, &wDir );
	pUIMiniMap->RemoveMarker( nMarkerID );
	pUIMiniMap->AddMarker( szFlagName + "Flag", CVec2( vPos.x, vPos.y ), true, nMarkerID, GetSingleton<IGameTimer>()->GetAbsTime(), 0, false );
	pUIMiniMap->AddCircle( CVec2( vPos.x, vPos.y ), fWorldCellSize * 16.0f, MMC_STYLE_MIXED, wCircleColor, GetSingleton<IGameTimer>()->GetAbsTime(), 5000, false, 0 );
	pUIMiniMap->AddCircle( CVec2( vPos.x, vPos.y ), fWorldCellSize * 8.0f, MMC_STYLE_MIXED, wCircleColor, GetSingleton<IGameTimer>()->GetAbsTime(), 5000, false, 0 );
	Scene()->AddSound( szSoundName.c_str(), VNULL3, SFX_INTERFACE, SAM_ADD_N_FORGET );
	}
	if ( action.nParam == 2 )
	SetDiplomacy( EDI_NEUTRAL );
	else if ( action.nParam == pUserPlayer->GetDiplomacySide() )
	SetDiplomacy( EDI_FRIEND );
	else
	SetDiplomacy( EDI_ENEMY );
	szFlagName = "Flag_" + szFlagName;
	if ( const SGDBObjectDesc *pNewDesc = GetSingleton<IObjectsDB>()->GetDesc(szFlagName.c_str()) )
	{
	const string szFlagFileName = pNewDesc->szPath + "\\1";
	if ( pVOB->ChangeObject(pVisObj, szFlagFileName.c_str(), 0, SGVOT_SPRITE) )
	{
	pDesc = pNewDesc;
	pRPG = NGDB::GetRPGStats<SObjectBaseRPGStats>( pDesc );
	pVisObj->Update( currTime, true );
	if ( pShadow )
	{
	pVOB->ChangeObject( pShadow, (szFlagFileName + "s").c_str(), 0, SGVOT_SPRITE );
	pShadow->Update( currTime, true );
	}
	}
	}
	break;
	}
	}
	break;
	//CRAP}
	*/
}

void CUpdatableWorld::UpdateSelection( const SAIBasicUpdate *_pUpdate )
{
	const SAIActionUpdate *pUpdate( checked_cast<const SAIActionUpdate*>( _pUpdate ) );
	const int nID = pUpdate->nObjUniqueID;
	CMapObj *pMO = GetMapObj( nID );
	if ( pMO == 0 )
		return;
	if ( pUpdate->nParam )
		Select( pMO );
	else
		DeSelect( pMO );
}

void CUpdatableWorld::UpdateRPGChanged( const SAIBasicUpdate *_pUpdate )
{
	typedef SAIRPGUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );
	const int nID = GetID2<T>( _pUpdate );
	CMapObj *pMO = GetMapObj( nID );
	if ( pMO )
	{
		IClientUpdatableProcess *pProcess = pMO->AIUpdateRPGStats( pUpdate->info, AckManager(), eSeason );
		if ( pProcess != 0 )
			RegisterProcess( pProcess );
		if ( pUpdate->info.fHitPoints <= 0 )
		{
			DeSelectDead( pMO );
			RemoveFromSelectionGroup( pMO );
		}
		else
			DoUpdateObjectStats( pMO );
	}
}

void CUpdatableWorld::UpdatePlacement( const SAIBasicUpdate *_pUpdate )
{
	typedef SAIPlacementUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );
	const int nID = GetIDFromInfo<T>( _pUpdate );
	CMapObj *pMO = GetMapObj( nID );
	if ( pMO )
		pMO->AIUpdatePlacement( pUpdate->info, Scene(), SoundScene(), eSeason );
}

void CUpdatableWorld::AIUpdateAreas( const SAIBasicUpdate *_pUpdate )
{
	//	const SAIShootAreaUpdate *pUpdate( checked_cast<const SAIShootAreaUpdate*>( _pUpdate ) );
	//CRAP{
	// Scene()->SetAreas( &pUpdate->info[0], pUpdate->info.size() );
	//CRAP}
}

void CUpdatableWorld::UpdateNewBridgeSpan( const SAIBasicUpdate *_pUpdate )
{
	typedef SAINewUnitUpdate T;
	const T *pUpdate = checked_cast<const SAINewUnitUpdate*>( _pUpdate );
	const int nUniqueID = GetID2<T>( _pUpdate );
	const int nTypeID = pUpdate->info.pStats == 0 ? -1 : pUpdate->info.pStats->GetTypeID();

	CreateNewObject( nUniqueID, nTypeID, _pUpdate );
	CMapObj *pMO = GetMapObj( nUniqueID );
	if ( pMO )
		pMO->SetDiplomacy( pUpdate->info.eDipl );
}

void CUpdatableWorld::UpdateNewUnits( const SAIBasicUpdate *_pUpdate )
{
	CDynamicCast<SAINewUnitUpdate> pUpdate( _pUpdate );
	NI_ASSERT( pUpdate, "Programmers: wrong update type (CUpdatableWorld::UpdateNewUnits)" );
	if ( !pUpdate )
		return;

	const int nUniqueID = GetID2<SAINewUnitUpdate>( pUpdate );
	const int nTypeID = pUpdate->info.pStats == 0 ? -1 : pUpdate->info.pStats->GetTypeID();

	CreateNewObject( nUniqueID, nTypeID, _pUpdate );
	CMapObj *pMO = GetMapObj( nUniqueID );
	if ( pMO )
	{
		pMO->SetDiplomacy( pUpdate->info.eDipl );
		if ( ACTION_NOTIFY_NEW_ST_OBJ == pUpdate->eUpdateType )
		{
			pMO->SetVisible( true, eSeason, ( eDayTime == NDb::DAY_NIGHT ) );
			if ( CMOUnit *pMOUnit = dynamic_cast<CMOUnit*>( pMO ) )
				pMOUnit->DisableIcons( true );
		}
		pMO->AINewUnitInfo( pUpdate->info, Scene(), SoundScene(), eSeason );
		OnUpdateNewUnit( pUpdate, pMO );
	}
}

void CUpdatableWorld::ProcessUpdate( const SAINewProjectileUpdate *pUpdate )
{
	if ( CMapObj *pMO = GetMapObj( pUpdate->info.nSourceUniqueID ) )
	{
		IMOUnit *pMOUnit = dynamic_cast<IMOUnit*>( pMO );
		NI_ASSERT( pMOUnit != 0, StrFmt( "Unknown object type %s tried to fire", typeid(*pMO).name() ) );

		if ( pMOUnit )
		{
			if ( CMapObj *pProjectile = pMOUnit->LaunchProjectile( pUpdate ) )
			{
				AddMapObj( pProjectile->GetID(), pProjectile );
				Scene()->ShowObject( pProjectile->GetID(), false );
			}
		}
	}
}

void CUpdatableWorld::ProcessUpdate( const SAIDeadProjectileUpdate *pUpdate )
{
	RemoveMapObj( pUpdate->nProjectileUnqiueID, true );
}

void CUpdatableWorld::UpdateAction( const SAIBasicUpdate * _pUpdate )
{
	const SAIActionUpdate *pUpdate = checked_cast<const SAIActionUpdate*>( _pUpdate );
	const int nUniqueID = GetID2<SAIActionUpdate>( _pUpdate );
	if ( CMapObj *pMO = GetMapObj(nUniqueID) )
	{
		pMO->AIUpdateAction( pUpdate, eSeason );
	}
}

void CUpdatableWorld::UpdateObjectsUnderConstruction( const SAIBasicUpdate * _pUpdate )
{
	static int nGreenTilesID = -1;
	static int nRedTilesID = -1;

	typedef SAIObjectsUnderConstructionUpdate T;
	const T *pUpdate = checked_cast<const T*>( _pUpdate );
	if ( !pUpdate )
		return;
	CObj<SAINewUnitUpdate> pTempUpdate = new SAINewUnitUpdate;

	//Temporary visual objects are deleted in any case
	for( int i = 0; i < ghostObjects.size(); ++i )
		RemoveMapObj( ghostObjects[ i ], true );

	ghostObjects.clear();

	if ( pUpdate->bSet )
	{
		// create temp visual objects, position them according to passed info
		ghostObjects.resize( pUpdate->objects.size() );
		for( int i = 0; i < pUpdate->objects.size(); ++i )
		{
			// unique ID of object passed is invalid, give ID here
			ghostObjects[i] = pTempUpdate->info.nObjUniqueID = (CLIENT_UNIQUE_ID_UNDER_CONSTRUCTION_LIST - i); //pUpdate->objects[ i ].info.nObjUniqueID;
			pTempUpdate->info = pUpdate->objects[i].info;
			const int nTypeID = pUpdate->objects[i].info.pStats == 0 ? -1 : pUpdate->objects[i].info.pStats->GetTypeID();
			CreateNewObject( ghostObjects[i], nTypeID, pTempUpdate );
			//Set fade for the object
			NGScene::SetFade( GetMapObj( ghostObjects[i] ), 0.1f );
		}

		// Display locked/unlocked tiles
		vector<SVector> tiles;

		tiles.resize( pUpdate->buildTiles.size() );
		for( int i = 0; i < pUpdate->buildTiles.size(); ++i )
			tiles[ i ] = SVector( pUpdate->buildTiles[ i ] );

		if ( nGreenTilesID == -1 ) 
			nGreenTilesID = DebugInfoManager()->CreateMarker( NDebugInfo::OBJECT_ID_GENERATE, tiles, NDebugInfo::GREEN );
		else
			DebugInfoManager()->CreateMarker( nGreenTilesID, tiles, NDebugInfo::GREEN );
		
		tiles.resize( pUpdate->impossibleToBuildTiles.size() );
		for( int i = 0; i < pUpdate->impossibleToBuildTiles.size(); ++i )
			tiles[ i ] = SVector( pUpdate->impossibleToBuildTiles[ i ] );
		if ( nRedTilesID == -1 ) 
			nRedTilesID = DebugInfoManager()->CreateMarker( NDebugInfo::OBJECT_ID_GENERATE, tiles, NDebugInfo::RED );
		else
			DebugInfoManager()->CreateMarker( nRedTilesID, tiles, NDebugInfo::RED );
	}
	else		//Remove markers
	{
		if ( nGreenTilesID != -1 ) 
		{
			DebugInfoManager()->DeleteObject( nGreenTilesID );
			nGreenTilesID = -1;
		}
		if ( nRedTilesID != -1 ) 
		{
			DebugInfoManager()->DeleteObject( nRedTilesID );
			nRedTilesID = -1;
		}
	}
}

/*
void CUpdatableWorld::UpdateInstall( const SAIBasicUpdate * _pUpdate )
{
const SAIActionUpdate *pUpdate = checked_cast<const SAIActionUpdate*>( _pUpdate );
const int nUniqueID = GetID2<SAIActionUpdate>( _pUpdate );
if ( CMapObj *pMO = GetMapObj(nUniqueID) )
{
pMO->AIUpdateAction( pUpdate, eSeason );
}
}

void CUpdatableWorld::UpdateUnInstall( const SAIBasicUpdate * _pUpdate )
{
const SAIActionUpdate *pUpdate = checked_cast<const SAIActionUpdate*>( _pUpdate );
const int nUniqueID = GetID2<SAIActionUpdate>( _pUpdate );
if ( CMapObj *pMO = GetMapObj(nUniqueID) )
{
pMO->AIUpdateAction( pUpdate, eSeason );
}
DebugTrace( "Update uninstall" );
}

void CUpdatableWorld::UpdateFinishInstall( const SAIBasicUpdate * _pUpdate )
{
const SAIActionUpdate *pUpdate = checked_cast<const SAIActionUpdate*>( _pUpdate );
const int nUniqueID = GetID2<SAIActionUpdate>( _pUpdate );
if ( CMapObj *pMO = GetMapObj(nUniqueID) )
{
pMO->AIUpdateAction( pUpdate, eSeason );
}
DebugTrace( "Update finish install" );
}

void CUpdatableWorld::UpdateFinishUnInstall( const SAIBasicUpdate * _pUpdate )
{
DebugTrace( "Update finish uninstall" );
}
*/

void CUpdatableWorld::AfterLoad()
{
	for ( CMapObjMap::iterator it = objects.begin(); it != objects.end(); ++it )
	{
		CMapObj *pMO = it->second;
		pMO->AfterLoad();
	}
}

void CUpdatableWorld::AddMapObj( int nID, CMapObj *pMO )
{
	objects[nID] = pMO;
	
	OnNewMapObj( nID, pMO );
}

void CUpdatableWorld::RemoveMapObj( int nID, bool bGlobalRemove )
{ 
	if ( bGlobalRemove )
	{
		Scene()->RemoveObject( nID );
		CMapObjMap::iterator pos = objects.find( nID );
		objects.erase( pos ); 
	}
	else
	{
		Scene()->ShowObject( nID, false );
	}
	OnDeadOrRemoveMapObj( nID );
}

void CUpdatableWorld::GetObjects( list<int> *pObjects ) const
{
	for ( CMapObjMap::const_iterator it = objects.begin(); it != objects.end(); ++it )
	{
		pObjects->push_back( it->second->GetID() );
	}
}

void CUpdatableWorld::GetObjects( vector<IB2MapObj*> *pObjects ) const
{
	pObjects->reserve( objects.size() );
	for ( CMapObjMap::const_iterator it = objects.begin(); it != objects.end(); ++it )
	{
		CMapObj *pMO = it->second;
		pObjects->push_back( pMO );
	}
}

void CUpdatableWorld::GetObjects( vector<CMapObj*> *pObjects ) const
{
	pObjects->reserve( objects.size() );
	for ( CMapObjMap::const_iterator it = objects.begin(); it != objects.end(); ++it )
	{
		CMapObj *pMO = it->second;
		pObjects->push_back( pMO );
	}
}

void CUpdatableWorld::CreateNewObject( const int nUniquieID, const int nTypeID, const SAIBasicUpdate *_pUpdate )
{
	CNewFuncsMap::iterator pos = newFuncs.find( nTypeID );
	if ( pos != newFuncs.end() ) 
	{
		CMapObj *pMO = 0;

		if ( nTypeID == NDb::SMechUnitRPGStats::typeID )
		{
			const SAINewUnitUpdate *pUpdate = dynamic_cast<const SAINewUnitUpdate *>( _pUpdate );
			if ( pUpdate )
			{
				const NDb::SMechUnitRPGStats *pMechUnitRPGStats = dynamic_cast_ptr<const NDb::SMechUnitRPGStats*>( pUpdate->info.pStats );
				if ( pMechUnitRPGStats && pMechUnitRPGStats->pM1UnitSpecific )
				{
					switch ( pMechUnitRPGStats->pM1UnitSpecific->GetTypeID() )
					{
					case NDb::SM1UnitHelicopter::typeID:
						pMO = new CMOUnitHelicopter();
						break;
					}
				}
			}
		}

		if ( pMO == 0 )
		{
			ObjectFactoryNewFunc pfnNewFunc = pos->second;
			pMO = dynamic_cast<CMapObj*>((*pfnNewFunc)());
		}

		if ( pMO )
		{
			if ( pMO->Create( nUniquieID, _pUpdate, eSeason, eDayTime, false ) )
				AddMapObj( nUniquieID, pMO );
			else
			{
				const SAINewUnitUpdate *pUpdate = dynamic_cast<const SAINewUnitUpdate *>( _pUpdate );
				if ( pUpdate )
				{
#ifndef _FINALRELEASE
					if ( NGlobal::GetVar( "show_broken_objects", 1.0f ).GetFloat() == 1.0f )
					{
						CObj<CMemObject> pMemObject = new CMemObject;
						CVec3 vPos;
						AI2Vis( &vPos, pUpdate->info.center.x, pUpdate->info.center.y, pUpdate->info.z );
						pMemObject->CreateCube( vPos, CVec3( 1.0f, 1.0f, 7.0f ) );
						badObjects.push_back( NGScene::CreateObjectInfo( pMemObject ) );
						NGScene::IGameView::SMeshInfo mesh;
						mesh.parts.push_back( NGScene::IGameView::SPartInfo( badObjects.back(), Scene()->GetGView()->CreateMaterial( CVec4( 1.0f, 0.5f, 0.0f, 1.0f ) ) ) );
						badObjectsMeshes.push_back( Scene()->GetGView()->CreateMesh( mesh, 0, 0, 0 ) );
					}
#endif
					if ( const NDb::SHPObjectRPGStats *pStats = pUpdate->info.pStats )
					{
						const CDBID &dbid = pStats->GetDBID();
						const string szType = typeid(*pStats).name();
						csSystem << CC_ORANGE << "ERROR: Failed to create object " << szType << ": " << dbid.ToString() << endl;
					}
				}
				CPtr<CMapObj> pTemp = pMO;
			}
		}
	}
}

void CUpdatableWorld::Update()
{
#ifdef _PROFILER
	VTResume();
#endif

	currTime = Singleton<IGameTimer>()->GetGameTime();
	for ( CEverySegment::iterator it = everySegment.begin(); it != everySegment.end(); ++it )
		(this->*(*it))();
	AckManager()->Update( SoundScene() );

	// Remove old corpses
	CWaitingCorpses::iterator itOld;
	for ( CWaitingCorpses::iterator it = graveyard.begin(); it != graveyard.end(); )
	{
		itOld = it;
		++it;

		if ( itOld->second < currTime )
		{
			// Remove object
			RemoveMapObj( itOld->first, true );

			graveyard.erase( itOld );
		}
	}
#ifdef _PROFILER
	VTPause();
#endif

}

void CUpdatableWorld::ProcessUpdate( const SAIPointLightUpdate *pUpdate )
{
	CMapObj *pMO = GetMapObj( pUpdate->nObjUniqueID );
	NI_ASSERT( pMO != 0, StrFmt( "Unknown object %d", pUpdate->nObjUniqueID ) );

	NI_ASSERT( false, "This method of using Point Lights is disabled" )
	/*if ( pMO )
	{
		if ( CMOPointLightObject *pObj = dynamic_cast<CMOPointLightObject*>( pMO ) )
		{
			if ( pUpdate->bLight )
				pObj->LightPointLight( pUpdate->nPointLight );
			else
				pObj->PutOutPointLight( pUpdate->nPointLight );
		}
		else
			NI_ASSERT( false, StrFmt( "Obj %d can't have pointlights", pUpdate->nObjUniqueID ) );
	}*/
}

void CUpdatableWorld::UpdateSwitchLightFX( const SAIBasicUpdate * _pUpdate )
{
	typedef SAIActionUpdate T;
	const T *pUpdate = checked_cast<const T*>( _pUpdate );
	if ( !pUpdate )
		return;

	const int nUniqueID = pUpdate->nObjUniqueID;
	CMapObj *pMO = GetMapObj(nUniqueID);
	if ( pMO )
		pMO->ForceSwitchLightFX( pUpdate->nParam, false );
}

void CUpdatableWorld::ProcessUpdate( const SAIHeadLightUpdate *pUpdate )
{
	CMapObj *pMO = GetMapObj( pUpdate->nObjUniqueID );
	NI_ASSERT( pMO != 0, StrFmt( "Unknown object %d", pUpdate->nObjUniqueID ) );

	NI_ASSERT( false, "This method of using Lights is disabled" )
	/*if ( pMO )
		pMO->ChangeLight( pUpdate->nHeadLight, eSeason, pUpdate->bLight );*/
}

void CUpdatableWorld::ProcessUpdate( const SAIToggleDayNightWindowsUpdate *pUpdate )
{
	CMapObj *pMO = GetMapObj( pUpdate->nObjUniqueID );
	NI_ASSERT( pMO != 0, StrFmt( "Unknown object %d", pUpdate->nObjUniqueID ) );

	if ( pMO )
	{
		CMOBuilding *pMOBuilding = dynamic_cast<CMOBuilding*>( pMO );
		NI_ASSERT( pMOBuilding != 0, StrFmt( "Object %s, id %d, can't update windows", typeid(*pMO).name(), pUpdate->nObjUniqueID ) );

		if ( pMOBuilding )
			pMOBuilding->ToggleNightWindows( pUpdate->bNightOn, eSeason );
	}
}

void CUpdatableWorld::ProcessUpdate( const SAIBreakWindowUpdate *pUpdate )
{
	CMapObj *pMO = GetMapObj( pUpdate->nObjUniqueID );
	NI_ASSERT( pMO != 0, StrFmt( "Unknown object %d", pUpdate->nObjUniqueID ) );

	if ( pMO )
	{
		CMOBuilding *pMOBuilding = dynamic_cast<CMOBuilding*>( pMO );
		NI_ASSERT( pMOBuilding != 0, StrFmt( "Object %s, id %d< can't update windows", typeid(*pMO).name(), pUpdate->nObjUniqueID ) );

		if ( pMOBuilding )
		{
			if ( pUpdate->nWindow >= 0 )
				pMOBuilding->BreakWindow( pUpdate->nWindow, eSeason );
			else
				pMOBuilding->BreakAllWindows( eSeason );
		}
	}
}

void CUpdatableWorld::ProcessUpdate( const SEnableAirStrike *pUpdate )
{
	NInput::PostEvent( "enable_air_strike", pUpdate->bEnable ? 1 : 0, 0 );
}

void CUpdatableWorld::UpdateStartFinishParadrop( const SAIBasicUpdate * _pUpdate )
{
	const SParadropStartFinishUpdate *pUpdate = dynamic_cast<const SParadropStartFinishUpdate *>( _pUpdate );
	if ( pUpdate == 0 )
		return;
	IMOUnit *pMO = dynamic_cast<IMOUnit*>( GetMapObj( pUpdate->nObjUniqueID ) );
	if ( pMO == 0 )
		return;
	if ( IClientUpdatableProcess *pProcess = pMO->AIUpdateStartFinishParadrop( pUpdate, eSeason ) )
		RegisterProcess( pProcess );
}

void CUpdatableWorld::UpdatePlayAllAnimations()
{
	if ( pAllAnimationsPlayer )
		pAllAnimationsPlayer->Update();
}

void CUpdatableWorld::PlayAllObjectsAnimations()
{
	if ( !pAllAnimationsPlayer )
	{
		everySegment.push_back( &CUpdatableWorld::UpdatePlayAllAnimations );
		pAllAnimationsPlayer = new CAllAnimationsPlayer( objects );
	}

	pAllAnimationsPlayer->SwitchToNextAnimation();
}

void CUpdatableWorld::UpdateDamage( const SAIBasicUpdate * _pUpdate )
{
	typedef SAIDamageUpdate T;
	const T *pUpdate( checked_cast<const T*>( _pUpdate ) );
	const int nID = GetID2<T>( _pUpdate );
	CMapObj *pMO = GetMapObj( nID );
	if ( pMO )
	{
		list<int> probableAttached;
		list<IScene::SPickObjInfo> pickedObj;
		Scene()->PickAllObjects( pUpdate->vExplosionPos, pMO->GetCenter(), &pickedObj, &probableAttached );

		CPtr<IClientUpdatableProcess> pProcess = pMO->AIUpdateDamage( pUpdate->nProjectileUniqueID, pUpdate->fDamage, probableAttached, Scene(), eSeason, true );
		if ( pProcess )
			RegisterProcess( pProcess );

		if ( !pMO->IsAlive() )
		{
			DeSelectDead( pMO );
			RemoveFromSelectionGroup( pMO );
		}
		else
			DoUpdateObjectStats( pMO );
	}
}

void CUpdatableWorld::UpdateParentOfAtomObj( const SAIBasicUpdate *pRawUpdate )
{
	CDynamicCast<SParentOfAtomObjectUpdate> pUpdate = pRawUpdate;
	if ( !pUpdate )
		return; 

	CMapObj *pMO = GetMapObj( pUpdate->nAtomObjectID );
	if ( pMO )
		pMO->SetParentID( pUpdate->nParentID );
}

void CUpdatableWorld::UpdateScriptCameraRun( const SAIBasicUpdate * _pUpdate )
{
	CDynamicCast<SScriptCameraRunUpdate> pUpdate = _pUpdate;
	if ( !pUpdate )
		return;

	NCamera::CCameraPlacement startCamera, finishCamera;
	CCSTime *pTimer = Scene()->GetGameTimer();
	const NTimer::STime timeStart = Singleton<IGameTimer>()->GetGameTime();

	GetCameraByName( &startCamera, pUpdate->szStartCam );
	GetCameraByName( &finishCamera, pUpdate->szFinishCam );
	CVec3 vTargetPos;
	if ( pUpdate->nTargetID != -1 )
		GetObjectPosByScriptID( &vTargetPos, pUpdate->nTargetID );

	NTimer::STime time = (NTimer::STime)(pUpdate->fTime * 1000.0f);
	if ( pUpdate->fLinSpeed != 0 )
		time = fabs( startCamera.vPosition - finishCamera.vPosition ) / pUpdate->fLinSpeed;

	NDb::SScriptMovies moviesData;
	moviesData.scriptCameraPlacements.clear();
	moviesData.scriptMovieSequences.clear();

	NDb::SScriptMovieSequence seq;
	seq.followKeys.clear();
	seq.posKeys.clear();

	NDb::SScriptMovieKeyPos startPos;
	NDb::SScriptMovieKeyPos finishPos;

	startPos.fStartTime = 0;
	startPos.nPositionIndex = 0;
	finishPos.fStartTime = pUpdate->fTime < FP_EPSILON ? FP_EPSILON : pUpdate->fTime;
	finishPos.nPositionIndex = 1;

	seq.posKeys.push_back( startPos );
	seq.posKeys.push_back( finishPos );

	moviesData.scriptMovieSequences.push_back( seq );

	NDb::SScriptCameraPlacement startCamPlacement;
	NDb::SScriptCameraPlacement finishCamPlacement;

	startCamPlacement.fYaw = startCamera.fYaw;
	startCamPlacement.fPitch = startCamera.fPitch;
	startCamPlacement.fFOV = startCamera.fFOV;
	startCamPlacement.vPosition = startCamera.vPosition;

	finishCamPlacement.fYaw = finishCamera.fYaw;
	finishCamPlacement.fPitch = finishCamera.fPitch;
	finishCamPlacement.fFOV = finishCamera.fFOV;
	finishCamPlacement.vPosition = finishCamera.vPosition;

	moviesData.scriptCameraPlacements.push_back( startCamPlacement );
	moviesData.scriptCameraPlacements.push_back( finishCamPlacement );

	CScriptMoviesMutatorHolder *pMoviesHolder = new CScriptMoviesMutatorHolder( moviesData, 0, pTimer );
	pMoviesHolder->SetTime( 0.0f );
	pMoviesHolder->SetSpeed( 1.0f );
	pMoviesHolder->SetLoopMode( false );
	pMoviesHolder->Play();
	Camera()->SetScriptMutatorsHolder( pMoviesHolder );

	//switch ( pUpdate->eRunType )
	//{
	//	case NDb::SCRT_DIRECT_MOVE:
	//	{
	//		Camera()->SetScriptMutator( new CSCamDMoveFlightMutator(camStart, camFinish, timeStart, time, pTimer) );
	//		break;
	//	}
	//	//
	//	case NDb::SCRT_DIRECT_ROTATE:
	//	{
	//		Camera()->SetScriptMutator( new CSCamDRotateFlightMutator(camStart, camFinish, timeStart, time, pUpdate->fAngle, pTimer) );
	//		break;
	//	}
	//	//
	//	case NDb::SCRT_DIRECT_FOLLOW:
	//	{
	//		Camera()->SetScriptMutator( new CSCamDFollowFlightMutator(camStart, camFinish, timeStart, time, pUpdate->nTargetID, pTimer) );
	//		break;
	//	}
	//	//
	//	case NDb::SCRT_SPLINE:
	//	{
	//		Camera()->SetScriptMutator( new CSCamSplineMutator(camStart, camFinish, timeStart, time, pUpdate->fSpline1, pUpdate->fSpline2, pTimer) );
	//		break;
	//	}
	//	//
	//	default:
	//	{
	//		return;
	//	}
	//}

	NGlobal::SetVar( "temp.script_movie", true );
	//NGlobal::SetVar( "game_camera_track_heights", 0 );
}

void CUpdatableWorld::UpdateScriptCameraReset( const SAIBasicUpdate * _pUpdate )
{
	Camera()->FinishMovie();

	NGlobal::SetVar( "temp.script_movie", false );
}

void CUpdatableWorld::UpdateScriptCameraStartMovie( const SAIBasicUpdate * _pUpdate )
{
	CDynamicCast<SScriptCameraStartMovieUpdate> pUpdate = _pUpdate;
	if ( !pUpdate )
		return;

	NDb::SScriptMovies moviesData;

	if ( GetMoviesData(&moviesData) )
	{
		if ( (moviesData.scriptMovieSequences.size() > pUpdate->nMovieIndex) &&
				 ((moviesData.scriptMovieSequences[pUpdate->nMovieIndex].posKeys.size() > 0) ||
					(moviesData.scriptMovieSequences[pUpdate->nMovieIndex].followKeys.size() > 0)) )
		{
			CCSTime *pTimer = Scene()->GetGameTimer();

			CScriptMoviesMutatorHolder *pMoviesHolder = new CScriptMoviesMutatorHolder( moviesData, pUpdate->nMovieIndex, pTimer );
			pMoviesHolder->SetTime( 0.0f );
			pMoviesHolder->SetSpeed( 1.0f );
			pMoviesHolder->SetLoopMode( pUpdate->bLoopPlayback );
			pMoviesHolder->SetCallbackFuncName( pUpdate->szCallbackFuncName );
			pMoviesHolder->Play();
			Camera()->SetScriptMutatorsHolder( pMoviesHolder );
		}
	}
}

void CUpdatableWorld::UpdateScriptCameraStopMovie( const SAIBasicUpdate * _pUpdate )
{
	CDynamicCast<SScriptCameraStopMovieUpdate> pUpdate = _pUpdate;
	if ( !pUpdate )
		return;

	if ( CScriptMoviesMutatorHolder *pMoviesHolder = Camera()->GetScriptMutatorsHolder() )
	{
		pMoviesHolder->Stop();
	}
}

void CUpdatableWorld::UpdateWeatherChanged( const SAIBasicUpdate * _pUpdate )
{
	CDynamicCast<SWeatherChangedUpdate> pUpdate = _pUpdate;
	if ( !pUpdate )
		return;

	NInput::PostEvent( "bad_weather", pUpdate->bActive, 0 );
	Scene()->SwitchWeather( pUpdate->bActive, pUpdate->nTimeTo );
}

void CUpdatableWorld::UpdatePlaneReturns( const SAIBasicUpdate * _pUpdate )
{
	NInput::PostEvent( "avia_returns", 0, 0 );
}

void CUpdatableWorld::UpdateNotifyDisableAction( const SAIBasicUpdate *_pUpdate )
{
	typedef const SAIActionUpdate* T;
	T pUpdate( checked_cast<T>( _pUpdate ) );

	if ( CMapObj *pMO = GetMapObj( pUpdate->nObjUniqueID ) )
	{
		if ( pMO->AIUpdateAction( false, (EActionCommand)( pUpdate->nParam ) ) )
			DoUpdateSpecialAbility( pMO );
	}
}

void CUpdatableWorld::UpdateNotifyEnableAction( const SAIBasicUpdate *_pUpdate )
{
	typedef const SAIActionUpdate* T;
	T pUpdate( checked_cast<T>( _pUpdate ) );

	if ( CMapObj *pMO = GetMapObj( pUpdate->nObjUniqueID ) )
	{
		if ( pMO->AIUpdateAction( true, (EActionCommand)( pUpdate->nParam ) ) )
			DoUpdateSpecialAbility( pMO );
	}
}

void CUpdatableWorld::UpdatePlayEffect( const SAIBasicUpdate *_pUpdate )
{
	typedef const SPlayEffectUpdate* T;
	T pUpdate( checked_cast<T>( _pUpdate ) );
	PlayComplexEffect( 0, pUpdate->pEffect, pUpdate->nUpdateTime, pUpdate->vPos );
}

void CUpdatableWorld::UpdateStatus( const SAIBasicUpdate *_pUpdate )
{
	const SUnitStatusUpdate* pUpdate( checked_cast<const SUnitStatusUpdate*>( _pUpdate ) );
	//DebugTrace( "SUnitStatusUpdate: unit: %d, status: %s, radius: %2.3f", pUpdate->nUnitID, GetStatusName( pUpdate->eStatus ), pUpdate->fRadius );
	// here we have status update
	CMapObj *pMO = GetMapObj( pUpdate->nUnitID );
	if ( pMO )
	{
		OnUpdateVisualStatus( pUpdate, pMO );
	}
}

void CUpdatableWorld::UpdateSuperWeaponControl( const SAIBasicUpdate *_pUpdate )
{
	const SSuperWeaponControl* pUpdate( checked_cast<const SSuperWeaponControl*>( _pUpdate ) );
	//DebugTrace( "SSuperWeaponControl: player: %d, unit: %d, stats: %s, enabled: %s", pUpdate->nPlayer, pUpdate->nUnitID, pUpdate->pUnit ? NDb::GetResName( pUpdate->pUnit ) : "null", pUpdate->bEnabled ? "true" : "false" );
	OnUpdateSuperWeaponControl( *pUpdate );
}

void CUpdatableWorld::UpdateSuperWeaponRecycle( const SAIBasicUpdate *_pUpdate )
{
	const SSuperWeaponRecycle* pUpdate( checked_cast<const SSuperWeaponRecycle*>( _pUpdate ) );
	//DebugTrace( "SSuperWeaponRecycle: player: %d, complete: %2.3f", pUpdate->nPlayer, pUpdate->fPartComplete );
	OnUpdateSuperWeaponRecycle( *pUpdate );
}

void CUpdatableWorld::ProcessUpdate( const SLaserMarkUpdate *pUpdate )
{
	if ( pUpdate->info.nUnitID == -1 )
	{
		hash_map<int, CObj<CLaserMarkTrace> >::iterator pos = laserMarks.find( pUpdate->info.nLaserMarkID );
		if ( pos != laserMarks.end() )
			laserMarks.erase( pos );

		hash_map<int, CObj<CObjectBase> >::iterator posMesh = laserMarksMeshes.find( pUpdate->info.nLaserMarkID );
		if ( posMesh != laserMarksMeshes.end() )
			laserMarksMeshes.erase( posMesh );
	}
	else
	{
		CDynamicCast<CMOUnit> pMOUnit = GetMapObj( pUpdate->info.nUnitID );
		const NDb::SMaterial *pShotTraceMaterial = Scene()->GetSceneConsts()->pShotTraceMaterial;
		if ( pMOUnit && pShotTraceMaterial )
		{
			const CVec3 vStart( pMOUnit->GetFirePoint( pUpdate->info.nPlatform, pUpdate->info.nGun ) );
			CVec3 vTarget( pUpdate->info.vTarget );
			AI2Vis( &vTarget );
			hash_map<int, CObj<CLaserMarkTrace> >::iterator pos = laserMarks.find( pUpdate->info.nLaserMarkID );
			hash_map<int, CObj<CObjectBase> >::iterator posMesh = laserMarksMeshes.find( pUpdate->info.nLaserMarkID );
			if ( pos != laserMarks.end() && posMesh != laserMarksMeshes.end() )
			{
				CPtr<CLaserMarkTrace> pTrace = pos->second;
				pTrace->UpdatePoints( vStart, vTarget );
			}
			else
			{
				CPtr<CLaserMarkTrace> pTrace = new CLaserMarkTrace( vStart, vTarget, Scene()->GetGameTimer() );
				//
				NGScene::SFullRoomInfo room( NGScene::SRoomInfo( NGScene::LF_SKIP_LIGHTING, -100 ), 0, 0 );
				CPtr<CCSBound> pBound = new CCSBound();
				SBound bound;
				bound.BoxInit( vStart, vTarget );
				pBound->Set( bound );

				CPtr<CObjectBase> pObj = Scene()->GetGView()->CreateDynamicMesh( Scene()->GetGView()->MakeMeshInfo( pTrace, pShotTraceMaterial ), 0, pBound, NGScene::MakeLargeHintBound(), room );
				laserMarks[pUpdate->info.nLaserMarkID] = pTrace;
				laserMarksMeshes[pUpdate->info.nLaserMarkID] = pObj;
			}
		}
	}
}

//void CUpdatableWorld::ProcessUpdate( const SAINewProjectileM1 *pUpdate )
//{
//	CPtr<CMOProjectile> pProjectile = 0;
//	CDynamicCast<IMOUnit> pMOUnit = GetMapObj( pUpdate->info.nSourceUniqueID );
//	const NDb::SWeaponRPGStats *pWeapon = NDb::Get<NDb::SWeaponRPGStats>( pUpdate->nWeaponID );
//	CPtr<SAINewProjectileUpdate> pNewProjectileUpdate = new SAINewProjectileUpdate();
//	pNewProjectileUpdate->info = pUpdate->info;
//	if ( pMOUnit )
//		pProjectile = pMOUnit->LaunchProjectile( pNewProjectileUpdate );
//	else if ( pWeapon )
//	{
//		CVec3 vVisStartPos;
//		AI2Vis( &vVisStartPos, pUpdate->info.vAIStartPos );
//		const float fDirection = NMath::GetAngle( pUpdate->vDirection );
//		CQuat qRotation( fDirection, V3_AXIS_Z, true );
//		pProjectile = new CMOProjectile();
//		if ( !pProjectile->Create( pNewProjectileUpdate, pWeapon->shells[pUpdate->info.nShell].pvisProjectile, vVisStartPos, qRotation ) )
//			pProjectile = 0;
//	}
//	if ( pProjectile )
//	{
//		CMapObj *pTargetMO = GetMapObj( pUpdate->nTargetID );
//		if ( pWeapon )
//			pProjectile->SetM1Info( pTargetMO, pWeapon, pUpdate->info.nShell, pUpdate->bTraceTargetIntersection, pUpdate->fDamage );
//		AddMapObj( pProjectile->GetID(), pProjectile );
//		Scene()->ShowObject( pProjectile->GetID(), false );
//	}
//}

void CUpdatableWorld::ProcessUpdate( const SExplodeProjectileUpdate *pUpdate )
{
	if ( CMapObj *pObj = GetMapObj( pUpdate->nProjectileID ) )
	{
		if ( CDynamicCast<CMOProjectile> pProjectile = pObj )
			pProjectile->Explode( pUpdate->eHitType, eSeason, pUpdate->vExplCenter, pUpdate->vExplDir );
	}
}

void CUpdatableWorld::ProcessUpdate( const SClientUpdateButtonsUpdate *pUpdate )
{
	NInput::PostEvent( "update_buttons", 0 ,0 );
}

void CUpdatableWorld::ProcessUpdate( const SClientUpdateSingleUnitUpdate *pUpdate )
{
	NInput::PostEvent( "mission_update_unit_stats", pUpdate->nUnitID, 0 );
}

void CUpdatableWorld::ProcessUpdate( const SChatMessageUpdate *pUpdate )
{
	if ( !pUpdate->bAnotherChat )
		WriteToPipe( PIPE_CHAT, pUpdate->wszMessage, pUpdate->dwColor );
	else
	{
		WriteToPipe( PIPE_CHAT, pUpdate->wszMessage, pUpdate->dwColor );
		// TODO: Здесь нужно реализовать вывод текста в центре экрана
	}

}

void CUpdatableWorld::ProcessUpdate( const SWinLoseUpdate *pUpdate )
{
	NInput::PostEvent( "winlose", pUpdate->bWin ? 1 : 0, 0 );
}

void CUpdatableWorld::ProcessUpdate( const SMoneyChangedUpdate *pUpdate )
{
	NInput::PostEvent( "money_changed", 0, 0 );
}

void CUpdatableWorld::ProcessUpdate( const SChatClear *pUpdate )
{
	NInput::PostEvent( "chat_clear", 0, 0 );
}

void CUpdatableWorld::ProcessUpdate( const struct SStartStopSequenceUpdate *pUpdate )
{
	if ( pUpdate->bStart )
		NInput::PostEvent( "begin_script_movie_sequence", 2, 0 );
	else
		NInput::PostEvent( "end_script_movie_sequence", 2, 0 );
}

void CUpdatableWorld::ProcessUpdate( const SMSChangePlaylistUpdate *pUpdate )
{
	Singleton<IMusicSystem>()->ChangePlayList( pUpdate->nPlayList );
}

//void CUpdatableWorld::ProcessUpdate( const SMSPlayVoiceUpdate *pUpdate )
//{
//	Singleton<IMusicSystem>()->PlayVoice( NDb::Get<NDb::SVoice>( pUpdate->nVoiceID ) );
//}

void CUpdatableWorld::ProcessUpdate( const SMSSetVolumeUpdate *pUpdate )
{
	Singleton<IMusicSystem>()->SetVolume( (EMusicSystemVolume)pUpdate->nVolumeType, pUpdate->fVolume );
}

void CUpdatableWorld::ProcessUpdate( const SMSPauseMusicUpdate *pUpdate )
{
	Singleton<IMusicSystem>()->PauseMusic( (EMusicSystemVolume)pUpdate->nVolumeType, pUpdate->bPause );
}

void CUpdatableWorld::ProcessUpdate( const SObjectiveChanged *pUpdate )
{
	PlayerObjectiveChanged( pUpdate->nObjectiveNumber, (EMissionObjectiveState)pUpdate->nStatus );
}

int CUpdatableWorld::operator&( IBinSaver &saver )
{
	saver.Add( 1, &objects );
	saver.Add( 2, &eSeason );
	saver.Add( 7, &pAllAnimationsPlayer );
	saver.Add( 8, &processesToUpdate );
	saver.Add( 9, &nLastRangeAreasTime );
	saver.Add( 10, &ghostObjects );
	saver.Add( 11, &pNotifications );
	saver.Add( 13, &graveyard );
	saver.Add( 14, &eDayTime );
	saver.Add( 15, &pAI );
	saver.Add( 16, &reinforcementPositions );
	saver.Add( 17, &enabledReinforcements );
	saver.Add( 18, &bReinfEnabled );
	saver.Add( 19, &reinfTimeRecycleStart );
	saver.Add( 20, &reinfTimeRecycleEnd );
	saver.Add( 21, &nReinfCallsLeft );
	saver.Add( 22, &keyBuildings );
	saver.Add( 23, &bEditor );
	saver.Add( 24, &fReinfRecycleProgress );

	return 0;
}



namespace NUpdatableProcess
{
	void Register( IClientUpdatableProcess *pProcess )
	{
		processesToUpdate.push_back( pProcess );
	}
}

