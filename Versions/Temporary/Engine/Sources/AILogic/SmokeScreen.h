
#pragma once

#include "StaticObject.h"

class CSmokeScreen : public CExistingObject
{
	OBJECT_BASIC_METHODS( CSmokeScreen );

	ZDATA_(CExistingObject)
	CVec3 vCenter;
	SVector tileCenter;
	float fRadius;
	int nTransparency;
	NTimer::STime timeOfDissapear;

	NTimer::STime nextSegmTime;
	bool bTransparencySet;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CExistingObject*)this); f.Add(2,&vCenter); f.Add(3,&tileCenter); f.Add(4,&fRadius); f.Add(5,&nTransparency); f.Add(6,&timeOfDissapear); f.Add(7,&nextSegmTime); f.Add(8,&bTransparencySet); return 0; }

	//
	void OctupleTrace( const int x, const int y, const bool bAdd );
	void TraceToPoint( const int x, const int y, const bool bAdd );
	void Trace( const bool bAdd );
protected:
	virtual void SetNewPlaceWithoutMapUpdate( const CVec3 &center, const WORD dir = 0 ) { }
public:
	CSmokeScreen() : bTransparencySet( false ) { }
	CSmokeScreen( const CVec3 &vCenter, const float fRadius, const int nTransparency, const int nTime );
	virtual void Init();

	virtual const SHPObjectRPGStats* GetStats() const { return 0; }

	virtual void Segment();
	virtual const NTimer::STime GetNextSegmentTime() const { return nextSegmTime; }

	// сдетонировать, если при наезде данного юнита мина взрывается; true - если сдетонировала
	virtual void TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit ) { }
	virtual void Die( const float fDamage ) { }
	virtual EStaticObjType GetObjectType() const { return ESOT_SMOKE_SCREEN; }

	virtual bool IsContainer() const { return false; }
	virtual const int GetNDefenders() const { return 0; }
	virtual class CSoldier* GetUnit( const int n ) const { return 0; }
	
	virtual bool CanUnitGoThrough( const EAIClasses &eClass ) const { return true; }

	virtual void LockTiles() { }
	virtual void UnlockTiles()  { }
	virtual void CreateLockedTilesInfo( std::list<SObjTileInfo> *pTiles ) { pTiles->clear(); }
	virtual void SetTransparencies();
	virtual void RemoveTransparencies();
	virtual void RestoreTransparenciesImmidiately ();

	virtual void GetCoveredTiles( std::list<SVector> *pTiles ) const;

	virtual void GetRPGStats( struct SAINotifyRPGStats *pStats ) {}
	virtual const CVec3& GetCenter() const { return vCenter; }
	virtual const CVec2 GetAttackCenter( const CVec2 &vPoint ) const { return CVec2(vCenter.x,vCenter.y); }
	virtual void GetBoundRect( SRect *pRect ) const { pRect->InitRect( CVec2(GetCenter().x,GetCenter().y), CVec2( 1.0f, 1.0f ), 0.0f, 0.0f ); }
	virtual bool IsPointInside( const CVec2 &point ) const { return false; }
	virtual const WORD GetDir() const { return 0; }

	virtual CObjectProfile* GetPassProfile() const { return 0; }
};


