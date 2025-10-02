#include "stdafx.h"

#include "aiconsts.h"
#include "Trains.h"
#include "CommonStates.h"
#include "Commands.h"
#include "../Common_RTS_AI/AIMap.h"
#include "../Common_RTS_AI/StaticMapHeights.h"
#include "GroupLogic.h"
#include "Guns.h"
#include "../DebugTools/DebugInfoManager.h"
#include "Graveyard.h"
#include "NewUpdater.h"

extern CEventUpdater updater;
extern NTimer::STime curTime;
extern SRailRoadSystem theRailRoadSystem;
extern CGroupLogic theGroupLogic;				
extern CGraveyard theGraveyard;

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

//	CTrainLocomotive

void CTrainLocomotive::Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *pStats, const float fHP, const WORD dir, const BYTE player, ICollisionsCollector *pCollisionsCollector )
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
	fBackLink = 1;
	const NDb::SMechUnitRPGStats *pMechStats = static_cast<const NDb::SMechUnitRPGStats *>( GetStats() );
	NI_ASSERT( pMechStats, StrFmt( "Not MechUnit stats in unit \"%s\" (locomotive)", NDb::GetResName(GetStats()) ) );
	if ( pMechStats )
	{
		fFrontOffset = fabs( pMechStats->vFrontWheel.y );
		fBackOffset = fabs( pMechStats->vBackWheel.y );
		fBackLink = fabs( pMechStats->vTowPoint.y );
	}
	cars.clear();
	fTrainLength = fBackOffset;

	if ( nTrack != -1 )					// Snap locomotive to track, if any
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
			bBackward = true;					// Facing 0 of the track
			fTrackPos = Clamp( fTrackPos, fFrontOffset, fTrackLimit - fBackOffset );
		}
		else
		{
			bBackward = false;				// Facing end of the track
			fTrackPos = Clamp( fTrackPos, fBackOffset, fTrackLimit - fFrontOffset );
		}

		SetToTrackPos( fTrackPos );
	}

	Stop();				// Just in case

	updater.AddUpdate( 0, ACTION_NOTIFY_PLACEMENT, this, -1 );
}

void CTrainLocomotive::SetToTrackPos( float fPos )
{
	if ( nTrack < 0 )
		return;

	const SRailRoadSystem::SRRInstance &track = theRailRoadSystem.segments[nTrack];

	//NI_ASSERT( fabs( fTrackPos - fPos ) <= 20.0f, "Value too big" );

	if ( bBackward )
		fTrackPos = Clamp( fPos, fFrontOffset, GetTrackLimit() - fTrainLength/*fBackOffset*/ );
	else
		fTrackPos = Clamp( fPos, fTrainLength/*fBackOffset*/, GetTrackLimit() - fFrontOffset );

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

	// Reposition cars
	float fCarPos = fTrackPos;
	if ( bBackward )
		fCarPos += fBackLink;
	else
		fCarPos -= fBackLink;
	for ( CCarList::iterator it = cars.begin(); it != cars.end(); ++it )
	{
		CTrainCar *pCar = dynamic_cast_ptr<CTrainCar*>( *it );
		if ( !pCar )
			continue;

		if ( bBackward )
			fCarPos += pCar->GetFrontLink();
		else
			fCarPos -= pCar->GetFrontLink();

		pCar->SetToTrackPos( fCarPos );
		pCar->CallUpdatePlacement();

		if ( bBackward )
			fCarPos += pCar->GetBackLink();
		else
			fCarPos -= pCar->GetBackLink();
	}
}

void CTrainLocomotive::TakeDamage( const float fDamage, const SWeaponRPGStats::SShell *pShell, const int nPlayerOfShoot, CAIUnit *pShotUnit )
{
	CMilitaryCar::TakeDamage( fDamage, pShell, nPlayerOfShoot, pShotUnit );
}

