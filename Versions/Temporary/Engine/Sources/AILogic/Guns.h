#pragma once

#include "LinkObject.h"
#include "Stats_B2_M1/RPGStats.h"

#include <cstdint>

namespace NDb
{
	struct SWeaponRPGStats;
	struct SBaseGunRPGStats;
}
class CAIUnit;

//*******************************************************************
//*								  Оружие юнита																		*
//*******************************************************************

struct SCommonGunInfo : public CAIObjectBase
{
	OBJECT_BASIC_METHODS( SCommonGunInfo );
public:
	ZDATA
	bool bFiring;
	int nAmmo;
	NTimer::STime lastShoot;
	int nGun;
	int nPlatform;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&bFiring); f.Add(3,&nAmmo); f.Add(4,&lastShoot); f.Add(5,&nGun); f.Add(6,&nPlatform); return 0; }
public:

	SCommonGunInfo() { }
	SCommonGunInfo( bool _bFiring, const int _nAmmo, const int _nPlatform, const int _nGun ) : bFiring( _bFiring ), nAmmo( _nAmmo ), nPlatform( _nPlatform ), lastShoot( 0 ), nGun( _nGun ) { }
};

struct IGunsFactory
{
	enum EGunTypes { MOMENT_CML_GUN, MOMENT_BURST_GUN, VIS_CML_BALLIST_GUN, VIS_BURST_BALLIST_GUN, PLANE_GUN, MORALE_GUN, TORPEDO_GUN, ROCKET_GUN, FLAME_GUN };

	virtual class CBasicGun* CreateGun( const EGunTypes eType, const int nShell, SCommonGunInfo *pCommonGunInfo ) const = 0;
	virtual int GetNCommonGun() const = 0;
};

class CBasicGunCRAPSaver : public CLinkObject
{
	ZDATA_(CLinkObject)
public: 
protected:
	CDBPtr<SWeaponRPGStats> pWeapon;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CLinkObject*)this); f.Add(2,&pWeapon); return 0; }
};

class CBasicGun : public CBasicGunCRAPSaver
{

	enum EShootState { EST_TURNING, EST_AIMING, WAIT_FOR_ACTION_POINT, EST_SHOOTING, EST_REST };
protected:
	// parallelGuns below is protected, so the type that names it has to be too:
	// CTurretGun::TraceAim iterates it.
	typedef std::list< CPtr<CBasicGun> > CParallelGuns;
private:

	ZDATA_(CBasicGunCRAPSaver)
protected:
	ZSKIP//	CDBPtr<SWeaponRPGStats> pWeapon;
private:
	EShootState shootState;

	bool bWaitForReload; //specific for artillery

	// можно ли производить выстрел
	bool bCanShoot;
	// сколько ещё осталось в очереди
	int nShotsLast;


	IGunsFactory::EGunTypes eType;
	EUnitAckType eRejectReason;

	CVec3 vLastShotPoint;

	float fRandom4Aim, fRandom4Relax;
	//
protected:	
	uint8_t nShellType;
	CPtr<CAIUnit> pOwner;
	int nOwnerParty;
	CPtr<SCommonGunInfo> pCommonGunInfo;

	// юнит, по которому стреляем ( в случае стрельбы по юниту )
	CPtr<CAIUnit> pEnemy;
	// Aviation target kept for predicted AA fire, which shoots at a point instead of pEnemy.
	CPtr<CAIUnit> pAntiAviationTarget;
	// куда стрелять
	CVec2 target;
	// время начала прицеливания или начала отдыха, в зависимости от состояния
	NTimer::STime lastCheck;
	CVec2 lastEnemyPos;
	bool bAngleLocked;

	// нужно ли прицеливаться
	bool bAim;
	bool bGrenade;
	// высота точки, в которую направлена стрельба
	float z;

