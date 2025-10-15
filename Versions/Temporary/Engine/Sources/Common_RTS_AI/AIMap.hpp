#pragma once

#include "AIMap.h"
#include "Misc/Bresenham.h"

inline bool operator < ( const SVector &cell1, const SVector &cell2 );

template <class TContainter>
inline void CAIMap::PushTile( const SVector &vCenter, const SVector &tile, TContainter *tiles, CArray2D1Bit &mask, const int nMaxRadius )
{
	if ( !mask.GetData( tile.x + nMaxRadius, tile.y + nMaxRadius ) )
	{
		if ( IsTileInside( vCenter + tile ) )
			tiles->push_back( vCenter + tile );
		mask.SetData( tile.x + nMaxRadius, tile.y + nMaxRadius );
	}
}

template <class TContainter>
inline bool CAIMap::Process8Tiles( const SVector &vCenter, const SVector &vOffset, TContainter *pTiles, const EAIClasses aiClass, const bool bAddOnly, CArray2D1Bit &mask )
{
	if ( bAddOnly )
	{
		const int nMaxRadius = ( mask.GetSizeX()-1 ) / 2;
		PushTile( vCenter, SVector( vOffset.x,  vOffset.y  ), pTiles, mask, nMaxRadius );
		PushTile( vCenter, SVector( -vOffset.x, vOffset.y  ), pTiles, mask, nMaxRadius );
		PushTile( vCenter, SVector( vOffset.x,  -vOffset.y ), pTiles, mask, nMaxRadius );
		PushTile( vCenter, SVector( -vOffset.x, -vOffset.y ), pTiles, mask, nMaxRadius );

		PushTile( vCenter, SVector( vOffset.y,  vOffset.x  ), pTiles, mask, nMaxRadius );
		PushTile( vCenter, SVector( -vOffset.y, vOffset.x  ), pTiles, mask, nMaxRadius );
		PushTile( vCenter, SVector( vOffset.y,  -vOffset.x ), pTiles, mask, nMaxRadius );
		PushTile( vCenter, SVector( -vOffset.y, -vOffset.x ), pTiles, mask, nMaxRadius );
		return false;
	}
	else
	{
		return 
			IsLocked( vCenter + SVector( vOffset.x,  vOffset.y  ), aiClass ) ||
			IsLocked( vCenter + SVector( -vOffset.x, vOffset.y  ), aiClass ) ||
			IsLocked( vCenter + SVector( vOffset.x,  -vOffset.y ), aiClass ) ||
			IsLocked( vCenter + SVector( -vOffset.x, -vOffset.y ), aiClass ) ||

			IsLocked( vCenter + SVector( vOffset.y,  vOffset.x  ), aiClass ) ||
			IsLocked( vCenter + SVector( -vOffset.y, vOffset.x  ), aiClass ) ||
			IsLocked( vCenter + SVector( vOffset.y,  -vOffset.x ), aiClass ) ||
			IsLocked( vCenter + SVector( -vOffset.y, -vOffset.x ), aiClass );
	}
}

template <class TContainter>
inline bool CAIMap::ProcessLineTiles( const SVector &vCenter, const SVector &vOffset, TContainter *pTiles, const EAIClasses aiClass, const bool bAddOnly, CArray2D1Bit &mask )
{
	CBres bres;
	bres.InitPoint( SVector( 0, 0 ), vOffset );

	SVector curOffset;
	do
	{
		curOffset = bres.GetDirection();
		if ( Process8Tiles( vCenter, curOffset, pTiles, aiClass, bAddOnly, mask ) )
			return true;
		bres.MakePointStep();
	}
	while ( curOffset != vOffset );
	return false;
}

template <class TContainter, int N>
inline bool CAIMap::ProcessLargeCircleTiles( const CVec2 &vCenter, const float fRadius, TContainter *pTiles, const EAIClasses aiClass, const SGenericNumber<N>& )
{
	const SVector vTileCenter = GetTile( vCenter );
	const int nTileRadius = fRadius/GetTileSize();

	if ( N )
		pTiles->clear();

	if ( !N )
	{
		const int nSizeX = GetSizeX() * GetTileSize();
		const int nSizeY = GetSizeY() * GetTileSize();
		if ( vTileCenter.x - nTileRadius < 0 || vTileCenter.x + nTileRadius > nSizeX ||
				 vTileCenter.y - nTileRadius < 0 || vTileCenter.y + nTileRadius > nSizeY )
			return true;
	}

	if ( nTileRadius < 2 )
	{
		if ( N )
			pTiles->push_back( vTileCenter );
		return false;
	}

	CArray2D1Bit mask;
	mask.SetSizes( 2*nTileRadius+1, 2*nTileRadius+1 );
	mask.FillZero();

	int x = 0;
	int y = nTileRadius;
	int d = 3 - 2*nTileRadius;
	while ( x <= y )
	{
		if ( N )
		{
			if ( ProcessLineTiles( vTileCenter, SVector( x, y ), pTiles, aiClass, true, mask ) )
				return true;
		}
		else
		{
			if ( ProcessLineTiles( vTileCenter, SVector( x, y ), pTiles, aiClass, false, mask ) )
				return true;
		}
		if ( d < 0 )
		{
			d += 4*x + 6;
		}
		else
		{
			d += 4*( x-y ) + 10;
			y--;
			if ( N )
			{
				if ( ProcessLineTiles( vTileCenter, SVector( x, y ), pTiles, aiClass, true, mask ) )
					return true;
			}
			else
			{
				if ( ProcessLineTiles( vTileCenter, SVector( x, y ), pTiles, aiClass, false, mask ) )
					return true;
			}
		}
		x++;
	}
	return false;
}

