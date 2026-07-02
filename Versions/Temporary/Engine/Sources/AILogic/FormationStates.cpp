#include "stdafx.h"

#include <float.h>

#include "FormationStates.h"
#include "Formation.h"
#include "Commands.h"
#include "GroupLogic.h"
#include "Entrenchment.h"
#include "NewUpdater.h"
#include "Technics.h"
#include "TransportStates.h"
#include "EntrenchmentCreation.h"
#include "Building.h"
#include "Artillery.h"
#include "Turret.h"
#include "Soldier.h"
#include "PathFinder.h"
#include "UnitCreation.h"
#include "FormationStates.h"
#include "SoldierStates.h"
#include "Bridge.h"
#include "Randomize.h"
#include "ArtilleryBulletStorage.h"
#include "UnitGuns.h"
#include "ArtilleryPaths.h"
#include "TimerChargeExecutor.h"
#include "Statistics.h"
#include "UnitsIterators2.h"
#include "Common_RTS_AI/PathFinder.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "ExecutorContainer.h"
#include "Stats_B2_M1/AnimationFromAction.h"

extern CStatistics theStatistics;

REGISTER_SAVELOAD_CLASS( 0x1108D4DD, CFormationSwarmState );
REGISTER_SAVELOAD_CLASS( 0x1108D4DB, CCatchFormationState );
REGISTER_SAVELOAD_CLASS( 0x1108D4D2, CFormationDisbandState );
REGISTER_SAVELOAD_CLASS( 0x1108D4D3, CFormationFormState );
REGISTER_SAVELOAD_CLASS( 0x1108D4D4, CFormationWaitToFormState );
REGISTER_SAVELOAD_CLASS( 0x1108D4CA, CFormationParadeState );
REGISTER_SAVELOAD_CLASS( 0x1108D4C5, CFormationAttackFormationState );
REGISTER_SAVELOAD_CLASS( 0x1108D4C6, CFormationUseSpyglassState );
REGISTER_SAVELOAD_CLASS( 0x1108D4C0, CFormationCaptureArtilleryState );
REGISTER_SAVELOAD_CLASS( 0x1108D4BC, CFormationCatchTransportState );
REGISTER_SAVELOAD_CLASS( 0x1108D4BD, CFormationParaDropState );
REGISTER_SAVELOAD_CLASS( 0x1108D4BE, CFormationBuildLongObjectState );
REGISTER_SAVELOAD_CLASS( 0x1108D4BF, CFormationGunCrewState );
REGISTER_SAVELOAD_CLASS( 0x1108D4B2, CFormationRotateState );
REGISTER_SAVELOAD_CLASS( 0x1108D455, CFormationStatesFactory );
REGISTER_SAVELOAD_CLASS( 0x1108D456, CFormationRestState );
REGISTER_SAVELOAD_CLASS( 0x1108D457, CFormationMoveToState );
REGISTER_SAVELOAD_CLASS( 0x1108D458, CFormationEnterBuildingState );
REGISTER_SAVELOAD_CLASS( 0x1108D459, CFormationEnterEntrenchmentState );
REGISTER_SAVELOAD_CLASS( 0x1108D45A, CFormationIdleBuildingState );
REGISTER_SAVELOAD_CLASS( 0x1108D45B, CFormationIdleEntrenchmentState );
REGISTER_SAVELOAD_CLASS( 0x1108D45C, CFormationLeaveBuildingState );
REGISTER_SAVELOAD_CLASS( 0x1108D45D, CFormationLeaveEntrenchmentState );
REGISTER_SAVELOAD_CLASS( 0x1108D45E, CFormationPlaceMine );
REGISTER_SAVELOAD_CLASS( 0x1108D45F, CFormationClearMine );
REGISTER_SAVELOAD_CLASS( 0x1108D460, CFormationAttackUnitState );
REGISTER_SAVELOAD_CLASS( 0x1108D461, CFormationAttackCommonStatObjState );
REGISTER_SAVELOAD_CLASS( 0x1108D462, CFormationEnterTransportState );
REGISTER_SAVELOAD_CLASS( 0x1108D463, CFormationEnterTransportNowState );
REGISTER_SAVELOAD_CLASS( 0x1108D464, CFormationIdleTransportState );
REGISTER_SAVELOAD_CLASS( 0x1508D480, CFormationDetonateChargeState );
REGISTER_SAVELOAD_CLASS( 0x1508D481, CFormationPlaceChargeState );
REGISTER_SAVELOAD_CLASS( 0x1508D483, CFormationEnterEntrenchmentNowState );
REGISTER_SAVELOAD_CLASS( 0x1508D484, CFormationEnterBuildingNowState );
REGISTER_SAVELOAD_CLASS( 0x1508D485, CFormationEnterTransportByCheatPathState );
REGISTER_SAVELOAD_CLASS( 0x1508D48C, CFormationInstallMortarState );
REGISTER_SAVELOAD_CLASS( 0x1508D49D, CFormationRepairBuildingState );
REGISTER_SAVELOAD_CLASS( 0x1508D4A3, CFormationRepairBridgeState );
REGISTER_SAVELOAD_CLASS( 0x1509BB00, CFormationThrowGrenadeState );
REGISTER_SAVELOAD_CLASS( 0x110AA480, CFormationEntrenchSelfState );
REGISTER_SAVELOAD_CLASS( 0x110B23C0, CFormationLeaveSelfEntrenchState );
REGISTER_SAVELOAD_CLASS( 0x1112DAC0, CFormationFirstAidState );

extern CStaticObjects theStatObjs;
extern NTimer::STime curTime;
extern CGroupLogic theGroupLogic;
extern CEventUpdater updater;
extern CDiplomacy theDipl;
extern CUnitCreation theUnitCreation;
extern CExecutorContainer theExecutorContainer;

static bool IsAlly( const int nMyPlayer, const int nPlayer )
{
	return theDipl.GetDiplStatus( nMyPlayer, nPlayer ) == EDI_FRIEND && nMyPlayer != nPlayer;
}

CPtr<CFormationStatesFactory> CFormationStatesFactory::pFactory = 0;

CFormationStatesFactory* CFormationStatesFactory::Instance()
{
	if ( pFactory == 0 )
		pFactory = new CFormationStatesFactory();

	return pFactory;
}

bool CFormationStatesFactory::CanCommandBeExecuted( CAICommand *pCommand )
{
	const EActionCommand &cmdType = pCommand->ToUnitCmd().nCmdType;
	return 
		( cmdType == ACTION_COMMAND_MOVE_TO				||
			cmdType == ACTION_COMMAND_ROTATE_TO			||
			cmdType == ACTION_COMMAND_ENTER					||
			cmdType == ACTION_COMMAND_IDLE_BUILDING	||
			cmdType == ACTION_COMMAND_IDLE_TRENCH		||
			cmdType == ACTION_COMMAND_LEAVE					||
			cmdType == ACTION_COMMAND_SWARM_TO			||
			cmdType == ACTION_COMMAND_ATTACK_UNIT		||
			cmdType == ACTION_COMMAND_ATTACK_OBJECT	||
//			cmdType == ACTION_COMMAND_AMBUSH				||
			cmdType == ACTION_COMMAND_GUARD					||
			cmdType == ACTION_COMMAND_PARADE				||
			cmdType == ACTION_COMMAND_LOAD					||
			cmdType == ACTION_COMMAND_IDLE_TRANSPORT ||
			cmdType == ACTION_COMMAND_LOAD_NOW ||
			cmdType == ACTION_MOVE_CATCH_TRANSPORT ||
			cmdType == ACTION_MOVE_PARACHUTE ||
			cmdType == ACTION_COMMAND_CATCH_ARTILLERY ||
			cmdType == ACTION_MOVE_SET_HOME_TRANSPORT ||
			cmdType == ACTION_COMMAND_USE_SPYGLASS ||
			cmdType == ACTION_MOVE_ATTACK_FORMATION ||
			cmdType == ACTION_MOVE_GUNSERVE  ||
			cmdType == ACTION_COMMAND_DISBAND_FORMATION ||
			cmdType == ACTION_COMMAND_FORM_FORMATION || 
			cmdType == ACTION_COMMAND_WAIT_TO_FORM ||
			cmdType == ACTION_COMMAND_FOLLOW ||
			cmdType == ACTION_COMMAND_FOLLOW_NOW ||
			cmdType == ACTION_COMMAND_STOP ||
			cmdType == ACTION_COMMAND_CATCH_FORMATION ||
			cmdType == ACTION_COMMAND_SWARM_ATTACK_UNIT ||
			cmdType == ACTION_MOVE_SWARM_ATTACK_FORMATION ||
			cmdType == ACTION_MOVE_REPAIR_UNIT ||
			cmdType == ACTION_MOVE_RESUPPLY_UNIT ||
			cmdType == ACTION_MOVE_LOAD_RU ||
			cmdType == ACTION_MOVE_BUILD_LONGOBJECT ||
			cmdType == ACTION_MOVE_PLACE_ANTITANK ||
			cmdType == ACTION_MOVE_PLACEMINE ||
			cmdType == ACTION_MOVE_CLEARMINE ||
			cmdType == ACTION_MOVE_REPAIR_BRIDGE ||
			cmdType == ACTION_MOVE_REPAIR_BUILDING ||
			cmdType == ACTION_COMMAND_ROTATE_TO_DIR ||
			cmdType == ACTION_MOVE_ENTER_TRANSPORT_CHEAT_PATH ||
			cmdType == ACTION_COMMAND_STAND_GROUND ||
			cmdType == ACTION_COMMAND_ENTER_BUILDING_NOW ||
			cmdType == ACTION_COMMAND_ENTER_ENTREHCMNENT_NOW ||
			cmdType == ACTION_COMMAND_MOVE_TO_GRID ||
			cmdType == ACTION_COMMAND_PLACE_CHARGE ||
			cmdType == ACTION_COMMAND_PLACE_CONTROLLED_CHARGE ||
			cmdType == ACTION_COMMAND_DETONATE	||
			cmdType == ACTION_COMMAND_THROW_GRENADE ||
			cmdType == ACTION_COMMAND_THROW_ANTITANK_GRENADE ||
			cmdType == ACTION_COMMAND_LAND_MINE ||
			cmdType == ACTION_COMMAND_ENTRENCH_SELF ||
			cmdType == ACTION_COMMAND_WAIT ||
			cmdType == ACTION_COMMAND_SPY_MODE ||
			cmdType == ACTION_COMMAND_PATROL ||
			cmdType == ACTION_MOVE_FIRST_AID ||
			cmdType == ACTION_COMMAND_ENTRENCH_BEGIN ||
			cmdType == ACTION_COMMAND_ENTRENCH_END ||
			cmdType == ACTION_COMMAND_CLEARMINE
		);
}

void StopAllUnits( CFormation *pFormation )
{
	for ( int i = 0; i < pFormation->Size(); ++i )
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP ), (*pFormation)[i], false );
}

bool IsFormationCloseToEnemy( const CVec2 &enemyCenter, CFormation *pFormation )
{
	const float fGoodDistance = pFormation->GetMaxFireRange() * 1.2f;

	if ( fabs( pFormation->GetCenterPlain() - enemyCenter ) < fGoodDistance )
		return true;
	
	for ( int i = 0; i < pFormation->Size(); ++i )
	{
		if ( fabs( (*pFormation)[i]->GetCenterPlain() - enemyCenter ) < fGoodDistance )
			return true;
	}

	return false;
}