	CParallelGuns parallelGuns;
	bool bParallelGun;
	NTimer::STime lastCheckTurnTime;
	bool bIgnoreObstacles;
private:
	CPtr<CAIUnit> pCanShootCachedEnemy;
	bool bCanShootToUnitWOMove;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CBasicGunCRAPSaver*)this); f.Add(3,&shootState); f.Add(4,&bWaitForReload); f.Add(5,&bCanShoot); f.Add(6,&nShotsLast); f.Add(7,&eType); f.Add(8,&eRejectReason); f.Add(9,&vLastShotPoint); f.Add(10,&fRandom4Aim); f.Add(11,&fRandom4Relax); f.Add(12,&nShellType); f.Add(13,&pOwner); f.Add(14,&nOwnerParty); f.Add(15,&pCommonGunInfo); f.Add(16,&pEnemy); f.Add(17,&target); f.Add(18,&lastCheck); f.Add(19,&lastEnemyPos); f.Add(20,&bAngleLocked); f.Add(21,&bAim); f.Add(22,&bGrenade); f.Add(23,&z); f.Add(24,&parallelGuns); f.Add(25,&bParallelGun); f.Add(26,&lastCheckTurnTime); f.Add(27,&bIgnoreObstacles); f.Add(28,&pCanShootCachedEnemy); f.Add(29,&bCanShootToUnitWOMove); f.Add(30,&pAntiAviationTarget); return 0; }

private:

	void Aiming();
	void WaitForActionPoint();
	void Shooting();
	const CVec2 GetShootingPoint() const;
	uint16_t GetVisAngleOfAim() const;

	// для ускорения стрельбы в хороших случаях
	void OnWaitForActionPointState();
	void OnTurningState();
	void OnAimState();

	bool CanShotBecauseOfObstacles( const CVec2 &point, const float fZ );
	void ToRestStateWOParallel();
protected:


	const NTimer::STime GetActionPoint() const;
	//
	virtual bool TurnGunToEnemy( const CVec2 &vEnemyCenter, const float zDiff ) = 0;
	// можно ли прямо сейчас стрельнуть по point ( не вращая ни turret ни base ), погрешность - угол addAngle, cDeltaAngle - учитывать ли deltaAngle
	virtual bool IsGoodAngle( const CVec2 &point, const uint16_t addAngle, const float z, const uint8_t cDeltaAngle ) const = 0;
	virtual void ToRestState();
	virtual void Rest() = 0;
	virtual bool AnalyzeTurning() = 0;
	// можно ли стрельнуть по цели не вращая ни turret ни base и не двигаясь
	// cDeltaAngle - учитывать ли deltaAngle
	bool CanShootWOGunTurn( const uint8_t cDeltaAngle, const float fZ );
	// можно ли стрелять по точке, если pUnit находится внутри статич. объекта
	bool AnalyzeLimitedAngle( class CCommonUnit *pUnit, const CVec2 &point ) const;
	void Turning();
	bool CanShootToTargetWOMove();

	void InitRandoms();
	const NDb::SUnitStatsModifier* GetAntiAviationModifier() const;
