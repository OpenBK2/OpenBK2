#pragma once

#include "StaticObject.h"
#include "StormableObject.h"
#include "RotatingFireplacesObject.h"

class CSoldier;
class CEntrenchmentPart;

//*******************************************************************
//*												  CEntrenchment														*
//*******************************************************************

class CEntrenchment : public CStaticObject, public ILoadableObject, public CStormableObject, public CRotatingFireplacesObject
{
	OBJECT_BASIC_METHODS( CEntrenchment );

	struct SFireplaceInfo
	{
		ZDATA
		CVec2 center;
		CPtr<CSoldier> pUnit;
		// номер сегмента, опис. данный fireplace в статах
		int nFrameIndex;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&center); f.Add(3,&pUnit); f.Add(4,&nFrameIndex); return 0; }
	public:
		SFireplaceInfo() : nFrameIndex( -1 ) { }
		SFireplaceInfo( const CVec2 &_center, CSoldier *_pUnit, const int _nFrameIndex ) : pUnit( _pUnit ), center( _center ), nFrameIndex( _nFrameIndex ) { }
	};

	struct SInsiderInfo
	{
		ZDATA
		CPtr<CSoldier> pUnit;
		// -1, если в резерве
		int nFireplace;
		ZEND int operator&( IBinSaver &f ) { f.Add(2,&pUnit); f.Add(3,&nFireplace); return 0; }
	public:
		SInsiderInfo() { }
		SInsiderInfo( CSoldier *_pUnit, const int _nFireplace ) : pUnit( _pUnit ), nFireplace( _nFireplace ) { }
	};

	typedef list<CPtr<CEntrenchmentPart> > CSegmentList;

	list<SInsiderInfo>::iterator iter;
	ZDATA_(CStaticObject)
		ZPARENT(CStormableObject)
		ZPARENT(CRotatingFireplacesObject)
	SRect rect;
	int z;

	int nBusyFireplaces;
	vector<SFireplaceInfo> fireplaces;	
	
	list<SInsiderInfo> insiders;
	ZSKIP

	CDBPtr<SEntrenchmentRPGStats> pStats;

	NTimer::STime nextSegmTime;

	CSegmentList segments;			// List of segments in this section
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CStaticObject*)this); f.Add(2,(CStormableObject*)this); f.Add(3,(CRotatingFireplacesObject*)this); f.Add(4,&rect); f.Add(5,&z); f.Add(6,&nBusyFireplaces); f.Add(7,&fireplaces); f.Add(8,&insiders); f.Add(10,&pStats); f.Add(11,&nextSegmTime); f.Add(12,&segments); return 0; }
	//
	static CVec2 GetShift( const CVec2 &vPoint, const CVec2 &vDir );
	void ProcessEmptyFireplace( const int nFireplace );
protected:
	virtual void AddSoldier( CSoldier *pUnit );
	void AddSoldierToFirePlace( CSoldier *pUnit, const int nFirePlace );
	virtual void DelSoldier( CSoldier *pUnit, const bool bFillEmptyFireplace );
	virtual void SoldierDamaged( class CSoldier *pUnit ) { }
public:
	CEntrenchment() { }
	CEntrenchment( CObjectBase** _segments, const int nLen, class CFullEntrenchment *pFullEntrenchment, const bool bPiecewise = false );

	const SHPObjectRPGStats* GetStats() const { return pStats; }

	virtual const CVec3& GetCenter() const;
	virtual const CVec2 GetAttackCenter( const CVec2 &vPoint ) const { return rect.center; }
	virtual void GetCoveredTiles( list<SVector> *pTiles ) const;
	virtual void GetBoundRect( SRect *pRect ) const { *pRect = rect; }
	virtual bool IsPointInside( const CVec2 &point ) const { return rect.IsPointInside( point ); }
	virtual const WORD GetDir() const { return GetDirectionByVector( rect.dir ); }

	virtual const float GetHitPoints() const { return pStats->fMaxHP; }
	virtual void TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit );

	virtual void Segment();
	virtual const NTimer::STime GetNextSegmentTime() const;
	
	virtual EStaticObjType GetObjectType() const { return ESOT_ENTRENCHMENT; }
	
	// итерирование по fire slots
	virtual void StartIterate() { iter = insiders.begin(); }
	virtual void Iterate();
	virtual bool IsIterateFinished() { return iter == insiders.end(); }
	virtual class CAIUnit* GetIteratedUnit();

	virtual bool IsContainer() const { return true; }
	virtual const int GetNDefenders() const;
	virtual class CSoldier* GetUnit( const int n ) const;
	virtual const BYTE GetPlayer() const;

	void Delete();
	
	// возвращает в pvResult точку в окопе, ближаюшую к vPoint 
	void GetClosestPoint( const CVec2 &vPoint, CVec2 *pvResult ) const;
	virtual const bool IsVisibleForDiplomacyUpdate() { return IsAnyInsiderVisible(); }
	
	virtual bool CanUnitGoThrough( const EAIClasses &eClass ) const { return true; }
	
	// можно ли менять слот у этого слодата
	virtual bool CanRotateSoldier( class CSoldier *pSoldier ) const;
	// поставить солдата в place вместо сидящего там
	virtual void ExchangeUnitToFireplace( class CSoldier *pSoldier, int nFirePlace );
	// количество fireplaces
	const int GetNFirePlaces() const;
	// солдат, сидящий в fireplace, если fireplace пуст, то возвращает 0
	class CSoldier* GetSoldierInFireplace( const int nFireplace) const;
	//
	const CVec2 GetFirePlaceCoord( const int nFirePlace );

	void SetVisible();
	const NDb::SUnitStatsModifier *GetUnitBonus() const { return pStats->pInnerUnitBonus; }
};

