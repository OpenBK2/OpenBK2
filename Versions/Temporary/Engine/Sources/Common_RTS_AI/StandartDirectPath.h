#pragma once

#include "Path.h"

class CStandartDirectPath : public IPath
{
	OBJECT_NOCOPY_METHODS( CStandartDirectPath )
	ZDATA
		int nCurrentTile;
		vector<SVector> tiles;
		float fTileSize;
		CVec2 vStartPoint;
		CVec2 vFinishPoint;
public:
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nCurrentTile); f.Add(3,&tiles); f.Add(4,&fTileSize); f.Add( 5,&vStartPoint); f.Add( 6, &vFinishPoint); return 0; }
private:
	const CVec2 GetPoint( const SVector &tile ) const { return CVec2( tile.x * fTileSize, tile.y * fTileSize ); }
public:
	CStandartDirectPath() : nCurrentTile( -1 ), fTileSize( 0.0f ), vStartPoint( VNULL2 ), vFinishPoint( VNULL2 ) {}
	void Init( const vector<SVector> &tiles, const int nTileSize );

	bool IsFinished() const { return nCurrentTile == tiles.size(); }

	const CVec2 PeekPoint( const int nShift ) const { return GetPoint( tiles[Min( nCurrentTile + nShift, tiles.size()-1 )] ); }
	void Shift( const int nShift ) { nCurrentTile = Min( nCurrentTile + nShift, tiles.size() ); }

	const CVec2& GetFinishPoint() const { return vFinishPoint; }
	const CVec2& GetStartPoint() const { return vStartPoint; }

	//! восстановить путь из новой точки ( vPoint )
	void RecoverPath( const CVec2 &vPoint, const bool bIsPointAtWater, const SVector &vLastKnownGoodTile ) {}
	//! пересчитать путь из новой точки ( vPoint )
	void RecalcPath( const CVec2 &vPoint, const bool bIsPointAtWater, const SVector &vLastKnownGoodTile ) {}
	//! добавить тайлы в начало пути
	void InsertTiles( const list<SVector> &tiles );
	//! можно ли проехать весь путь задом
	const bool CanGoBackward( const CBasePathUnit *pUnit ) const { return true; }
	const bool ShouldCheckTurn() const { return false; }
	//! можно ли для этого пути построить сложный разворот
	const bool CanBuildComplexTurn() const { return false; }

	void MarkPath( const int nID, const NDebugInfo::EColor color ) const;
};
