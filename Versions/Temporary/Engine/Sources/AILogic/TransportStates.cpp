#include "stdafx.h"

#include "Misc/bresenham.h"
#include "System/Time.h"
#include <float.h>
#include "Common_RTS_AI/PathFinder.h"
#include "TransportStates.h"
#include "SoldierStates.h"
#include "Technics.h"
#include "GroupLogic.h"
#include "Commands.h"
#include "UnitCreation.h"
#include "NewUpdater.h"
#include "Building.h"
#include "Formation.h"
#include "Soldier.h"
#include "Artillery.h"
#include "FormationStates.h"
#include "TechnicsStates.h"
#include "PresizePath.h"
#include "Turret.h"
#include "Bridge.h"
#include "Randomize.h"
#include "ArtilleryBulletStorage.h"
#include "PathFinder.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "DebugTools/DebugInfoManager.h"
#include "FeedBackSystem.h"
#include "UnitsIterators2.h"

//REGISTER_SAVELOAD_CLASS( 0x1108D4D9, CTransportResupplyHumanResourcesState );
REGISTER_SAVELOAD_CLASS( 0x1108D4DA, CTransportLoadRuState );
REGISTER_SAVELOAD_CLASS( 0x1108D496, CTransportLandState );
REGISTER_SAVELOAD_CLASS( 0x1108D495, CTransportWaitPassengerState );
REGISTER_SAVELOAD_CLASS( 0x1108D494, CTransportStatesFactory );
REGISTER_SAVELOAD_CLASS( 0x1508D48B, CMoveToPointNotPresize );
REGISTER_SAVELOAD_CLASS( 0x1508D49A, CTransportRepairBuildingState );
REGISTER_SAVELOAD_CLASS( 0x1508D49F, CTransportBuildBridgeState );
REGISTER_SAVELOAD_CLASS( 0x1508D4A1, CTransportRepairBridgeState );
REGISTER_SAVELOAD_CLASS( 0x1508D4A4, CTransportPlaceAntitankState );
REGISTER_SAVELOAD_CLASS( 0x1508D4A5, CTransportPlaceMineState );
REGISTER_SAVELOAD_CLASS( 0x1508D4A6, CTransportClearMineState );
REGISTER_SAVELOAD_CLASS( 0x1508D4A7, CTransportBuildEntrenchmentState );
REGISTER_SAVELOAD_CLASS( 0x1508D4A8, CTransportBuildFenceState );
REGISTER_SAVELOAD_CLASS( 0x19178340, CTransportWaitForUnload );

extern CFeedBackSystem theFeedBackSystem;
extern CDiplomacy theDipl;
extern CEventUpdater updater;
extern CUnitCreation theUnitCreation;
extern CGroupLogic theGroupLogic;
extern CUnits units;
extern NTimer::STime curTime;
extern CStaticObjects theStatObjs;

//*******************************************************************
//*										  CTransportUnitFactory												*
//*******************************************************************

CPtr<CTransportStatesFactory> CTransportStatesFactory::pFactory = 0;

IStatesFactory* CTransportStatesFactory::Instance()
{
	if ( pFactory == 0 )
		pFactory = new CTransportStatesFactory();

	return pFactory;
}

bool CTransportStatesFactory::CanCommandBeExecuted( CAICommand *pCommand )
{
	const EActionCommand &cmdType = pCommand->ToUnitCmd().nCmdType;
	return 
		(
			cmdType == ACTION_COMMAND_DIE					||
			cmdType == ACTION_COMMAND_MOVE_TO			||
			cmdType == ACTION_COMMAND_WAIT_FOR_UNITS ||
			cmdType == ACTION_COMMAND_ROTATE_TO		||
			cmdType == ACTION_MOVE_BY_DIR					||
			cmdType == ACTION_COMMAND_UNLOAD			||
			cmdType == ACTION_COMMAND_GUARD				||
			cmdType == ACTION_COMMAND_DISAPPEAR		||
			cmdType == ACTION_COMMAND_RESUPPLY_HR ||
			cmdType == ACTION_COMMAND_RESUPPLY ||
			cmdType == ACTION_COMMAND_REPAIR ||
			cmdType == ACTION_MOVE_LOAD_RU ||
			cmdType == ACTION_COMMAND_TAKE_ARTILLERY ||
			cmdType == ACTION_COMMAND_DEPLOY_ARTILLERY ||
			cmdType == ACTION_COMMAND_FOLLOW ||
			cmdType == ACTION_COMMAND_FOLLOW_NOW ||
			cmdType == ACTION_COMMAND_ENTRENCH_SELF ||
			cmdType == ACTION_COMMAND_ENTRENCH_BEGIN ||
			cmdType == ACTION_COMMAND_ENTRENCH_END ||
			cmdType == ACTION_COMMAND_BUILD_FENCE_BEGIN ||
			cmdType == ACTION_COMMAND_BUILD_FENCE_END ||
			cmdType == ACTION_COMMAND_CLEARMINE ||
			cmdType == ACTION_COMMAND_PLACEMINE ||
			cmdType == ACTION_COMMAND_PLACE_ANTITANK ||
			cmdType == ACTION_COMMAND_REPEAR_OBJECT ||
			cmdType == ACTION_COMMAND_BUILD_BRIDGE ||
			cmdType == ACTION_MOVE_TO_NOT_PRESIZE ||
			cmdType == ACTION_COMMAND_ROTATE_TO_DIR ||
			cmdType == ACTION_COMMAND_STAND_GROUND ||
			cmdType == ACTION_COMMAND_FILL_RU ||
			cmdType == ACTION_COMMAND_MOVE_TO_GRID ||
			cmdType == ACTION_COMMAND_ATTACK_UNIT ||
			cmdType == ACTION_COMMAND_SWARM_ATTACK_UNIT ||
			cmdType == ACTION_COMMAND_SWARM_ATTACK_OBJECT ||
			cmdType == ACTION_COMMAND_ATTACK_OBJECT ||
			cmdType == ACTION_COMMAND_WAIT ||
			cmdType == ACTION_COMMAND_UNLOAD_NOW ||
			cmdType == ACTION_COMMAND_MECH_ENTER||
			cmdType == ACTION_MOVE_MECH_ENTER_NOW ||
			cmdType == ACTION_MOVE_ONBOARD_ATTACK_UNIT ||
			cmdType == ACTION_COMMAND_HOLD_SECTOR ||
			cmdType == ACTION_COMMAND_PATROL ||
			cmdType == ACTION_MOVE_BY_FORMATION
		);
}

