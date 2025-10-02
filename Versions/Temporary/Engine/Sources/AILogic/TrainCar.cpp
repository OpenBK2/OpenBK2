#include "stdafx.h"

#include "ArtilleryStates.h"
#include "Trains.h"
#include "Commands.h"
#include "CommonStates.h"
#include "GroupLogic.h"
#include "../Common_RTS_AI/StaticMapHeights.h"
#include "../DebugTools/DebugInfoManager.h"
#include "Guns.h"
#include "Graveyard.h"
#include "NewUpdater.h"

extern NTimer::STime curTime;
extern SRailRoadSystem theRailRoadSystem;
extern CGroupLogic theGroupLogic;				
extern CGraveyard theGraveyard;
extern CEventUpdater updater;

static void DisplayDebugCross( const CVec2 &vPos, const float fSize = 5.0f, const int nWidth = 1, const NDebugInfo::EColor eColor = NDebugInfo::WHITE )
{
	CSegment segm;
	segm.p1 = vPos + CVec2( fSize, 0 );
	segm.p2 = vPos + CVec2( -fSize, 0 );
	segm.dir = segm.p2 - segm.p1;
	DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, nWidth, eColor );

	segm.p1 = vPos + CVec2( 0, fSize );
	segm.p2 = vPos + CVec2( 0, -fSize );
	segm.dir = segm.p2 - segm.p1;
	DebugInfoManager()->CreateSegment( NDebugInfo::OBJECT_ID_GENERATE, segm, nWidth, eColor );
}

//	CTrainCarStatesFactory

CPtr<CTrainCarStatesFactory> CTrainCarStatesFactory::pFactory = 0;

IStatesFactory* CTrainCarStatesFactory::Instance()
{
	if ( pFactory == 0 )
		pFactory = new CTrainCarStatesFactory();

	return pFactory;
}

bool CTrainCarStatesFactory::CanCommandBeExecuted( CAICommand *pCommand )
{
	const EActionCommand &cmdType = pCommand->ToUnitCmd().nCmdType;
	return 
		( 
		cmdType == ACTION_COMMAND_TRAIN_ATTACK_UNIT				||
		cmdType == ACTION_COMMAND_TRAIN_ATTACK_OBJECT			||
		cmdType == ACTION_COMMAND_TRAIN_MOVE							||
		cmdType == ACTION_COMMAND_TRAIN_STOP							||
		cmdType == ACTION_COMMAND_TRAIN_UNLOAD_NOW				||
		cmdType == ACTION_COMMAND_ATTACK_UNIT							||
		cmdType == ACTION_COMMAND_ATTACK_OBJECT						||
		cmdType == ACTION_COMMAND_MOVE_TO									||
		cmdType == ACTION_COMMAND_STOP										||
		cmdType == ACTION_COMMAND_STOP_THIS_ACTION				||
		cmdType == ACTION_COMMAND_SWARM_TO								||
		cmdType == ACTION_COMMAND_PATROL									||
		cmdType == ACTION_COMMAND_UNLOAD									||
		cmdType == ACTION_COMMAND_WAIT										||
		cmdType == ACTION_COMMAND_ART_BOMBARDMENT
		);
}

