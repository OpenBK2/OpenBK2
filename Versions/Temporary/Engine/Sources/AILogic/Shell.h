#pragma once

#include "LinkObject.h"
#include "Stats_B2_M1/Actions.h"
#include "Stats_B2_M1/RPGStats.h"

#include <queue>

class CAIUnit;

//*******************************************************************
//*														Hits																	*
//*******************************************************************

class CHitInfo : public CLinkObject
{
	OBJECT_BASIC_METHODS( CHitInfo );
public:
	ZDATA_(CLinkObject)
		
	CDBPtr<SWeaponRPGStats> pWeapon;
	WORD wShell;
	SAIAngle wDir;

	CPtr<CObjectBase> pVictim;  // для попадания по юниту
	CVec3 explCoord;					// для попадания по земле

	SAINotifyHitInfo::EHitType eHitType;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CLinkObject*)this); f.Add(2,&pWeapon); f.Add(3,&wShell); f.Add(4,&wDir); f.Add(5,&pVictim); f.Add(6,&explCoord); f.Add(7,&eHitType); return 0; }
	
	CHitInfo() { }
	CHitInfo( const SWeaponRPGStats *_pWeapon, const WORD _wShell, const WORD &_wDir, CObjectBase *_pVictim, const SAINotifyHitInfo::EHitType _eHitType, const CVec3 &_explCoord )
		: pWeapon( _pWeapon ), wShell( _wShell ), wDir( _wDir ), pVictim( _pVictim ), eHitType( _eHitType ), explCoord( _explCoord ) { SetUniqueIdForObjects(); }

	CHitInfo( const SWeaponRPGStats *_pWeapon, const WORD _wShell, const WORD &_wDir, const CVec3 &_explCoord, const SAINotifyHitInfo::EHitType _eHitType )
		: pWeapon( _pWeapon ), wShell( _wShell ), wDir( _wDir ), pVictim( 0 ), explCoord( _explCoord ), eHitType( _eHitType ) { SetUniqueIdForObjects(); }

	CHitInfo( const class CExplosion *pExpl, CObjectBase *pVictim, const enum SAINotifyHitInfo::EHitType &eHitType, const CVec3 &explCoord );

	virtual void GetHitInfo( struct SAINotifyHitInfo *pHitInfo ) const;
	
	virtual const bool IsVisible( const BYTE party ) const { return true; }
	virtual void GetTilesForVisibility( CTilesSet *pTiles ) const { pTiles->clear(); }
	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const { return false; }
};

//*******************************************************************
//*												Траектории																*
//*******************************************************************

struct IBallisticTraj : public CAIObjectBase
{
public:
	virtual const NTimer::STime& GetExplTime() const = 0;
	virtual const NTimer::STime& GetStartTime() const = 0;

	virtual const CVec3& GetStartPoint() const = 0;
	virtual const WORD GetStart2DDir() const = 0;

	virtual const CVec3 GetCoordinates() const = 0;
	virtual const CVec3 GetVelocity() const { return VNULL3; }

	// Dynamic trajectories advance explicitly; analytic trajectories need no work here.
	virtual void Segment() { }
	virtual bool IsFinished( const NTimer::STime &time ) const { return GetExplTime() <= time; }

	virtual const NDb::SWeaponRPGStats::SShell::ETrajectoryType GetTrajType() const = 0;
};

class CBallisticTraj: public IBallisticTraj
{
	OBJECT_BASIC_METHODS( CBallisticTraj	);
	ZDATA
	CVec3 vStart3D;
	// скорости
	float fVx, fVy;
	// ускорения свободного падения

	SAIAngle wAngle; //вертикальнй угол

	SAIAngle wDir;
	CVec2 vDir;

	float fG; // для данной траектории ускорение свободного падения

	NTimer::STime startTime, explTime;

	NDb::SWeaponRPGStats::SShell::ETrajectoryType eType;
	public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&vStart3D); f.Add(3,&fVx); f.Add(4,&fVy); f.Add(5,&wAngle); f.Add(6,&wDir); f.Add(7,&vDir); f.Add(8,&fG); f.Add(9,&startTime); f.Add(10,&explTime); f.Add(11,&eType); return 0; }