IUnitState* CTransportStatesFactory::ProduceState( class CQueueUnit *pObj, class CAICommand *pCommand )
{
	NI_ASSERT( dynamic_cast<CAITransportUnit*>( pObj ) != 0, "Wrong unit type" );
	CAITransportUnit *pUnit = checked_cast<CAITransportUnit*>( pObj );
	
	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();
	IUnitState* pResult = 0;

	bool bSwarmAttack = false;
	
	switch ( cmd.nCmdType )
	{
	case ACTION_COMMAND_MECH_ENTER:
	case ACTION_MOVE_MECH_ENTER_NOW:
		{
			CObjectBase * pObj = GetObjectByCmd( cmd );
			if ( pObj )
			{
				CMilitaryCar * pTransport = checked_cast<CMilitaryCar*>( pObj );
				pResult = new CMechUnitInsideMechUnitState( pUnit, pTransport, cmd.nCmdType == ACTION_MOVE_MECH_ENTER_NOW );
			}
		}
		
		break;
	case ACTION_COMMAND_WAIT:
		pResult = CMechUnitRestState::Instance( pUnit, cmd.vPos, pUnit->GetDirection(), 0, cmd.fNumber );

		break;
	case ACTION_COMMAND_UNLOAD_NOW:
		pResult = CTransportWaitForUnload::Instance( pUnit, cmd.vPos );
		break;
	case ACTION_COMMAND_UNLOAD:
		{
			if ( pUnit->GetState()->GetName() == EUSN_MECHUNIT_REST_ON_BOARD )
			{
				CMechUnitInsideMechUnitState *pState ( checked_cast<CMechUnitInsideMechUnitState*>( pUnit->GetState() ) );
				pState->Unload( pUnit->GetCenterPlain() );
				pResult = pState;
			}
			else
			{
				CCommonUnit * pUnitToUnload = checked_cast<CCommonUnit*>( GetObjectByCmd( cmd ) );
				if ( pUnitToUnload )
				{
					if ( pUnitToUnload->IsFormation() )
					{
						CFormation *formation = checked_cast<CFormation*>(GetObjectByCmd(cmd));
						// the old pUnitToUnload->IsSelectable() is NOT a deterministic data, so it was replaced by the usage of more fitting
						// UniqueID comparison, which is a deterministic variable
						if ( pUnit->IsTowing() && formation->GetUniqueID() == pUnit->GetTowedArtilleryCrew()->GetUniqueID())
						{
							// This is a gun crew, unhook
							pResult = CTransportUnhookArtilleryState::Instance( pUnit, VNULL2, true );
						}
						else
						{
							EActionLeaveParam eLeaveParam = EActionLeaveParam ( int( cmd.fNumber ) );
							pResult = new CTransportLandState( pUnit, eLeaveParam, cmd.vPos, checked_cast<CFormation*>( GetObjectByCmd( cmd ) ) );
						}
					}
					else // mech unit
					{
						if ( pUnitToUnload->GetState()->GetName() == EUSN_MECHUNIT_REST_ON_BOARD )
						{
							CMechUnitInsideMechUnitState *pState ( checked_cast<CMechUnitInsideMechUnitState*>( pUnitToUnload->GetState() ) );
							if ( pState )
								pState->Unload( pUnit->GetCenterPlain() );
						}
						pResult = 0;
					}
				}
				else
					pResult = new CTransportLandState( pUnit, ALP_POSITION_VALID, cmd.vPos, checked_cast<CFormation*>( GetObjectByCmd( cmd ) ) );
			}
		}

		break;
	case ACTION_MOVE_ONBOARD_ATTACK_UNIT:
		{
			CObjectBase * pObj = GetObjectByCmd( cmd );
			CMechUnitInsideMechUnitState *pState ( checked_cast<CMechUnitInsideMechUnitState*>( pUnit->GetState() ) );
			if ( pObj )
			{
				CAIUnit * pEnemy = checked_cast<CMilitaryCar*>( pObj );
				pState->AttackTarget( pEnemy );
			}
			pResult = pState;
		}

		break;
	case ACTION_COMMAND_FILL_RU:
		pResult = CTransportLoadRuState::Instance( pUnit, false, checked_cast<CBuilding*>( GetObjectByCmd( cmd ) ) );

		break;
	case ACTION_MOVE_TO_NOT_PRESIZE:
		pResult = CMoveToPointNotPresize::Instance( pUnit, cmd.vPos, cmd.fNumber );

		break;
	case ACTION_COMMAND_REPEAR_OBJECT:
		{
			CStaticObject * pObject = checked_cast<CStaticObject*>( GetObjectByCmd( cmd ) );
			if ( ESOT_BRIDGE_SPAN == pObject->GetObjectType() )
			{
				CBridgeSpan * pSpan = checked_cast<CBridgeSpan*>(GetObjectByCmd( cmd ));
				pResult = CTransportRepairBridgeState::Instance( pUnit, pSpan );
			}
			else if ( ESOT_BUILDING == pObject->GetObjectType() )
			{
				CBuilding * pBuilding = checked_cast<CBuilding*>(GetObjectByCmd( cmd ));
				pResult = CTransportRepairBuildingState::Instance( pUnit, pBuilding );
			}
		}

		break;
	case ACTION_COMMAND_BUILD_BRIDGE:
		{
			CBridgeSpan * pSpan = checked_cast<CBridgeSpan*>(GetObjectByCmd( cmd ));
			pResult = CTransportBuildBridgeState::Instance( pUnit, pSpan->GetFullBridge() );
		}

		break;
	case ACTION_COMMAND_ENTRENCH_BEGIN:
		{
			CTransportBuildEntrenchmentState * pState = new CTransportBuildEntrenchmentState( pUnit, cmd.vPos );
			pResult = pState;
		}
		break;
	case ACTION_COMMAND_ENTRENCH_END:
		if ( pUnit->GetState()->GetName() == EUSN_BUILD_ENTRENCHMENT )
		{
			NI_ASSERT( dynamic_cast<CTransportBuildEntrenchmentState*>( pUnit->GetState()) != 0, "bad state sequence" );

			if ( cmd.nNumber > 0 )			// if it is a cancellation command, nNumber=-1, do not set endpoint
				checked_cast<CTransportBuildEntrenchmentState*>( pUnit->GetState() )->SetEndPoint( cmd.vPos );

			pResult = pUnit->GetState();
		}
		break;
	case ACTION_COMMAND_PLACE_ANTITANK:
		pResult = CTransportPlaceAntitankState::Instance( pUnit, cmd.vPos );

		break;
	case ACTION_COMMAND_PLACEMINE:
		pResult = CTransportPlaceMineState::Instance( pUnit, cmd.vPos );
		
		break;
	case ACTION_COMMAND_CLEARMINE:
		pResult = CTransportClearMineState::Instance( pUnit, cmd.vPos );

		break;
	case ACTION_COMMAND_BUILD_FENCE_BEGIN:
		pResult = new CTransportBuildFenceState( pUnit, cmd.vPos );
		break;
	case ACTION_COMMAND_BUILD_FENCE_END:
		if ( pUnit->GetState()->GetName() == EUSN_BUILD_FENCE )
		{
			NI_ASSERT( dynamic_cast<CTransportBuildFenceState*>( pUnit->GetState())!=0, "bad state sequence" );

			if ( cmd.nNumber > 0 )			// if it is a cancellation command, nNumber=-1, do not set endpoint
				checked_cast<CTransportBuildFenceState*>( pUnit->GetState() )->SetEndPoint( cmd.vPos );

			pResult = pUnit->GetState();
		}

		break;
	case ACTION_COMMAND_ENTRENCH_SELF:
		pResult = CMechUnitEntrenchSelfState::Instance( pUnit );

		break;
	case ACTION_COMMAND_HOLD_SECTOR:
		switch ( cmd.nNumber )
		{	
		case 0:
			{
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_ROTATE_TO, cmd.vPos ), pUnit );
				SAIUnitCmd newCmd( ACTION_COMMAND_HOLD_SECTOR );
				newCmd.nNumber = 2;
				theGroupLogic.InsertUnitCommand( newCmd, pUnit );
			}
			break;
		case 1:
			pUnit->ResetHoldSector();
			break;
		case 2:
			pResult = CSoldierEnterHoldSectorState::Instance( pUnit );
			break;
		default:
			NI_ASSERT( false, "Wrong param to ACTION_COMMAND_HOLD_SECTOR" );
			break;
		}
		break;
	case ACTION_MOVE_LOAD_RU:
		pResult = CTransportLoadRuState::Instance( pUnit );

		break;
	case ACTION_COMMAND_DEPLOY_ARTILLERY:
		if ( pUnit->IsTowing() )
			pResult = CTransportUnhookArtilleryState::Instance( pUnit, cmd.vPos, bool(cmd.fNumber) );
		
		break;
	case ACTION_COMMAND_TAKE_ARTILLERY:
		{
			if ( 0 == cmd.nObjectID )
			{
				// take nearest artillery
				pResult = CTransportHookArtilleryState::Instance( pUnit, 0, cmd.vPos );
				break;
			}
			// clicked on gunner near gun
			CObjectBase * pB = GetObjectByCmd( cmd );
			if ( CInfantry * pInf = dynamic_cast<CInfantry*>( pB ) )
			{
				CFormation *pForm = pInf->GetFormation();
				if ( pForm && pForm->GetState()->GetName() == EUSN_GUN_CREW_STATE )
				{
					CFormationGunCrewState *pGSState = checked_cast<CFormationGunCrewState *>( pForm->GetState() );
					CArtillery * pArtillery = pGSState->GetArtillery();
					pResult = CTransportHookArtilleryState::Instance( pUnit, pArtillery, VNULL2 );
				}
			}
			else // clicked on artillery itself
			{
				CONVERT_OBJECT( CArtillery, pArtillery, GetObjectByCmd( cmd ), "Wrong artillery unit" );
				pResult = CTransportHookArtilleryState::Instance( pUnit, pArtillery, VNULL2 );
			}
		}

		break;
		/*
	case ACTION_COMMAND_RESUPPLY_HR:
		NI_ASSERT( GetObjectByCmd( cmd ) ? dynamic_cast_ptr<CArtillery*>( GetObjectByCmd( cmd ) ) != 0 : true, StrFmt( "Wrong preferred unit %s",typeid( *pObj ).name()) );
		pResult = CTransportResupplyHumanResourcesState::Instance( pUnit, cmd.vPos, static_cast_ptr<CArtillery*>(GetObjectByCmd( cmd )) );

		break;*/
	case ACTION_COMMAND_REPAIR:
		NI_ASSERT( GetObjectByCmd( cmd ) ? dynamic_cast<CAIUnit*>( GetObjectByCmd( cmd ) ) != 0 : true, StrFmt( "Wrong preferred unit %s",typeid( *pObj ).name()) );
		pResult = CTransportRepairState::Instance( pUnit, cmd.vPos, checked_cast<CAIUnit*>(GetObjectByCmd( cmd )) );

		break;
	case ACTION_COMMAND_RESUPPLY:
		NI_ASSERT( GetObjectByCmd( cmd ) ? dynamic_cast<CAIUnit*>( GetObjectByCmd( cmd ) ) != 0 : true, StrFmt( "Wrong preferred unit %s",typeid( *pObj ).name()) );
		pResult = CTransportResupplyState::Instance( pUnit, cmd.vPos, checked_cast<CAIUnit*>(GetObjectByCmd( cmd )) );
		
		break;
	case ACTION_COMMAND_DIE:
		NI_ASSERT( false, "Command to die in the queue" );

		break;
	case ACTION_COMMAND_MOVE_TO:
		pUnit->UnsetFollowState();
		//pResult = CTransportPlaceMineState::Instance( pUnit, cmd.vPos );
		pResult = CSoldierMoveToState::Instance( pUnit, cmd.vPos );

		break;
	case ACTION_COMMAND_WAIT_FOR_UNITS:
		pResult = CTransportWaitPassengerState::Instance( pUnit, checked_cast<CFormation*>(GetObjectByCmd( cmd )) );

		break;
	case ACTION_COMMAND_ROTATE_TO:
		pResult = CSoldierTurnToPointState::Instance( pUnit, cmd.vPos );

		break;
	case ACTION_COMMAND_ROTATE_TO_DIR:
		{
			CVec2 vDir = cmd.vPos;
			Normalize( &vDir );
			pResult = CSoldierTurnToPointState::Instance( pUnit, pUnit->GetCenterPlain() + vDir );
		}
		
		break;
	case ACTION_MOVE_BY_DIR:
		pResult = CSoldierMoveByDirState::Instance( pUnit, cmd.vPos );

		break;
	case ACTION_COMMAND_GUARD:
		pResult = CMechUnitRestState::Instance( pUnit, cmd.vPos, cmd.fNumber, 0, -1 );

		break;
	case ACTION_COMMAND_FOLLOW:
		{
			CONVERT_OBJECT( CCommonUnit, pUnitToFollow, GetObjectByCmd( cmd ), "Wrong unit to follow" );
			pUnit->SetFollowState( pUnitToFollow );
		}

		break;
	case ACTION_COMMAND_FOLLOW_NOW:
		{
			CONVERT_OBJECT( CCommonUnit, pUnitToFollow, GetObjectByCmd( cmd ), "Wrong unit to follow" );
			pResult = CFollowState::Instance( pUnit, pUnitToFollow );
		}

		break;
	case ACTION_COMMAND_STAND_GROUND:
		pUnit->Stop();
		pUnit->UnsetFollowState();				
		pUnit->SetBehaviourMoving( SBehaviour::EMHoldPos );

		break;
	case ACTION_COMMAND_MOVE_TO_GRID:
		pResult = CCommonMoveToGridState::Instance( pUnit, cmd.vPos, GetVectorByDirection( cmd.fNumber ) );

		break;
	case ACTION_COMMAND_SWARM_ATTACK_UNIT:
		bSwarmAttack = true;
	case ACTION_COMMAND_ATTACK_UNIT:
		{
			if ( GetObjectByCmd( cmd ) && IsValid( GetObjectByCmd( cmd ) ) )
			{
				CONVERT_OBJECT( CAIUnit, pTarget, GetObjectByCmd( cmd ), "Wrong unit to attack" );

				if ( pTarget->IsAlive() )
				{
					if ( pTarget->GetStats()->IsInfantry() && checked_cast<CSoldier*>(pTarget)->IsInBuilding() )
						pResult = CSoldierAttackUnitInBuildingState::Instance( pUnit, checked_cast<CSoldier*>(pTarget), cmd.fNumber == 0, bSwarmAttack );
					else
						pResult = CMechAttackUnitState::Instance( pUnit, checked_cast<CAIUnit*>( GetObjectByCmd( cmd ) ), cmd.fNumber == 0, bSwarmAttack );
				}
			}
			else
				pUnit->SendAcknowledgement( pCommand, ACK_INVALID_TARGET, !pCommand->IsFromAI() );
		}

		break;
	case ACTION_COMMAND_SWARM_ATTACK_OBJECT:
		bSwarmAttack = true;
	case ACTION_COMMAND_ATTACK_OBJECT:
		{
			CONVERT_OBJECT( CStaticObject, pStaticObj, GetObjectByCmd( cmd ), "Wrong object to attack" );
			// attack the artillery
			if ( pStaticObj->GetObjectType() == ESOT_ARTILLERY_BULLET_STORAGE )
			{
				pCommand->ToUnitCmd().nCmdType = bSwarmAttack ? ACTION_COMMAND_SWARM_ATTACK_UNIT : ACTION_COMMAND_ATTACK_UNIT;
				pCommand->ToUnitCmd().nObjectID = checked_cast<CArtilleryBulletStorage*>(pStaticObj)->GetOwner()->GetUniqueId();
				pCommand->ToUnitCmd().fNumber = 0;
				pResult = ProduceState( pObj, pCommand );
			}
			else
				pResult = CSoldierAttackCommonStatObjState::Instance( pUnit, pStaticObj, bSwarmAttack );
		}

		break;
	case ACTION_COMMAND_PATROL:
		{
			CVec2 vTarget( cmd.vPos );
			if ( pUnit->CanMove() )
				pResult = CCommonPatrolState::Instance( pUnit, vTarget );
			else
			{
				pUnit->SendAcknowledgement( pCommand, ACK_NEGATIVE, !pCommand->IsFromAI() );
			}
		}
		break;
	case ACTION_MOVE_BY_FORMATION:
		pResult = CMoveByFormationState::Instance( pUnit, cmd.nObjectID );
		break;

 	default:
		NI_ASSERT( false, "Wrong command" );
	}

	if ( pResult )
		theFeedBackSystem.RemoveFeedback( pUnit->GetUniqueID(), EFB_ENGINEER_WORK_FINISHED );

	return pResult;
}