IUnitState* CTrainCarStatesFactory::ProduceState( class CQueueUnit *pObj, class CAICommand *pCommand )
{
	NI_ASSERT( dynamic_cast<CTrainCar*>( pObj ) != 0, "Wrong unit type (not Train Car)" );
	CTrainCar *pUnit = checked_cast<CTrainCar*>( pObj );

	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();	
	IUnitState* pResult = 0;

	switch ( cmd.nCmdType )
	{	
	case ACTION_COMMAND_TRAIN_STOP:
		pUnit->Stop();
		pResult = CTrainRestState::Instance( pUnit );
		break;
	case ACTION_COMMAND_TRAIN_MOVE:
		pResult = CTrainCarMoveToState::Instance( pUnit );
		break;
	case ACTION_COMMAND_TRAIN_UNLOAD_NOW:
		pResult = 0;
		break;
	case ACTION_COMMAND_TRAIN_ATTACK_UNIT:
		{
			CONVERT_OBJECT( CAIUnit, pTarget, GetObjectByCmd( cmd ), "Wrong unit to attack" );

			pResult = CTrainCarAttackUnitState::Instance( pUnit, pTarget );
		}
		break;
	case ACTION_COMMAND_TRAIN_ATTACK_OBJECT:
		{
			CONVERT_OBJECT( CStaticObject, pTarget, GetObjectByCmd( cmd ), "Wrong object to attack" );

			pResult = CTrainCarAttackObjectState::Instance( pUnit, pTarget );
		}
		break;
	case ACTION_COMMAND_ATTACK_UNIT:
		{
			if ( pUnit->GetLocomotive() && pUnit->GetLocomotive()->GetHitPoints() > 0.0f && pUnit->GetLocomotive()->IsAlive() )
			{
				theGroupLogic.UnitCommand( cmd, pUnit->GetLocomotive(), false );
				pResult = pUnit->GetState();
			}
			else
			{
				if ( GetObjectByCmd( cmd ) && IsValid( GetObjectByCmd( cmd ) ) )
				{
					CONVERT_OBJECT( CAIUnit, pTarget, GetObjectByCmd( cmd ), "Wrong unit to attack" );

					if ( pTarget->IsAlive() )
					{
						pResult = CTrainCarAttackUnitState::Instance( pUnit, pTarget );
					}
				}
				else
				{
					pUnit->SendAcknowledgement( pCommand, ACK_INVALID_TARGET, !pCommand->IsFromAI() );
				}
			}
		}
		break;
	case ACTION_COMMAND_ATTACK_OBJECT:
		{
			if ( pUnit->GetLocomotive() && pUnit->GetLocomotive()->GetHitPoints() > 0.0f && pUnit->GetLocomotive()->IsAlive() )
			{
				theGroupLogic.UnitCommand( cmd, pUnit->GetLocomotive(), false );
				pResult = pUnit->GetState();
			}
			else
			{
				if ( GetObjectByCmd( cmd ) && IsValid( GetObjectByCmd( cmd ) ) )
				{
					CONVERT_OBJECT( CStaticObject, pTarget, GetObjectByCmd( cmd ), "Wrong object to attack" );

					if ( pTarget->IsAlive() )
					{
						pResult = CTrainCarAttackObjectState::Instance( pUnit, pTarget );
					}
				}
				else
				{
					pUnit->SendAcknowledgement( pCommand, ACK_INVALID_TARGET, !pCommand->IsFromAI() );
				}
			}
		}
		break;
	case ACTION_COMMAND_ART_BOMBARDMENT:
		if ( pUnit->GetFirstArtilleryGun() != 0 )
		{ 
			if ( pUnit->GetFirstArtilleryGun()->CanShootToPointWOMove( cmd.vPos, 0.0f ) )
				pResult = CArtilleryBombardmentState::Instance( pUnit, cmd.vPos, cmd.fNumber );
			else
				pUnit->SendAcknowledgement( pCommand, pUnit->GetFirstArtilleryGun()->GetRejectReason(), !pCommand->IsFromAI() );
		}

		break;
	default:
		{
			// Pass command on to the locomotive
			if ( pUnit->GetLocomotive() && pUnit->GetLocomotive()->GetHitPoints() > 0.0f && pUnit->GetLocomotive()->IsAlive() )
			{
				theGroupLogic.UnitCommand( cmd, pUnit->GetLocomotive(), false );
				pResult = pUnit->GetState();
			}
		}
	}
	return pResult;
}

IUnitState* CTrainCarStatesFactory::ProduceRestState( class CQueueUnit *pUnit )
{
	NI_ASSERT( dynamic_cast<CTrainCar*>( pUnit ) != 0, "Wrong unit type" );	
	CTrainCar * pLoc = checked_cast<CTrainCar*>( pUnit );
	return CMechUnitRestState::Instance( pLoc, pLoc->GetCenterPlain(), pLoc->GetDirection(), false, -1 );
}