IUnitState* CFormationStatesFactory::ProduceState( CQueueUnit *pObj, CAICommand *pCommand )
{
	NI_ASSERT( dynamic_cast<CFormation*>(pObj) != 0, "Wrong unit passed" );
	CFormation *pFormation = checked_cast<CFormation*>(pObj);

	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();
	IUnitState* pResult = 0;
	bool bSwarmAttack = false;

	switch ( cmd.nCmdType )
	{
	case ACTION_MOVE_FIRST_AID:
		pResult = new CFormationFirstAidState( pFormation, dynamic_cast<CSoldier*>( GetObjectByCmd( cmd ) ) );

		break;
	case ACTION_COMMAND_WAIT:
		pResult = CFormationRestState::Instance( pFormation, cmd.vPos, pFormation->GetDirection(), cmd.fNumber );

		break;

	case ACTION_COMMAND_ENTRENCH_SELF:
		{
			const ESpecialAbilityParam eState = (const ESpecialAbilityParam)(int)cmd.fNumber;
			if ( eState ==  PARAM_ABILITY_OFF )
				pResult = new CFormationLeaveSelfEntrenchState( pFormation );
			else if ( eState == PARAM_ABILITY_ON )
				pResult = new CFormationEntrenchSelfState( pFormation );
		}
		break;
	case ACTION_MOVE_LEAVE_SELF_ENTRENCH:
		{
			pResult = new CFormationLeaveSelfEntrenchState( pFormation );
		}
		break;
	case ACTION_MOVE_ENTER_TRANSPORT_CHEAT_PATH:
		{
			CONVERT_OBJECT( CMilitaryCar, pCar, GetObjectByCmd( cmd ), "Wrong unit in idle in transport command" );
			pResult = CFormationEnterTransportByCheatPathState::Instance( pFormation, pCar );
		}

		break;
	case ACTION_MOVE_REPAIR_BUILDING:
		{
			CONVERT_OBJECT( CBuilding, pBuilding, GetObjectByCmd( cmd ), "not building asked to repair" );
			pResult = CFormationRepairBuildingState::Instance( pFormation, pBuilding );
		}

		break;
	case ACTION_MOVE_REPAIR_BRIDGE:
		{
			CObjectBase * pObject = GetObjectByCmd( cmd );
			CONVERT_OBJECT( CBridgeSpan, pFullBridge, GetObjectByCmd( cmd ), "not bridge asked to repair as a bridge" );
			pResult = CFormationRepairBridgeState::Instance( pFormation, pFullBridge );
		}

		break;
	case ACTION_MOVE_LOAD_RU:
		{
			CONVERT_OBJECT( CBuilding, pBuildingStorage, GetObjectByCmd( cmd ), "Wrong storage building" );
			pResult = CFormationLoadRuState::Instance( pFormation, pBuildingStorage );
		}

		break;
	case ACTION_MOVE_GUNSERVE:
		{
			CONVERT_OBJECT( CArtillery, pArtillery, GetObjectByCmd( cmd ), "Wrong artillery unit" );
			pResult = CFormationGunCrewState::Instance( pFormation, pArtillery );
		}

		break;
	case ACTION_COMMAND_CATCH_ARTILLERY:
		{
			CONVERT_OBJECT( CArtillery, pArtillery, GetObjectByCmd( cmd ), "Wrong artillery unit" );
			const bool bDisableUsePart = NGlobal::GetVar( "use_whole_formation_for_guncrew", false );
			pResult = CFormationCaptureArtilleryState::Instance( pFormation, pArtillery, bDisableUsePart ? false : cmd.fNumber );
		}

		break;
	case ACTION_MOVE_PARACHUTE:
		pResult = CFormationParaDropState::Instance( pFormation );

		break;
	case ACTION_COMMAND_SPY_MODE:
		{
			pResult = ProduceRestState( pFormation );
			for ( int i = 0; i < pFormation->Size(); ++i )
			{
				theGroupLogic.UnitCommand( cmd, (*pFormation)[i], false );
			}
		}
		break;
	case ACTION_MOVE_BUILD_LONGOBJECT:
		NI_ASSERT( dynamic_cast<CLongObjectCreation*>(GetObjectByCmd( cmd )) != 0, "wrong creator passed" );
		pResult = CFormationBuildLongObjectState::Instance( pFormation, checked_cast<CLongObjectCreation*>(GetObjectByCmd( cmd )) );

		break;
	case ACTION_MOVE_PLACE_ANTITANK:
		pResult = CFormationPlaceAntitankState::Instance( pFormation, cmd.vPos );

		break;
	case ACTION_COMMAND_MOVE_TO:
		pFormation->UnsetFollowState();
		if ( pFormation->IsInEntrenchment() )
		{
			StopAllUnits( pFormation );					
			pResult = CFormationLeaveEntrenchmentState::Instance( pFormation, pFormation->GetEntrenchment(), ALP_POSITION_VALID, cmd.vPos + pFormation->GetGroupShift() );
		}
		else if ( pFormation->IsInBuilding() )
		{
			pResult = CFormationLeaveBuildingState::Instance( pFormation, pFormation->GetBuilding(), ALP_POSITION_VALID, cmd.vPos );
		}
		else if ( pFormation->IsInTankPit() && !pCommand->IsFromAI() )// сначала выйти из TankPit, потом поехать куда послали
		{
			theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pFormation);
			theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, VNULL2, PARAM_ABILITY_OFF), pFormation );
		}
		else
		{
			StopAllUnits( pFormation );
			pResult = CFormationMoveToState::Instance( pFormation, cmd.vPos );
		}

		break;
	case ACTION_COMMAND_USE_SPYGLASS:
		pResult = CFormationUseSpyglassState::Instance( pFormation, cmd.vPos );

		break;
	case ACTION_COMMAND_PARADE:
		{
			StopAllUnits( pFormation );
			pResult = CFormationParadeState::Instance( pFormation, cmd.fNumber );
		}

		break;
	case ACTION_COMMAND_ROTATE_TO:
		if ( pFormation->IsInTankPit() )
		{
			theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pFormation);
			theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, VNULL2, PARAM_ABILITY_OFF), pFormation );
		}
		else
		{
			StopAllUnits( pFormation );
			pResult = CFormationRotateState::Instance( pFormation, GetDirectionByVector( cmd.vPos - pFormation->GetCenterPlain() ) );
		}

		break;
	case ACTION_COMMAND_ROTATE_TO_DIR:
		if ( pFormation->IsInTankPit() )
		{
			theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pFormation);
			theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, VNULL2, PARAM_ABILITY_OFF), pFormation );
		}
		else
		{
			StopAllUnits( pFormation );
			pResult = CFormationRotateState::Instance( pFormation, GetDirectionByVector( cmd.vPos ) );
		}
		break;
	case ACTION_COMMAND_ENTER:
		if ( GetObjectByCmd( cmd ) != 0 )
		{
			StopAllUnits( pFormation );

			CONVERT_OBJECT( CStaticObject, pObj, GetObjectByCmd( cmd ), "Not static object for command ACTION_COMMAND_ENTER passed" );
			switch ( pObj->GetObjectType() )
			{
			case ESOT_ENTRENCHMENT:
				pResult = CFormationEnterEntrenchmentState::Instance( pFormation, checked_cast<CEntrenchment*>(pObj) );
				break;
			case ESOT_ENTR_PART:
				if ( CEntrenchment *pEntrenchment = checked_cast<CEntrenchmentPart*>(pObj)->GetOwner() )
					pResult = CFormationEnterEntrenchmentState::Instance( pFormation, pEntrenchment );

				break;
			case ESOT_BUILDING:
				pResult = CFormationEnterBuildingState::Instance( pFormation, checked_cast<CBuilding*>(pObj) );
				break;
			default:
				NI_ASSERT( false, StrFmt( "Can't enter to object of type %d, from AI flag %d", pObj->GetObjectType(), (int)cmd.bFromAI ) );
			}
		}

		break;
	case ACTION_COMMAND_IDLE_BUILDING:
		NI_ASSERT( dynamic_cast<CBuilding*>( GetObjectByCmd( cmd ) ) != 0, StrFmt( "Wrong static object (%s) is passed, command ACTION_COMMAND_IDLE_BUILDING", typeid(*GetObjectByCmd( cmd )).name() ) );
		pResult = CFormationIdleBuildingState::Instance( pFormation, checked_cast<CBuilding*>( GetObjectByCmd( cmd ) ) );

		break;
	case ACTION_COMMAND_IDLE_TRENCH:
		NI_ASSERT( dynamic_cast<CEntrenchment*>( GetObjectByCmd( cmd ) ) != 0, StrFmt( "Wrong static object (%s) is passed, command ACTION_COMMAND_IDLE_TRENCH", typeid(*GetObjectByCmd( cmd )).name() ) );
		pResult = CFormationIdleEntrenchmentState::Instance( pFormation, checked_cast<CEntrenchment*>( GetObjectByCmd( cmd ) ) );

		break;
	case ACTION_COMMAND_LEAVE:
		StopAllUnits( pFormation );			
		if ( pFormation->IsInBuilding() )
			pResult = CFormationLeaveBuildingState::Instance( pFormation, pFormation->GetBuilding(), (const EActionLeaveParam)int(cmd.fNumber), cmd.vPos );
		else if ( pFormation->IsInEntrenchment() )
			pResult = CFormationLeaveEntrenchmentState::Instance( pFormation, pFormation->GetEntrenchment(), (const EActionLeaveParam)int(cmd.fNumber), cmd.vPos );

		break;
	case ACTION_MOVE_PLACEMINE:
		StopAllUnits( pFormation );
		pResult = CFormationPlaceMine::Instance( pFormation, cmd.vPos, static_cast<enum EMineType>(int(cmd.fNumber)) );

		break;
	case ACTION_MOVE_CLEARMINE:
	case ACTION_COMMAND_CLEARMINE:
		StopAllUnits( pFormation );			
		pResult = CFormationClearMine::Instance( pFormation, cmd.vPos );

		break;
	case ACTION_COMMAND_SWARM_TO:
		if ( pFormation->IsInTankPit() )
		{
			theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pFormation);
			theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, VNULL2, PARAM_ABILITY_OFF), pFormation );
		}
		else
		{
			StopAllUnits( pFormation );
			pResult = CFormationSwarmState::Instance( pFormation, cmd.vPos, cmd.fNumber );
		}

		break;
	case ACTION_MOVE_SWARM_ATTACK_FORMATION:
		bSwarmAttack = true;
	case ACTION_MOVE_ATTACK_FORMATION:
		NI_ASSERT( dynamic_cast<CFormation*>( GetObjectByCmd( cmd ) ) != 0, "must be formation unit" );
		pResult = CFormationAttackFormationState::Instance( pFormation, checked_cast<CFormation*>( GetObjectByCmd( cmd )), bSwarmAttack );

		break;
	case ACTION_COMMAND_SWARM_ATTACK_UNIT:
		bSwarmAttack = true;
	case ACTION_COMMAND_ATTACK_UNIT:
		{
			CObjectBase * pObj = GetObjectByCmd( cmd );
			if ( dynamic_cast<CFormation*>( pObj ) )
			{
				pResult = CFormationAttackFormationState::Instance( pFormation, static_cast<CFormation*>(pObj), bSwarmAttack );
				break;
			}
			CONVERT_OBJECT( CAIUnit, pTarget, GetObjectByCmd( cmd ), "Wrong unit to attack" );

			if ( !pTarget || !pTarget->IsAlive() )
				pFormation->SendAcknowledgement( ACK_INVALID_TARGET, !pCommand->IsFromAI() );
			else if ( pTarget->GetStats()->IsArtillery()  )
			{
				NI_ASSERT( dynamic_cast<CArtillery*>( GetObjectByCmd( cmd ) ) != 0, "must be artillery unit" );
				CArtillery *pArt = dynamic_cast<CArtillery*>( GetObjectByCmd( cmd ) );
				if (!pArt)
					break;

				if ( pArt->GetCrew() )
					pResult = CFormationAttackFormationState::Instance( pFormation, pArt->GetCrew(), bSwarmAttack );
				else
					pResult = CFormationAttackUnitState::Instance( pFormation, pArt, bSwarmAttack );
			}
			else
				pResult = CFormationAttackUnitState::Instance( pFormation, checked_cast<CAIUnit*>( GetObjectByCmd( cmd ) ), bSwarmAttack );

		}

		break;
	case ACTION_COMMAND_ATTACK_OBJECT:
		{
			CONVERT_OBJECT( CStaticObject, pStaticObj, GetObjectByCmd( cmd ), StrFmt( "Wrong static object to attack (%s)", typeid(GetObjectByCmd( cmd )).name() ) );
			// attack the artillery
			if ( pStaticObj->GetObjectType() == ESOT_ARTILLERY_BULLET_STORAGE )
			{
				pCommand->ToUnitCmd().nCmdType = ACTION_COMMAND_ATTACK_UNIT;
				pCommand->ToUnitCmd().nObjectID = checked_cast<CArtilleryBulletStorage*>(pStaticObj)->GetOwner()->GetUniqueId();
				pCommand->ToUnitCmd().fNumber = 0;
				pResult = ProduceState( pObj, pCommand );
			}
			else
				pResult = CFormationAttackCommonStatObjState::Instance( pFormation, pStaticObj );
		}

		break;
	case ACTION_COMMAND_THROW_GRENADE:
	case ACTION_COMMAND_THROW_ANTITANK_GRENADE:
		{
			const int action = pCommand->ToUnitCmd().fNumber;
			if ( action == PARAM_ABILITY_AUTOCAST_ON ||
				action == PARAM_ABILITY_AUTOCAST_OFF ) 
				break;
			//NI_ASSERT( cmd.nNumber == EASS_READY_TO_ON, "Wrong param to throw grenade command" );
			if ( pFormation->GetState()->GetName() == EUSN_FORM_THROW_GRENADE )
				pResult = checked_cast<CFormationThrowGrenadeState *>(pFormation->GetState());
			else
				pResult = CFormationThrowGrenadeState::Instance( pFormation );
			CFormationThrowGrenadeState *pThrowGrenadeState = checked_cast<CFormationThrowGrenadeState *>( pResult );
			if ( pCommand->ToUnitCmd().nNumber == ATGP_ATACK_POINT ) 
			{
				pThrowGrenadeState->AddTarget( 0, cmd.vPos, cmd.nNumber );
			}
			else
			{
				CObjectBase *pTarget = GetObjectByCmd( cmd );
				if ( pTarget )
					pThrowGrenadeState->AddTarget( checked_cast<CAIUnit*>( pTarget ), cmd.vPos, cmd.nNumber );
			}
		}
		break;
	case ACTION_COMMAND_GUARD:
		if ( pFormation->IsFree() && pFormation->HasMortar() )
			return CFormationInstallMortarState::Instance( pFormation );
		else if ( pFormation->IsInBuilding() || pFormation->IsInEntrenchment() )
			return pFormation->GetState();
		else
		{
			StopAllUnits( pFormation );
			pResult = CFormationRestState::Instance( pFormation, cmd.vPos, cmd.fNumber, -1 );
		}

		break;
		/*		case ACTION_COMMAND_AMBUSH:
		StopAllUnits( pFormation );
		pResult = CCommonAmbushState::Instance( pFormation );

		break;*/
	case ACTION_COMMAND_IDLE_TRANSPORT:
		NI_ASSERT( dynamic_cast<CMilitaryCar*>( GetObjectByCmd( cmd ) ) != 0, "Wrong unit in idle in transport command" );
		pResult = CFormationIdleTransportState::Instance( pFormation, checked_cast<CMilitaryCar*>( GetObjectByCmd( cmd ) ) );

		break;
	case ACTION_COMMAND_LOAD:
		{
			CONVERT_OBJECT( CMilitaryCar, pCar, GetObjectByCmd( cmd ), StrFmt( "Wrong unit to load to %s",typeid( *pObj ).name()) );
			pResult = CFormationEnterTransportState::Instance( pFormation, pCar );
		}

		break;
	case ACTION_COMMAND_LOAD_NOW:
		{
			CONVERT_OBJECT( CMilitaryCar, pCar, GetObjectByCmd( cmd ), StrFmt( "Wrong unit to load to %s",typeid( *pObj ).name()) );
			pResult = CFormationEnterTransportNowState::Instance( pFormation, pCar );
		}

		break;
	case ACTION_MOVE_REPAIR_UNIT:
		{
			CAIUnit *pUnitToRepair = GetObjectByCmd( cmd ) ? dynamic_cast<CAIUnit*>( GetObjectByCmd( cmd ) ) : 0;
			pResult = CFormationRepairUnitState::Instance( pFormation, pUnitToRepair );
		}

		break;
	case ACTION_MOVE_SET_HOME_TRANSPORT:
		NI_ASSERT( dynamic_cast<IEngineerFormationState*>( pFormation->GetState())!=0, "bad state sequence" );
		NI_ASSERT( dynamic_cast<CAITransportUnit*>( GetObjectByCmd( cmd ) ) != 0, "Wrong home transport" );

		checked_cast<IEngineerFormationState*>( pFormation->GetState() )->SetHomeTransport(  checked_cast<CAITransportUnit*>( GetObjectByCmd( cmd ) ) );
		pResult = pFormation->GetState();

		break;
	case ACTION_MOVE_RESUPPLY_UNIT:
		NI_ASSERT( GetObjectByCmd( cmd ) ? dynamic_cast<CAIUnit*>( GetObjectByCmd( cmd ) ) != 0 : true, StrFmt( "Wrong preferred unit %s",typeid( *pObj ).name()) );
		pResult = CFormationResupplyUnitState::Instance( pFormation, checked_cast<CAIUnit*>(GetObjectByCmd( cmd )) );

		break;
	case ACTION_MOVE_CATCH_TRANSPORT:
		NI_ASSERT( dynamic_cast<CAITransportUnit*>( GetObjectByCmd( cmd ) ) != 0, "Wrong unit to load to" );
		pResult = CFormationCatchTransportState::Instance( pFormation, checked_cast<CAITransportUnit*>(GetObjectByCmd( cmd )), cmd.fNumber );

		break;
	case ACTION_COMMAND_DISBAND_FORMATION:
		pResult = CFormationDisbandState::Instance( pFormation );

		break;
	case ACTION_COMMAND_FORM_FORMATION:
		pResult = CFormationFormState::Instance( pFormation );

		break;
	case ACTION_COMMAND_WAIT_TO_FORM:
		NI_ASSERT( dynamic_cast<CSoldier*>(GetObjectByCmd( cmd )) != 0, "Not common unit in follow command" );
		pResult = CFormationWaitToFormState::Instance( pFormation, cmd.fNumber, checked_cast<CSoldier*>(GetObjectByCmd( cmd )) );

		break;
	case ACTION_COMMAND_FOLLOW:
		{
			CONVERT_OBJECT( CCommonUnit, pUnit, GetObjectByCmd( cmd ), "Not common unit in follow command" );
			if ( IsValidObj( pUnit ) )
			{
				if ( pFormation->IsInTankPit() )
				{
					theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pFormation);
					theGroupLogic.InsertUnitCommand( SAIUnitCmd(ACTION_COMMAND_ENTRENCH_SELF, VNULL2, PARAM_ABILITY_OFF), pFormation );
				}
				else
					pFormation->SetFollowState( pUnit );
			}
		}

		break;
	case ACTION_COMMAND_FOLLOW_NOW:
		{
			CONVERT_OBJECT( CCommonUnit, pUnit, GetObjectByCmd( cmd ), "Not common unit in follow command" );
			pResult = CFollowState::Instance( pFormation, pUnit );
		}

		break;
	case ACTION_COMMAND_STOP:
		if ( !pFormation->IsIdle() )
			pFormation->BalanceCenter();

		break;
	case ACTION_COMMAND_CATCH_FORMATION:
		{
			CONVERT_OBJECT( CFormation, pFormationToCatch, GetObjectByCmd( cmd ), "Not formation in ACTION_COMMAND_CATCH_FORMATION command" );
			pResult = CCatchFormationState::Instance( pFormation, pFormationToCatch );
		}

		break;
	case ACTION_COMMAND_STAND_GROUND:
		pFormation->Stop();
		pFormation->UnsetFollowState();				
		pFormation->SetBehaviourMoving( SBehaviour::EMHoldPos );

		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			CSoldier *pSoldier = (*pFormation)[i];
			pSoldier->Stop();
			pSoldier->SetBehaviourMoving( SBehaviour::EMHoldPos );
		}

		break;
	case ACTION_COMMAND_ENTER_BUILDING_NOW:
		{
			CONVERT_OBJECT( CBuilding, pBuilding, GetObjectByCmd( cmd ), "Not building in ACTION_COMMAND_ENTER_BUILDING_NOW command" );
			pResult = CFormationEnterBuildingNowState::Instance( pFormation, pBuilding );
		}

		break;
	case ACTION_COMMAND_ENTER_ENTREHCMNENT_NOW:
		{
			CONVERT_OBJECT( CEntrenchmentPart, pEntrenchmentPart, GetObjectByCmd( cmd ), "Not entrechmnent in ACTION_COMMAND_ENTER_BUILDING_NOW command" );
			if ( pEntrenchmentPart->GetOwner() )
				pResult = CFormationEnterEntrenchmentNowState::Instance( pFormation, pEntrenchmentPart->GetOwner() );
		}

		break;
	case ACTION_COMMAND_MOVE_TO_GRID:
		pResult = CCommonMoveToGridState::Instance( pFormation, cmd.vPos, GetVectorByDirection( cmd.fNumber ) );

		break;
	case ACTION_COMMAND_PLACE_CHARGE:
		pResult = CFormationPlaceChargeState::Instance( pFormation, cmd.vPos, NDb::ABILITY_PLACE_CHARGE, 20000 );

		break;
	case ACTION_COMMAND_PLACE_CONTROLLED_CHARGE:
		pResult = CFormationPlaceChargeState::Instance( pFormation, cmd.vPos, NDb::ABILITY_PLACE_CONTROLLED_CHARGE, 0 );

		break;
	case ACTION_COMMAND_LAND_MINE:
		pResult = CFormationPlaceChargeState::Instance( pFormation, cmd.vPos, NDb::ABILITY_LAND_MINE, 0 );

		break;
	case ACTION_COMMAND_DETONATE:
		pResult = CFormationDetonateChargeState::Instance( pFormation );

		break;
	case ACTION_COMMAND_PATROL:
		{
			CVec2 vTarget( cmd.vPos );
			if ( pFormation->CanMove() )
				pResult = CCommonPatrolState::Instance( pFormation, vTarget );
			else
			{
				pFormation->SendAcknowledgement( pCommand, ACK_NEGATIVE, !pCommand->IsFromAI() );
			}
		}
		break;
	case ACTION_COMMAND_ENTRENCH_BEGIN:
		{
			pResult = CFormationBuildEntrenchmentState::Instance( pFormation, NLongObjectCreation::Create<CEntrenchmentCreation>( cmd.vPos, pFormation->GetPlayer(), true ), cmd.vPos );
			break;
		}
	case ACTION_COMMAND_ENTRENCH_END:
		if ( pFormation->GetState()->GetName() == EUSN_BUILD_ENTRENCHMENT )
		{
      NI_ASSERT( dynamic_cast<CFormationBuildEntrenchmentState*>( pFormation->GetState() ) != 0, "bad state sequence" );
			if ( cmd.nNumber > 0 )			// if it is a cancellation command, nNumber=-1, do not set endpoint
				checked_cast<CFormationBuildEntrenchmentState*>( pFormation->GetState() )->SetEndPoint( cmd.vPos );

			pResult = pFormation->GetState();
		}
		break;
	default:
		NI_ASSERT( false, "Wrong command" );
	}

	return pResult;
}

