#include "stdafx.h"

#include "../Stats_B2_M1/AbilityActions.h"
#include "PlaneStates.h"
#include "Building.h"
#include "GroupLogic.h"
#include "Commands.h"
#include "Formation.h"
#include "Aviation.h"
#include "PlanesFormation.h"
#include "UnitsIterators2.h"
#include "UnitsIterators.h"
#include "Soldier.h"
#include "UnitCreation.h"
#include "Guns.h"
#include "NewUpdater.h"
#include "Turret.h"
#include "ShootEstimatorInternal.h"
#include "PlanePath.h"
#include "General.h"
#include "Weather.h"
#include "ManuverInternal.h"
#include "DBAIConsts.h"
//CRAP{ FOR TEXT
#include "../SceneB2/StatSystem.h"
//CRAP}
#include "../Common_RTS_AI/StaticMapHeights.h"
#include "Shell.h"
#include "AILogicInternal.h"
#include "../DebugTools/DebugInfoManager.h"
#include "../System/Commands.h"

extern CWeather theWeather;
extern CScripts *pScripts;
extern CSupremeBeing theSupremeBeing;
extern CEventUpdater updater;
extern CUnitCreation theUnitCreation;
extern NTimer::STime curTime;
extern CDiplomacy theDipl;
extern CGroupLogic theGroupLogic;
extern CShellsStore theShellsStore;

static float g_fBombDiveDistance = 0.0f;
static const float LEAVE_DISSAPEAR_DISTANCE = 512.0f;

START_REGISTER(SuicideVars)
REGISTER_VAR_EX( "AI.Aviation.FAU2_DiveDistance", NGlobal::VarFloatHandler, &g_fBombDiveDistance, 0.0f, STORAGE_NONE );
FINISH_REGISTER

//*******************************************************************
//*										  CPlaneStatesFactory													*
//*******************************************************************

CPtr<CPlaneStatesFactory> CPlaneStatesFactory::pFactory = 0;

IStatesFactory* CPlaneStatesFactory::Instance()
{
	if ( pFactory == 0 )
		pFactory = new CPlaneStatesFactory();

	return pFactory;
}

bool CPlaneStatesFactory::CanCommandBeExecuted( class CAICommand *pCommand )
{
	const EActionCommand &cmdType = pCommand->ToUnitCmd().nCmdType;
	return 
		(
			cmdType == ACTION_COMMAND_DIE			||
			cmdType == ACTION_COMMAND_DISAPPEAR ||
			cmdType == ACTION_MOVE_PLANE_LEAVE ||
			cmdType == ACTION_MOVE_FLY_DEAD ||
			cmdType == ACTION_COMMAND_MOVE_TO  ||
			cmdType == ACTION_COMMAND_ATTACK_UNIT ||
			cmdType == ACTION_MOVE_ATTACKPLANE_SETPOINT ||
			cmdType == ACTION_MOVE_FIGHTER_SETPOINT ||
			cmdType == ACTION_COMMAND_UNLOAD ||
			cmdType == ACTION_COMMAND_SWARM_TO ||
			cmdType == ACTION_MOVE_DROP_BOMBS_TO_TARGET ||
			cmdType == ACTION_MOVE_DROP_BOMBS_TO_POINT ||
			cmdType == ACTION_COMMAND_PATROL
		);
}

IUnitState* CPlaneStatesFactory::ProduceState( class CQueueUnit *pObj, CAICommand *pCommand )
{
	NI_ASSERT( dynamic_cast<CAviation*>( pObj ) != 0, "Wrong unit type" );
	CAviation *pUnit = checked_cast<CAviation*>( pObj );
	
	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();
	IUnitState* pResult = 0;
	
	switch ( cmd.nCmdType )
	{
	case ACTION_COMMAND_PATROL:
		if ( pUnit->GetState()->GetName() == EUSN_PLANE_PATROL )
		{
			CPlanePatrolState * pState = checked_cast<CPlanePatrolState*>( pUnit->GetState() );
			pState->AddPoint( cmd.vPos );
			pResult = pState;
		}
		else
		{
			if ( pUnit->GetStats()->etype == RPG_TYPE_AVIA_FIGHTER )
				pResult = new CPlaneFighterPatrolState( pUnit, cmd.vPos, 0 );
			else if ( pUnit->GetStats()->etype == RPG_TYPE_AVIA_ATTACK )
				pResult = new CPlaneShturmovikPatrolState( pUnit, cmd.vPos, 0, 0, false );
			else
			{
				CPlanePatrolState * pState = new CPlaneScoutState( pUnit );
				pState->AddPoint( cmd.vPos );
				pResult = pState;
			}
		}
 
		break;
	case ACTION_MOVE_PLANE_LEAVE:
		pResult = new CPlaneLeaveState( pUnit );

		break;
	case ACTION_COMMAND_UNLOAD:
		pResult = new CPlaneParaDropState( pUnit, (const EActionLeaveParam)int(cmd.fNumber), cmd.vPos, checked_cast<CFormation*>( GetObjectByCmd( cmd ) ) );

		break;
	case ACTION_MOVE_ATTACKPLANE_SETPOINT:
		pResult = new CPlaneShturmovikPatrolState( pUnit, cmd.vPos, 0, 0, false );

		break;
	case ACTION_MOVE_DROP_BOMBS_TO_TARGET:
		{
			if ( 0 == cmd.nNumber )
			{
				CAIUnit *pTarget = checked_cast<CAIUnit*>( GetObjectByCmd( cmd ) );
				pResult = new CPlaneShturmovikPatrolState( pUnit, VNULL2, pTarget, 0, true );
			}
			else if ( 1 == cmd.nNumber )
			{
				CBuilding *pTarget = checked_cast<CBuilding*>( GetObjectByCmd( cmd ) );
				pResult = new CPlaneShturmovikPatrolState( pUnit, VNULL2, 0, pTarget, true );
			}
		}

		break;
	case ACTION_MOVE_DROP_BOMBS_TO_POINT:
	case ACTION_COMMAND_ART_BOMBARDMENT:
		pResult = new CPlaneBombState( pUnit, cmd.vPos );
		
		break;
	case ACTION_MOVE_FIGHTER_SETPOINT:
		pResult = new CPlaneFighterPatrolState( pUnit, cmd.vPos, 0 );

		break;
	case ACTION_COMMAND_SWARM_TO:
		if ( pUnit->GetStats()->etype == NDb::RPG_TYPE_AVIA_BOMBER || pUnit->GetStats()->etype == NDb::RPG_TYPE_AVIA_SUPER )
		{
			if ( cmd.fNumber == 0.0f )
				pResult = new CPlaneBombState( pUnit, cmd.vPos );
			else
				pResult = new CPlaneSuicideState( pUnit, cmd.vPos );
		} 
		else
			pResult = new CPlaneSwarmToState( pUnit, cmd.vPos, true );

		break;
	case ACTION_COMMAND_MOVE_TO:
		if ( pUnit->GetStats()->etype == NDb::RPG_TYPE_AVIA_BOMBER )
		{
			if ( cmd.fNumber == 0.0f )
				pResult = new CPlaneBombState( pUnit, cmd.vPos );
			else
				pResult = new CPlaneSuicideState( pUnit, cmd.vPos );
		}
		else
			pResult = new CPlaneSwarmToState( pUnit, cmd.vPos, false );

		break;
	case ACTION_COMMAND_ATTACK_OBJECT:
		if ( pUnit->GetStats()->etype != RPG_TYPE_AVIA_FIGHTER )
		{
			CLinkObject *pLinkObject = CLinkObject::GetObjectByUniqueIdSafe( cmd.nObjectID );
			CBuilding *pBuilding = dynamic_cast<CBuilding*>( pLinkObject );
			if ( pBuilding )
				pResult = new CPlaneShturmovikPatrolState( pUnit, VNULL2, 0, pBuilding, false );
		}
		else
			pUnit->SendAcknowledgement( pCommand, ACK_INVALID_TARGET, false );

		break;
	case ACTION_COMMAND_ATTACK_UNIT:
		{
			if ( 0 == cmd.nNumber )
			{
				CAIUnit *pTarget = checked_cast<CAIUnit*>( GetObjectByCmd( cmd ) );
				if ( pTarget->GetStats()->IsAviation() )
				{
					if ( pUnit->GetStats()->etype != RPG_TYPE_AVIA_ATTACK || pTarget->GetStats()->etype != RPG_TYPE_AVIA_BOMBER )
						pResult = new CPlaneFighterPatrolState( pUnit, VNULL2, checked_cast<CAviation*>( pTarget ) );
					else
						pUnit->SendAcknowledgement( pCommand, ACK_INVALID_TARGET, false );
				}
				else 
				{
					if ( pUnit->GetStats()->etype != RPG_TYPE_AVIA_FIGHTER )
						pResult = new CPlaneShturmovikPatrolState( pUnit, VNULL2, pTarget, 0, false );
					else
						pUnit->SendAcknowledgement( pCommand, ACK_INVALID_TARGET, false );
				}
			}
			else if ( 1 == cmd.nNumber )
			{
				if ( pUnit->GetStats()->etype != RPG_TYPE_AVIA_FIGHTER )
				{
					CBuilding *pTarget = checked_cast<CBuilding*>( GetObjectByCmd( cmd ) );
					pResult = new CPlaneShturmovikPatrolState( pUnit, VNULL2, 0, pTarget, false );
				}
				else
					pUnit->SendAcknowledgement( pCommand, ACK_INVALID_TARGET, false );
			}
		}

		break;
	case ACTION_MOVE_FLY_DEAD:
		pResult = new CPlaneFlyDeadState( pUnit );
		
		break;
	case ACTION_COMMAND_DIE:
		NI_ASSERT( false, "Command to die in the queue" );

		break;
	}

	return pResult;
}

IUnitState* CPlaneStatesFactory::ProduceRestState( class CQueueUnit *pUnit )
{
	NI_ASSERT( dynamic_cast<CAviation*>( pUnit ) != 0, "Wrong unit type" );
	return new CPlaneRestState( checked_cast<CAviation*>( pUnit ) );
}

//*******************************************************************
//*										  CPlanePatrolState*
//*******************************************************************

CPlaneSwarmToState::CPlaneSwarmToState( CAviation *pUnit, const CVec2 &_vPoint, const bool _bScanForTarget )
: CPlanePatrolState( pUnit ), eState( PSTS_ESTIMATING ),
	CPlaneDeffensiveFire( pUnit ), bScanForTarget( _bScanForTarget )
{
	AddPoint( _vPoint );
}

void CPlaneSwarmToState::Segment()
{
	switch( eState )
	{
	case PSTS_ESTIMATING:
		//CRAP{ TEST
		InitPathToPoint( CVec3( GetPoint(), pPlane->GetCenter().z ), true, false );
		//CRAP}
		eState = PSTS_MOVING;
		break;
	case PSTS_MOVING:
		break;
	}

	if ( bScanForTarget )
		TargetScan();
	
	if ( pPlane->GetPlanesFormation()->IsManuverFinished() )
	{
		TryInterruptState( 0 );
	}
	
	PathSegment();
	AnalyzeBSU();
}

