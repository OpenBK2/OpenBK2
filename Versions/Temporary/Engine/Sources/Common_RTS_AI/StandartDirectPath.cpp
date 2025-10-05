#include "stdafx.h"
#include "StandartDirectPath.h"

void CStandartDirectPath::Init( const std::vector<SVector> &_tiles, const int nTileSize )
{
	tiles.clear();

	for ( std::vector<SVector>::const_iterator it = _tiles.begin(); it != _tiles.end(); ++it )
		tiles.push_back( *it );

	nCurrentTile = 0;
	fTileSize = (float)nTileSize;
	if ( tiles.size() > 0 )
	{
		vStartPoint = GetPoint( tiles[0] );
		vFinishPoint = GetPoint( tiles[tiles.size() - 1] );
	}
}

void CStandartDirectPath::InsertTiles( const std::list<SVector> &_tiles )
{
	tiles.clear();

	for ( std::list<SVector>::const_iterator it = _tiles.begin(); it != _tiles.end(); ++it )
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


