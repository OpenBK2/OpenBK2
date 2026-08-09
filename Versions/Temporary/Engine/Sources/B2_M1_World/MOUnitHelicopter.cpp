#include "stdafx.h"

#include "MOUnitHelicopter.h"
#include "UpdatableProcess.h"

#include "Main/GameTimer.h"

REGISTER_SAVELOAD_CLASS( 0x31197AC0, CMOUnitHelicopter );

class CHelicopterDeviationProcess : public IClientUpdatableProcess
{
	OBJECT_NOCOPY_METHODS( CHelicopterDeviationProcess );

	ZDATA
		CPtr<CMOUnitHelicopter> pUnit;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); return 0; }

	CHelicopterDeviationProcess() {}
public:
	CHelicopterDeviationProcess( CMOUnitHelicopter *_pUnit ) : pUnit( _pUnit ) {}

	bool Update( const NTimer::STime &time )
	{
		return IsValid( pUnit ) && pUnit->UpdateStandingDeviation( time );
	}
};

// Keep the existing process ID so helicopter hover deviation remains serializable.
REGISTER_SAVELOAD_CLASS( 0x31197AC2, CHelicopterDeviationProcess );

const NDb::SHelicopterStats *CMOUnitHelicopter::GetHelicopterStats() const
{
	const NDb::SMechUnitRPGStats *pStats = checked_cast<const NDb::SMechUnitRPGStats *>( GetStats() );
	return pStats->pHelicopterStats ? pStats->pHelicopterStats.GetPtr() : 0;
}

bool CMOUnitHelicopter::Create( const int nUniqueID, const SAIBasicUpdate *_pUpdate, NDb::ESeason eSeason,
	const NDb::EDayNight eDayTime, bool bInEditor )
{
	vStandingDeviationBasePlacement = VNULL3;
	vStandingDeviationOffset = VNULL3;
	vStandingDeviationTarget = VNULL3;
	timeStandingDeviationLastUpdate = GameTimer()->GetGameTime();
	dwStandingDeviationRandomState = DWORD( nUniqueID ) ^ 0xA511E9B3u;
	if ( dwStandingDeviationRandomState == 0 )
		dwStandingDeviationRandomState = 0x6D2B79F5u;
	bStandingDeviationActive = false;

	if ( !CMOUnitMechanical::Create( nUniqueID, _pUpdate, eSeason, eDayTime, bInEditor ) )
		return false;

	if ( !bInEditor && GetHelicopterStats() )
	{
		// Hover drift needs client-frame updates even while AI sends no movement notifications.
		NUpdatableProcess::Register( new CHelicopterDeviationProcess( this ) );

		const SAINewUnitUpdate *pUpdate = checked_cast<const SAINewUnitUpdate *>( _pUpdate );
		const NDb::SMechUnitRPGStats *pStats = checked_cast<const NDb::SMechUnitRPGStats *>( GetStats() );
		if ( pUpdate->info.fSpeed <= 0.0f && pStats->pSoundIdle )
		{
			// An initially stationary helicopter may not receive a movement update that starts this loop.
			AttachSound( EAST_IDLE, pStats->pSoundIdle, true );
		}
	}

	return true;
}

void CMOUnitHelicopter::SetupSpecialAnimationMutator( NAnimation::ISkeletonAnimator *pAnimator )
{
	// The shared mutator lets helicopter propeller rotations compose with Basis jogging.
	CMOUnitMechanical::SetupSpecialAnimationMutator( pAnimator );
	const NDb::SHelicopterStats *pHelicopterStats = GetHelicopterStats();
	if ( !pHelicopterStats || pHelicopterStats->propellerObjects.empty() ||
		pHelicopterStats->propellerSpeedsRad.empty() )
		return;

	GetOrCreateJoggingMutator()->SetupPropellers( pAnimator, pHelicopterStats->propellerObjects,
		pHelicopterStats->propellerSpeedsRad );
}

float CMOUnitHelicopter::NextStandingDeviationRandom()
{
	// A per-unit PRNG keeps this visual effect independent from gameplay and other client effects.
	dwStandingDeviationRandomState ^= dwStandingDeviationRandomState << 13;
	dwStandingDeviationRandomState ^= dwStandingDeviationRandomState >> 17;
	dwStandingDeviationRandomState ^= dwStandingDeviationRandomState << 5;
	return float( dwStandingDeviationRandomState & 0x00FFFFFFu ) / float( 0x01000000u );
}

CVec3 CMOUnitHelicopter::GetRandomStandingDeviationPoint( const float fRadius )
{
	for ( int i = 0; i < 16; ++i )
	{
		const CVec3 vPoint( NextStandingDeviationRandom() * 2.0f - 1.0f,
			NextStandingDeviationRandom() * 2.0f - 1.0f,
			NextStandingDeviationRandom() * 2.0f - 1.0f );
		if ( fabs2( vPoint ) <= 1.0f )
			return vPoint * fRadius;
	}

	return VNULL3;
}