IUnitState* CFormationStatesFactory::ProduceRestState( CQueueUnit *pUnit )
{
	NI_ASSERT( dynamic_cast<CFormation*>(pUnit) != 0, "Wrong unit's type" );

	CFormation *pFormation = checked_cast<CFormation*>(pUnit);
	StopAllUnits( pFormation );
	pFormation->Stop();

	if ( pFormation->IsFree() )
	{
		if ( pFormation->HasMortar()  )
			return CFormationInstallMortarState::Instance( pFormation );
		return CFormationRestState::Instance( pFormation, CVec2( -1, -1 ), 0, -1 );
	}
	else if ( pFormation->IsInBuilding() )
		return CFormationIdleBuildingState::Instance( pFormation, pFormation->GetBuilding() );
	else if ( pFormation->IsInEntrenchment() )
		return CFormationIdleEntrenchmentState::Instance( pFormation, pFormation->GetEntrenchment() );
	else
		return CFormationIdleTransportState::Instance( pFormation, pFormation->GetTransportUnit() );
}

//*******************************************************************
//*											 CFormationRestState												*
//*******************************************************************

IUnitState* CFormationRestState::Instance( CFormation *pFormation, const CVec2 &guardPoint, const WORD wDir, const float _fTimeToWait )
{
	return new CFormationRestState( pFormation, guardPoint, wDir, _fTimeToWait );
}

CFormationRestState::CFormationRestState( CFormation *_pFormation, const CVec2 &guardPoint, const WORD wDir, const float _fTimeToWait )
: pFormation( _pFormation ), CCommonRestState( guardPoint, wDir, _pFormation, _fTimeToWait )
{
	pFormation->SetToWaitingState();
}

void CFormationRestState::Segment()
{
	CCommonRestState::Segment();
}

CCommonUnit* CFormationRestState::GetUnit() const 
{ 
	return pFormation; 
}

ETryStateInterruptResult CFormationRestState::TryInterruptState( CAICommand *pCommand )
{
	pFormation->UnsetFromWaitingState();
	pFormation->SetCommandFinished();

	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*											 CFormationMoveToState											*
//*******************************************************************

IUnitState* CFormationMoveToState::Instance( CFormation *pFormation, const CVec2 &point )
{
	return new CFormationMoveToState( pFormation, point );
}

CFormationMoveToState::CFormationMoveToState(  CFormation *_pFormation, const CVec2 &_point )
: pFormation( _pFormation ), startTime( curTime ), bWaiting( true ), eMoveToState( EMTS_FORMATION_MOVING )
{
}

void CFormationMoveToState::FormationMovingState()
{
	if ( bWaiting )
	{
		if ( curTime - startTime >= TIME_OF_WAITING )
		{
			bWaiting = false;

			pFormation->MoveGeometries2Center();
			if ( CPtr<IStaticPath> pStaticPath = pFormation->GetCurCmd()->CreateStaticPath( pFormation ) )
				pFormation->SendAlongPath( pStaticPath, pFormation->GetGroupShift(), true );
			else
			{
				pFormation->SendAcknowledgement( ACK_NEGATIVE, true );
				pFormation->SetCommandFinished();
			}
		}
	}
	else if ( pFormation->IsIdle() )
	{
		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			CSoldier *pSoldier = (*pFormation)[i];
			const CVec2 vPoint = pSoldier->GetUnitPointInFormation();
			const bool bTooFar = mDistance( vPoint, pSoldier->GetCenterPlain() ) > 1.5f * SConsts::TILE_SIZE;

			if ( bTooFar )
			{
				CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( vPoint, VNULL2, pSoldier, false, GetAIMap() ); 
				if ( pStaticPath )
					pSoldier->SendAlongPath( pStaticPath, VNULL2, true );
			}
		}

		eMoveToState = EMTS_UNITS_MOVING_TO_FORMATION_POINTS;
	}
}

void CFormationMoveToState::UnitsMovingToFormationPoints()
{
	int i = 0;
	while ( i < pFormation->Size() && (*pFormation)[i]->IsIdle() )
		++i;

	if ( i >= pFormation->Size() )
		pFormation->SetCommandFinished();
}

void CFormationMoveToState::Segment()
{
	switch ( eMoveToState )
	{
		case EMTS_FORMATION_MOVING:
			FormationMovingState();

			break;
		case EMTS_UNITS_MOVING_TO_FORMATION_POINTS:
			UnitsMovingToFormationPoints();

			break;
	}
}

ETryStateInterruptResult CFormationMoveToState::TryInterruptState( class CAICommand *pCommand )
{ 
	pFormation->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*										CFormationEnterBuildingState									*
//*******************************************************************

IUnitState* CFormationEnterBuildingState::Instance( CFormation *pFormation, CBuilding *pBuilding )
{
	return new CFormationEnterBuildingState( pFormation, pBuilding );
}

CFormationEnterBuildingState::CFormationEnterBuildingState( CFormation *_pFormation, CBuilding *_pBuilding )
: CStatusUpdatesHelper( EUS_LOAD, _pFormation ), pFormation( _pFormation ), pBuilding( _pBuilding ), state( EES_START ), timeSent( curTime )
{
	int i = 0;
	while ( i < pBuilding->GetNEntrancePoints() )
	{
		if ( pBuilding->IsGoodPointForRunIn( pFormation->GetCenterPlain(), i, pFormation->GetMaxProjection() * 1.2f ) )
		{
			nEntrance = i;
			state = EES_RUN_UP;
			break;
		}
		else
			++i;
	}
}

bool CFormationEnterBuildingState::SetPathForRunUp()
{
	CPtr<IStaticPath> pBestPath = pFormation->GetPathToBuilding( pBuilding, &nEntrance );
	
	if ( pBestPath == 0 )
		return false;
	else
	{
		pFormation->SendAlongPath( pBestPath, VNULL2, true );
		return true;
	}
}

void CFormationEnterBuildingState::SendUnitsToBuilding()
{
	// послать юнитов в здание
	pFormation->Stop();
	for ( int i = 0; i < pFormation->Size(); ++i )
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_ENTER, pBuilding->GetUniqueId() ), (*pFormation)[i], false );
}

bool CFormationEnterBuildingState::IsNotEnoughSpace()
{
	return
		theDipl.GetDiplStatus( pFormation->GetPlayer(), pBuilding->GetPlayer() ) != EDI_ENEMY && pBuilding->GetNFreePlaces() < pFormation->Size() ||
		theDipl.GetDiplStatus( pFormation->GetPlayer(), pBuilding->GetPlayer() ) == EDI_ENEMY && pBuilding->GetNFriendlyAttackers( pFormation->GetPlayer() ) + pFormation->Size() > pBuilding->GetNOverallPlaces();
}

void CFormationEnterBuildingState::Segment()
{
	//NI_ASSERT( !pFormation->IsInWaitingState(), "Wrong formation waiting state" );
	
	// здание разрушено или там не хватит места
	if ( !IsValidObj( pBuilding ) || state != EES_WAITINIG_TO_ENTER && pBuilding->GetPlayer() != -1 && IsNotEnoughSpace() )
	{
		pFormation->SendAcknowledgement( ACK_NEGATIVE );
		TryInterruptState( 0 );
	}
	else if ( IsAlly( pBuilding->GetPlayer(), pFormation->GetPlayer() ) && pBuilding->GetNDefenders() > 0 )
	{
		pFormation->SendAcknowledgement( ACK_NEGATIVE );
		TryInterruptState( 0 );
	}
	else
	{
		switch ( state )
		{
			case EES_START:
				if ( SetPathForRunUp() )
					state = EES_RUN_UP;
				else
				{
					pFormation->SendAcknowledgement( ACK_NEGATIVE );					
					pFormation->SetCommandFinished();
				}
	
				break;
			case EES_RUN_UP:
				{
					InitStatus();
					bool bGoodPoint = pBuilding->IsGoodPointForRunIn( pFormation->GetCenterPlain(), nEntrance, pFormation->GetRadius() );
					if ( pFormation->IsIdle() || bGoodPoint )
					{
						if ( !bGoodPoint )
							state = EES_START;
						else
						{
							if ( pFormation->Size() > pBuilding->GetNFreePlaces() )
								pFormation->SetCommandFinished();
							else if ( pBuilding->IsLocked( pFormation->GetPlayer() ) )
								state = EES_WAIT_FOR_UNLOCK;
							else
							{
								pBuilding->Lock( pFormation );
								state = EES_WAITINIG_TO_ENTER;
								timeSent = curTime;
								SendUnitsToBuilding();
							}
						}
					}
				}

				break;
			case EES_WAIT_FOR_UNLOCK:
				if ( pFormation->Size() > pBuilding->GetNFreePlaces() )
					pFormation->SetCommandFinished();
				else if ( !pBuilding->IsLocked( pFormation->GetPlayer() ) )
				{
					timeSent = curTime;
					pBuilding->Lock( pFormation );
					state = EES_WAITINIG_TO_ENTER;
					SendUnitsToBuilding();
				}

				break;
			case EES_WAITINIG_TO_ENTER:
				// wait for some time for soldiers to enter state, or, possibly, building
				if ( timeSent + 1000 < curTime )
				{
					int nInBuilding = 0;
					for ( int i = 0; i < pFormation->Size(); ++i )
					{
						CSoldier *pSold = (*pFormation)[i];
						if ( pSold->IsIdle() )
						{
							if ( pSold->IsInBuilding() )
								++nInBuilding;
							else if ( pSold->GetState()->IsRestState() )
							{
								theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_BUILDING, pBuilding->GetUniqueId() ), pSold, false );
								timeSent = curTime;
							}
						}
					}

					if ( nInBuilding == pFormation->Size() )
					{
						pBuilding->Unlock( pFormation );
						state = EES_FINISHED;
						pFormation->SetCommandFinished();
						theGroupLogic.PushFrontUnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_BUILDING, pBuilding->GetUniqueId() ), pFormation );
					}
				}

				break;
		}
	}
}

ETryStateInterruptResult CFormationEnterBuildingState::TryInterruptState( class CAICommand *pCommand )
{
	const EActionCommand eCmd = pCommand ? pCommand->ToUnitCmd().nCmdType : EActionCommand(0);
	if ( state == EES_WAITINIG_TO_ENTER && pCommand && eCmd != ACTION_COMMAND_LEAVE )
	{
		int i = 0;
		while ( i < pFormation->Size() && !(*pFormation)[i]->IsInBuilding() )
			++i;

		// part of the squad is already in the building
		if ( i < pFormation->Size() )
			return TSIR_NO_COMMAND_INCOMPATIBLE;
		else
		{
			for ( int i = 0; i < pFormation->Size(); ++i )
				theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP ), (*pFormation)[i], false );

			if ( IsValidObj( pBuilding ) )
				pBuilding->Unlock( pFormation );

			pFormation->SetCommandFinished();

			return TSIR_YES_IMMIDIATELY;
		}
	}

	if ( state != EES_WAITINIG_TO_ENTER )
	{
		pFormation->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	else if ( pCommand && ( eCmd == ACTION_COMMAND_LEAVE || eCmd == ACTION_COMMAND_MOVE_TO ) && IsValidObj( pBuilding ) )
	{
		pBuilding->Unlock( pFormation );
		pFormation->SetInBuilding( pBuilding );
		pFormation->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	else
		return TSIR_YES_WAIT;
}

const CVec2 CFormationEnterBuildingState::GetPurposePoint() const
{
	if ( IsValidObj( pBuilding ) )
		return CVec2(pBuilding->GetCenter().x,pBuilding->GetCenter().y);
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*									CFormationEnterEntrenchmentState								*
//*******************************************************************

IUnitState* CFormationEnterEntrenchmentState::Instance( CFormation *pFormation, CEntrenchment *pEntrenchment )
{
	return new CFormationEnterEntrenchmentState( pFormation, pEntrenchment );
}

CFormationEnterEntrenchmentState::CFormationEnterEntrenchmentState( CFormation *_pFormation, CEntrenchment *_pEntrenchment )
: CStatusUpdatesHelper( EUS_LOAD, _pFormation ), pFormation( _pFormation ), pEntrenchment( _pEntrenchment ), state( EES_START ), timeToWait( curTime )
{
	if ( IsAnyPartCloseToEntrenchment() )
		state = EES_RUN;
}

bool CFormationEnterEntrenchmentState::IsAnyPartCloseToEntrenchment() const
{
	// посмотреть положение центра формации
	const float fFormationRadius2 = sqr( (std::max)( pFormation->GetRadius(), SConsts::TILE_SIZE * 4.0f ) );
	const CVec2 vFormationCenter = pFormation->GetCenterPlain();
	CVec2 vClosestPoint;
	pEntrenchment->GetClosestPoint( vFormationCenter, &vClosestPoint );
	
	if ( fabs2( vClosestPoint - vFormationCenter ) < fFormationRadius2 )
		return true;
	
	// посмотреть положения каждого из солдатиков
	int i = 0;
	while ( i < pFormation->Size() )
	{
		const CVec2 vSoldierCenter = (*pFormation)[i]->GetCenterPlain();
		CVec2 vClosestPoint;
		pEntrenchment->GetClosestPoint( vSoldierCenter, &vClosestPoint );

		if ( fabs2( vClosestPoint - vSoldierCenter ) < fFormationRadius2 )
			return true;
		else
			++i;
	}

	return false;
}

bool CFormationEnterEntrenchmentState::SetPathForRunIn()
{
	CPtr<IStaticPath> pPath = pFormation->GetPathToEntrenchment( pEntrenchment );

	if ( pPath )
	{
		pFormation->SendAlongPath( pPath, VNULL2, true );
		return true;
	}
	else
		return false;
}

void CFormationEnterEntrenchmentState::EnterToEntrenchment()
{
	for ( int i = 0; i < pFormation->Size(); ++i )
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_ENTER, pEntrenchment->GetUniqueId(), float(1) ), (*pFormation)[i], false );

	state = EES_WAIT_TO_ENTER;
}

void CFormationEnterEntrenchmentState::Segment()
{
	// окоп non valid или там кто-то уже сидит
	if ( !IsValidObj( pEntrenchment ) )
		TryInterruptState( 0 );	
	switch ( state )
	{
		case EES_START:
			if ( SetPathForRunIn() )
				state = EES_RUN;
			else
			{
				pFormation->SendAcknowledgement( ACK_NEGATIVE );
				pFormation->SetCommandFinished();
			}

			break;
		case EES_RUN:
			InitStatus();
			if ( !IsValidObj( pEntrenchment ) )
				pFormation->SetCommandFinished();
			else if ( IsAnyPartCloseToEntrenchment() )
			{
				EnterToEntrenchment();
				timeToWait = curTime + 200;
			}
			else if ( pFormation->IsIdle() )
				state = EES_START;

			break;
		case EES_WAIT_TO_ENTER:
		
			if ( curTime > timeToWait )
			{
				int nInBuilding = 0;
				for ( int i = 0; i < pFormation->Size(); ++i )
				{
					CSoldier *pSold = (*pFormation)[i];
					if ( pSold->GetState()->IsRestState() && !pSold->IsInEntrenchment() )
					{
						theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRENCH, pEntrenchment->GetUniqueId(), float(2) ), pSold, false );
						//theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRENCH, pEntrenchment->GetUniqueId() ), pSold, false );
						timeToWait = curTime + 200;
					}
					else if ( pSold->IsInEntrenchment() )
							++nInBuilding;
				}

				if ( nInBuilding == pFormation->Size() )
				{
					pFormation->SetCommandFinished();
					theGroupLogic.PushFrontUnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRENCH, pEntrenchment->GetUniqueId() ), pFormation );
				}
			}
		/*{
			int i = 0;
			while ( i < pFormation->Size() && (*pFormation)[i]->IsInEntrenchment() )
				++i;

			if ( i >= pFormation->Size() )
			{
				pFormation->SetCommandFinished();
				theGroupLogic.PushFrontUnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRENCH, pEntrenchment->GetUniqueId() ), pFormation );
			}
		}*/

		break;
	}
}

ETryStateInterruptResult CFormationEnterEntrenchmentState::TryInterruptState( class CAICommand *pCommand )
{
	if ( state == EES_WAIT_TO_ENTER && pCommand && pCommand->ToUnitCmd().nCmdType != ACTION_COMMAND_LEAVE )
	{
		int i = 0;
		while ( i < pFormation->Size() && !(*pFormation)[i]->IsInEntrenchment() )
			++i;

		if ( i < pFormation->Size() )
			return TSIR_NO_COMMAND_INCOMPATIBLE;
		else
		{
			for ( int i = 0; i < pFormation->Size(); ++i )
				theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP ), (*pFormation)[i], false );
			pFormation->SetCommandFinished();

			return TSIR_YES_IMMIDIATELY;
		}
	}

	pFormation->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CFormationEnterEntrenchmentState::GetPurposePoint() const
{
	return CVec2(pEntrenchment->GetCenter().x,pEntrenchment->GetCenter().y);
}

//*******************************************************************
//*									CFormationIdleBuildingState											*
//*******************************************************************

IUnitState* CFormationIdleBuildingState::Instance( CFormation *pFormation, CBuilding *pBuilding )
{
	return new CFormationIdleBuildingState( pFormation, pBuilding );
}

CFormationIdleBuildingState::CFormationIdleBuildingState( CFormation *_pFormation, CBuilding *_pBuilding )
: pFormation( _pFormation ), pBuilding( _pBuilding )
{
	pFormation->StopFormationCenter();
	pFormation->SetInBuilding( pBuilding );
}

void CFormationIdleBuildingState::Segment()
{
}