void CTrainLocomotive::Die( const bool fromExplosion, const float fDamage )
{
	for ( CCarList::iterator it = cars.begin(); it != cars.end(); ++it )
	{
		CTrainCar *pCar = dynamic_cast_ptr<CTrainCar*>( *it );
		if ( !pCar )
			continue;

		pCar->LinkToLocomotive( 0 );
		if ( !pCar->IsAlive() )
			theGraveyard.AddToSoonBeDead( pCar, 0.0f );
	}

	CMilitaryCar::Die( fromExplosion, fDamage );

	theGraveyard.AddToSoonBeDead( this, fDamage );
}

void CTrainLocomotive::Segment()
{
	CMilitaryCar::Segment();
}

const SRailRoadSystem::SRRInstance *CTrainLocomotive::GetTrack() const
{
	if ( nTrack == -1 )
		return 0;
	else
		return &(theRailRoadSystem.segments[nTrack]);
}

void CTrainLocomotive::AddCar( CTrainCar *pNewCar )
{
	cars.push_back( pNewCar );

	pNewCar->nTrack = nTrack;
	pNewCar->bBackward = bBackward;

	fTrainLength = 0.0f;
	for ( CCarList::iterator it = cars.begin(); it != cars.end(); ++it )
	{
		CTrainCar *pCar = dynamic_cast_ptr<CTrainCar*>( *it );
		if ( !pCar )
			continue;

		fTrainLength += pCar->GetLength();
	}

	if ( cars.size() > 0 )
		fTrainLength += fBackLink;
	else
		fTrainLength += fBackOffset;

	// Reposition train if necessary
	// SetToTrackPos( fTrackPos );
}

CAIUnit* CTrainLocomotive::GetLastCar()
{
	if ( cars.size() > 0 )
		return cars.back();
	else
		return this;
}

void CTrainLocomotive::PassCommandToAll( CAICommand *pCommand )
{
	CPtr<CAICommand> pToDelete = pCommand;

	for ( CCarList::iterator it = cars.begin(); it != cars.end(); ++it )
	{
		CTrainCar *pCar = dynamic_cast_ptr<CTrainCar*>( *it );
		if ( !pCar )
			continue;

		pCar->UnitCommand( pCommand, false, true );
	}
}

const bool CTrainLocomotive::TryFiringAll()
{
	bool bResult = false;

	// Try firing with all cars
	for ( CCarList::iterator it = cars.begin(); it != cars.end(); ++it )
	{
		CTrainCar *pCar = dynamic_cast_ptr<CTrainCar*>( *it );
		if ( !pCar )
			continue;

		if ( pCar->GetState()->GetName() == EUSN_ATTACK_OBJECT )
		{
			CTrainCarAttackObjectState *pCarState = dynamic_cast<CTrainCarAttackObjectState*>( pCar->GetState() );
			if ( pCarState )
				bResult |= pCarState->TryFiring();
		}
		else if ( pCar->GetState()->GetName() == EUSN_ATTACK_UNIT )
		{
			CTrainCarAttackUnitState *pCarState = dynamic_cast<CTrainCarAttackUnitState*>( pCar->GetState() );
			if ( pCarState )
				bResult |= pCarState->TryFiring();
		}
	}

	return bResult;
}

const float CTrainLocomotive::GetTrainLength()
{
	return fTrainLength;			// this should return the distance from the center of the locomotive to the center of the last car
														// it doesnt; unless the train and the last car are very different
}

void CTrainLocomotive::SetCollision( ICollision *pCollision, IPath *pPath )
{
	if ( pCollision->GetName() != NCollision::ECN_GIVE_PLACE )
		CMilitaryCar::SetCollision( pCollision, pPath );
}

const ECollidingType CTrainLocomotive::GetCollidingType( CBasePathUnit *pUnit ) const
{
	if ( CTrainCar *pTrainCar = dynamic_cast<CTrainCar *>( pUnit ) )
	{
		if ( pTrainCar->GetLocomotive() == this )			// it's from this train: ignore
			return ECT_NONE;
	}
  return ECT_ALL;
}