//	CTrainCar


void CTrainCar::Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *pStats, const float fHP, const WORD dir, const BYTE player, ICollisionsCollector *pCollisionsCollector )
{
	CMilitaryCar::Init( center, z, pStats, fHP, dir, player, pCollisionsCollector );

	// Find the closest tracks VSO
	CVec3 vOwnPos( center, z );
	float fMinDist = 0.0f;
	int nPoint = -1;
	nTrack = -1;
	for ( int i = 0; i < theRailRoadSystem.segments.size(); ++i )
	{
		const SRailRoadSystem::SRRInstance &track = theRailRoadSystem.segments[i];

		for ( int j = 0; j < track.points.size(); ++j )
		{
			float fDist = fabs2( track.points[j].vPos - vOwnPos );

			if ( nTrack == -1 || fDist < fMinDist )
			{
				nTrack = i;
				nPoint = j;
				fMinDist = fDist;
			}
		}
	}

	fFrontOffset = 1;
	fBackOffset = 1;
	fFrontLink = 1;
	fBackLink = 1;
	const NDb::SMechUnitRPGStats *pMechStats = static_cast<const NDb::SMechUnitRPGStats *>( GetStats() );
	NI_ASSERT( pMechStats, StrFmt( "Not MechUnit stats in unit \"%s\" (train car)", NDb::GetResName(GetStats()) ) );
	if ( pMechStats )
	{
		fFrontOffset = fabs( pMechStats->vFrontWheel.y );
		fBackOffset = fabs( pMechStats->vBackWheel.y );
		fFrontLink = fabs( pMechStats->vHookPoint.y );
		fBackLink = fabs( pMechStats->vTowPoint.y );
	}

	if ( nTrack != -1 )					// Snap car to track, if any
	{
		const SRailRoadSystem::SRRInstance &track = theRailRoadSystem.segments[nTrack];

		vOwnPos = track.points[nPoint].vPos;
		vOwnPos.z = GetHeights()->GetZ( vOwnPos );
		if( !nPoint )
			++nPoint;

		vOwnPos = track.points[nPoint].vPos - track.points[nPoint - 1].vPos;

		WORD wOwnDir = dir;
		WORD wDir1 = GetDirectionByVector( CVec2( -vOwnPos.x, -vOwnPos.y ) );			// To point 0
		WORD wDir2 = GetDirectionByVector( CVec2( vOwnPos.x, vOwnPos.y ) );				// To end point

		fTrackPos = ( nPoint - 1 ) * track.fPointLength;
		fTrackLimit = track.GetTrackLength();

		if ( DirsDifference( wOwnDir, wDir2 ) > DirsDifference( wOwnDir, wDir1 ) )
		{
			bBackward = true;
			fTrackPos = Clamp( fTrackPos, fFrontOffset, fTrackLimit - fBackOffset );
		}
		else
		{
			bBackward = false;
			fTrackPos = Clamp( fTrackPos, fBackOffset, fTrackLimit - fFrontOffset );
		}

		SetToTrackPos( fTrackPos );
	}

	pLinkFront = 0;
	pLinkBack = 0;
	pLocomotive = 0;

	updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, this, -1 );
}

