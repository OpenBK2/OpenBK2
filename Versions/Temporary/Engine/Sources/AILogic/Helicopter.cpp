#include "stdafx.h"

#include "Helicopter.h"
#include "HelicopterStates.h"
#include "AIClassesID.h"
#include "AIGeometry.h"
#include "GroupLogic.h"
#include "Guns.h"
#include "NewUpdater.h"
#include "UnitsIterators2.h"

#include "Common_RTS_AI/AIMap.h"
#include "Common_RTS_AI/StaticMapHeights.h"
#include "System/FastMath.h"

REGISTER_SAVELOAD_CLASS( AI_HELICOPTER, CHelicopter );

extern CEventUpdater updater;
extern CGroupLogic theGroupLogic;
extern CDiplomacy theDipl;
extern NTimer::STime curTime;

namespace
{
	const WORD HELICOPTER_MOVE_WHILE_TURNING_ANGLE = 4096; // 22.5 degrees.
	const float HELICOPTER_LEAVE_DISTANCE = 512.0f;

	const CVec2 GetSafeDirection( const CVec2 &vDirection, const CVec2 &vFallback )
	{
		CVec2 vResult( vDirection );
		if ( fabs2( vResult ) < 0.0001f )
			vResult = vFallback;
		if ( fabs2( vResult ) < 0.0001f )
			vResult = CVec2( 1.0f, 0.0f );
		Normalize( &vResult );
		return vResult;
	}

	float ApproachAngle( const float fCurrent, const float fTarget, const float fMaxStep )
	{
		const float fDelta = Clamp( fTarget - fCurrent, -fMaxStep, fMaxStep );
		return fCurrent + fDelta;
	}

	WORD InterpolateDirection( const WORD wStart, const WORD wFinish, const float fCoeff )
	{
		int nDelta = int( wFinish ) - int( wStart );
		if ( nDelta > 32767 )
			nDelta -= 65536;
		else if ( nDelta < -32768 )
			nDelta += 65536;

		return WORD( int( wStart ) + Float2Int( float( nDelta ) * fCoeff ) );
	}
}

CHelicopter::CHelicopter()
: vHeliNextPos( VNULL3 ),
	vHeliNextSpeed( VNULL3 ),
	vHeliNextNormal( V3_AXIS_Z ),
	vMoveTarget( VNULL2 ),
	vVisualAimPoint( VNULL3 ),
	bMoveTargetSet( false ),
	bVisualAimPointSet( false ),
	bAttackTilt( false ),
	bDeadSpiralStarted( false ),
	vDeathStartPos( VNULL3 ),
	vDeathVelocity( VNULL3 ),
	fDeathGroundZ( 0.0f ),
	fDeathSpiralAngle( 0.0f ),
	fDeathSelfRotation( 0.0f ),
	fMovementVisualAngle( 0.0f ),
	fSideVisualAngle( 0.0f ),
	fVisualStartMovementAngle( 0.0f ),
	fVisualFinishMovementAngle( 0.0f ),
	fVisualStartSideAngle( 0.0f ),
	fVisualFinishSideAngle( 0.0f ),
	fAttackVisualAngle( 0.0f ),
	fVisualStartAttackAngle( 0.0f ),
	fVisualFinishAttackAngle( 0.0f ),
	timeDeathStarted( 0 ),
	wVisualStartDirection( 0 ),
	wVisualFinishDirection( 0 ),
	wDesiredDirection( 0 ),
	bDesiredDirectionSet( false )
{
}

void CHelicopter::Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *_pStats, const float fHP, const WORD dir, const BYTE player, ICollisionsCollector *pCollisionsCollector )
{
	pStats = checked_cast<const SMechUnitRPGStats*>( _pStats );
	vInitialPoint = center;
	timeNextGroundScan = curTime;
	fFuel = pStats->fFuel;
	bBombsAutocast = false;

	CMilitaryCar::Init( center, z, pStats, fHP, dir, player, pCollisionsCollector );
	const float fFlightZ = GetFlightZ( center );
	SetCenter( CVec3( center, fFlightZ ) );

	vHeliNextPos = GetCenter();
	vHeliNextSpeed = VNULL3;
	vHeliNextNormal = V3_AXIS_Z;
	vPos = vHeliNextPos;
	vSpeed = vHeliNextSpeed;
	vNormale = vHeliNextNormal;

	SetSpeed( 0.0f );
	wVisualStartDirection = dir;
	wVisualFinishDirection = dir;
	wDesiredDirection = dir;
	bDesiredDirectionSet = false;
	fVisualStartMovementAngle = 0.0f;
	fVisualFinishMovementAngle = 0.0f;
	fVisualStartSideAngle = 0.0f;
	fVisualFinishSideAngle = 0.0f;
	fAttackVisualAngle = 0.0f;
	fVisualStartAttackAngle = 0.0f;
	fVisualFinishAttackAngle = 0.0f;
}

