#include "stdafx.h"

#include "Helicopter.h"
#include "HelicopterStates.h"
#include "AIClassesID.h"
#include "AIGeometry.h"
#include "GroupLogic.h"
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
	timeDeathStarted( 0 )
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
	bAttackTilt = false;
	bVisualAimPointSet = false;
}

bool CHelicopter::IsNearTarget( const CVec2 &vTarget, const float fRadius ) const
{
	return fabs2( GetCenterPlain() - vTarget ) <= sqr( fRadius );
}

void CHelicopter::AimAtPoint( const CVec2 &vPoint )
{
	if ( fabs2( vPoint - GetCenterPlain() ) > 0.0001f )
		TurnToDirection( GetDirectionByVector( vPoint - GetCenterPlain() ), false, true );

	vVisualAimPoint = CVec3( vPoint, GetAIMap()->GetHeights()->GetZ( vPoint ) );
	bVisualAimPointSet = true;
}

void CHelicopter::AimAtUnit( const CAIUnit *pTarget )
{
	if ( !pTarget )
		return;

	AimAtPoint( pTarget->GetCenterPlain() );
	vVisualAimPoint = pTarget->GetCenter();
}

void CHelicopter::SetB2( const CVec3 &_vPos, const CVec3 &_vSpeed, const CVec3 &_vNormal )
{
	vHeliNextPos = _vPos;
	vHeliNextSpeed = _vSpeed;
	vHeliNextNormal = _vNormal;
	SetCenter( _vPos );
	SetSpeed( fabs( CVec2( _vSpeed.x, _vSpeed.y ) ) );
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

		if ( fDist <= (std::max)( fMaxMove, 0.001f ) )
		{
			vHeliNextPos = CVec3( vMoveTarget, GetFlightZ( vMoveTarget ) );
			bMoveTargetSet = false;
		}
		else
		{
			CVec2 vDir( vToTarget );
			Normalize( &vDir );
			const CVec2 vNewPoint( vOldPoint + vDir * fMaxMove );
			vHeliNextPos = CVec3( vNewPoint, GetFlightZ( vNewPoint ) );
			vHeliNextSpeed = ( vHeliNextPos - GetCenter() ) / float( timeDiff );
			TurnToDirection( GetDirectionByVector( vDir ), false, true );
		}
	}
	else
	{
		vHeliNextPos = CVec3( GetCenterPlain(), GetFlightZ( GetCenterPlain() ) );
	}

	if ( const NDb::SHelicopterStats *pHeliStats = GetHelicopterStats() )
	{
		const float fSeconds = float( timeDiff ) / 1000.0f;
		const float fTargetMoveAngle = bMoveTargetSet && !bAttackTilt ? pHeliStats->fMovmentAngleDownRadians : 0.0f;
		float fTargetSideAngle = 0.0f;
		if ( bMoveTargetSet || IsTurning() )
		{
			const CVec2 vAimPoint( bVisualAimPointSet ? CVec2( vVisualAimPoint.x, vVisualAimPoint.y ) : vMoveTarget );
			const WORD wDesired = GetDirectionByVector( vAimPoint - GetCenterPlain() );
			const WORD wClockWise = wDesired - GetFrontDirection();
			const float fSign = wClockWise < 32768 ? 1.0f : -1.0f;
			fTargetSideAngle = fSign * pHeliStats->fSideRotatingAngleRad;
		}

		// Visual tilt changes are smoothed in AI segments so replay/network state stays deterministic.
		fMovementVisualAngle = ApproachAngle( fMovementVisualAngle, fTargetMoveAngle, pHeliStats->fMovementAngleDownSpeedRPS * fSeconds );
		fSideVisualAngle = ApproachAngle( fSideVisualAngle, fTargetSideAngle, pHeliStats->fSideRotatingAngleRPS * fSeconds );
	}
	else
	{
		fMovementVisualAngle = 0.0f;
		fSideVisualAngle = 0.0f;
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
	const CVec3 vInterpolatedSpeed( GetSpeedNext() - fCoeff * ( GetSpeedNext() - vSpeed ) );
	const CVec3 vInterpolatedNormal( GetNormalNext() - fCoeff * ( GetNormalNext() - vNormale ) );
	const CVec3 vInterpolatedPos( GetPosNext() - fCoeff * ( GetPosNext() - vPos ) );

	CVec3 vVisualSpeed( vInterpolatedSpeed );
	if ( fabs( CVec2( vVisualSpeed.x, vVisualSpeed.y ) ) < 0.001f )
		vVisualSpeed = CVec3( GetFrontDirectionVector(), 0.0f );

	const NDb::SHelicopterStats *pHeliStats = GetHelicopterStats();
	if ( bDeadSpiralStarted )
	{
		vVisualSpeed.z -= (std::max)( 1.0f, pStats->fSpeed );
	}
	else if ( bAttackTilt && bVisualAimPointSet )
	{
		const float fHorizontalDist = (std::max)( fabs( CVec2( vVisualAimPoint.x, vVisualAimPoint.y ) - GetCenterPlain() ), 1.0f );
		const float fPitch = Clamp( atan2f( (std::max)( 0.0f, GetCenter().z - vVisualAimPoint.z ), fHorizontalDist ), 0.0f, 80.0f / 180.0f * FP_PI );
		vVisualSpeed.z = -fabs( CVec2( vVisualSpeed.x, vVisualSpeed.y ) ) * tanf( fPitch );
	}
	else if ( pHeliStats && fabs( fMovementVisualAngle ) > 0.001f )
	{
		vVisualSpeed.z = -fabs( CVec2( vVisualSpeed.x, vVisualSpeed.y ) ) * tanf( fMovementVisualAngle );
	}

	CVec3 vVisualNormal( fabs( vInterpolatedNormal ) > 0.001f ? vInterpolatedNormal : V3_AXIS_Z );
	if ( pHeliStats && !bDeadSpiralStarted && fabs( fSideVisualAngle ) > 0.001f )
	{
		const CVec2 vSide( GetFrontDirectionVector().y, -GetFrontDirectionVector().x );
		vVisualNormal += CVec3( vSide * tanf( fSideVisualAngle ), 0.0f );
	}

	Normalize( &vVisualNormal );
	MakeQuatBySpeedAndNormale( &pPlacement->rotation, vVisualSpeed, vVisualNormal );
	if ( bDeadSpiralStarted )
		pPlacement->rotation *= CQuat( fDeathSelfRotation, V3_AXIS_Y );
	pPlacement->vPlacement = vInterpolatedPos;
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
	CAviation::Die( fromExplosion, fDamage );
}

