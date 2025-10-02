#include "StdAfx.h"

#include "StaticMapHeights.h"
#include "DebugTools/DebugInfoManager.h"

void CStaticMapHeights::GetPoint4Spline( const CVec2 &vPoint, float *pu, float *pv, float ptCtrls[] ) const
{
	int nTileX = vPoint.x / 2 / nTileSize;
	int nTileY = vPoint.y / 2 / nTileSize;

	if ( nTileX > heights.GetSizeX() - 4 )
		nTileX = heights.GetSizeX() - 4;
	if ( nTileY > heights.GetSizeY() - 4 )
		nTileY = heights.GetSizeY() - 4;

	CVec2 vTileStart( nTileX * nTileSize * 2, nTileY * nTileSize * 2 );
	*pu = ( vPoint.x - vTileStart.x ) / ( 2.0f * nTileSize );
	*pv = ( vPoint.y - vTileStart.y ) / ( 2.0f * nTileSize );

	NI_ASSERT( *pu >= 0 && *pu <= 1, "Wrong u" );
	NI_ASSERT( *pv >= 0 && *pv <= 1, "Wrong v" );

	for ( int i = 0; i < 4; ++i )
		for ( int j = 0; j < 4; ++j )
			ptCtrls[i * 4 + j] = heights[nTileY + i][nTileX + j] / ( 2.0f * nTileSize );
}

const DWORD CStaticMapHeights::GetNormal( const float x, const float y ) const
{
	if ( !(x >= 0 && x < nStaticMapSizeX*nTileSize && y >= 0 && y < nStaticMapSizeY*nTileSize) )
		return DWORD( BYTE(127.0f) ) << 16;
	else
	{
		float u, v;
		float ptCtrls[16];

		GetPoint4Spline( CVec2( x, y ), &u, &v, ptCtrls );

		CVec3 result;
		betaSpline3D.GetNormale( &result, u, v, ptCtrls );

		return Vec3ToDWORD( result );
	}
}

const float CStaticMapHeights::GetVisZ( float x, float y ) const
{
	float u, v;
	float ptCtrls[16];

	x = Clamp( x, 0.0f, float( nTileSize * nStaticMapSizeX ) );
	y = Clamp( y, 0.0f, float( nTileSize * nStaticMapSizeY ) );

	// not initialized
	if ( heights.GetSizeX() == 0 || heights.GetSizeY() == 0 )
		return 0;
	//
	GetPoint4Spline( CVec2( x, y ), &u, &v, ptCtrls );
	// высоты разжимаются обратно, т.к. для сплайна даётся сетка с шагом 1 ( а не 2 * TILE_SIZE )
	// умножается на fAITileZCoeff1, чтобы перевести в AI высоты
	return betaSpline3D.Value( u, v, ptCtrls ) * 2.0f * nTileSize;
}

const void CStaticMapHeights::UpdateVisZ( CVec3 *pVec ) const
{
	NI_ASSERT( pVec != 0, "CStaticMapHeights::UpdateZ() Wrong parameter: pVec == 0" );
	pVec->z = GetVisZ( pVec->x, pVec->y );
}


const float CStaticMapHeights::GetTileHeight( const int nTileX, const int nTileY ) const 
{ 
	const int inMapX = Min( Max( 0, nTileX ), nStaticMapSizeX - 1 );
	const int inMapY = Min( Max( 0, nTileY ), nStaticMapSizeY - 1 );

	return tileHeights[inMapY][inMapX];
}


/**
const float CStaticMapHeights::GetVisHeight( const int nVisTileX, const int nVisTileY ) const
{
	const int inMapX = Min( Max( 1, nVisTileX + 1 ), heights.GetSizeX() - 2 );
	const int inMapY = Min( Max( 1, nVisTileY + 1 ), heights.GetSizeY() - 2 );

	return AI2Vis( heights[inMapY][inMapX] );
	//return Singleton<IScene>()->GetTerrain()->GetRealTerraHeight( nVisTileX * VIS_TILE_SIZE, nVisTileY * VIS_TILE_SIZE );
}
/**/