ETryStateInterruptResult CFormationIdleBuildingState::TryInterruptState( CAICommand *pCommand )
{
	if ( !pCommand || pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_LEAVE )
	{
		pFormation->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	const EActionCommand cmdType = pCommand->ToUnitCmd().nCmdType;

	if ( cmdType == ACTION_COMMAND_ENTER )
	{
		// the same building or command from mass load
		if ( pFormation->GetBuilding()->GetUniqueId() == pCommand->ToUnitCmd().nObjectID || pCommand->GetFlag() == 1 )
			return TSIR_NO_COMMAND_INCOMPATIBLE;
	}

	// mass load
	if ( cmdType == ACTION_COMMAND_LOAD && pCommand->GetFlag() == 1 )
		return TSIR_NO_COMMAND_INCOMPATIBLE;
	
	//некотороые команды в данном состоянии невозможны
	if ( cmdType == ACTION_COMMAND_ROTATE_TO || cmdType == ACTION_COMMAND_ROTATE_TO_DIR ||
			 cmdType == ACTION_COMMAND_USE_SPYGLASS || IsRestCommand( cmdType ) || cmdType == ACTION_COMMAND_STAND_GROUND )
		return TSIR_NO_COMMAND_INCOMPATIBLE;

	// эти команды можно выполнить, не выходя из здания
	if ( cmdType == ACTION_COMMAND_CATCH_FORMATION )
	{
		pFormation->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}

	// некоторые команды передать солдатам
	if (	cmdType == ACTION_COMMAND_ATTACK_OBJECT ||
				cmdType == ACTION_COMMAND_ATTACK_UNIT ||
//				cmdType == ACTION_COMMAND_AMBUSH ||
				cmdType == ACTION_COMMAND_STOP ||
				cmdType == ACTION_COMMAND_STOP_THIS_ACTION )
	{
		bool bPossible = true;
		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			if ( !(*pFormation)[i]->IsInBuilding() )
			{
				bPossible = false;
				break;
			}
		}
		
		if ( bPossible )		
		{
			for ( int i = 0; i< pFormation->Size(); ++i )
				theGroupLogic.UnitCommand( pCommand->ToUnitCmd(), (*pFormation)[i], false );

			return TSIR_YES_MANAGED_ALREADY;
		}
		else
			return TSIR_NO_COMMAND_INCOMPATIBLE;
	}

	// все остальные команды - через выход из здания
	if ( cmdType != ACTION_COMMAND_WAIT_TO_FORM	&&
			 cmdType != ACTION_COMMAND_DISBAND_FORMATION &&
			 cmdType != ACTION_COMMAND_FORM_FORMATION )
	{
		int nEntrance;
		CVec2 vPoint( GetGoPointByCommand( pCommand->ToUnitCmd() ) );
		if ( pBuilding->ChooseEntrance( pFormation, vPoint, &nEntrance ) )
		{
			vPoint = pBuilding->GetEntrancePoint( nEntrance );
			theGroupLogic.PushFrontUnitCommand( pCommand->ToUnitCmd(), pFormation );
			theGroupLogic.PushFrontUnitCommand( SAIUnitCmd( ACTION_COMMAND_LEAVE, vPoint ), pFormation );
		}
		else
			return TSIR_NO_COMMAND_INCOMPATIBLE;
	}
	else
		theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pFormation );

	pFormation->SetCommandFinished();
	return TSIR_YES_MANAGED_ALREADY;
}

const CVec2 CFormationIdleBuildingState::GetPurposePoint() const
{
	return pFormation->GetCenterPlain();
}

//*******************************************************************
//*								CFormationIdleEntrenchmentState										*
//*******************************************************************

IUnitState* CFormationIdleEntrenchmentState::Instance( CFormation *pFormation, CEntrenchment *pEntrenchment )
{
	return new CFormationIdleEntrenchmentState( pFormation, pEntrenchment );
}

CFormationIdleEntrenchmentState::CFormationIdleEntrenchmentState( CFormation *_pFormation, CEntrenchment *_pEntrenchment )
: pFormation( _pFormation ), pEntrenchment( _pEntrenchment )
{
	pFormation->StopFormationCenter();
	pFormation->SetInEntrenchment( pEntrenchment );
}

void CFormationIdleEntrenchmentState::Segment()
{
}

ETryStateInterruptResult CFormationIdleEntrenchmentState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand || pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_MOVE_TO )
	{
		pFormation->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}

	const EActionCommand cmdType = pCommand->ToUnitCmd().nCmdType;

	if ( cmdType == ACTION_COMMAND_ENTER )
	{
		// the same entrehcment or command from mass load
		if ( pFormation->GetEntrenchment()->GetUniqueId() == pCommand->ToUnitCmd().nObjectID || pCommand->GetFlag() == 1 )
			return TSIR_NO_COMMAND_INCOMPATIBLE;
	}

		// mass load
	if ( cmdType == ACTION_COMMAND_LOAD && pCommand->GetFlag() == 1 )
		return TSIR_NO_COMMAND_INCOMPATIBLE;

	//некотороые команды в данном состоянии невозможны
	if ( cmdType == ACTION_COMMAND_ROTATE_TO || cmdType == ACTION_COMMAND_ROTATE_TO_DIR || 
			 IsRestCommand( cmdType ) || cmdType == ACTION_COMMAND_STAND_GROUND )
		return TSIR_NO_COMMAND_INCOMPATIBLE;

		// эти команды можно выполнить, не выходя из здания
	if ( cmdType == ACTION_COMMAND_CATCH_FORMATION )
	{
		pFormation->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}

	// некоторые команды передать солдатам
	if (	cmdType == ACTION_COMMAND_ATTACK_OBJECT ||
				cmdType == ACTION_COMMAND_ATTACK_UNIT ||
//				cmdType == ACTION_COMMAND_AMBUSH ||
				cmdType == ACTION_COMMAND_USE_SPYGLASS ||
				cmdType == ACTION_COMMAND_STOP ||
				cmdType == ACTION_COMMAND_STOP_THIS_ACTION )
	{
		bool bPossible = true;
		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			if ( !(*pFormation)[i]->IsInEntrenchment() )
			{
				bPossible = false;
				break;
			}
		}
		
		if ( bPossible )
		{
			for ( int i=0; i< pFormation->Size(); ++i )
				theGroupLogic.UnitCommand( pCommand->ToUnitCmd(), (*pFormation)[i], false );

			return TSIR_YES_MANAGED_ALREADY;
		}
		else
			return TSIR_NO_COMMAND_INCOMPATIBLE;
	}

	// все остальные команды - через выход из окопа.
	if ( cmdType != ACTION_COMMAND_WAIT_TO_FORM	&&
			 cmdType != ACTION_COMMAND_DISBAND_FORMATION &&
			 cmdType != ACTION_COMMAND_FORM_FORMATION )
	{
		theGroupLogic.PushFrontUnitCommand( pCommand->ToUnitCmd(), pFormation );

		CVec2 vPos;
		pEntrenchment->GetClosestPoint( GetGoPointByCommand( pCommand->ToUnitCmd() ), &vPos );
		theGroupLogic.PushFrontUnitCommand( SAIUnitCmd( ACTION_COMMAND_LEAVE, vPos ), pFormation );
	}
	else
		theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pFormation );

	pFormation->SetCommandFinished();

	return TSIR_YES_MANAGED_ALREADY;
}

const CVec2 CFormationIdleEntrenchmentState::GetPurposePoint() const
{
	return pFormation->GetCenterPlain();
}

//*******************************************************************
//*									CFormationLeaveBuildingState										*
//*******************************************************************

IUnitState* CFormationLeaveBuildingState::Instance( CFormation *pFormation, CBuilding *pBuilding, const enum EActionLeaveParam param, const CVec2 &point )
{
	return new CFormationLeaveBuildingState( pFormation, pBuilding, param, point );
}

CFormationLeaveBuildingState::CFormationLeaveBuildingState( CFormation *_pFormation, CBuilding *_pBuilding, const enum EActionLeaveParam param, const CVec2 &_point )
: CStatusUpdatesHelper( EUS_LEAVE, _pFormation ), pFormation( _pFormation ), pBuilding( _pBuilding )
{
	if ( ALP_POSITION_VALID == param )
		point = _point;
	else // choose point by random
		{ point = pBuilding->GetEntrancePoint( NRandom::Random( pBuilding->GetNEntrancePoints() ) ); RecordRandomCall(); }
}

void CFormationLeaveBuildingState::Segment()
{
	InitStatus();
	if ( pBuilding->GetLastLeaveTime( pFormation->GetPlayer() ) + 2000 < curTime )
	{
		pBuilding->SetLastLeaveTime( pFormation->GetPlayer() );
		
		int nEntrance;
		if ( pBuilding->ChooseEntrance( pFormation, point, &nEntrance ) )
		{
			for ( int i = 0; i < pFormation->Size(); ++i )
			{
				CSoldier *pSoldier = (*pFormation)[i];
				theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP ), pSoldier, false );
				if ( !pSoldier->IsFree() )
				{
					if ( pSoldier->IsInSolidPlace() )
					{
						pBuilding->GoOutFromEntrance( nEntrance, pSoldier );
						pSoldier->SetFree();
						pSoldier->SetCenter( GetHeights()->Get3DPoint( pBuilding->GetEntrancePoint( nEntrance ) ), false );
						updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, pSoldier, -1 );
					}
					else 
					{
						pBuilding->GoOutFromEntrance( nEntrance, pSoldier );
						pSoldier->SetFree();
						pSoldier->SetCenter( GetHeights()->Get3DPoint( pBuilding->GetEntrancePoint( nEntrance ) ), false );
					}
				}

				pSoldier->SetCommandFinished();
				pSoldier->FreezeByState( false );
			}

			pFormation->SetFree();
			pFormation->SetCenter( GetHeights()->Get3DPoint( pBuilding->GetEntrancePoint( nEntrance ) ) );
			
			CVec2 vRand;
			RandUniformlyInCircle( 1.5f * SConsts::TILE_SIZE, &vRand );
			theGroupLogic.PushFrontUnitCommand( SAIUnitCmd( ACTION_COMMAND_MOVE_TO, CVec2(point.x + vRand.x, point.y + vRand.y) ), pFormation );
		}

		pFormation->SetCommandFinished();
	}
}

ETryStateInterruptResult CFormationLeaveBuildingState::TryInterruptState( class CAICommand *pCommand )
{
	pFormation->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*									CFormationLeaveEntrenchmentState								*
//*******************************************************************

IUnitState* CFormationLeaveEntrenchmentState::Instance( CFormation *pFormation, CEntrenchment *pEntrenchment, const enum EActionLeaveParam param, const CVec2 &point )
{
	return new CFormationLeaveEntrenchmentState( pFormation, pEntrenchment, param, point );
}

CFormationLeaveEntrenchmentState::CFormationLeaveEntrenchmentState( CFormation *_pFormation, CEntrenchment *_pEntrenchment, const enum EActionLeaveParam param, const CVec2 &_point )
: CStatusUpdatesHelper( EUS_LEAVE, _pFormation ), pFormation( _pFormation ), pEntrenchment( _pEntrenchment )
{
	if ( ALP_POSITION_VALID == param )
		point = _point;
	else 
		point = pFormation->GetCenterPlain();
}

void CFormationLeaveEntrenchmentState::Segment()
{
	InitStatus();
	CVec2 startPoint;
	pEntrenchment->GetClosestPoint( point, &startPoint );

	// найти первую точку пути вне окопа
	CPtr<IStaticPath> pPath = CreateStaticPathToPoint( startPoint, point, VNULL2, pFormation, false, GetAIMap() );
	if ( pPath != 0 )
	{
		int nFirePlace = 0;
		const int nFirePlaces = pEntrenchment->GetNFirePlaces();
		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			CSoldier *pSoldier = (*pFormation)[i];
			theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP ), pSoldier, false );
			if ( !pSoldier->IsFree() )
			{
				if ( pSoldier->IsInSolidPlace() )
				{
					const CVec3 vPointToGo =
						nFirePlaces == 0 ? GetHeights()->Get3DPoint( pSoldier->GetCenterPlain() ) : 
															 GetHeights()->Get3DPoint( pEntrenchment->GetFirePlaceCoord( nFirePlace ) ) ;
					if ( nFirePlaces )
						nFirePlace = ( nFirePlace + 1 ) % nFirePlaces;

					pSoldier->SetCenter( vPointToGo, false );
				}
				else
					pSoldier->SetCenter( GetHeights()->Get3DPoint( pSoldier->GetCenterPlain() ) );

				pEntrenchment->DelInsider( pSoldier );				
				pSoldier->SetFree();
			}
		}

		pFormation->SetCenter( GetHeights()->Get3DPoint( startPoint ) );
		pFormation->SetFree();
		pFormation->TurnToDirection( -GetDirectionByVector( point - startPoint ), true, true );

		CVec2 vRand;
		RandUniformlyInCircle( 1.5f * SConsts::TILE_SIZE, &vRand );
		theGroupLogic.PushFrontUnitCommand( SAIUnitCmd( ACTION_COMMAND_MOVE_TO, point.x + vRand.x, point.y + vRand.y ), pFormation );
	}

	pFormation->SetCommandFinished();
}

