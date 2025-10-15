#include "stdafx.h"

#include "AIMap.h"
#include "StaticMapHeights.h"
#include "Terrain.h"
#include "Misc/Bresenham.h"

#include <cstdint>

const CVec2 CAIMap::GetCenterOfTile( const float x, const float y ) const
{
	const SVector tile( GetTile( x, y ) );
	return CVec2( tile.x * nTileSize + nTileSize/2, tile.y * nTileSize + nTileSize/2 );
}

const CVec2 CAIMap::GetCenterOfTile( const CVec2 &point ) const
{
	return GetCenterOfTile( point.x, point.y );
}

bool CAIMap::IsTileInside( const int x, const int y ) const
{
	return x >= 0 && y >= 0 && x < nSizeX && y < nSizeY;
}

bool CAIMap::IsTileInside( const SVector &tile ) const
{
	return IsTileInside( tile.x, tile.y );
}

bool CAIMap::IsPointInside( const float x, const float y ) const
{
	return x >= 0 && y >= 0 && x < GetSizeX() * nTileSize && y < GetSizeY() * nTileSize;
}

bool CAIMap::IsPointInside( const CVec2 &point ) const
{
	return IsPointInside( point.x, point.y );
}

const SVector CAIMap::GetTile( const float x, const float y ) const
{
	return SVector( x/nTileSize, y/nTileSize );
}

const SVector CAIMap::GetTile( const CVec2 &point ) const
{
	return GetTile( point.x, point.y );
}

CVec2 CAIMap::GetPointByTile( const int x, const int y ) const
{
	return CVec2( x * nTileSize + nTileSize/2, y * nTileSize + nTileSize/2 );
}

CVec2 CAIMap::GetPointByTile( const SVector &tile ) const
{
	return GetPointByTile( tile.x, tile.y );
}

CVec2 CAIMap::GetPointByTile( const SObjTileInfo &tileInfo ) const
{
	return GetPointByTile( tileInfo.tile );
}

CAIMap::CAIMap( const int _nSizeX, const int _nSizeY, const int _nTileSize, 
								const int _nMaxUnitTileRadius, const int _nMaxMapSize )
:	nSizeX( _nSizeX ), nSizeY( _nSizeY ), nTileSize( _nTileSize ),
	nMaxUnitTileRadius( _nMaxUnitTileRadius ), nMaxMapSize( _nMaxMapSize )
{
	pHeights = new CStaticMapHeights( nSizeX, nSizeY, nTileSize );
	pTerrain = new CTerrain( this, true );
}

bool CAIMap::IsRectInside( const SRect &rect ) const
{
	return IsPointInside( rect.v1() ) && IsPointInside( rect.v2() ) &&
				 IsPointInside( rect.v3() ) && IsPointInside( rect.v4() );
}

static std::vector< std::vector<SVector> > circleTiles;

void CAIMap::AddTile( const SVector &tile, std::vector<SVector> &tiles, CArray2D1Bit &mask, const int nMaxRadius )
{
	if ( !mask.GetData( tile.x + nMaxRadius, tile.y + nMaxRadius ) )
	{
		tiles.push_back( tile );
		mask.SetData( tile.x + nMaxRadius, tile.y + nMaxRadius );
	}
}

void CAIMap::Add8TilesEven( const SVector vOffset, std::vector<SVector> &tiles, CArray2D1Bit &mask, const int nMaxRadius )
{
	AddTile( SVector( vOffset.x,     vOffset.y     ), tiles, mask, nMaxRadius );
	AddTile( SVector( 1 - vOffset.x, vOffset.y     ), tiles, mask, nMaxRadius );
	AddTile( SVector( vOffset.x,     1 - vOffset.y ), tiles, mask, nMaxRadius );
	AddTile( SVector( 1 - vOffset.x, 1 - vOffset.y ), tiles, mask, nMaxRadius );

	AddTile( SVector( vOffset.y,     vOffset.x     ), tiles, mask, nMaxRadius );
	AddTile( SVector( 1 - vOffset.y, vOffset.x     ), tiles, mask, nMaxRadius );
	AddTile( SVector( vOffset.y,     1 - vOffset.x ), tiles, mask, nMaxRadius );
	AddTile( SVector( 1 - vOffset.y, 1 - vOffset.x ), tiles, mask, nMaxRadius );
}

void CAIMap::AddLinesEven( const SVector vOffset, std::vector<SVector> &tiles, CArray2D1Bit &mask, const int nMaxRadius )
{
	CBres bres;
	bres.InitPoint( SVector( 1, 1 ), vOffset );

	SVector curOffset;
	do
	{
		curOffset = bres.GetDirection();
		Add8TilesEven( curOffset, tiles, mask, nMaxRadius );
		bres.MakePointStep();
	}
	while ( curOffset != vOffset );
}