IUnitState* CTransportStatesFactory::ProduceRestState( class CQueueUnit *pUnit )
{
	return CMechUnitRestState::Instance( checked_cast<CAITransportUnit*>( pUnit ), CVec2( -1, -1 ), 0, 0, -1 );
}

//*******************************************************************
//*									 CTransportWaitPassengerState										*
//*******************************************************************

IUnitState* CTransportWaitPassengerState::Instance( CMilitaryCar *pTransport, CFormation *pFormation )
{
	return new CTransportWaitPassengerState( pTransport, pFormation );
}

CTransportWaitPassengerState::CTransportWaitPassengerState( CMilitaryCar *_pTransport, CFormation *pFormation )
: pTransport( _pTransport )
{
	for ( int i = 0; i < pTransport->GetNTurrets(); ++i )
		pTransport->GetTurret( i )->SetCanReturn();

	formationsToWait.push_back( pFormation );
}

void CTransportWaitPassengerState::Segment()
{
	std::list< CPtr<CFormation> >::iterator iter = formationsToWait.begin();
	while ( iter != formationsToWait.end() )
	{
		if ( !IsValidObj( *iter ) || (*iter)->GetState()->GetName() != EUSN_ENTER_TRANSPORT )
			iter = formationsToWait.erase( iter );
		else
			++iter;
	}

	if ( formationsToWait.empty() )
		pTransport->SetCommandFinished();
}