void CTrainCar::SetToTrackPos( float fPos )
{
	if ( nTrack < 0 )
		return;

	const SRailRoadSystem::SRRInstance &track = theRailRoadSystem.segments[nTrack];

	if ( bBackward )
		fTrackPos = Clamp( fPos, fFrontOffset, GetTrackLimit() - fBackOffset );
	else
		fTrackPos = Clamp( fPos, fBackOffset, GetTrackLimit() - fFrontOffset );

	float fWheelPos;
	if ( bBackward )
		fWheelPos = Clamp( fTrackPos - fFrontOffset, 0.0f, GetTrackLimit() );
	else
		fWheelPos = Clamp( fTrackPos + fFrontOffset, 0.0f, GetTrackLimit() );
	float fFraction;
	int nPoint;
	track.Decompose( fWheelPos, &nPoint, &fFraction );

	CVec3 vFrontPos;
	if ( nPoint + 1 >= track.points.size() )
		vFrontPos = track.points.back().vPos;
	else
		vFrontPos.Lerp( fFraction, track.points[nPoint].vPos, track.points[nPoint + 1].vPos );
	vFrontPos.z = GetHeights()->GetZ( vFrontPos );

	CVec3 vBackPos;
	if ( bBackward )
		fWheelPos = Clamp( fTrackPos + fBackOffset, 0.0f, GetTrackLimit() );
	else
		fWheelPos = Clamp( fTrackPos - fBackOffset, 0.0f, GetTrackLimit() );

	track.Decompose( fWheelPos, &nPoint, &fFraction );
	if ( nPoint + 1 >= track.points.size() )
		vBackPos = track.points.back().vPos;
	else
		vBackPos.Lerp( fFraction, track.points[nPoint].vPos, track.points[nPoint + 1].vPos );
	vBackPos.z = GetHeights()->GetZ( vBackPos );

	CVec3 vPos;
	vPos = vBackPos - vFrontPos;

	WORD wDir = GetDirectionByVector( vPos.x, vPos.y );
	if ( IsGoForward() )
		wDir += 32768;

	SetDirection( wDir );

	vPos.Lerp( fFrontOffset / ( fFrontOffset + fBackOffset ), vFrontPos, vBackPos );
	SetCenter( vPos );
}

void CTrainCar::TakeDamage( const float fDamage, const SWeaponRPGStats::SShell *pShell, const int nPlayerOfShoot, CAIUnit *pShotUnit )
{
	CMilitaryCar::TakeDamage( fDamage, pShell, nPlayerOfShoot, pShotUnit );
}

void CTrainCar::Die( const bool fromExplosion, const float fDamage )
{
	CMilitaryCar::Die( fromExplosion, fDamage );

	if ( !pLocomotive )
	{
		theGraveyard.AddToSoonBeDead( this, fDamage );
	}
}

void CTrainCar::LinkToCar( CMilitaryCar *pLinkTo )
{
	CTrainCar *pLinkToCar = dynamic_cast<CTrainCar*>( pLinkTo );

	if ( pLinkToCar )					// It is another car
	{
		pLinkToCar->pLinkBack = this;
		pLinkFront = pLinkToCar;

		if ( pLinkToCar->GetLocomotive() )
			LinkToLocomotive( pLinkToCar->GetLocomotive() );
	}
	else
	{
		CTrainLocomotive *pLinkToLocomotive = dynamic_cast<CTrainLocomotive*>( pLinkTo );

		if ( pLinkToLocomotive )
			LinkToLocomotive( pLinkToLocomotive );
		else
			NI_ASSERT( 0, StrFmt( "Wrong link for train car (not another car or locomotive), link ID %d ", GetLink() ) );
	}
}

void CTrainCar::LinkToLocomotive( CTrainLocomotive *pLinkTo )
{
	if ( !pLinkTo )
	{
		pLocomotive = 0;
		return;
	}
	// Link this car
	pLinkTo->AddCar( this );
	pLocomotive = pLinkTo;
	bBackward = pLocomotive->IsBackwards();

	// Link other cars
	if ( pLinkBack )
		pLinkBack->LinkToLocomotive( pLinkTo );
	pLocomotive->SetToTrackPos( pLocomotive->GetTrackPos() );
	SetCenter( GetCenter(), false );
	CallUpdatePlacement();
}

void CTrainCar::Segment()
{
	CMilitaryCar::Segment();
}

void CTrainCar::SetCollision( ICollision *pCollision, IPath *pPath )
{
	if ( pCollision->GetName() != NCollision::ECN_GIVE_PLACE )
		CMilitaryCar::SetCollision( pCollision, pPath );
}

