#pragma once

#include "Obstacle.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CObstacle : public IObstacle
{
	ZDATA
	float fFirePower;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(2,&fFirePower); return 0; }
public:
	CObstacle() : fFirePower( 0 ) {  }

	virtual void UpdateTakenDamagePower( const float fUpdate ) { fFirePower += fUpdate; }
	virtual const float GetTakenDamagePower() const { return fFirePower; }

};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CStaticObject;
class CObstacleStaticObject : public CObstacle
{
	OBJECT_BASIC_METHODS(CObstacleStaticObject);
	ZDATA_(CObstacle)
	CPtr<CStaticObject> pObj;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CObstacle*)this); f.Add(2,&pObj); return 0; }
public:
	CObstacleStaticObject() {  }
	CObstacleStaticObject( class CStaticObject *pObj ) : pObj( pObj ) {  }

	virtual class CBasicGun* ChooseGunToShootToSelf( class CCommonUnit *pUnit, NTimer::STime *pTime );
	virtual int GetPlayer() const;
	virtual float GetHPPercent() const;
	const CVec3 GetCenter() const;
	virtual bool IsAlive() const ;
	virtual void IssueUnitAttackCommand( class CCommonUnit *pUnit );
	
	virtual bool CanDeleteByMovingOver( class CAIUnit * pUnit );
	class CUpdatableObj * GetObject() const ;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