void CAIMap::Add8TilesOdd( const SVector vOffset, std::vector<SVector> &tiles, CArray2D1Bit &mask, const int nMaxRadius )
{
	AddTile( SVector( vOffset.x,  vOffset.y  ), tiles, mask, nMaxRadius );
	AddTile( SVector( -vOffset.x, vOffset.y  ), tiles, mask, nMaxRadius );
	AddTile( SVector( vOffset.x,  -vOffset.y ), tiles, mask, nMaxRadius );
	AddTile( SVector( -vOffset.x, -vOffset.y ), tiles, mask, nMaxRadius );

	AddTile( SVector( vOffset.y,  vOffset.x  ), tiles, mask, nMaxRadius );
	AddTile( SVector( -vOffset.y, vOffset.x  ), tiles, mask, nMaxRadius );
	AddTile( SVector( vOffset.y,  -vOffset.x ), tiles, mask, nMaxRadius );
	AddTile( SVector( -vOffset.y, -vOffset.x ), tiles, mask, nMaxRadius );
}

void CAIMap::AddLinesOdd( const SVector vOffset, std::vector<SVector> &tiles, CArray2D1Bit &mask, const int nMaxRadius )
{
	CBres bres;
	bres.InitPoint( SVector( 0, 0 ), vOffset );

	SVector curOffset;
	do
	{
		curOffset = bres.GetDirection();
		Add8TilesOdd( curOffset, tiles, mask, nMaxRadius );
		bres.MakePointStep();
	}
	while ( curOffset != vOffset );
}

SVector CAIMap::GetOffset( const uint16_t wAngle, const int nDiameter )
{
	const float fRadius = nDiameter/2;
	const CVec2 vAngel = fRadius * GetVectorByDirection( wAngle );
	return SVector( vAngel.x, vAngel.y );
}

void CAIMap::RecreateCircles()
{
	const int nMaxRadius = GetMaxUnitTileRadius() + 1;
	circleTiles.clear();
	//circleTiles.reserve( 2*nMaxRadius );

	std::vector<SVector> tiles;
	circleTiles.push_back( tiles );

	tiles.push_back( SVector( 0, 0 ) );
	circleTiles.push_back( tiles );

	tiles.push_back( SVector( 0, 1 ) );
	tiles.push_back( SVector( 1, 0 ) );
	tiles.push_back( SVector( 1, 1 ) );
	circleTiles.push_back( tiles );

	CArray2D1Bit mask;
	mask.SetSizes( 2*nMaxRadius + 1, 2*nMaxRadius + 1 );

	for ( int i = 3; i <= 2*nMaxRadius; ++i )
	{
		tiles.clear();
		mask.FillZero();

		const float fDelta = atan2f( 1, 4*nMaxRadius ) * 65536.0f / FP_2PI;
		const uint16_t wDelta = fDelta;
		uint16_t wAngle = 0;
		SVector vOffset = GetOffset( wAngle, i );
		do 
		{
			if ( i%2 == 0 )
				AddLinesEven( vOffset, tiles, mask, nMaxRadius );
			else 
				AddLinesOdd( vOffset, tiles, mask, nMaxRadius );

			const SVector vPrevOffset = vOffset;
			do 
			{
				wAngle += wDelta;
				vOffset = GetOffset( wAngle, i );
			} while( vPrevOffset == vOffset );
		} while( wAngle < 16384 );

		circleTiles.push_back( tiles );
	}
}

std::vector<SVector>& CAIMap::GetTilesForCircle( const float fRadius )
{
	const int nDiameter = 2.0f*fRadius/GetTileSize();

	if ( nDiameter >= circleTiles.size() )
		return circleTiles[0];
	else
		return circleTiles[nDiameter];
}

bool CAIMap::IsRectOnLockedTiles( const SRect &rect, const EAIClasses aiClass )
{
	return ProcessQuadrangleTiles( rect.v1(), rect.v2(), rect.v3(), rect.v4(), (std::list<SVector>*)0, aiClass, SGenericNumber<0>() );
}

bool CAIMap::IsCircleOnLockedTiles( const CCircle &circle, const EAIClasses aiClass )
{
	return ProcessCircleTiles( circle.center, circle.r, (std::list<SVector>*)0, aiClass, SGenericNumber<0>() );
}

bool CAIMap::IsLocked( const int x, const int y, const EAIClasses aiClass ) const
{
	return pTerrain->IsLocked( x, y, aiClass );
}

bool CAIMap::IsLocked( const SVector &tile, const EAIClasses aiClass ) const
{
	return IsLocked( tile.x, tile.y, aiClass );
}

void CAIMap::OnSerialize( IBinSaver &saver )
{
	if ( saver.IsReading() )
	{
		pHeights = new CStaticMapHeights( nSizeX, nSizeY, nTileSize );
		pTerrain = new CTerrain( this, true );
	}
}

void CAIMap::Clear()
{
	pHeights->Clear();
	pTerrain->Clear();
}

REGISTER_SAVELOAD_CLASS( COMMON_RTS_AI, 0x30159B00, CAIMap );