IStatesFactory* CHelicopter::GetStatesFactory() const
{
	return CHelicopterStatesFactory::Instance();
}

const NDb::SHelicopterStats* CHelicopter::GetHelicopterStats() const
{
	return pStats && pStats->pHelicopterStats ? pStats->pHelicopterStats.GetPtr() : 0;
}

float CHelicopter::GetFlightZ( const CVec2 &vPoint ) const
{
	const CVec2 vDir( GetSafeDirection( GetFrontDirectionVector(), CVec2( 1.0f, 0.0f ) ) );
	const CVec2 vSide( vDir.y, -vDir.x );
	const CVec2 vCenterShift( vDir * pStats->vAABBCenter.y + vSide * pStats->vAABBCenter.x );
	const CVec2 vSampleCenter( vPoint + vCenterShift );
	const float fHalfLength = pStats->vAABBHalfSize.y;
	const float fHalfWidth = pStats->vAABBHalfSize.x;

	CStaticMapHeights *pHeights = GetAIMap()->GetHeights();
	float fGround = pHeights->GetZ( vSampleCenter );
	fGround += pHeights->GetZ( vSampleCenter + vDir * fHalfLength + vSide * fHalfWidth );
	fGround += pHeights->GetZ( vSampleCenter + vDir * fHalfLength - vSide * fHalfWidth );
	fGround += pHeights->GetZ( vSampleCenter - vDir * fHalfLength + vSide * fHalfWidth );
	fGround += pHeights->GetZ( vSampleCenter - vDir * fHalfLength - vSide * fHalfWidth );

	// Helicopters keep a fixed height above terrain, but the terrain sample follows the whole hull.
	return fGround / 5.0f + pStats->fMaxHeight;
}

void CHelicopter::BeginMoveTo( const CVec2 &vTarget )
{
	vMoveTarget = vTarget;
	bMoveTargetSet = true;
	bAttackTilt = false;
	bVisualAimPointSet = false;
}

void CHelicopter::BeginHover()
{
	bMoveTargetSet = false;
	vHeliNextSpeed = VNULL3;
	SetSpeed( 0.0f );
}

void CHelicopter::Stop()
{
	BeginHover();
	StopTurning();
	bAttackTilt = false;
	bVisualAimPointSet = false;
}

const bool CHelicopter::TurnToDirection( const WORD wDirection, const bool bCanBackward, const bool bCanForward )
{
	// Queue every body-yaw request. Only SecondSegment advances the deterministic rotation.
	(void)bCanBackward;
	(void)bCanForward;
	SetDirection( wDirection );
	return DirsDifference( GetFrontDirection(), wDesiredDirection ) == 0;
}

void CHelicopter::SetDirection( const WORD wDirection, const bool bNeedUpdate )
{
	(void)bNeedUpdate;
	wDesiredDirection = wDirection;
	bDesiredDirectionSet = true;
}

bool CHelicopter::AdvanceDesiredDirection( const NTimer::STime timeDiff )
{
	if ( !bDesiredDirectionSet )
		return true;

	const WORD wClockWise = wDesiredDirection - GetFrontDirection();
	const WORD wAntiClockWise = GetFrontDirection() - wDesiredDirection;
	const WORD wTurnRemaining = (std::min)( wClockWise, wAntiClockWise );
	const int nMaxTurn = (std::max)( 1, Float2Int( GetTurnSpeed() * float( timeDiff ) ) );

	if ( wTurnRemaining <= nMaxTurn )
	{
		// Bypass the queued override only for the deterministic rotation step itself.
		CBasePathUnit::SetDirection( wDesiredDirection );
		CBasePathUnit::StopTurning();
		bDesiredDirectionSet = false;
		return true;
	}

	const int nTurn = wClockWise < wAntiClockWise ? nMaxTurn : -nMaxTurn;
	CBasePathUnit::SetDirection( GetFrontDirection() + nTurn );
	return false;
}