void CStaticMapHeights::Init( const int nSizeXInTiles, const int nSizeYInTiles, const int _nTileSize )
{
	const int nSizeX = nSizeXInTiles / 2 + 1;
	const int nSizeY = nSizeYInTiles / 2 + 1;
	nTileSize = _nTileSize;

	heights.SetSizes( nSizeX + 3, nSizeY + 3 );
	heights.FillEvery( 0.0f );

	betaSpline3D.Init( 1.0f, -1.0f );

	nStaticMapSizeX = nSizeXInTiles;
	nStaticMapSizeY = nSizeYInTiles;

	tileHeights.SetSizes( nStaticMapSizeX, nStaticMapSizeY );
	for ( int i = 0; i < nStaticMapSizeY; ++i )
	{
		for ( int j = 0; j < nStaticMapSizeX; ++j )
		{
			const CVec2 vTileCenter( j * nTileSize + nTileSize / 2, i * nTileSize + nTileSize / 2 );
			tileHeights[i][j] = GetVisZ( vTileCenter.x, vTileCenter.y );
		}
	}
	nLastHeightsID = 0;
	oldHeightsMap.clear();
}

CStaticMapHeights::CStaticMapHeights( const int nSizeXInTiles, const int nSizeYInTiles, const int nTileSize )
: nStaticMapSizeX( -1 ), nStaticMapSizeY( -1 ), nLastHeightsID( -1 ), nTileSize( -1 )
{
	Init( nSizeXInTiles, nSizeYInTiles, nTileSize );
}

void CStaticMapHeights::Init4Editor( const int nSizeXInTiles, const int nSizeYInTiles, const int nTileSize )
{
	Init( nSizeXInTiles, nSizeYInTiles, nTileSize );
}

void CStaticMapHeights::FinalizeUpdateHeights()
{
	FinalizeUpdateHeights( 0, 0, nStaticMapSizeX, nStaticMapSizeY );
}

void CStaticMapHeights::FinalizeUpdateHeights( const int nX1, const int nY1, const int nX2, const int nY2 )
{
	betaSpline3D.Init( 1.0f, -1.0f );

	const int nMinX = Max( 0, 2 * nX1 - 4 );
	const int nMaxX = Min( nStaticMapSizeX, 2 * nX2 + 4 );
	const int nMinY = Max( 0, 2 * nY1 - 4 );
	const int nMaxY = Min( nStaticMapSizeY, 2 * nY2 + 4 );

	for ( int i = nMinY; i < nMaxY; ++i )
	{
		for ( int j = nMinX; j < nMaxX; ++j )
		{
			const CVec2 vTileCenter( j * nTileSize + nTileSize / 2, i * nTileSize + nTileSize / 2 );
			tileHeights[i][j] = GetVisZ( vTileCenter.x, vTileCenter.y );
		}
	}
}