ETryStateInterruptResult CFormationLeaveEntrenchmentState::TryInterruptState( class CAICommand *pCommand )
{
	pFormation->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*											CFormationAttackUnitState										*
//*******************************************************************

IUnitState* CFormationAttackUnitState::Instance( CFormation *pFormation, CAIUnit *pEnemy, const bool bSwarmAttack )
{
	return new CFormationAttackUnitState( pFormation, pEnemy, bSwarmAttack );
}

CFormationAttackUnitState::CFormationAttackUnitState( CFormation *_pFormation, CAIUnit *_pEnemy, const bool _bSwarmAttack )
: pFormation( _pFormation ), pEnemy( _pEnemy ), eState( EPM_MOVING ), bSwarmAttack( _bSwarmAttack ),
	nEnemyParty( _pEnemy->GetParty() )
{
/*	
	if ( bSwarmAttack )
	{
		for ( int i = 0; i < pFormation->Size(); ++i )
			(*pFormation)[i]->ResetTargetScan();
	}
*/
}

void CFormationAttackUnitState::SetToWaitingState()
{
	eState = EPM_WAITING;
	pFormation->StopFormationCenter();
	pFormation->SetToWaitingState();

	const int nSize = pFormation->Size();
	for ( int i = 0; i < nSize; ++i )
	{
		CSoldier *pSoldier = (*pFormation)[i];
		const bool bOfficer = pSoldier->GetStats()->etype == RPG_TYPE_OFFICER;
		const bool bShouldSendAttackCommand = 
			nSize == 1 || 
			!bOfficer ||
			bOfficer && fabs2( pSoldier->GetCenter() - pEnemy->GetCenter() ) < sqr( SConsts::OFFICER_COEFFICIENT_FOR_SCAN ) * sqr( pSoldier->GetGun( 0 )->GetFireRange( 0 ) );

		if ( bShouldSendAttackCommand )
		{
			SAIUnitCmd cmd( ACTION_COMMAND_ATTACK_UNIT, pEnemy->GetUniqueId(), float(bSwarmAttack) );
			if ( pFormation->GetCurCmd() && pFormation->GetCurCmd()->IsRefValid() && pFormation->Size() == 1 )
				cmd.bFromAI = pFormation->GetCurCmd()->ToUnitCmd().bFromAI;

			theGroupLogic.UnitCommand( cmd, (*pFormation)[i], false );
		}
	}
}

void CFormationAttackUnitState::SetToMovingState()
{
	if ( CPtr<IStaticPath> pStaticPath = CreateStaticPathForAttack( pFormation, pEnemy, 0, pFormation->GetMaxFireRange(), 0.0f, true ) )
	{
		eState = EPM_MOVING;
		StopAllUnits( pFormation );

		pFormation->SendAlongPath( pStaticPath, VNULL2, true );
	}
	else
	{
		pFormation->SendAcknowledgement( ACK_NEGATIVE_NOTIFICATION );
		TryInterruptState( 0 );
	}
}

void CFormationAttackUnitState::Segment()
{
	if ( !IsValidObj( pEnemy ) )
		TryInterruptState( 0 );
	else
	{
		switch ( eState )
		{
			case EPM_MOVING:
				if ( bSwarmAttack )
				{
					for ( int i = 0; i < pFormation->Size(); ++i )
						(*pFormation)[i]->AnalyzeTargetScan( pEnemy, false, false );
				}

				if ( IsFormationCloseToEnemy( pEnemy->GetCenterPlain(), pFormation ) )
					SetToWaitingState();
				else if ( pFormation->IsIdle() )
					SetToMovingState();

				break;
			case EPM_WAITING:
				if ( pFormation->IsEveryUnitResting() || !IsValidObj( pEnemy ) ||
						 pEnemy->GetParty() != nEnemyParty )
					TryInterruptState( 0 );

				break;
		}
	}
}

ETryStateInterruptResult CFormationAttackUnitState::TryInterruptState( class CAICommand *pCommand )
{
	pFormation->UnsetFromWaitingState();
	StopAllUnits( pFormation );
	pFormation->SetCommandFinished();

	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CFormationAttackUnitState::GetPurposePoint() const
{
	if ( IsValidObj( pEnemy ) )
		return pEnemy->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

EUnitStateNames CFormationAttackUnitState::GetName()
{
	if ( !IsValidObj( pEnemy ) || pEnemy->IsFree() )
		return EUSN_ATTACK_UNIT;
	else 
		return EUSN_ATTACK_UNIT_IN_BUILDING;
}
//*******************************************************************
//*										CFormationAttackCommonStatObjState						*
//*******************************************************************

IUnitState* CFormationAttackCommonStatObjState::Instance( CFormation *pFormation, CStaticObject *pObj )
{
	return new CFormationAttackCommonStatObjState( pFormation, pObj );
}

CFormationAttackCommonStatObjState::CFormationAttackCommonStatObjState( CFormation *_pFormation, CStaticObject *_pObj )
: pFormation( _pFormation ), pObj( _pObj ), eState( EPM_START )
{
}

void CFormationAttackCommonStatObjState::SetToWaitingState()
{
	eState = EPM_WAITING;
	pFormation->StopFormationCenter();
	pFormation->SetToWaitingState();

	for ( int i = 0; i < pFormation->Size(); ++i )
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_ATTACK_OBJECT, pObj->GetUniqueId() ), (*pFormation)[i], false );
}

void CFormationAttackCommonStatObjState::Segment()
{
	if ( !IsValidObj( pObj ) )
		TryInterruptState( 0 );
	else
	{
		switch ( eState )
		{
			case EPM_START:
				if ( IsFormationCloseToEnemy( pObj->GetAttackCenter( pFormation->GetCenterPlain() ), pFormation ) )
					SetToWaitingState();
				else if ( CPtr<IStaticPath> pPath = CreateStaticPathToPoint( pObj->GetAttackCenter( pFormation->GetCenterPlain() ), VNULL2, pFormation, true, GetAIMap() ) )
				{
					StopAllUnits( pFormation );
					pFormation->SendAlongPath( pPath, VNULL2, true );
					eState = EPM_MOVING;
				}
				else
				{
					pFormation->SendAcknowledgement( ACK_NEGATIVE_NOTIFICATION );
					TryInterruptState( 0 );
				}

			case EPM_MOVING:
				if ( IsFormationCloseToEnemy( pObj->GetAttackCenter( pFormation->GetCenterPlain() ), pFormation ) )
					SetToWaitingState();
				else if ( pFormation->IsIdle() )
					eState = EPM_START;

				break;
			case EPM_WAITING:
				if ( pFormation->IsEveryUnitResting() || !IsValidObj( pObj ) )
					TryInterruptState( 0 );

				break;
		}
	}
}

ETryStateInterruptResult CFormationAttackCommonStatObjState::TryInterruptState( class CAICommand *pCommand )
{
	pFormation->UnsetFromWaitingState();
	StopAllUnits( pFormation );
	pFormation->SetCommandFinished();

	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CFormationAttackCommonStatObjState::GetPurposePoint() const
{
	if ( IsValidObj( pObj ) && pFormation && pFormation->IsRefValid() && pFormation->IsAlive() )
		return pObj->GetAttackCenter( pFormation->GetCenterPlain() );
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										CFormationRotateState													*
//*******************************************************************

IUnitState* CFormationRotateState::Instance( CFormation *pFormation, const WORD wDir )
{
	return new CFormationRotateState( pFormation, wDir );
}

CFormationRotateState::CFormationRotateState( CFormation *_pFormation, const WORD wDir )
: pFormation( _pFormation )
{
	// повернуть формацию
	pFormation->TurnToDirection( wDir, false, true );

	// сказать каждому юниту формации построиться
	const SAIUnitCmd paradeCmd( ACTION_COMMAND_PARADE );
	for ( int i = 0; i < pFormation->Size(); ++i )
		theGroupLogic.UnitCommand( paradeCmd, (*pFormation)[i], false );
}

void CFormationRotateState::Segment()
{
	int i = 0;
	while ( i < pFormation->Size() && (*pFormation)[i]->GetState()->GetName() == EUSN_REST )
		++i;

	if ( i == pFormation->Size() )
		pFormation->SetCommandFinished();
}

ETryStateInterruptResult CFormationRotateState::TryInterruptState( class CAICommand *pCommand )
{
	pFormation->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CFormationRotateState::GetPurposePoint() const
{
	return pFormation->GetCenterPlain();
}

//*******************************************************************
//*									CFormationEnterTransportState										*
//*******************************************************************

IUnitState* CFormationEnterTransportState::Instance( CFormation *pFormation, CMilitaryCar *pTransport )
{
	return new CFormationEnterTransportState( pFormation, pTransport );
}

CFormationEnterTransportState::CFormationEnterTransportState( CFormation *_pFormation, CMilitaryCar *_pTransport )
: CStatusUpdatesHelper( EUS_LOAD, _pFormation ), pFormation( _pFormation ), pTransport( _pTransport ), eState( EETS_START ), lastCheck( curTime ), 
	lastTransportPos( _pTransport->GetCenterPlain() ), lastTransportDir( _pTransport->GetFrontDirection() )
{
}

bool CFormationEnterTransportState::SetPathToRunUp()
{
	lastTransportPos = pTransport->GetCenterPlain();
	CPtr<IStaticPath> pPath = CreateStaticPathToPoint( pTransport->GetEntrancePoint(), VNULL2, pFormation, true, GetAIMap() );
	if ( pPath != 0 )
	{
		pFormation->SendAlongPath( pPath, VNULL2, true );
		return true;
	}
	else
		return false;
}	

void CFormationEnterTransportState::SendUnitsToTransport()
{
	pTransport->Stop();
	pFormation->Stop();
	for ( int i = 0; i < pFormation->Size(); ++i )
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_ENTER_TRANSPORT_NOW, pTransport->GetUniqueId() ), (*pFormation)[i], false );
}

bool CFormationEnterTransportState::IsAllUnitsInside()
{
	int i = 0;
	while ( i < pFormation->Size() && (*pFormation)[i]->IsInTransport() )
		++i;

	return i >= pFormation->Size();
}

void CFormationEnterTransportState::SetTransportToWaitState()
{
	if ( pTransport->GetState()->GetName() == EUSN_WAIT_FOR_PASSENGER )
		checked_cast<CTransportWaitPassengerState*>(pTransport->GetState())->AddFormationToWait( pFormation );
	else
	{
		SAIUnitCmd cmd( ACTION_COMMAND_WAIT_FOR_UNITS, pFormation->GetUniqueId()  );
		theGroupLogic.UnitCommand( cmd, pTransport, false );
	}
}

bool CFormationEnterTransportState::IsAllTransportTurretsReturned() const
{
	for ( int i = 0; i < pTransport->GetNTurrets(); ++i )
	{
		if ( pTransport->GetTurret( i )->GetHorCurAngle() != 0 )
			return false;
	}

	return true;
}

void CFormationEnterTransportState::Segment()
{
	if ( !IsValidObj( pTransport ) || eState != EETS_WAITING && pTransport->GetNAvailableSeats() < pFormation->Size() )
	{
		pFormation->SendAcknowledgement( 0, ACK_NEGATIVE, true );
		pFormation->SetCommandFinished();
	}
	else if ( IsAlly( pTransport->GetPlayer(), pFormation->GetPlayer() ) )
	{
		pFormation->SendAcknowledgement( ACK_NEGATIVE );
		TryInterruptState( 0 );
	}
	else 
	{
		switch ( eState )
		{
			case EETS_START:
				if ( SetPathToRunUp() )
				{
					eState = EETS_MOVING;
					InitStatus();
				}
				else
				{
					pFormation->SendAcknowledgement( ACK_NEGATIVE );
					pFormation->SetCommandFinished();
				}
				break;
			case EETS_MOVING:
				{
					const bool bTransportMoved = curTime - lastCheck >= CHECK_PERIOD && fabs( pTransport->GetCenterPlain() - lastTransportPos ) >= SConsts::TILE_SIZE;
					const float fAddLength = ( pFormation->Size() == 1 ) ? ( 4 * SConsts::TILE_SIZE ) : ( SConsts::TILE_SIZE + pFormation->GetRadius() );
					const bool bCloseToEntrance = fabs( pFormation->GetCenterPlain() - pTransport->GetEntrancePoint() ) < fAddLength;

					if ( bCloseToEntrance || pFormation->IsIdle() || bTransportMoved || lastTransportDir != pTransport->GetFrontDirection() )
					{
						if ( bCloseToEntrance )
							eState = EETS_WAIT_TO_UNLOCK_TRANSPORT;
						else if ( pFormation->IsIdle() || bTransportMoved || lastTransportDir != pTransport->GetFrontDirection() )
							eState = EETS_START;

						lastCheck = curTime;
						lastTransportPos = pTransport->GetCenterPlain();
						lastTransportDir = pTransport->GetFrontDirection();
					}
				}

				break;
			case EETS_WAIT_TO_UNLOCK_TRANSPORT:
				if ( !pTransport->IsLocked() )
				{
					SetTransportToWaitState();
					pTransport->Lock( pFormation );
					eState = EETS_WAIT_FOR_TURRETS_RETURN;
				}

				break;
			case EETS_WAIT_FOR_TURRETS_RETURN:
				if ( IsAllTransportTurretsReturned() )
				{
					SendUnitsToTransport();
					eState = EETS_WAITING;
				}

				break;
			case EETS_WAITING:
				if ( IsAllUnitsInside() )
				{
					eState = EETS_FINISHED;
					theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRANSPORT, pTransport->GetUniqueId() ), pFormation, false );
					pTransport->Unlock();
				}

				break;
		}
	}
}

ETryStateInterruptResult CFormationEnterTransportState::TryInterruptState( class CAICommand *pCommand )
{
	if ( pCommand && pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_LOAD &&
			 pTransport->GetUniqueId() == pCommand->ToUnitCmd().nObjectID )
		return TSIR_NO_COMMAND_INCOMPATIBLE;

	if ( eState != EETS_WAITING && eState != EETS_WAIT_FOR_TURRETS_RETURN )
	{
		pFormation->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	else
	{
		int i = 0;
		while ( i < pFormation->Size() && !(*pFormation)[i]->IsInTransport() )
			++i;

		if ( i < pFormation->Size() )
			return TSIR_YES_WAIT;
		else
		{
			for ( int i = 0; i < pFormation->Size(); ++i )
				theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP ), (*pFormation)[i], false );

			pTransport->Unlock();
			pFormation->SetCommandFinished();

			return TSIR_YES_IMMIDIATELY;
		}
	}
}

const CVec2 CFormationEnterTransportState::GetPurposePoint() const
{
	if ( IsValidObj( pTransport ) )
		return pTransport->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*									CFormationIdleTransportState										*
//*******************************************************************

IUnitState* CFormationIdleTransportState::Instance( CFormation *pFormation, CMilitaryCar *pTransport )
{
	return new CFormationIdleTransportState( pFormation, pTransport );
}

CFormationIdleTransportState::CFormationIdleTransportState( CFormation *_pFormation, CMilitaryCar *_pTransport )
: pFormation( _pFormation ), pTransport( _pTransport )
{
	pFormation->SetInTransport( pTransport );
}

void CFormationIdleTransportState::Segment()
{
}

ETryStateInterruptResult CFormationIdleTransportState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand ||
			pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_MOVE_TO  || 
			pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_REPAIR_UNIT ||
			pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_RESUPPLY_UNIT || 
			pCommand->ToUnitCmd().nCmdType == ACTION_MOVE_PARACHUTE )
	{
		pFormation->SetCommandFinished();
		pFormation->SetFree();
		return TSIR_YES_IMMIDIATELY;
	}

	return TSIR_NO_COMMAND_INCOMPATIBLE;
}

const CVec2 CFormationIdleTransportState::GetPurposePoint() const
{
	if ( IsValidObj( pTransport ) )
		return pTransport->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*									CFormationEnterTransportByCheatPathState*
//*******************************************************************

IUnitState* CFormationEnterTransportByCheatPathState::Instance( class CFormation *_pFormation, class CMilitaryCar *_pTransport )
{
	return new CFormationEnterTransportByCheatPathState( _pFormation, _pTransport );
}

CFormationEnterTransportByCheatPathState::CFormationEnterTransportByCheatPathState( class CFormation *_pFormation, class CMilitaryCar *_pTransport )
: pFormation( _pFormation ), pTransport( _pTransport )
{
	// send all squad soldiers to transport via cheat path
	const int nSoldiers = pFormation->Size();
	for ( int i = 0; i < nSoldiers; ++i )
	{
		CSoldier *pSoldier = (*pFormation)[i];
		//pSoldier->AddRef()
		pSoldier->SetSmoothPath( new CArtilleryCrewPath( pSoldier, pSoldier->GetCenterPlain(), pTransport->GetEntrancePoint(), pSoldier->GetMaxPossibleSpeed() ) );
	}
}

void CFormationEnterTransportByCheatPathState::Segment()
{
	// wait while all is near transport,
	// set them inside and set squad inside.
	const int nSoldiers = pFormation->Size();
	bool bAllInTransport = true;

	for ( int i = 0; i < nSoldiers; ++i )
	{
		CSoldier *pSoldier = (*pFormation)[i];
		if ( pSoldier->GetSmoothPath()->IsFinished() && !pSoldier->IsInTransport() )
		{
			pSoldier->RestoreSmoothPath();
			pSoldier->SetInTransport( pTransport );
			pTransport->AddPassenger( pSoldier );
			theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRANSPORT, pTransport->GetUniqueId() ), pSoldier, false );
		}
		if ( !pSoldier->IsInTransport() )
		{
			bAllInTransport = false;
		}
	}

	if ( bAllInTransport )
	{
		pFormation->SetCommandFinished();
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRANSPORT, pTransport->GetUniqueId() ), pFormation, false );
	}
}

ETryStateInterruptResult CFormationEnterTransportByCheatPathState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand )
	{
		pFormation->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	else
		return TSIR_YES_WAIT;
}

const CVec2 CFormationEnterTransportByCheatPathState::GetPurposePoint() const 
{ 
	return pTransport->GetEntrancePoint(); 
}

//*******************************************************************
//*									CFormationEnterTransportNowState								*
//*******************************************************************

IUnitState* CFormationEnterTransportNowState::Instance( CFormation *pFormation, CMilitaryCar *pTransport )
{
	return new CFormationEnterTransportNowState( pFormation, pTransport );
}

CFormationEnterTransportNowState::CFormationEnterTransportNowState( CFormation *_pFormation, CMilitaryCar *_pTransport )
: pFormation( _pFormation ), pTransport( _pTransport )
{
	for ( int i = 0; i < pFormation->Size(); ++i )
	{
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRANSPORT, pTransport->GetUniqueId() ), (*pFormation)[i], false );
		(*pFormation)[i]->SetInTransport( pTransport );
		pTransport->AddPassenger( (*pFormation)[i] );
	}
	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRANSPORT, pTransport->GetUniqueId() ), pFormation, false );
}

void CFormationEnterTransportNowState::Segment()
{

	pFormation->SetCommandFinished();
}

ETryStateInterruptResult CFormationEnterTransportNowState::TryInterruptState( class CAICommand *pCommand )
{
	return TSIR_YES_WAIT;
}

const CVec2 CFormationEnterTransportNowState::GetPurposePoint() const
{
	if ( IsValidObj( pTransport ) )
		return pTransport->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										CFormationCatchTransportState									*
//*******************************************************************

IUnitState* CFormationCatchTransportState::Instance( CFormation *pUnit, CAITransportUnit *pTransportToCatch, float fResursPerSoldier)
{
	return new CFormationCatchTransportState( pUnit, pTransportToCatch, fResursPerSoldier );
}

CFormationCatchTransportState::CFormationCatchTransportState( class CFormation *_pUnit, class CAITransportUnit *_pTransportToCatch, float fResursPerSoldier )
: pUnit ( _pUnit ), pTransportToCatch( _pTransportToCatch ), vEnterPoint( -1, -1 ),
	fResursPerSoldier(  fResursPerSoldier ), timeLastUpdate( curTime )
{
	if ( !pTransportToCatch || !IsValidObj( pTransportToCatch ) )
	{
		pUnit->SendAcknowledgement( ACK_NEGATIVE, true );
		pUnit->SetCommandFinished();
	}
	else
	{
		pUnit->Stop();
		for ( int i = 0; i < pUnit->Size(); ++i )
		{
			NI_ASSERT( dynamic_cast<CSoldier*>((*pUnit)[i])!=0, "not soldier attempting to catch transport");
			CSoldier * pSold = checked_cast<CSoldier*>((*pUnit)[i]);
			pSold->RestoreSmoothPath();
			pSold->Stop();
			theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_IDLE), pSold, false );
		}
			
		eState = E_SENDING;
	}
}

void CFormationCatchTransportState::Segment()
{
	if ( !IsValidObj( pTransportToCatch ) )
		Interrupt();
	else 
	if ( curTime > timeLastUpdate + 200 )
	{
		timeLastUpdate = curTime ;

		switch( eState )
		{
		case E_SENDING:
			for ( int i = 0; i < pUnit->Size(); ++i )
			{
				NI_ASSERT( dynamic_cast<CSoldier*>((*pUnit)[i])!=0, "not soldier attempting to catch transport");
				CSoldier * pSold = checked_cast<CSoldier*>((*pUnit)[i]);
				UpdatePath( pSold, true );
			}
			
			eState = E_CHECKING;
			break;
		case E_CHECKING:
			{
				int nNotInTransport = 0;//number of solders, that didn't catch transport yet.
				SRect transpRect = pTransportToCatch->GetUnitRect();
				for ( int i = 0; i < pUnit->Size(); ++i )
				{
					NI_ASSERT( dynamic_cast<CSoldier*>((*pUnit)[i]) != 0, "not soldier attempting to catch transport");
					CSoldier *pSold = checked_cast<CSoldier*>( (*pUnit)[i] );
					if ( pSold && pSold->IsAlive() )
					{
						++nNotInTransport;
						if ( IsUnitNearPoint( pSold, pTransportToCatch->GetEntrancePoint(), SConsts::TILE_SIZE ) ||
								 IsUnitNearUnit( pSold, pTransportToCatch ) )
							deleted.push_back( pSold );
						else if ( pSold->IsIdle() )
						{
							if ( IsUnitNearPoint( pSold, pTransportToCatch->GetEntrancePoint(), SConsts::TILE_SIZE * 5 ) )
								deleted.push_back( pSold );
							else
								UpdatePath( pSold, pSold->IsIdle() );
						}
					}
				}
				while ( !deleted.empty() )
				{
					updater.ClearUpdates( *deleted.begin() );
					(*deleted.begin())->Disappear();
					deleted.pop_front();

					float fRes = pTransportToCatch->GetResursUnitsLeft() + fResursPerSoldier;
					if ( fRes < SConsts::TRANSPORT_RU_CAPACITY )
						pTransportToCatch->SetResursUnitsLeft( fRes );
					else
						pTransportToCatch->SetResursUnitsLeft( SConsts::TRANSPORT_RU_CAPACITY );
				}
				
				if ( 0 != nNotInTransport ) //не все добежали
					vEnterPoint = pTransportToCatch->GetEntrancePoint();
			}

			break;
		}
		vEnterPoint = pTransportToCatch->GetEntrancePoint();
	}
}

