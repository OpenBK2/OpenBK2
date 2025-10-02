
#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ShootEstimator.h"
#include "Obstacle.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAIUnit;
class CBasicGun;
namespace NDb
{
	struct SUnitBaseRPGStats;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTankShootEstimator : public IShootEstimator
{
	OBJECT_BASIC_METHODS( CTankShootEstimator );

	ZDATA
	CPtr<CAIUnit> pOwner;

	CPtr<CAIUnit> pBestUnit;
	CPtr<CBasicGun> pBestGun;
	int nBestGun;
	CPtr<CAIUnit> pCurTarget;
	bool bDamageToCurTargetUpdated;

	float fBestRating;
	DWORD dwForbidden;
	DWORD dwDefaultForbidden;
	
	CDBPtr<SUnitBaseRPGStats> pMosinStats;

	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pOwner); f.Add(3,&pBestUnit); f.Add(4,&pBestGun); f.Add(5,&nBestGun); f.Add(6,&pCurTarget); f.Add(7,&bDamageToCurTargetUpdated); f.Add(8,&fBestRating); f.Add(9,&dwForbidden); f.Add(10,&dwDefaultForbidden); f.Add(11,&pMosinStats); return 0; }
	// время, требуемое, чтобы повернуть pGun на pEnemy
	//const float FindTimeToTurn( CAIUnit *pEnemy, CBasicGun *pGun ) const;
	// выбрать gun для pEnemy
	void ChooseGun( CBasicGun **pBestGun, int *nBestGun, CAIUnit *pEnemy );

	const float GetRating( CAIUnit *pEnemy, CBasicGun *pGun ) const;
	const float GetRating( const SUnitBaseRPGStats *pStats, const CVec2 &vCenter, CBasicGun *pGun ) const;
public:
	CTankShootEstimator() : pOwner( 0 ) { }
	explicit CTankShootEstimator( class CAIUnit *pOwner );

	virtual void Reset( class CAIUnit *pCurEnemy, const bool bDamageUpdated, const DWORD dwForbidden );
	virtual void AddUnit( class CAIUnit *pEnemy );
	virtual class CAIUnit* GetBestUnit() const;
	virtual class CBasicGun* GetBestGun() const;
	virtual const int GetNumberOfBestGun() const;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CSoldierShootEstimator : public IShootEstimator
{
	OBJECT_BASIC_METHODS( CSoldierShootEstimator );

	static const int N_GOOD_NUMBER_ATTACKING_GRENADES;

	ZDATA
	CPtr<CAIUnit> pOwner;
	CPtr<CAIUnit> pBestUnit;
	CPtr<CBasicGun> pBestGun;
	int nBestGun;
	CPtr<CAIUnit> pCurTarget;
	bool bDamageToCurTargetUpdated;

	float fBestRating;

	bool bHasGrenades;
	// бросаем гранату, не учитываю общую функцию выбора цели по рейтингу
	bool bThrowGrenade;
	bool bUseGrenadeFixed;
	bool bUseGrenadeAutocast;

	DWORD dwForbidden;

	CDBPtr<SUnitBaseRPGStats> pMosinStats;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pOwner); f.Add(3,&pBestUnit); f.Add(4,&pBestGun); f.Add(5,&nBestGun); f.Add(6,&pCurTarget); f.Add(7,&bDamageToCurTargetUpdated); f.Add(8,&fBestRating); f.Add(9,&bHasGrenades); f.Add(10,&bThrowGrenade); f.Add(11,&bUseGrenadeFixed); f.Add(12,&bUseGrenadeAutocast); f.Add(13,&dwForbidden); f.Add(14,&pMosinStats); return 0; }

	// выбрать gun для pEnemy
	void ChooseGun( CBasicGun **pBestGun, int *nBestGun, CAIUnit *pEnemy );

	const float GetRating( CAIUnit *pEnemy, CBasicGun *pGun ) const;
	const float GetRating( const SUnitBaseRPGStats *pStats, const CVec2 &vCenter, CBasicGun *pGun ) const;
public:
	CSoldierShootEstimator() : pOwner( 0 ) { }
	explicit CSoldierShootEstimator( class CAIUnit *pOwner );

	virtual void Reset( class CAIUnit *pCurEnemy, const bool bDamageUpdated, const DWORD dwForbidden );
	virtual void AddUnit( class CAIUnit *pEnemy );
	virtual class CAIUnit* GetBestUnit() const;
	virtual class CBasicGun* GetBestGun() const;
	virtual const int GetNumberOfBestGun() const;
	void SetGrenadeAutocast( bool bOn );
	void SetGrenadeFixed( bool bOn );
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// стрельба из бортовых стрелковых точек для самолетов
class CPlaneDeffensiveFireShootEstimator : public IShootEstimator
{
	OBJECT_BASIC_METHODS( CPlaneDeffensiveFireShootEstimator );

	ZDATA
	CPtr<CAIUnit> pOwner;
	CPtr<CAIUnit> pBestUnit;
	CPtr<CAIUnit> pCurTarget;
	CPtr<CBasicGun> pGun;
	bool bDamageToCurTargetUpdated;
	float fBestRating;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pOwner); f.Add(3,&pBestUnit); f.Add(4,&pCurTarget); f.Add(5,&pGun); f.Add(6,&bDamageToCurTargetUpdated); f.Add(7,&fBestRating); return 0; }
		void OnSerialize( IBinSaver &f );
	const float CalcTimeToOpenFire( class CAIUnit *pEnemy, CBasicGun *pGun ) const; // время для открытия огня (учитывая поворот оружия и скорость сближения с врагом)

	const float CalcRating( CAIUnit *pEnemy, CBasicGun *pGun ) const;