ETryStateInterruptResult CPlaneSwarmToState::TryInterruptState( class CAICommand *pCommand )
{
	
	pPlane->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*										  CPlaneScoutState*
//*******************************************************************

CPlaneScoutState::CPlaneScoutState ( CAviation *_pPlane  ) 
: CPlanePatrolState( _pPlane ), CPlaneDeffensiveFire( _pPlane ),
	eState( EPSS_GOTO_GUARDPOINT )
{
}

void CPlaneScoutState::Segment()
{
	if ( pPlane->GetNextCommand() != 0 && pPlane->GetNextCommand()->ToUnitCmd().nCmdType == ACTION_COMMAND_PATROL )
		pPlane->SetCommandFinished();

	switch( eState )
	{
	case EPSS_GOTO_GUARDPOINT:
		InitPathToPoint( CVec3( GetPoint(), pPlane->GetPreferencesB2().GetPatrolHeight()), true, false );
		eState = EPSS_GOING_TO_GUARDPOINT;	

		break;
	case EPSS_GOING_TO_GUARDPOINT:
		if ( pPlane->GetPlanesFormation()->IsManuverFinished() )
			eState = EPSS_AIM_TO_NEXT_POINT;

		break;
	case EPSS_AIM_TO_NEXT_POINT:
		if ( vPatrolPoints.size() == 1 )
			TryInterruptState( 0 );
		else
		{
			ToNextPoint();
			eState = EPSS_GOTO_GUARDPOINT;
		}
		
		break;
	case EPSS_ESCAPE:
		break;
	}
	// path segment
	if ( PathSegment() && EPSS_ESCAPE != eState )
		eState = EPSS_ESCAPE;
	AnalyzeBSU();
}

ETryStateInterruptResult CPlaneScoutState::TryInterruptState( class CAICommand *pCommand )
{
	pPlane->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

//*******************************************************************
//*										  CPlaneDeffensiveFire*
//*******************************************************************

CPlaneDeffensiveFire::CPlaneDeffensiveFire( class CAviation *pPlane ) 
: timeLastBSUUpdate( 0 ), pOwner( pPlane )
{  
	pDefShootEstimator = new CPlaneDeffensiveFireShootEstimator( pPlane );
}

void CPlaneDeffensiveFire::AnalyzeBSU()
{
	if ( curTime - timeLastBSUUpdate > SConsts::AA_BEH_UPDATE_DURATION )
	{
		timeLastBSUUpdate = curTime;
		const int nGuns = pOwner->GetNGuns();
		for ( int i = 0; i < nGuns; ++i )
		{
			CBasicGun *pGun = pOwner->GetGun( i );
			CTurret *pTurret = pGun->GetTurret();
			if ( pTurret && !pGun->IsFiring() ) 
			{
				if ( !pTurret->IsLocked( pGun ) )
				{
					pDefShootEstimator->Reset( 0, true, 0 );
					pDefShootEstimator->SetGun( pGun );
					//выбирать лучшего врага
					for( CPlanesIter planes; !planes.IsFinished(); planes.Iterate() )
					{
						CAviation *pEnemy = *planes;
						if ( pOwner!= pEnemy && 
								 EDI_ENEMY == theDipl.GetDiplStatus( pOwner->GetPlayer(), pEnemy->GetPlayer() ) )
						{
							pDefShootEstimator->AddUnit( pEnemy );
						}
					}
					if ( pDefShootEstimator->GetBestUnit() )
						pGun->StartEnemyBurst( pDefShootEstimator->GetBestUnit(), true );
				}
				else
				{
					pGun->StartEnemyBurst( pTurret->GetTracedUnit(), false );
				}
			}
		}
	}
}

//*******************************************************************
//*										  CPlanePatrolState*
//*******************************************************************

CPlanePatrolState::CPlanePatrolState( CAviation *_pPlane )
	: pPlane( _pPlane ), nCurPointIndex( 0 ),
	enemie( _pPlane ), timeNextScan( curTime ), bEconomyMode ( false )
{
	pShootEstimator = new CPlaneShturmovikShootEstimator( pPlane );
	AdvancePlane();
}

bool CPlanePatrolState::IsBombsPresent() const
{
	bool bPresent = false;
	const int nGun = pPlane->GetNGuns();
	for ( int i = 0; i < nGun; ++i )
	{
		CBasicGun *pGun = pPlane->GetGun( i );
		if ( NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB == pGun->GetShell().etrajectory && 
			   0 != pGun->GetNAmmo() )
		{
			bPresent = true;
			break;
		}
	}
	return bPresent;
}

void CPlanePatrolState::Escape()
{
	pPlane->SetCommandFinished();
	theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_PLANE_LEAVE), pPlane, false );
}

CAviation * CPlanePatrolState::FindBetterEnemiyPlane( CAviation * pEnemie, const CVec2 &vPoint, const float fRadius ) const
{
	CPtr<CAviation> pBetter; 
	CPlanesIter planes;
	float Dist = 0;
	const SMechUnitRPGStats * pStats = checked_cast<const SMechUnitRPGStats*>(pPlane->GetStats());
	while ( !planes.IsFinished() )
	{
		if ( CanAttackEnemy( *planes ) )
		{
			//found enemie that is near and can be hit by plane's weapon
			float curDist = fabs2( vPoint - (*planes)->GetCenterPlain() );

			CVec2 ce((*planes)->GetCenterPlain());
			
			float distFromPatrolpoint = fabs2( ce - GetPoint() );
			CVec2 planeToEnemie( (*planes)->GetCenterPlain() - vPoint );
			float scalarProduct = pPlane->GetDirVector() * planeToEnemie;
  		
			if ( distFromPatrolpoint < sqr( fRadius ) && //near guard point
					 theDipl.GetDiplStatus( pPlane->GetPlayer(), (*planes)->GetPlayer() ) == EDI_ENEMY &&//enemie
					 (*planes)->GetCenter().z <  pStats->fMaxHeight &&					// in our height range
					 ( !pBetter || Dist < curDist ) && //nearer then former or new enemie
						pStats->fMaxHeight >= (*planes)->GetCenter().z &&
						// the current velocity is near to distance from enemie
						DirsDifference( pPlane->GetDirection(), (*planes)->GetDirection()) < /*30deg*/ 65535.0f/360.0f*30.0f &&
						scalarProduct > 0 
				 )
			{
				int nGuns = pPlane->GetNGuns();
				bool bCanBreak = false;
				for ( int i=0; i< nGuns && !bCanBreak; ++i )
				{
					bCanBreak = pPlane->GetGun( i )->CanBreakArmor( *planes ); //can damage
				}
				if ( bCanBreak )
				{
					Dist = curDist;
					pBetter = (*planes);
				}
			}
		}
		planes.Iterate();
	}
	if( pBetter != 0 && pBetter!= pEnemie )
	{
		return pBetter;
	}
	return pEnemie;
}

bool CPlanePatrolState::FindNewGroundEnemie( const CVec2 &vPoint, const float fRadius )
{
	pShootEstimator->Reset( 0, true, 0 );
	for ( CUnitsIter<1,3> iter( pPlane->GetParty(), EDI_ENEMY, vPoint, fRadius ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit *pUnit = *iter;
		if ( pUnit->IsRefValid() && pUnit->IsAlive() && pUnit->IsVisible( pPlane->GetParty()) && CanAttackEnemy( pUnit ) )
			pShootEstimator->AddUnit( pUnit );
	}

	enemie.SetUnitEnemy( pShootEstimator->GetBestUnit() );
	if ( !enemie.IsValid() )
	{
		pShootEstimator->CalcBestBuilding();
		enemie.SetBuildingEnemy( pShootEstimator->GetBestBuilding() );
	}

	return enemie.IsValid();
}

void CPlanePatrolState::DisableBombAbility()
{
	CPtr<SAISpecialAbilityUpdate> pUpdate = new SAISpecialAbilityUpdate;

	//CRAP{ FAST BOMBS
	pUpdate->info.nAbilityType = NDb::ABILITY_DROP_BOMBS;
	//CRAP}
	pUpdate->info.state = EASS_DISABLE;
	pUpdate->info.fCurValue = 0.0f;
	pUpdate->info.nObjUniqueID = pPlane->GetUniqueId();

	updater.AddUpdate( pUpdate, ACTION_NOTIFY_SPECIAL_ABLITY, pPlane, -1 );
}

bool CPlanePatrolState::CanAttackEnemy( CAIUnit *pEnemy ) const
{
	return pPlane != pEnemy && ( pPlane->GetStats()->etype != RPG_TYPE_AVIA_ATTACK || pEnemy->GetStats()->etype != RPG_TYPE_AVIA_BOMBER );
}

CAviation * CPlanePatrolState::FindNewEnemyPlane( const CVec2 &vPoint, const float fRadius ) const
{
	CPtr<CAviation> pEnemie;
	CPlanesIter planes;
	pEnemie = 0;
	
	float Dist = 0;
	const SMechUnitRPGStats * pStats = checked_cast<const SMechUnitRPGStats*>(pPlane->GetStats());
	while ( !planes.IsFinished() )
	{
		if ( CanAttackEnemy( *planes ) )
		{
			//found enemie that is near and can be hit by plane's weapon
			float curDist = fabs2( vPoint - (*planes)->GetCenterPlain() );

			CVec2 ce((*planes)->GetCenterPlain());
			float distFromPatrolpoint = fabs2(  ce - GetPoint() );
  		
			if ( //distFromPatrolpoint < sqr( fRadius ) && //near guard point
					 theDipl.GetDiplStatus( pPlane->GetPlayer(), (*planes)->GetPlayer() ) == EDI_ENEMY &&//enemie
					 ( !pEnemie || Dist < curDist ) //nearer then former or new enemie
				 )
			{
				int nGuns = pPlane->GetNGuns();
				bool bCanBreak = false;
				for ( int i = 0; i< nGuns && !bCanBreak; ++i )
				{
					bCanBreak = pPlane->GetGun( i )->CanBreakArmor( *planes ); //can damage
				}
				if ( bCanBreak )
				{
					Dist = curDist;
					pEnemie = (*planes);
				}
			}
		}
		planes.Iterate();
	}
	return pEnemie;
}

bool CPlanePatrolState::TargetScan()
{
	bool bFoundEnemy = false;

	if ( curTime >= timeNextScan )
	{
		timeNextScan = curTime + SConsts::PLANE_TARGET_SCAN_PERIOD;
		const NDb::EUnitRPGType eType = pPlane->GetStats()->etype;
		// guard state
		// if fighter - find target (aviation)
		CAviation * pEnemyPlane = 0;
		if ( eType == RPG_TYPE_AVIA_FIGHTER || eType == RPG_TYPE_AVIA_ATTACK )
		{
			const CPlanePreferences &pref = pPlane->GetPreferencesB2();
			pEnemyPlane = FindNewEnemyPlane( pPlane->GetCenterPlain(), SConsts::PLANE_GUARD_STATE_RADIUS + pref.GetR( pref.GetMaxSpeed() ));
			if ( pEnemyPlane )
				bFoundEnemy = true;
		}
		// if ground attack - find ground target or enemy plane
		if ( !bFoundEnemy && eType == RPG_TYPE_AVIA_ATTACK )
		{
			const CPlanePreferences &pref = pPlane->GetPreferencesB2();
			if ( FindNewGroundEnemie( pPlane->GetCenterPlain(), SConsts::PLANE_GUARD_STATE_RADIUS + pref.GetR( pref.GetMaxSpeed() )) )
				bFoundEnemy = true;
		}

		if ( bFoundEnemy )
		{
			if ( eType == RPG_TYPE_AVIA_ATTACK )
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_MOVE_ATTACKPLANE_SETPOINT, GetPoint() ), pPlane );
			else if ( eType == RPG_TYPE_AVIA_FIGHTER )
				theGroupLogic.InsertUnitCommand( SAIUnitCmd( ACTION_COMMAND_ATTACK_UNIT, pEnemyPlane->GetUniqueId() ), pPlane );
			TryInterruptState( 0 );
		}
	}
	return bFoundEnemy;
}

void CPlanePatrolState::AddPoint( const CVec2 &vAddPoint )
{
	vPatrolPoints.push_back( vAddPoint + pPlane->GetGroupShift() );
}