const ECollidingType CTrainCar::GetCollidingType( CBasePathUnit *pUnit ) const
{
	if ( CTrainCar *pTrainCar = dynamic_cast<CTrainCar *>( pUnit ) )
	{
		if ( pTrainCar->GetLocomotive() == GetLocomotive() )	// it's from this train: ignore
			return ECT_NONE;
	}
	else if ( CTrainLocomotive *pLocoCar = dynamic_cast<CTrainLocomotive *>( pUnit ) )
	{
		if ( pLocoCar == GetLocomotive() )	// or it's loco from this train: ignore
			return ECT_NONE;
	}
	return ECT_ALL;
}

//	CTrainCarPath

bool CTrainCarPath::Init( CBasePathUnit *_pUnit, CLocomotivePath *pOwnerPath )
{
	pUnit = dynamic_cast<CTrainCar*>( _pUnit );
	pOwner = pOwnerPath;

	if ( pUnit )
	{
		pUnit->SendAlongSmoothPath( this );

		pUnit->SetGoForward( !pUnit->IsBackwards() && !CanGoBackward() ||
			pUnit->IsBackwards() && CanGoBackward() );
	}

	return ( pUnit != 0 );
}

//	CTrainCarMoveToState

IUnitState* CTrainCarMoveToState::Instance( CAIUnit *_pUnit )
{
	NI_ASSERT( dynamic_cast<CTrainCar*>( _pUnit ) != 0, "Wrong unit type (not Train Car)" );
	CTrainCar *pUnit = checked_cast<CTrainCar*>( _pUnit );
	if ( pUnit )
		return new CTrainCarMoveToState( pUnit );
	else
		return 0;
}

CTrainCarMoveToState::CTrainCarMoveToState( CTrainCar *_pUnit ) :
CFreeFireManager( _pUnit ), pUnit( _pUnit ), bWaiting( true ), pPath( 0 )
{
	//pUnit->UnlockTiles();
	//pUnit->FixUnlocking();
}

void CTrainCarMoveToState::Segment()
{
	if ( bWaiting )
	{
		CLocomotiveMoveState *pOwnerState = dynamic_cast<CLocomotiveMoveState*>( pUnit->GetLocomotive()->GetState() );

		if ( pOwnerState && pOwnerState->GetPath() )
		{
			bWaiting = false;

			// Create path
			pPath = new CTrainCarPath;
			pPath->Init( pUnit, pOwnerState->GetPath() );
		}
	}
	else
	{
		if ( pUnit->GetBehaviourFire() == SBehaviour::EFAtWill )
			CFreeFireManager::Analyze( pUnit, 0 );

		if ( pPath->IsFinished() || pUnit->GetLocomotive()->GetState()->GetName() != EUSN_MOVE )
		{
			pUnit->SetCommandFinished();
		}
	}
}

ETryStateInterruptResult CTrainCarMoveToState::TryInterruptState( class CAICommand *pCommand )
{
	pUnit->Stop();
	//pUnit->UnfixUnlocking();
	pUnit->SetCommandFinished();

	return TSIR_YES_IMMIDIATELY;
}

//	CTrainCarAttackUnitState

IUnitState* CTrainCarAttackUnitState::Instance( CAIUnit *_pUnit, CAIUnit *_pTarget )
{
	NI_ASSERT( dynamic_cast<CTrainCar*>( _pUnit ) != 0, "Wrong unit type (not Train Car)" );
	CTrainCar *pUnit = checked_cast<CTrainCar*>( _pUnit );
	if ( pUnit )
		return new CTrainCarAttackUnitState( pUnit, _pTarget );
	else
		return 0;
}

CTrainCarAttackUnitState::CTrainCarAttackUnitState( CTrainCar *_pUnit, CAIUnit *_pTarget ) :
pUnit( _pUnit ), pTarget( _pTarget ), eState( EAS_STARTING ), pPath( 0 )
{
	//pUnit->UnlockTiles();
	//pUnit->FixUnlocking();
}