template <class TContainter, int N>
inline bool CAIMap::ProcessCircleTiles( const CVec2 &vCenter, const float fRadius, TContainter *pTiles, const EAIClasses aiClass, const SGenericNumber<N>& )
{
	const SVector vTileCenter = GetTile( vCenter );
	const int nTileRadius = fRadius/GetTileSize();

	if ( pTiles )
		pTiles->clear();

	if ( !N )
	{
		const int nSizeX = GetSizeX() * GetTileSize();
		const int nSizeY = GetSizeY() * GetTileSize();
		if ( vTileCenter.x - nTileRadius < 0 || vTileCenter.x + nTileRadius > nSizeX ||
				 vTileCenter.y - nTileRadius < 0 || vTileCenter.y + nTileRadius > nSizeY )
			return true;
	}
/*
	if ( nTileRadius < 2 )
	{
		if ( N )
			pTiles->push_back( vTileCenter );
		return false;
	}
*/
	std::vector<SVector> tiles = GetTilesForCircle( fRadius );
	for ( int i = 0; i < tiles.size(); ++i )
	{
		if ( N )
		{
			if ( IsTileInside( vTileCenter + tiles[i] ) )
				pTiles->push_back( vTileCenter + tiles[i] );
		}
		else if ( GetTerrain()->IsLocked( vTileCenter + tiles[i], aiClass ) )
			return true;
	}
	return false;
}

template <class TContainter, int N>
inline bool CAIMap::ProcessQuadrangleTiles( const CVec2 &v1, const CVec2 &v2, const CVec2 &v3, const CVec2 &v4, TContainter *pTiles, const EAIClasses aiClass, const SGenericNumber<N>& )
{
	if ( N )
		pTiles->clear();

	// выход за пределы карты
	if ( !N )
	{
		const int nSizeX = GetSizeX() * GetTileSize();
		const int nSizeY = GetSizeY() * GetTileSize();
		if ( v1.x < 0 || v1.y < 0 || v2.x < 0 || v2.y < 0 || v3.x < 0 || v3.y < 0 || v4.x < 0 || v4.y < 0 ||
			v1.x >= nSizeX || v1.y >= nSizeY || v2.x >= nSizeX || v2.y >= nSizeY ||
			v3.x >= nSizeX || v3.y >= nSizeY || v4.x >= nSizeX || v4.y >= nSizeY )
			return true;
	}

	const CVec2 center = 0.25f * ( v1 + v2 + v3 + v4 );
	const CLine2 line1( v1, v2 );
	const int sign1 = line1.GetSign( center );
	const CLine2 line2( v2, v3 );
	const int sign2 = line2.GetSign( center );
	const CLine2 line3( v3, v4 );
	const int sign3 = line3.GetSign( center );
	const CLine2 line4( v4, v1 );
	const int sign4 = line4.GetSign( center );

	const SVector tile1 = GetTile( v1 );
	const SVector tile2 = GetTile( v2 );
	const SVector tile3 = GetTile( v3 );
	const SVector tile4 = GetTile( v4 );

	SVector p1( tile1 ), p2( tile1 );
	if ( tile2.y < p1.y ) p1 = tile2;
	if ( tile2.y > p2.y ) p2 = tile2;
	if ( tile3.y < p1.y ) p1 = tile3;
	if ( tile3.y > p2.y ) p2 = tile3;
	if ( tile4.y < p1.y ) p1 = tile4;
	if ( tile4.y > p2.y ) p2 = tile4;

	if ( p1 == p2 )
	{
		if ( N )
		{
			if ( IsTileInside( p1 ) )
				pTiles->push_back( p1 );
		}
		else if ( IsTileInside( p1 ) && IsLocked( p1, aiClass ) )
			return true;
	}
	else if ( TriangleArea2( tile1.ToCVec2(), tile2.ToCVec2(), tile3.ToCVec2() ) == 0 )
	{
		CBres bres;
		bres.InitPoint( p1, p2 );

		SVector curTile;
		do
		{
			curTile = bres.GetDirection();
			if ( N )
				pTiles->push_back( curTile );
			else if ( IsLocked( curTile, aiClass ) )
				return true;

			bres.MakePointStep();
		}
		while ( curTile != p2 );
	}
	else
	{
		CBres bres;
		bres.InitPoint( p1, p2 );

		bool bFlag = true;
		while ( bFlag )
		{
			SVector curTile( bres.GetDirection() );
			CVec2 curPoint = GetPointByTile( curTile );

			if ( line1.GetSign( curPoint ) == sign1 && line2.GetSign( curPoint ) == sign2 &&
					 line3.GetSign( curPoint ) == sign3 && line4.GetSign( curPoint ) == sign4 )
			{
				if ( IsTileInside( curTile ) )
				{
					if ( N )
						pTiles->push_back( curTile );
					else if ( IsLocked( curTile, aiClass ) )
						return true;
				}
			}

			--curTile.x;
			curPoint.x -= GetTileSize();
			while ( line1.GetSign( curPoint ) == sign1 && line2.GetSign( curPoint ) == sign2 &&
							line3.GetSign( curPoint ) == sign3 && line4.GetSign( curPoint ) == sign4 )
			{
				if ( IsTileInside( curTile ) )
				{
					if ( N )
						pTiles->push_back( curTile );
					else if ( IsLocked( curTile, aiClass ) )
						return true;
				}

				--curTile.x;
				curPoint.x -= GetTileSize();
			}

			curTile = bres.GetDirection();
			curPoint = GetPointByTile( curTile );

			++curTile.x;
			curPoint.x += GetTileSize();
			while ( line1.GetSign( curPoint ) == sign1 && line2.GetSign( curPoint ) == sign2 &&
							line3.GetSign( curPoint ) == sign3 && line4.GetSign( curPoint ) == sign4 )
			{
				if ( IsTileInside( curTile ) )
				{
					if ( N )
						pTiles->push_back( curTile );
					else if ( IsLocked( curTile, aiClass ) )
						return true;
				}

				++curTile.x;
				curPoint.x += GetTileSize();
			}

			if ( bres.GetDirection() == p2 )
				bFlag = false;
			else
			{
				const int y  = bres.GetDirection().y;
				do
				{
					bres.MakePointStep();
					if ( bres.GetDirection() == p2 )
					{
						bFlag = false;
						break;
					}
				} while ( bres.GetDirection().y == y );
			}
		}
	}

	if ( N )
	{ 
		const SVector tile( GetTile( center ) );
		if ( pTiles->empty() && IsTileInside( tile ) )
			pTiles->push_back( tile );
	}

	return false;
}

