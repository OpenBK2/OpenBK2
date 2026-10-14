#pragma once

#include "Guns.h"

#include <cstdint>

//*******************************************************************
//*								  Орудийные стволы																*
//*******************************************************************

// пушка на турельке
class CTurretGun : public CBasicGun
{
	OBJECT_BASIC_METHODS( CTurretGun );

	ZDATA_( CBasicGun )
	SAIAngle wBestWayDir;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	bool bTurnByBestWay = {};
	CPtr<CTurret> pTurret;
	bool bCircularAttack = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CBasicGun *)this );
		f.Add( 2, &wBestWayDir );
		f.Add( 3, &bTurnByBestWay );
		f.Add( 4, &pTurret );
		f.Add( 5, &bCircularAttack );
		return 0;
	}

	//
	bool TurnByVer( const CVec2 &vEnemyCenter, const float zDiff );
	bool TurnArtilleryToEnemy( const CVec2 &vEnemyCenter );
	bool TurnByBestWay( const uint16_t wDirToEnemy );

	// эта функция считает угол, под которым нужно повернуть турель, чтобы обстреливать
	// заданную точку. с учетом вертикальных ограничений.
	uint16_t CalcVerticalAngle( const class CVec3 &pt ) const;

protected:
	bool TurnGunToEnemy( const CVec2 &vEnemyCenter, const float zDiff ) override;
	// можно ли прямо сейчас стрельнуть по point ( не вращая ни turret ни base ), погрешность - угол addAngle
	bool
	IsGoodAngle( const CVec2 &point, const uint16_t addAngle, const float z, const uint8_t cDeltaAngle ) const override;
	void Rest() override;
	bool AnalyzeTurning() override;

public:
	CTurretGun()
		: bCircularAttack( false )
	{
	}

	CTurretGun( class CAIUnit *pOwner,
	            const uint8_t nShellType,
	            SCommonGunInfo *pCommonGunInfo,
	            const IGunsFactory::EGunTypes eType,
	            const int nTurret );

	bool IsOnTurret() const override
	{
		return true;
	}

	class CTurret *GetTurret() const override
	{
		return pTurret;
	}

	void TraceAim( class CAIUnit *pUnit ) override;
	void StopTracing() override;

	void StopFire() override;

	// можно ли дострельнуть по высоте
	bool CanShootByHeight( class CAIUnit *pTarget ) const override;

	// куда в данный момент смотрит gun
	const uint16_t GetGlobalDir() const override;
	void TurnToRelativeDir( const uint16_t wAngle ) override;

	const float GetRotateSpeed() const override;

	uint16_t GetHorTurnConstraint() const override;
	uint16_t GetVerTurnConstraint() const override;

	void SetCircularAttack( const bool bCanAttack ) override;

	void StartPointBurst( const CVec3 &target, bool bReAim ) override;
	void StartPointBurst( const CVec2 &target, bool bReAim, bool bStartParallelGuns = true ) override;
	void StartEnemyBurst( class CAIUnit *pEnemy, bool bReAim ) override;

	const NTimer::STime GetTimeToShootToPoint( const CVec3 &vPoint ) const override;
	const NTimer::STime GetTimeToShoot( const CVec3 &vPoint ) const override;
};

// пушка на базовой платформе
class CBaseGun : public CBasicGun
{
	OBJECT_BASIC_METHODS( CBaseGun );
	ZDATA_( CBasicGun )
public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CBasicGun *)this );
		return 0;
	}

protected:
	bool TurnGunToEnemy( const CVec2 &vEnemyCenter, const float zDiff ) override;
	// можно ли прямо сейчас стрельнуть по point ( не вращая ни turret ни base ), погрешность - угол addAngle
	bool
	IsGoodAngle( const CVec2 &point, const uint16_t addAngle, const float z, const uint8_t cDeltaAngle ) const override;

	void Rest() override {}

	bool AnalyzeTurning() override;

public:
	CBaseGun() {}

	CBaseGun( class CAIUnit *pOwner,
	          const uint8_t nShellType,
	          SCommonGunInfo *pCommonGunInfo,
	          const IGunsFactory::EGunTypes eType )
		: CBasicGun( pOwner, nShellType, pCommonGunInfo, eType )
	{
	}

	bool IsOnTurret() const override
	{
		return false;
	}

	class CTurret *GetTurret() const override
	{
		return 0;
	}

	void TraceAim( class CAIUnit *pUnit ) override {}

	void StopTracing() override {}

	void StopFire() override;

	// куда в данный момент смотрит gun
	const uint16_t GetGlobalDir() const override;

	void TurnToRelativeDir( const uint16_t wAngle ) override {}

	const float GetRotateSpeed() const override;

	uint16_t GetHorTurnConstraint() const override
	{
		return 32768;
	}

	uint16_t GetVerTurnConstraint() const override
	{
		return 32768;
	}

	void SetCircularAttack( const bool bCanAttack ) override {}
};

class CUnitsGunsFactory : public IGunsFactory
{
	class CAIUnit *pUnit = {};
	const int nCommonGun = {};
	int nTurret = {};

public:
	CUnitsGunsFactory( class CAIUnit *_pUnit, const int _nCommonGun, const int _nTurret )
		: pUnit( _pUnit )
		, nCommonGun( _nCommonGun )
		, nTurret( _nTurret )
	{
	}

	int GetNCommonGun() const override
	{
		return nCommonGun;
	}

	CBasicGun *CreateGun( const EGunTypes eType, const int nShell, SCommonGunInfo *pCommonGunInfo ) const override
	{
		CBasicGun *pGun = 0;
		if ( nTurret != -1 )
		{
			pGun = new CTurretGun( pUnit, nShell, pCommonGunInfo, eType, nTurret );
		}
		else
		{
			pGun = new CBaseGun( pUnit, nShell, pCommonGunInfo, eType );
		}

		return pGun;
	}
};