void CPlanePatrolState::SetPoint( const CVec2 &_vPoint )
{
	InternalSetPoint( _vPoint, false );
}

void CPlanePatrolState::InternalSetPoint( const CVec2 &_vPoint, const bool _bAttackState )
{
	vPatrolPoints.clear();
	vPatrolPoints.push_back( _vPoint );
}

void CPlanePatrolState::AdvancePlane()
{
	CPlanesFormation *pFormation = pPlane->GetPlanesFormation();
	pFormation->Advance( SConsts::AI_SEGMENT_DURATION );

	pPlane->DecFuel( bEconomyMode );
	if ( pFormation->IsManuverFinished() )
	{
		CVec3 vSpeed( pPlane->GetSpeedB2() );
		Normalize( &vSpeed );
		pFormation->CreatePointManuver( pPlane->GetPosB2() + 1000 * CVec3(vSpeed.x, vSpeed.y, 0.0f), true );
		bEconomyMode = true;
	}
}

bool CPlanePatrolState::PathSegment()
{
	AdvancePlane();
	bool bReturnHome = false;
	if ( !pPlane->GetFuel() || theWeather.IsActive() && pPlane->GetUnitAbilityDesc( NDb::ABILITY_MASTER_PILOT ) == 0 )
	{
		Escape();
		return true;
	}
	return false;
}

void CPlanePatrolState::InitPathToEnemyPlane( class CPlanesFormation *_pEnemy )
{
	CPlanesFormation * pFormation = pPlane->GetPlanesFormation();
	if ( pFormation )
	{
		pFormation->CreateManuver( _pEnemy );
		bEconomyMode = false;
	}
}

void CPlanePatrolState::InitPathToPoint( const CVec3 &vPoint, const bool _bEconomyMode, const bool _bToHorisontal )
{
	CPlanesFormation * pFormation = pPlane->GetPlanesFormation();
	if ( pFormation )
	{
		const float fSpeed = fabs( pFormation->GetSpeedB2() );
		const float fMinPathLength = 2.1f * pFormation->GetPreferencesB2().GetR( fSpeed ); 
		const bool bTooShortPath = fabs( pFormation->GetPosB2() - vPoint ) < fMinPathLength;
		if ( !bTooShortPath )
			pFormation->CreatePointManuver( vPoint, _bToHorisontal );
		else
		{
			CVec3 vForwardDir( pFormation->GetSpeedB2() );
			Normalize( &vForwardDir );
			pFormation->CreatePointManuver( vPoint + vForwardDir * fMinPathLength, _bToHorisontal );
		}
		bEconomyMode = _bEconomyMode;
	}
}


//*										  CPlaneRestState *


CPlaneRestState::CPlaneRestState( CAviation *pUnit )
: CPlanePatrolState( pUnit ), CPlaneDeffensiveFire( pUnit )
{
	if ( pUnit->GetPlanesFormation() )
	{
		// init triangle to fly 
		const CPlanePreferences &pref = pUnit->GetPreferencesB2();
		CVec3 vSpeed3D( pUnit->GetSpeedB2() );
		CVec2 vSpeed( vSpeed3D.x, vSpeed3D.y );
		if ( VNULL2 == vSpeed )
			vSpeed = GetVectorByDirection( NRandom::Random(65535) );
		else
			Normalize( &vSpeed );
		
		CVec2 vPos( pUnit->GetPosB2().x, pUnit->GetPosB2().y );
		// ensure thant vPos2 is inside map
		vPos.x = Clamp( vPos.x, 0.0f, 1.0f * GetAIMap()->GetSizeX() * SConsts::TILE_SIZE );
		vPos.y = Clamp( vPos.y, 0.0f, 1.0f * GetAIMap()->GetSizeY() * SConsts::TILE_SIZE );

		const float fTriangleLength = pref.GetR( pref.GetMaxSpeed() );
		const CVec2 vPoint1( vPos + vSpeed * fTriangleLength );
		const int nRot = NRandom::Random(0,1) == 0 ? 1 : -1;
		const CVec2 vPoint2( vPos + fTriangleLength * ( ( sqrt(3.0f) * 0.5f ) * nRot * CVec2( vSpeed.y, -vSpeed.x ) - 0.5f * vSpeed ) );
		const CVec2 vPoint3( vPos + fTriangleLength * ( - ( sqrt(3.0f) * 0.5f ) * nRot * CVec2( vSpeed.y, -vSpeed.x ) - 0.5f * vSpeed ) );

		AddPoint( vPoint2 );
		AddPoint( vPoint3 );
		AddPoint( vPoint1 );
#ifndef _FINALRELEASE
		if ( NGlobal::GetVar( "plane_rest_state_markers", 0 ) )
		{
				CSegment segm;

			segm.p1 = vPoint1;
			segm.p2 = vPoint2;
			segm.dir = segm.p2 - segm.p1;
			DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
			segm.p1 = vPoint2;
			segm.p2 = vPoint3;
			segm.dir = segm.p2 - segm.p1;
			DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
			segm.p1 = vPoint3;
			segm.p2 = vPoint1;
			segm.dir = segm.p2 - segm.p1;
			DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, 2, NDebugInfo::WHITE );
		}
#endif
		// let plane finish current manuver
		// InitPathToPoint( CVec3( GetPoint(), pPlane->GetPreferencesB2().GetPatrolHeight() ), true, true );
	}
}

void CPlaneRestState::Segment()
{
	CPlanesFormation *pFormation = pPlane->GetPlanesFormation();

	if ( pFormation )
	{
		TargetScan();

		if ( pFormation->IsManuverFinished() )
		{
			ToNextPoint();
			InitPathToPoint( CVec3( GetPoint(), pPlane->GetPreferencesB2().GetPatrolHeight() ), true, true );
		}
		PathSegment();
	}
 	AnalyzeBSU();
}

ETryStateInterruptResult CPlaneRestState::TryInterruptState( class CAICommand *pCommand )
{
	
	pPlane->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}


//*******************************************************************
//*										  CPlaneBombState															*
//*******************************************************************

CPlaneBombState::CPlaneBombState( CAviation *_pPlane, const CVec2 &vPoint )
: CPlanePatrolState( _pPlane ), CPlaneDeffensiveFire( _pPlane ),
	eState( ECBS_ESTIMATE ), fInitialHeight ( _pPlane->GetCenter().z ), 
	fStartAttackDist( 0.0f ), bHaveBombs( true )
{
	bHaveBombs = IsBombsPresent();
	SetPoint( vPoint + _pPlane->GetGroupShift() );
	//TODO{ check bHaveBombs and refuse to attack if no bombs
	//TODO}
}

float CPlaneBombState::RecalcStartAttack() const
{
	float fStartAttackDist = 0.0f;
	// определять по скорости бомбера и скорости падения бомбы
	CVec3 vSpeed3( pPlane->GetSpeedB2() );
	const CVec3 vCenter( pPlane->GetPosB2() );
	const float fTimeToFly = CBombBallisticTraj::GetTimeOfFly( vCenter.z - GetHeights()->GetZ( vCenter ), vSpeed3.z );
	const CVec3 vOffset =  CBombBallisticTraj::CalcTrajectoryFinish( pPlane->GetPosB2(), vSpeed3, VNULL2, fTimeToFly );
	fStartAttackDist += fabs( vOffset.x - vCenter.x, vOffset.y - vCenter.y );

	// вычислить поправку на длину очереди (чтобы в цель попала середина очереди)
	// считаем, что все очереди одинаковой длины и у всех gun одинаковое число патронов
	const int nGuns = pPlane->GetNGuns();
	for ( int i = 0; i < nGuns; ++i )
	{
		CBasicGun *pGun = pPlane->GetGun( i );
		if ( pGun->GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB )
		{
			const SWeaponRPGStats * pStats = pGun->GetWeapon();
			fStartAttackDist += pPlane->GetSpeed() * 
				( 
				pStats->fAimingTime + 
				Min( pStats->nAmmoPerBurst, pGun->GetNAmmo() ) * pGun->GetFireRate()
				) / 2.0f;

			break;
		}
	}
	return fStartAttackDist;
}

void CPlaneBombState::Segment()
{
	switch ( eState )
	{
	case ECBS_ESTIMATE:
		fStartAttackDist = RecalcStartAttack();
		eState = ECBS_GAIN_DISTANCE;

		break;
	case ECBS_GAIN_DISTANCE:
		// если расстояние до точки бомбометания больше 2 радиусов поворота самолета
		// + расстояние начала бомбометания - то начать заход на цель, иначе удалиться от цели.
		{
			const SMechUnitRPGStats * pStats = checked_cast<const SMechUnitRPGStats *>( pPlane->GetStats() );
			const CPlanePreferences &pref = pPlane->GetPreferencesB2();
			fStartAttackDist = RecalcStartAttack();
			if ( fabs2( pPlane->GetCenterPlain() - GetPoint() ) > sqr( fStartAttackDist ) )
			{
				fStartAttackDist = RecalcStartAttack();
				InitPathToPoint( CVec3( GetPoint(), fInitialHeight ), false, true );
				eState = ECBS_APPROACH;
			}
		}
		
		break;
	case ECBS_APPROACH:
		{
			CVec2 vPlaneCenter = pPlane->GetCenterPlain();
			fStartAttackDist = RecalcStartAttack();
			const float fDist2 = fabs2( vPlaneCenter - GetPoint() );

			if ( fDist2 <= sqr( fStartAttackDist ) )
				eState = ECBS_ATTACK_HORISONTAL;
			else if ( pPlane->GetPlanesFormation()->IsManuverFinished() )
			{
				const float fMinPathLength2 = sqr( 2.1f * pPlane->GetPreferencesB2().GetR( pPlane->GetSpeed() ) ); 
				if ( fDist2 < fMinPathLength2 )
					eState = ECBS_ATTACK_HORISONTAL;
				else
				{
					eState = ECBS_GAIN_DISTANCE;
					fStartAttackDist = RecalcStartAttack();
				}
			}
		}		

		break;
	case ECBS_ATTACK_HORISONTAL:
		{
			int nGun = pPlane->GetNGuns();
			for ( int i = 0; i < nGun; ++i )
			{
				CBasicGun *pGun = pPlane->GetGun( i );
				if ( pGun->GetNAmmo() != 0 &&
						 pGun->GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB &&
						 !pGun->IsFiring() && 0 == pGun->GetRestTimeOfRelax() )
				{
					pGun->StartPointBurst( pPlane->GetCenter(), false );
				}
			}
			eState = ECBS_AIM_TO_NEXT_POINT;
		}
		
		break;
	case ECBS_AIM_TO_NEXT_POINT:
		{
			bool bFiring = false;
			const int nGun = pPlane->GetNGuns();
			for ( int i = 0; i < nGun; ++i )
			{
				CBasicGun *pGun = pPlane->GetGun( i );
				if ( NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB == pGun->GetShell().etrajectory && pGun->IsFiring() )
				{
					bFiring = true;
					break;
				}
			}
			if ( !bFiring )
				eState = ECBS_AIM_TO_NEXT_POINT_2;
			
			bHaveBombs = IsBombsPresent();
			if ( !bHaveBombs )
				DisableBombAbility();
		}

		break;
	case ECBS_AIM_TO_NEXT_POINT_2:
		{
			if ( bHaveBombs )
				eState = ECBS_GAIN_DISTANCE;
			else if ( pPlane->GetStats()->etype == RPG_TYPE_AVIA_BOMBER && pPlane->GetNextCommand() == 0 )
				Escape();
			else
				TryInterruptState( 0 );
		}

		break;
	}

	bHaveBombs = IsBombsPresent();

	PathSegment();
	AnalyzeBSU();
}

ETryStateInterruptResult CPlaneBombState::TryInterruptState( class CAICommand *pCommand )
{
	
	pPlane->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}