void CHelicopter::StartDeathSpiral()
{
	if ( bDeadSpiralStarted )
		return;

	bMoveTargetSet = false;
	bAttackTilt = false;
	bVisualAimPointSet = false;
	bDeadSpiralStarted = true;
	vDeathStartPos = GetCenter();
	fDeathGroundZ = GetAIMap()->GetHeights()->GetZ( GetCenterPlain() );
	fDeathSpiralAngle = 0.0f;
	fDeathSelfRotation = 0.0f;
	fMovementVisualAngle = 0.0f;
	fSideVisualAngle = 0.0f;
	timeDeathStarted = curTime;
}

bool CHelicopter::AdvanceDeathSpiral( const NTimer::STime timeDiff )
{
	const NDb::SHelicopterStats *pHeliStats = GetHelicopterStats();
	const float fDownSpeed = pHeliStats ? pHeliStats->fSpiralDownSpeed : 15.0f;
	const float fRadius = pHeliStats ? pHeliStats->fSpiralRadius : 11.0f;
	const float fSteps = pHeliStats ? pHeliStats->fSpiralSteps : 2.5f;
	const float fSelfRotSpeed = pHeliStats ? pHeliStats->fDeathSelfPointRotationSpeedRad : 2.1f;
	const float fSeconds = float( timeDiff ) / 1000.0f;
	const float fFallHeight = (std::max)( 1.0f, vDeathStartPos.z - fDeathGroundZ );
	const float fTotalFallTime = (std::max)( 0.001f, fFallHeight / fDownSpeed );

	const float fOldSpiralAngle = fDeathSpiralAngle;
	fDeathSpiralAngle += fSteps * FP_2PI * fSeconds / fTotalFallTime;
	fDeathSelfRotation += fSelfRotSpeed * fSeconds;

	const CVec2 vDrift( vDeathVelocity.x * float( timeDiff ), vDeathVelocity.y * float( timeDiff ) );
	const CVec2 vOldSpiral( NMath::Cos( fOldSpiralAngle ) * fRadius, NMath::Sin( fOldSpiralAngle ) * fRadius );
	const CVec2 vNewSpiral( NMath::Cos( fDeathSpiralAngle ) * fRadius, NMath::Sin( fDeathSpiralAngle ) * fRadius );
	const CVec2 vNewPoint( GetCenterPlain() + vDrift + ( vNewSpiral - vOldSpiral ) );
	const float fNewZ = (std::max)( fDeathGroundZ, GetCenter().z - fDownSpeed * fSeconds );

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
	const float fMargin = float( GetAIMap()->GetTileSize() );
	const CVec2 vCur( GetCenterPlain() );
	const CVec2 vDir( GetSafeDirection( GetFrontDirectionVector(), vCur - CVec2( fMapX * 0.5f, fMapY * 0.5f ) ) );

	float fScale = FLT_MAX;
	if ( vDir.x > 0.001f )
		fScale = (std::min)( fScale, ( fMapX - fMargin - vCur.x ) / vDir.x );
	else if ( vDir.x < -0.001f )
		fScale = (std::min)( fScale, ( fMargin - vCur.x ) / vDir.x );

	if ( vDir.y > 0.001f )
		fScale = (std::min)( fScale, ( fMapY - fMargin - vCur.y ) / vDir.y );
	else if ( vDir.y < -0.001f )
		fScale = (std::min)( fScale, ( fMargin - vCur.y ) / vDir.y );

	if ( fScale == FLT_MAX || fScale < 0.0f )
		fScale = (std::max)( fMapX, fMapY );

	return CVec2( Clamp( vCur.x + vDir.x * fScale, fMargin, fMapX - fMargin ),
		Clamp( vCur.y + vDir.y * fScale, fMargin, fMapY - fMargin ) );
}