ETryStateInterruptResult CTransportWaitPassengerState::TryInterruptState( CAICommand *pCommand )
{ 
	if ( !pCommand )
	{
		pTransport->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}

	return TSIR_YES_WAIT;
}

void CTransportWaitPassengerState::AddFormationToWait( CFormation *pFormation  )
{
	std::list< CPtr<CFormation> >::iterator iter = formationsToWait.begin();
	while ( iter != formationsToWait.end() && *iter != pFormation )
		++iter;

	if ( iter == formationsToWait.end() )
		formationsToWait.push_back( pFormation );
}

const CVec2 CTransportWaitPassengerState::GetPurposePoint() const
{
	return pTransport->GetCenterPlain();
}

//*******************************************************************
//*											CTransportLandState													*
//*******************************************************************


CTransportLandState::CTransportLandState( CMilitaryCar *_pTransport, const enum EActionLeaveParam param, const CVec2 &_vLandPoint, CFormation *_pUnload )
: CStatusUpdatesHelper( EUS_UNLOAD, _pTransport ), pTransport( _pTransport ), vLandPoint( _vLandPoint ), state( ELS_STARTING ), pUnload( _pUnload )
{
	if ( ALP_POSITION_VALID == param )
		vLandPoint = _vLandPoint + pTransport->GetGroupShift();
	else
		vLandPoint = pTransport->GetEntrancePoint();

	if ( pTransport->GetNPassengers() == 0 )
	{
		pTransport->SendAcknowledgement( ACK_NEGATIVE, true );
		pTransport->SetCommandFinished();
	}

	if ( !pTransport->CanMove() )
		state = ELS_LANDING;
}

void CTransportLandState::LandPassenger( CSoldier *pLandUnit, const CVec3 &vPos )
{
	pLandUnit->Stop();
	pLandUnit->SetCenter( vPos, false );
	pLandUnit->SetFree();
	pTransport->DelPassenger( pLandUnit );
	pLandUnit->GetState()->TryInterruptState( 0 );
}

void CTransportLandState::Segment()
{
	switch ( state )
	{
		case ELS_STARTING:
			{
				if ( CPtr<IStaticPath> pPath = pTransport->GetCurCmd()->CreateStaticPath( pTransport ) )
				{
					pTransport->SendAlongPath( pPath, pTransport->GetGroupShift(), true );
					state = ELS_MOVING;
					InitStatus();
				}
				else
				{
					pTransport->SendAcknowledgement( ACK_NEGATIVE );
					pTransport->SetCommandFinished();
				}
			}

			break;
		case ELS_MOVING:
			if ( fabs2( pTransport->GetCenterPlain() - vLandPoint ) <= sqr( 2 * pTransport->GetDistanceToLandPoint() ) || pTransport->IsIdle() )
			{
				const CVec2 vEntrancePoint( pTransport->GetEntrancePoint() );
				const SVector tile( AICellsTiles::GetTile( vEntrancePoint ) );
				const int nOnBoard = pTransport->GetNBoarded();
				bool bCanLand = false;
				bool bCanLandInfantry = false;
				for ( int i = 0; i < nOnBoard; ++i )
				{
					CAIUnit * pUnit = pTransport->GetBoarded( i );
					if ( IsValidObj( pUnit ) )//&& GetTerrain()->CanUnitGo( pUnit->GetBoundTileRadius(), tile, pUnit->GetAIPassabilityClass() ) == FREE_TERRAIN )
					{
						NI_ASSERT( pUnit->GetState()->GetName() == EUSN_MECHUNIT_REST_ON_BOARD, "UNIT is inside but not in mech rest inside mech state" );
						if ( pUnit->GetState()->GetName() == EUSN_MECHUNIT_REST_ON_BOARD )
							checked_cast<CMechUnitInsideMechUnitState*>( pUnit->GetState() )->Unload( vLandPoint );
						bCanLand = true;
					}
				}
				const int nPassangers = pTransport->GetNPassengers();
				const EAIClasses eLock = GetTerrain()->GetTileLockInfo( tile );
				if ( 0 != nPassangers )
				{
					// unload from sea unit is always allowed
					const ETerrainTypes eTerraType = GetTerrain()->GetTerrainType( tile );
					if(  eTerraType == ETT_WATER_TERRAIN || eTerraType == ETT_MARINE_TERRAIN )
						bCanLand = true;
					else if ( !( eLock & EAC_HUMAN ) )
						bCanLand = true;
				}

				if ( bCanLand )
				{
					state = ELS_LANDING;
					pTransport->Stop();
				}
			}
			else if ( pTransport->IsIdle() )
			{
				state = ELS_LANDING;
				TryInterruptState( 0 );
			}

			break;
		case ELS_LANDING_MECH:

			break;
		case ELS_LANDING:
			{
				InitStatus();
				// найти все формации в транспорте, которые не принадлежат пушке, которая болтается сзади
				// find all formations in the transport that do not belong to the gun hanging out at the back
				CPtr<CArtillery> pArt = pTransport->GetTowedArtillery();
				CFormation *pGunCrew = !IsValidObj( pArt ) ? 0 : pTransport->GetTowedArtilleryCrew();
				const int nGunCrew = pGunCrew == 0 ? 0 : pGunCrew->Size();
				
				while ( pTransport->GetNPassengers() != nGunCrew )
				{
					CFormation *pFormation = pUnload;
					const int nPassangers = pTransport->GetNPassengers();

					if ( !pFormation )
					{
						// найти формацию, которую нужно высадить
						// find a formation to land
						for ( int i = 0; i < nPassangers && pFormation == 0; ++i )
						{
							CFormation *pTmp = pTransport->GetPassenger( i )->GetFormation();
							if ( pTmp != pGunCrew )
								pFormation = pTmp;
						}
					}

					// check if still can unload
					CVec2 vDropPoint = pTransport->GetEntrancePoint();
					if ( FindAllowedDropPoint( pFormation, &vDropPoint ) )
					{

						NI_ASSERT( pFormation != 0, "Something wrong inside this transport" );
						const int nFormSize = pFormation->Size();
						
						CVec3 vGoToPoint( GetHeights()->Get3DPoint( vDropPoint ) );
						for ( int i = 0; i < nFormSize ; ++i )
						{
							CSoldier *pSoldier = (*pFormation)[i];
							if ( pSoldier->IsInTransport() )
								LandPassenger( pSoldier, vGoToPoint );
						}
						pFormation->SetCenter( vGoToPoint, true );

						CVec2 vRand;
						RandUniformlyInCircle( 3.5f * SConsts::TILE_SIZE, &vRand );
						theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_MOVE_TO, vLandPoint.x + vRand.x, vLandPoint.y + vRand.y ), pFormation, false );
						pFormation->SetSelectable( pFormation->GetPlayer() == theDipl.GetMyNumber(), true );
						if ( pUnload )
							break;
					}
					else
					{
						pTransport->SetCommandFinished();
						break;
					}
				}

				pTransport->SetCommandFinished();
			}

			break;
	}
}