public:
	CPlaneDeffensiveFireShootEstimator() : pOwner( 0 ) { }
	explicit CPlaneDeffensiveFireShootEstimator( class CAIUnit *pOwner );

	void SetGun( CBasicGun *_pGun);

	virtual void Reset( class CAIUnit *pCurEnemy, const bool bDamageUpdated, const DWORD dwForbidden );//dwFirbidden is ignored
	virtual void AddUnit( class CAIUnit *pEnemy );
	virtual class CAIUnit* GetBestUnit() const;
	virtual class CBasicGun* GetBestGun() const;
	virtual const int GetNumberOfBestGun() const;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// for 
class CBuilding;
class CPlaneShturmovikShootEstimator : public IShootEstimator
{
	OBJECT_BASIC_METHODS( CPlaneShturmovikShootEstimator );

	struct STargetInfo
	{
	public:
		ZDATA
		CPtr<CAIUnit> pTarget;
		bool bCanTargetShootToPlanes;
		bool bCanAttackerBreakTarget;
		SAIAngle wSpeedDiff;
		SAIAngle wDirToTarget;
		float fRating;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pTarget); f.Add(3,&bCanTargetShootToPlanes); f.Add(4,&bCanAttackerBreakTarget); f.Add(5,&wSpeedDiff); f.Add(6,&wDirToTarget); f.Add(7,&fRating); return 0; }
	public:
		//
		void Reset()
		{
			bCanTargetShootToPlanes = false;
			bCanAttackerBreakTarget = false;
			fRating = 0;
			wSpeedDiff = 65535;
			pTarget = 0;
			wDirToTarget = 0;
		}
		STargetInfo() { Reset(); }
	};

	typedef hash_set< int/*unique id of building*/ >  CBuildings;
	ZDATA
	CPtr<CAIUnit> pOwner;
	CPtr<CAIUnit> pCurEnemy;
	CVec2 vCenter;
	
	STargetInfo bestForGuns;
	STargetInfo bestForBombs;
	STargetInfo bestAviation;
	
	CBuildings buildings;
	CPtr<CBuilding> pBestBuilding;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pOwner); f.Add(3,&pCurEnemy); f.Add(4,&vCenter); f.Add(5,&bestForGuns); f.Add(6,&bestForBombs); f.Add(7,&bestAviation); f.Add(8,&buildings); f.Add(9,&pBestBuilding); return 0; }

	const float CPlaneShturmovikShootEstimator::CalcTimeToOpenFire( CAIUnit *pEnemy ) const;
	void CollectTarget( CPlaneShturmovikShootEstimator::STargetInfo * pInfo, class CAIUnit *pTarget, const DWORD dwPossibleGuns );
	const float CalcRating( CAIUnit *pEnemy, const DWORD dwPossibleGuns ) const;
public:
	CPlaneShturmovikShootEstimator() : pOwner( 0 ) {  }
	CPlaneShturmovikShootEstimator( class CAIUnit    *pOwner );
	void SetCurCenter( const CVec2 &vNewCenter ) { vCenter = vNewCenter; }

	virtual void Reset( class CAIUnit *pCurEnemy, const bool bDamageUpdated, const DWORD dwForbidden );//dwFirbidden is ignored
	virtual void AddUnit( class CAIUnit *pEnemy );
	virtual class CAIUnit* GetBestUnit() const;

	void CalcBestBuilding();
	class CBuilding * GetBestBuilding() const { return pBestBuilding; }

	virtual class CBasicGun* GetBestGun() const { NI_ASSERT(false,"Wrong call"); return 0;} 
	virtual const int GetNumberOfBestGun() const{ NI_ASSERT(false,"Wrong call"); return 0;} 
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// для стрельбы по препятствиям. 
class CShootEstimatorForObstacles : public IObstacleEnumerator
{
	ZDATA
	CPtr<CCommonUnit> pOwner;
	float fCurRating;
	CPtr<IObstacle> pBest;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pOwner); f.Add(3,&fCurRating); f.Add(4,&pBest); return 0; }
public:
	CShootEstimatorForObstacles( class CCommonUnit *pOwner ) : pOwner( pOwner ), fCurRating( 0 ) {  }

	virtual bool AddObstacle( IObstacle *pObstacle );
	virtual interface IObstacle * GetBest() const;
};

class CShootEstimatorSupportAAGun : public IShootEstimator
{
	OBJECT_BASIC_METHODS( CShootEstimatorSupportAAGun )

		ZDATA
	CPtr<CAIUnit> pOwner;
	CPtr<CAIUnit> pBestTarget; 
	DWORD dwForbidden;
	float fBestRating;
	bool bDamageToCurTargetUpdated ;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&pOwner); f.Add(3,&pBestTarget); f.Add(4,&dwForbidden); f.Add(5,&fBestRating); f.Add(6,&bDamageToCurTargetUpdated); return 0; }
	const float CalcRating( CAIUnit *pEnemy, CBasicGun *pGun ) const;
public:

	CShootEstimatorSupportAAGun() {  }
	CShootEstimatorSupportAAGun( class CAIUnit *pOwner );
	void Reset( class CAIUnit *pCurEnemy, const bool bDamageUpdated, const DWORD dwForbidden );
	void AddUnit( class CAIUnit *pEnemy );
	class CAIUnit* GetBestUnit() const { return pBestTarget; }

	virtual class CBasicGun* GetBestGun() const { NI_ASSERT(false,"Wrong call"); return 0;} 
	virtual const int GetNumberOfBestGun() const{ NI_ASSERT(false,"Wrong call"); return 0;} 
};


