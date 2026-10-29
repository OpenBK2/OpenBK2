#pragma once
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "Obstacle.h"
#include "ShootEstimator.h"

#include <cstdint>

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
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	int nBestGun = {};
	CPtr<CAIUnit> pCurTarget;
	bool bDamageToCurTargetUpdated = {};

	float fBestRating = {};
	uint32_t dwForbidden = {};
	uint32_t dwDefaultForbidden = {};

	CDBPtr<SUnitBaseRPGStats> pMosinStats;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pOwner );
		f.Add( 3, &pBestUnit );
		f.Add( 4, &pBestGun );
		f.Add( 5, &nBestGun );
		f.Add( 6, &pCurTarget );
		f.Add( 7, &bDamageToCurTargetUpdated );
		f.Add( 8, &fBestRating );
		f.Add( 9, &dwForbidden );
		f.Add( 10, &dwDefaultForbidden );
		f.Add( 11, &pMosinStats );
		return 0;
	}

	// время, требуемое, чтобы повернуть pGun на pEnemy
	//const float FindTimeToTurn( CAIUnit *pEnemy, CBasicGun *pGun ) const;
	// выбрать gun для pEnemy
	void ChooseGun( CBasicGun **pBestGun, int *nBestGun, CAIUnit *pEnemy );
	bool IsStationaryTarget( CAIUnit *pEnemy ) const;

	const float GetRating( CAIUnit *pEnemy, CBasicGun *pGun ) const;
	const float GetRating( const SUnitBaseRPGStats *pStats, const CVec2 &vCenter, CBasicGun *pGun ) const;

public:
	CTankShootEstimator()
		: pOwner( 0 )
	{
	}

	explicit CTankShootEstimator( class CAIUnit *pOwner );

	void Reset( class CAIUnit *pCurEnemy, const bool bDamageUpdated, const uint32_t dwForbidden ) override;
	void AddUnit( class CAIUnit *pEnemy ) override;
	class CAIUnit *GetBestUnit() const override;
	class CBasicGun *GetBestGun() const override;
	const int GetNumberOfBestGun() const override;
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
	int nBestGun = {};
	CPtr<CAIUnit> pCurTarget;
	bool bDamageToCurTargetUpdated = {};

	float fBestRating = {};

	bool bHasGrenades = {};
	// бросаем гранату, не учитываю общую функцию выбора цели по рейтингу
	bool bThrowGrenade = {};
	bool bUseGrenadeFixed = {};
	bool bUseGrenadeAutocast = {};

	uint32_t dwForbidden = {};

	CDBPtr<SUnitBaseRPGStats> pMosinStats;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pOwner );
		f.Add( 3, &pBestUnit );
		f.Add( 4, &pBestGun );
		f.Add( 5, &nBestGun );
		f.Add( 6, &pCurTarget );
		f.Add( 7, &bDamageToCurTargetUpdated );
		f.Add( 8, &fBestRating );
		f.Add( 9, &bHasGrenades );
		f.Add( 10, &bThrowGrenade );
		f.Add( 11, &bUseGrenadeFixed );
		f.Add( 12, &bUseGrenadeAutocast );
		f.Add( 13, &dwForbidden );
		f.Add( 14, &pMosinStats );
		return 0;
	}

	// выбрать gun для pEnemy
	void ChooseGun( CBasicGun **pBestGun, int *nBestGun, CAIUnit *pEnemy );

	const float GetRating( CAIUnit *pEnemy, CBasicGun *pGun ) const;
	const float GetRating( const SUnitBaseRPGStats *pStats, const CVec2 &vCenter, CBasicGun *pGun ) const;

public:
	CSoldierShootEstimator()
		: pOwner( 0 )
	{
	}

	explicit CSoldierShootEstimator( class CAIUnit *pOwner );

	void Reset( class CAIUnit *pCurEnemy, const bool bDamageUpdated, const uint32_t dwForbidden ) override;
	void AddUnit( class CAIUnit *pEnemy ) override;
	class CAIUnit *GetBestUnit() const override;
	class CBasicGun *GetBestGun() const override;
	const int GetNumberOfBestGun() const override;
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
	bool bDamageToCurTargetUpdated = {};
	float fBestRating = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pOwner );
		f.Add( 3, &pBestUnit );
		f.Add( 4, &pCurTarget );
		f.Add( 5, &pGun );
		f.Add( 6, &bDamageToCurTargetUpdated );
		f.Add( 7, &fBestRating );
		return 0;
	}

	void OnSerialize( IBinSaver &f );
	const float CalcTimeToOpenFire( class CAIUnit *pEnemy, CBasicGun *pGun )
			const; // время для открытия огня (учитывая поворот оружия и скорость сближения с врагом)

	const float CalcRating( CAIUnit *pEnemy, CBasicGun *pGun ) const;