public:
	CBallisticTraj() { }
	CBallisticTraj( const CVec3 &vStart, const CVec3 &vFinish, float fV, const NDb::SWeaponRPGStats::SShell::ETrajectoryType eType, WORD wMaxAngle, float fMaxRange );
	
	virtual const NTimer::STime& GetExplTime() const { return explTime; }
	virtual const NTimer::STime& GetStartTime() const { return startTime; }
	virtual const CVec3& GetStartPoint() const { return vStart3D; }
	virtual const WORD GetStart2DDir() const { return wDir; }

	virtual const CVec3 GetCoordinates() const;

	static WORD GetTrajectoryZAngle( const CVec3 &vToAim, float fV, const NDb::SWeaponRPGStats::SShell::ETrajectoryType eType, WORD wMaxAngle, float fMaxRange );

	virtual const NDb::SWeaponRPGStats::SShell::ETrajectoryType GetTrajType() const { return eType; }
};

class CFakeBallisticTraj : public IBallisticTraj
{
	OBJECT_BASIC_METHODS( CFakeBallisticTraj );
	ZDATA

	NTimer::STime startTime, explTime;

	CVec3 point;
	CVec3 v;
	float A1, A2;
	SAIAngle wDir;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&startTime); f.Add(3,&explTime); f.Add(4,&point); f.Add(5,&v); f.Add(6,&A1); f.Add(7,&A2); f.Add(8,&wDir); return 0; }
public:
	CFakeBallisticTraj() { }
	CFakeBallisticTraj( const CVec3 &point, const CVec3 &v, const NTimer::STime &explTime, const float A1, const float A2 );

	virtual const NTimer::STime& GetExplTime() const { return explTime; }
	virtual const NTimer::STime& GetStartTime() const { return startTime; }

	virtual const CVec3& GetStartPoint() const { return point; }
	virtual const WORD GetStart2DDir() const { return wDir; }

	virtual const CVec3 GetCoordinates() const;
	static WORD GetTrajectoryZAngle( const CVec3 &vToAim, float fV, const NDb::SWeaponRPGStats::SShell::ETrajectoryType eType ) { return 0; }

	virtual const NDb::SWeaponRPGStats::SShell::ETrajectoryType GetTrajType() const;
};

class CBombBallisticTraj : public IBallisticTraj
{
	OBJECT_BASIC_METHODS( CBombBallisticTraj );

	ZDATA
	CVec3 point;
	CVec3 v;
	CVec2 vRandAcc;
	SAIAngle wDir;
	NTimer::STime startTime, explTime;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&point); f.Add(3,&v); f.Add(4,&vRandAcc); f.Add(5,&wDir); f.Add(6,&startTime); f.Add(7,&explTime); return 0; }

public:
	CBombBallisticTraj() { };
	CBombBallisticTraj( const CVec3 &point, const CVec3 &v, const NTimer::STime &explTime, const CVec2 &vRandAcc );

	virtual const NTimer::STime& GetExplTime() const { return explTime; }
	virtual const NTimer::STime& GetStartTime() const { return startTime; }

	virtual const CVec3& GetStartPoint() const { return point; }
	virtual const WORD GetStart2DDir() const { return wDir; }

	virtual const CVec3 GetCoordinates() const;

	const NDb::SWeaponRPGStats::SShell::ETrajectoryType GetTrajType() const { return NDb::SWeaponRPGStats::SShell::TRAJECTORY_BOMB; }

	static float GetCoeff( const float &timeDiff );
	static WORD GetTrajectoryZAngle( const CVec3 &vToAim, float fV, const NDb::SWeaponRPGStats::SShell::ETrajectoryType eType ) { return 16384 * 3; }
	static CVec3 CalcTrajectoryFinish( const CVec3 &vSourcePoint, const CVec3 &vInitialSpeed, const CVec2 &vRandAcc, const float fTimeOfFly );
	static float GetTimeOfFly( const float fZ, const float fZSpeed );
};

class CAARocketTraj: public IBallisticTraj
{
	OBJECT_BASIC_METHODS( CAARocketTraj	);
	ZDATA
	CVec3 vStart3D;
	SAIAngle wDir;
	CVec3 vSpeed;

	NTimer::STime startTime, explTime;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&vStart3D); f.Add(3,&wDir); f.Add(4,&vSpeed); f.Add(5,&startTime); f.Add(6,&explTime); return 0; }
