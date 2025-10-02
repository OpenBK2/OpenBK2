#ifndef __BULLETSTORAGE_H__
#define __BULLETSTORAGE_H__

#pragma once

#include "StaticObject.h"

class CAIUnit;

// ящичек с патронами, который лежит у пушки.
// артиллеристы бегают к нему и обратно за патронами.
class CArtilleryBulletStorage: public CGivenPassabilityStObject
{
	OBJECT_BASIC_METHODS( CArtilleryBulletStorage );
	ZDATA_(CGivenPassabilityStObject)
	CDBPtr<SStaticObjectRPGStats> pStats;

	CPtr<CAIUnit> pOwner;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CGivenPassabilityStObject*)this); f.Add(2,&pStats); f.Add(3,&pOwner); return 0; }
public:
	CArtilleryBulletStorage() : pOwner( 0 ) { }
	CArtilleryBulletStorage( const SStaticObjectRPGStats* pStats, const CVec3 &center,  const float fHP, const int nFrameIndex, CAIUnit *pOwner );

	virtual const SHPObjectRPGStats* GetStats() const { return pStats; }

	virtual void TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit ) {}
	virtual void Die( const float fDamage ) { }
	virtual void Segment() { }

	void MoveTo( const CVec3 &newCenter );

	virtual EStaticObjType GetObjectType() const { return ESOT_ARTILLERY_BULLET_STORAGE; }
	
	virtual bool IsContainer() const { return false; }
	virtual const int GetNDefenders() const { return 0; }
	virtual class CSoldier* GetUnit( const int n ) const { return 0; }
	
	virtual bool CanUnitGoThrough( const EAIClasses &eClass ) const { return true; }

	CAIUnit* GetOwner() const { return pOwner; }
};

#endif // __BRIDGE_H__