bool CHelicopter::IsNearTarget( const CVec2 &vTarget, const float fRadius ) const
{
	return fabs2( GetCenterPlain() - vTarget ) <= sqr( fRadius );
}

void CHelicopter::AimAtPoint( const CVec2 &vPoint )
{
	vVisualAimPoint = CVec3( vPoint, GetAIMap()->GetHeights()->GetZ( vPoint ) );
	bVisualAimPointSet = true;
	if ( fabs2( vPoint - GetCenterPlain() ) > 0.0001f )
		TurnToDirection( GetDirectionByVector( vPoint - GetCenterPlain() ), false, true );
}

void CHelicopter::AimAtUnit( CAIUnit *pTarget, CBasicGun *pGun )
{
	if ( !pTarget )
		return;

	AimAtPoint( pTarget->GetCenterPlain() );
	vVisualAimPoint = pTarget->GetCenter();

	if ( pGun && !pGun->CanShootWOGunTurn( pTarget, 1 ) )
	{
		// Fixed or limited-angle weapons may require the whole helicopter to face their arc.
		const WORD wTargetDirection = GetDirectionByVector( pTarget->GetCenterPlain() - GetCenterPlain() );
		TurnToDirection( wTargetDirection - pGun->GetGun().wDirection, false, true );
	}
}

void CHelicopter::SetB2( const CVec3 &_vPos, const CVec3 &_vSpeed, const CVec3 &_vNormal )
{
	vHeliNextPos = _vPos;
	vHeliNextSpeed = _vSpeed;
	vHeliNextNormal = _vNormal;
	SetCenter( _vPos );
	SetSpeed( fabs( CVec2( _vSpeed.x, _vSpeed.y ) ) );
	wVisualStartDirection = GetFrontDirection();
	wVisualFinishDirection = GetFrontDirection();
	wDesiredDirection = GetFrontDirection();
	bDesiredDirectionSet = false;
	fVisualStartMovementAngle = fMovementVisualAngle;
	fVisualFinishMovementAngle = fMovementVisualAngle;
	fVisualStartSideAngle = fSideVisualAngle;
	fVisualFinishSideAngle = fSideVisualAngle;
	fVisualStartAttackAngle = fAttackVisualAngle;
	fVisualFinishAttackAngle = fAttackVisualAngle;
}

void CHelicopter::UpdatePlacement( const CVec3 &vOldPosition, const WORD wOldDirection, const bool bNeedUpdate )
{
	// Helicopter placement builds its own visual quaternion, but AI cells/profiles still must be updated.
	wVisualStartDirection = bNeedUpdate ? wOldDirection : GetFrontDirection();
	wVisualFinishDirection = GetFrontDirection();
	CAIUnit::UpdatePlacement( vOldPosition, wOldDirection, bNeedUpdate );
}

void CHelicopter::Segment()
{
	vSpeed = vHeliNextSpeed;
	vPos = vHeliNextPos;
	vNormale = vHeliNextNormal;

	CMilitaryCar::Segment();
	if ( !IsAlive() )
		return;

	// Keep the same "being attacked by aviation" acknowledgement behavior as planes.
	if ( curTime > timeNextGroundScan )
	{
		for ( CUnitsIter<1,3> iter( GetParty(), EDI_ENEMY, GetCenterPlain(), 1000 ); !iter.IsFinished(); iter.Iterate() )
		{
			CAIUnit *pUnit = *iter;
			if ( pUnit->IsRefValid() && pUnit->IsAlive() )
			{
				pUnit->SendAcknowledgement( NDb::ACK_BEING_ATTACKED_BY_AVIATION, pUnit->GetPlayer() != theDipl.GetMyNumber() );
				break;
			}
		}

		timeNextGroundScan = curTime + 1000 + NRandom::Random( 0, 2000 ); RecordRandomCall();
	}
}

