#pragma once

#include "Aviation.h"

class CHelicopter : public CAviation
{
	OBJECT_NOCOPY_METHODS( CHelicopter );

	ZDATA_(CAviation)
	CVec3 vHeliNextPos;
	CVec3 vHeliNextSpeed;
	CVec3 vHeliNextNormal;
	CVec2 vMoveTarget;
	CVec3 vVisualAimPoint;
	bool bMoveTargetSet;
	bool bVisualAimPointSet;
	bool bAttackTilt;
	bool bDeadSpiralStarted;
	CVec3 vDeathStartPos;
	CVec3 vDeathVelocity;
	float fDeathGroundZ;
	float fDeathSpiralAngle;
	float fDeathSelfRotation;
	float fMovementVisualAngle;
	float fSideVisualAngle;
	float fVisualStartMovementAngle;
	float fVisualFinishMovementAngle;
	float fVisualStartSideAngle;
	float fVisualFinishSideAngle;
	float fAttackVisualAngle;
	float fVisualStartAttackAngle;
	float fVisualFinishAttackAngle;
	NTimer::STime timeDeathStarted;
	WORD wVisualStartDirection;
	WORD wVisualFinishDirection;
	WORD wDesiredDirection;
	bool bDesiredDirectionSet;
protected:
	virtual void UpdatePlacement( const CVec3 &vOldPosition, const WORD wOldDirection, const bool bNeedUpdate );
	bool AdvanceDesiredDirection( const NTimer::STime timeDiff );
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CAviation*)this); f.Add(2,&vHeliNextPos); f.Add(3,&vHeliNextSpeed); f.Add(4,&vHeliNextNormal); f.Add(5,&vMoveTarget); f.Add(6,&vVisualAimPoint); f.Add(7,&bMoveTargetSet); f.Add(8,&bVisualAimPointSet); f.Add(9,&bAttackTilt); f.Add(10,&bDeadSpiralStarted); f.Add(11,&vDeathStartPos); f.Add(12,&vDeathVelocity); f.Add(13,&fDeathGroundZ); f.Add(14,&fDeathSpiralAngle); f.Add(15,&fDeathSelfRotation); f.Add(16,&fMovementVisualAngle); f.Add(17,&fSideVisualAngle); f.Add(18,&timeDeathStarted); f.Add(19,&wVisualStartDirection); f.Add(20,&wVisualFinishDirection); f.Add(21,&fVisualStartMovementAngle); f.Add(22,&fVisualFinishMovementAngle); f.Add(23,&fVisualStartSideAngle); f.Add(24,&fVisualFinishSideAngle); f.Add(25,&wDesiredDirection); f.Add(26,&bDesiredDirectionSet); f.Add(27,&fAttackVisualAngle); f.Add(28,&fVisualStartAttackAngle); f.Add(29,&fVisualFinishAttackAngle); return 0; }

	CHelicopter();

	virtual void Init( const CVec2 &center, const int z, const SUnitBaseRPGStats *pStats, const float fHP, const WORD dir, const BYTE player, ICollisionsCollector *pCollisionsCollector );
	virtual IStatesFactory* GetStatesFactory() const;

	virtual void Segment();
	virtual void FirstSegment( const NTimer::STime timeDiff ) {}
	virtual void SecondSegment( const NTimer::STime timeDiff );
	virtual void Stop();
	virtual void StopTurning() { CBasePathUnit::StopTurning(); bDesiredDirectionSet = false; }
	virtual void SetDirection( const WORD wDirection, const bool bNeedUpdate = true );
	virtual const bool TurnToDirection( const WORD wDirection, const bool bCanBackward, const bool bCanForward );

	virtual bool IsHelicopter() const { return true; }
	// Unlike planes, a hovering helicopter can rotate its whole body to bring fixed guns onto a target.
	virtual bool IsLocked( const CBasicGun *pGun ) const { return false; }
	virtual const bool IsIdle() const { return !bMoveTargetSet && !bDeadSpiralStarted; }
	virtual const bool IsMoving() const { return bMoveTargetSet || fabs( vHeliNextSpeed ) > 0.001f; }
	// Fast aircraft can cross a target's visible area during the normal ground-unit scan delay.
	virtual const NTimer::STime GetBehUpdateDuration() const { return SConsts::AA_BEH_UPDATE_DURATION; }
	virtual const NTimer::STime GetNextSecondPathSegmTime() const { return 0; }

	virtual const WORD GetDir() const { return GetFrontDirection(); }
	virtual const CVec2 GetDirVector() const { return GetFrontDirectionVector(); }
	virtual const float GetSpeed() const { return fabs( CVec2( vHeliNextSpeed.x, vHeliNextSpeed.y ) ); }
	virtual void GetSpeed3( CVec3 *pSpeed ) const { *pSpeed = vHeliNextSpeed; }
	virtual CVec3 GetSpeedB2() const { return vHeliNextSpeed; }
	virtual CVec3 GetPosB2() const { return vHeliNextPos; }
	virtual CVec3 GetNormalB2() const { return vHeliNextNormal; }
	virtual CVec3 GetPosNext() const { return vHeliNextPos; }
	virtual CVec3 GetSpeedNext() const { return vHeliNextSpeed; }
	virtual CVec3 GetNormalNext() const { return vHeliNextNormal; }
	virtual const IManuver * GetManuver() const { return 0; }
	virtual void SetB2( const CVec3 &_vPos, const CVec3 &_vSpeed, const CVec3 &_vNormal );

	virtual void GetPlacement( struct SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff );
	virtual void DecFuel( const bool bEconomyMode );
	virtual void Die( const bool fromExplosion, const float fDamage );

	void BeginMoveTo( const CVec2 &vTarget );
	void BeginHover();
	bool IsNearTarget( const CVec2 &vTarget, const float fRadius ) const;
	void AimAtPoint( const CVec2 &vPoint );
	void AimAtUnit( CAIUnit *pTarget, class CBasicGun *pGun = 0 );
	void SetAttackTilt( const bool bTilt ) { bAttackTilt = bTilt; }

	void StartDeathSpiral();
	bool AdvanceDeathSpiral( const NTimer::STime timeDiff );
	bool IsDeathSpiralFinished() const { return bDeadSpiralStarted && GetCenter().z <= fDeathGroundZ + 0.1f; }
	CVec2 GetLeavePoint() const;

	const NDb::SHelicopterStats* GetHelicopterStats() const;
	float GetFlightZ( const CVec2 &vPoint ) const;

	virtual CArtillery* GetTowedArtillery() const { return 0; }
};