void CFormationCatchTransportState::Interrupt()
{
	if ( pUnit->IsIdle() )
		pUnit->Stop();
	pUnit->SetCommandFinished();
}

ETryStateInterruptResult CFormationCatchTransportState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand )
	{
		pUnit->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	else
		return TSIR_NO_COMMAND_INCOMPATIBLE;
}

void CFormationCatchTransportState::UpdatePath( CSoldier * pSold, const bool bForce )
{
	const CVec2 vPt( pTransportToCatch->GetEntrancePoint() );
	if ( /*vEnterPoint != vPt || */bForce )
	{
		CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( vPt, VNULL2, pSold, true, GetAIMap() );
		if ( pStaticPath )
			pSold->SendAlongPath( pStaticPath, VNULL2, true );
	}
}

const CVec2 CFormationCatchTransportState::GetPurposePoint() const
{
	if ( IsValidObj( pTransportToCatch ) )
		return pTransportToCatch->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										CFormationParaDropState												*
//*******************************************************************

IUnitState* CFormationParaDropState::Instance( CFormation *pFormation )
{
	return new CFormationParaDropState( pFormation );
}

CFormationParaDropState::CFormationParaDropState( class CFormation *pFormation )
: pFormation( pFormation ), eState( EPS_WAIT_FOR_PARADROP_BEGIN )
{
	pFormation->SetSelectable( false, true );
}

void CFormationParaDropState::Segment()
{
	switch( eState )
	{
	case EPS_WAIT_FOR_PARADROP_BEGIN:
		for ( int i = 0; i < pFormation->Size(); ++i )
		{ 
			IUnitState  *pState = (*pFormation)[i]->GetState();
			if ( EUSN_PARTROOP == pState->GetName() )
			{
				//кто-то уже выпрыгнул
				eState = EPS_WAIT_FOR_PARADROP_END;
				return;
			}
		}

		break;
	case EPS_WAIT_FOR_PARADROP_END:
		{
			for ( int i = 0; i < pFormation->Size(); ++i )
			{
				IUnitState  *pState = (*pFormation)[i]->GetState();
				if ( EUSN_PARTROOP == pState->GetName() )
				{
					//кто-то еще не долетел
					if ( !static_cast<CSoldierParaDroppingState*>( pState )->IsLanded() )
						return;
				}
			}
			for ( int i = 0; i < pFormation->Size(); ++i )
			{
				IUnitState  *pState = (*pFormation)[i]->GetState();
				pState->TryInterruptState( 0 );
			}
			CSoldier * pCenter = (*pFormation)[pFormation->Size()/2];
			pFormation->SetCenter( pCenter->GetCenter() );
			pFormation->SetCommandFinished();
			pFormation->SetSelectable( pFormation->GetPlayer() == theDipl.GetMyNumber(), true );
		}
	}
}

ETryStateInterruptResult CFormationParaDropState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand )
	{
		pFormation->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	else if ( pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_PARADE )
		return TSIR_YES_WAIT;
	else
		return TSIR_NO_COMMAND_INCOMPATIBLE;
}

const CVec2 CFormationParaDropState::GetPurposePoint() const
{
	return pFormation->GetCenterPlain();
}

//*******************************************************************
//*										CFormationAttackFormationState								*
//*******************************************************************

IUnitState* CFormationAttackFormationState::Instance( CFormation *pFormation, CFormation *pTarget, const bool bSwarmAttack )
{
	return new CFormationAttackFormationState( pFormation, pTarget, bSwarmAttack );
}

CFormationAttackFormationState::CFormationAttackFormationState( CFormation *pFormation, CFormation *_pTarget, const bool _bSwarmAttack )
: pUnit( pFormation ), pTarget( _pTarget ), bSwarmAttack( _bSwarmAttack ), nEnemyParty( _pTarget->GetParty() )
{
}

void CFormationAttackFormationState::Segment()
{
	if ( !IsValidObj( pTarget ) || pTarget->Size() == 0 || pTarget->GetParty() != nEnemyParty )
	{
	}
	else
	{
		if ( bSwarmAttack )
		{
			theGroupLogic.PushFrontUnitCommand( SAIUnitCmd( ACTION_MOVE_SWARM_ATTACK_FORMATION, pTarget->GetUniqueId()), pUnit );
			theGroupLogic.PushFrontUnitCommand( SAIUnitCmd( ACTION_COMMAND_SWARM_ATTACK_UNIT, (*pTarget)[0]->GetUniqueId()), pUnit );
		}
		else
		{
			theGroupLogic.PushFrontUnitCommand( SAIUnitCmd( ACTION_MOVE_ATTACK_FORMATION, pTarget->GetUniqueId()), pUnit );
			theGroupLogic.PushFrontUnitCommand( SAIUnitCmd( ACTION_COMMAND_ATTACK_UNIT, (*pTarget)[0]->GetUniqueId()), pUnit );
		}
	}

	TryInterruptState( 0 ) ;
}

ETryStateInterruptResult CFormationAttackFormationState::TryInterruptState( class CAICommand *pCommand )
{
	pUnit->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CFormationAttackFormationState::GetPurposePoint() const
{
	if ( IsValidObj( pTarget ) )
		return pTarget->GetCenterPlain();
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										CFormationParadeState													*
//*******************************************************************

IUnitState* CFormationParadeState::Instance( CFormation *pFormation, const int nType )
{
	return new CFormationParadeState( pFormation, nType );
}

CFormationParadeState::CFormationParadeState( CFormation *_pFormation, const int nType )
: pFormation( _pFormation ), startTime( curTime )
{
	if ( nType != -1 )
	{
		int nGeometry = 0;
		while ( nGeometry < pFormation->GetStats()->formations.size() &&
						pFormation->GetStats()->formations[nGeometry].etype != nType )
			++nGeometry;

		if ( nGeometry < pFormation->GetStats()->formations.size() &&
				 nGeometry < pFormation->GetGeometriesCount() )
		{
			pFormation->ChangeGeometry( nGeometry );

			for ( int i = 0; i < pFormation->Size(); ++i )
			{
				CSoldier *pSold = (*pFormation)[i];
				pSold->FreezeByState( false );
				pSold->Stopped();

				if ( pSold->GetState()->GetName() == EUSN_REST )
					checked_cast<CSoldierRestState*>(pSold->GetState())->SetNullLastMoveTime();
			}
		}
	}
}

void CFormationParadeState::Segment()
{
	if ( startTime + 1000 < curTime && pFormation->IsEveryUnitResting() )
		pFormation->SetCommandFinished();
}

ETryStateInterruptResult CFormationParadeState::TryInterruptState( class CAICommand *pCommand )
{
	pFormation->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CFormationParadeState::GetPurposePoint() const
{
	return pFormation->GetCenterPlain();
}

//*******************************************************************
//*										CFormationDisbandState												*
//*******************************************************************

IUnitState* CFormationDisbandState::Instance( CFormation *pFormation )
{
	return new CFormationDisbandState( pFormation );
}

CFormationDisbandState::CFormationDisbandState( CFormation *_pFormation )
: pFormation( _pFormation )
{
}

void CFormationDisbandState::Segment()
{
	const int nGroup = pFormation->GetNGroup();
//	CObjectBase **pUnitsBuffer = 0;
	if ( nGroup > 0 )
		theGroupLogic.DelUnitFromGroup( pFormation );
	
	pFormation->Stop();
	for ( int i = 0; i < pFormation->Size(); ++i )
	{
		CSoldier *pSoldier = (*pFormation)[i];
		pSoldier->MemCurFormation();
		CCommonUnit *pSingleFormation = theUnitCreation.CreateSingleUnitFormation( pSoldier );

		if ( pSoldier->IsFree() )
			theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_GUARD, CVec2( -1.0f, -1.0f ), 0 ), pSingleFormation, false );
		else if ( pSoldier->IsInTransport() )
			theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRANSPORT, pSoldier->GetTransportUnit()->GetUniqueId() ), pSingleFormation, false );
		else if ( pSoldier->IsInBuilding() )
			theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_BUILDING, pSoldier->GetBuilding()->GetUniqueId() ), pSingleFormation, false );
		else if ( pSoldier->IsInEntrenchment() )
			theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRENCH, pSoldier->GetEntrenchment()->GetUniqueId() ), pSingleFormation, false );

		if ( nGroup > 0 )
			theGroupLogic.AddUnitToGroup( pSingleFormation, nGroup );
	}

	pFormation->Disable();
	pFormation->SetCommandFinished();
}

ETryStateInterruptResult CFormationDisbandState::TryInterruptState( CAICommand *pCommand )
{
	return TSIR_NO_COMMAND_INCOMPATIBLE;
}

const CVec2 CFormationDisbandState::GetPurposePoint() const
{
	return pFormation->GetCenterPlain();
}

//*******************************************************************
//*										CFormationFormState														*
//*******************************************************************

IUnitState* CFormationFormState::Instance( CFormation *pFormation )
{
	return new CFormationFormState( pFormation );
}

CFormationFormState::CFormationFormState( CFormation *_pFormation )
: pFormation( _pFormation )
{
}

void CFormationFormState::Segment()
{
	CSoldier *pMainSoldier = (*pFormation)[0];
	NI_ASSERT( pMainSoldier->IsFree() || pMainSoldier->IsInEntrenchment() || pMainSoldier->IsInBuilding(), "Wrong soldier state for command FORM_FORMATION" );

	CFormation *pMemFormation = pMainSoldier->GetMemFormation();
	
	int i = 0;
	while ( i < pMemFormation->Size() && !(*pMemFormation)[i]->IsInBuilding() )
		++i;

	if ( i < pMemFormation->Size() )
	{
		for ( int j = 0; j < pMemFormation->Size(); ++j )
			(*pMemFormation)[j]->GetFormation()->SendAcknowledgement( ACK_NEGATIVE );

		pFormation->SetCommandFinished();
	}
	else
	{
		pMemFormation->SetCenter( pMainSoldier->GetCenter() );

		// сформировать команду
		SAIUnitCmd cmd;
		if ( pMainSoldier->IsFree() )
			cmd.nCmdType = ACTION_COMMAND_MOVE_TO;
		else
		{
			cmd.nCmdType = ACTION_COMMAND_ENTER;
			if ( pMainSoldier->IsInEntrenchment() )
			{
				cmd.nObjectID = pMainSoldier->GetEntrenchment()->GetUniqueId();
				cmd.fNumber = 1;
			}
			else
			{
				cmd.nObjectID	= pMainSoldier->GetBuilding()->GetUniqueId();
				cmd.fNumber = 0;
			}
		}

		// раздать команду всем
		SAIUnitCmd waitCmd( ACTION_COMMAND_WAIT_TO_FORM, pMainSoldier->GetUniqueId(), 0.0f );
		for ( int i = 0; i < pMemFormation->Size(); ++i )
		{
			CSoldier *pSoldier = (*pMemFormation)[i];
			if ( pSoldier != pMainSoldier && pSoldier->GetFormation()->GetState()->GetName() != EUSN_WAIT_TO_FORM )
			{
				if ( cmd.nCmdType == ACTION_COMMAND_MOVE_TO )
					cmd.vPos = pMemFormation->GetUnitCoord( pSoldier );

				theGroupLogic.UnitCommand( cmd, pSoldier->GetFormation(), false );
				theGroupLogic.UnitCommand( waitCmd, pSoldier->GetFormation(), true );
				pSoldier->SetWait2FormFlag( true );
			}
		}

		waitCmd.fNumber = 1;
		theGroupLogic.PushFrontUnitCommand( waitCmd, pFormation );
		pMainSoldier->SetWait2FormFlag( true );

		pFormation->SetCommandFinished();
	}
}

ETryStateInterruptResult CFormationFormState::TryInterruptState( CAICommand *pCommand )
{
	return TSIR_YES_WAIT;
}

const CVec2 CFormationFormState::GetPurposePoint() const
{
	return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										CFormationWaitToFormState											*
//*******************************************************************

IUnitState* CFormationWaitToFormState::Instance( CFormation *pFormation, const float fMain, class CSoldier *pMainSoldier )
{
	return new CFormationWaitToFormState( pFormation, fMain, pMainSoldier );
}

CFormationWaitToFormState::CFormationWaitToFormState( CFormation *_pFormation, const float fMain, class CSoldier *_pMainSoldier )
: pFormation( _pFormation ), bMain( fMain == 1.0f ), pMainSoldier( _pMainSoldier )
{
	pFormFormation = pMainSoldier->GetMemFormation();
	startTime = curTime;
}

void CFormationWaitToFormState::FinishState()
{
	if ( IsValidObj( pFormFormation ) && pFormFormation->Size() > 0 )
	{
		for ( int i = 0; i < pFormFormation->Size(); ++i )
		{
			CSoldier *pSoldier = (*pFormFormation)[i];
			if ( pSoldier != pMainSoldier && pSoldier->IsInWait2Form() )
				theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP ), pSoldier->GetFormation(), false );
		}
	}
}

void CFormationWaitToFormState::FormFormation()
{
	CVec2 vCenter( VNULL2 );
	int nGroupID = 0;
	for ( int i = 0; i < pFormFormation->Size(); ++i )
	{
		CSoldier *pSoldier = (*pFormFormation)[i];
		CFormation *pSingleFormation = pSoldier->GetFormation();
		if ( pSingleFormation->GetNGroup() != 0 )
		{
			nGroupID = pSingleFormation->GetNGroup();
			theGroupLogic.DelUnitFromGroup( pSingleFormation );
		}
		
		pSingleFormation->DeleteSoldier( pSoldier );
		if ( pSoldier != pMainSoldier )
			pSingleFormation->Disappear();

		pSoldier->MemorizeFormation();
		pSoldier->SetFormation( pFormFormation );
		updater.AddUpdate( 0, ACTION_NOTIFY_NEW_FORMATION, pSoldier, -1 );

		vCenter += pSoldier->GetCenterPlain();
	}

	vCenter /= float( pFormFormation->Size() );
	
	pFormFormation->KillStatesAndCmdsInfo();
	pFormFormation->Enable();

	if ( nGroupID != 0 )
		theGroupLogic.AddUnitToGroup( pFormFormation, nGroupID );

	if ( pMainSoldier->IsFree() )
		pFormFormation->SetCurState( CFormationRestState::Instance( pFormFormation, CVec2( -1.0f, -1.0f ), 0, -1 ) );
	else if ( pMainSoldier->IsInBuilding() )
		pFormFormation->SetCurState( CFormationIdleBuildingState::Instance( pFormFormation, pMainSoldier->GetBuilding() ) );
	else if ( pMainSoldier->IsInEntrenchment() )
		pFormFormation->SetCurState( CFormationIdleEntrenchmentState::Instance( pFormFormation, pMainSoldier->GetEntrenchment() ) );
	else
		NI_ASSERT( false, "Wrong main soldier state" );
}

void CFormationWaitToFormState::Segment()
{
	// проверять не сразу же, а когда команда дойдёт до всех солдат формации
	if ( curTime - startTime > 1000 )
	{
		if ( bMain )
		{
			bool bCanForm = true;
			for ( int i = 0; i < pFormFormation->Size(); ++i )
			{
				// команда на формирование прервана
				if ( !(*pFormFormation)[i]->IsInWait2Form() )
				{
					pFormation->SetCommandFinished();
					return;
				}

				bCanForm &= ( (*pFormFormation)[i]->GetFormation()->GetState()->GetName() == EUSN_WAIT_TO_FORM );
			}

			if ( bCanForm )
			{
				FormFormation();
				pFormation->Disappear();
			}
		}
		else if ( !pMainSoldier->IsInWait2Form() )
			pFormation->SetCommandFinished();
	}
}

ETryStateInterruptResult CFormationWaitToFormState::TryInterruptState( CAICommand *pCommand )
{
	if ( pCommand && pCommand->ToUnitCmd().nCmdType == ACTION_COMMAND_WAIT_TO_FORM )
		return TSIR_NO_COMMAND_INCOMPATIBLE;
	else
	{
		if ( bMain )	
			FinishState();

		if ( pFormation && pFormation->IsAlive() && pFormation->Size() > 0 )
		{
			if ( IsValidObj( pMainSoldier ) )
				pMainSoldier->SetWait2FormFlag( false );
		}

		pFormation->SetCommandFinished();

		return TSIR_YES_IMMIDIATELY;
	}
}

