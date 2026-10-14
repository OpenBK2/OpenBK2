#pragma once

#include "RotatingFireplacesObject.h"
#include "StaticObject.h"
#include "StormableObject.h"

#include <cstdint>

class CSoldier;
class CEntrenchmentPart;

//*******************************************************************
//*												  CEntrenchment														*
//*******************************************************************

class CEntrenchment : public CStaticObject,
											public ILoadableObject,
											public CStormableObject,
											public CRotatingFireplacesObject
{
	OBJECT_BASIC_METHODS( CEntrenchment );

	struct SFireplaceInfo
	{
		ZDATA
		CVec2 center;
		CPtr<CSoldier> pUnit;
		// номер сегмента, опис. данный fireplace в статах
		// Defaults cover construction before Init/load; explicit constructor values still take precedence.
		int nFrameIndex = {};

		ZEND int operator&( IBinSaver &f )
		{
			f.Add( 2, &center );
			f.Add( 3, &pUnit );
			f.Add( 4, &nFrameIndex );
			return 0;
		}

	public:
		SFireplaceInfo()
			: nFrameIndex( -1 )
		{
		}

		SFireplaceInfo( const CVec2 &_center, CSoldier *_pUnit, const int _nFrameIndex )
			: pUnit( _pUnit )
			, center( _center )
			, nFrameIndex( _nFrameIndex )
		{
		}
	};

	struct SInsiderInfo
	{
		ZDATA
		CPtr<CSoldier> pUnit;
		// -1, если в резерве
		int nFireplace = {};

		ZEND int operator&( IBinSaver &f )
		{
			f.Add( 2, &pUnit );
			f.Add( 3, &nFireplace );
			return 0;
		}

	public:
		SInsiderInfo() {}

		SInsiderInfo( CSoldier *_pUnit, const int _nFireplace )
			: pUnit( _pUnit )
			, nFireplace( _nFireplace )
		{
		}
	};

	typedef std::list<CPtr<CEntrenchmentPart>> CSegmentList;

	std::list<SInsiderInfo>::iterator iter;
	ZDATA_( CStaticObject )
	ZPARENT( CStormableObject )
	ZPARENT( CRotatingFireplacesObject )
	SRect rect;
	int z = {};

	int nBusyFireplaces = {};
	std::vector<SFireplaceInfo> fireplaces;

	std::list<SInsiderInfo> insiders;
	ZSKIP

	CDBPtr<SEntrenchmentRPGStats> pStats;

	NTimer::STime nextSegmTime = {};

	CSegmentList segments; // List of segments in this section
public:
	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CStaticObject *)this );
		f.Add( 2, (CStormableObject *)this );
		f.Add( 3, (CRotatingFireplacesObject *)this );
		f.Add( 4, &rect );
		f.Add( 5, &z );
		f.Add( 6, &nBusyFireplaces );
		f.Add( 7, &fireplaces );
		f.Add( 8, &insiders );
		f.Add( 10, &pStats );
		f.Add( 11, &nextSegmTime );
		f.Add( 12, &segments );
		return 0;
	}

	//
	static CVec2 GetShift( const CVec2 &vPoint, const CVec2 &vDir );
	void ProcessEmptyFireplace( const int nFireplace );

protected:
	void AddSoldier( CSoldier *pUnit ) override;
	void AddSoldierToFirePlace( CSoldier *pUnit, const int nFirePlace );
	void DelSoldier( CSoldier *pUnit, const bool bFillEmptyFireplace ) override;

	void SoldierDamaged( class CSoldier *pUnit ) override {}