bool CTransportLandState::FindAllowedDropPoint( CFormation *pUnit, CVec2 *vDropPoint )
{
	// search by radius
	WORD wStartDir = NRandom::Random( 0, 65535 );
	CVec2 vCenterPoint = *vDropPoint;
	const int nIterations = 8;
	for ( float fRadius = 150; fRadius < 1000; fRadius += 50 )
	{
		for ( int i = 0; i < nIterations; ++i )
		{
			WORD wDir = wStartDir + 65535 / nIterations * i;
			const CVec2 vDir = GetVectorByDirection( wDir );
			const CVec2 vPoint( vCenterPoint + vDir * fRadius );
			const SVector vTile( AICellsTiles::GetTile( vPoint ) );

#ifndef _FINALRELEASE
			if ( NGlobal::GetVar( "unload_tries_markers", 0 ) )
			{
				CSegment segm;
				segm.p1 = CVec2( vPoint.x + 10, vPoint.y + 10 );
				segm.p2 = CVec2( vPoint.x - 10, vPoint.y - 10 );
				segm.dir = segm.p2 - segm.p1;
				DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
				segm.p1 = CVec2( vPoint.x + 10, vPoint.y - 10 );
				segm.p2 = CVec2( vPoint.x - 10, vPoint.y + 10 );
				segm.dir = segm.p2 - segm.p1;
				DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
			}
#endif

			if ( GetTerrain()->CanUnitGo( 0, vTile, EAC_HUMAN ) == FREE_TERRAIN )
			{
				*vDropPoint = vPoint;
				return true;
			}
		}
	}
	return false;
}

ETryStateInterruptResult CTransportLandState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand || state != ELS_LANDING )
	{
		pTransport->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	else
		return TSIR_YES_WAIT;
}

const CVec2 CTransportLandState::GetPurposePoint() const
{
	return vLandPoint;
}

//*******************************************************************
//*											CTransportHookArtilleryState								*
//*******************************************************************

IUnitState* CTransportHookArtilleryState::Instance( CAITransportUnit *pTransport, CArtillery *pArtillery, const CVec2 &vHookPoint )
{
	return new CTransportHookArtilleryState( pTransport, pArtillery, vHookPoint );
}

CTransportHookArtilleryState::CTransportHookArtilleryState( class CAITransportUnit *_pTransport, class CArtillery * _pArtillery, const CVec2 &vHookPoint )
: CStatusUpdatesHelper( EUS_HOOK_ARTILLERY, _pTransport ), pArtillery( _pArtillery ), pTransport( _pTransport ), eState( TTGS_ESTIMATING ), 
	timeLast( 0 ), wDesiredTransportDir( 0 ), bInterrupted( false ), vArtilleryPoint( _pArtillery ? _pArtillery->GetCenterPlain() : vHookPoint )
{
	// find nearest artillery and hook it 
	if ( pArtillery == 0 )
	{
		float fDist = 100000000;
		CArtillery * pCurArtillery = 0;
		for ( CUnitsIter<0,2> iter( pTransport->GetParty(), EDI_FRIEND, vHookPoint, SConsts::RESUPPLY_RADIUS ); !iter.IsFinished(); iter.Iterate() )
		{
			CAIUnit * pRawUnit = *iter;
			if ( pRawUnit->GetStats()->IsArtillery() )
			{
				CArtillery * pUnit = checked_cast<CArtillery*>( pRawUnit );
				if ( pUnit->MustHaveCrewToOperate() && !pUnit->IsBeingHooked() && pUnit->CanHook() )
				{
					const float fCurDist = fabs( pUnit->GetCenterPlain() - vHookPoint );
					if ( fCurDist < fDist )
					{
						fDist = fCurDist;
						pArtillery = pUnit;
					}
				}
			}
		}
		if ( pArtillery )
		{
			pArtillery->SetBeingHooked( pTransport );
			vArtilleryPoint = pArtillery->GetCenterPlain();
		}
	}

	if ( !pArtillery || !pArtillery->MustHaveCrewToOperate() || pTransport->IsTowing() )
	{
		TryInterruptState( 0 );
	}
}

bool CTransportHookArtilleryState::CanInterrupt()
{
	return eState == TTGS_ESTIMATING || 
				eState == TTGS_APPROACHING ||
				eState == TTGS_APPROACH_BY_MOVE_BACK;
}

void CTransportHookArtilleryState::InterruptBecauseOfPath()
{
	pTransport->SendAcknowledgement( ACK_NEGATIVE_NOTIFICATION );
	pTransport->RestoreSmoothPath();
	pTransport->SetCommandFinished();
	if ( IsValidObj( pArtillery ) )
	{
		if ( pTransport == pArtillery->GetHookingTransport() )
			pArtillery->SetBeingHooked( 0 );
	}
}