void CStaticMapHeights::UpdateHeights( const int nX1, const int nY1, const int nX2, const int nY2, const CArray2D<float> &initHeights )
{
	if ( initHeights.IsEmpty() ) 
		return;
	const int nInitSizeX = initHeights.GetSizeX();
	const int nInitSizeY = initHeights.GetSizeY();

	//heights.SetSizes( nInitSizeX + 2, nInitSizeY + 2 );
	/**
	DebugTrace( "CStaticMapHeights::UpdateHeights():[%d,%d,%d,%d] in (%dx%d) to [%d,%d,%d,%d] in(%dx%d)",
							nX1, nY1, nX2 - 1, nY2 - 1, nInitSizeX, nInitSizeY,
							nX1 + 1, nY1 + 1, nX2, nY2, heights.GetSizeX(), heights.GetSizeY() );
	/**/
	for ( int i = nY1; i < nY2; ++i )
	{
		for ( int j = nX1; j < nX2; ++j )
		{
			heights[i+1][j+1] = initHeights[i][j];
		}
	}

	if ( nY1 <= 0 )
	{
		/**
		DebugTrace( "CStaticMapHeights::UpdateHeights():[%d,%d,%d,%d] in (%dx%d) to [%d,%d,%d,%d] in(%dx%d)",
								0, nY1, nInitSizeX - 1, 0, nInitSizeX, nInitSizeY,
								1, 0, nInitSizeX, 0, heights.GetSizeX(), heights.GetSizeY() );
		/**/
		for ( int j = 0; j < nInitSizeX; ++j )
			heights[0][j+1] = initHeights[0][j];
	}

	if ( nY2 >= nInitSizeY - 1 ) 
	{
		/**
		DebugTrace( "CStaticMapHeights::UpdateHeights():[%d,%d,%d,%d] in (%dx%d) to [%d,%d,%d,%d] in(%dx%d)",
								0, nInitSizeY - 1, nInitSizeX - 1, nInitSizeY - 1, nInitSizeX, nInitSizeY,
								1, nInitSizeY + 1, nInitSizeX, nInitSizeY + 1, heights.GetSizeX(), heights.GetSizeY() );
		/**/
		for ( int j = 0; j < nInitSizeX; ++j )
			heights[nInitSizeY + 1][j + 1] = initHeights[nInitSizeY - 1][j];
	}
	if ( nX1 <= 0 )
	{
		/**
		DebugTrace( "CStaticMapHeights::UpdateHeights():[%d,%d,%d,%d] in (%dx%d) to [%d,%d,%d,%d] in(%dx%d)",
								0, 0, 0, nInitSizeY - 1, nInitSizeX, nInitSizeY,
								0, 1, 0, nInitSizeY, heights.GetSizeX(), heights.GetSizeY() );
		/**/
		for ( int i = 0; i < nInitSizeY; ++i )
			heights[i+1][0] = initHeights[i][0];
	}

	if ( nX2 >= nInitSizeX - 1 )
	{
		/**
		DebugTrace( "CStaticMapHeights::UpdateHeights():[%d,%d,%d,%d] in (%dx%d) to [%d,%d,%d,%d] in(%dx%d)",
								nInitSizeX - 1, 0, nInitSizeX - 1, nInitSizeY - 1, nInitSizeX, nInitSizeY,
								nInitSizeX + 1, 1, nInitSizeX + 1, nInitSizeY, heights.GetSizeX(), heights.GetSizeY() );
		/**/
		for ( int i = 0; i < nInitSizeY; ++i )
			heights[i+1][nInitSizeX + 1] = initHeights[i][nInitSizeX - 1];
	}

	//{ this part is quick, so may update all the time
	heights[0][0] = initHeights[0][0];
	heights[0][nInitSizeX + 1] = initHeights[0][nInitSizeX - 1];
	heights[nInitSizeY + 1][0] = initHeights[nInitSizeY - 1][0];
	heights[nInitSizeY + 1][nInitSizeX + 1] = initHeights[nInitSizeY - 1][nInitSizeX - 1];
	//}

}