void CTrainCarAttackUnitState::Segment()
{
	switch( eState ) 
	{
	case EAS_STARTING:
		{
			if ( pUnit->GetLocomotive() && pUnit->GetLocomotive()->GetHitPoints() > 0.0f && pUnit->GetLocomotive()->IsAlive() )
			{
				CLocomotiveAttackUnitState *pOwnerState = dynamic_cast<CLocomotiveAttackUnitState*>( pUnit->GetLocomotive()->GetState() );

				if ( pOwnerState ) 
				{
					eState = EAS_WAITING;
				}
			}
			else
			{
				if ( !pTarget->IsAlive() ||
					!pTarget->IsVisible( pUnit->GetParty() ) )
				{
					TryInterruptState( 0 );
					break;
				}

				eState = EAS_FIRING;
			}
		}
		break;
	case EAS_WAITING:
		{
			CLocomotiveAttackUnitState *pOwnerState = dynamic_cast<CLocomotiveAttackUnitState*>( pUnit->GetLocomotive()->GetState() );

			if ( pOwnerState && pOwnerState->GetPath() )
			{
				eState = EAS_FOLLOWING;

				// Create path
				pPath = new CTrainCarPath;
				pPath->Init( pUnit, pOwnerState->GetPath() );
			}
		}
		break;
	case EAS_FOLLOWING:
		{
			CLocomotiveAttackUnitState *pOwnerState = dynamic_cast<CLocomotiveAttackUnitState*>( pUnit->GetLocomotive()->GetState() );

			if ( !pOwnerState || !pOwnerState->GetPath() || pOwnerState->GetPath()->IsFinished() )
			{
				eState = EAS_WAITING;
				pPath = 0;
			}
		}
		break;
	case EAS_FIRING:
		{
			if ( !TryFiring() )
			{
				eState = EAS_STARTING;

				if ( !pTarget->IsVisible( pUnit->GetParty() ) )
				{
					TryInterruptState( 0 );
				}
			}
		}
		break;
	}
}

const bool CTrainCarAttackUnitState::TryFiring()
{
	bool bResult = false;
	for ( int i = 0; i < pUnit->GetNGuns(); ++i )
	{
		CBasicGun *pGun = pUnit->GetGun( i );

		if ( pGun->CanShootWOGunTurn( pTarget, 1 ) && 
			!pGun->IsRelaxing() &&
			!pGun->IsBursting() &&
			pTarget->IsVisible( pUnit->GetParty() ) )
		{
			pGun->StartEnemyBurst( pTarget, true );
			bResult = true;
		}
		else
			pGun->TraceAim( pTarget );
	}

	return bResult;
}

ETryStateInterruptResult CTrainCarAttackUnitState::TryInterruptState( class CAICommand *pCommand )
{
	for ( int i = 0; i < pUnit->GetNGuns(); ++i )
		pUnit->GetGun( i )->StopFire();

	pUnit->Stop();
	//pUnit->UnfixUnlocking();
	pUnit->SetCommandFinished();

	return TSIR_YES_IMMIDIATELY;
}

//	CTrainCarAttackObjectState

IUnitState* CTrainCarAttackObjectState::Instance( CAIUnit *_pUnit, CStaticObject *_pTarget )
{
	NI_ASSERT( dynamic_cast<CTrainCar*>( _pUnit ) != 0, "Wrong unit type (not Train Car)" );
	CTrainCar *pUnit = checked_cast<CTrainCar*>( _pUnit );
	if ( pUnit )
		return new CTrainCarAttackObjectState( pUnit, _pTarget );
	else
		return 0;
}

CTrainCarAttackObjectState::CTrainCarAttackObjectState( CTrainCar *_pUnit, CStaticObject *_pTarget ) :
pUnit( _pUnit ), pTarget( _pTarget ), eState( EAS_STARTING ), pPath( 0 )
{
	//pUnit->UnlockTiles();
	//pUnit->FixUnlocking();
}