public:
	CBasicGun() : pOwner( 0 ), pAntiAviationTarget( 0 ), bParallelGun( false ), vLastShotPoint( VNULL3	), lastCheckTurnTime( 0 ), bCanShootToUnitWOMove( false ) { }
	CBasicGun( class CAIUnit *pOwner, const uint8_t nShellType, SCommonGunInfo *pCommonGunInfo, const IGunsFactory::EGunTypes eType );

	virtual int GetShellType() const { return nShellType; } 

	virtual const float GetAimTime( bool bRandomize = true ) const;
	virtual const float GetRelaxTime( bool bRandomize = true ) const;
	virtual class CAIUnit* GetOwner() const { return pOwner; }	
	virtual void SetOwner( CAIUnit *pOwner );
	bool IsGrenade() const { return bGrenade; }

	virtual void GetMechShotInfo( SAINotifyMechShot *pMechShotInfo, const NTimer::STime &time ) const;
	virtual void GetInfantryShotInfo( SAINotifyInfantryShot *pInfantryShotInfo, const NTimer::STime &time ) const;

	virtual bool InFireRange( class CAIUnit *pTarget ) const;
	virtual bool InFireRange( const CVec3 &vPoint ) const;
	virtual float GetFireRange( float z ) const;
	// возвращает fRandgeMax from rpgstats с учётом всех модификаторов - коэффициентов
	virtual float GetFireRangeMax() const;
	virtual bool InGoToSideRange( const class CAIUnit *pTarget ) const;
	virtual bool TooCloseToFire( const class CAIUnit *pTarget ) const;
	virtual bool TooCloseToFire( const CVec3 &vPoint ) const;

	virtual void StartPointBurst( const CVec3 &target, bool bReAim );
	virtual void StartPointBurst( const CVec2 &target, bool bReAim, bool bStartParallelGuns = true );
	virtual void StartEnemyBurst( class CAIUnit *pEnemy, bool bReAim );
	void Segment();
	void SetAntiAviationTarget( class CAIUnit *pTarget );

	bool IsWaitForReload() const { return bWaitForReload; }
	virtual void ClearWaitForReload() { bWaitForReload = false; }

	// в данный момент в состоянии наводки и стрельбы
	virtual bool IsFiring() const ;
	bool IsBursting() const { return shootState == WAIT_FOR_ACTION_POINT || shootState == EST_SHOOTING; }

	virtual const SBaseGunRPGStats& GetGun() const;
	virtual const SWeaponRPGStats* GetWeapon() const;
	virtual const SWeaponRPGStats::SShell& GetShell() const;

	virtual bool IsRelaxing() const;
	// можно ли стрельнуть по pEnemy, не вращая ни base ни turret, cDeltaAngle - учитывать ли deltaAngle
	virtual bool CanShootWOGunTurn( class CAIUnit *pEnemy, const uint8_t cDeltaAngle );
	virtual const NTimer::STime GetRestTimeOfRelax() const;

	// стрельба, когда двигаться запрещено
	virtual bool CanShootToUnitWOMove( class CAIUnit *pEnemy );
	virtual bool CanShootToObjectWOMove( class CStaticObject *pObj );
	virtual bool CanShootToPointWOMove( const CVec2 &point, const float fZ, const uint16_t wHorAddAngle = 0, const uint16_t wVertAddAngle = 0, CAIUnit *pEnemy = 0 );

	// можно ли дострельнуть по высоте	
	virtual bool CanShootByHeight( class CAIUnit *pTarget ) const;	
	virtual bool CanShootByHeight( const float fZ ) const;

	// можно ли стрельнуть в объект по прямому приказу
	virtual bool CanShootToUnit( class CAIUnit *pEnemy );
	virtual bool CanShootToObject( class CStaticObject *pObj );
	virtual bool CanShootToPoint( const CVec2 &point, const float fZ, const uint16_t wHorAddAngle = 0, const uint16_t wVertAddAngle = 0 );

	// можно пристрелить, не поворачивая base ( turret вращать можно )
	virtual bool IsInShootCone( const CVec2 &point, const uint16_t wAddAngle = 0 ) const;

	virtual const float GetDispersion() const;
	virtual const float GetDispRatio( uint8_t nShellType, const float fDist ) const;
	virtual const int GetFireRate() const;
	void LockInCurAngle() { bAngleLocked = true; }
	void UnlockCurAngle() { bAngleLocked = false; }

	// для самолётов
	virtual void StartPlaneBurst( class CAIUnit *pEnemy, bool bReAim );

	// можно пробить броню с учётом стороны, которой повёрнут pTarget
	virtual bool CanBreakArmor( class CAIUnit *pTarget ) const;
	// можно пробить броню с какой-нибудь стороны
	virtual bool CanBreach( const class CCommonUnit *pTarget ) const;
	// можно пробить броню со стороны nSide
	virtual bool CanBreach( const class CCommonUnit *pTarget, const int nSide ) const;
	virtual bool CanBreach( const SHPObjectRPGStats *pStats, const int nSide ) const;

	// будет делать все действия, нужные для стрельбы по цели (повороты, прицеливание), но не будет стрелять
	void DontShoot() { bCanShoot = false; }
	// отменяет DontShoot()
	void CanShoot() { bCanShoot = true; }
	bool IsShootAllowed(){ return bCanShoot; }

	// стреляет ли общий gun ( с учётом патронов - т.е. учитываются все guns, находящиеся с ним в одном стволе )
	bool IsCommonGunFiring() const { return pCommonGunInfo->bFiring; }
	// равен ли pGun ( с учётом патронов )
	virtual bool IsCommonEqual( const CBasicGun *pGun ) const;

	// "номер ствола" ( gun-ы, отличающиеся только патронами, но находящиеся в одном стволе )
	int GetCommonGunNumber() const { return pCommonGunInfo->nGun; }
	int GetPlatform() const { return pCommonGunInfo->nPlatform; }

	int GetNAmmo() const { return pCommonGunInfo->nAmmo; }
	void SetNAmmo( int nAmmo ) { pCommonGunInfo->nAmmo = nAmmo; }

	virtual struct IBallisticTraj* CreateTraj( const CVec3 &vTarget ) const;
	virtual void Fire( const CVec2 &target, const float z, const bool bShowBombEffect );
	virtual uint16_t GetTrajectoryZAngle( const CVec3 &vToAim ) const;

	// сказать, почему отказался стрелять
	virtual const EUnitAckType& GetRejectReason() const { return eRejectReason; }
	virtual void SetRejectReason( const EUnitAckType &eReason );

	virtual const bool IsVisible( const uint8_t party ) const { return true; }
	virtual void GetTilesForVisibility( CTilesSet *pTiles ) const { pTiles->clear(); }
	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const { return false; }

	void AddParallelGun( CBasicGun *pGun ) { parallelGuns.push_back( pGun ); }
	void SetToParallelGun() { bParallelGun = true; }

	// среднее значение
	virtual const int GetPiercing() const;
	// разброс
	virtual const int GetPiercingRandom() const;
	// рандомное значение piercing
	virtual const int GetRandomPiercing() const;
	virtual const int GetMaxPossiblePiercing() const;
	virtual const int GetMinPossiblePiercing() const;

	// среднее значение damage
	virtual const float GetDamage() const;
	// разброс
	virtual const float GetDamageRandom() const;
	// рандомное значение damage
	virtual const float GetRandomDamage() const;

	virtual bool IsBallisticTrajectory() const;
	bool IsIgnoreObstacles() const { return bIgnoreObstacles; }

	virtual bool IsOnTurret() const = 0;

	virtual void StopFire() = 0;

	virtual uint16_t GetHorTurnConstraint() const = 0;
	virtual uint16_t GetVerTurnConstraint() const = 0;
	virtual class CTurret* GetTurret() const = 0;

	// направление, по которому в данный момент смотрит орудие
	virtual const uint16_t GetGlobalDir() const = 0;
	// for AA guns.
	virtual const NTimer::STime GetTimeToShootToPoint( const CVec3 &vPoint ) const { return 0; }
	virtual const NTimer::STime GetTimeToShoot( const CVec3 &vPoint ) const { return 0; }
	virtual void TraceAim( class CAIUnit *pUnit ) = 0;
	// если на турели, то повернуть в относительный угол wAngle
	virtual void TurnToRelativeDir( const uint16_t wAngle ) = 0;
	// разрешить/запретить атаковать без учёта ограничения на поворот орудия по горизонтали
	virtual void SetCircularAttack( const bool bCanAttack ) = 0;
	virtual class CBuilding *GetMountBuilding() const { return 0; }
	virtual void StopTracing() = 0;
	virtual const float GetRotateSpeed() const = 0;
};

float GetDispByRadius( const class CBasicGun *pGun, const CVec2 &attackerPos, const CVec2 &explCoord );
float GetDispByRadius( const class CBasicGun *pGun, const float fDist );
float GetDispByRadius( const float fDispRadius, const float fRangeMax, const float fDist );

const float GetFireRangeMax( const SWeaponRPGStats *pStats, CAIUnit *pOwner );