void CTrainLocomotive::Stop()
{
	for ( CCarList::iterator it = cars.begin(); it != cars.end(); ++it )
	{
		CTrainCar *pCar = dynamic_cast_ptr<CTrainCar*>( *it );
		if ( !pCar )
			continue;

		theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_TRAIN_STOP ), pCar, false );
	}

	CMilitaryCar::Stop();
}

void CTrainLocomotive::SetMovingTileLocks( bool bMoving )
{
	if ( bMoving )
	{
		UnlockTiles();
		FixUnlocking();
	}
	else
	{
		UnfixUnlocking();
	}

	for ( CCarList::iterator it = cars.begin(); it != cars.end(); ++it )
	{
		CTrainCar *pCar = dynamic_cast_ptr<CTrainCar*>( *it );
		if ( !pCar )
			continue;

		if ( bMoving )
		{
			pCar->UnlockTiles();
			pCar->FixUnlocking();
		}
		else
		{
			pCar->UnfixUnlocking();
		}
	}
}

//	CLocomotiveStatesFactory

CPtr<CLocomotiveStatesFactory> CLocomotiveStatesFactory::pFactory = 0;

IStatesFactory* CLocomotiveStatesFactory::Instance()
{
	if ( pFactory == 0 )
		pFactory = new CLocomotiveStatesFactory();

	return pFactory;
}

bool CLocomotiveStatesFactory::CanCommandBeExecuted( CAICommand *pCommand )
{
	const EActionCommand &cmdType = pCommand->ToUnitCmd().nCmdType;
	return 
		( cmdType == ACTION_COMMAND_ATTACK_UNIT			||
		cmdType == ACTION_COMMAND_ATTACK_OBJECT			||
		cmdType == ACTION_COMMAND_MOVE_TO						||
		cmdType == ACTION_COMMAND_STOP							||
		cmdType == ACTION_COMMAND_STOP_THIS_ACTION	||
		cmdType == ACTION_COMMAND_SWARM_TO					||
		cmdType == ACTION_COMMAND_PATROL						||
		cmdType == ACTION_COMMAND_UNLOAD						||
		cmdType == ACTION_COMMAND_WAIT							
		);
}