public:
	CAARocketTraj() { }
	CAARocketTraj( const CVec3 &vStart, const CVec3 &vFinish, float fV );

	virtual const NTimer::STime& GetExplTime() const { return explTime; }
	virtual const NTimer::STime& GetStartTime() const { return startTime; }
	virtual const CVec3& GetStartPoint() const { return vStart3D; }
	virtual const WORD GetStart2DDir() const { return wDir; }

	virtual const CVec3 GetCoordinates() const;

	virtual const NDb::SWeaponRPGStats::SShell::ETrajectoryType GetTrajType() const { return NDb::SWeaponRPGStats::SShell::TRAJECTORY_ROCKET; }
};

// A deterministic, target-aware trajectory for wire/radio-guided ATGMs.
class CATGMTraj : public IBallisticTraj
{
	OBJECT_BASIC_METHODS( CATGMTraj );
	ZDATA
	CVec3 vStart3D;
	CVec3 vCenter;
	CVec3 vVelocity;
	CVec3 vFixedTarget;
	CPtr<CAIUnit> pTarget;
	int nShooterParty;
	float fSpeed;
	float fTurnRateRad;
	float fStrayModeTime;
	NTimer::STime startTime;
	NTimer::STime lastUpdateTime;
	NTimer::STime explTime;
	SAIAngle wStartDir;
	bool bStrayMode;
	bool bFinished;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&vStart3D); f.Add(3,&vCenter); f.Add(4,&vVelocity); f.Add(5,&vFixedTarget); f.Add(6,&pTarget); f.Add(7,&nShooterParty); f.Add(8,&fSpeed); f.Add(9,&fTurnRateRad); f.Add(10,&fStrayModeTime); f.Add(11,&startTime); f.Add(12,&lastUpdateTime); f.Add(13,&explTime); f.Add(14,&wStartDir); f.Add(15,&bStrayMode); f.Add(16,&bFinished); return 0; }

	CVec3 GetGuidancePoint() const;
	void EnterStrayMode();
	float FindImpactRatio( const CVec3 &vFrom, const CVec3 &vTo ) const;
public:
	CATGMTraj() : nShooterParty( -1 ), fSpeed( 0.0f ), fTurnRateRad( 0.0f ), fStrayModeTime( 0.0f ), bStrayMode( false ), bFinished( false ) { }
	CATGMTraj( const CVec3 &vStart, const CVec3 &vFinish, float fV, CAIUnit *pTarget, int nShooterParty, const NDb::SMissleParams *pParams );

	virtual const NTimer::STime& GetExplTime() const { return explTime; }
	virtual const NTimer::STime& GetStartTime() const { return startTime; }
	virtual const CVec3& GetStartPoint() const { return vStart3D; }
	virtual const WORD GetStart2DDir() const { return wStartDir; }
	virtual const CVec3 GetCoordinates() const { return vCenter; }
	virtual const CVec3 GetVelocity() const { return vVelocity; }
	virtual void Segment();
	virtual bool IsFinished( const NTimer::STime &time ) const { return bFinished; }
	virtual const NDb::SWeaponRPGStats::SShell::ETrajectoryType GetTrajType() const { return NDb::SWeaponRPGStats::SShell::TRAJECTORY_ATGM_LINE; }
};
//*******************************************************************
//*								  Взрывы																					*
//*******************************************************************

class CExplosion : public CAIObjectBase
{
protected:
	ZDATA
	BYTE nShellType;
	CDBPtr<SWeaponRPGStats> pWeapon;
	CPtr<CAIUnit> pUnit;
	
	CVec3 explCoord;
	SAIAngle attackDir;
	int nPlayerOfShoot;

	CPtr<CHitInfo> pHitToSend;
	NDb::SUnitStatsModifier::SParameterModifier weaponDamageModifier;
	NDb::SUnitStatsModifier::SParameterModifier weaponPiercingModifier;
	NDb::SUnitStatsModifier::SParameterModifier weaponAreaModifier;
	NDb::SUnitStatsModifier::SParameterModifier weaponArea2Modifier;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nShellType); f.Add(3,&pWeapon); f.Add(4,&pUnit); f.Add(5,&explCoord); f.Add(6,&attackDir); f.Add(7,&nPlayerOfShoot); f.Add(8,&pHitToSend); f.Add(9,&weaponDamageModifier); f.Add(10,&weaponPiercingModifier); f.Add(11,&weaponAreaModifier); f.Add(12,&weaponArea2Modifier); return 0; }