int CStaticMapHeights::UpdateLocalHeights( const int nX1, const int nY1, const CArray2D<bool> &bridge, const float fBridgeHeight )
//int CStaticMapHeights::UpdateLocalHeights( const int nX1, const int nY1, const CArray2D<float> &initHeights )
{
	const int nVisX1 = (nX1 + 1)/2 + 1;
	const int nVisY1 = (nY1 + 1)/2 + 1;

	const int nVisHeightsSizeX = Min( ( bridge.GetSizeX() )/2 - nX1%2 + 1, heights.GetSizeX() - nVisX1 );
	const int nVisHeightsSizeY = Min( ( bridge.GetSizeY() )/2 - nY1%2 + 1, heights.GetSizeY() - nVisY1 );

	CArray2D<float> oldHeights;

#ifndef _FINALRELEASE
	vector<SVector> tiles;
#endif

	oldHeights.SetSizes( nVisHeightsSizeX, nVisHeightsSizeY );

	for ( int xVis = 0, nAIX = nX1%2; xVis < nVisHeightsSizeX; ++xVis, nAIX += 2 )
	{
		const int x1 = ( nAIX == 0 ) ? nAIX : nAIX - 1;
		const int x2 = ( nAIX == bridge.GetSizeX() ) ? nAIX - 1 : nAIX;
		for ( int yVis = 0, nAIY = nY1%2; yVis < nVisHeightsSizeY; ++yVis, nAIY += 2 )
		{
			const int y1 = ( nAIY == 0 ) ? nAIY : nAIY - 1;
			const int y2 = ( nAIY == bridge.GetSizeY() ) ? nAIY - 1 : nAIY;

			oldHeights[yVis][xVis] = heights[nVisY1 + yVis][nVisX1 + xVis];
			if ( bridge[y1][x1] || bridge[y2][x1] || bridge[y1][x2] || bridge[y2][x2] )
			{
				heights[nVisY1 + yVis][nVisX1 + xVis] = fBridgeHeight;

#ifndef _FINALRELEASE
				if ( NGlobal::GetVar( "bridge_vis_tile_markers", 0 ) )
				{
					SVector tile( 2*( nVisX1 + xVis - 1 ), 2*( nVisY1 + yVis - 1 ) );
					tiles.push_back( tile );
					tile.x++;
					tiles.push_back( tile );
					tile.y++;
					tiles.push_back( tile );
					tile.x--;
					tiles.push_back( tile );
				}
#endif
			}
		}
	}
	FinalizeUpdateHeights( nVisX1, nVisY1, nVisX1+nVisHeightsSizeX+1, nVisY1+nVisHeightsSizeY+1 );
	
#ifndef _FINALRELEASE
	if ( NGlobal::GetVar( "bridge_vis_tile_markers", 0 ) )
	{
		DebugInfoManager()->CreateMarker( NDebugInfo::OBJECT_ID_FORGET, tiles, NDebugInfo::RED );
		tiles.clear();

		for ( int x = 0; x < bridge.GetSizeX(); ++x )
			for ( int y = 0; y < bridge.GetSizeY(); ++y )
				if ( bridge[y][x] )
					tiles.push_back( SVector( nX1 + x, nY1 + y ) );

		DebugInfoManager()->CreateMarker( NDebugInfo::OBJECT_ID_FORGET, tiles, NDebugInfo::GREEN );
	}
#endif

	nLastHeightsID++;
	oldHeightsMap[nLastHeightsID] = SOldHeights( nVisX1, nVisY1, oldHeights );

	return nLastHeightsID;
}

void CStaticMapHeights::RestoreHeights( const int nID )
{
	CHeightsMap::iterator pos = oldHeightsMap.find( nID );
	if ( pos != oldHeightsMap.end() )
	{
		for ( int x = 0; x < pos->second.heights.GetSizeX(); ++x )
		{
			for ( int y = 0; y < pos->second.heights.GetSizeY(); ++y )
			{
				heights[pos->second.nY1 + y][pos->second.nX1 + x] = pos->second.heights[y][x];
			}
		}
		FinalizeUpdateHeights( pos->second.nX1, pos->second.nY1, pos->second.nX1+pos->second.heights.GetSizeX()+1, pos->second.nY1+pos->second.heights.GetSizeY()+1 );
		oldHeightsMap.erase( pos );
	}
}

bool CStaticMapHeights::GetLocalHeightsInfo( SVector *pvTopLeft, SVector *pvBottomRight, const int nID ) const
{
	if ( pvTopLeft )
	{
		pvTopLeft->x = 0;
		pvTopLeft->y = 0;
	}
	if ( pvBottomRight )
	{
		pvBottomRight->x = 0;
		pvBottomRight->y = 0;
	}
	CHeightsMap::const_iterator pos = oldHeightsMap.find( nID );
	if ( pos == oldHeightsMap.end() )
		return false;
	if ( pvTopLeft )
	{
		pvTopLeft->x = pos->second.nX1;
		pvTopLeft->y = pos->second.nY1;
	}
	if ( pvBottomRight )
	{
		pvBottomRight->x = pos->second.nX1 + pos->second.heights.GetSizeX();
		pvBottomRight->y = pos->second.nY1 + pos->second.heights.GetSizeY();
	}
	return true;
}

