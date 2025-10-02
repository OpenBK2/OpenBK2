#pragma once

#include "UpdateUnitContainer.h"
#include "Misc/BitData.h"

namespace NDb
{
	struct SScriptArea;
}

struct SSector
{
	ZDATA
		SAIAngle wStartAngle;
		SAIAngle wEndAngle;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&wStartAngle); f.Add(3,&wEndAngle); return 0; }

	SSector() : wStartAngle( 0 ), wEndAngle( 65535 ) {}
	SSector( const DWORD _wStartAngle, const DWORD _wEndAngle ) : wStartAngle( _wStartAngle ), wEndAngle( _wEndAngle ) {}

	bool operator!=( const SSector &sector ) { return ( wStartAngle != sector.wStartAngle || wEndAngle != sector.wEndAngle ); }
	bool IsIntersec( const SSector &sector ) const
	{
		return IsInTheAngle( sector.wStartAngle, wStartAngle, wEndAngle ) || IsInTheAngle( sector.wEndAngle, wStartAngle, wEndAngle ) ||
			IsInTheAngle( wStartAngle, sector.wStartAngle, sector.wEndAngle ) || IsInTheAngle( wEndAngle, sector.wStartAngle, sector.wEndAngle );
	}
	bool IsWholeCircle() const { return ( wEndAngle - wStartAngle == 65535 ); }
};

struct SSpiralPoint
{
	ZDATA
		SVector vOffset;
		int nRadius;
		SSector sector;
		SAIAngle wAngle;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&vOffset); f.Add(3,&nRadius); f.Add(4,&sector); f.Add(5,&wAngle); return 0; }

	SSpiralPoint() : vOffset( 0, 0 ), nRadius( 0 ), sector() {}
	SSpiralPoint( const int x, const int y, const int r ) : vOffset( x, y ), nRadius( r )	{ wAngle = sector.wStartAngle = sector.wEndAngle = GetDirectionByVector( x, y ); }
};

struct SWarFogUnitInfo
{
	ZDATA
		SVector vPos;
		SSector sector;
		int nRadius;
		bool bPlane;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&vPos); f.Add(3,&sector); f.Add(4,&nRadius); f.Add(5,&bPlane); return 0; }

	SWarFogUnitInfo() : vPos( 0, 0 ) , sector(), nRadius( 0 ), bPlane( false ) {}
};

class CGlobalWarFog : public CAIObjectBase
{

// internal stuctures
	struct SWarForFullUnitInfo : public SWarFogUnitInfo
	{
		ZDATA_( SWarFogUnitInfo )
			SVector vOldPos;
			int nOldRadius;
			int nParty;	// warfog works only with two parties, so if updateFlag is UPD_CHANGE_PARTY new party is 1 - nParty ;-)
			CArray1Bit visValues;
			EUpdateWarFogUnitInfoFlag updateFlag;
		ZEND int operator&( IBinSaver &f ) { f.Add(1,( SWarFogUnitInfo *)this); f.Add(2,&vOldPos); f.Add(3,&nOldRadius); f.Add(4,&nParty); f.Add(5,&visValues); f.Add(6,&updateFlag); return 0; }

		SWarForFullUnitInfo() : SWarFogUnitInfo(), vOldPos( 0, 0 ), nOldRadius( 0 ), nParty( 0 ), updateFlag( UPD_UPDATED ) {}
		SWarForFullUnitInfo( const SWarFogUnitInfo &unitInfo, const int nParty, const int nSpiralLength );

		void Update()
		{
			vOldPos = vPos;
			nOldRadius = nRadius;
			updateFlag = UPD_UPDATED;
		}
	};

	struct SWarFogTileInfo
	{
		int nVisible;
		int nCloseVisible;
		SWarFogTileInfo() : nVisible( 0 ), nCloseVisible( 0 ) {}
	};

// internal types
	typedef	hash_map<int, SWarForFullUnitInfo> TWarFogUnitsMap;
	typedef CArray2D<SWarFogTileInfo> TWarFog;
	typedef hash_set<SVector, STilesHash> TTilesToSmooth;

	OBJECT_NOCOPY_METHODS( CGlobalWarFog );
	CArray2D<CArray1Bit> areas;					// calculated spirals for every point
	CArray2D1Bit valid;									// valid tiles on map
	CArray2D1Bit calced;								// tiles that sometimes calced
	int nAreasCalced;
	CArray2D<int> heights;							// heights
	bool bInitialization;
	bool bHasInvalidTile;

	CArray2D<BYTE> miniMapWarFog;
	bool bNeedFullCalc;
	NTimer::STime nLastFogCalcTime;
	vector<BYTE> miniMapSums;					
	int nMiniMapY;


	/*
	TTilesToSmooth miniMapSmoothTiles;
	BYTE cMiniMapWarFogParty;
	bool bMiniMapWarFogReady;
	bool bFullSmooth;
	*/
	
	vector<SSpiralPoint> spiral;   // S(круга) * sizeof( SSpiralPoint ) = S(круга) * 18
	CArray2D<int> spiralCoords;    // MaxRadius * MaxRadius
	vector<int> lengths;           // MaxRadius * sizeof( int )
	ZDATA
		int nSizeX;
		int nSizeY;
		int nMaxRadius;
		int nUnitHeight;