//*******************************************************************
//*										  CPlaneParaDropState													*
//*******************************************************************

CPlaneParaDropState::CPlaneParaDropState ( CAviation *pPlane, const enum EActionLeaveParam param, const CVec2 &vLandPoint, CFormation *_pSquad ) 
: CPlanePatrolState( pPlane ), 
	CPlaneDeffensiveFire( pPlane ), nDroppingSoldier( 0 ),
	vLastDrop( VNULL2 ),
	pSquad( _pSquad ),
	bDrop1Squad( IsValidObj( _pSquad ) )
{
	if ( param == ALP_POSITION_VALID )
	{
		eState = PPDS_ESTIMATE;
		AddPoint( vLandPoint );
	}
	else
		eState = PPDS_PREPARE_TO_DROP;
}

bool CPlaneParaDropState::CanDrop( const CVec2 & point )
{
	if ( !GetAIMap()->IsTileInside( AICellsTiles::GetTile( point ) ) )
		return true;
	STerrainModeSetter terrainMode( ELM_ALL, GetTerrain() );

	const SVector centerTile = AICellsTiles::GetTile( point );
	if ( GetAIMap()->IsTileInside( centerTile ) && !GetTerrain()->IsLocked( centerTile, EAC_HUMAN ) )
		return true;
	else
	{
		for ( int i = centerTile.x-SConsts::PARADROP_SPRED; i< centerTile.x+SConsts::PARADROP_SPRED; ++i )
			for ( int j = centerTile.y-SConsts::PARADROP_SPRED; j< centerTile.y+SConsts::PARADROP_SPRED; ++j )
				if ( GetAIMap()->IsTileInside( i, j ) && !GetTerrain()->IsLocked( i, j, EAC_HUMAN ) )
					return true;
	}
	
	//fall to locked tile ( death will occur )
	return false;
}

void CPlaneParaDropState::Segment()
{
	bool bRepeat = true;
	while ( bRepeat )
	{
		bRepeat = false;

		if ( pPlane->GetNPassengers() == 0 )
			Escape();
		switch( eState )
		{
		case PPDS_ESTIMATE:
			InitPathToPoint( CVec3( GetPoint(), pPlane->GetCenter().z ), true, true );
			eState = PPDS_APPROACHNIG;

			break;
		case PPDS_APPROACHNIG:
			if ( fabs( GetPoint() - pPlane->GetCenterPlain() ) < SConsts::TILE_SIZE * 5  )
				eState = PPDS_PREPARE_TO_DROP;

			break;
		case PPDS_PREPARE_TO_DROP:
			{
				CVec3 where( pPlane->GetCenter() );

				if ( pPlane->GetPlayer() == theDipl.GetNeutralPlayer() || pPlane->GetNPassengers() == 0 )
					TryInterruptState( 0 );
				else 
				{
					if ( !IsValidObj( pSquad ) )
						pSquad = pPlane->GetPassenger( 0 )->GetFormation();
					vLastDrop = pPlane->GetCenterPlain(); //vLastDrop = CVec2( -1,-1 );
					eState = PPDS_DROPPING;
					pPlane->SendAcknowledgement( ACK_PLANE_REACH_POINT_START_ATTACK, true );
					NI_ASSERT( IsValid( pSquad ), "no squad to drop" );
					theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_PARACHUTE), pSquad, false );
					bRepeat = true;
				}
			}


			break;
		case PPDS_TO_NEXT_SQUAD:
			if ( bDrop1Squad )
				TryInterruptState( 0 );
			else if ( pPlane->GetNPassengers() == 0 )
			{
				Escape();
			}
			else
			{
				pSquad = 0;
				eState = PPDS_PREPARE_TO_DROP;
				bRepeat = true;
			}

			break;
		case PPDS_DROPPING:
			if ( fabs2 ( vLastDrop - pPlane->GetCenterPlain() ) > sqr(SConsts::PLANE_PARADROP_INTERVAL) )
			{
				// найти солдата, который сидит в самолете
				CPtr<CSoldier> pDropper = 0;
				for ( int i = 0; i < pSquad->Size(); ++i )
				{
					if ( (*pSquad)[i]->IsRefValid() && (*pSquad)[i]->IsAlive() &&  (*pSquad)[i]->IsInSolidPlace() )
					{
						pDropper = (*pSquad)[i];
						break;
					}
				}
				if ( !pDropper )
				{
					eState = PPDS_TO_NEXT_SQUAD;
					bRepeat = true;
				}
				else
				{
					bRepeat = (nDroppingSoldier % 2) == 0;
					if ( !bRepeat )
						vLastDrop = pPlane->GetCenterPlain() ;
					// случайный разброс парашютистов.
					
					const CVec2 vDropPoint = vLastDrop + 2.0f * ( nDroppingSoldier % 2 - 0.5f ) * 
						NRandom::Random( SConsts::PLANE_PARADROP_INTERVAL_PERP_MIN, SConsts::PLANE_PARADROP_INTERVAL_PERP_MAX ) *
						GetVectorByDirection( pPlane->GetDirection() + 65535 / 4 );
					// вычислить возможно ли приземление где-нить.
					bool bSafeLanding = CanDrop( vDropPoint );
					//
					if ( bSafeLanding )
					{
						// выбросить этого солдата
						pDropper->SetFree();
						DRAW_WHITE_CROSS( CVec3( vDropPoint, pPlane->GetCenter().z ) );
						pDropper->SetCenter( CVec3( vDropPoint, pPlane->GetCenter().z ), false );
						pDropper->SetSelectable( false, true );
						pPlane->DelPassenger( pDropper );
						pDropper->GetState()->TryInterruptState( 0 );

						theGroupLogic.UnitCommand( SAIUnitCmd(ACTION_MOVE_PARACHUTE, pPlane->GetUniqueID() ), pDropper, false );
						pDropper->SetCenter( CVec3( vDropPoint, pPlane->GetCenter().z ) );
						++nDroppingSoldier;
					}
					else
						bRepeat = false;
				}
			}

			break;
		}
	}
	PathSegment();
	AnalyzeBSU();
}

ETryStateInterruptResult CPlaneParaDropState::TryInterruptState( CAICommand *pCommand )
{
	if ( !pCommand )
	{
		if ( IsValidObj( pSquad ) )
		{
			// kill undropped soldiers
			list<CSoldier*> onboardSoldiers;
			for ( int i = 0; i < pSquad->Size(); ++i )
			{
				CSoldier *pSold = (*pSquad)[i];
				if ( pSold->IsRefValid() && pSold->IsAlive() && pSold->IsInSolidPlace() )
					onboardSoldiers.push_back( pSold );
			}
			for ( list<CSoldier*>::iterator it = onboardSoldiers.begin(); it != onboardSoldiers.end(); ++it )
				(*it)->Disappear();
		}			
	}
	else if ( IsValid( pSquad ) )
	{
		bDrop1Squad = true;
		return TSIR_YES_WAIT;
	}
	pPlane->SetCommandFinished();
	return TSIR_YES_IMMIDIATELY;
}

const CVec2 CPlaneParaDropState::GetPurposePoint() const
{
	return GetPoint();
}

//*******************************************************************
//*										  CPlaneLeaveState														*
//*******************************************************************

CPlaneLeaveState::CPlaneLeaveState( CAviation *_pPlane )
: eState( EPLS_STARTING ), 
	CPlanePatrolState( _pPlane ),
	CPlaneDeffensiveFire( _pPlane )
{ 
	pPlane->SendAcknowledgement( ACK_PLANE_LEAVING, true );

	const CVec2 vCenter( pPlane->GetCenterPlain() );
	const CVec2 vMapSize( GetAIMap()->GetSizeX() * SConsts::TILE_SIZE, GetAIMap()->GetSizeY() * SConsts::TILE_SIZE );
	const CVec2 vAwayPoint1( vCenter.x, vCenter.y < vMapSize.y - vCenter.y ? -LEAVE_DISSAPEAR_DISTANCE : vMapSize.y + LEAVE_DISSAPEAR_DISTANCE );
	const CVec2 vAwayPoint2( vCenter.x < vMapSize.x - vCenter.x ? -LEAVE_DISSAPEAR_DISTANCE : vMapSize.x + LEAVE_DISSAPEAR_DISTANCE, vCenter.y );
	const CVec2 vAwayPoint = fabs( vAwayPoint1.y - vCenter.y ) < fabs( vAwayPoint2.x - vCenter.x ) ? vAwayPoint1 : vAwayPoint2;

	InitPathToPoint( CVec3( vAwayPoint, pPlane->GetCenter().z ), true, true );
}

void CPlaneLeaveState::Segment()
{
	switch( eState )
	{
	case EPLS_STARTING:
		{
			eState = EPLS_IN_ROUTE;
		}

		break;
	case EPLS_IN_ROUTE:
		if( pPlane->GetPlanesFormation()->IsManuverFinished() )
		{
			pPlane->SetCommandFinished();
			pPlane->Disappear();
		}

		break;
	}

	CPlanesFormation *pFormation = pPlane->GetPlanesFormation();
	pFormation->Advance( SConsts::AI_SEGMENT_DURATION );

	AnalyzeBSU();
}