public:
	CPlaneDeffensiveFireShootEstimator()
		: pOwner( 0 )
	{
	}

	explicit CPlaneDeffensiveFireShootEstimator( class CAIUnit *pOwner );

	void SetGun( CBasicGun *_pGun );

	void Reset( class CAIUnit *pCurEnemy,
	            const bool bDamageUpdated,
	            const uint32_t dwForbidden ) override; //dwFirbidden is ignored
	void AddUnit( class CAIUnit *pEnemy ) override;
	class CAIUnit *GetBestUnit() const override;
	class CBasicGun *GetBestGun() const override;
	const int GetNumberOfBestGun() const override;
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
		bool bCanTargetShootToPlanes = {};
		bool bCanAttackerBreakTarget = {};
		SAIAngle wSpeedDiff;
		SAIAngle wDirToTarget;
		float fRating = {};

		ZEND int operator&( IBinSaver &f )
		{
			f.Add( 2, &pTarget );
			f.Add( 3, &bCanTargetShootToPlanes );
			f.Add( 4, &bCanAttackerBreakTarget );
			f.Add( 5, &wSpeedDiff );
			f.Add( 6, &wDirToTarget );
			f.Add( 7, &fRating );
			return 0;
		}

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

		STargetInfo()
		{
			Reset();
		}
	};

	typedef det_set<int /*unique id of building*/> CBuildings;
	ZDATA
	CPtr<CAIUnit> pOwner;
	CPtr<CAIUnit> pCurEnemy;
	CVec2 vCenter;

	STargetInfo bestForGuns;
	STargetInfo bestForBombs;
	STargetInfo bestAviation;

	CBuildings buildings;
	CPtr<CBuilding> pBestBuilding;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pOwner );
		f.Add( 3, &pCurEnemy );
		f.Add( 4, &vCenter );
		f.Add( 5, &bestForGuns );
		f.Add( 6, &bestForBombs );
		f.Add( 7, &bestAviation );
		f.Add( 8, &buildings );
		f.Add( 9, &pBestBuilding );
		return 0;
	}

	const float CalcTimeToOpenFire( CAIUnit *pEnemy ) const;
	void CollectTarget( CPlaneShturmovikShootEstimator::STargetInfo *pInfo,
	                    class CAIUnit *pTarget,
	                    const uint32_t dwPossibleGuns );
	const float CalcRating( CAIUnit *pEnemy, const uint32_t dwPossibleGuns ) const;

public:
	CPlaneShturmovikShootEstimator()
		: pOwner( 0 )
	{
	}

	CPlaneShturmovikShootEstimator( class CAIUnit *pOwner );

	void SetCurCenter( const CVec2 &vNewCenter )
	{
		vCenter = vNewCenter;
	}

	void Reset( class CAIUnit *pCurEnemy,
	            const bool bDamageUpdated,
	            const uint32_t dwForbidden ) override; //dwFirbidden is ignored
	void AddUnit( class CAIUnit *pEnemy ) override;
	class CAIUnit *GetBestUnit() const override;

	void CalcBestBuilding();

	class CBuilding *GetBestBuilding() const
	{
		return pBestBuilding;
	}

	class CBasicGun *GetBestGun() const override
	{
		NI_ASSERT( false, "Wrong call" );
		return 0;
	}

	const int GetNumberOfBestGun() const override
	{
		NI_ASSERT( false, "Wrong call" );
		return 0;
	}
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// для стрельбы по препятствиям.
class CShootEstimatorForObstacles : public IObstacleEnumerator
{
	ZDATA
	CPtr<CCommonUnit> pOwner;
	float fCurRating = {};
	CPtr<IObstacle> pBest;

	ZEND int operator&( IBinSaver &f )
	{
		f.Add( 2, &pOwner );
		f.Add( 3, &fCurRating );
		f.Add( 4, &pBest );
		return 0;
	}

public:
	CShootEstimatorForObstacles( class CCommonUnit *pOwner )
		: pOwner( pOwner )
		, fCurRating( 0 )
	{
	}

	bool AddObstacle( IObstacle *pObstacle ) override;
	virtual struct IObstacle *GetBest() const;
};

class CShootEstimatorSupportAAGun : public IShootEstimator
{
	OBJECT_BASIC_METHODS( CShootEstimatorSupportAAGun )

	ZDATA
	CPtr<CAIUnit> pOwner;
	CPtr<CAIUnit> pBestTarget;
	uint32_t dwForbidden = {};
	float fBestRating = {};
	bool bDamageToCurTargetUpdated = {};

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &pOwner );
		f.Add( 3, &pBestTarget );
		f.Add( 4, &dwForbidden );
		f.Add( 5, &fBestRating );
		f.Add( 6, &bDamageToCurTargetUpdated );
		return 0;
	}

	const float CalcRating( CAIUnit *pEnemy, CBasicGun *pGun ) const;

public:
	CShootEstimatorSupportAAGun() {}

	CShootEstimatorSupportAAGun( class CAIUnit *pOwner );
	void Reset( class CAIUnit *pCurEnemy, const bool bDamageUpdated, const uint32_t dwForbidden ) override;
	void AddUnit( class CAIUnit *pEnemy ) override;

	class CAIUnit *GetBestUnit() const override
	{
		return pBestTarget;
	}

	class CBasicGun *GetBestGun() const override
	{
		NI_ASSERT( false, "Wrong call" );
		return 0;
	}

	const int GetNumberOfBestGun() const override
	{
		NI_ASSERT( false, "Wrong call" );
		return 0;
	}
};