const CVec2 CFormationWaitToFormState::GetPurposePoint() const
{
	return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*											CCatchFormationState												*
//*******************************************************************

IUnitState* CCatchFormationState::Instance( CFormation *pCatchingFormation, CFormation *pFormation )
{
	return new CCatchFormationState( pCatchingFormation, pFormation );
}

CCatchFormationState::CCatchFormationState( CFormation *_pCatchingFormation, CFormation *_pFormation )
: pCatchingFormation( _pCatchingFormation ), pFormation( _pFormation ), lastUpdateTime( 0 ), eState( ECFS_NONE ), 
	lastFormationPos( _pFormation->GetCenterPlain() ), pLastFormationObject( 0 )
{
}

void CCatchFormationState::MoveSoldierToFormation()
{
	pCatchingFormation->Stop();
	CSoldier *pSoldier = (*pCatchingFormation)[0];
	
	pCatchingFormation->DeleteSoldier( pSoldier );

	pFormation->MakeVirtualUnitReal( pSoldier );
	pSoldier->SetSelectable( pFormation->IsSelectable(), true );
}

void CCatchFormationState::JoinToFormation()
{
	CSoldier *pSoldier = (*pCatchingFormation)[0];
	
	MoveSoldierToFormation();
	pSoldier->SendAlongPath( 0, VNULL2, true );
	pCatchingFormation->Disappear();
}

void CCatchFormationState::LeaveCurStaticObject()
{
	const CVec2 vFormationCenter = pFormation->GetCenterPlain();
	CSoldier *pSoldier = (*pCatchingFormation)[0];
	
	CVec2 vLeavePoint( -1.0f, -1.0f );
	if ( pSoldier->IsInEntrenchment() )
		vLeavePoint = pSoldier->GetCenterPlain();
	else if ( pSoldier->IsInBuilding() )
	{
		CBuilding *pBuilding = pSoldier->GetBuilding();			
		int nEntrance;
		if ( pBuilding->ChooseEntrance( pFormation, vFormationCenter, &nEntrance ) )
			vLeavePoint = pSoldier->GetBuilding()->GetEntrancePoint( nEntrance );
	}
	else
		NI_ASSERT( false, "Wrong soldier state" );

	if ( vLeavePoint.x != -1.0f )
		theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_LEAVE, vLeavePoint ), pCatchingFormation );
}

void CCatchFormationState::AnalyzeFreeFormation()
{
	const CVec2 vFormationCenter = pFormation->GetCenterPlain();
	CSoldier *pSoldier = (*pCatchingFormation)[0];
	
	// если нужно откуда-то выбегать
	if ( !pSoldier->IsFree() )
		LeaveCurStaticObject();
	// просто бежать к формации
	else 
	{
		// добежали или формация сдвинулась
		if ( pCatchingFormation->IsIdle() || fabs2( vFormationCenter - lastFormationPos ) >= sqr( float( 2 * SConsts::TILE_SIZE ) ) )
		{
			lastFormationPos = vFormationCenter;
			const float fDist2 = fabs2( vFormationCenter - pCatchingFormation->GetCenterPlain() );
			
			// близко
			if ( fDist2 <= sqr( float( 8 * SConsts::TILE_SIZE ) ) )
			{
				if ( (*pFormation)[0]->CanJoinToFormation() )
					JoinToFormation();
			}
			else
			{
				CPtr<IStaticPath> pPath = CreateStaticPathToPoint( lastFormationPos, VNULL2, pCatchingFormation, true, GetAIMap() );
				if ( IsValid( pPath ) )
					pCatchingFormation->SendAlongPath( pPath, VNULL2, true );
			}
		}
	}
}

void CCatchFormationState::AnalyzeFormationInBuilding( CBuilding *pBuilding )
{
	CSoldier *pSoldier = (*pCatchingFormation)[0];

	// внутри объекта, но не того, который нужно
	if ( !pSoldier->IsFree() && ( !pSoldier->IsInBuilding() || pSoldier->GetBuilding() != pBuilding ) )
		LeaveCurStaticObject();
	// не внутри объекта
	else if ( pSoldier->IsFree() )
	{
		const CVec2 vCatchingFormationCenter = pCatchingFormation->GetCenterPlain();
		int nEntrance;
		CVec2 vPointToGo( -1.0f, -1.0f );
		if ( pBuilding->ChooseEntrance( pCatchingFormation, vCatchingFormationCenter, &nEntrance ) )
			vPointToGo = pBuilding->GetEntrancePoint( nEntrance );

		// можно вбежать в это здание
		if ( vPointToGo.x != -1.0f )
		{
			// уже близко
			if ( fabs2( vCatchingFormationCenter - vPointToGo ) <= sqr( float( 5 * SConsts::TILE_SIZE ) ) )
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_ENTER, pBuilding->GetUniqueId(), float(0) ), pCatchingFormation );
			else
			{
				// нужно подправить путь
				if ( pBuilding != pLastFormationObject || pCatchingFormation->IsIdle() )
				{
					pLastFormationObject = pBuilding;
					if ( CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vPointToGo, VNULL2, pCatchingFormation, true, GetAIMap() ) )
						pCatchingFormation->SendAlongPath( pPath, VNULL2, true );
				}
			}
		}
	}
	else
	{
		NI_ASSERT( pSoldier->IsInBuilding() && pSoldier->GetBuilding() == pBuilding, "Wrong state of CCatchFormationState state" );

		if ( (*pFormation)[0]->CanJoinToFormation() )
			JoinToFormation();
	}
}

void CCatchFormationState::AnalyzeFormationInEntrenchment( CEntrenchment *pEntrenchment )
{
	CSoldier *pSoldier = (*pCatchingFormation)[0];

	if ( !pSoldier->IsFree() && ( !pSoldier->IsInEntrenchment() || pSoldier->GetEntrenchment() != pEntrenchment ) )
		LeaveCurStaticObject();
	else if ( pSoldier->IsFree() )
	{
		const CVec2 vCatchingFormationCenter = pCatchingFormation->GetCenterPlain();
		CVec2 vPointToGo;
		pEntrenchment->GetClosestPoint( pSoldier->GetCenterPlain(), &vPointToGo );

		// близко к точке входа
		if ( fabs2( vCatchingFormationCenter - vPointToGo ) <= sqr( float( 5 * SConsts::TILE_SIZE ) ) )
			theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_ENTER, pEntrenchment->GetUniqueId(), float(1) ), pCatchingFormation );
		else if ( pCatchingFormation->IsIdle() || pLastFormationObject != (CObjectBase*)pEntrenchment )
		{
			pLastFormationObject = pEntrenchment;
			if ( CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vPointToGo, VNULL2, pCatchingFormation, true, GetAIMap() ) )
				pCatchingFormation->SendAlongPath( pPath, VNULL2, true );
		}
	}
	else
	{
		NI_ASSERT( pSoldier->IsInEntrenchment() && pSoldier->GetEntrenchment() == pEntrenchment, "Wrong state of CCatchFormationState" );

		if ( (*pFormation)[0]->CanJoinToFormation() )
			JoinToFormation();
	}
}

void CCatchFormationState::AnalyzeFormationInTransport( CMilitaryCar *pCar )
{
	CSoldier *pSoldier = (*pCatchingFormation)[0];

	if ( !pSoldier->IsFree() )
		LeaveCurStaticObject();
	else
	{
		const CVec2 vTransportCenter = pCar->GetCenterPlain();
		const float fDist2 = fabs2( pCatchingFormation->GetCenterPlain() - vTransportCenter );
		// далеко, нужно подбежать
		if ( fDist2 > sqr( float( 5*SConsts::TILE_SIZE ) ) && 
				( pLastFormationObject != (CObjectBase*)pCar || pCatchingFormation->IsIdle() ) )
		{
			pLastFormationObject = pCar;
			if ( CPtr<IStaticPath> pPath = CreateStaticPathToPoint( vTransportCenter, VNULL2, pCatchingFormation, true, GetAIMap() ) )
				pCatchingFormation->SendAlongPath( pPath, VNULL2, true );
		}
	}
}

void CCatchFormationState::SetToDisbandedState()
{
	CSoldier *pSoldier = (*pCatchingFormation)[0];
	MoveSoldierToFormation();

	pSoldier->MemCurFormation();
	CCommonUnit *pSingleFormation = theUnitCreation.CreateSingleUnitFormation( pSoldier );

	if ( pSoldier->IsFree() )
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_GUARD, CVec2( -1.0f, -1.0f ), 0 ), pSingleFormation, false );
	else if ( pSoldier->IsInTransport() )
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRANSPORT, pSoldier->GetTransportUnit()->GetUniqueId() ), pSingleFormation, false );
	else if ( pSoldier->IsInBuilding() )
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_BUILDING, pSoldier->GetBuilding()->GetUniqueId() ), pSingleFormation, false );
	else if ( pSoldier->IsInEntrenchment() )
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRENCH, pSoldier->GetEntrenchment()->GetUniqueId() ), pSingleFormation, false );

	pCatchingFormation->Disappear();
}

void CCatchFormationState::Segment()
{
	// формация расформирована
	if ( pFormation->IsDisabled() )
		SetToDisbandedState();
	// пора проверить состояние формации
	else if ( curTime - lastUpdateTime >= 1500 )
	{
		// всех невиртуальных убили
		if ( pFormation->Size() == 0 )
			JoinToFormation();
		else
		{
			CSoldier *pFormationSoldier = (*pFormation)[0];

			if ( pFormationSoldier->IsFree() )
				AnalyzeFreeFormation();
			else if ( pFormationSoldier->IsInBuilding() )
				AnalyzeFormationInBuilding( pFormationSoldier->GetBuilding() );
			else if ( pFormationSoldier->IsInEntrenchment() )
				AnalyzeFormationInEntrenchment( pFormationSoldier->GetEntrenchment() );
			else
				AnalyzeFormationInTransport( pFormationSoldier->GetTransportUnit() );
		}

		lastUpdateTime = curTime;
	}
}

ETryStateInterruptResult CCatchFormationState::TryInterruptState( CAICommand *pCommand )
{
	pCatchingFormation->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*												CFormationSwarmState											*
//*******************************************************************

IUnitState* CFormationSwarmState::Instance( CFormation *pFormation, const CVec2 &point, const float fContinue )
{
	return new CFormationSwarmState( pFormation, point, fContinue );
}

CFormationSwarmState::CFormationSwarmState( CFormation *_pFormation, const CVec2 &_point, const float fContinue )
: pFormation( _pFormation ), point( _point ), state( EFSS_START ), startTime( curTime ), bContinue( fContinue)
{
	for ( int i = 0; i < pFormation->Size(); ++i )
		(*pFormation)[i]->ResetTargetScan();
}

void CFormationSwarmState::AnalyzeTargetScan()
{
	bool bAttacking = false;
	for ( int i = 0; i < pFormation->Size(); ++ i )
	{
		if ( ( (*pFormation)[i]->AnalyzeTargetScan( 0, false, false ) & 1 ) )
		{
			bAttacking = true;
			break;
		}
	}

	if ( bAttacking )
	{
		state = EFSS_WAIT;
		pFormation->Stop();
		pFormation->SetToWaitingState();
		//DebugInfoManager()->CreateCircle( pFormation->GetUniqueID(), CCircle( pFormation->GetCenterPlain(), 16 ), NDebugInfo::RED );
		startTime = curTime;
	}
}

void CFormationSwarmState::Segment()
{
	switch ( state )
	{
		case EFSS_START:
			if ( curTime - startTime >= TIME_OF_WAITING )
			{
				state = EFSS_MOVING;
				if ( !bContinue )
				{
					if ( CPtr<IStaticPath> pStaticPath = pFormation->GetCurCmd()->CreateStaticPath( pFormation ) )
						pFormation->SendAlongPath( pStaticPath, pFormation->GetGroupShift(), true );
				}
				else if ( CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( point, pFormation->GetGroupShift(), pFormation, false, GetAIMap() ) )
				{
					pFormation->SetGroupShift( VNULL2 );
					pFormation->SendAlongPath( pStaticPath, pFormation->GetGroupShift(), true );
				}
				else
				{
					pFormation->SendAcknowledgement( ACK_NEGATIVE );
					TryInterruptState( 0 );
				}
			}
			AnalyzeTargetScan();

			break;
		case EFSS_MOVING:
			if ( pFormation->IsIdle() )
				pFormation->SetCommandFinished();
			else
				AnalyzeTargetScan();

			break;
		case EFSS_WAIT:
			if ( curTime - startTime >= TIME_OF_WAITING && pFormation->IsAnyUnitResting() )
			{
				pFormation->BalanceCenter();
				//DebugInfoManager()->CreateCircle( pFormation->GetUniqueID(), CCircle( pFormation->GetCenterPlain(), 16 ), NDebugInfo::RED );
				if ( pFormation->IsEveryUnitResting() )
				{
					startTime = curTime - TIME_OF_WAITING;
					bContinue = true;

					state = EFSS_START;
				}
			}
	}
}

ETryStateInterruptResult CFormationSwarmState::TryInterruptState( CAICommand *pCommand )
{
	if ( pFormation->GetCurCmd() != 0 )
		pFormation->GetCurCmd()->ToUnitCmd().fNumber = 1;

	pFormation->SetCommandFinished();
	pFormation->UnsetFromWaitingState();

	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*										CFormationEnterBuildingNowState								*
//*******************************************************************

IUnitState* CFormationEnterBuildingNowState::Instance( CFormation *pFormation, CBuilding *pBuilding )
{
	return new CFormationEnterBuildingNowState( pFormation, pBuilding );
}

CFormationEnterBuildingNowState::CFormationEnterBuildingNowState( CFormation *_pFormation, CBuilding *pBuilding )
: pFormation( _pFormation )
{
	NI_ASSERT( pBuilding->GetNEntrancePoints() != 0, StrFmt( "infantry inside Building without entrance points, Building Link ID = %i, at pos (%.0f, %.0f), DBID = \"%s\"", pBuilding->GetLink(), pBuilding->GetCenter().x, pBuilding->GetCenter().y, NDb::GetResName(pBuilding->GetStats()) ) );
	if ( pBuilding->GetNEntrancePoints() != 0 )
	{
		for ( int i = 0; i < pFormation->Size(); ++i )
			theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_BUILDING, pBuilding->GetUniqueId() ), (*pFormation)[i], false );

		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_BUILDING, pBuilding->GetUniqueId() ), pFormation, false );
	}
}

void CFormationEnterBuildingNowState::Segment()
{
	pFormation->SetCommandFinished();
}

ETryStateInterruptResult CFormationEnterBuildingNowState::TryInterruptState( CAICommand *pCommand )
{
	pFormation->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*										CFormationEnterEntrenchmentNowState						*
//*******************************************************************

IUnitState* CFormationEnterEntrenchmentNowState::Instance( CFormation *pFormation, CEntrenchment *pEntrenchment )
{
	return new CFormationEnterEntrenchmentNowState( pFormation, pEntrenchment );
}

CFormationEnterEntrenchmentNowState::CFormationEnterEntrenchmentNowState( CFormation *_pFormation, CEntrenchment *pEntrenchment )
: pFormation( _pFormation )
{
	for ( int i = 0; i < pFormation->Size(); ++i )
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRENCH, pEntrenchment->GetUniqueId(), float(2) ), (*pFormation)[i], false );

	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_IDLE_TRENCH, pEntrenchment->GetUniqueId(), float(2) ), pFormation, false );
}

void CFormationEnterEntrenchmentNowState::Segment()
{
	pFormation->SetCommandFinished();
}

ETryStateInterruptResult CFormationEnterEntrenchmentNowState::TryInterruptState( CAICommand *pCommand )
{
	pFormation->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*										CFormationPlaceChargeState        						*
//*******************************************************************

IUnitState* CFormationPlaceChargeState::Instance( class CFormation *pUnit, const CVec2 &vDesiredPoint, NDb::EUnitSpecialAbility _eChargeType, int nTimeOffset )
{
	return new CFormationPlaceChargeState( pUnit, vDesiredPoint, _eChargeType, nTimeOffset );
}

CFormationPlaceChargeState::CFormationPlaceChargeState( class CFormation *pUnit, const CVec2 &vDesiredPoint, NDb::EUnitSpecialAbility _eChargeType, int nTimeOffset ) :
	CStatusUpdatesHelper( EUS_BLASTING_CHARGE, pUnit ), pFormation( pUnit ), vTarget( vDesiredPoint ), nOffset( nTimeOffset ), eState( EPCS_MOVING_TO ), eChargeType( _eChargeType )
{
	pSapper = 0;
	for ( int i = 0; i < pFormation->Size(); ++i )
	{
		if ( (*pFormation)[i]->GetChargesLeft( eChargeType ) > 0 )
		{
			pSapper = (*pFormation)[i];
			break;
		}
	}
	if ( pSapper == 0 )
	{
		TryInterruptState( 0 );
		return;
	}
	if ( pSapper->IsInTankPit() ) 
	{
		pSapper->SetOffTankPit();
	}
	
	CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( vTarget, VNULL2, pSapper, false, GetAIMap() ); 
	if ( pStaticPath )
		pSapper->SendAlongPath( pStaticPath, VNULL2, true );
	else
	{
		pFormation->SendAcknowledgement( ACK_NEGATIVE, true );
		pFormation->SetCommandFinished(); 
	}
}

void CFormationPlaceChargeState::Segment()
{
	InitStatus();
	if ( !IsValidObj( pSapper ) )
	{
		pFormation->SetCommandFinished();
		return;
	}
	switch ( eState )
	{
		case EPCS_MOVING_TO:
			if ( pSapper->IsIdle() )
			{
				eState = EPCS_LAYING_CHARGE;
				nBeginAnimTime = curTime;
				updater.AddUpdate( 0, ACTION_NOTIFY_USE_DOWN, pSapper, -1 );
			}
			break;
		case EPCS_RETURNING:
			if ( pSapper->IsIdle() )
				pFormation->SetCommandFinished();
			break;
		case EPCS_LAYING_CHARGE:
			if ( curTime - nBeginAnimTime >= pSapper->GetStats()->GetAnimTime( GetAnimationFromAction( ACTION_NOTIFY_USE_DOWN ) ) )
			{
				CMineStaticObject *pMine = theUnitCreation.CreateMine( eChargeType != NDb::ABILITY_LAND_MINE ? MT_CHARGE : MT_LANDMINE, CVec3(vTarget,0.0f), pSapper->GetPlayer() );
				pSapper->UsedCharge( eChargeType, pMine );
				if ( eChargeType == NDb::ABILITY_PLACE_CHARGE )
				{
					CPtr<CTimerChargeExecutor> pExecutor = new CTimerChargeExecutor( pMine, nOffset );
					pTheExecutorsContainer->Add( pExecutor );
				}
				ReturnSapper();
			}
			break;
	}
}

ETryStateInterruptResult CFormationPlaceChargeState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !IsValidObj( pSapper ) )
	{
		pFormation->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	if ( eState == EPCS_MOVING_TO )
	{
		if ( !pSapper->IsIdle() )
			pSapper->Stop();
		if ( pSapper->IsAlive() && pSapper->GetHitPoints() > 0.0f )
			ReturnSapper();		
	}
	return TSIR_YES_WAIT;
}