ETryStateInterruptResult CPlaneLeaveState::TryInterruptState( class CAICommand *pCommand )
{
	if ( 0 == pCommand )
	{
		pPlane->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	else
	{
		return TSIR_NO_COMMAND_INCOMPATIBLE;
	}
}

const CVec2 CPlaneLeaveState::GetPurposePoint() const
{
	if ( pPlane && pPlane->IsRefValid() && pPlane->IsAlive() )
	{
		CVec3 vEndPos( pPlane->GetPlanesFormation()->GetManuver()->GetEndPoint() );
		return CVec2( vEndPos.x, vEndPos.y );
	}
	else
		return CVec2( -1.0f, -1.0f );
}

//*******************************************************************
//*										  CPlaneFighterPatrolState										*
//*******************************************************************

class CAIUnit* CPlaneFighterPatrolState::GetTargetUnit() const 
{ 
	return pEnemie;
}

bool CPlaneFighterPatrolState::IsAttacksUnit() const 
{ 
	return IsValidObj( pEnemie );
}

bool CPlaneFighterPatrolState::SetNewEnemy( CAviation *pNewEnemy, CAviation *pCompareEnemy )
{
	pEnemie = pNewEnemy;
	enemie.SetUnitEnemy( pNewEnemy );
	return pNewEnemy != pCompareEnemy;
}

CPlaneFighterPatrolState::CPlaneFighterPatrolState ( CAviation *_pPlane, const CVec2 &_vPoint, CAviation *pTarget )
: CPlanePatrolState( _pPlane ), CPlaneDeffensiveFire( _pPlane ),
	eState( ECFS_GOTO_GUARDPOINT ), 
	timeLastCheck( curTime ), timeOfLastPathUpdate( 0 ), 
	bAmmoRemains( true ), nextPathUpdate( curTime )
{ 
	if ( SetNewEnemy( pTarget, 0 ) )
		SetPoint( pEnemie->GetCenterPlain() );
	else 
		SetPoint( _vPoint );

	const CPlanePreferences &pref = _pPlane->GetPreferencesB2();
	fPartolRadius = SConsts::PLANE_GUARD_STATE_RADIUS +  pref.GetR( pref.GetMaxSpeed() );
	if ( !bAmmoRemains )
	{
		//TODO{ send ack that no ammo
		//TODO}
	}
}

bool CPlaneFighterPatrolState::IsEnemyAlive( CAviation *pEnemie ) const 
{
	return IsValid( pEnemie ) && pEnemie->IsAlive() &&
		EDI_ENEMY == theDipl.GetDiplStatus( pPlane->GetPlayer(), pEnemie->GetPlayer() ) ;
}

void CPlaneFighterPatrolState::Segment()
{
	if ( pPlane->GetNextCommand() != 0 && pPlane->GetNextCommand()->ToUnitCmd().nCmdType == ACTION_COMMAND_PATROL )
	{
		FinishState();
	}
//		pPlane->SetCommandFinished();

	// if plane are too far off the map it should move to map center
	// untill it reaches map again
	if ( eState != ECFS_RETURN_TO_MAP )
	{
		CVec3 vCenter3 = pPlane->GetPosB2();
		SVector vCenter( vCenter3.x / SConsts::TILE_SIZE, vCenter3.y / SConsts::TILE_SIZE );
		SVector vSize( GetAIMap()->GetSizeX(), GetAIMap()->GetSizeY() );
		int nRadius = pPlane->GetPreferencesB2().GetR( fabs( pPlane->GetSpeedB2() ) ) / SConsts::TILE_SIZE;
		if ( vCenter.x < -nRadius || vCenter.x > vSize.x + nRadius ||
				vCenter.y < -nRadius || vCenter.y > vSize.y + nRadius )
		{
			// should return
			eState = ECFS_RETURN_TO_MAP;
			InitPathToPoint( CVec3( vSize.x / 2.0f * SConsts::TILE_SIZE, vSize.y / 2.0f * SConsts::TILE_SIZE, pPlane->GetPreferencesB2().GetPatrolHeight() ), true, true );
		}
	}

	switch ( eState )
	{
	case ECFS_RETURN_TO_MAP:
		if ( GetAIMap()->IsPointInside( pPlane->GetCenterPlain() ) )
			eState = ECFS_FIND_ENEMY_OR_NEXT_POINT;

		break;
	case ECFS_GOTO_GUARDPOINT:
		if ( pEnemie )
			eState = ECFS_ENGAGE_TARGET;
		else
		{
			InitPathToPoint( CVec3( GetPoint(), pPlane->GetPreferencesB2().GetPatrolHeight() ), true, true );
			eState = ECFS_GOING_TO_GUARDPOINT;	
		}

		break;
	case ECFS_FIND_ENEMY_OR_NEXT_POINT:
		if ( SetNewEnemy( FindNewEnemyPlane( GetPoint(), fPartolRadius ), 0 ) )
			eState = ECFS_ENGAGE_TARGET;
		else
			eState = ECFS_AIM_TO_NEXT_POINT;

		break;
	case ECFS_GOING_TO_GUARDPOINT:
  	if ( SetNewEnemy( FindNewEnemyPlane( GetPoint(), fPartolRadius ), 0 ) )
			eState = ECFS_ENGAGE_TARGET;
		else if ( pPlane->GetPlanesFormation()->IsManuverFinished() )
			eState = ECFS_AIM_TO_NEXT_POINT;

		break;
	case ECFS_AIM_TO_NEXT_POINT:
		if ( vPatrolPoints.size() == 1 )
			TryInterruptState( 0 );
		else
		{
			ToNextPoint();
			eState = ECFS_GOTO_GUARDPOINT;
		}
		
		break;
	case ECFS_ESCAPE:
		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_MOVE_PLANE_LEAVE ), pPlane, false );
		
		break;
	case ECFS_AVOID_COLLISION:
		if ( curTime > nextPathUpdate )
			eState = ECFS_FIND_ENEMY_OR_NEXT_POINT;

		break;
	case ECFS_ENGAGE_TARGET:
		if ( !IsEnemyAlive( pEnemie ) )
		{
			eState = ECFS_FIND_ENEMY_OR_NEXT_POINT;
		}
		else if ( fabs( pEnemie->GetCenterPlain() - pPlane->GetCenterPlain() ) < 
			( pEnemie->GetBoundTileRadius() + pPlane->GetBoundTileRadius() ) * 32 )
		{
			CPlanesFormation *pFormation = pPlane->GetPlanesFormation();
			const CVec3 vAdd( 0, 0, NRandom::Random( 0, 2 ) * 250 );
			CVec3 vSpeed( pPlane->GetSpeedB2() );
			Normalize( &vSpeed );
			pFormation->CreatePointManuver( vAdd + pPlane->GetPosB2() - vSpeed * 1000, false );
			nextPathUpdate = curTime + 1000;
			eState = ECFS_AVOID_COLLISION;
		}
		else if ( SetNewEnemy( FindBetterEnemiyPlane( pEnemie, GetPoint(), fPartolRadius), pEnemie ) )
		{
			TryInitPathToEnemie( true );
			eState = ECFS_ENGAGE_TARGET;
		}
		else	//enemie not killed yet
		{
			TryInitPathToEnemie( false );


			int nGun = pPlane->GetNGuns();
			for ( int i = 0; i < nGun; ++i )
			{
				CBasicGun *pGun = pPlane->GetGun( i );
				// атака только пушками
				if ( pGun->GetShell().etrajectory != NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB &&
					!pGun->IsFiring() && pGun->GetRestTimeOfRelax() == 0 && 
					pGun->CanShootWOGunTurn( pEnemie, 1 ) )
						pGun->StartPlaneBurst( pEnemie, false );
			}
		}

		break;
	}

	if ( bAmmoRemains && timeLastCheck - curTime > SConsts::TIME_QUANT )
	{
		int nGuns = pPlane->GetNGuns();
		bool bAmmoRemains = false;
		for ( int i=0; i< nGuns; ++i )
		{
			CBasicGun *pGun = pPlane->GetGun( i );
			if ( pGun->GetNAmmo() != 0 )
			{
				bAmmoRemains = true;
				break;
			}
		}
	}
	if ( PathSegment() && ECFS_ESCAPE != eState )
		eState = ECFS_ESCAPE;
	AnalyzeBSU();
}

void CPlaneFighterPatrolState::TryInitPathToEnemie( const bool _bNewEnemy )
{
	CPlanesFormation *pFormation = pPlane->GetPlanesFormation();
	if ( nextPathUpdate < curTime || (_bNewEnemy || pFormation->IsManuverFinished()) && IsValidObj( pEnemie ) )
	{
		pFormation->CreateManuver( pEnemie->GetPlanesFormation() );
		nextPathUpdate = curTime + 1000;
	}
}

ETryStateInterruptResult CPlaneFighterPatrolState::TryInterruptState( class CAICommand *pCommand )
{
	vPatrolPoints.clear();
	//pPlane->SetCommandFinished();
	FinishState();
	return TSIR_YES_IMMIDIATELY;
}

void CPlaneFighterPatrolState::FinishState()
{
	pPlane->SetCommandFinished();
	SetNewEnemy( 0, 0 );
}

//*******************************************************************
//*										  CEnemyContainer															*
//*******************************************************************

void CPlanePatrolState::CEnemyContainer::SetBuildingEnemy( CBuilding * _pBuilding )
{
	if ( pEnemy )
		pEnemy->UpdateTakenDamagePower( -fTakenDamage );
	SetUnitEnemy( 0 );
	pBuilding = _pBuilding;
}

bool CPlanePatrolState::CEnemyContainer::CanBreakTarget( const bool bBombsAllowed )
{
	if ( IsValidBuilding() )
		return true;
	if ( IsValidUnit() )
	{
		return true;
		/*const int nGuns = pOwner->GetNGuns();
		for ( int i = 0; i < nGuns; ++i )
		{

		}*/
	}
	return true;
}

void CPlanePatrolState::CEnemyContainer::SetUnitEnemy( CAIUnit *pNewEnemy )
{
	if ( pEnemy == pNewEnemy ) return;
	const bool bNeedModif = IsValidObj( pNewEnemy ) && pNewEnemy->IsAviation();
	if ( bNeedModif != bModifApplied )
	{
		const CDBPtr<NDb::SMechUnitRPGStats> pStats = dynamic_cast<const NDb::SMechUnitRPGStats *>( pOwner->GetStats() );
		if ( pStats && pStats->pGAPAirAttackModifier )
			pOwner->ApplyStatsModifier( pStats->pGAPAirAttackModifier, bNeedModif );
		bModifApplied = bNeedModif;
		//DebugTrace( "%s to unit %d due %d", bModifApplied ? "Modificators applied" : "Modificatiors cleared", pOwner->GetUniqueID(), pNewEnemy ? pNewEnemy->GetUniqueID() : 0 );
	}

	//DebugTrace( "(%d) CPlanePatrolState::CEnemyContainer::SetUnitEnemy( %d )", pOwner->GetUniqueID(), pNewEnemy ? pNewEnemy->GetUniqueID() : 0 );
	if ( pEnemy )
		pEnemy->UpdateTakenDamagePower( -fTakenDamage );
	if ( pNewEnemy )
	{
		fTakenDamage = pOwner->GetKillSpeed( pNewEnemy, DWORD(-1) );
		pNewEnemy->UpdateTakenDamagePower( fTakenDamage );
	}
	pEnemy = pNewEnemy;
}

bool CPlanePatrolState::CEnemyContainer::CanShootToTarget( class CBasicGun *pGun ) const
{
	if ( IsValidUnit() )
		return pGun->CanShootWOGunTurn( pEnemy, 1 );
	if ( IsValidBuilding() )
		return pGun->CanShootToPointWOMove( GetCenter(), GetZ() );

	return false;
}

void CPlanePatrolState::CEnemyContainer::StartBurst( class CBasicGun *pGun )
{
	if ( IsValidUnit() )
		pGun->StartEnemyBurst( GetEnemy(), false );
	if ( IsValidBuilding() )
		pGun->StartPointBurst( GetCenter(), false );
}

float CPlanePatrolState::CEnemyContainer::GetZ() const 
{
	if ( IsValidUnit() )
		return pEnemy->GetCenter().z;
	if ( IsValidBuilding() )
		return 0;
	NI_ASSERT( false, "asked invalid target about Z" );
	return 0;
}

CVec2 CPlanePatrolState::CEnemyContainer::GetCenter() const
{
	if ( IsValidUnit() )
		return pEnemy->GetCenterPlain();
	if ( IsValidBuilding() )
		return pBuilding->GetAttackCenter( pOwner->GetCenterPlain() );
	NI_ASSERT( false, "asked invalid target about attack center" );
	return VNULL2;
}

CAIUnit * CPlanePatrolState::CEnemyContainer::GetGroundEnemy()
{
	return IsValidUnit() && !pEnemy->GetStats()->IsAviation() ? pEnemy : 0;
}

CAviation * CPlanePatrolState::CEnemyContainer::GetAviationEnemy()
{
	if ( IsValidUnit() && pEnemy->GetStats()->IsAviation() )
	{
		CAviation *pAviation = static_cast_ptr<CAviation*>( pEnemy );
		return pAviation;
	}
	return 0;
}

CBuilding * CPlanePatrolState::CEnemyContainer::GetBuilding()
{
	return IsValidBuilding() ? pBuilding : 0;
}

bool CPlanePatrolState::CEnemyContainer::IsValidBuilding() const
{
	return IsValidObj( pBuilding ) &&
		EDI_ENEMY == theDipl.GetDiplStatus( pBuilding->GetPlayer(), pOwner->GetPlayer() ) ;
}

bool CPlanePatrolState::CEnemyContainer::IsValidUnit() const
{
	//CRAP{ TO FAST START
	return IsValidObj( pEnemy ) ;//&&
		//EDI_ENEMY == theDipl.GetDiplStatus( pEnemy->GetPlayer(), pOwner->GetPlayer() ) ;
	//CRAP}
}

bool CPlanePatrolState::CEnemyContainer::IsValid() const
{
	return IsValidUnit() || IsValidBuilding();
}

//*******************************************************************
//*										  CPlaneShturmovikPatrolState *
//*******************************************************************

