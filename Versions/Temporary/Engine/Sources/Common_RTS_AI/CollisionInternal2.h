#pragma once

#include "BasePathUnit.h"
#include "Collision.h"
#include "../Misc/nqueue.h"

struct IPath;

class CBasePathUnitHolder : public CAIObjectBase
{
	OBJECT_NOCOPY_METHODS( CBasePathUnitHolder )
	CBasePathUnit *pUnit;
public:
	CBasePathUnitHolder() : pUnit( 0 ) {};
	CBasePathUnitHolder( CBasePathUnit *_pUnit ) : pUnit( _pUnit ) {}

	CBasePathUnit *operator->() const { return pUnit; }
	CBasePathUnit *GetUnit() const { return pUnit; }

	const int GetUniqueID() const { return pUnit ? pUnit->GetUniqueID() : -1; }
	int operator&( IBinSaver &f ) { SerializeBasePathUnit( f, 1, &pUnit ); return 0; }
};

class CCollisionsCollector : public ICollisionsCollector
{
	struct SPusherInfo
	{
		ZDATA
			CPtr<CBasePathUnitHolder> pUnit;
			float fDistance;
			NCollision::ECollideType eCollideType;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&fDistance); f.Add(4,&eCollideType); return 0; }
		SPusherInfo() : pUnit( 0 ), fDistance( 0.0f ), eCollideType( NCollision::ECT_NONE ) {}
		SPusherInfo( CBasePathUnit *_pUnit, const float _fDistance, const NCollision::ECollideType _eCollideType )
			: fDistance( _fDistance ), eCollideType( _eCollideType ) { pUnit = new CBasePathUnitHolder( _pUnit ); }
	};
	struct SPushersSort
	{
		const bool operator()( const SPusherInfo &pusher1, const SPusherInfo &pusher2 ) const;
	};
	struct SCollision
	{
		ZDATA
			CPtr<CBasePathUnitHolder> pUnit;
			vector<SPusherInfo> pushers;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&pushers); return 0; }
		SCollision() : pUnit( 0 ) {}
		SCollision( CBasePathUnit *_pUnit ) { pUnit = new CBasePathUnitHolder( _pUnit ); }
		SCollision( CBasePathUnit *pUnit1, CBasePathUnit *pUnit2, const float fDistance, const NCollision::ECollideType eCollideType ) { pUnit = new CBasePathUnitHolder( pUnit1 ); pushers.push_back( SPusherInfo( pUnit2, fDistance, eCollideType ) ); }
		void AddPusher( CBasePathUnit *_pUnit, const float fDistance, const NCollision::ECollideType eCollideType ) { pushers.push_back( SPusherInfo( _pUnit, fDistance, eCollideType ) ); }
	};
	typedef hash_map<int, SCollision> TCollisions;
	OBJECT_NOCOPY_METHODS( CCollisionsCollector )
	ZDATA
		TCollisions collisions;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&collisions); return 0; }
private:
	const bool PassOneAnother( CBasePathUnit *pUnit1, CBasePathUnit *pUnit2, const int nTileSize ) const;
	const bool NotifyAboutClosestThreat( CBasePathUnit *pUnit, CBasePathUnit *pPusher, const float fDistance ) const;
	const bool FindWayForInfantry(  CBasePathUnit *pInfantry, CBasePathUnit *pPusher, CAIMap *pAIMap ) const;
public:
	void AddCollision( CBasePathUnit *pUnit, CBasePathUnit *pPusher, const float fDistance, const NCollision::ECollideType eCollideType );
	void HandOutCollisions( CAIMap *pAIMap );

	friend struct SSortCollisions;
};

class CCollisionBase : public ICollision
{
	CBasePathUnit *pUnit;
	CBasePathUnit *pPushUnit;
	ZDATA
		ZONSERIALIZE
		ZSKIP // for pUnit
		ZSKIP // for pPushUnit
		int nPriority; // legacy for old pathfinding, not using so far
		CPtr<IPath> pPath;
public:
	ZEND int operator&( IBinSaver &f ) { OnSerialize( f ); f.Add(4,&nPriority); f.Add(5,&pPath); return 0; }
private:
	void OnSerialize( IBinSaver &f );
protected:
	CBasePathUnit* GetUnit() const { return pUnit; }
	IPath* GetPath() const { return pPath; }
	void SetPath( IPath *_pPath ) { pPath = _pPath; }
	const bool CanFindCandidates() const;
public:
	CCollisionBase() : pUnit( 0 ), pPushUnit( 0 ), nPriority( -1 ), pPath( 0 ) {}

	virtual void Init( CBasePathUnit *pUnit, CBasePathUnit *pPushUnit, const int nPriority );

	const int GetPriority() const { return nPriority; }
	CBasePathUnit* GetPushUnit() const { return pPushUnit; }

	void FindCandidates( ICollisionsCollector *pCollisionCollector );
};

// no collision
class CFreeCollision : public CCollisionBase
{
	OBJECT_NOCOPY_METHODS( CFreeCollision )
public:
	void Segment( const NTimer::STime timeDiff ) {}
	const bool IsSolved() const;
	const NCollision::ECollisionName GetName() const { return NCollision::ECN_FREE; }
};

// pUnit waits when pPushUnit pass away
class CWaitingCollision : public CCollisionBase
{
	OBJECT_NOCOPY_METHODS( CWaitingCollision )
	ZDATA_( CCollisionBase )
		bool bIsSolved;
		NTimer::STime timeToWait;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CCollisionBase *)this); f.Add(2,&bIsSolved); f.Add(3,&timeToWait); return 0; }
	CWaitingCollision() : CCollisionBase(), bIsSolved( false ) {}
	void Init( CBasePathUnit *pUnit, CBasePathUnit *pPushUnit, const int nPriority, const float fDistance );
	void Segment( const NTimer::STime timeDiff );
	const bool IsSolved() const { return bIsSolved; }
	const NCollision::ECollisionName GetName() const { return NCollision::ECN_WAIT; }
};

// pUnit gives place for pPushUnit (pPushUnit waits for pUnit !!!)
class CGivingPlaceCollision : public CCollisionBase
{
	OBJECT_NOCOPY_METHODS( CGivingPlaceCollision )
public:
	void Init( CBasePathUnit *pUnit, CBasePathUnit *pPushUnit, const int nPriority, const CVec2 &vFinishPoint, const int nTileSize );
	void Segment( const NTimer::STime timeDiff ) {}
	const bool IsSolved() const;
	const NCollision::ECollisionName GetName() const { return NCollision::ECN_GIVE_PLACE; }
};

// pUnit blocks tiles under itself and waits when pPushUnit gives
class CStopCollision : public CCollisionBase
{
	OBJECT_NOCOPY_METHODS( CStopCollision )
	ZDATA_( CCollisionBase )
		NTimer::STime timeLeft;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,( CCollisionBase *)this); f.Add(2,&timeLeft); return 0; }
	CStopCollision() : CCollisionBase(), timeLeft( 0 ) {}
	void Init( CBasePathUnit *pUnit, CBasePathUnit *pPushUnit, const int nPriority );
	void Segment( const NTimer::STime timeDiff );
	const bool IsSolved() const { return timeLeft == 0; }
	const NCollision::ECollisionName GetName() const { return NCollision::ECN_STOP; }
};