IUnitState* CLocomotiveStatesFactory::ProduceState( class CQueueUnit *pObj, class CAICommand *pCommand )
{
	NI_ASSERT( dynamic_cast<CTrainLocomotive*>( pObj ) != 0, "Wrong unit type (not Train Locomotive)" );
	CTrainLocomotive *pUnit = checked_cast<CTrainLocomotive*>( pObj );

	const SAIUnitCmd &cmd = pCommand->ToUnitCmd();	
	IUnitState* pResult = 0;

	switch ( cmd.nCmdType )
	{	
	case ACTION_COMMAND_STOP:
	case ACTION_COMMAND_STOP_THIS_ACTION:
		pUnit->Stop();
		//pResult = ProduceRestState( pUnit );
		pResult = CTrainRestState::Instance( pUnit );
		break;
	case ACTION_COMMAND_WAIT:
		pUnit->Stop();
		//pResult = CMechUnitRestState::Instance( pUnit, cmd.vPos, pUnit->GetDirection(), 0, cmd.fNumber );
		pResult = CTrainRestState::Instance( pUnit );
		break;
	case ACTION_COMMAND_MOVE_TO:
	case ACTION_COMMAND_SWARM_TO:
		pResult = CLocomotiveMoveState::Instance( pUnit, cmd.vPos );
		break;
	case ACTION_COMMAND_ATTACK_OBJECT:
		{
			if ( GetObjectByCmd( cmd ) && IsValid( GetObjectByCmd( cmd ) ) )
			{
				CONVERT_OBJECT( CStaticObject, pTarget, GetObjectByCmd( cmd ), "Wrong object to attack" );

				if ( pTarget->IsAlive() )
				{
					pResult = CLocomotiveAttackObjState::Instance( pUnit, pTarget );
				}
			}
			else
			{
				pUnit->SendAcknowledgement( pCommand, ACK_INVALID_TARGET, !pCommand->IsFromAI() );
			}
		}
		break;
	case ACTION_COMMAND_ATTACK_UNIT:
		{
			if ( GetObjectByCmd( cmd ) && IsValid( GetObjectByCmd( cmd ) ) )
			{
				CONVERT_OBJECT( CAIUnit, pTarget, GetObjectByCmd( cmd ), "Wrong unit to attack" );

				if ( pTarget->IsAlive() )
				{
					pResult = CLocomotiveAttackUnitState::Instance( pUnit, pTarget );
				}
			}
			else
			{
				pUnit->SendAcknowledgement( pCommand, ACK_INVALID_TARGET, !pCommand->IsFromAI() );
			}
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
	case ACTION_COMMAND_UNLOAD:
		{
			pResult = 0;
		}
		break;
	default:
		NI_ASSERT( false, "Wrong command" );
	}
	return pResult;
}

IUnitState* CLocomotiveStatesFactory::ProduceRestState( class CQueueUnit *pUnit )
{
	NI_ASSERT( dynamic_cast<CTrainLocomotive*>( pUnit ) != 0, "Wrong unit type" );	
	CTrainLocomotive * pLoc = checked_cast<CTrainLocomotive*>( pUnit );
	/*if ( pLoc->GetGuns() )
		return CTrainRestState::Instance( pLoc );
	else*/
		return CMechUnitRestState::Instance( pLoc, pLoc->GetCenterPlain(), pLoc->GetDirection(), false, -1 );
}

//	CTrainRestState

IUnitState* CTrainRestState::Instance( CAIUnit *_pUnit )
{
	return new CTrainRestState( _pUnit );
}

CTrainRestState::CTrainRestState( CAIUnit *_pUnit ) :
CFreeFireManager( _pUnit ), pUnit( _pUnit )
{ 
	CTrainLocomotive *pUnit = dynamic_cast<CTrainLocomotive*>( _pUnit );
	if ( pUnit )
	{
		SAIUnitCmd cmd( ACTION_COMMAND_TRAIN_STOP );
		pUnit->PassCommandToAll( new CAICommand( cmd ) );
	}
}

void CTrainRestState::Segment()
{
	if ( pUnit->GetBehaviourFire() == SBehaviour::EFAtWill )
		CFreeFireManager::Analyze( pUnit, 0 );
}

ETryStateInterruptResult CTrainRestState::TryInterruptState( class CAICommand *pCommand )
{
	pUnit->SetCommandFinished();

	return TSIR_YES_IMMIDIATELY;
}

//	CLocomotiveMoveState

IUnitState* CLocomotiveMoveState::Instance( CAIUnit *_pUnit, const CVec2 &point )
{
	NI_ASSERT( dynamic_cast<CTrainLocomotive*>( _pUnit ) != 0, "Wrong unit type (not Train Locomotive)" );
	CTrainLocomotive *pUnit = checked_cast<CTrainLocomotive*>( _pUnit );
	if ( pUnit )
		return new CLocomotiveMoveState( pUnit, point );
	else
		return 0;
}

CLocomotiveMoveState::CLocomotiveMoveState( CTrainLocomotive *_pUnit, const CVec2 &_point ) :
CFreeFireManager( _pUnit ), pUnit( _pUnit ), point( _point ), startTime( curTime ), bWaiting( true ), pPath( 0 )
{
	pUnit->SetMovingTileLocks( true );

	// Set all cars in motion
	SAIUnitCmd cmd( ACTION_COMMAND_TRAIN_MOVE );
	_pUnit->PassCommandToAll( new CAICommand( cmd ) );
}

void CLocomotiveMoveState::Segment()
{
	if ( bWaiting )
	{
		if ( curTime - startTime >= SAIConsts::AI_SEGMENT_DURATION )
		{
			bWaiting = false;

			// Create path
			pPath = new CLocomotivePath;
			pPath->Init( pUnit, point );
		}
	}
	else
	{
		if ( pUnit->GetBehaviourFire() == SBehaviour::EFAtWill )
			CFreeFireManager::Analyze( pUnit, 0 );

		if ( pPath->IsFinished() || pUnit->GetNextCommand() != 0 && fabs2( pUnit->GetCenterPlain() - point ) <= sqr( 2.5f * (float)SConsts::TILE_SIZE ) )
		{
			if ( pUnit->GetNextCommand() == 0 )
				theGroupLogic.UnitCommand( SAIUnitCmd( ACTION_COMMAND_STOP ), pUnit, false );

			pUnit->SetCommandFinished();
		}

	}
}

ETryStateInterruptResult CLocomotiveMoveState::TryInterruptState( class CAICommand *pCommand )
{
	pUnit->Stop();
	pUnit->SetMovingTileLocks( false );
	pUnit->SetCommandFinished();

	return TSIR_YES_IMMIDIATELY;
}

//	CLocomotivePath

bool CLocomotivePath::Init( CBasePathUnit *_pUnit, const CVec2 &_vTargetPoint )
{
	pUnit = checked_cast<CTrainLocomotive*>( _pUnit );
	NI_ASSERT( pUnit, "Invalid Unit passed to LocomotivePath" );
	if ( !pUnit )
		return false;

	const SRailRoadSystem::SRRInstance *pTrack = pUnit->GetTrack();
	NI_ASSERT( pTrack, "Invalid Track VSO passed to LocomotivePath" );

	bFinished = false;

	// Find closest point
	int nEndPoint = 0;
	CVec3 vPoint = pTrack->points[0].vPos;
	float fEndDist = fabs2( vPoint - CVec3( _vTargetPoint, 0 ) );

	for( int i = 0; i < pTrack->points.size() ; ++i )
	{
		vPoint = pTrack->points[i].vPos;
		const float fTemp = fabs2( vPoint - CVec3( _vTargetPoint, 0 ) );
		if ( fTemp < fEndDist )
		{
			fEndDist = fTemp;
			nEndPoint = i;
		}
	}

	fCurPos = pUnit->GetTrackPos();
	fEndPos = nEndPoint * pTrack->fPointLength;

	pUnit->SendAlongSmoothPath( this );

	pUnit->SetGoForward( !pUnit->IsBackwards() && !CanGoBackward() ||
		pUnit->IsBackwards() && CanGoBackward() );

	return true;
}

IMemento* CLocomotivePath::CreateMemento() const
{
	SLocomotivePathMemento *pMemento = new SLocomotivePathMemento;

	pMemento->fCurPos = fCurPos;
	pMemento->fEndPos = fEndPos;
	pMemento->vEndPoint = vEndPoint;

	return pMemento;
}

bool CLocomotivePath::Init( IMemento *pMemento, CBasePathUnit *_pUnit, CAIMap *pAIMap )
{
	SLocomotivePathMemento *pLocMemento = checked_cast<SLocomotivePathMemento*>(pMemento);
	if ( !pLocMemento )
		return false;

	pUnit = checked_cast<CTrainLocomotive*>( _pUnit );
	NI_ASSERT( pUnit, "Invalid Unit passed to LocomotivePath" );
	if ( !pUnit )
		return false;

	vEndPoint = pLocMemento->vEndPoint;
	fCurPos = pUnit->GetTrackPos();
	fEndPos = pLocMemento->fEndPos;
	bFinished = false;

	pUnit->SendAlongSmoothPath( this );

	pUnit->SetGoForward( pUnit->IsBackwards() && !CanGoBackward() ||
		!pUnit->IsBackwards() && CanGoBackward() );

	return true;
}

void CLocomotivePath::Segment( const NTimer::STime timeDiff )
{
	const float fStep = pUnit->GetSpeed() * timeDiff;
	const float fOldPos = fCurPos;

	if ( bFinished )
		return;

	if ( pUnit->GetSpeed() == 0.0f ) 
	{
		fEndPos = fCurPos;
		bFinished = true;
	}

	if ( fabs( fCurPos - fEndPos ) <= fStep )				// Arrived to end point?
	{
		bFinished = true;

		fCurPos = fEndPos;
	}
	else
	{
		if ( fCurPos > fEndPos )
			fCurPos -= fStep;
		else
      fCurPos += fStep;
	}

	pUnit->SetToTrackPos( fCurPos );
	if ( fCurPos != pUnit->GetTrackPos() )
	{
		fCurPos = pUnit->GetTrackPos();
		bFinished = true;
	}

	if ( !bFinished )															// Analyze locked tiles
	{
		CAIUnit *pUnitToCheck = pUnit;

		if ( ( fCurPos < fEndPos ) == pUnit->IsBackwards() )
			pUnitToCheck = pUnit->GetLastCar();

		if ( pUnitToCheck->IsOnLockedTiles( pUnitToCheck->GetUnitProfile() ) )
		{
			fCurPos = fOldPos;
			fEndPos = fOldPos;
			bFinished = true;
		}
	}

	if ( bFinished )
		pUnit->Stop();
}

//	CLocomotiveAttackUnitState

IUnitState* CLocomotiveAttackUnitState::Instance( CAIUnit *_pUnit, CAIUnit *_pTarget )
{
	NI_ASSERT( dynamic_cast<CTrainLocomotive*>( _pUnit ) != 0, "Wrong unit type (not Train Locomotive)" );
	CTrainLocomotive *pUnit = checked_cast<CTrainLocomotive*>( _pUnit );
	if ( pUnit )
		return new CLocomotiveAttackUnitState( pUnit, _pTarget );
	else
		return 0;
}

CLocomotiveAttackUnitState::CLocomotiveAttackUnitState( CTrainLocomotive *_pUnit, CAIUnit *_pTarget ) :
pUnit( _pUnit ), pTarget( _pTarget ), eState( EAS_STARTING ), pPath( 0 )
{
	pUnit->SetMovingTileLocks( true );

	// Set all cars to attack
	SAIUnitCmd cmd( ACTION_COMMAND_TRAIN_ATTACK_UNIT, _pTarget->GetUniqueId() );
	_pUnit->PassCommandToAll( new CAICommand( cmd ) );
}

void CLocomotiveAttackUnitState::Segment()
{
	switch( eState ) 
	{
	case EAS_STARTING:
		{
			if ( !pTarget->IsAlive() ||
				!pTarget->IsVisible( pUnit->GetParty() ) )
			{
				TryInterruptState( 0 );
				break;
			}

			// Create path
			pPath = new CLocomotivePath;
			pPath->Init( pUnit, pTarget->GetCenterPlain() );

			eState = EAS_MOVING;
		}
		break;
	case EAS_MOVING:
		{
			if ( TryFiring() )
			{
				if ( pPath->IsFinished() )
					eState = EAS_FIRING;
			}
			else
			{
				if ( pPath->IsFinished() )
				{
					pUnit->SendAcknowledgement( ACK_NEGATIVE_NOTIFICATION );
					eState = EAS_STARTING;
				}
			}
		}
		break;
	case EAS_FIRING:
		{
			if ( !TryFiring() )
			{
				pPath = 0;
				eState = EAS_STARTING;
			}
		}
		break;
	}
}

const bool CLocomotiveAttackUnitState::TryFiring()
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

	bResult |= pUnit->TryFiringAll();

	return bResult;
}

