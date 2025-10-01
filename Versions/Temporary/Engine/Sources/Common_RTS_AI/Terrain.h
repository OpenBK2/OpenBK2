#pragma once

#include "Common_RTS_AI_export.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "AIClasses.h"
#include "../Misc/BitData.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum ELockMode
{
	ELM_STATIC = 0x00,
	ELM_ALL    = 0x01
};
enum ETerrainTypes
{
	ETT_EARTH_TERRAIN  = 0,
	ETT_WATER_TERRAIN  = 1,
	ETT_MARINE_TERRAIN = 2,
};	
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum EFreeTileInfo
{
	FREE_NONE    = 0x00,
	FREE_TERRAIN = 0x01,
	FREE_WATER   = 0x02,
	FREE_ANY     = FREE_TERRAIN | FREE_WATER,
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SObjTileInfo
{
	SVector tile;
	EAIClasses lockInfo;

	SObjTileInfo() : tile( 0, 0 ), lockInfo( EAC_ANY ) { }
	SObjTileInfo( const SVector &_tile ) : tile( _tile ), lockInfo( EAC_ANY ) { }
	SObjTileInfo( const SVector &_tile, const BYTE _lockInfo ) : tile( _tile ), lockInfo( (EAIClasses)_lockInfo ) { }
	SObjTileInfo( const SVector &_tile, const EAIClasses _lockInfo ) : tile( _tile ), lockInfo( _lockInfo ) { }
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CAIMap;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SUnitProfile
{
	bool bRect;
	SRect rect;
	CCircle circle;
	//
	SUnitProfile() : bRect( false ), circle( VNULL2, 0 ) { rect.InitRect( VNULL2, VNULL2, VNULL2, VNULL2 ); }
	SUnitProfile( const SRect &_rect ) : bRect( true ), rect( _rect ), circle( VNULL2, 0 ) { }
	SUnitProfile( const CCircle &_circle ) : bRect( false ), circle( _circle ), rect() { rect.InitRect( VNULL2, VNULL2, VNULL2, VNULL2 ); }
	SUnitProfile( const CVec2 &vCenter, const float fRadius ) : bRect( false ), circle( vCenter, fRadius ), rect() { rect.InitRect( VNULL2, VNULL2, VNULL2, VNULL2 ); }

	const bool IsIntersected( const SUnitProfile &profile ) const { return profile.bRect ? IsIntersected( profile.rect ) : IsIntersected( profile.circle ); }
	const bool IsIntersected( const SRect &_rect ) const {	return bRect ? _rect.IsIntersected( rect ) : _rect.IsIntersectCircle( circle );	}
	const bool IsIntersected( const CCircle &_circle ) const {	return bRect ? rect.IsIntersectCircle( _circle ) : circle.IsIntersected( _circle ); }

	const bool IsPointInside( const CVec2 &point ) const { return ( bRect ) ? rect.IsPointInside( point ) : ( fabs( circle.center - point ) <= circle.r ); }

	const CVec2 GetCenter() const {	return ( bRect ) ? rect.center : circle.center; }

	const float GetRadius() const {	return ( bRect ) ? fabs( GetHalfLength(), GetHalfWidth() ) : circle.r; }

	const float GetHalfLength() const { return ( bRect ) ? (rect.lengthAhead + rect.lengthBack )/2.0f: circle.r; }
	const float GetHalfWidth() const { return ( bRect ) ? rect.width : circle.r; }
	const float GetLengthAhead() const { return ( bRect ) ? rect.lengthAhead : circle.r; }
	const float GetLengthBack() const { return ( bRect ) ? rect.lengthBack : circle.r; }

	const void Compress( const float fFactor )
	{
		if ( bRect )
			rect.Compress( fFactor );
		else
			circle.r *= fFactor;
	}

	const bool IsCircle() const { return !bRect; }
	const bool IsRect() const { return bRect; }

	int operator&( IBinSaver &saver )
	{
		saver.Add( 1, &bRect );
		saver.Add( 2, &rect );
		saver.Add( 3, &circle );

		return 0;
	}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct STerrainLockInfo
{
	int nCount;
	int nUnitID;
	int bLock;
	SUnitProfile profile;

	STerrainLockInfo() : nCount( -1 ), nUnitID( -1 ), bLock( false ), profile() {}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SUnitTileInfo
{
	SUnitProfile profile;
	bool bWater;

	SUnitTileInfo() : bWater(), profile() {}
	SUnitTileInfo( const SRect _rect, const bool _bWater ) :  profile( _rect ), bWater( _bWater ) {}
	SUnitTileInfo( const SUnitProfile _profile, const bool _bWater ) :  profile( _profile ), bWater( _bWater ) {}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef hash_map<int, SUnitTileInfo> CUnitsRects;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class COMMON_RTS_AI_EXPORT CTerrain : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CTerrain );

	struct STmpLockInfo
	{ 
		SVector tile; 
		int nUnitsBuf;
		EAIClasses aiClass;
		
		STmpLockInfo() : tile( -1, -1 ), nUnitsBuf( -1 ), aiClass( EAC_FORCE_DWORD ) {}
		STmpLockInfo( const SVector &_tile, const int _nUnitsBuf, const EAIClasses _aiClass )
			: tile( _tile ), nUnitsBuf( _nUnitsBuf ), aiClass( _aiClass ) {}
	};

	struct STmpLockInfo2
	{ 
		SVector tile; 
		EAIClasses aiClass;

		STmpLockInfo2() : tile( 0, 0 ), aiClass( EAC_NONE ) {}
		STmpLockInfo2( const SVector &_tile, const EAIClasses _aiClass )
			: tile( _tile ), aiClass( _aiClass ) {}
	};
	typedef list<STmpLockInfo2> CTmpLockInfoBuf2;
	typedef list<STmpLockInfo> CTmpLockInfoBuf;

	vector< CArray2D<BYTE> > unitsBuf;  // юниты, для воды и для суши
	CUnitsRects unitsRects;
	CArray2D<BYTE> passTypes;
	vector<float> passabilities;
	// 0 - статич. объекты, 0xff - статич. и динамич. объекты
	ELockMode eMode;
	CArray2D<EAIClasses> buf;
	CArray2D4Bit terrainTypes;
	hash_map< int, pair< bool, CTmpLockInfoBuf > > tmpUnlockUnitsMap;
	// по типу terrain - его ai проходимость
	vector<DWORD> passClasses;
	// по номеру тайла terrain - его тип
	vector<BYTE> terrSubTypes;
	CArray2D1Bit digImpossible;		// невозможность строительства окопов на тайле
	CArray2D1Bit bridgeTiles;
	CArray2D<BYTE> soil;
	vector<BYTE> tileDigImpossible;
	vector<BYTE> soilParams;
	CArray2D<CArray2D4Bit> maxes;
	bool bInitMode;
	hash_map< int, CTmpLockInfoBuf2 > tmpLockUnitsMap;
	int nTmpLockUnitID;

	CPtr<CAIMap> pAIMap;
	vector<int> classIndices;

	CArray2D1Bit maskForSmooth;
	CArray2D<CArray2D4Bit> maxesSmooth;
	//
#ifndef _FINALRELEASE
	vector<STerrainLockInfo> debugLockInfo;
	int nDebugLockInfoCount;
	int nDebugLockInfoPos;
#endif
	//
	void LoadPassabilities( const struct STerrainDesc* pTerrainDesc );
	//void LoadRivers( const struct STerrainInfo &terrainInfo );
	//void Load3DRoads( const struct STerrainInfo &terrainInfo );
	void InitExplosionTerrainTypes();

	// залокано только для индеска этого класса, без учёта AI_CLASS_ANY
	// если проверяется для AI_CLASS_ANY, то учитывается и локание юнитов
	bool IsLockedByUnits( const int x, const int y, const EAIClasses aiClass ) const;
	bool IsLocked4Class( const int x, const int y, const EAIClasses aiClass ) const;
	//
	void AddWaterTiles( const list<SVector> &tiles );
	//
	void SetMode( const ELockMode _eMode ) { eMode = _eMode; }

	// lock/unlock operations are unsafe without maxes update!
	void LockTile( const int x, const int y, const EAIClasses aiClass );
	void LockTile( const SObjTileInfo &tileInfo );
	void UnlockTile( const int x, const int y, const EAIClasses aiClass );
	void UnlockTile( const SObjTileInfo &tileInfo );

	void LockUnitProfile( const SUnitProfile &profile, const int id, SVector *pDownTile, SVector *pUpTile, const bool bWater );
	bool UnlockUnitProfile( const int id, SVector *pDownTile, SVector *pUpTile, bool *bWater );

	void LockTiles( const list<SObjTileInfo> &tiles, SVector *pDownTile, SVector *pUpTile );
	void UnlockTiles( const list<SObjTileInfo> &tiles, SVector *pDownTile, SVector *pUpTile );

	void UpdateMaxesForAddedTiles( int downX, int upX, int downY, int upY, const EAIClasses aiClasses );
	void UpdateMaxesForRemovedTiles( int downX, int upX, int downY, int upY, const EAIClasses aiClasses );

	void UpdateMaxesForAddedStObject( const SVector &downTile, const SVector &upTile );
	void UpdateMaxesForRemovedStObject( const SVector &downTile, const SVector &upTile );

	void InitMaxesDefault( const int nSizeX, const int nSizeY );
	void InitMaxes( const int nX1, const int nX2, const int nY1, const int nY2 );

	const int GetTerrainPassabilityType( const int nX, const int nY ) const;

	const int GetClassIndexFast( const BYTE aiClass ) const { return classIndices[aiClass]; }
	bool IsStaticLockedWOBoundaryCheck( const int x, const int y, const EAIClasses aiClass ) const;
	bool IsLocked4ClassWOBoundaryCheck( const int x, const int y, const EAIClasses aiClass ) const;

	void SmoothLock( const int xMin, const int yMin, const int xMax, const int yMax, const EAIClasses aiClass );
#ifndef _FINALRELEASE
	void AddLockInfo( const int nUnitID, const bool bLock, const SUnitProfile &profile );
#endif

	int operator&( IBinSaver &f ) { NI_ASSERT( false, "Can't serialize CTerrain, should be restored after load" ); return 0; }
public:
	CTerrain();
	CTerrain( CAIMap *pAIMap, const bool bInitMode );

	void PrepareTerraTypes( const int nCount );
	void SetTerraTypes( const int nIndex, const float fPass, const DWORD passClass, const BYTE soilType, const BYTE digImpossible );
	void UpdateTypes( const int nX1, const int nY1, const int nX2, const int nY2, const CArray2D<BYTE> &types );
	
	void AddTiles( const list<SVector> vTiles, const EAIClasses aiPassClass, const float fPassability, const int nSoilType, const bool bCanEntrench );
	void AddMarineTiles( const list<SVector> coastTiles, const BYTE coastSoilType, const list<SVector> waterTiles, const BYTE waterSoilType );

	// залокано конкретно для этого класса статическим объектом
	bool IsStaticLocked( const int x, const int y, const EAIClasses aiClass ) const;
	// залокано с учётом eMode
	bool IsLocked( const int x, const int y, const EAIClasses aiClass ) const;
	// give lock by static objects info
	EAIClasses GetTileLockInfo( const int x, const int y ) const;

	// Than tiles.size() more, than better, don't call very often with small tiles.size()
	// The best way of using - rare calls with big tiles.size()
	void AddStaticObjectTiles( const list<SObjTileInfo> &tiles );
	void RemoveStaticObjectTiles( const list<SObjTileInfo> &tiles );
	void RemoveStaticObjectTilesForBridge( const list<SObjTileInfo> &tiles );

	void AddUnitTiles( const int id, const SRect &rect, const bool bWater );
	void AddUnitTiles( const int id, const SUnitProfile &profile, const bool bWater );
	void RemoveUnitTiles( const int id );

	// units related functions
	EFreeTileInfo CanUnitGo( const int nBoundTileRadius, const SVector &tile, const EAIClasses aiClass ) const;
	EFreeTileInfo CanUnitGoToPoint( const int nBoundTileRadius, const CVec2 &point, const EAIClasses aiClass, CAIMap *pAIMap ) const;
	const CVec2 GetValidPoint( const int nBoundTileRadius, const CVec2 &vStart, const CVec2 &vEnd, const EAIClasses aiClass, const bool bFindWayBack, CAIMap *pAIMap ) const;

	const ETerrainTypes GetTerrainType( const int nX, const int nY ) const;

	// true - если был произведён unlock при вызове, false - если уже до этого был сделан unlock
	bool TemporaryUnlockUnitProfile( const int id, const SUnitProfile &unitProfile, const int nDecrease, const bool bWater );
	void RemoveTemporaryUnlocking( const int id );

	int TemporaryLockUnitProfile( const SUnitProfile &profile, const EAIClasses aiClass );
	int TemporaryLockTiles( list<SObjTileInfo> &tiles );
	void RemoveTemporaryLock( const int nLockID );

	// terrain passability
	const float GetPass( const int nX, const int nY ) const;

	// разлокивает тайл в соответствии с проходимостью terrain
	void RemoveTerrainPassability( const int nX, const int nY );
	// в прямоугольнике либо убирает террайн, либо ставит старый
	void UpdateTerrainPassabilityRect( const int nMinX, const int nMinY, const int nMaxX, const int nMaxY, bool bRemove );
	// локает тайл и проставаляет проходимость в соответствии с terrain nTerrainType, 
	void SetTerrainPassability( const int nX, const int nY, const int nTerrainType );
	// по номеру тайла выдаёт тип terrain
	const int GetTerrainPassTypeByTileNum( const int nTile ) { return terrSubTypes[nTile]; }

	const bool CanDigEntrenchment( const int x, const int y ) const;
	const void AddUndigableTiles( const list<SVector> &tiles );

	// is the tile on a bridge?
	const bool IsBridge( const SVector &tile ) const;
	void AddBridgeTile( const SVector &tile );
	void RemoveBridgeTile( const SVector &tile );

	BYTE GetSoilType( const SVector &tile ) const;

	void StartInitMode() { bInitMode = true; }
	void FinishInitMode();

	//void UpdateRiverPassability( const SVectorStripeObject &river, bool bAdd, bool bUpdate );

	// help functions
	bool IsLocked ( const SVector &coord, const EAIClasses aiClass ) const
	{
		return IsLocked( coord.x, coord.y, aiClass );
	}
	EAIClasses GetTileLockInfo( const SVector &tile ) const
	{
		return GetTileLockInfo( tile.x, tile.y );
	}
	const ETerrainTypes GetTerrainType( const SVector &tile ) const
	{
		return GetTerrainType( tile.x, tile.y );
	}
	const float GetPass( const CVec2 &point ) const
	{
		return GetPass( point.x, point.y );
	}

	// debug helpers
	void GetLockedTiles( vector<SVector> *pTiles, const EAIClasses aiClass, const EFreeTileInfo tileInfo );
	int ShowUnitLock( const int nUnitID, const int nMarkerID ) const;

	friend struct STerrainModeSetter;

	//for debug purpose only
	void DumpMaxes( const ELockMode lockMode, const EAIClasses aiClass, const string &szFileName );
	void DumpMaxes( const ELockMode lockMode, const EAIClasses aiClass, const string &szFileName, const vector<SVector> &markTiles );
	void DumpMaxes( const ELockMode lockMode, const EAIClasses aiClass, const string &szFileName, const vector<SVector> &linkTiles1, const vector<SVector> &linkTiles2, const bool bLink );
	void DumpStaticLock( const EAIClasses aiClass, const string &szFileName );
	void DumpPassability( const string &szFileName );
	void DumpBridges( const string &szFileName );
	void DumpUnitsBuf( const string &szFileName );
	void DumpLockInfo() const;

	//сгладить lock для всех maxes'ов
	void SmoothLock();
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTemporaryUnitProfileUnlocker
{
	bool bLocking;
	int nID;
	CTerrain *pTerrain;
public:
	CTemporaryUnitProfileUnlocker( const int nUnitID, const SUnitProfile &profile, const int nDecrease, const bool bWater, CTerrain *pTerrain );
	~CTemporaryUnitProfileUnlocker();
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTemporaryUnitProfileLocker
{
	int nLockID;
	CTerrain *pTerrain;
public:
	CTemporaryUnitProfileLocker( const SUnitProfile &profile, const EAIClasses aiClass, CTerrain *pTerrain );
	~CTemporaryUnitProfileLocker();
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct COMMON_RTS_AI_EXPORT STerrainModeSetter
{
	ELockMode eMemMode;
	CTerrain *pTerrain;

	explicit STerrainModeSetter( const ELockMode &eMode, CTerrain *pTerrain );
	~STerrainModeSetter();
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