public:
	CEntrenchment() {}

	CEntrenchment( CObjectBase **_segments,
	               const int nLen,
	               class CFullEntrenchment *pFullEntrenchment,
	               const bool bPiecewise = false );

	const SHPObjectRPGStats *GetStats() const override
	{
		return pStats;
	}

	const CVec3 &GetCenter() const override;

	const CVec2 GetAttackCenter( const CVec2 &vPoint ) const override
	{
		return rect.center;
	}

	void GetCoveredTiles( std::list<SVector> *pTiles ) const override;

	void GetBoundRect( SRect *pRect ) const override
	{
		*pRect = rect;
	}

	bool IsPointInside( const CVec2 &point ) const override
	{
		return rect.IsPointInside( point );
	}

	const uint16_t GetDir() const override
	{
		return GetDirectionByVector( rect.dir );
	}

	const float GetHitPoints() const override
	{
		return pStats->fMaxHP;
	}

	void
	TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit ) override;

	void Segment() override;
	const NTimer::STime GetNextSegmentTime() const override;

	EStaticObjType GetObjectType() const override
	{
		return ESOT_ENTRENCHMENT;
	}

	// итерирование по fire slots
	void StartIterate() override
	{
		iter = insiders.begin();
	}

	void Iterate() override;

	bool IsIterateFinished() override
	{
		return iter == insiders.end();
	}
	class CAIUnit *GetIteratedUnit() override;

	bool IsContainer() const override
	{
		return true;
	}

	const int GetNDefenders() const override;
	class CSoldier *GetUnit( const int n ) const override;
	const uint8_t GetPlayer() const override;

	void Delete();

	// возвращает в pvResult точку в окопе, ближаюшую к vPoint
	void GetClosestPoint( const CVec2 &vPoint, CVec2 *pvResult ) const;

	const bool IsVisibleForDiplomacyUpdate() override
	{
		return IsAnyInsiderVisible();
	}

	bool CanUnitGoThrough( const EAIClasses &eClass ) const override
	{
		return true;
	}

	// можно ли менять слот у этого слодата
	bool CanRotateSoldier( class CSoldier *pSoldier ) const override;
	// поставить солдата в place вместо сидящего там
	void ExchangeUnitToFireplace( class CSoldier *pSoldier, int nFirePlace ) override;
	// количество fireplaces
	const int GetNFirePlaces() const override;
	// солдат, сидящий в fireplace, если fireplace пуст, то возвращает 0
	class CSoldier *GetSoldierInFireplace( const int nFireplace ) const override;
	//
	const CVec2 GetFirePlaceCoord( const int nFirePlace );

	void SetVisible();

	const NDb::SUnitStatsModifier *GetUnitBonus() const
	{
		return pStats->pInnerUnitBonus;
	}
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

	bool bVisible = {};
	bool bOwnerChanged = {}; // client-dependent, for sending creation update
	bool bDigBySegment = {};

	std::list<SVector> coveredTiles;
	CObj<CFullEntrenchment> pFullEntrenchment;

	NTimer::STime nextSegmTime = {};
	bool bSuspendedAppear = {}; // client-dependent
	bool bPlayerCreates = {};

	int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CExistingObject *)this );
		f.Add( 2, &pStats );
		f.Add( 3, &pOwner );
		f.Add( 4, &center );
		f.Add( 5, &dir );
		f.Add( 6, &boundRect );
		if ( !f.IsChecksum() )
		{
			f.Add( 7, &bVisible );
			f.Add( 8, &bOwnerChanged );
			f.Add( 9, &bDigBySegment );
		}
		f.Add( 10, &coveredTiles );
		f.Add( 11, &pFullEntrenchment );
		f.Add( 12, &nextSegmTime );
		if ( !f.IsChecksum() )
		{
			f.Add( 13, &bSuspendedAppear );
		}
		f.Add( 14, &bPlayerCreates );
		return 0;
	}

	//
	// виден всеми сторонами
	bool CanUnregister() const;
	//
	static CVec2 GetShift( const CVec2 &vPoint, const CVec2 &vDir );

protected:
	void SetNewPlaceWithoutMapUpdate( const CVec3 &_center, const uint16_t _dir = 0 ) override;

public:
	CEntrenchmentPart() {}

	void Init();
	// nFrameIndex - индекс в векторе SEntrenchmentRPGStats::segments
	CEntrenchmentPart( const SEntrenchmentRPGStats *pStats,
	                   const CVec3 &center,
	                   const uint16_t dir,
	                   const int nFrameIndex,
	                   float fHP,
	                   int nPlayer,
	                   bool bPlayerCreates );
	static SRect CalcBoundRect( const CVec2 &center,
	                            const uint16_t _dir,
	                            const SEntrenchmentRPGStats::SEntrenchSegmentRPGStats &stats );

	const SEntrenchmentRPGStats::SEntrenchSegmentRPGStats &GetSegmStats() const
	{
		return pStats->segments[GetFrameIndex()];
	}

	const EEntrenchSegmType GetType() const
	{
		return pStats->segments[GetFrameIndex()].eType;
	}

	CEntrenchment *GetOwner() const
	{
		return pOwner;
	}

	void SetOwner( CEntrenchment *_pOwner, bool _bDigBySegment = false )
	{
		pOwner = _pOwner;
		bOwnerChanged = true;
		bDigBySegment = _bDigBySegment;
	}

	void GetPlacement( SAINotifyPlacement *pPlacement, const NTimer::STime timeDiff ) override;

	const CVec3 &GetCenter() const override
	{
		return center;
	}

	const CVec2 GetAttackCenter( const CVec2 &vPoint ) const override
	{
		return boundRect.center;
	}

	const uint16_t GetDir() const override
	{
		return dir;
	}

	void GetRPGStats( struct SAINotifyRPGStats *pStats ) override;

	void GetBoundRect( SRect *pRect ) const override
	{
		*pRect = boundRect;
	}

	bool IsPointInside( const CVec2 &point ) const override;
	void GetCoveredTiles( std::list<SVector> *pTiles ) const override;

	void Segment() override;
	const NTimer::STime GetNextSegmentTime() const override;

	void LockTiles() override {}

	void UnlockTiles() override {}

	void CreateLockedTilesInfo( std::list<SObjTileInfo> *pTiles ) override
	{
		pTiles->clear();
	}

	void SetTransparencies() override {}

	void RemoveTransparencies() override {}

	void RestoreTransparenciesImmidiately() override {}

	const SHPObjectRPGStats *GetStats() const override
	{
		return pStats;
	}

	void
	TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit ) override;

	// бессмертен
	void Die( const float fDamage ) override {}

	EStaticObjType GetObjectType() const override
	{
		return ESOT_ENTR_PART;
	}

	bool IsContainer() const override
	{
		return true;
	}

	const int GetNDefenders() const override;

	class CSoldier *GetUnit( const int n ) const override
	{
		return pOwner->GetUnit( n );
	}

	void SetVisible( const bool bLastSegment = false );

	void SetFullEntrench( class CFullEntrenchment *_pFullEntrenchment )
	{
		pFullEntrenchment = _pFullEntrenchment;
	}

	const class CFullEntrenchment *GetFullEntrench() const
	{
		return pFullEntrenchment;
	}

	bool CanUnitGoThrough( const EAIClasses &eClass ) const override
	{
		return true;
	}

	CObjectProfile *GetPassProfile() const override
	{
		return 0;
	}

	bool ShouldSuspendAction( const EActionNotify &eAction ) const override
	{
		return ( CExistingObject::ShouldSuspendAction( eAction ) || eAction == ACTION_NOTIFY_DISSAPEAR_OBJ ||
		         eAction == ACTION_NOTIFY_NEW_ST_OBJ && bSuspendedAppear );
	}
};

