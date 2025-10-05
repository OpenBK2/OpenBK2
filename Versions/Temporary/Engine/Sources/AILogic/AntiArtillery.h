
#pragma once

#include "LinkObject.h"

class CRevealCircle : public CLinkObject
{
	OBJECT_BASIC_METHODS( CRevealCircle );
	ZDATA_(CLinkObject)
	CCircle circle;
	public: ZEND int operator&( IBinSaver &f ) { f.Add(1,(CLinkObject*)this); f.Add(2,&circle); return 0; }
public:
	CRevealCircle() { }
	CRevealCircle( const CVec2 &center, const float fR ) : circle( center, fR ) { SetUniqueIdForObjects(); }
	CRevealCircle( const CCircle &_circle ) : circle( _circle ) { SetUniqueIdForObjects(); }

	virtual void GetRevealCircle( CCircle *pCircle ) const { *pCircle = circle; }
	
	virtual const bool IsVisible( const BYTE party ) const { return true; }
	virtual void GetTilesForVisibility( CTilesSet *pTiles ) const { pTiles->clear(); }
	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const { return false; }
};
class CAIUnit;

class CAntiArtillery : public CLinkObject
{
	OBJECT_BASIC_METHODS( CAntiArtillery );
	
	float fMaxRadius;
	int nParty;

	NTimer::STime lastScan;
	// время последнего услышанного выстрела и последнего посланного круга из этой артиллерии для каждой из сторон
	std::vector<NTimer::STime> lastShotTime;
	std::vector<NTimer::STime> lastRevealCircleTime;
	bool bIsAA;		// is the gun an Anti-Aircraft Gun?

	// расстояние до ближайшего врага ( считается только для врагов )
	std::vector<float> closestEnemyDist2;
	std::vector<CVec2> lastHeardPos;
	std::vector<BYTE> nHeardShots;
	std::vector<CVec2> lastRevealCenter;
	CPtr<CAIUnit> pOwner;
public: 
	int operator&( IBinSaver &f ) 
	{ 
		if ( !f.IsChecksum() )
		{
			f.Add(1,( CLinkObject*)this); f.Add(2,&fMaxRadius); f.Add(3,&nParty); f.Add(4,&lastScan); f.Add(5,&lastShotTime); f.Add(6,&lastRevealCircleTime); f.Add(7,&bIsAA); f.Add(8,&closestEnemyDist2); f.Add(9,&lastHeardPos); f.Add(10,&nHeardShots); f.Add(11,&lastRevealCenter); 
			f.Add( 12, &pOwner );
		}
		return 0; 
	}
	//
	void Scan( const CVec2 &center );
public:
	CAntiArtillery() { }
	explicit CAntiArtillery( class CAIUnit *pOwner );
	
	void SetParty( const int _nParty ) { nParty = _nParty; }

	void Init( const float fMaxRadius, const int nParty );
	void Fired( const float _fGunRadius, const CVec2 &center );

	// bOwnerVisible - видет ли owner игроком
	void Segment( bool bOwnerVisible );

	const CCircle GetRevealCircle( const int nParty ) const;
	const NTimer::STime GetLastHeardTime( const int nParty ) const;
	const CVec2 GetLastRevealCenter( const int nParty ) const { return lastRevealCenter[ nParty ]; }

	//
	virtual const bool IsVisible( const BYTE party ) const { return true; }
	virtual void GetTilesForVisibility( CTilesSet *pTiles ) const { pTiles->clear(); }
	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const { return false; }
	
	//
	friend struct SAntiArtillerySort;
	friend class CAntiArtilleryManager;
};