//*******************************************************************
//*											CEntrenchmentPart														*
//*******************************************************************

class CFullEntrenchment;

class CEntrenchmentPart : public CExistingObject
{
	OBJECT_BASIC_METHODS( CEntrenchmentPart );
	CDBPtr<SEntrenchmentRPGStats> pStats;
	CPtr<CEntrenchment> pOwner; // окоп, которому он принадлежит

	CVec3 center;
	SAIAngle dir;

	SRect boundRect;

	bool bVisible;
	bool bOwnerChanged;			// client-dependent, for sending creation update
	bool bDigBySegment;

	list<SVector> coveredTiles;	
	CObj<CFullEntrenchment> pFullEntrenchment;

	NTimer::STime nextSegmTime;
	bool bSuspendedAppear;	// client-dependent
	bool bPlayerCreates;
	int operator&( IBinSaver &f ) 
	{ 
		f.Add(1,( CExistingObject*)this); 
		f.Add(2,&pStats); f.Add(3,&pOwner); f.Add(4,&center); f.Add(5,&dir);
		f.Add(6,&boundRect); 
		if ( !f.IsChecksum() )
		{
			f.Add(7,&bVisible);
			f.Add(8,&bOwnerChanged);
			f.Add(9,&bDigBySegment);
		}
		f.Add(10,&coveredTiles); f.Add(11,&pFullEntrenchment); f.Add(12,&nextSegmTime); 
		if ( !f.IsChecksum() )
			f.Add( 13, &bSuspendedAppear ); 
		f.Add( 14, &bPlayerCreates );
		return 0; 
	}
	//
	// виден всеми сторонами
	bool CanUnregister() const;
	//
	static CVec2 GetShift( const CVec2 &vPoint, const CVec2 &vDir );
protected:
	virtual void SetNewPlaceWithoutMapUpdate( const CVec3 &_center, const WORD _dir = 0 );
public:
	CEntrenchmentPart() { }
	void Init();
	// nFrameIndex - индекс в векторе SEntrenchmentRPGStats::segments
	CEntrenchmentPart( const SEntrenchmentRPGStats *pStats, const CVec3& center, const WORD dir, const int nFrameIndex, float fHP, int nPlayer, bool bPlayerCreates );
	static SRect CalcBoundRect( const CVec2 & center, const WORD _dir, const SEntrenchmentRPGStats::SEntrenchSegmentRPGStats& stats);

	const SEntrenchmentRPGStats::SEntrenchSegmentRPGStats& GetSegmStats() const { return pStats->segments[GetFrameIndex()]; }
	const EEntrenchSegmType GetType() const { return pStats->segments[GetFrameIndex()].eType; }
	CEntrenchment *GetOwner() const { return pOwner; }
	void SetOwner( CEntrenchment *_pOwner, bool _bDigBySegment = false ) 
	{ 
		pOwner = _pOwner; bOwnerChanged = true; bDigBySegment = _bDigBySegment; 
	}

	virtual void GetPlacement( SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff );
	virtual const CVec3& GetCenter() const { return center; }
	virtual const CVec2 GetAttackCenter( const CVec2 &vPoint ) const { return boundRect.center; }
	virtual const WORD GetDir() const { return dir; }

	virtual void GetRPGStats( struct SAINotifyRPGStats *pStats );

	void GetBoundRect( SRect *pRect ) const { *pRect = boundRect; }
	virtual bool IsPointInside( const CVec2 &point ) const;
	virtual void GetCoveredTiles( list<SVector> *pTiles ) const;

	virtual void Segment();
	virtual const NTimer::STime GetNextSegmentTime() const;

	virtual void LockTiles() { }
	virtual void UnlockTiles()  { }
	virtual void CreateLockedTilesInfo( list<SObjTileInfo> *pTiles ) { pTiles->clear(); }
	virtual void SetTransparencies() { }
	virtual void RemoveTransparencies() { }
	virtual void RestoreTransparenciesImmidiately() { }

