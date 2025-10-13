#pragma once

#include "LinkObject.h"

#include <cstdint>

class CAIUnit;
class CCommonUnit;
class CUpdatableObj;
struct SKilledUnit;

class CDeadUnit : public CLinkObject
{
	OBJECT_BASIC_METHODS( CDeadUnit );
	ZDATA_(CLinkObject)
	CPtr<CCommonUnit> pDieObj;
	NTimer::STime dieTime;
	EActionNotify dieAction;
	int nFatality;
	int nDeathAnimType;
	bool bPutMud;

	SVector tileCenter;
	bool bVisibleWhenDie;
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CLinkObject*)this); f.Add(2,&pDieObj); f.Add(3,&dieTime); f.Add(4,&dieAction); f.Add(5,&nFatality); f.Add(6,&bPutMud); f.Add(7,&tileCenter); f.Add(8,&bVisibleWhenDie); f.Add(9,&nDeathAnimType); return 0; }

public:
	CDeadUnit() { }
	CDeadUnit( class CCommonUnit *_pDieObj, const NTimer::STime _dieTime, const EActionNotify _dieAction, bool bPutMud, const int _nDeathAnimType );
	CDeadUnit( class CCommonUnit *_pDieObj, const NTimer::STime _dieTime, const EActionNotify _dieAction, const int _nFatality, bool bPutMud, const int _nDeathAnimType );

	virtual void GetDyingInfo( struct SAINotifyAction *pDyingInfo, bool *pbVisibleWhenDie );

	virtual const bool IsVisible( const uint8_t cParty ) const;
	virtual void GetTilesForVisibility( CTilesSet *pTiles ) const;
	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const;

	virtual CUpdatableObj* GetDieObject() const;
};

class CGraveyard
{
	typedef det_map< int, std::pair<CObj<CAIUnit>, float> > UpdateObjSet;
	typedef det_map<int, std::list< CObj<CDeadUnit> > > CBridgeDeadSoldiers;
	typedef std::vector< CObj<CAIUnit> > CDissapeared;

	det_map<int,bool> diedVisible;	// for units that died visible and went under warfog, not used when counting checksum, store it's separately
	ZDATA
	ZONSERIALIZE
	std::list< CPtr<SKilledUnit> > killed;
	UpdateObjSet soonBeDead;

	ZSKIP //list<CObj<CAIUnit> > dissapeared;

	CBridgeDeadSoldiers bridgeDeadSoldiers;
	ZSKIP //hash_map<int,bool> diedVisible;
	CDissapeared dissapeared;
public:
	ZEND int operator&( IBinSaver &f ) { OnSerialize( f ); f.Add(2,&killed); f.Add(3,&soonBeDead); f.Add(5,&bridgeDeadSoldiers); f.Add(7,&dissapeared); return 0; }
private:
	void CheckSoonBeDead();
	void OnSerialize( IBinSaver &saver );
public:
	void Segment();

	bool IsDiedVisible( int nUniqieID );

	void AddKilledUnit( class CAIUnit *pUnit, const NTimer::STime &timeOfVisDeath, const int nFatality, const int nDeathAnimType );
	void AddToSoonBeDead( class CAIUnit *pUnit, const float fDamage );
	void AddToDissapeared( CAIUnit *pUnit );

	void DelKilledUnitsFromBridge( const SRect &bridgeRect );
	void AddBridgeKilledSoldier( const SVector &tile, CAIUnit *pSoldier );
	
	void Clear();
};