CPlaneShturmovikPatrolState::CPlaneShturmovikPatrolState ( CAviation *_pPlane, const CVec2 &_vPoint, CAIUnit *pTarget, CBuilding *pBuilding, const bool _bMustDropBombs ) 
: CPlanePatrolState( _pPlane ),
	CPlaneDeffensiveFire( _pPlane ),
	eState( PSPS_GOTO_GUARDPOINT ), 
	timeOfLastPathUpdate( curTime ),
	pPlane( _pPlane ),
	timeLastCheck ( curTime ),
	vCurTargetPoint( -1.0f, -1.0f ),
	fStartAttackDist( 0.0f ), fFinishAttckDist( 0.0f ), fTurnRadius( 0.0f ),
	bAmmoRemains( true ),
	bMustDrop1Bomb( _bMustDropBombs ),
	bBombsDropped( false ),
	bShootedToEnemie ( false ), bFirstApproach( true ), bDiveInProgress( false )
{
	if ( IsValidObj( pTarget ) )
	{
		enemie.SetUnitEnemy( pTarget );
		SetPoint( enemie.GetCenter() );
	}
	else if ( pBuilding )
	{
		enemie.SetBuildingEnemy( pBuilding );
		SetPoint( enemie.GetCenter() );
	}
	else 
		SetPoint( _vPoint );

	if ( enemie.IsValid() && !enemie.CanBreakTarget( bMustDrop1Bomb ) )
	{
		pPlane->SendAcknowledgement( ACK_CANNOT_PIERCE );
		TryInterruptState( 0 );
	}
	const SMechUnitRPGStats * pStats = checked_cast<const SMechUnitRPGStats *>( pPlane->GetStats() );
	fTurnRadius = pStats->fTurnRadius;

	const float fVertTurnRadius = fTurnRadius;
	const float fDiveAngle = pStats->wDivingAngle * 2.0f * PI / 65535;
	NI_ASSERT( pStats->wDivingAngle != 0, StrFmt("DESIGNER'S BUG: GROUND ATTACK PLANE \"%s\" DIVING ANGLE == 0", NDb::GetResName(pStats)) );
	if ( pStats->wDivingAngle != 0 )
	{
		const float fBetta = ( PI - fDiveAngle ) / 2.0f;
		fStartAttackDist = (fVertTurnRadius / tan( fBetta )) + (pPlane->GetCenter().z / tan( fDiveAngle )) + pStats->fSpeed * 3 * SConsts::AI_SEGMENT_DURATION;

		fFinishAttckDist = SPlanesConsts::GetMinHeight() / tan( fDiveAngle );
		//TODO{ check for gun ammo and bombs and send ack if no ammo
		if ( !bAmmoRemains )
		{
		}
		//TODO}
	}
}

void CPlaneShturmovikPatrolState::OnFindEnemy()
{
	if ( enemie.IsValid() || FindNewGroundEnemie( GetPoint(), SConsts::PLANE_GUARD_STATE_RADIUS ) )
	{
		vCurTargetPoint = enemie.GetCenter();
		eState = PSPS_TURN_TO_TARGET;
		OnTurnToTarget();
	}
	else
		eState = PSPS_AIM_TO_NEXT_POINT;
}

void CPlaneShturmovikPatrolState::OnTurnToTarget()
{
	if ( !enemie.IsValid() )
	{
		eState = PSPS_GOING_TO_GUARDPOINT;
	}
	else
	{
		CVec3 vPos( enemie.GetCenter(), pPlane->GetPreferencesB2().GetPatrolHeight() );
		TryInitPathToPoint( vPos, true, true );
		eState = PSPS_TURNING_TO_TARGET;
	}
}

void CPlaneShturmovikPatrolState::Segment()
{
	if ( pPlane->GetNextCommand() != 0 && pPlane->GetNextCommand()->ToUnitCmd().nCmdType == ACTION_COMMAND_PATROL )
	{
		FinishState();
	}
//		pPlane->SetCommandFinished();

	//DEBUG{
	static hash_map<int, string> nameconv;
	if ( nameconv.empty() )
	{
		nameconv[PSPS_ESCAPE] = "PSPS_ESCAPE";
		nameconv[PSPS_GOTO_GUARDPOINT] = "PSPS_GOTO_GUARDPOINT";
		nameconv[PSPS_GOING_TO_GUARDPOINT] = "PSPS_GOING_TO_GUARDPOINT";
		nameconv[PSPS_AIM_TO_NEXT_POINT] = "PSPS_AIM_TO_NEXT_POINT";
		nameconv[PSPS_FIND_ENEMY_OR_NEXT_POINT] = "PSPS_FIND_ENEMY_OR_NEXT_POINT";
		nameconv[PSPS_APPROACHING_TARGET] = "PSPS_APPROACHING_TARGET";
		nameconv[PSPS_ENGAGING_TARGET] = "PSPS_ENGAGING_TARGET";
		nameconv[PSPS_FIRE_TO_WORLD] = "PSPS_FIRE_TO_WORLD";
		nameconv[PSPS_TURN_TO_TARGET] = "PSPS_TURN_TO_TARGET";
		nameconv[PSPS_TURNING_TO_TARGET] = "PSPS_TURNING_TO_TARGET";
		nameconv[PSPS_GAIN_HEIGHT] = "PSPS_GAIN_HEIGHT";
		nameconv[PSPS_GAINING_HEIGHT] = "PSPS_GAINING_HEIGHT";

	}
	EPlaneShturmovikPatrolState eRem = eState;
	//DEBUG}

	switch ( eState )
	{
	case PSPS_ESCAPE:
		Escape();

		break;
	case PSPS_GOTO_GUARDPOINT:
		if ( enemie.IsValid() )
			eState = PSPS_TURN_TO_TARGET;
		else
		{
			pShootEstimator->SetCurCenter( GetPoint() );
			InitPathToPoint( CVec3( GetPoint(), pPlane->GetPreferencesB2().GetPatrolHeight() ), true, true );
			eState = PSPS_GOING_TO_GUARDPOINT;
		}

		break;
	case PSPS_FIND_ENEMY_OR_NEXT_POINT:
		OnFindEnemy();
		
		break;
	case PSPS_GOING_TO_GUARDPOINT:
		if ( FindNewGroundEnemie( GetPoint(), SConsts::PLANE_GUARD_STATE_RADIUS ) )
			eState = PSPS_TURN_TO_TARGET;
		else if ( pPlane->GetPlanesFormation()->IsManuverFinished() )
			eState = PSPS_AIM_TO_NEXT_POINT;

		break;
	case PSPS_AIM_TO_NEXT_POINT:
		TryInterruptState( 0 );

		break;
	case PSPS_TURN_TO_TARGET:
		OnTurnToTarget();

		break;
	case PSPS_TURNING_TO_TARGET:
		{
			if ( !enemie.IsValid() )
			{
				eState = PSPS_FIND_ENEMY_OR_NEXT_POINT;
				break;
			}
			CVec3 vSpeed3 = pPlane->GetSpeedB2();
			CVec2 vSpeed( vSpeed3.x, vSpeed3.y );
			CVec3 vPos3( pPlane->GetPosB2() );
			CVec2 vPos( vPos3.x, vPos3.y );
			
			CVec2 vDirToTarget = enemie.GetCenter() - vPos;
			if (  DirsDifference( GetDirectionByVector( vDirToTarget ), GetDirectionByVector( vSpeed ) ) < 65535 / 8 )
			{
				TryInitPathToEnemie( true );
				eState = PSPS_ENGAGING_TARGET;
			}
			else if ( fabs( vPos - enemie.GetCenter() ) > SConsts::PLANE_GUARD_STATE_RADIUS * 2 )
				eState = PSPS_TURN_TO_TARGET;
		}

		break;
	case PSPS_APPROACH_TARGET:
		{
			eState = PSPS_APPROACHING_TARGET;
			CPlanesFormation *pFormation = pPlane->GetPlanesFormation();

			if ( CAviation *pEnemyPlane = enemie.GetAviationEnemy() )
				InitPathToEnemyPlane( pEnemyPlane->GetPlanesFormation() );
			else if ( !bFirstApproach )
			{
				TryInitPathToEnemie( true );
			}
			else if ( CAIUnit * pUnit = enemie.GetGroundEnemy() )
				pFormation->CreateManuver( pUnit->GetCenter(), pUnit->GetUniqueID() );
			else if ( CBuilding * pBuilding = enemie.GetBuilding() )
				pFormation->CreateManuver( pBuilding->GetCenter(), pBuilding->GetUniqueId() );
			else
				eState = PSPS_FIND_ENEMY_OR_NEXT_POINT;
		}

		break;
	case PSPS_APPROACHING_TARGET:
		if ( !enemie.IsValid() )
			eState = PSPS_FIND_ENEMY_OR_NEXT_POINT;
		else 
		{
			CPlanesFormation *pFormation = pPlane->GetPlanesFormation();
			if ( pFormation->IsManuverFinished() )
			{
				TryInitPathToEnemie( true );
				eState = PSPS_ENGAGING_TARGET;
				bShootedToEnemie = false;
			}
		}
		break;
	case PSPS_ENGAGING_TARGET:
		if ( !enemie.IsValid() )
		{
			const CVec2 vPoint( pPlane->GetCenterPlain() + pPlane->GetDirVector() * fStartAttackDist * 2 );
			TryInitPathToPoint( CVec3( vPoint.x, vPoint.y, pPlane->GetPreferencesB2().GetPatrolHeight() ), true, true );

			eState = PSPS_FIRE_TO_WORLD;
		}
		else 
		{
			CVec3 vSpeed3 = pPlane->GetSpeedB2();
			if ( !bDiveInProgress && vSpeed3.z < 0.0f )
			{
				updater.AddUpdate( 0, ACTION_NOTIFY_DIVEBOMBER_DIVE, pPlane, 1 );
				bDiveInProgress = true;
			}
			TryBurstAllGuns();
			if ( fabs( pPlane->GetPlanesFormation()->GetManuver()->GetEndPoint().z - enemie.GetZ() ) > SPlanesConsts::GetMinHeight() )
			{
				// too low height, maneuver was canceled by itself
				if ( !bShootedToEnemie )
					pPlane->SendAcknowledgement( NDb::ACK_NEGATIVE_NOTIFICATION );
				eState = PSPS_FIRE_TO_WORLD;
				TryBurstAllGunsToPoints();
			}
			else
			{
				// если враг живой - атаковать
				TryBurstAllGuns();
				TryInitPathToEnemie( false );
				TryBurstAllGunsToPoints();
			}
		}

		
		break;
	case PSPS_FIRE_TO_WORLD:
		{
			bFirstApproach = false;
			if ( !bMustDrop1Bomb )
				bBombsDropped = false;

			const int nGun = pPlane->GetNGuns();
			for ( int i = 0; i < nGun; ++i )
			{
				CBasicGun *pGun = pPlane->GetGun( i );
				pGun->StopFire();
			}
			const CVec3 vPos3( pPlane->GetPosB2() );
			CVec2 vDir( vPos3.x - vCurTargetPoint.x, vPos3.y - vCurTargetPoint.y );
			Normalize( &vDir );
			if ( !pPlane->GetPlanesFormation()->GetManuver()->IsToHorisontal() )
			{
				InitPathToPoint( 
					CVec3 ( 
									( SConsts::PLANE_GUARD_STATE_RADIUS + fTurnRadius * 2 ) * vDir + CVec2( vPos3.x, vPos3.y ),
									  pPlane->GetPreferencesB2().GetPatrolHeight() 
								),
					true, true );
			}
			eState = PSPS_GAINING_HEIGHT;
		}

		break;
	case PSPS_GAINING_HEIGHT:
		if ( bDiveInProgress )
		{
			bDiveInProgress = false;
			updater.AddUpdate( 0, ACTION_NOTIFY_DIVEBOMBER_DIVE, pPlane, 0 );
		}
		if ( pPlane->GetSpeedB2().z < 0 )
			TryBurstAllGunsToPoints();

		if ( pPlane->GetPlanesFormation()->IsManuverFinished() )
		{
			eState = PSPS_FIND_ENEMY_OR_NEXT_POINT;
			OnFindEnemy();
		}

		break;
	}

	//DEBUG{
	if ( eState != eRem )
	{
		CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 2, 
			StrFmt( "GunplaneSubstate =  \"%s\"", nameconv[eState] ) );
		Singleton<IStatSystem>()->UpdateEntry( "test: ", nameconv[eState], 0xffff0000 );
	}
	//DEBUG}
	AnalyzeBSU();

	if ( pPlane->GetCenter().z < SPlanesConsts::GetMinHeight() + (pPlane->GetPreferencesB2().GetPatrolHeight() - SPlanesConsts::GetMinHeight()) * SConsts::BOMB_START_HEIGHT &&
		pPlane->GetSpeedB2().z < 0.0f )
		TryDropBombs();

	// 1 bomb drop per command
	if ( bBombsDropped )
	{
		bool bFinishedDropBombs = true;
		for ( int i = 0; i < pPlane->GetNGuns(); ++i )
		{
			CBasicGun *pGun = pPlane->GetGun( i );
			if ( pGun->GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB && pGun->IsFiring() )
				bFinishedDropBombs = false;
		}
		if ( bFinishedDropBombs )
		{
			if ( !IsBombsPresent() )
				DisableBombAbility();
			TryInterruptState( 0 );
		}
	}
	if ( bAmmoRemains &&	PSPS_ESCAPE != eState && timeLastCheck - curTime > SConsts::TIME_QUANT )
	{
		const int nGuns = pPlane->GetNGuns();
		bool bAmmoRemains = false;
		for ( int i = 0; i < nGuns; ++i )
		{
			CBasicGun *pGun = pPlane->GetGun( i );
			if ( pGun->GetNAmmo() != 0 )
			{
				bAmmoRemains = true;
				break;
			}
		}
	}

	if ( PathSegment() && PSPS_ESCAPE != eState )
		eState = PSPS_ESCAPE;
}

