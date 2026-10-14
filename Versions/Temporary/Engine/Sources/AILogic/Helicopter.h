#pragma once

#include "Aviation.h"

class CHelicopter : public CAviation
{
	OBJECT_NOCOPY_METHODS( CHelicopter );

	ZDATA_( CAviation )
	CVec3 vHeliNextPos;
	CVec3 vHeliNextSpeed;
	CVec3 vHeliNextNormal;
	CVec2 vMoveTarget;
	CVec3 vVisualAimPoint;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	bool bMoveTargetSet = {};
	bool bVisualAimPointSet = {};
	bool bAttackTilt = {};
	bool bDeadSpiralStarted = {};
	CVec3 vDeathStartPos;
	CVec3 vDeathVelocity;
	float fDeathGroundZ = {};
	float fDeathSpiralRadius = {};
	float fDeathSpiralSteps = {};
	float fDeathSpiralDownAcceleration = {};
	float fDeathSpiralAngle = {};
	float fDeathSelfRotation = {};
	float fMovementVisualAngle = {};
	float fSideVisualAngle = {};
	float fVisualStartMovementAngle = {};
	float fVisualFinishMovementAngle = {};
	float fVisualStartSideAngle = {};
	float fVisualFinishSideAngle = {};
	float fAttackVisualAngle = {};
	float fVisualStartAttackAngle = {};
	float fVisualFinishAttackAngle = {};
	NTimer::STime timeDeathStarted = {};
	uint16_t wVisualStartDirection = {};
	uint16_t wVisualFinishDirection = {};
	uint16_t wDesiredDirection = {};
	bool bDesiredDirectionSet = {};

protected:
	void UpdatePlacement( const CVec3 &vOldPosition, const uint16_t wOldDirection, const bool bNeedUpdate ) override;
	bool AdvanceDesiredDirection( const NTimer::STime timeDiff );

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CAviation *)this );
		f.Add( 2, &vHeliNextPos );
		f.Add( 3, &vHeliNextSpeed );
		f.Add( 4, &vHeliNextNormal );
		f.Add( 5, &vMoveTarget );
		f.Add( 6, &vVisualAimPoint );
		f.Add( 7, &bMoveTargetSet );
		f.Add( 8, &bVisualAimPointSet );
		f.Add( 9, &bAttackTilt );
		f.Add( 10, &bDeadSpiralStarted );
		f.Add( 11, &vDeathStartPos );
		f.Add( 12, &vDeathVelocity );
		f.Add( 13, &fDeathGroundZ );
		f.Add( 14, &fDeathSpiralAngle );
		f.Add( 15, &fDeathSelfRotation );
		f.Add( 16, &fMovementVisualAngle );
		f.Add( 17, &fSideVisualAngle );
		f.Add( 18, &timeDeathStarted );
		f.Add( 19, &wVisualStartDirection );
		f.Add( 20, &wVisualFinishDirection );
		f.Add( 21, &fVisualStartMovementAngle );
		f.Add( 22, &fVisualFinishMovementAngle );
		f.Add( 23, &fVisualStartSideAngle );
		f.Add( 24, &fVisualFinishSideAngle );
		f.Add( 25, &wDesiredDirection );
		f.Add( 26, &bDesiredDirectionSet );
		f.Add( 27, &fAttackVisualAngle );
		f.Add( 28, &fVisualStartAttackAngle );
		f.Add( 29, &fVisualFinishAttackAngle );
		f.Add( 30, &fDeathSpiralRadius );
		f.Add( 31, &fDeathSpiralSteps );
		f.Add( 32, &fDeathSpiralDownAcceleration );
		return 0;
	}

	CHelicopter();

	void Init( const CVec2 &center,
	           const int z,
	           const SUnitBaseRPGStats *pStats,
	           const float fHP,
	           const uint16_t dir,
	           const uint8_t player,
	           ICollisionsCollector *pCollisionsCollector ) override;
	IStatesFactory *GetStatesFactory() const override;

	void Segment() override;

	void FirstSegment( const NTimer::STime timeDiff ) override {}

	void SecondSegment( const NTimer::STime timeDiff ) override;
	void Stop() override;

	void StopTurning() override
	{
		CBasePathUnit::StopTurning();
		bDesiredDirectionSet = false;
	}

	void SetDirection( const uint16_t wDirection, const bool bNeedUpdate = true ) override;
	const bool TurnToDirection( const uint16_t wDirection, const bool bCanBackward, const bool bCanForward ) override;

	bool IsHelicopter() const override
	{
		return true;
	}

	// Unlike planes, a hovering helicopter can rotate its whole body to bring fixed guns onto a target.
	bool IsLocked( const CBasicGun *pGun ) const override
	{
		return false;
	}

	const bool IsIdle() const override
	{
		return !bMoveTargetSet && !bDeadSpiralStarted;
	}

	const bool IsMoving() const override
	{
		return bMoveTargetSet || fabs( vHeliNextSpeed ) > 0.001f;
	}

	// Fast aircraft can cross a target's visible area during the normal ground-unit scan delay.
	const NTimer::STime GetBehUpdateDuration() const override
	{
		return SConsts::AA_BEH_UPDATE_DURATION;
	}

	const NTimer::STime GetNextSecondPathSegmTime() const override
	{
		return 0;
	}

	const uint16_t GetDir() const override
	{
		return GetFrontDirection();
	}

	const CVec2 GetDirVector() const override
	{
		return GetFrontDirectionVector();
	}

	const float GetSpeed() const override
	{
		return fabs( CVec2( vHeliNextSpeed.x, vHeliNextSpeed.y ) );
	}

	void GetSpeed3( CVec3 *pSpeed ) const override
	{
		*pSpeed = vHeliNextSpeed;
	}

	CVec3 GetSpeedB2() const override
	{
		return vHeliNextSpeed;
	}

	CVec3 GetPosB2() const override
	{
		return vHeliNextPos;
	}

	CVec3 GetNormalB2() const override
	{
		return vHeliNextNormal;
	}

	CVec3 GetPosNext() const override
	{
		return vHeliNextPos;
	}

	CVec3 GetSpeedNext() const override
	{
		return vHeliNextSpeed;
	}

	CVec3 GetNormalNext() const override
	{
		return vHeliNextNormal;
	}

	const IManuver *GetManuver() const override
	{
		return 0;
	}

	void SetB2( const CVec3 &_vPos, const CVec3 &_vSpeed, const CVec3 &_vNormal ) override;

	void GetPlacement( struct SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff ) override;
	void DecFuel( const bool bEconomyMode ) override;
	void Die( const bool fromExplosion, const float fDamage ) override;

	void BeginMoveTo( const CVec2 &vTarget );
	void BeginHover();
	bool IsNearTarget( const CVec2 &vTarget, const float fRadius ) const;
	void AimAtPoint( const CVec2 &vPoint );
	void AimAtUnit( CAIUnit *pTarget, class CBasicGun *pGun = 0 );

	void SetAttackTilt( const bool bTilt )
	{
		bAttackTilt = bTilt;
	}

	void StartDeathSpiral();
	bool AdvanceDeathSpiral( const NTimer::STime timeDiff );

	bool IsDeathSpiralFinished() const
	{
		return bDeadSpiralStarted && GetCenter().z <= fDeathGroundZ + 0.1f;
	}

	CVec2 GetLeavePoint() const;

	const NDb::SHelicopterStats *GetHelicopterStats() const;
	float GetFlightZ( const CVec2 &vPoint ) const;

	CArtillery *GetTowedArtillery() const override
	{
		return 0;
	}
};