//*******************************************************************
//*												 CFullEntrenchment												*
//*******************************************************************

class CFullEntrenchment : public CAIObjectBase
{
	OBJECT_BASIC_METHODS( CFullEntrenchment );

	typedef std::list<CPtr<CEntrenchment>> CSectionList;

	ZDATA
	ZSKIP
	CSectionList sections;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 3, &sections );
		return 0;
	}

public:
	CFullEntrenchment() {}

	void AddEntrenchmentSection( class CEntrenchment *pSection );

	void SetVisible();
};

// танковый окоп
class CEntrenchmentTankPit : public CGivenPassabilityStObject
{
	OBJECT_NOCOPY_METHODS( CEntrenchmentTankPit );
	ZDATA_( CGivenPassabilityStObject )
	CPtr<CAIUnit> pOwner;

	CDBPtr<SMechUnitRPGStats> pStats;
	SAIAngle wDir;

	CVec2 vHalfSize;
	SRect boundRect;
	std::list<SObjTileInfo> tilesToLock;

	ZEND int operator&( IBinSaver &f ) override
	{
		f.Add( 1, (CGivenPassabilityStObject *)this );
		f.Add( 2, &pOwner );
		f.Add( 3, &pStats );
		f.Add( 4, &wDir );
		f.Add( 5, &vHalfSize );
		f.Add( 6, &boundRect );
		f.Add( 7, &tilesToLock );
		return 0;
	}

protected:
	void SetNewPlaceWithoutMapUpdate( const CVec3 &_center, const uint16_t _dir = 0 ) override {}

public:
	CEntrenchmentTankPit() {}

	~CEntrenchmentTankPit() override;
	// nFrameIndex - индекс в векторе SEntrenchmentRPGStats::segments
	CEntrenchmentTankPit( const SMechUnitRPGStats *pStats,
	                      const CVec3 &center,
	                      const uint16_t dir,
	                      const int nFrameIndex,
	                      const class CVec2 &vResizeFactor,
	                      const std::list<SObjTileInfo> &tiles,
	                      class CAIUnit *_pOwner );

	const uint16_t GetDir() const override
	{
		return wDir;
	}

	void GetRPGStats( struct SAINotifyRPGStats *pStats ) override {}

	void GetBoundRect( SRect *pRect ) const override
	{
		*pRect = boundRect;
	}

	bool IsPointInside( const CVec2 &point ) const override
	{
		return false;
	}

	void GetCoveredTiles( std::list<SVector> *pTiles ) const override;
	void GetTilesForVisibility( CTilesSet *pTiles ) const override;

	void Segment() override {}

	void LockTiles() override;
	void UnlockTiles() override;
	void CreateLockedTilesInfo( std::list<SObjTileInfo> *pTiles ) override;

	void SetTransparencies() override {}

	void RemoveTransparencies() override {}

	const SHPObjectRPGStats *GetStats() const override
	{
		return pStats;
	}

	void
	TakeDamage( const float fDamage, const bool bFromExplosion, const int nPlayerOfShoot, CAIUnit *pShotUnit ) override;
	void Die( const float fDamage ) override;

	EStaticObjType GetObjectType() const override
	{
		return ESOT_TANKPIT;
	}

	void GetNewUnitInfo( struct SNewUnitInfo *pNewUnitInfo ) override;

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
		return false;
	}

	bool ShouldSuspendAction( const EActionNotify &eAction ) const override;
};
