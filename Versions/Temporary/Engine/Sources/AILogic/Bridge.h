#ifndef __BRIDGE_H__
#define __BRIDGE_H__

#pragma ONCE

#include "StaticObject.h"

class CFullBridge;
// remember in what order bridges applied heigths and ensures heights removal in reverse order.
class CBridgeHeightRemover
{
	typedef list<int/*heightID*/> CHeightsOrder;
	ZDATA
	CHeightsOrder heightsOrder;
	hash_map<int/*heightID*/, bool> heightToRemove;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&heightsOrder); f.Add(3,&heightToRemove); return 0; }
	void RegisterOrder( const int nHeightID );
	void RemoveHeight( const int nHeightID );
	void Segment();
	void Clear();
};

class CBridgeSpan : public CGivenPassabilityStObject
{
	OBJECT_BASIC_METHODS( CBridgeSpan );

	ZDATA_(CGivenPassabilityStObject)
	CDBPtr<SBridgeRPGStats> pStats;

	ZSKIP//CArray2D<BYTE> unlockTypes;	// разлоканные типы террэйна, 0 - если нечего было разлокивать
	CObj<CFullBridge> pFullBridge;
	bool bNewBuilt;												// этот мост построили во время тгры
	bool bLocked;													// залочены ли тайл

	list<SObjTileInfo> oldTilesInfo;      // информация о залоканых тайлах, где теперь стоит мост
	int nOldHeightsID;										// информация о высотах, где теперь стоит мост

	// умирает данный сегмент, начинает удалять все вокруг.
	bool bDeletingAround;

	int nScriptID;

	//DEBUG{
	vector<CPtr<CObjectBase> > segments;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CGivenPassabilityStObject*)this); f.Add(2,&pStats); f.Add(4,&pFullBridge); f.Add(5,&bNewBuilt); f.Add(6,&bLocked); f.Add(7,&oldTilesInfo); f.Add(8,&nOldHeightsID); f.Add(9,&bDeletingAround); f.Add(10,&nScriptID); f.Add(11,&segments); return 0; }
#ifndef _FINALRELEASE
	int nTilesMarkerID;

	void DisplayBridgeTiles();
#endif
	//DEBUG}

	void GetTilesForVisibilityInternal( CTilesSet *pTiles ) const;
	void GetVisibility( CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > *visibity ) const;
	void GetPassability( CSmoothRotatedArray2D<BYTE, const SHPObjectRPGStats::SByteArray2 > *passability ) const;
protected:
	virtual int GetHeight() const { return 0; }
public:
	CBridgeSpan() : nScriptID( -1 ) { }
	CBridgeSpan( const SBridgeRPGStats *pStats, const CVec3 &center, const float fHP, const WORD _nDir, const int nFrameIndex );
	
	void Build();													// построить сегмент моста, залокать как положено, послать в мир.

	const SBridgeRPGStats * GetBridgeStats() const { return pStats; }
	virtual const SHPObjectRPGStats* GetStats() const { return pStats; }

	virtual void LockTiles(); // here we just copy lock info to internal array
	void RealLockTiles(); // here we real lock tiles
	virtual void UnlockTiles();
	virtual void CreateLockedTilesInfo( list<SObjTileInfo> *pTiles );

	virtual void TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit );
	virtual bool IsPointInside( const CVec2 &point ) const;
	virtual void Die( const float fDamage );
	virtual void SetHitPoints( const float fNewHP );
	
	virtual void Segment() { }

	virtual EStaticObjType GetObjectType() const { return ESOT_BRIDGE_SPAN; }
	
	virtual bool IsContainer() const { return false; }
	virtual const int GetNDefenders() const { return 0; }
	virtual class CSoldier* GetUnit( const int n ) const { return 0; }

	void SetFullBrige( CFullBridge *pFullBridge );
	CFullBridge * GetFullBridge() { return pFullBridge; }
	
	virtual bool CanUnitGoThrough( const EAIClasses &eClass ) const { return true; }
	virtual void GetTilesForVisibility( CTilesSet *pTiles ) const;
	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const;
	virtual void GetCoveredTiles( list<SVector> *pTiles ) const;
	virtual void SetTransparencies();
	virtual void RemoveTransparencies();

	virtual void SetScriptID( const int _nScriptID ) { nScriptID = _nScriptID; }
	void GetPlacement( SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff );
	void SetHeights();
	void RemoveHeights();
	friend class CFullBridge;
};

class CFullBridge : public CLinkObject
{
	OBJECT_BASIC_METHODS( CFullBridge );
	public: int operator&( IBinSaver &saver ); private:
	
	list<CBridgeSpan*> spans;					// построенные части моста
	list<CBridgeSpan*> projectedSpans;	// части моста, которые находятся в проекте

	bool bGivingDamage;
public:
	struct SSpanLock : public CAIObjectBase
	{
		OBJECT_BASIC_METHODS( SSpanLock );
public: int operator&( IBinSaver &saver ); private:
		list<SVector> tiles;
		list<EAIClasses> formerTiles;
		CBridgeSpan * pSpan;
	public:
		//
		SSpanLock(): pSpan( 0 ) {  }
		SSpanLock( CBridgeSpan * pSpan, const WORD wDir );
		void Unlock();
		const CBridgeSpan * GetSpan() const { return pSpan; }
	};
private:
	typedef list< CPtr<SSpanLock> > LockedSpans;
	LockedSpans lockedSpans;
	int nSpans;														//full number of bridge spans
	bool bLockingBridge;

public:
	CFullBridge()	: bGivingDamage( false ), nSpans( 0 ), bLockingBridge( true ) { SetUniqueIdForObjects(); }

	const float GetHPPercent() const;

	// when span was built
	void SpanBuilt( CBridgeSpan * pSpan );

	void AddSpan( CBridgeSpan *pSpan );
	void DamageTaken( CBridgeSpan *pDamagedSpan, const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit );

	void EnumSpans( vector< CObj<CBridgeSpan> > *pSpans );
	virtual void GetTilesForVisibility( CTilesSet *pTiles ) const;
	const bool IsVisible( const BYTE cParty ) const;

	void LockSpan( CBridgeSpan * pSpan, const WORD wDir );
	void UnlockSpan( CBridgeSpan * pSpan );
	void UnlockAllSpans();

	bool CanTakeDamage() const;
	const int GetNSpans() const;
	void InitEntireBridge();

	void FinishRepair() { bLockingBridge = true; }
	void NeedRepair() { bLockingBridge = false; }

	friend class CBridgeCreation;
};

#endif // __BRIDGE_H__

