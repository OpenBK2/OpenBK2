#ifndef __GUNS_INTERNAL_H__
#define __GUNS_INTERNAL_H__

#pragma ONCE

#include "Guns.h"


//*******************************************************************
//*								  Орудийные стволы																*
//*******************************************************************

// пушка на турельке
class CTurretGun : public CBasicGun
{
	OBJECT_BASIC_METHODS( CTurretGun );
	
	ZDATA_(CBasicGun)
	SAIAngle wBestWayDir;
	bool bTurnByBestWay;
	CPtr<CTurret> pTurret;
	bool bCircularAttack;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CBasicGun*)this); f.Add(2,&wBestWayDir); f.Add(3,&bTurnByBestWay); f.Add(4,&pTurret); f.Add(5,&bCircularAttack); return 0; }

	//
	bool TurnByVer( const CVec2 &vEnemyCenter, const float zDiff );
	bool TurnArtilleryToEnemy( const CVec2 &vEnemyCenter );
	bool TurnByBestWay( const WORD wDirToEnemy );
	
	// эта функция считает угол, под которым нужно повернуть турель, чтобы обстреливать
	// заданную точку. с учетом вертикальных ограничений.
	WORD CalcVerticalAngle( const class CVec3 &pt ) const;
protected:
	virtual bool TurnGunToEnemy( const CVec2 &vEnemyCenter, const float zDiff );
	// можно ли прямо сейчас стрельнуть по point ( не вращая ни turret ни base ), погрешность - угол addAngle
	virtual bool IsGoodAngle( const CVec2 &point, const WORD addAngle, const float z, const BYTE cDeltaAngle ) const;
	virtual void Rest();
	virtual bool AnalyzeTurning();
public:
	CTurretGun() : bCircularAttack( false ) { }
	CTurretGun( class CAIUnit *pOwner, const BYTE nShellType, SCommonGunInfo *pCommonGunInfo, const IGunsFactory::EGunTypes eType, const int nTurret );

	virtual bool IsOnTurret() const { return true; }
	virtual class CTurret* GetTurret() const { return pTurret; }
	virtual void TraceAim( class CAIUnit *pUnit );
	virtual void StopTracing();

	virtual void StopFire();

	// можно ли дострельнуть по высоте
	virtual bool CanShootByHeight( class CAIUnit *pTarget ) const;

	// куда в данный момент смотрит gun
	virtual const WORD GetGlobalDir() const;
	virtual void TurnToRelativeDir( const WORD wAngle );

	virtual const float GetRotateSpeed() const;

	virtual WORD GetHorTurnConstraint() const;
	virtual WORD GetVerTurnConstraint() const;
	
	void SetCircularAttack( const bool bCanAttack );

	virtual void StartPointBurst( const CVec3 &target, bool bReAim );
	virtual void StartPointBurst( const CVec2 &target, bool bReAim );
	virtual void StartEnemyBurst( class CAIUnit *pEnemy, bool bReAim );
	
	virtual const NTimer::STime GetTimeToShootToPoint( const CVec3 &vPoint ) const;
	virtual const NTimer::STime GetTimeToShoot( const CVec3 &vPoint ) const;
};

// пушка на базовой платформе
class CBaseGun : public CBasicGun
{
	OBJECT_BASIC_METHODS( CBaseGun );
	ZDATA_(CBasicGun)
public:
		ZEND int operator&( IBinSaver &f ) { f.Add(1,(CBasicGun*)this); return 0; }

protected:
	virtual bool TurnGunToEnemy( const CVec2 &vEnemyCenter, const float zDiff );
	// можно ли прямо сейчас стрельнуть по point ( не вращая ни turret ни base ), погрешность - угол addAngle
	virtual bool IsGoodAngle( const CVec2 &point, const WORD addAngle, const float z, const BYTE cDeltaAngle ) const;
	virtual void Rest() { }
	virtual bool AnalyzeTurning();
public:
	CBaseGun() { }
	CBaseGun( class CAIUnit *pOwner, const BYTE nShellType, SCommonGunInfo *pCommonGunInfo, const IGunsFactory::EGunTypes eType )
	: CBasicGun( pOwner, nShellType, pCommonGunInfo, eType ) { }

	virtual bool IsOnTurret() const { return false; }
	virtual class CTurret* GetTurret() const { return 0; }
	virtual void TraceAim( class CAIUnit *pUnit ) { }
	virtual void StopTracing() { }

	virtual void StopFire();

	// куда в данный момент смотрит gun
	virtual const WORD GetGlobalDir() const;
	virtual void TurnToRelativeDir( const WORD wAngle ) { }

	virtual const float GetRotateSpeed() const;

	virtual WORD GetHorTurnConstraint() const { return 32768; }
	virtual WORD GetVerTurnConstraint() const { return 32768; }

	void SetCircularAttack( const bool bCanAttack ) { }
};

class CUnitsGunsFactory : public IGunsFactory
{
	class CAIUnit *pUnit;
	const int nCommonGun;
	int nTurret;
public:
	CUnitsGunsFactory( class CAIUnit *_pUnit, const int _nCommonGun, const int _nTurret )
		: pUnit( _pUnit ), nCommonGun( _nCommonGun ), nTurret( _nTurret ) { }

	virtual int GetNCommonGun() const { return nCommonGun; }
	virtual CBasicGun* CreateGun( const EGunTypes eType, const int nShell, SCommonGunInfo *pCommonGunInfo ) const 
	{ 
		CBasicGun *pGun = 0;
		if ( nTurret != -1 )
			pGun = new CTurretGun( pUnit, nShell, pCommonGunInfo, eType, nTurret );
		else
			pGun = new CBaseGun( pUnit, nShell, pCommonGunInfo, eType );

		return pGun;
	}
};

#endif // __GUNS_INTERNAL_H__