void CMOUnitHelicopter::ChooseStandingDeviationTarget( const float fRadius )
{
	// Aim through a random inner point and continue to the sphere boundary. At the
	// boundary the next chord necessarily points back into the allowed volume.
	const CVec3 vInnerPoint( GetRandomStandingDeviationPoint( fRadius * 0.65f ) );
	CVec3 vDirection( vInnerPoint - vStandingDeviationOffset );
	if ( fabs2( vDirection ) < 0.000001f )
		vDirection = CVec3( 1.0f, 0.0f, 0.0f );
	Normalize( &vDirection );

	const float fDirectionDotOffset = vDirection.x * vStandingDeviationOffset.x
		+ vDirection.y * vStandingDeviationOffset.y + vDirection.z * vStandingDeviationOffset.z;
	const float fDiscriminant = (std::max)( 0.0f, fDirectionDotOffset * fDirectionDotOffset
		+ fRadius * fRadius - fabs2( vStandingDeviationOffset ) );
	const float fDistanceToBoundary = -fDirectionDotOffset + float( sqrt( fDiscriminant ) );
	vStandingDeviationTarget = vStandingDeviationOffset + vDirection * fDistanceToBoundary;
}

void CMOUnitHelicopter::AdjustVisualPlacement( SAINotifyPlacement *pPlacement )
{
	const NDb::SHelicopterStats *pHeliStats = GetHelicopterStats();
	const bool bShouldDeviate = pPlacement->bNewFormat && IsAlive() && pHeliStats
		&& pHeliStats->fStandingDeviationRadius > 0.0f && pHeliStats->fStandingDeviationSpeed > 0.0f
		&& !HasMoved() && pPlacement->fSpeed <= 0.001f;

	vStandingDeviationBasePlacement = pPlacement->bNewFormat
		? pPlacement->vPlacement : CVec3( pPlacement->center, pPlacement->z );

	if ( bShouldDeviate && !bStandingDeviationActive )
	{
		// Begin at the authoritative position so entering hover never causes a visual jump.
		vStandingDeviationOffset = VNULL3;
		ChooseStandingDeviationTarget( pHeliStats->fStandingDeviationRadius );
		timeStandingDeviationLastUpdate = GameTimer()->GetGameTime();
	}
	else if ( !bShouldDeviate )
	{
		vStandingDeviationOffset = VNULL3;
		vStandingDeviationTarget = VNULL3;
		timeStandingDeviationLastUpdate = GameTimer()->GetGameTime();
	}

	bStandingDeviationActive = bShouldDeviate;
	if ( bStandingDeviationActive )
	{
		pPlacement->vPlacement += vStandingDeviationOffset;
		pPlacement->center = CVec2( pPlacement->vPlacement.x, pPlacement->vPlacement.y );
		pPlacement->z = pPlacement->vPlacement.z;
	}
}

bool CMOUnitHelicopter::UpdateStandingDeviation( const NTimer::STime &time )
{
	const NDb::SHelicopterStats *pHeliStats = GetHelicopterStats();
	if ( !pHeliStats )
		return false;

	if ( !IsAlive() )
	{
		if ( bStandingDeviationActive )
		{
			// Do not leave a dead helicopter at its last client-only hover offset.
			CVec3 vPosition, vScale;
			CQuat qRotation;
			GetPlacement( &vPosition, &qRotation, &vScale );
			SetPlacement( vStandingDeviationBasePlacement, qRotation );
			Scene()->MoveObject( GetID(), vStandingDeviationBasePlacement, qRotation, vScale );
		}
		bStandingDeviationActive = false;
		vStandingDeviationOffset = VNULL3;
		vStandingDeviationTarget = VNULL3;
		return false;
	}

	if ( !bStandingDeviationActive )
	{
		timeStandingDeviationLastUpdate = time;
		return true;
	}

	const float fRadius = (std::max)( 0.0f, pHeliStats->fStandingDeviationRadius );
	const float fSpeed = (std::max)( 0.0f, pHeliStats->fStandingDeviationSpeed );
	const NTimer::STime timeDiff = (std::max)( NTimer::STime( 0 ), time - timeStandingDeviationLastUpdate );
	timeStandingDeviationLastUpdate = time;
	if ( fRadius <= 0.0f || fSpeed <= 0.0f || timeDiff <= 0 )
		return true;

	if ( fabs2( vStandingDeviationOffset ) > fRadius * fRadius )
	{
		Normalize( &vStandingDeviationOffset );
		vStandingDeviationOffset *= fRadius;
		ChooseStandingDeviationTarget( fRadius );
	}

	float fMovementLeft = fSpeed * float( timeDiff ) / 1000.0f;
	for ( int i = 0; i < 32 && fMovementLeft > 0.0f; ++i )
	{
		CVec3 vToTarget( vStandingDeviationTarget - vStandingDeviationOffset );
		const float fDistance = fabs( vToTarget );
		if ( fDistance <= 0.000001f )
		{
			ChooseStandingDeviationTarget( fRadius );
			continue;
		}

		if ( fMovementLeft < fDistance )
		{
			vStandingDeviationOffset += vToTarget * ( fMovementLeft / fDistance );
			fMovementLeft = 0.0f;
		}
		else
		{
			vStandingDeviationOffset = vStandingDeviationTarget;
			fMovementLeft -= fDistance;
			ChooseStandingDeviationTarget( fRadius );
		}
	}

	CVec3 vPosition, vScale;
	CQuat qRotation;
	GetPlacement( &vPosition, &qRotation, &vScale );
	vPosition = vStandingDeviationBasePlacement + vStandingDeviationOffset;
	SetPlacement( vPosition, qRotation );
	Scene()->MoveObject( GetID(), vPosition, qRotation, vScale );
	return true;
}