		vector<TWarFog> warFog;

		CArray2D1Bit areasOpenTiles;
		hash_set<string> scriptAreas;

		CArray2D<int> staticObjects;

		TWarFogUnitsMap units;
		CUpdateUnitContainer updateUnitList;
		ZONSERIALIZE
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nSizeX); f.Add(3,&nSizeY); f.Add(4,&nMaxRadius); f.Add(5,&nUnitHeight); f.Add(6,&warFog); f.Add(7,&areasOpenTiles); f.Add(8,&scriptAreas); f.Add(9,&staticObjects); f.Add(10,&units); f.Add(11,&updateUnitList); OnSerialize( f ); return 0; }
private:
	void OnSerialize( IBinSaver &saver );
	void ReInit();
	void ReInitDefaults();

	const bool IsTileValid( const int x, const int y ) const { return valid.GetData( x, y ); }
	const bool IsTileValid( const SVector &tile ) const { return IsTileValid( tile.x, tile.y ); }

	void RemoveTileValid( const int x, const int y );
	void RemoveTileValid( const SVector &vTile ) { RemoveTileValid( vTile.x, vTile.y ); }
	
	void RecalculateArea( const SVector &vTile );
  
	void SetTileHeight( const int x, const int y, const float fHeight );
	void SetTileHeight( const SVector &tile, const float fHeight ) { SetTileHeight( tile.x, tile.y, fHeight ); }
	const int GetHeight( const SVector &tile ) const { return heights[tile.y][tile.x]; }
	const int GetUnitHeight() const { return nUnitHeight; }

	const bool IsTileInside( const int x, const int y ) const { return ( x >= 0 && y >= 0 && x < GetSizeX() && y < GetSizeY() ); }
	const bool IsTileInside( const SVector &tile ) const { return IsTileInside( tile.x, tile.y ); }

	const CArray1Bit &GetVisibleInfoForTile( const SVector &tile );

	void AddVisibleTiles( const int nID, SWarForFullUnitInfo &unitInfo );
	void RemoveVisibleTiles( const int nID, SWarForFullUnitInfo &unitInfo );

	bool IsOpenBySriptArea( const int x, const int y ) const { return areasOpenTiles.GetData( x, y ); }
	bool IsOpenBySriptArea( const SVector &tile ) const { return IsOpenBySriptArea( tile.x, tile.y ); }
	const int GetStaticObjectAtTile( const SVector &vTile ) const { return staticObjects[vTile.y][vTile.x]; }

	const int GetSpiralLength() const { return spiral.size(); }
	const int GetSpiralLength( const int nRadius ) { return lengths[ Min( GetMaxRadius(), nRadius ) ]; }
	const SSpiralPoint &GetSpiralPoint( const int nIndex ) const { return spiral[ nIndex ]; }

	void OnTileChangeVisibility( const SVector &vTile, const bool bVisible, const int nParty );

	const BYTE GetClientTileVis( const int x, const int y, const int nParty ) const { return IsTileVisible( SVector( x, y ), nParty ) ? SAIConsts::VIS_POWER : 0; }
	const bool IsValidParty( const int nParty ) const { return nParty == 0 || nParty == 1; }
public:
	void Init( const int nSizeX, const int nSizeY, const int nMaxRadius, const float fUnitHeight );
	void FinishInitialization() { bInitialization = false; bHasInvalidTile = true; valid.FillZero(); }
	void Clear();

	void AddUnit( const int nID, const SWarFogUnitInfo &unitInfo, const int nParty );
	void DeleteUnit( const int nID );
	void UpdateUnit( const int nID, const SWarFogUnitInfo &unitInfo );
	void ChangeParty( const int nID, const SWarFogUnitInfo &updateInfo, const int nNewParty );

	void SynchronizeHeights( const SVector &vUpLeftTile, const SVector &vBottomRightTile );

	bool GetWarForInfo( CArray2D<BYTE> **pWarFogInfo, const int nParty, const bool bFirstTime );
	
	int GetSizeX() const { return nSizeX; }
	int GetSizeY() const { return nSizeY; }
	int GetMaxRadius() const { return nMaxRadius; }

	bool IsTileVisible( const SVector &tile, const int nParty ) const;
	bool IsUnitVisible( const SVector &tile, const int nParty, bool bCamouflated ) const;

	void ToggleOpenForScriptAreaTiles( const NDb::SScriptArea &scriptArea, bool bOpen );

	void AddStaticObjectTile( const SVector &vTile, const int nObjectID, const float fHeight );
	void RemoveStaticObjectTile( const SVector &vTile, const int nObjectID );
	void ReplaceStaticObjects( const int nNewID, const hash_set<int> &oldIDs );

	bool IsTraceable( const SVector &tile1, const SVector &tile2 );

	void Segment();

	// debug functions
	void DumpWarFog();

	friend class CWarFogVisibility;
	friend class CWarFogTracer;
};