ETryStateInterruptResult CLocomotiveAttackUnitState::TryInterruptState( class CAICommand *pCommand )
{
	for ( int i = 0; i < pUnit->GetNGuns(); ++i )
		pUnit->GetGun( i )->StopFire();

	pUnit->Stop();
	pUnit->SetMovingTileLocks( false );
	pUnit->SetCommandFinished();

	return TSIR_YES_IMMIDIATELY;
}

//	CLocomotiveAttackObjState

IUnitState* CLocomotiveAttackObjState::Instance( CAIUnit *_pUnit, CStaticObject *_pTarget )
{
	NI_ASSERT( dynamic_cast<CTrainLocomotive*>( _pUnit ) != 0, "Wrong unit type (not Train Locomotive)" );
	CTrainLocomotive *pUnit = checked_cast<CTrainLocomotive*>( _pUnit );
	if ( pUnit )
		return new CLocomotiveAttackObjState( pUnit, _pTarget );
	else
		return 0;
}

CLocomotiveAttackObjState::CLocomotiveAttackObjState( CTrainLocomotive *_pUnit, CStaticObject *_pTarget ) :
pUnit( _pUnit ), pTarget( _pTarget ), eState( EAS_STARTING ), pPath( 0 )
{
	pUnit->SetMovingTileLocks( true );

	// Set all cars to attack
	SAIUnitCmd cmd( ACTION_COMMAND_TRAIN_ATTACK_OBJECT, _pTarget->GetUniqueId() );
	_pUnit->PassCommandToAll( new CAICommand( cmd ) );
}