protected:
	//
	const SAINotifyHitInfo::EHitType ProcessExactHit( class CAIUnit *pTarget, const SRect &combatRect, const CVec3 &explCoord, const int nRandPiercing, const int nRandArmor ) const;
	void Init( class CAIUnit *pUnit, const struct SWeaponRPGStats *pWeapon, const float fDispersion, const float fDispRatio, const CVec3 &_explCoord, const CVec3 &attackerPos, const BYTE nShellType, const bool bRandomize, const int nPlayerOfShoot );
	//
	bool InOnGround( const float fExplTerrainZ ) const 
	{ 
		return fabs(GetExplCoordinates().z - fExplTerrainZ) <= SConsts::TILE_SIZE; 
	}
public:
	CExplosion() : nPlayerOfShoot( -1 ) { }
	CExplosion( CAIUnit *pUnit, const class CBasicGun *pGun, const CVec3 &explCoord, const CVec3 &attackerPos, const BYTE nShellType, const bool bRandomize = true );
	CExplosion( CAIUnit *pUnit, const struct SWeaponRPGStats *pWeapon, const CVec3 &explCoord, const CVec3 &attackerPos, const BYTE nShellType, const bool bRandomize = true );

	const CVec3& GetExplCoordinates() const { return explCoord; }
	CAIUnit* GetWhoFire() const { return pUnit; }

	const SWeaponRPGStats *GetWeapon() const { return pWeapon; }
	const BYTE GetShellType() const { return nShellType; }
	const SWeaponRPGStats::SShell& GetShellStats() const { return pWeapon->shells[nShellType]; }
	const WORD GetAttackDir() const { return attackDir; }
	// Guided shells update the eventual impact as their path changes.
	void SetExplCoordinates( const CVec3 &vCoordinates ) { explCoord = vCoordinates; }
	void SetAttackDir( const CVec3 &vVelocity ) { attackDir = GetDirectionByVector( -vVelocity.x, -vVelocity.y ); }

	const int GetRandomPiercing() const;
	const float GetRandomDamage() const;
	const float GetArea() const;
	const float GetArea2() const;
	
	const int GetPartyOfShoot() const;
	const int GetPlayerOfShoot() const;

	virtual void Explode() = 0;
	float GetMaxDamage() const ;
	NDb::SWeaponRPGStats::SShell::ETrajectoryType GetTrajectoryType() const { return pWeapon->shells[nShellType].etrajectory; }
	
	// если explosion дымовой, то вернется true.
	bool ProcessSmokeScreenExplosion() const;

	void AddHitToSend( CHitInfo *pHit );
};

namespace NAsyncExplosionDebug
{
	unsigned __int64 GetRandomCallCounter();
	void SetAreaDamageCandidate( const CExplosion *pExpl, CAIUnit *pTarget, int nRawIndex, int nSortedIndex, int nDuplicateCount, int nRawCount, int nUniqueCount );
	void ClearAreaDamageCandidate();
	void RecordAreaDamageTrace( const char *szStage, const CExplosion *pExpl, CAIUnit *pTarget, int nArmorDir, float fRadius, float fSmallRadius,
		bool bPreconditionsPassed, bool bCoverCalled, bool bSavedByCover, bool bCircleHit, bool bArmorPassed, bool bDamageApplied,
		float fDist2, float fZDiff, unsigned __int64 nRngBefore, unsigned __int64 nRngAfter );
	void ResetAreaDamageTrace();
	void DumpAreaDamageTrace( FILE* f );
}

class CBurstExpl : public CExplosion
{
	OBJECT_BASIC_METHODS( CBurstExpl );
	ZDATA_(CExplosion)
	int nArmorDir;
	bool bShowEffect;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExplosion*)this); f.Add(2,&nArmorDir); f.Add(3,&bShowEffect); return 0; }