void CHelicopter::SecondSegment( const NTimer::STime timeDiff )
{
	if ( bDeadSpiralStarted )
	{
		AdvanceDeathSpiral( timeDiff );
		CallUpdatePlacement();
		return;
	}

	vHeliNextSpeed = VNULL3;
	vHeliNextNormal = V3_AXIS_Z;

	if ( bMoveTargetSet )
	{
		const CVec2 vOldPoint( GetCenterPlain() );
		const CVec2 vToTarget( vMoveTarget - vOldPoint );
		const float fDist = fabs( vToTarget );
		const float fMaxMove = GetMaxPossibleSpeed() * float( timeDiff );

		if ( fDist <= 0.001f )
		{
			vHeliNextPos = CVec3( vMoveTarget, GetFlightZ( vMoveTarget ) );
			bMoveTargetSet = false;
		}
		else
		{
			CVec2 vDir( vToTarget );
			Normalize( &vDir );
			const WORD wMoveDirection = GetDirectionByVector( vDir );
			const WORD wTurnBefore = DirsDifference( wMoveDirection, GetFrontDirection() );
			TurnToDirection( wMoveDirection, false, true );
			AdvanceDesiredDirection( timeDiff );

			// Large course changes still turn in place. Small corrections may move at reduced
			// speed while the deterministic body rotation catches up.
			if ( wTurnBefore <= HELICOPTER_MOVE_WHILE_TURNING_ANGLE )
			{
				const float fMoveScale = Clamp( 1.0f - float( wTurnBefore ) / float( HELICOPTER_MOVE_WHILE_TURNING_ANGLE ), 0.25f, 1.0f );
				const float fMove = fMaxMove * fMoveScale;
				if ( fDist <= (std::max)( fMove, 0.001f ) )
				{
					vHeliNextPos = CVec3( vMoveTarget, GetFlightZ( vMoveTarget ) );
					bMoveTargetSet = false;
				}
				else
				{
					const CVec2 vNewPoint( vOldPoint + vDir * fMove );
					vHeliNextPos = CVec3( vNewPoint, GetFlightZ( vNewPoint ) );
					vHeliNextSpeed = ( vHeliNextPos - GetCenter() ) / float( timeDiff );
				}
			}
			else
			{
				vHeliNextPos = CVec3( vOldPoint, GetFlightZ( vOldPoint ) );
			}
		}
	}
	else
	{
		vHeliNextPos = CVec3( GetCenterPlain(), GetFlightZ( GetCenterPlain() ) );
		AdvanceDesiredDirection( timeDiff );
	}

	if ( const NDb::SHelicopterStats *pHeliStats = GetHelicopterStats() )
	{
		const float fSeconds = float( timeDiff ) / 1000.0f;
		const bool bTranslating = fabs( CVec2( vHeliNextSpeed.x, vHeliNextSpeed.y ) ) > 0.001f;
		const float fTargetMoveAngle = bTranslating && !bAttackTilt ? pHeliStats->fMovmentAngleDownRadians : 0.0f;
		float fTargetSideAngle = 0.0f;
		float fTargetAttackAngle = 0.0f;
		if ( bAttackTilt && bVisualAimPointSet )
		{
			const float fHorizontalDist = (std::max)( fabs( CVec2( vVisualAimPoint.x, vVisualAimPoint.y ) - GetCenterPlain() ), 1.0f );
			fTargetAttackAngle = Clamp( atan2f( (std::max)( 0.0f, GetCenter().z - vVisualAimPoint.z ), fHorizontalDist ),
				0.0f, pHeliStats->fMaxAttackAngleDownRadians );
		}
		// Attack orientation uses only yaw and forward pitch; do not add a banking axis.
		if ( !bAttackTilt && !bTranslating && ( bMoveTargetSet || bVisualAimPointSet || IsTurning() ) )
		{
			const CVec2 vAimPoint( bVisualAimPointSet ? CVec2( vVisualAimPoint.x, vVisualAimPoint.y ) : vMoveTarget );
			const CVec2 vAimDir( vAimPoint - GetCenterPlain() );
			if ( fabs2( vAimDir ) > 0.0001f )
			{
				const WORD wDesired = GetDirectionByVector( vAimDir );
				if ( DirsDifference( wDesired, GetFrontDirection() ) > 64 )
				{
					const WORD wClockWise = wDesired - GetFrontDirection();
					// The visual side tilt follows the model basis, which is opposite to the yaw turn sign.
					const float fSign = wClockWise < 32768 ? -1.0f : 1.0f;
					fTargetSideAngle = fSign * pHeliStats->fSideRotatingAngleRad;
				}
			}
		}

		// Store visual-only tilt endpoints so the renderer can interpolate between deterministic AI segments.
		fVisualStartMovementAngle = fMovementVisualAngle;
		fVisualStartSideAngle = fSideVisualAngle;
		fVisualStartAttackAngle = fAttackVisualAngle;
		fMovementVisualAngle = ApproachAngle( fMovementVisualAngle, fTargetMoveAngle, pHeliStats->fMovementAngleDownSpeedRPS * fSeconds );
		fSideVisualAngle = ApproachAngle( fSideVisualAngle, fTargetSideAngle, pHeliStats->fSideRotatingAngleRPS * fSeconds );
		// Attack pitch is visual-only, but advances from deterministic AI-segment endpoints.
		fAttackVisualAngle = ApproachAngle( fAttackVisualAngle, fTargetAttackAngle, pHeliStats->fMovementAngleDownSpeedRPS * fSeconds );
		fVisualFinishMovementAngle = fMovementVisualAngle;
		fVisualFinishSideAngle = fSideVisualAngle;
		fVisualFinishAttackAngle = fAttackVisualAngle;
	}
	else
	{
		fMovementVisualAngle = 0.0f;
		fSideVisualAngle = 0.0f;
		fVisualStartMovementAngle = 0.0f;
		fVisualFinishMovementAngle = 0.0f;
		fVisualStartSideAngle = 0.0f;
		fVisualFinishSideAngle = 0.0f;
		fAttackVisualAngle = 0.0f;
		fVisualStartAttackAngle = 0.0f;
		fVisualFinishAttackAngle = 0.0f;
	}

	SetCenter( vHeliNextPos );
	SetSpeed( fabs( CVec2( vHeliNextSpeed.x, vHeliNextSpeed.y ) ) );
	CallUpdatePlacement();
}

