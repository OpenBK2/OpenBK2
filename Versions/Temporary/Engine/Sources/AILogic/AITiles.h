#pragma once

namespace AICellsTiles
{

inline const SVector GetTileNC( const float x, const float y )
{ 
	SVector res;
	res.x = x / SAIConsts::TILE_SIZE;
	res.y = y / SAIConsts::TILE_SIZE;

	return res;
}

inline const SVector GetTileNC( const CVec2 &vPos )
{ 
	return GetTileNC( vPos.x, vPos.y );
}

inline const SVector GetTile( const float x, const float y )
{ 
	SVector res;
	res.x = x < 0 ? 0 : x / SAIConsts::TILE_SIZE;
	res.y = y < 0 ? 0 : y / SAIConsts::TILE_SIZE;

	return res;
}

inline const SVector GetTile( const CVec2 &point )
{
	return GetTile( point.x, point.y );
}

// get center of the tile in point's coordinates by the point's coordinates
inline const CVec2 GetCenterOfTile( const float x, const float y )
{
	const SVector tile( GetTile( x, y ) );
	return CVec2( tile.x * SAIConsts::TILE_SIZE + SAIConsts::TILE_SIZE/2, 
		tile.y * SAIConsts::TILE_SIZE + SAIConsts::TILE_SIZE/2 );
}

// get center of the tile in point's coordinates by the point's coordinates
inline const CVec2 GetCenterOfTile( const CVec2& point )
{
	return GetCenterOfTile( point.x, point.y );
}

// point coordinates by AI tile coordinates
inline const CVec2 GetPointByTile( const int x, const int y )
{
	return CVec2( x * SAIConsts::TILE_SIZE + SAIConsts::TILE_SIZE/2, y * SAIConsts::TILE_SIZE + SAIConsts::TILE_SIZE/2 );
}

inline const CVec2 GetPointByTile( const SVector &tile )
{
	return GetPointByTile( tile.x, tile.y );
}

}