void CTransportHookArtilleryState::Segment()
{
	if ( !IsValidObj( pArtillery ) || ( pArtillery->IsBeingHooked() && pArtillery->GetHookingTransport() != pTransport ))
	{
		pTransport->SetCommandFinished();
		return;
	}
	if (	eState != TTGS_WAIT_FOR_TURN &&
				eState != TTGS_SEND_CREW_TO_TRANSPORT &&
				eState != TTGS_WAIT_FOR_CREW &&
				fabs2( vArtilleryPoint - pArtillery->GetCenterPlain() ) > 1.0f  )
	{
		InterruptBecauseOfPath();
		return;
	}

	bool bRepeat = true;
	while ( bRepeat )
	{
		bRepeat = false;
		switch ( eState )
		{
		case TTGS_ESTIMATING:		
			{
				if ( !pTransport->CanHookUnit( pArtillery ) )
				{
					pTransport->SendAcknowledgement( ACK_NEGATIVE, true );
					pTransport->SetCommandFinished();
				}
				else if ( !pArtillery->HasServeCrew() || pArtillery->GetPlayer() != pTransport->GetPlayer() )
				{
					//not possible, cannot take nither other player's artillery nor free artillery
					pTransport->SendAcknowledgement( ACK_NEGATIVE, true );
					pTransport->SetCommandFinished();
				}
				else		
				{
					if ( fabs2( pArtillery->GetCenter() - pTransport->GetCenter()) < SConsts::TRANSPORT_MOVE_BACK_DISTANCE )
					{
						eState = TTGS_APPROACHING;
						InitStatus();
					}
					else
					{
						// подъехать передом.
						CPtr<IStaticPath> pPath = CreateStaticPathToPoint( pArtillery->GetCenterPlain(), VNULL2, pTransport, true, GetAIMap() );
						if ( pPath )
						{
							eState = TTGS_APPROACHING;
							pTransport->SendAlongPath( pPath, VNULL2, true );
							InitStatus();
						}
						else
							InterruptBecauseOfPath();
					}
				}
			}
			
			break;
		case TTGS_APPROACHING:
			if ( !pArtillery->HasServeCrew() || pArtillery->GetPlayer() != pTransport->GetPlayer() )
			{
				//not possible, cannot take nither other player's artillery nor free artillery
				pTransport->SetCommandFinished();
			}
			else if ( fabs2( pArtillery->GetCenter() - pTransport->GetCenter()) < SConsts::TRANSPORT_MOVE_BACK_DISTANCE ||
								pTransport->IsIdle() )
			{
				pTransport->Stop();

				CPtr<IStaticPath> pPath = CreateStaticPathToPoint( pArtillery->GetCenterPlain(), VNULL2, pTransport, true, GetAIMap() );
				if ( pPath )
				{
					eState = TTGS_APPROACH_BY_MOVE_BACK;
					pArtillery->SetBeingHooked( pTransport );
					pTransport->SendAlongPath( pPath, VNULL2, true );
					pTransport->SetGoForward( false );
				}
				else
					InterruptBecauseOfPath();
			}

			break;
		case TTGS_APPROACH_BY_MOVE_BACK:
			// расстояние между пушкой и грузовиком почти равно размерам грузовичка и пушки
			if ( !pArtillery->HasServeCrew() || pArtillery->GetPlayer() != pTransport->GetPlayer() )
			{
				//not possible, cannot take nither other player's artillery nor free artillery
				pArtillery->SetBeingHooked( 0 );
				pTransport->SetCommandFinished();
			}
			else		
			{
				// подъезжаем на нужное расстояние к пушке
				const float dist2 = fabs( pArtillery->GetCenterPlain() - pArtillery->GetHookPoint() ) +
														fabs( pTransport->GetCenterPlain() - pTransport->GetHookPoint() );
				const float dist1 = fabs( pArtillery->GetCenterPlain() - pTransport->GetCenterPlain() );
				const float diff = fabs( dist2 - dist1 );
				
				if ( !pArtillery->IsInTankPit() && diff < SConsts::GUN_CREW_TELEPORT_RADIUS )
				{
					pTransport->RestoreSmoothPath();
					pTransport->Stop();
					eState = TTGS_START_UNINSTALL;
				}
 				else if ( pTransport->IsIdle() && ( pArtillery->IsInTankPit() || diff < SConsts::TILE_SIZE * 3 ) )
				{
					eState = TTGS_START_APPROACH_BY_CHEAT_PATH;
				}
				else if ( pTransport->IsIdle() )
				{
					pTransport->SetGoForward( true );
					InterruptBecauseOfPath();
				}
			}
			
			break;
		case TTGS_WAIT_FOR_LEAVE_TANKPIT:
			if ( !pArtillery->IsInTankPit() )
				eState = TTGS_APPROACHING;
			else if ( pArtillery->GetState()->GetName() != EUSN_LEAVE_SELF_ENTRENCH )
				eState = TTGS_START_APPROACH_BY_CHEAT_PATH;

			break;
		case TTGS_START_APPROACH_BY_CHEAT_PATH:
			{
				if ( pArtillery->IsInTankPit() )
				{
					theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_LEAVE_SELF_ENTRENCH), pArtillery, false );
					//theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, VNULL2, PARAM_ABILITY_OFF), pArtillery, false );
					eState = TTGS_WAIT_FOR_LEAVE_TANKPIT;
				}
				else
				{
					const float dist2 = fabs( pArtillery->GetCenterPlain() - pArtillery->GetHookPoint() ) +
															fabs( pTransport->GetCenterPlain() - pTransport->GetHookPoint() );

					CVec2 vDestPoint( pArtillery->GetCenterPlain() - pArtillery->GetDirectionVector()*dist2 );
					pTransport->SetSmoothPath( new CPresizePath( pTransport, vDestPoint, -pArtillery->GetDirectionVector() ) );

					pTransport->SetGoForward( false );
					CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vDestPoint, VNULL2, pTransport, true, GetAIMap() );
					if ( pPath )
					{
						eState = TTGS_APPROACH_BY_CHEAT_PATH;
					}
					else
					{
						pTransport->SetGoForward( true );
						InterruptBecauseOfPath();
					}
				}
			}
			break;
		case TTGS_APPROACH_BY_CHEAT_PATH:
			if ( fabs( pTransport->GetHookPoint() - pArtillery->GetHookPoint() )< SConsts::GUN_CREW_TELEPORT_RADIUS )
			{
				pTransport->RestoreSmoothPath();
				///eState = TTGS_APPROACHING;
				eState = TTGS_START_UNINSTALL;
				bRepeat = true;
			}
			else if ( pTransport->GetSmoothPath()->IsFinished() )
			{
				pTransport->RestoreSmoothPath();
				eState = TTGS_APPROACHING;
				bRepeat = true;
			}

			break;
		case TTGS_START_UNINSTALL:
			pTransport->Stop();
			pArtillery->GetState()->TryInterruptState( 0 );
			theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_UNINSTALL), pArtillery, false );
			theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_IDLE), pArtillery, true );
			//theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_BEING_TOWED, pTransport ), pArtillery, false );
			eState = TTGS_WAIT_FOR_UNINSTALL;

			break;
		case TTGS_WAIT_FOR_UNINSTALL:
			if ( !pArtillery->HasServeCrew() || pArtillery->GetPlayer() != pTransport->GetPlayer() )
			{
				//not possible, cannot take nither other player's artillery nor free artillery
				pArtillery->SetBeingHooked( 0 );
				pTransport->SetCommandFinished();
			}
			else if ( pArtillery->IsUninstalled() && 
								pArtillery->GetCurUninstallAction() == ACTION_NOTIFY_UNINSTALL_TRANSPORT )
			{
					// повернуть пушку к грузовичку
				pTransport->SetGoForward( true );
				eState = TTGS_WAIT_FOR_TURN;
				timeLast = curTime;
				wDesiredTransportDir = GetDirectionByVector( pTransport->GetCenterPlain() - pArtillery->GetCenterPlain() );
			}

			break;
		case TTGS_WAIT_FOR_TURN:
			{
				CFormation *pArtCrew = pArtillery->GetCrew();
				if ( !IsValidObj( pArtCrew ) || !pArtCrew->IsFree() || pArtillery->GetPlayer() != pTransport->GetPlayer() )
				{
					//not possible, cannot take nither other player's artillery nor free artillery
					pArtillery->SetBeingHooked( 0 );
					pTransport->SetCommandFinished();
				}
				else 
				{
					const bool trTurn = pTransport->TurnToDirection( wDesiredTransportDir, false, true );
					const bool arTurn = pArtillery->TurnToDirection( wDesiredTransportDir - 65535/2, false, true );

					if ( trTurn && arTurn )
					{
						if ( fabs( pTransport->GetHookPoint() - pArtillery->GetHookPoint() ) > SConsts::TILE_SIZE )
						{
							eState = TTGS_APPROACH_BY_CHEAT_PATH;
						}
						else if ( pArtillery->HasServeCrew() )
						{
							eState = TTGS_SEND_CREW_TO_TRANSPORT;
							pTransport->SetTowedArtillery( pArtillery );
							pTransport->SetTowedArtilleryCrew( pArtCrew );
							pArtillery->SetBeingHooked( 0 );
							theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_BEING_TOWED, pTransport->GetUniqueId() ), pArtillery, false );
						}
						else
						{
							eState = TTGS_WAIT_FOR_CREW;
							pTransport->SetTowedArtillery( pArtillery );
							pTransport->SetTowedArtilleryCrew( pArtCrew );
							pArtillery->SetBeingHooked( 0 );
							theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_BEING_TOWED, pTransport->GetUniqueId() ), pArtillery, false );
						}
					}
					else
						timeLast = curTime;
				}
			}

			break;
		case TTGS_SEND_CREW_TO_TRANSPORT:
			if ( EUSN_BEING_TOWED == pArtillery->GetState()->GetName() )
			{
				theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_ENTER_TRANSPORT_CHEAT_PATH, pTransport->GetUniqueId() ), pTransport->GetTowedArtilleryCrew(), false );
				eState = TTGS_WAIT_FOR_CREW;
			}
			
			break;
		case TTGS_WAIT_FOR_CREW:
			if ( !pTransport->HasTowedArtilleryCrew() || 
						pTransport->GetTowedArtilleryCrew()->IsEveryUnitInTransport() )
			{
				pArtillery->SetBeingHooked( 0 );
				pTransport->SetCommandFinished();
			}

			break;
		}
	}
}