void CHelicopter::GetPlacement( SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff )
{
	pPlacement->bNewFormat = true;
	pPlacement->nObjUniqueID = GetUniqueId();

	const float fCoeff = Clamp( float( timeDiff ) / float( SConsts::AI_SEGMENT_DURATION ), 0.0f, 1.0f );
	const float fVisualCoeff = 1.0f - fCoeff;
	const WORD wVisualDirection = InterpolateDirection( wVisualStartDirection, wVisualFinishDirection, fVisualCoeff );
	const CVec2 vVisualFrontDirection( GetVectorByDirection( wVisualDirection ) );
	const float fVisualMovementAngle = fVisualStartMovementAngle + ( fVisualFinishMovementAngle - fVisualStartMovementAngle ) * fVisualCoeff;
	const float fVisualSideAngle = fVisualStartSideAngle + ( fVisualFinishSideAngle - fVisualStartSideAngle ) * fVisualCoeff;
	const float fVisualAttackAngle = fVisualStartAttackAngle + ( fVisualFinishAttackAngle - fVisualStartAttackAngle ) * fVisualCoeff;
	const CVec3 vInterpolatedSpeed( GetSpeedNext() - fCoeff * ( GetSpeedNext() - vSpeed ) );
	const CVec3 vInterpolatedNormal( GetNormalNext() - fCoeff * ( GetNormalNext() - vNormale ) );
	const CVec3 vInterpolatedPos( GetPosNext() - fCoeff * ( GetPosNext() - vPos ) );

	CVec3 vVisualSpeed( vInterpolatedSpeed );
	CVec3 vVisualPos( vInterpolatedPos );
	float fVisualDeathSelfRotation = fDeathSelfRotation;
	float fVisualDeathDownwardsAngle = 0.1f;
	if ( !bDeadSpiralStarted && fabs( CVec2( vVisualSpeed.x, vVisualSpeed.y ) ) < 0.001f )
		vVisualSpeed = CVec3( vVisualFrontDirection, 0.0f );

	const NDb::SHelicopterStats *pHeliStats = GetHelicopterStats();
	if ( bDeadSpiralStarted )
	{
		const float fDownAcceleration = (std::max)( 0.001f, pHeliStats ? pHeliStats->fSpiralDownAcceleration : 15.0f );
		const float fRadius = (std::max)( 0.0f, pHeliStats ? pHeliStats->fSpiralRadius : 11.0f );
		const float fSteps = pHeliStats ? pHeliStats->fSpiralSteps : 2.5f;
		const float fSelfRotSpeed = pHeliStats ? pHeliStats->fDeathSelfPointRotationSpeedRad : 2.1f;
		const float fFallHeight = (std::max)( 0.001f, vDeathStartPos.z - fDeathGroundZ );
		const float fOldProgress = Clamp( ( vDeathStartPos.z - vPos.z ) / fFallHeight, 0.0f, 1.0f );
		const float fOldFallDistance = fOldProgress * fFallHeight;
		const float fOldDownSpeed = sqrtf( 2.0f * fDownAcceleration * fOldFallDistance );
		const float fVisualSeconds = fVisualCoeff * float( SConsts::AI_SEGMENT_DURATION ) / 1000.0f;
		const float fVisualFallDistance = (std::min)( fFallHeight,
			fOldFallDistance + fOldDownSpeed * fVisualSeconds + 0.5f * fDownAcceleration * sqr( fVisualSeconds ) );
		const float fVisualProgress = Clamp( fVisualFallDistance / fFallHeight, 0.0f, 1.0f );
		const float fOldAngle = fSteps * FP_2PI * fOldProgress;
		const float fVisualAngle = fSteps * FP_2PI * fVisualProgress;
		const CVec2 vSpiralDirection( GetSafeDirection( CVec2( vDeathVelocity.x, vDeathVelocity.y ), GetFrontDirectionVector() ) );
		const CVec2 vSpiralSide( -vSpiralDirection.y, vSpiralDirection.x );
		const CVec2 vOldSpiralLocal( NMath::Cos( fOldAngle ) * fRadius * fOldProgress,
			NMath::Sin( fOldAngle ) * fRadius * fOldProgress );
		const CVec2 vVisualSpiralLocal( NMath::Cos( fVisualAngle ) * fRadius * fVisualProgress,
			NMath::Sin( fVisualAngle ) * fRadius * fVisualProgress );
		const CVec2 vOldSpiral( vSpiralDirection * vOldSpiralLocal.x + vSpiralSide * vOldSpiralLocal.y );
		const CVec2 vVisualSpiral( vSpiralDirection * vVisualSpiralLocal.x + vSpiralSide * vVisualSpiralLocal.y );
		const float fVisualElapsedMs = fVisualCoeff * float( SConsts::AI_SEGMENT_DURATION );

		// Evaluate the accelerating fall and spiral between deterministic AI endpoints.
		const CVec2 vVisualPoint( CVec2( vPos.x, vPos.y ) + CVec2( vDeathVelocity.x, vDeathVelocity.y ) * fVisualElapsedMs
			+ ( vVisualSpiral - vOldSpiral ) );
		vVisualPos.x = vVisualPoint.x;
		vVisualPos.y = vVisualPoint.y;
		vVisualPos.z = vDeathStartPos.z - fVisualFallDistance;

		fVisualDeathSelfRotation = fSelfRotSpeed * sqrtf( 2.0f * fVisualFallDistance / fDownAcceleration );
		fVisualDeathDownwardsAngle = Clamp( pHeliStats ? pHeliStats->fDeathSpiralDownwardsAngleRad : 0.1f, 0.0f, FP_PI2 );
	}
	else if ( pHeliStats && fabs( fVisualMovementAngle ) > 0.001f )
	{
		vVisualSpeed.z = -fabs( CVec2( vVisualSpeed.x, vVisualSpeed.y ) ) * tanf( fVisualMovementAngle );
	}

	CVec3 vVisualNormal( fabs( vInterpolatedNormal ) > 0.001f ? vInterpolatedNormal : V3_AXIS_Z );
	if ( pHeliStats && !bDeadSpiralStarted && !bAttackTilt && fabs( fVisualSideAngle ) > 0.001f )
	{
		const CVec2 vSide( vVisualFrontDirection.y, -vVisualFrontDirection.x );
		vVisualNormal += CVec3( vSide * tanf( fVisualSideAngle ), 0.0f );
	}

	Normalize( &vVisualNormal );
	if ( bDeadSpiralStarted )
	{
		// Keep the death heading fixed so only the configured local-Z spin changes yaw, then apply a stable pitch.
		const float fDeathYaw = float( GetFrontDirection() ) / 65536.0f * FP_2PI;
		pPlacement->rotation = CQuat( fDeathYaw, V3_AXIS_Z )
			* CQuat( fVisualDeathSelfRotation, V3_AXIS_Z )
			* CQuat( -fVisualDeathDownwardsAngle, V3_AXIS_X );
	}
	else if ( ( bAttackTilt && bVisualAimPointSet ) || fabs( fVisualAttackAngle ) > 0.001f )
	{
		// Smoothly enter and leave the attack pose, using only yaw (Z) and local forward pitch (X).
		const float fYaw = float( wVisualDirection ) / 65536.0f * FP_2PI;
		pPlacement->rotation = CQuat( fYaw, V3_AXIS_Z ) * CQuat( -fVisualAttackAngle, V3_AXIS_X );
	}
	else
		MakeQuatBySpeedAndNormale( &pPlacement->rotation, vVisualSpeed, vVisualNormal );
	pPlacement->dir = wVisualDirection;
	pPlacement->vPlacement = vVisualPos;
	pPlacement->fSpeed = GetSpeed();
	pPlacement->fWaterCoeff = 0.0f;
	pPlacement->cSoil = 0;
}