public:
	CBurstExpl() { }
	// nArmorDir == 0  -  просто по плоскому направлению ( это для снарядов )
	// nArmorDir == 1  -  взрыв под днищем ( для мин )
	// nArmorDir == 2  -  взрыв над крышей

	CBurstExpl( CAIUnit *pUnit, const class CBasicGun *pGun, const CVec3 &explCoord, const CVec3 &attackerPos, const BYTE nShellType, const bool bRandomize, const int ArmorDir, const bool bShowEffect );
	CBurstExpl( CAIUnit *pUnit, const SWeaponRPGStats *pWeapon, const CVec3 &explCoord, const CVec3 &attackerPos, const BYTE nShellType, const bool bRandomize, const int ArmorDir, const bool bShowEffect );

	virtual void Explode();
};

class CCumulativeExpl : public CExplosion
{
	OBJECT_BASIC_METHODS( CCumulativeExpl );
	ZDATA_(CExplosion)
	int nArmorDir;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExplosion*)this); f.Add(2,&nArmorDir); return 0; }
public:
	CCumulativeExpl() { }
	CCumulativeExpl( CAIUnit *pUnit, const class CBasicGun *pGun, const CVec3 &explCoord, const CVec3 &attackerPos, const BYTE nShellType, const bool bRandomize = true );

	virtual void Explode();
};

class CFlameThrowerExpl : public CExplosion
{
	OBJECT_BASIC_METHODS( CFlameThrowerExpl )
	ZDATA_(CExplosion)
	CVec3 vShooterPos;
	CVec3 vTargetPos;
	ZEND
public:
	CFlameThrowerExpl() { }
	CFlameThrowerExpl( CAIUnit *pUnit, const class CBasicGun *pGun,
										 const CVec3 &explCoord, const CVec3 &attackerPos, 
										 const BYTE nShellType, const bool bRandomize = true );
	void Explode();
};

//*******************************************************************
//*								  Снаряды																					*
//*******************************************************************

// снаряд, попадающй мгновенно
class CMomentShell
{
	ZDATA

	CPtr<CExplosion> expl;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&expl); return 0; }
public:
	CMomentShell( CExplosion *_expl ) : expl( _expl ) { }

	const CVec3& GetExplCoordinates() const { return expl->GetExplCoordinates(); }
	void Explode() { expl->Explode(); }

	float GetMaxDamage() const { return expl->GetMaxDamage(); }
};

// снаряд
class CShell
{

	ZDATA
	NTimer::STime explTime;
	CPtr<CExplosion> expl;
	int nGun;

	float vStartVisZ, vFinishVisZ;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&explTime); f.Add(3,&expl); f.Add(4,&nGun); f.Add(5,&vStartVisZ); f.Add(6,&vFinishVisZ); return 0; }

protected:
	const float GetStartVisZ() const { return vStartVisZ; }
	const float GetFinishVisZ() const { return vFinishVisZ; }
	void SetDynamicImpact( const CVec3 &vCoordinates, const CVec3 &vVelocity ) { expl->SetExplCoordinates( vCoordinates ); expl->SetAttackDir( vVelocity ); }
public:
	CShell() { }
	CShell( const NTimer::STime &explTime, CExplosion *expl, const int nGun );

	const NTimer::STime GetExplTime() const { return explTime; }

	const CVec3& GetExplCoordinates() const { return expl->GetExplCoordinates(); }	
	const SWeaponRPGStats *GetWeapon() const { return expl->GetWeapon(); }
	const BYTE GetShellType() const { return expl->GetShellType(); }

	CObjectBase* GetWhoFired() const;
	const int GetNGun() const { return nGun; }

	void Explode() { expl->Explode(); }
	float GetMaxDamage() const { return expl->GetMaxDamage(); }
	NDb::SWeaponRPGStats::SShell::ETrajectoryType GetTrajectoryType() const { return expl->GetTrajectoryType(); }
};

// невидимый снаряд
class CInvisShell : public CAIObjectBase, public CShell
{
	OBJECT_BASIC_METHODS( CInvisShell );
	ZDATA_(CShell)
	int nOrder;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CShell*)this); f.Add(2,&nOrder); return 0; }
public:
	CInvisShell() : nOrder( 0 ) { }
	CInvisShell( const NTimer::STime &explTime, CExplosion *expl, const int nGun )
		: CShell( explTime, expl, nGun ), nOrder( 0 ) { }

	void SetOrder( const int _nOrder ) { nOrder = _nOrder; }
	const int GetOrder() const { return nOrder; }
};