ETryStateInterruptResult CTransportHookArtilleryState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand )
	{
		pTransport->SetCommandFinished();
		if ( pArtillery )
			pArtillery->SetBeingHooked( 0 );
		return TSIR_YES_IMMIDIATELY;
	}

	// не по смерти разрещается прерывать только если не начали крутить пушку
	if ( !pTransport->IsAlive () )
	{
		theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_STOP_THIS_ACTION), pArtillery, false );
		if ( pArtillery->HasServeCrew() )
		{
 			theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_STOP_THIS_ACTION), pArtillery->GetCrew(), false );
			theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_COMMAND_CATCH_ARTILLERY, pArtillery->GetUniqueId()), pArtillery->GetCrew(), false );
		}
		pTransport->SetCommandFinished();
		bInterrupted = true;
		pArtillery->SetBeingHooked( 0 );
		return TSIR_YES_IMMIDIATELY ;
	}
	else if ( CanInterrupt() && !bInterrupted )
	{
		pTransport->SetCommandFinished();
		pTransport->SetGoForward( true );
		bInterrupted = true;
		pArtillery->SetBeingHooked( 0 );
		return TSIR_YES_IMMIDIATELY ;
	}
	else
		return TSIR_NO_COMMAND_INCOMPATIBLE;
	
//	return TSIR_YES_WAIT;
}

const CVec2 CTransportHookArtilleryState::GetPurposePoint() const
{
	return pTransport->GetCenterPlain();
}

//*******************************************************************
//*											CTransportUnhookArtilleryState							*
//*******************************************************************

IUnitState* CTransportUnhookArtilleryState::Instance( CAITransportUnit *pTransport, const CVec2 &vDestPoint, const bool _bNow )
{
	return new CTransportUnhookArtilleryState( pTransport, vDestPoint, _bNow );
}

CTransportUnhookArtilleryState::CTransportUnhookArtilleryState ( CAITransportUnit *_pTransport, const CVec2 &_vDestPoint, const bool _bNow )
: CStatusUpdatesHelper( EUS_DEPLOY_ARTILLERY, _pTransport ), pTransport( _pTransport ), eState( TUAS_START_APPROACH ),	vDestPoint( _vDestPoint ), bInterrupted( false ), nAttempt( 0 ), bNow( _bNow )
{
	NI_ASSERT( pTransport->IsTowing(), "wrong towed artillery");
	// if clicked near current transport position, then unload now
	CArtillery * pArt = pTransport->GetTowedArtillery();
	if ( !IsValidObj( pArt ) )
		return;
	const float fDist = 2.0f * ( pTransport->GetBoundTileRadius() * SConsts::TILE_SIZE + pArt->GetBoundTileRadius() * SConsts::TILE_SIZE );
	if ( fabs( vDestPoint - (pTransport->GetCenterPlain() + pArt->GetCenterPlain()) / 2.0f ) < fDist )
	{
		bNow = true;
	}
}

bool CTransportUnhookArtilleryState::CanPlaceUnit( const class CAIUnit * pUnit ) const
{
	pTransport->UnlockTiles();
	SRect rect( pUnit->GetUnitRect() );
	const EAIClasses aiClass = pUnit->GetAIPassabilityClass();

	std::list<SVector> tiles;
	GetAIMap()->GetTilesCoveredByRect( rect, &tiles );
	for ( std::list<SVector>::iterator i = tiles.begin(); i !=tiles.end(); ++i )
	{
		if ( GetTerrain()->IsLocked( (*i), aiClass ) )
			return false;
	}
	//return theStaticMap.CanUnitGoToPoint( pUnit->GetBoundTileRadius(), pUnit->GetCenter(), )
	return true;
}

