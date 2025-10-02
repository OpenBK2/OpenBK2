#include "stdafx.h"
#include "StandartDirectPath.h"

void CStandartDirectPath::Init( const vector<SVector> &_tiles, const int nTileSize )
{
	tiles.clear();

	for ( vector<SVector>::const_iterator it = _tiles.begin(); it != _tiles.end(); ++it )
		tiles.push_back( *it );

	nCurrentTile = 0;
	fTileSize = (float)nTileSize;
	if ( tiles.size() > 0 )
	{
		vStartPoint = GetPoint( tiles[0] );
		vFinishPoint = GetPoint( tiles[tiles.size() - 1] );
	}
}

void CStandartDirectPath::InsertTiles( const list<SVector> &_tiles )
{
	tiles.clear();

	for ( list<SVector>::const_iterator it = _tiles.begin(); it != _tiles.end(); ++it )
		tiles.push_back( *it );

	nCurrentTile = 0;
	if ( tiles.size() > 0 )
	{
		vStartPoint = GetPoint( tiles[0] );
		vFinishPoint = GetPoint( tiles[tiles.size() - 1] );
	}
}

void CStandartDirectPath::MarkPath( const int nID, const NDebugInfo::EColor color ) const
{
	DebugInfoManager()->CreateMarker( nID, tiles, color );
}

REGISTER_SAVELOAD_CLASS( 0x31224300, CStandartDirectPath );