bool CPlaneShturmovikPatrolState::IsTargetBehind( const CVec2 &vTarget ) const
{
	const CVec2 vDist = vTarget - pPlane->GetCenterPlain();
	return DirsDifference( GetDirectionByVector( vDist ), pPlane->GetFrontDirection() /* GetDirectionByVector( pPlane->GetSpeed() ) */ ) > 65535/4;
}

void CPlaneShturmovikPatrolState::TryBurstAllGunsToPoints()
{
	const CVec3 vSpeed( pPlane->GetSpeedB2() );
	if ( fabs( vSpeed.z ) < fabs( vSpeed.x, vSpeed.y ) * 0.01f && vSpeed.z < -FP_EPSILON ) 
		return;
	
	const CVec3 vPos( pPlane->GetPosB2() );
	
	CVec3 vShoot;
	GetHeights()->GetIntersectionWithTerrain( &vShoot, vPos, vPos + vSpeed * vPos.z / vSpeed.z );
	if ( GetAIMap()->IsTileInside( AICellsTiles::GetTile( vShoot.x, vShoot.y ) ) )
	{
		const float fRange = fabs( vPos - vShoot );
		const int nGun = pPlane->GetNGuns();
		for ( int i = 0; i < nGun; ++i )
		{
			CBasicGun *pGun = pPlane->GetGun( i );
			if (	pGun->GetShell().etrajectory != NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB && !pGun->IsFiring() &&
						0 == pGun->GetRestTimeOfRelax() && !pGun->GetTurret() && fRange <= pGun->GetFireRangeMax() )
			{
				//CRAP{ UNTILL 2D SHOOTING
				pGun->StartPointBurst( CVec2(vShoot.x, vShoot.y ), true );
				//CRAP}
			}
		}
	}
}

void CPlaneShturmovikPatrolState::TryDropBombs()
{
	const CVec3 vSpeed3 ( pPlane->GetSpeedB2() );
	if (  enemie.IsValid() &&
				( 
					bMustDrop1Bomb && !bBombsDropped || 
					( pPlane->IsBombsAutocast() || theDipl.IsAIPlayer( pPlane->GetPlayer() ) ) 
				)
	   )
	{
		const int nGun = pPlane->GetNGuns();
		for ( int i = 0; i < nGun; ++i )
		{
			CBasicGun *pGun = pPlane->GetGun( i );
			if ( pGun->GetShell().etrajectory == NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB )
			{
				// атака бомбами.
				if ( !pGun->IsFiring() && pGun->GetRestTimeOfRelax() == 0 && pGun->GetNAmmo() != 0 )
				{
					const CVec3 vSpeed3 ( pPlane->GetSpeedB2() );
					const CVec3 vCurPoint3( pPlane->GetPosB2() );
					const CVec2 vCurPoint2( vCurPoint3.x, vCurPoint3.y );
					const float fTimeToFly = CBombBallisticTraj::GetTimeOfFly( pPlane->GetZ() - GetHeights()->GetZ( pPlane->GetCenterPlain() ), vSpeed3.z );
					const CVec3 vTrajFinish( CBombBallisticTraj::CalcTrajectoryFinish( vCurPoint3, vSpeed3, VNULL2, fTimeToFly ) );
					const float fDisp = pGun->GetDispersion() * pPlane->GetCenter().z / pGun->GetFireRangeMax();

					if ( fabs( CVec2( vTrajFinish.x, vTrajFinish.y ) - enemie.GetCenter() ) < fDisp * 2.0f )
					{
						pGun->StartPointBurst( vTrajFinish, false );
						bBombsDropped = true;
					}
				}
			}
		}
	}
}

void CPlaneShturmovikPatrolState::TryBurstAllGuns()
{
	// do not burst durin gain height
	const CVec3 vSpeed( pPlane->GetSpeedB2() );
	if ( vSpeed.z >= 0 ) 
		return;

	const int nGun = pPlane->GetNGuns();
	for ( int i = 0; i < nGun; ++i )
	{
		CBasicGun *pGun = pPlane->GetGun( i );
		if ( pGun->GetShell().etrajectory != NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB  && 
			   !pGun->IsFiring() && pGun->GetRestTimeOfRelax() == 0 && 
				 enemie.CanShootToTarget( pGun ) )
		{
			enemie.StartBurst( pGun );
			bShootedToEnemie = true;
		}
	}
}