void CHelicopter::DecFuel( const bool bEconomyMode )
{
	float fDec = ( bEconomyMode ? SConsts::PLANE_FUEL_DEC_ECONOMY : SConsts::PLANE_FUEL_DEC ) * SConsts::AI_SEGMENT_DURATION / 1000;
	if ( bEconomyMode )
	{
		if ( const NDb::SHelicopterStats *pHeliStats = GetHelicopterStats() )
			fDec *= pHeliStats->fStandingFuelDrainModifier;
	}

	fFuel = (std::max)( fFuel - fDec, 0.0f );
	updater.AddUpdate( 0, ACTION_NOTIFY_RPG_CHANGED, this, -1 );
}

void CHelicopter::Die( const bool fromExplosion, const float fDamage )
{
	vDeathVelocity = vHeliNextSpeed;
	// Start immediately; waiting for the queued fly-dead state leaves a visible pause after the hit.
	StartDeathSpiral();
	CAviation::Die( fromExplosion, fDamage );
}

void CHelicopter::StartDeathSpiral()
{
	if ( bDeadSpiralStarted )
		return;

	bMoveTargetSet = false;
	bAttackTilt = false;
	bVisualAimPointSet = false;
	fAttackVisualAngle = 0.0f;
	fVisualStartAttackAngle = 0.0f;
	fVisualFinishAttackAngle = 0.0f;
	bDeadSpiralStarted = true;
	vDeathStartPos = GetCenter();
	fDeathGroundZ = GetAIMap()->GetHeights()->GetZ( GetCenterPlain() );
	fDeathSpiralAngle = 0.0f;
	fDeathSelfRotation = 0.0f;
	fMovementVisualAngle = 0.0f;
	fSideVisualAngle = 0.0f;
	fVisualStartMovementAngle = 0.0f;
	fVisualFinishMovementAngle = 0.0f;
	fVisualStartSideAngle = 0.0f;
	fVisualFinishSideAngle = 0.0f;
	timeDeathStarted = curTime;
}

