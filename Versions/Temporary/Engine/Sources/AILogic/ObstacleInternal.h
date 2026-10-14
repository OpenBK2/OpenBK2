#pragma once

#include "Obstacle.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CObstacle : public IObstacle
{
	ZDATA
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	float fFirePower = {};

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 2, &fFirePower );
		return 0;
	}

public:
	CObstacle()
		: fFirePower( 0 )
	{
	}

	void UpdateTakenDamagePower( const float fUpdate ) override
	{
		fFirePower += fUpdate;
	}

	const float GetTakenDamagePower() const override
	{
		return fFirePower;
	}
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CStaticObject;

class CObstacleStaticObject : public CObstacle
{
	OBJECT_BASIC_METHODS( CObstacleStaticObject );
	ZDATA_( CObstacle )
	CPtr<CStaticObject> pObj;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CObstacle *)this );
		f.Add( 2, &pObj );
		return 0;
	}

public:
	CObstacleStaticObject() {}

	CObstacleStaticObject( class CStaticObject *pObj )
		: pObj( pObj )
	{
	}

	class CBasicGun *ChooseGunToShootToSelf( class CCommonUnit *pUnit, NTimer::STime *pTime ) override;
	int GetPlayer() const override;
	float GetHPPercent() const override;
	const CVec3 GetCenter() const override;
	bool IsAlive() const override;
	void IssueUnitAttackCommand( class CCommonUnit *pUnit ) override;

	bool CanDeleteByMovingOver( class CAIUnit *pUnit ) override;
	class CUpdatableObj *GetObject() const override;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
