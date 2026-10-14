#ifndef __BULLETSTORAGE_H__
#define __BULLETSTORAGE_H__

#pragma once

#include "StaticObject.h"

class CAIUnit;

// ящичек с патронами, который лежит у пушки.
// артиллеристы бегают к нему и обратно за патронами.
class CArtilleryBulletStorage : public CGivenPassabilityStObject
{
	OBJECT_BASIC_METHODS( CArtilleryBulletStorage );
	ZDATA_( CGivenPassabilityStObject )
	CDBPtr<SStaticObjectRPGStats> pStats;

	CPtr<CAIUnit> pOwner;

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CGivenPassabilityStObject *)this );
		f.Add( 2, &pStats );
		f.Add( 3, &pOwner );
		return 0;
	}

public:
	CArtilleryBulletStorage()
		: pOwner( 0 )
	{
	}

	CArtilleryBulletStorage( const SStaticObjectRPGStats *pStats,
	                         const CVec3 &center,
	                         const float fHP,
	                         const int nFrameIndex,
	                         CAIUnit *pOwner );

	const SHPObjectRPGStats *GetStats() const override
	{
		return pStats;
	}

	void
	TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit ) override
	{
	}

	void Die( const float fDamage ) override {}

	void Segment() override {}

	void MoveTo( const CVec3 &newCenter );

	EStaticObjType GetObjectType() const override
	{
		return ESOT_ARTILLERY_BULLET_STORAGE;
	}

	bool IsContainer() const override
	{
		return false;
	}

	const int GetNDefenders() const override
	{
		return 0;
	}

	class CSoldier *GetUnit( const int n ) const override
	{
		return 0;
	}

	bool CanUnitGoThrough( const EAIClasses &eClass ) const override
	{
		return true;
	}

	CAIUnit *GetOwner() const
	{
		return pOwner;
	}
};

#endif // __BRIDGE_H__