bool CHelicopter::AdvanceDeathSpiral( const NTimer::STime timeDiff )
{
	const NDb::SHelicopterStats *pHeliStats = GetHelicopterStats();
	const float fDownAcceleration = (std::max)( 0.001f, pHeliStats ? pHeliStats->fSpiralDownAcceleration : 15.0f );
	const float fRadius = (std::max)( 0.0f, pHeliStats ? pHeliStats->fSpiralRadius : 11.0f );
	const float fSteps = pHeliStats ? pHeliStats->fSpiralSteps : 2.5f;
	const float fSelfRotSpeed = pHeliStats ? pHeliStats->fDeathSelfPointRotationSpeedRad : 2.1f;
	const float fSeconds = float( timeDiff ) / 1000.0f;
	const float fFallHeight = (std::max)( 0.001f, vDeathStartPos.z - fDeathGroundZ );
	const float fOldProgress = Clamp( ( vDeathStartPos.z - GetCenter().z ) / fFallHeight, 0.0f, 1.0f );
	const float fOldFallDistance = fOldProgress * fFallHeight;
	const float fOldDownSpeed = sqrtf( 2.0f * fDownAcceleration * fOldFallDistance );
	// Integrating from the distance-derived speed keeps the simulation deterministic
	// without adding another serialized velocity field.
	const float fNewFallDistance = (std::min)( fFallHeight,
		fOldFallDistance + fOldDownSpeed * fSeconds + 0.5f * fDownAcceleration * sqr( fSeconds ) );
	const float fNewZ = vDeathStartPos.z - fNewFallDistance;
	const float fNewProgress = Clamp( fNewFallDistance / fFallHeight, 0.0f, 1.0f );
	const float fOldSpiralAngle = fSteps * FP_2PI * fOldProgress;
	fDeathSpiralAngle = fSteps * FP_2PI * fNewProgress;
	fDeathSelfRotation = fSelfRotSpeed * sqrtf( 2.0f * fNewFallDistance / fDownAcceleration );

	// SpiralRadius is the final radius; grow it linearly from zero over the complete fall.
	const CVec2 vSpiralDirection( GetSafeDirection( CVec2( vDeathVelocity.x, vDeathVelocity.y ), GetFrontDirectionVector() ) );
	const CVec2 vSpiralSide( -vSpiralDirection.y, vSpiralDirection.x );
	const CVec2 vOldSpiralLocal( NMath::Cos( fOldSpiralAngle ) * fRadius * fOldProgress,
		NMath::Sin( fOldSpiralAngle ) * fRadius * fOldProgress );
	const CVec2 vNewSpiralLocal( NMath::Cos( fDeathSpiralAngle ) * fRadius * fNewProgress,
		NMath::Sin( fDeathSpiralAngle ) * fRadius * fNewProgress );
	const CVec2 vOldSpiral( vSpiralDirection * vOldSpiralLocal.x + vSpiralSide * vOldSpiralLocal.y );
	const CVec2 vNewSpiral( vSpiralDirection * vNewSpiralLocal.x + vSpiralSide * vNewSpiralLocal.y );
	const CVec2 vDrift( vDeathVelocity.x * float( timeDiff ), vDeathVelocity.y * float( timeDiff ) );
	const CVec2 vNewPoint( GetCenterPlain() + vDrift + ( vNewSpiral - vOldSpiral ) );

	vHeliNextSpeed = ( CVec3( vNewPoint, fNewZ ) - GetCenter() ) / float( timeDiff );
	vHeliNextPos = CVec3( vNewPoint, fNewZ );
	vHeliNextNormal = V3_AXIS_Z;
	SetCenter( vHeliNextPos );
	SetSpeed( fabs( CVec2( vHeliNextSpeed.x, vHeliNextSpeed.y ) ) );

	return fNewZ <= fDeathGroundZ + 0.1f;
}

CVec2 CHelicopter::GetLeavePoint() const
{
	const float fMapX = float( GetAIMap()->GetSizeX() * GetAIMap()->GetTileSize() );
	const float fMapY = float( GetAIMap()->GetSizeY() * GetAIMap()->GetTileSize() );
	const CVec2 vCur( GetCenterPlain() );

	// Project onto the closest map edge so returning helicopters always take the
	// shortest route, then continue beyond that edge before disappearing.
	float fClosestDistance = vCur.x;
	CVec2 vTarget( -HELICOPTER_LEAVE_DISTANCE, vCur.y );
	if ( fMapX - vCur.x < fClosestDistance )
	{
		fClosestDistance = fMapX - vCur.x;
		vTarget = CVec2( fMapX + HELICOPTER_LEAVE_DISTANCE, vCur.y );
	}
	if ( vCur.y < fClosestDistance )
	{
		fClosestDistance = vCur.y;
		vTarget = CVec2( vCur.x, -HELICOPTER_LEAVE_DISTANCE );
	}
	if ( fMapY - vCur.y < fClosestDistance )
		vTarget = CVec2( vCur.x, fMapY + HELICOPTER_LEAVE_DISTANCE );

	return vTarget;
}