// видимый снаряд
class CVisShell : public CLinkObject, public CShell
{
	OBJECT_BASIC_METHODS( CVisShell );		
public: int operator&( IBinSaver &saver ); private:
	CPtr<IBallisticTraj> pTraj;
	CVec3 center;
	CVec3 speed;
	bool bVisible;
	int nPlatform;
	int nOrder;
	
	void CalcVisibility();
public:
	CVisShell() : nOrder( 0 ) { }
	CVisShell( CExplosion *_expl, IBallisticTraj *_pTraj, const int nGun, const int nPlatform );

	const NTimer::STime GetStartTime() const { return pTraj->GetStartTime(); }
	const NTimer::STime GetExplTime() const { return pTraj->GetExplTime(); }
	bool IsFinished( const NTimer::STime &time ) const { return pTraj->IsFinished( time ); }

	virtual void GetPlacement( struct SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff );
	virtual void GetSpeed3( CVec3 *pSpeed ) const { *pSpeed = speed; }
	virtual void GetProjectileInfo( struct SAINotifyNewProjectile *pProjectileInfo );

	void Segment();

	const CVec3 GetCoordinates() const { return pTraj->GetCoordinates(); }

	virtual float GetTerrainHeight( const float x, const float y, const NTimer::STime timeDiff ) const;
	virtual const bool IsVisible( const BYTE party ) const { return true; }
	virtual void GetTilesForVisibility( CTilesSet *pTiles ) const { pTiles->clear(); }
	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const { return false; }
	
	virtual const bool IsVisibleByPlayer() const;

	void SetOrder( const int _nOrder ) { nOrder = _nOrder; }
	const int GetOrder() const { return nOrder; }
};

//*******************************************************************
//*								  Склад снарядов																	*
//*******************************************************************

// for remove ambiguity with CPtr opeartor <
struct SInvisShell
{
	ZDATA
	CPtr<CInvisShell> ptr;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&ptr); return 0; }
public:
	SInvisShell() {  }
	SInvisShell ( CInvisShell * p ) : ptr( p ) {  }
	SInvisShell ( CPtr<CInvisShell> &s ) : ptr( s ) {  }

	CInvisShell* operator->() const { return ptr; }
	operator CInvisShell*() const { return ptr; }
};

struct SInvisShellCompare
{
	bool operator()( const SInvisShell &s1, const SInvisShell &s2 ) const
	{
		const NTimer::STime s1e = s1.ptr->GetExplTime();
		const NTimer::STime s2e = s2.ptr->GetExplTime();
		if ( s1e != s2e )
			return s1e > s2e;
		return s1.ptr->GetOrder() > s2.ptr->GetOrder();
	}
};
typedef std::list< CObj<CVisShell> > CVisShellList;
struct SVisShellCompare
{
	bool operator()( const CObj<CVisShell> &s1, const CObj<CVisShell> &s2 ) const
	{
		const NTimer::STime s1e = s1->GetExplTime();
		const NTimer::STime s2e = s2->GetExplTime();
		if ( s1e != s2e )
			return s1e < s2e;
		if ( s1->GetOrder() != s2->GetOrder() )
			return s1->GetOrder() < s2->GetOrder();
		return s1->GetUniqueId() < s2->GetUniqueId();
	}
};

class CShellsStore
{

	// все невидимые снаряды
	typedef std::priority_queue< SInvisShell, std::vector<SInvisShell>, SInvisShellCompare > CInvisShells;
	ZDATA
	CInvisShells invisShells;
	// все видимые снаряды
	CVisShellList visShells;
	int nNextInvisShellOrder;
	int nNextVisShellOrder;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&invisShells); f.Add(3,&visShells); f.Add(4,&nNextInvisShellOrder); f.Add(5,&nNextVisShellOrder); return 0; }

public:
	CShellsStore() : nNextInvisShellOrder( 0 ), nNextVisShellOrder( 0 ) { }
	void Clear();

	void AddShell( CMomentShell	&shell ); 
	void AddShell( CInvisShell *pShell );
	void AddShell( CVisShell *pShell );

	void Segment();
	
	// для тестирования multiplayer
	void UpdateCheckSum( uLong *pCheckSum );
	void UpdateDebugChecksums(FILE* f);
};