void CTransportUnhookArtilleryState::Segment()
{
	CArtillery *pArt = pTransport->GetTowedArtillery();
			
	if ( !pArt || !pArt->IsAlive() )
	{
		TryInterruptState( 0 );
		return;
	}

	switch ( eState )
	{
	case TUAS_START_APPROACH:
		{
			if ( bNow )
			{
				eState = TUAS_ESTIMATING;
				InitStatus();
			}
			else
			{
				CPtr<IStaticPath> pPath = pTransport->GetCurCmd()->CreateStaticPath( pTransport );
				if ( !pPath )
					pPath = CreateStaticPathToPoint( vDestPoint, pTransport->GetGroupShift(), pTransport, true, GetAIMap() );

				if ( pPath )
				{
					pTransport->SendAlongPath( pPath, pTransport->GetGroupShift(), true );
					vDestPoint += pTransport->GetGroupShift();
					eState = TUAS_APPROACHING;
					InitStatus();
				}
				else
				{
					pTransport->SendAcknowledgement( ACK_NEGATIVE );
					TryInterruptState( 0 );
				}
			}
		}

		break;
	case TUAS_APPROACHING:
		if ( IsUnitNearPoint( pTransport, vDestPoint, SConsts::TILE_SIZE ) ||
				 pTransport->IsIdle() && IsUnitNearPoint( pTransport, vDestPoint, 6 * SConsts::TILE_SIZE ) )
		{
			const SMechUnitRPGStats *pArtStats = checked_cast<const SMechUnitRPGStats *>(pArt->GetStats());
			const SMechUnitRPGStats *pTranspStats = checked_cast<const SMechUnitRPGStats *>(pTransport->GetStats());
			float fLenght = pArtStats->vAABBHalfSize.y + pTranspStats->vAABBHalfSize.y;
			CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vDestPoint+pTransport->GetDirectionVector()*fLenght, 
																												 VNULL2, pTransport, true, GetAIMap() );
			if ( pPath )
			{
				pTransport->SendAlongPath( pPath, VNULL2, true );
				eState = TUAS_MOVE_ARTILLERY_TO_THIS_POINT;
			}
			else
				eState = TUAS_ESTIMATING;
		}
		else if ( pTransport->IsIdle() )
			TryInterruptState( 0 );

		break;
	case TUAS_ESTIMATING:
		{
			nAttempt++;
			pTransport->UnlockTiles();
			if ( CanPlaceUnit( pArt ) )
			{
				eState = TUAS_START_UNHOOK;
				pArt->LockTiles();
			}
			else
			{
				eState = TUAS_ADVANCE_A_LITTLE;
			}
		}

		break;
	case TUAS_MOVE_ARTILLERY_TO_THIS_POINT:
		if ( pTransport->IsIdle() )
			eState = TUAS_ESTIMATING;

		break;
	case TUAS_ADVANCE_A_LITTLE:
		if ( nAttempt > SConsts::TRIES_TO_UNHOOK_ARTILLERY )
		{
			pTransport->SendAcknowledgement( ACK_NEGATIVE_NOTIFICATION );
			TryInterruptState( 0 );
		}
		else
		{
			const SMechUnitRPGStats *pArtStats = checked_cast<const SMechUnitRPGStats *>(pArt->GetStats());
			const SMechUnitRPGStats *pTranspStats = checked_cast<const SMechUnitRPGStats *>(pTransport->GetStats());
			float fLenght = pArtStats->vAABBHalfSize.y + pTranspStats->vAABBHalfSize.y;
			CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vDestPoint+pTransport->GetDirectionVector()*fLenght, 
																												 VNULL2, pTransport, true, GetAIMap() );
			if ( pPath )
			{
				pTransport->SendAlongPath( pPath, VNULL2, true );
				eState = TUAS_MOVE_A_LITTLE;
			}
			else
				eState = TUAS_ESTIMATING;
		}
		
		break;
	case TUAS_MOVE_A_LITTLE:
		pTransport->UnlockTiles();
		pTransport->GetTowedArtillery()->UnlockTiles();
		if ( CanPlaceUnit( pTransport->GetTowedArtillery() ) )
		{
			pTransport->Stop();
			eState = TUAS_START_UNHOOK;
			pArt->LockTiles();
		}
		else if ( pTransport->IsIdle() )
		{
			nAttempt++;
			eState = TUAS_ADVANCE_A_LITTLE;
		}

		break;
	case TUAS_START_UNHOOK:
		{
			CArtillery *pArt = pTransport->GetTowedArtillery();

			//отпустить пушку
			pTransport->SetTowedArtillery( 0 );
			pArt->GetState()->TryInterruptState( 0 );
			
			if ( !pTransport->HasTowedArtilleryCrew() )
				TryInterruptState( 0 );
			else
			{
				CFormation *pCrew = pTransport->GetTowedArtilleryCrew();
				//выгнать артиллеристов из транспорта
				CSoldier *pSold = 0;
				CVec2 point2D( pTransport->GetEntrancePoint() );
				CVec3 point3D( point2D.x, point2D.y, GetHeights()->GetZ( point2D ) );

				pCrew->Stop();
				pCrew->GetState()->TryInterruptState( 0 );

				for ( int i = 0; i < pCrew->Size(); ++i )
				{
					pSold = (*pCrew)[i];
					pTransport->DelPassenger( pSold );
					pSold->SetFree();
					pSold->GetState()->TryInterruptState( 0 );
					pSold->SetCenter( point3D, false );
					pSold->CallUpdatePlacement();
					pSold->RestoreSmoothPath();
					pSold->Stop();
				}
				pCrew->SetCenter( point3D );

				// пусть пушку могут теперь селектить
				pArt->SetCrew( pCrew );

				// дать команду пушке инталлироваться
				theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_INSTALL ), pArt, false );

				// все , можем уезжать, справятся без нас
				pTransport->SetCommandFinished();
			}
		}

		break;

	}
}

ETryStateInterruptResult CTransportUnhookArtilleryState::TryInterruptState( class CAICommand *pCommand )
{
	bInterrupted = true;
	pTransport->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*										  CMoveToPointNotPresize											*
//*******************************************************************

IUnitState* CMoveToPointNotPresize::Instance( class CAIUnit *_pTransport, const CVec2 &_vGeneralCell, const float _fRadius )
{
	return new CMoveToPointNotPresize( _pTransport, _vGeneralCell, _fRadius );
}

CMoveToPointNotPresize::CMoveToPointNotPresize( class CAIUnit *_pTransport, const CVec2 &vGeneralCell, const float _fRadius )
: vPurposePoint( vGeneralCell ), fRadius( _fRadius ), pTransport( _pTransport )
{
	SendToPurposePoint();
}

void CMoveToPointNotPresize::SendToPurposePoint()
{
	CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vPurposePoint, pTransport->GetGroupShift(), pTransport, true, GetAIMap() );
	if ( pPath )
		pTransport->SendAlongPath( pPath, pTransport->GetGroupShift(), false );
}

void CMoveToPointNotPresize::Segment()
{
	if ( fabs2( pTransport->GetCenterPlain() - vPurposePoint ) < sqr( fRadius ) )
	{
		if ( pTransport->GetNextCommand() )
		{
			//hack! 
			pTransport->GetNextCommand()->ToUnitCmd().vPos = pTransport->GetCenterPlain();
		}
		pTransport->Stop();
		pTransport->SetCommandFinished();
	}
	else if ( pTransport->IsIdle() )
	{
		SendToPurposePoint();
	}
}

ETryStateInterruptResult CMoveToPointNotPresize::TryInterruptState( class CAICommand *pCommand )
{
	pTransport->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*										  CTransportWaitForUnload											*
//*******************************************************************

IUnitState* CTransportWaitForUnload::Instance( class CAIUnit *_pTransport, const CVec2 &vTarget )
{
	NI_VERIFY( dynamic_cast<CMilitaryCar*>( _pTransport ) != 0, "Wrong unit type", return 0 );
	CMilitaryCar *pTransport = checked_cast<CMilitaryCar*>( _pTransport );

	return new CTransportWaitForUnload( pTransport, vTarget );
}

CTransportWaitForUnload::CTransportWaitForUnload( class CMilitaryCar *_pTransport, const CVec2 &vTarget ) :
pTransport( _pTransport ), vPurposePoint( vTarget )
{
	pTransport->Stop();
	CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW, "Transport unloading started" );
	// Tell mech units to unload
	for ( int i = 0; i < pTransport->GetNBoarded(); ++i )
	{
		CAIUnit *pPass = pTransport->GetBoarded( i );
		if ( !pPass )
			continue;

		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_UNLOAD, pTransport->GetUniqueId() ), pPass, false );
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_SWARM_TO, vPurposePoint ), pPass, true );
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_MOVE_TO, vPurposePoint ), pPass, true );
	}

	// Tell infantry to unload
	std::unordered_map<int, bool> formations;
	for ( int i = 0; i < pTransport->GetNPassengers(); ++i )
	{
		CSoldier *pPass = pTransport->GetPassenger( i );
		if ( !pPass )
			continue;
		formations[pPass->GetFormation()->GetUniqueId()] = true;
	}
	for ( std::unordered_map<int, bool>::iterator it = formations.begin(); it != formations.end(); ++it )
	{
		CFormation *pPass = checked_cast<CFormation*>( CLinkObject::GetObjectByUniqueId( it->first ) );
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_UNLOAD, pTransport->GetUniqueId() ), pPass, false );
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_SWARM_TO, vPurposePoint ), pPass, true );
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_MOVE_TO, vPurposePoint ), pPass, true );
	}
}

void CTransportWaitForUnload::Segment()
{
	bool bAllLeft = true;

	for ( int i = 0; i < pTransport->GetNBoarded(); ++i )
	{
		CAIUnit *pPass = pTransport->GetBoarded( i );
		if ( pPass )
		{
			bAllLeft = false;
			break;
		}
	}

	for ( int i = 0; i < pTransport->GetNPassengers(); ++i )
	{
		CSoldier *pPass = pTransport->GetPassenger( i );
		if ( pPass )
		{
			bAllLeft = false;
			break;
		}
	}

	if ( bAllLeft )
	{
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW, "Transport unloading finished" );
		pTransport->SetCommandFinished();
	}
}

ETryStateInterruptResult CTransportWaitForUnload::TryInterruptState( class CAICommand *pCommand )
{
	pTransport->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}