void CTrainCarAttackObjectState::Segment()
{
	switch( eState ) 
	{
	case EAS_STARTING:
		{
			if ( pUnit->GetLocomotive() && pUnit->GetLocomotive()->GetHitPoints() > 0.0f && pUnit->GetLocomotive()->IsAlive() )
			{
				CLocomotiveAttackObjState *pOwnerState = dynamic_cast<CLocomotiveAttackObjState*>( pUnit->GetLocomotive()->GetState() );

				if ( pOwnerState ) 
				{
					eState = EAS_WAITING;
				}
			}
			else
			{
				if ( !pTarget->IsAlive() ||
					!pTarget->IsVisible( pUnit->GetParty() ) )
				{
					TryInterruptState( 0 );
					break;
				}

				eState = EAS_FIRING;
			}
		}
		break;
	case EAS_WAITING:
		{
			CLocomotiveAttackObjState *pOwnerState = dynamic_cast<CLocomotiveAttackObjState*>( pUnit->GetLocomotive()->GetState() );

			if ( pOwnerState && pOwnerState->GetPath() )
			{
				eState = EAS_FOLLOWING;

				// Create path
				pPath = new CTrainCarPath;
				pPath->Init( pUnit, pOwnerState->GetPath() );
			}
		}
		break;
	case EAS_FOLLOWING:
		{
			CLocomotiveAttackObjState *pOwnerState = dynamic_cast<CLocomotiveAttackObjState*>( pUnit->GetLocomotive()->GetState() );

			if ( !pOwnerState || !pOwnerState->GetPath() || pOwnerState->GetPath()->IsFinished() )
			{
				eState = EAS_WAITING;
				pPath = 0;
			}
		}
		break;
	case EAS_FIRING:
		{
			if ( !TryFiring() )
			{
				eState = EAS_STARTING;

				if ( !pTarget->IsVisible( pUnit->GetParty() ) )
				{
					TryInterruptState( 0 );
				}
			}
		}
		break;
	}
}

const bool CTrainCarAttackObjectState::TryFiring()
{
	bool bResult = false;
	for ( int i = 0; i < pUnit->GetNGuns(); ++i )
	{
		CBasicGun *pGun = pUnit->GetGun( i );

		if ( pGun->CanShootToObjectWOMove( pTarget ) && 
			!pGun->IsRelaxing() &&
			!pGun->IsBursting() &&
			pTarget->IsVisible( pUnit->GetParty() ) )
		{
			pGun->StartPointBurst( GetTargetCenter(), true );
			bResult = true;
		}
		else
		{
			const CVec2 vDir = GetTargetCenter() - pUnit->GetCenterPlain();
			const WORD wGunDir = GetDirectionByVector( vDir ) - pUnit->GetDirection();
			pGun->TurnToRelativeDir( wGunDir );
		}
	}

	return bResult;
}

ETryStateInterruptResult CTrainCarAttackObjectState::TryInterruptState( class CAICommand *pCommand )
{
	for ( int i = 0; i < pUnit->GetNGuns(); ++i )
		pUnit->GetGun( i )->StopFire();

	pUnit->Stop();
	//pUnit->UnfixUnlocking();
	pUnit->SetCommandFinished();

	return TSIR_YES_IMMIDIATELY;
}

REGISTER_SAVELOAD_CLASS( 0x191992C1, CTrainCar );
REGISTER_SAVELOAD_CLASS( 0x191992C0, CTrainCarStatesFactory );
REGISTER_SAVELOAD_CLASS( 0x19199340, CTrainCarPath );
REGISTER_SAVELOAD_CLASS( 0x19199341, CTrainCarMoveToState );
REGISTER_SAVELOAD_CLASS( 0x1919DD40, CTrainCarAttackUnitState );
REGISTER_SAVELOAD_CLASS( 0x1919DD41, CTrainCarAttackObjectState );