CAIUnit *CPlaneShturmovikPatrolState::FindEnemyInFiringSector()
{
	const CVec3 vCurPoint( pPlane->GetPosB2() );

	//гнутость ствола найти по честному
	const int nGuns = pPlane->GetNGuns();
	float fBentRange = 0;
	for ( int i = 0; i < nGuns; ++i )
	{
		CBasicGun * pGun = pPlane->GetGun( i );
		if ( pGun->GetNAmmo() != 0 && pGun->GetShell().etrajectory != NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB )
			fBentRange = Max( fBentRange, float(pGun->GetWeapon()->wDeltaAngle * 2.0f * PI / 65535) );
	}

	const CVec2 vBentRange( GetVectorByDirection( fBentRange + 65535 * 3 / 4 ) );
	pShootEstimator->Reset( enemie.GetGroundEnemy(), true, 0 );
	if ( enemie.GetAviationEnemy() )
		pShootEstimator->AddUnit( enemie.GetAviationEnemy() );
	for ( CUnitsIter<1,3> iter( pPlane->GetParty(), EDI_ENEMY, GetPoint(), SConsts::PLANE_GUARD_STATE_RADIUS ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit * pUnit = *iter;
		if ( pUnit->IsRefValid() && pUnit->IsAlive() && pUnit->IsVisible( pPlane->GetParty()) && CanAttackEnemy( pUnit ) )
			pShootEstimator->AddUnit( pUnit );
	}
	return pShootEstimator->GetBestUnit();
}

CAIUnit* CPlaneShturmovikPatrolState::FindEnemyInPossibleDiveSector() 
{
	CVec3 vSpeed = pPlane->GetSpeedB2();
	Normalize( &vSpeed );

	const CVec3 vCenter( pPlane->GetPosB2() );
	const float fMinPossibleDivePoint( vCenter.z / pPlane->GetPreferencesB2().GetPatrolHeight() * fStartAttackDist );

	//гнутость и максимальную дальнобойность найти 
	const int nGuns = pPlane->GetNGuns();
	float fGnutost = 0;
	float fMaxRange = 0;
	for ( int i = 0; i < nGuns; ++i )
	{
		CBasicGun *pGun = pPlane->GetGun( i );
		if ( pGun->GetNAmmo() != 0 )
		{
			if ( pGun->GetShell().etrajectory != NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB )
			{
				fGnutost = Max( fGnutost, float(pGun->GetWeapon()->wDeltaAngle * 2.0f * PI / 65535) );
				fMaxRange = Max( fMaxRange, pGun->GetFireRangeMax() );
			}
		}
	}

	const float fMaxPossibleDivePoint( fMaxRange + (pPlane->GetCenter().z -SPlanesConsts::GetMinHeight())/ pPlane->GetPreferencesB2().GetPatrolHeight() * fStartAttackDist );
	const float fAvePossibleDivePoint( (fMaxPossibleDivePoint + fMinPossibleDivePoint)*0.5f );
	
	const CVec3 vAimCenter( vCenter + vSpeed * fAvePossibleDivePoint );

	pShootEstimator->Reset( enemie.GetGroundEnemy(), true, 0 );
	if ( enemie.GetAviationEnemy() )
		pShootEstimator->AddUnit( enemie.GetAviationEnemy() );
	for ( CUnitsIter<1,3> iter( pPlane->GetParty(), EDI_ENEMY, GetPoint(), SConsts::PLANE_GUARD_STATE_RADIUS ); !iter.IsFinished(); iter.Iterate() )
	{
		CAIUnit * pUnit = *iter;
		if ( pUnit->IsRefValid() && pUnit->IsAlive() && pUnit->IsVisible( pPlane->GetParty()) && CanAttackEnemy( pUnit ) )
			pShootEstimator->AddUnit( pUnit );
	}
	return pShootEstimator->GetBestUnit();
}

void CPlaneShturmovikPatrolState::TryInitPathToEnemie( const bool bForceNewPath )
{
	CPlanesFormation *pFormation = pPlane->GetPlanesFormation();

	if ( pFormation && ( pFormation->IsManuverFinished() || bForceNewPath ) )
	{
		if ( CAviation *pEnemyPlane = enemie.GetAviationEnemy() )
			InitPathToEnemyPlane( pEnemyPlane->GetPlanesFormation() );
		else 
			InitPathToPoint( CVec3( enemie.GetCenter(), enemie.GetZ() ), false, false );
	}
}

void CPlaneShturmovikPatrolState::TryInitPathToPoint( const CVec3 &_vPos, const bool _bNewPoint, const bool _bToHorisontal )
{
	CPlanesFormation *pFormation = pPlane->GetPlanesFormation();
	if ( _bNewPoint || pFormation->IsManuverFinished() )
		InitPathToPoint( _vPos, false, _bToHorisontal );
}


ETryStateInterruptResult CPlaneShturmovikPatrolState::TryInterruptState( class CAICommand *pCommand )
{
	vPatrolPoints.clear();
	pShootEstimator = 0;
	//pPlane->SetCommandFinished();
	FinishState();
	return TSIR_YES_IMMIDIATELY;
}

void CPlaneShturmovikPatrolState::FinishState()
{
	pPlane->SetCommandFinished();
	enemie.SetUnitEnemy( 0 );
}

//*******************************************************************
//*										  CPlaneFlyDeadState													*
//*******************************************************************

CPlaneFlyDeadState::CPlaneFlyDeadState ( CAviation *_pPlane )
: eState( EPDS_START_DIVE ), timeStart( curTime ), bExplodeInstantly( true ),
	fHeight( 0.0f ), bFatality( false ), CPlanePatrolState( _pPlane ), bGroundCrash( false )
{
	const SMechUnitRPGStats * pStats = checked_cast<const SMechUnitRPGStats*>( pPlane->GetStats() );
	bFatality = pStats->pEffectFatality != 0;
	if ( bFatality )
		bExplodeInstantly = NRandom::Random( 1 );
	if ( !bExplodeInstantly )
		timeStart = curTime + NRandom::Random( 0, SConsts::DIVE_BEFORE_EXPLODE_TIME );

	const NDb::SAIGameConsts * pConsts = Singleton<IAILogic>()->GetAIConsts(); 
	bGroundCrash = pPlane->GetStats()->vAABBHalfSize.y < pConsts->nGroundCrashPlaneSize;
	const CPlanePreferences &pref = pPlane->GetPreferencesB2();
	CPlanesFormation *pFormation = pPlane->GetPlanesFormation();
	if ( bGroundCrash )
		pFormation->SetCanViolateHeghtLimits();
	fHeight = bGroundCrash ? -500.0f : Max( SPlanesConsts::GetMinHeight(), pPlane->GetCenter().z * 0.8f );
	pFormation->CreatePointManuver( CVec3( pPlane->GetCenterPlain() + pPlane->GetDirectionVector() * pref.GetR( fabs( pPlane->GetSpeedB2() ) ) * 2, fHeight), bGroundCrash || bFatality ? false : true  );

	deadZone.Init();
}

void CPlaneFlyDeadState::CDeadZone::Init() 
{
	fMaxX = GetAIMap()->GetSizeX() * SConsts::TILE_SIZE + 3000;
	fMaxY = GetAIMap()->GetSizeY() * SConsts::TILE_SIZE + 3000;
	fMinX = -3000;
	fMinY = -3000;
}

void CPlaneFlyDeadState::CDeadZone::AdjustEscapePoint( CVec2 * pPoint )
{
	const float fXMaxDiff = fabs(pPoint->x - fMaxX);
	const float fXMinDiff = fabs(pPoint->x - fMinX);

	const float fYMaxDiff = fabs(pPoint->y - fMaxY);
	const float fYMinDiff = fabs(pPoint->y - fMinY);
	
	const float fXDiff = Min( fXMaxDiff, fXMinDiff );
	const float fYDiff = Min( fYMaxDiff, fYMinDiff );
	if ( fXDiff < fYDiff )
	{
		if ( fXMaxDiff > fXMinDiff )
			pPoint->x = fMinX;
		else
			pPoint->x = fMaxX;
	}
	else
	{
		if ( fYMaxDiff > fYMinDiff )
			pPoint->y = fMinY;
		else
			pPoint->y = fMaxY;
	}
}

bool CPlaneFlyDeadState::CDeadZone::IsInZone( const CVec2 &vPoint )
{
	return vPoint.x < fMinX || vPoint.x > fMaxX || vPoint.y < fMinY || vPoint.y > fMaxY;
}

void CPlaneFlyDeadState::InitPathToNearestPoint()
{
	const CVec2 vDirVector( pPlane->GetDirVector() );

	CVec2 vPoint( pPlane->GetCenterPlain() );
	deadZone.AdjustEscapePoint( &vPoint );

	CPlanesFormation *pFormation = pPlane->GetPlanesFormation();
	if ( pFormation )
		pFormation->CreatePointManuver( CVec3(vPoint,Max(SPlanesConsts::GetMinHeight(),pPlane->GetCenter().z * 0.8f)), true );
}

void CPlaneFlyDeadState::Segment()
{
	switch( eState )
	{
	case EPDS_START_DIVE:
		{
			if ( bGroundCrash )
				eState = EPDS_DIVE;
			else if ( bFatality )
			{
				if ( bExplodeInstantly || timeStart < curTime )
				{
					updater.AddUpdate( 0, ACTION_NOTIFY_DEADPLANE, pPlane, 0 );
					timeStart = curTime + SConsts::DIVE_AFTER_EXPLODE_TIME;
					eState = EPDS_DIVE;
				}
			}
			else
				eState = EPDS_DIVE;
		}
		break;
	case EPDS_DIVE:
		if ( bGroundCrash )
		{
			const CVec3 vPos ( pPlane->GetPosB2() );
			CVec3 vGroundPos ( GetHeights()->GetGroundPoint( vPos ) );
			CVec3 vSpeed ( pPlane->GetSpeedB2() );
			Normalize( &vSpeed );
		
			GetHeights()->GetIntersectionWithTerrain( &vGroundPos, vPos, vPos + 2000 * vSpeed );
			
			if ( vPos.z <= vGroundPos.z + pPlane->GetStats()->vAABBHalfSize.y )
			{
				pPlane->Disappear();
				const NDb::SAIGameConsts * pConsts = Singleton<IAILogic>()->GetAIConsts(); 
				if ( pConsts->pAviationGroundCrashExplosion )
				{
					theShellsStore.AddShell
						( new CInvisShell( curTime, new CBurstExpl( 0, pConsts->pAviationGroundCrashExplosion, 
						vGroundPos, VNULL3, 0, false, 1, true ), 0 ) );
				}
			}
		}
		else if ( bFatality )
		{
			if ( timeStart < curTime )
			{
				pPlane->Disappear();
				break;
			}
		}
		else 
		{
			CPlanesFormation *pFormation = pPlane->GetPlanesFormation();
			if ( pFormation )
				InitPathToNearestPoint();
			eState = EPDS_WAIT_FINISH_PATH;
		}

		break;
	case EPDS_ESTIMATE:
		eState = EPDS_WAIT_FINISH_PATH;
	
		//break; убран сознательно
	case EPDS_WAIT_FINISH_PATH:
		if ( deadZone.IsInZone( pPlane->GetCenterPlain()) )
			pPlane->Disappear();

		break;
	}

	PathSegment();

}

ETryStateInterruptResult CPlaneFlyDeadState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand || ACTION_COMMAND_DISAPPEAR == pCommand->ToUnitCmd().nCmdType )
	{
		pPlane->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	return TSIR_YES_WAIT;
}

const CVec2 CPlaneFlyDeadState::GetPurposePoint() const 
{
	if ( pPlane && pPlane->IsRefValid() && pPlane->IsAlive() )	
	{
		const CVec3 vEndPoint( pPlane->GetPlanesFormation()->GetManuver()->GetEndPoint() );
		return CVec2( vEndPoint.x, vEndPoint.y );
	}
	else
		return CVec2( -1.0f, -1.0f );
}

// CPlaneSuicideState

CPlaneSuicideState::CPlaneSuicideState( CAviation *_pPlane, const CVec2 &_vTarget )
: pPlane( _pPlane ), eState( EPSS_START ), vTarget( _vTarget ), pWeapon( 0 ), fDistToDive2( 0.0f )
{
	const int nCommonGunsCount = pPlane->GetNCommonGuns();
	for ( int i = 0; i < nCommonGunsCount; ++i )
		if ( pPlane->GetCommonGunStats( i ).pWeapon )
		{
			pWeapon = pPlane->GetCommonGunStats( i ).pWeapon;
			break;
		}
	if ( !pWeapon )
	{
		const NDb::SAIGameConsts * pConsts = Singleton<IAILogic>()->GetAIConsts(); 
		pWeapon = pConsts->pAviationGroundCrashExplosion;
	}

/*
	const CVec3 vPos3( pPlane->GetPosB2() );
	fDistToDive2 = sqr( 2.0f * ( vPos3.z - GetHeights()->GetZ( vTarget ) ) );
*/
	const CPlanePreferences &planePrefs = pPlane->GetPreferencesB2();
	fDistToDive2 = sqr( Max( planePrefs.GetR( fabs( pPlane->GetSpeedB2() ) ), g_fBombDiveDistance ) );
}

void CPlaneSuicideState::Segment()
{
	switch( eState )
	{
	case EPSS_START:
		{
			CPlanesFormation *pFormation = pPlane->GetPlanesFormation();
			const CVec3 vPos3( pPlane->GetPosB2() );
			pFormation->CreatePointManuver( CVec3( vTarget, vPos3.z ), false );
			eState = EPSS_FLY;
		}

		// нет тут break, не нужен
	case EPSS_FLY:
		{
			const CVec3 vPos3( pPlane->GetPosB2() );
			const CVec2 vPos2( vPos3.x, vPos3.y );
			if ( fabs2( vTarget - vPos2 ) < fDistToDive2 )
			{
				CPlanesFormation *pFormation = pPlane->GetPlanesFormation();
				pFormation->SetCanViolateHeghtLimits();
				pFormation->CreatePointManuver( CVec3( vTarget, GetHeights()->GetZ( vTarget ) ), false );
				eState = EPSS_DIVE;
			}
		}
		break;
	case EPSS_DIVE:
		{
			const CVec3 vPos( pPlane->GetPosB2() );
			CVec3 vGroundPos( GetHeights()->GetGroundPoint( vPos ) );
			CVec3 vSpeed( pPlane->GetSpeedB2() );
			Normalize( &vSpeed );
			GetHeights()->GetIntersectionWithTerrain( &vGroundPos, vPos, vPos + 2000 * vSpeed );

			CPlanesFormation *pFormation = pPlane->GetPlanesFormation();
			if ( vPos.z <= vGroundPos.z + pPlane->GetStats()->vAABBHalfSize.y || pFormation->IsManuverFinished() )
			{
				pPlane->Disappear();
				theShellsStore.AddShell( new CInvisShell( curTime, new CBurstExpl( 0, pWeapon, vGroundPos, VNULL3, 0, false, 1, true ), 0 ) );
			}
		}
		break;
	}

	CPlanesFormation *pFormation = pPlane->GetPlanesFormation();
	pFormation->Advance( SConsts::AI_SEGMENT_DURATION );
}

ETryStateInterruptResult CPlaneSuicideState::TryInterruptState( class CAICommand *pCommand )
{
	if ( !pCommand || ACTION_COMMAND_DISAPPEAR == pCommand->ToUnitCmd().nCmdType )
	{
		pPlane->SetCommandFinished();
		return TSIR_YES_IMMIDIATELY;
	}
	return TSIR_YES_WAIT;
}

REGISTER_SAVELOAD_CLASS( 0x1108D489, CPlaneStatesFactory );
REGISTER_SAVELOAD_CLASS( 0x1108D48B, CPlaneBombState );
REGISTER_SAVELOAD_CLASS( 0x1108D46B, CPlaneParaDropState );
REGISTER_SAVELOAD_CLASS( 0x1108D46C, CPlaneFighterPatrolState );
REGISTER_SAVELOAD_CLASS( 0x1108D46D, CPlaneShturmovikPatrolState );
REGISTER_SAVELOAD_CLASS( 0x1108D46E, CPlaneLeaveState );
REGISTER_SAVELOAD_CLASS( 0x1108D46F, CPlaneScoutState );
REGISTER_SAVELOAD_CLASS( 0x1508D4A2, CPlaneFlyDeadState );
REGISTER_SAVELOAD_CLASS( 0x110993C0, CPlaneRestState );
REGISTER_SAVELOAD_CLASS( 0x11099400, CPlaneSwarmToState );
REGISTER_SAVELOAD_CLASS( 0x3124CB40, CPlaneSuicideState );