const CVec2 CFormationPlaceChargeState::GetPurposePoint() const
{
	return vTarget;
}

void CFormationPlaceChargeState::ReturnSapper()
{
	eState = EPCS_RETURNING;
	const CVec2 vPoint = pSapper->GetUnitPointInFormation();
	CPtr<IStaticPath> pStaticPath = CreateStaticPathToPoint( vPoint, VNULL2, pSapper, false, GetAIMap() ); 
	if ( pStaticPath )
	{
		pSapper->SendAlongPath( pStaticPath, VNULL2, true );		
	}
	else 
		pFormation->SetCommandFinished();
}

//*******************************************************************
//*										CFormationDetonateChargeState        						*
//*******************************************************************

IUnitState* CFormationDetonateChargeState::Instance( class CFormation *pUnit )
{
	return new CFormationDetonateChargeState( pUnit );
}

CFormationDetonateChargeState::CFormationDetonateChargeState( class CFormation *pUnit ) :
	pFormation( pUnit )
{
	NI_ASSERT( pFormation->Size() > 0, "Detonating charge for empty formation!" );
	nBeginAnimTime = curTime;
	for ( int i = 0; i < pFormation->Size(); ++i )
	{
		CSoldier *pSoldier = (*pFormation)[i];
		if ( pSoldier->HasChargesToDetonate() )
		{
			sappers.push_back( pSoldier );
			updater.AddUpdate( 0, ACTION_NOTIFY_USE_DOWN, pSoldier, -1 );
		}
	}
}

void CFormationDetonateChargeState::Segment()
{
	for ( std::list< CPtr<CSoldier> >::iterator it = sappers.begin(); it != sappers.end(); )
	{
		if ( !IsValidObj( *it ) )
			it = sappers.erase( it );
		else
			++it;
	}

	
	for ( std::list< CPtr<CSoldier> >::iterator it = sappers.begin(); it != sappers.end(); )
	{
		if ( curTime - nBeginAnimTime >= (*it)->GetStats()->GetAnimTime( GetAnimationFromAction( ACTION_NOTIFY_USE_DOWN ) ) )
		{
			(*it)->DetonateCharges();
			it = sappers.erase( it );
		}
		else
			++it;
	}

	if ( sappers.empty() )
		pFormation->SetCommandFinished();
}

ETryStateInterruptResult CFormationDetonateChargeState::TryInterruptState( class CAICommand *pCommand )
{
	pFormation->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

// ************************************************************************************************************************ //
// ** CFormationThrowGrenadeState 
// ************************************************************************************************************************ //

CFormationThrowGrenadeState::CFormationThrowGrenadeState( class CFormation *_pUnit )
: pFormation( _pUnit )
{
}

void CFormationThrowGrenadeState::AddTarget( CAIUnit *pEnemy, const CVec2 &vTarget, const int nParam )
{
	CSoldier *pSoldier = 0;
	
	// get any free soldier with grenade gun with non zero ammo
	for ( int i = 0; i < pFormation->Size(); ++i )
	{
		if ( (*pFormation)[i]->HasGrenades() )
		{
			pSoldier = (*pFormation)[i];
			break;
		}
	}

	// no free soldiers, let's get one from used
	if ( pSoldier == 0 )
	{
		for ( std::vector< SThrowInfo >::iterator it = vSoldiers.begin(); it != vSoldiers.end(); ++it )
		{
			if ( it->pSoilder->HasGrenades() )
			{
				pSoldier = it->pSoilder;
				vSoldiers.erase( it );
				break;
			}
		}
	}

	// this sqaud cannot throw grenade :(
	if ( pSoldier == 0 )
		return;

	// add unit to busy unit storage, and send command for it
	vSoldiers.push_back( SThrowInfo(pSoldier) );
	SAIUnitCmd cmd( ACTION_COMMAND_THROW_GRENADE );
	cmd.nNumber = nParam;
	cmd.nObjectID = pEnemy ? pEnemy->GetUniqueId() : 0;
	cmd.vPos = vTarget;
	theGroupLogic.UnitCommand( cmd, pSoldier, false );
}

IUnitState* CFormationThrowGrenadeState::Instance( class CFormation *pUnit )
{
	return new CFormationThrowGrenadeState( pUnit );
}

void CFormationThrowGrenadeState::Segment()
{
	for ( std::vector< SThrowInfo >::iterator it = vSoldiers.begin(); it != vSoldiers.end(); )
	{
		it->pSoilder->Segment();		//CRAP? - to allow the soldier to pick the command, if any

		const EUnitStateNames eState = ( it->pSoilder->GetState() == 0) ? EUSN_ERROR : it->pSoilder->GetState()->GetName();

		if ( !it->bPassSegment && eState != EUSN_ATTACK_UNIT && eState != EUSN_ATTACK_OBJECT && 
			eState != EUSN_ATTACK_IN_ENTRENCH && eState != EUSN_ATTACK_UNIT_IN_BUILDING )
		{
			it = vSoldiers.erase( it );
		}
		else
		{
			it->bPassSegment = false;
			++it;
		}
	}

	if ( vSoldiers.empty() )
		TryInterruptState( 0 );
}

ETryStateInterruptResult CFormationThrowGrenadeState::TryInterruptState( class CAICommand *pCommand )
{
	pFormation->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

// ************************************************************************************************************************ //
// ** CFormationEntrenchSelfState
// ************************************************************************************************************************ //

CFormationEntrenchSelfState::CFormationEntrenchSelfState( class CFormation *_pFormation ) 
: CStatusUpdatesHelper( EUS_ENTRENCH, _pFormation ), pFormation( _pFormation ), timeStart( curTime + 200 ), bWaitForSoldiers( false )
{
	pFormation->Stop();
}

void CFormationEntrenchSelfState::Segment()
{
	InitStatus();
	if ( bWaitForSoldiers )
	{
		bool bAllReady = true;
		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			CSoldier *pMember = (*pFormation)[i];

			if ( !pMember->IsInTankPit() && pMember->GetState()->GetName() == EUSN_ENTRENCH_SELF )		// is the soldier still digging in
			{
				bAllReady = false;
				break;
			}
		}

		if ( bAllReady )
			TryInterruptState( 0 );

		return;
	}

	if ( curTime > timeStart )
	{
		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			if ( (*pFormation)[i]->GetState()->GetName() == EUSN_ENTRENCH_SELF )
				return;
		}
		// find deffensive geometry
		for ( int i = 0; i < pFormation->GetStats()->formations.size(); ++i )
		{
			if ( pFormation->GetStats()->formations[i].etype == NDb::SSquadRPGStats::SFormation::DEFENSIVE )
			{
				pFormation->ChangeGeometry( i );
				break;
			}
		}
		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_SELF_ENTRENCH), (*pFormation)[i], false );
		}

		//TryInterruptState( 0 );
		bWaitForSoldiers = true;
	}
}

ETryStateInterruptResult CFormationEntrenchSelfState::TryInterruptState( class CAICommand *pCommand )
{
	if ( pCommand )
	{
		return TSIR_NO_COMMAND_INCOMPATIBLE;
	}
	else
	{
		pFormation->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
}

// ************************************************************************************************************************ //
// ** CFormationLeaveSelfEntrenchState
// ************************************************************************************************************************ //

CFormationLeaveSelfEntrenchState::CFormationLeaveSelfEntrenchState( class CFormation *_pFormation )
: pFormation( _pFormation ), timeStart( curTime + 200 )
{
	for ( int i = 0; i < pFormation->Size(); ++i )
	{
		theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_LEAVE_SELF_ENTRENCH), (*pFormation)[i], false );
	}
}

void CFormationLeaveSelfEntrenchState::Segment()
{
	if ( curTime > timeStart )
	{
		for ( int i = 0; i < pFormation->Size(); ++i )
		{
			if ( (*pFormation)[i]->IsInTankPit() )
				return;
		}
		TryInterruptState( 0 );
	}
}

ETryStateInterruptResult CFormationLeaveSelfEntrenchState::TryInterruptState( class CAICommand *pCommand )
{
	if ( pCommand )
		return TSIR_NO_COMMAND_INCOMPATIBLE;
	else
	{
		pFormation->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
}

// ************************************************************************************************************************ //
// ** CFormationFirstAidState
// ************************************************************************************************************************ //

CFormationFirstAidState::SHealingPair::~SHealingPair()
{
	if ( IsValidObj( pPatient ) )
		pPatient->SetBeingHealed( false );
}

CFormationFirstAidState::SHealingPair::SHealingPair( CSoldier *_pPatient, CSoldier *_pDoctor ) 
: pPatient( _pPatient ), pDoctor( _pDoctor ), timeHealed( 0 ), eState( EPS_JUST_CREATED )
{  
}

void CFormationFirstAidState::SHealingPair::SetNewPatient( CSoldier *_pPatient )
{
	timeHealed = 0;
	eState = EPS_JUST_CREATED;
	if ( IsValidObj( pPatient ) )
		pPatient->SetBeingHealed( false );
	pPatient = _pPatient;
	pPatient->SetBeingHealed( true );
}

void CFormationFirstAidState::SHealingPair::Heal()
{
	pPatient->IncreaseHitPoints( pPatient->GetStats()->fMaxHP );
	pPatient->SetBeingHealed( false );
}

CFormationFirstAidState::CFormationFirstAidState( class CFormation *_pFormation, CSoldier *pPriorityUnit )
: pFormation( _pFormation ), vStartPoint( _pFormation->GetCenterPlain() ), timeNextCheck( curTime )
{
	// choose doctors from current squad and assign them to jobs
	for ( int i = 0; i < pFormation->Size(); ++i )
	{
		CSoldier *pSold = (*pFormation)[i];
		const int nLevel = theStatistics.GetAbilityLevel( pSold->GetPlayer(), pSold->GetReinforcementType() );
		const NDb::SUnitBaseRPGStats *pStats = pSold->GetStats();
		const int nAbilities = (std::min<int>)( nLevel+1, pStats->GetActions()->specialAbilities.size() );
		bool bCanHeal = false;
		for ( int nAbility = 0; nAbility < nAbilities; ++nAbility )
		{
			if ( pStats->GetActions()->specialAbilities[nAbility]->eName == NDb::ABILITY_FIRST_AID )
			{
				bCanHeal = true;
				break;
			}
		}

		if ( bCanHeal )
			healingPairs.push_back( SHealingPair( 0, pSold ) );
	}

	NI_ASSERT( !healingPairs.empty(), "no units with healing ability in this squad" );
	if ( pPriorityUnit )
		pHealeadFormation = pPriorityUnit->GetFormation();
}

bool CFormationFirstAidState::FindNearestPatient( CSoldier *pDoctor, CSoldier *pFormerPatient, CSoldier **pNewPatient )
{
	if ( pHealeadFormation )
	{
		if ( IsValidObj( pHealeadFormation ) )
		{
			for ( int i = 0; i < pHealeadFormation->Size(); ++i )
			{
				CSoldier *pSold = (*pHealeadFormation)[i];
				if ( IsValidObj( pSold ) )
				{
					const NDb::SUnitBaseRPGStats *pStats = pSold->GetStats();
					if ( pStats->fMaxHP > pSold->GetHitPoints() && !pSold->IsBeingHealed() )
					{
						*pNewPatient = pSold;
						return true;
					}
				}
			}
		}
	}
	else
	{
		float fCurWeight = 0;

		for ( CUnitsIter<0,2> iter( pDoctor->GetParty(), EDI_FRIEND, vStartPoint, SConsts::MED_TRUCK_HEAL_RADIUS ); !iter.IsFinished(); iter.Iterate() )
		{
			CAIUnit *pUnit = *iter;
			if ( IsSoldierNeedHealing( pUnit ) )
			{
				const float fWeight = //1.0f / Min( 1.0f, fabs( pSold->GetCenter() - vStartPoint ) ) + 
											1.0f / (std::max)( 1.0f, fabs( pUnit->GetCenter() - pDoctor->GetCenter() ) );
				if ( fWeight > fCurWeight )
					*pNewPatient = checked_cast<CSoldier*>( pUnit );
			}
		}
	}
	return *pNewPatient;
}

bool CFormationFirstAidState::IsSoldierNeedHealing( CAIUnit *pUnit )
{
	if ( IsValidObj( pUnit ) )
	{
		const NDb::SUnitBaseRPGStats *pStats = pUnit->GetStats();
		if ( pStats->IsInfantry() && pUnit->IsFree() && pUnit->IsIdle() && pStats->fMaxHP > pUnit->GetHitPoints() )
		{
			CSoldier *pSold = checked_cast<CSoldier*>( pUnit );
			if ( pSold && !pSold->IsBeingHealed() )
			{
				return true;
			}
		}
	}
	return false;
}

bool CFormationFirstAidState::IsAnyNeedHealing( const int nParty, const CVec3 &vCenter )
{
	for ( CUnitsIter<0,2> iter( nParty, EDI_FRIEND, CVec2(vCenter.x, vCenter.y), SConsts::MED_TRUCK_HEAL_RADIUS ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( IsSoldierNeedHealing( pUnit ) )
			return true;
	}
	return false;
}

void CFormationFirstAidState::Segment()
{
	bool bValidDocFound = false;
	bool bValidPatientFound = true;
	const NTimer::STime timePassed = SConsts::AI_SEGMENT_DURATION;

	for ( int i = 0; i < healingPairs.size(); ++i )
	{
		if ( IsValidObj( healingPairs[i].pDoctor ) )
		{
			bValidDocFound = true;
			if ( !IsValidObj( healingPairs[i].pPatient ) )
				healingPairs[i].eState = SHealingPair::EPS_HEAL_COMPLETE;

			switch( healingPairs[i].eState )
			{
			case SHealingPair::EPS_DOC_NEAR_PATIENT:
				// launch use animation
				theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_USE, float(ACTION_NOTIFY_USE_UP) ), healingPairs[i].pDoctor, false );
				healingPairs[i].eState = SHealingPair::EPS_HEAL_STARTED;

				break;
			case SHealingPair::EPS_HEAL_STARTED:
				{
					healingPairs[i].timeHealed += timePassed;
					const NDb::SUnitSpecialAblityDesc *pSA = healingPairs[i].pDoctor->GetUnitAbilityDesc( NDb::ABILITY_FIRST_AID );
					NTimer::STime timeFullHeal;
					if ( pSA ) 
						timeFullHeal = pSA->nWorkTime;
					else
						timeFullHeal = SConsts::INFANTRY_FULL_HEAL_TIME;

					if ( healingPairs[i].timeHealed > timeFullHeal )
					{
						healingPairs[i].Heal();
						healingPairs[i].eState = SHealingPair::EPS_HEAL_COMPLETE;
						theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP ), healingPairs[i].pDoctor, false );
					}
				}
				break;
			case SHealingPair::EPS_HEAL_COMPLETE:
				{		
					if ( !healingPairs[i].pDoctor->CanMove() ) // wait untill doc can move
						break;
					if ( bValidPatientFound )
					{
						CSoldier * pHealedUnit = healingPairs[i].pPatient;
						CSoldier * pNewPatient = 0;

						bValidPatientFound = FindNearestPatient( healingPairs[i].pDoctor, pHealedUnit, &pNewPatient );
						if ( bValidPatientFound )
						{
							healingPairs[i].SetNewPatient( pNewPatient );
							RecordRandomCall();
							CPtr<IStaticPath> pPath = CreateStaticPathToPoint( 
													pNewPatient->GetCenterPlain() + SConsts::TILE_SIZE * GetVectorByDirection( NRandom::Random( 65535 ) ), 
													VNULL2, 
													healingPairs[i].pDoctor,
													true, GetAIMap() );
							if ( pPath )
							{
								healingPairs[i].pDoctor->SendAlongPath( pPath, VNULL2, true );
								healingPairs[i].eState = SHealingPair::EPS_HEADING_TO_PATIENT;
							}
						}
					}
				}

				break;
			case SHealingPair::EPS_HEADING_TO_PATIENT:
				if ( healingPairs[i].pDoctor->IsIdle() )
				{
					healingPairs[i].eState = SHealingPair::EPS_DOC_NEAR_PATIENT;
					const CVec2 vDir ( healingPairs[i].pPatient->GetCenterPlain() - healingPairs[i].pDoctor->GetCenterPlain() );
					healingPairs[i].pDoctor->SetDirectionVec( vDir );
				}

				break;
			}
		}
	}
	timeNextCheck = curTime + SConsts::TIME_QUANT;

	if ( !bValidDocFound )
		TryInterruptState( 0 );
}

ETryStateInterruptResult CFormationFirstAidState::TryInterruptState( class CAICommand *pCommand )
{
	pFormation->SetCommandFinished();
	if ( pCommand )
	{
		const NDb::SUnitSpecialAblityDesc *pDesc = pFormation->GetUnitAbilityDesc( ABILITY_FIRST_AID );
		if ( pDesc )
		{
			SExecutorEventParam param( EID_ABILITY_DEACTIVATE, 0, pFormation->GetUniqueID() );
			CExecutorEventSpecialAbilityActivate event( param, ABILITY_FIRST_AID, 0, pDesc );
			theExecutorContainer.RaiseEvent( event );
		}
		theGroupLogic.InsertUnitCommand( pCommand->ToUnitCmd(), pFormation );
		return TSIR_YES_MANAGED_ALREADY;
	}
	return TSIR_YES_IMMIDIATELY;
}


