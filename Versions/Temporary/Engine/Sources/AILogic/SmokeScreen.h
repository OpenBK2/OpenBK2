#pragma once

#include "StaticObject.h"

#include <cstdint>

class CSmokeScreen : public CExistingObject
{
	OBJECT_BASIC_METHODS( CSmokeScreen );

	ZDATA_( CExistingObject )
	CVec3 vCenter;
	SVector tileCenter;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	float fRadius = {};
	int nTransparency = {};
	NTimer::STime timeOfDissapear = {};

	NTimer::STime nextSegmTime = {};
	bool bTransparencySet = {};

public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CExistingObject *)this );
		f.Add( 2, &vCenter );
		f.Add( 3, &tileCenter );
		f.Add( 4, &fRadius );
		f.Add( 5, &nTransparency );
		f.Add( 6, &timeOfDissapear );
		f.Add( 7, &nextSegmTime );
		f.Add( 8, &bTransparencySet );
		return 0;
	}

	//
	void OctupleTrace( const int x, const int y, const bool bAdd );
	void TraceToPoint( const int x, const int y, const bool bAdd );
	void Trace( const bool bAdd );

protected:
	void SetNewPlaceWithoutMapUpdate( const CVec3 &center, const uint16_t dir = 0 ) override {}

public:
	CSmokeScreen()
		: bTransparencySet( false )
	{
	}

	CSmokeScreen( const CVec3 &vCenter, const float fRadius, const int nTransparency, const int nTime );
	virtual void Init();

	const SHPObjectRPGStats *GetStats() const override
	{
		return 0;
	}

	void Segment() override;

	const NTimer::STime GetNextSegmentTime() const override
	{
		return nextSegmTime;
	}

	// сдетонировать, если при наезде данного юнита мина взрывается; true - если сдетонировала
	void
	TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit ) override
	{
	}

	void Die( const float fDamage ) override {}

	EStaticObjType GetObjectType() const override
	{
		return ESOT_SMOKE_SCREEN;
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

	void LockTiles() override {}

	void UnlockTiles() override {}

	void CreateLockedTilesInfo( std::list<SObjTileInfo> *pTiles ) override
	{
		pTiles->clear();
	}

	void SetTransparencies() override;
	void RemoveTransparencies() override;
	void RestoreTransparenciesImmidiately() override;

	void GetCoveredTiles( std::list<SVector> *pTiles ) const override;

	void GetRPGStats( struct SAINotifyRPGStats *pStats ) override {}

	const CVec3 &GetCenter() const override
	{
		return vCenter;
	}

	const CVec2 GetAttackCenter( const CVec2 &vPoint ) const override
	{
		return CVec2( vCenter.x, vCenter.y );
	}

	void GetBoundRect( SRect *pRect ) const override
	{
		pRect->InitRect( CVec2( GetCenter().x, GetCenter().y ), CVec2( 1.0f, 1.0f ), 0.0f, 0.0f );
	}

	bool IsPointInside( const CVec2 &point ) const override
	{
		return false;
	}

	const uint16_t GetDir() const override
	{
		return 0;
	}

	CObjectProfile *GetPassProfile() const override
	{
		return 0;
	}
};