void CLocomotiveAttackObjState::Segment()
{
	switch( eState ) 
	{
	case EAS_STARTING:
		{
			if ( !pTarget->IsAlive() ||
				!pTarget->IsVisible( pUnit->GetParty() ) )
			{
				TryInterruptState( 0 );
				break;
			}

			// Create path
			pPath = new CLocomotivePath;
			pPath->Init( pUnit, GetTargetCenter() );

			eState = EAS_MOVING;
		}
		break;
	case EAS_MOVING:
		{
			if ( TryFiring() )
			{
				if ( pPath->IsFinished() )
					eState = EAS_FIRING;
			}
			else
			{
				if ( pPath->IsFinished() )
				{
					pUnit->SendAcknowledgement( ACK_NEGATIVE_NOTIFICATION );
					eState = EAS_STARTING;
				}
			}
		}
		break;
	case EAS_FIRING:
		{
			if ( !TryFiring() )
			{
				pPath = 0;
				eState = EAS_STARTING;
			}
		}
		break;
	}
}

const bool CLocomotiveAttackObjState::TryFiring()
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

	bResult |= pUnit->TryFiringAll();

	return bResult;
}

ETryStateInterruptResult CLocomotiveAttackObjState::TryInterruptState( class CAICommand *pCommand )
{
	for ( int i = 0; i < pUnit->GetNGuns(); ++i )
		pUnit->GetGun( i )->StopFire();

	pUnit->Stop();
	pUnit->SetMovingTileLocks( false );
	pUnit->SetCommandFinished();

	return TSIR_YES_IMMIDIATELY;
}

REGISTER_SAVELOAD_CLASS( 0x19171400, CTrainLocomotive );
REGISTER_SAVELOAD_CLASS( 0x19171401, CLocomotiveStatesFactory );
REGISTER_SAVELOAD_CLASS( 0x19172B80, CLocomotivePath );
REGISTER_SAVELOAD_CLASS( 0x19172C80, CLocomotiveMoveState );
REGISTER_SAVELOAD_CLASS( 0x19173C40, CLocomotiveAttackUnitState );
REGISTER_SAVELOAD_CLASS( 0x19174380, CLocomotiveAttackObjState );
REGISTER_SAVELOAD_CLASS( 0x191BF340, CTrainRestState );
REGISTER_SAVELOAD_CLASS( 0x1926A3C0, SLocomotivePathMemento );