void CStaticMapHeights::SetHeightForPatternApplying( const int nX, const int nY, const float fHeight )
{
	heights[heights.GetSizeY() - 2 - nY][nX + 1] += fHeight;
}

const float CStaticMapHeights::GetHeight( const int x, const int y ) const
{
	const int x1 = Clamp( x, 1, heights.GetSizeX()-3 );
	const int y1 = Clamp( y, 1, heights.GetSizeY()-3 );

	return heights[y1][x1];
}

const bool CStaticMapHeights::GetIntersectionWithTerrain( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const
{
	// все происходит в Vis координатах
	const float maxX = (heights.GetSizeX() - 4) * 2.0f * nTileSize;
	const float maxY = (heights.GetSizeY() - 4) * 2.0f * nTileSize;

	const CVec2 vLineX = CVec2( vEnd.x - vBegin.x, vBegin.x );
	const CVec2 vLineY = CVec2( vEnd.y - vBegin.y, vBegin.y );
	const CVec2 vLineZ = CVec2( vEnd.z - vBegin.z, vBegin.z );

	float t01, t02;

	if ( vBegin.x < 0 )
		t01 = (vLineX[0] == 0) ? -1.0f : -vLineX[1]/vLineX[0];
	else if ( vBegin.x > maxX )
		t01 = (vLineX[0] == 0) ? -1.0f : ( maxX - vLineX[1] )/vLineX[0];
	else
		t01 = 0.0f;

	if ( vBegin.y < 0 )
		t02 = (vLineY[0] == 0) ? -1.0f : -vLineY[1]/vLineY[0];
	else if ( vBegin.y > maxY )
		t02 = (vLineY[0] == 0) ? -1.0f : ( maxY - vLineY[1] )/vLineY[0];
	else 
		t02 = 0.0f;
	const float t1 = Max( t01, t02 );

	if ( vEnd.x < 0 )
		t01 = (vLineX[0] == 0) ? 2.0f : -vLineX[1]/vLineX[0];
	else if ( vEnd.x > maxX )
		t01 = (vLineX[0] == 0) ? 2.0f : ( maxX - vLineX[1] )/vLineX[0];
	else 
		t01 = 1.0f;

	if ( vEnd.y < 0 )
		t02 = (vLineY[0] == 0) ? 2.0f : -vLineY[1]/vLineY[0];
	else if ( vEnd.y > maxY )
		t02 = (vLineY[0] == 0) ? 2.0f : ( maxY - vLineY[1] )/vLineY[0];
	else 
		t02 = 1.0f;
	const float t2 = Min( t01, t02 );

  // просматривать отрезок в интервале t1 .. t2, все что вне - лежит за пределами карты
	pvResult->x = 0.0f;
	pvResult->y = 0.0f;
	pvResult->z = 0.0f;

	if ( t1 >= 0.0f && t2 <= 1.0f && vBegin != vEnd )
	{
		// т - текущее пересечение луча и сетки, т0 - предыдущее
		float t = t1, t0 = t1;
		// зВ0 - высота по нашему лучу в точке т
		float zV0 = vLineZ[0] * t + vLineZ[1];
		// зТ0 - реальная высота в точке т
		float zT0 = GetVisZ( vLineX[0] * t + vLineX[1], vLineY[0] * t + vLineY[1] );
		float zV, zT;
		// дх, ду - направление движения по сетке
		const int dX = Sign( vEnd.x - vBegin.x );
		const int dY = Sign( vEnd.y - vBegin.y );
		// х, у - координаты ближайшего узла сетки высот
		int x = ((vLineX[0] * t + vLineX[1]) / nTileSize / 2.0f);
		int y = ((vLineY[0] * t + vLineY[1]) / nTileSize / 2.0f);
		//
		if ( (float)x >=  ((vLineX[0] * t + vLineX[1]) / nTileSize / 2.0f) && dX < 0)
			--x;
		if ( (float)y >=  ((vLineY[0] * t + vLineY[1]) / nTileSize / 2.0f) && dY < 0)
			--y;

		t01 = t02 = 1.1f; // something more than 1.0f
		while ( t < t2 )
		{
			t0 = t;
			if ( dX > 0 )
				t01 = ( ((x + 1) * nTileSize * 2.0f - vLineX[1])/vLineX[0] );
			else if ( dX < 0 )
				t01 = ( (x * nTileSize * 2.0f - vLineX[1])/vLineX[0] );

			if ( dY > 0 )
				t02 = ( ((y + 1) * nTileSize * 2.0f - vLineY[1])/vLineY[0] );
			else if ( dY < 0 )
				t02 = ( (y * nTileSize * 2.0f - vLineY[1])/vLineY[0] );

			if ( dX == 0 || (t02 < t01 && t02 != t0) || t01 == t0 )
			{
				t = t02;
				y += dY;
			}
			else
			{
				t = t01;
				x += dX;
			}

			if ( t > 1.0f )
				t = 1.0f;

			const float fX = vLineX[0] * t + vLineX[1];
			const float fY = vLineY[0] * t + vLineY[1];
			zT = GetVisZ( fX, fY );

			zV = vLineZ[0] * t + vLineZ[1];

			const float tX = t - ( zV - zT )*( t - t0 )/( zV - zT - ( zV0 - zT0 ) );			
			if ( t0 <= tX && tX <= t )
			{
				pvResult->x = vLineX[0] * tX + vLineX[1];
				pvResult->y = vLineY[0] * tX + vLineY[1];
				pvResult->z = GetVisZ( pvResult->x, pvResult->y );
#ifndef _FINALRELEASE
				if ( NGlobal::GetVar( "show_pick_terrain_marker", 0 ) )
				{
					if ( t1 <= tX && tX <= t2 )
						DebugInfoManager()->DrawLine( 78577, NDebugInfo::SArrowHead( CVec3( pvResult->x, pvResult->y, 0.0f ), 20.0f, 10.0f), NDebugInfo::SArrowHead( *pvResult, 20, 10 ), NDebugInfo::GREEN );
					else
						DebugInfoManager()->DrawLine( 78577, NDebugInfo::SArrowHead( CVec3( pvResult->x, pvResult->y, 0.0f ), 20.0f, 10.0f), NDebugInfo::SArrowHead( *pvResult, 20, 10 ), NDebugInfo::RED );
				}
#endif
				return ( t1 <= tX && tX <= t2 );
			}

			zV0 = zV;
			zT0 = zT;
		}
	}

	const float t = -vLineZ[1]/vLineZ[0];
	pvResult->x = Clamp( vLineX[0] * t + vLineX[1], 0.0f, maxX );
	pvResult->y = Clamp( vLineY[0] * t + vLineY[1], 0.0f, maxY );
	pvResult->z = 0.0f;

	return false;
}

const bool CStaticMapHeights::GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const
{
	const float maxX = (heights.GetSizeX() - 4) * 2.0f * nTileSize;
	const float maxY = (heights.GetSizeY() - 4) * 2.0f * nTileSize;

	const CVec2 vLineX = CVec2( vEnd.x - vBegin.x, vBegin.x );
	const CVec2 vLineY = CVec2( vEnd.y - vBegin.y, vBegin.y );
	const CVec2 vLineZ = CVec2( vEnd.z - vBegin.z, vBegin.z );

	float t01, t02;

	if ( vBegin.x < 0 )
		t01 = (vLineX[0] == 0) ? -1.0f : -vLineX[1]/vLineX[0];
	else if ( vBegin.x > maxX )
		t01 = (vLineX[0] == 0) ? -1.0f : ( maxX - vLineX[1] )/vLineX[0];
	else
		t01 = 0.0f;

	if ( vBegin.y < 0 )
		t02 = (vLineY[0] == 0) ? -1.0f : -vLineY[1]/vLineY[0];
	else if ( vBegin.y > maxY )
		t02 = (vLineY[0] == 0) ? -1.0f : ( maxY - vLineY[1] )/vLineY[0];
	else 
		t02 = 0.0f;
	const float t1 = Max( t01, t02 );

	if ( vEnd.x < 0 )
		t01 = (vLineX[0] == 0) ? 2.0f : -vLineX[1]/vLineX[0];
	else if ( vEnd.x > maxX )
		t01 = (vLineX[0] == 0) ? 2.0f : ( maxX - vLineX[1] )/vLineX[0];
	else 
		t01 = 1.0f;

	if ( vEnd.y < 0 )
		t02 = (vLineY[0] == 0) ? 2.0f : -vLineY[1]/vLineY[0];
	else if ( vEnd.y > maxY )
		t02 = (vLineY[0] == 0) ? 2.0f : ( maxY - vLineY[1] )/vLineY[0];
	else 
		t02 = 1.0f;
	const float t2 = Min( t01, t02 );

	pvResult->x = 0.0f;
	pvResult->y = 0.0f;
	pvResult->z = 0.0f;

	if ( vBegin != vEnd )
	{
		float t = 0.0f, t0 = 0.0f;
		float zV0 = vLineZ[0] * t + vLineZ[1];
		float zT0 = GetVisZ( vLineX[0] * t + vLineX[1], vLineY[0] * t + vLineY[1] );
		float zV, zT;
		int x = ((vLineX[0] * t + vLineX[1]) / nTileSize / 2.0f);
		int y = ((vLineY[0] * t + vLineY[1]) / nTileSize / 2.0f);
		const int dX = Sign( vEnd.x - vBegin.x );
		const int dY = Sign( vEnd.y - vBegin.y );
		t01 = t02 = 1.1f; // something more than 1.0f
		while ( t < 1.0f )
		{
			t0 = t;
			if ( dX > 0 )
				t01 = ( ((x + 1) * nTileSize * 2.0f - vLineX[1])/vLineX[0] );
			else if ( dX < 0 )
				t01 = ( (x * nTileSize * 2.0f - vLineX[1])/vLineX[0] );

			if ( dY > 0 )
				t02 = ( ((y + 1) * nTileSize * 2.0f - vLineY[1])/vLineY[0] );
			else if ( dY < 0 )
				t02 = ( (y * nTileSize * 2.0f - vLineY[1])/vLineY[0] );

			if ( dX == 0 || (t02 < t01 && t02 != t0) || t01 == t0 )
			{
				t = t02;
				y += dY;
			}
			else
			{
				t = t01;
				x += dX;
			}

			if ( t > 1.0f )
				t = 1.0f;

			const float fX = vLineX[0] * t + vLineX[1];
			const float fY = vLineY[0] * t + vLineY[1];
			zT = GetVisZ( fX, fY );

			zV = vLineZ[0] * t + vLineZ[1];

			const float tX = t - ( zV - zT )*( t - t0 )/( zV - zT - ( zV0 - zT0 ) );
			if ( t0 <= tX && tX <= t )
			{
				pvResult->x = vLineX[0] * tX + vLineX[1];
				pvResult->y = vLineY[0] * tX + vLineY[1];
				pvResult->z = GetVisZ( pvResult->x, pvResult->y );
#ifndef _FINALRELEASE
				if ( NGlobal::GetVar( "show_pick_terrain_marker", 0 ) )
				{
					if ( t1 <= tX && tX <= t2 )
						DebugInfoManager()->DrawLine( 78577, NDebugInfo::SArrowHead( CVec3( pvResult->x, pvResult->y, 0.0f ), 20.0f, 10.0f), NDebugInfo::SArrowHead( *pvResult, 20, 10 ), NDebugInfo::GREEN );
					else
						DebugInfoManager()->DrawLine( 78577, NDebugInfo::SArrowHead( CVec3( pvResult->x, pvResult->y, 0.0f ), 20.0f, 10.0f), NDebugInfo::SArrowHead( *pvResult, 20, 10 ), NDebugInfo::RED );
				}
#endif
				return ( t1 <= tX && tX <= t2 );
			}

			zV0 = zV;
			zT0 = zT;
		}
	}

	return false;
}

REGISTER_SAVELOAD_CLASS( 0x3015A480, CStaticMapHeights )