	virtual const SHPObjectRPGStats* GetStats() const { return pStats; }

	virtual void TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit );
	// бессмертен
	virtual void Die( const float fDamage ) { }
	
	virtual EStaticObjType GetObjectType() const { return ESOT_ENTR_PART; }

	virtual bool IsContainer() const { return true; }
	virtual const int GetNDefenders() const ;
	virtual class CSoldier* GetUnit( const int n ) const { return pOwner->GetUnit( n ); }

	void SetVisible( const bool bLastSegment = false );
	void SetFullEntrench( class CFullEntrenchment *_pFullEntrenchment ) { pFullEntrenchment = _pFullEntrenchment; }
	const class CFullEntrenchment *GetFullEntrench() const { return pFullEntrenchment; } 

	virtual bool CanUnitGoThrough( const EAIClasses &eClass ) const { return true; }
	virtual CObjectProfile* GetPassProfile() const { return 0; }
	bool ShouldSuspendAction( const EActionNotify &eAction ) const
	{
		return
			( CExistingObject::ShouldSuspendAction( eAction ) || 
			eAction == ACTION_NOTIFY_DISSAPEAR_OBJ ||
			eAction == ACTION_NOTIFY_NEW_ST_OBJ && bSuspendedAppear );
	}

};

//*******************************************************************
//*												 CFullEntrenchment												*
//*******************************************************************

class CFullEntrenchment : public CAIObjectBase
{
	OBJECT_BASIC_METHODS( CFullEntrenchment );

	typedef list<CPtr<CEntrenchment> > CSectionList;

	ZDATA
	ZSKIP
	CSectionList sections;
	ZEND int operator&( IBinSaver &f ) { f.Add(3,&sections); return 0; }

public:
	CFullEntrenchment() { }
	
	void AddEntrenchmentSection( class CEntrenchment *pSection );

	void SetVisible();
};

// танковый окоп
class CEntrenchmentTankPit : public CGivenPassabilityStObject
{
	OBJECT_NOCOPY_METHODS( CEntrenchmentTankPit );
	ZDATA_(CGivenPassabilityStObject)
	CPtr<CAIUnit> pOwner;

	CDBPtr<SMechUnitRPGStats> pStats;
	SAIAngle wDir;
	
	CVec2 vHalfSize; 
	SRect boundRect;
	list<SObjTileInfo> tilesToLock;

	ZEND int operator&( IBinSaver &f ) { f.Add(1,(CGivenPassabilityStObject*)this); f.Add(2,&pOwner); f.Add(3,&pStats); f.Add(4,&wDir); f.Add(5,&vHalfSize); f.Add(6,&boundRect); f.Add(7,&tilesToLock); return 0; }
protected:
	virtual void SetNewPlaceWithoutMapUpdate( const CVec3 &_center, const WORD _dir = 0 ) { }
public:
	CEntrenchmentTankPit() { }
	~CEntrenchmentTankPit();
	// nFrameIndex - индекс в векторе SEntrenchmentRPGStats::segments
	CEntrenchmentTankPit( const SMechUnitRPGStats *pStats, const CVec3 &center, const WORD dir,const int nFrameIndex, const class CVec2 &vResizeFactor, const list<SObjTileInfo> &tiles, class CAIUnit *_pOwner );

	virtual const WORD GetDir() const { return wDir; }

	virtual void GetRPGStats( struct SAINotifyRPGStats *pStats ) { }
	void GetBoundRect( SRect *pRect ) const { *pRect = boundRect; }
	virtual bool IsPointInside( const CVec2 &point ) const { return false; }
	virtual void GetCoveredTiles( list<SVector> *pTiles ) const ;
	virtual void GetTilesForVisibility( CTilesSet *pTiles ) const;
	virtual void Segment() { }

	virtual void LockTiles();
	virtual void UnlockTiles();
	virtual void CreateLockedTilesInfo( list<SObjTileInfo> *pTiles );
	virtual void SetTransparencies() { }
	virtual void RemoveTransparencies() { }
	virtual const SHPObjectRPGStats* GetStats() const { return pStats; }

	virtual void TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit );
	virtual void Die( const float fDamage );

	virtual EStaticObjType GetObjectType() const { return ESOT_TANKPIT; }
	
	virtual void GetNewUnitInfo( struct SNewUnitInfo *pNewUnitInfo );

	virtual bool IsContainer() const { return false; }
	virtual const int GetNDefenders() const { return 0; }
	virtual class CSoldier* GetUnit( const int n ) const { return 0; }

	virtual bool CanUnitGoThrough( const EAIClasses &eClass ) const { return false; }
	virtual bool ShouldSuspendAction( const EActionNotify &eAction ) const;
};