template<class TContainter>
inline void CAIMap::GetTilesCoveredByQuadrangle( const CVec2 &v1, const CVec2 &v2, const CVec2 &v3, const CVec2 &v4, TContainter *pTiles )
{
	ProcessQuadrangleTiles( v1, v2, v3, v4, pTiles, EAC_NONE, SGenericNumber<1>() );
}

template<class TContainter>
inline void CAIMap::GetTilesCoveredByCircle( const CVec2 &vCenter, const float fRadius, TContainter *pTiles )
{
	ProcessCircleTiles( vCenter, fRadius, pTiles, EAC_NONE, SGenericNumber<1>() ); 
}

template<class TContainter>
inline void CAIMap::GetTilesCoveredByLargeCircle( const CVec2 &vCenter, const float fRadius, TContainter *pTiles )
{
	ProcessLargeCircleTiles( vCenter, fRadius, pTiles, EAC_NONE, SGenericNumber<1>() ); 
}

template<class TContainter>
inline void CAIMap::GetTilesCoveredByRect( const SRect &rect, TContainter *pTiles )
{
	ProcessQuadrangleTiles( rect.v1(), rect.v2(), rect.v3(), rect.v4(), pTiles, EAC_NONE, SGenericNumber<1>() );
}

// для GetTilesCoveredByRectSides
template<typename TContainter>
class CTilesCollector
{
public:
	TContainter *pTiles;
	CAIMap *pAIMap;
	CTilesCollector( TContainter* _pTiles, CAIMap *_pAIMap ) : pTiles( _pTiles ), pAIMap( _pAIMap ) { }

	bool operator()( float x, float y ) 
	{ 
		if ( pAIMap->IsTileInside( x, y ) )
			pTiles->push_back( SVector( x, y ) );

		return true; 
	}
};

template<class TContainter>
inline void CAIMap::GetTilesCoveredByRectSides( const SRect &rect, TContainter *pTiles )
{
	CTilesCollector<TContainter> a( pTiles, this );

	MakeLine2( rect.v1().x / GetTileSize(), rect.v1().y / GetTileSize(), rect.v2().x / GetTileSize(), rect.v2().y / GetTileSize(), a );
	MakeLine2( rect.v2().x / GetTileSize(), rect.v2().y / GetTileSize(), rect.v3().x / GetTileSize(), rect.v3().y / GetTileSize(), a );
	MakeLine2( rect.v3().x / GetTileSize(), rect.v3().y / GetTileSize(), rect.v4().x / GetTileSize(), rect.v4().y / GetTileSize(), a );
	MakeLine2( rect.v4().x / GetTileSize(), rect.v4().y / GetTileSize(), rect.v1().x / GetTileSize(), rect.v1().y / GetTileSize(), a );
}


