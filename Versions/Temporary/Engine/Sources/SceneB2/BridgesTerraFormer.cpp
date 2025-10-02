#include "StdAfx.h"

#include "../System/FastMath.h"
#include "TerraTools.h"
#include "GenTerrain.h"

#define DEF_EXPAND_TILES_NUM 7
#define DEF_EXPAND_TILES_NUM_EXT ( int( FP_SQRT_2 * DEF_EXPAND_TILES_NUM ) )
#define DEF_CONST_TILES_NUM 1
#define DEF_CONST_DIST ( DEF_TILE_SIZE * FP_SQRT_2 * DEF_CONST_TILES_NUM )
#define DEF_NON_CONST_DIST ( DEF_TILE_SIZE * FP_SQRT_2 * DEF_EXPAND_TILES_NUM - DEF_CONST_DIST )
#define DEF_INV_INTERPOLATE_DIST ( 1.0f / DEF_NON_CONST_DIST )

inline float GetBridgeTerraProfile( const float x )
{
	return 0.5f + NMath::Sin( x * FP_PI - FP_PI2 ) * 0.5f;
}

void CTerraGen::ApplyBridgeTerraForm( const CVec2 &_p1, const CVec2 &_p2, const float fWidth, const float fHeight )
{
	const CVec2 p1( AI2Vis(_p1.x), AI2Vis(_p1.y) );
	const CVec2 p2( AI2Vis(_p2.x), AI2Vis(_p2.y) );

	CVec2 vNorm( p2.y - p1.y, p1.x - p2.x );

	NI_ASSERT( fabs2(vNorm) > DEF_EPS, "Wrong bridge's stayed points" );
	if ( fabs2(vNorm) <= DEF_EPS )
		return;

	Normalize( &vNorm );

	const float fBridgeHeight = AI2Vis( fHeight );
	const float fWidthHalf = AI2Vis( fWidth ) * 0.5f;

	const CVec2 vNode1( p1.x - vNorm.x * fWidthHalf, p1.y - vNorm.y * fWidthHalf );
	const CVec2 vNode2( p1.x + vNorm.x * fWidthHalf, p1.y + vNorm.y * fWidthHalf );
	const CVec2 vNode3( p2.x + vNorm.x * fWidthHalf, p2.y + vNorm.y * fWidthHalf );
	const CVec2 vNode4( p2.x - vNorm.x * fWidthHalf, p2.y - vNorm.y * fWidthHalf );

	const CVec2i v1( int(vNode1.x * DEF_INV_TILE_SIZE), int(vNode1.y * DEF_INV_TILE_SIZE) );
	const CVec2i v2( int(vNode2.x * DEF_INV_TILE_SIZE), int(vNode2.y * DEF_INV_TILE_SIZE) );
	const CVec2i v3( int(vNode3.x * DEF_INV_TILE_SIZE), int(vNode3.y * DEF_INV_TILE_SIZE) );
	const CVec2i v4( int(vNode4.x * DEF_INV_TILE_SIZE), int(vNode4.y * DEF_INV_TILE_SIZE) );

	const CVec2 vt1( (float)v1.x * DEF_TILE_SIZE, (float)v1.y * DEF_TILE_SIZE );
	const CVec2 vt2( (float)v2.x * DEF_TILE_SIZE, (float)v2.y * DEF_TILE_SIZE );
	const CVec2 vt3( (float)v3.x * DEF_TILE_SIZE, (float)v3.y * DEF_TILE_SIZE );
	const CVec2 vt4( (float)v4.x * DEF_TILE_SIZE, (float)v4.y * DEF_TILE_SIZE );

	const CVec2i vMin( min(min(v1.x, v2.x), min(v3.x, v4.x)) - DEF_EXPAND_TILES_NUM_EXT,
										 min(min(v1.y, v2.y), min(v3.y, v4.y)) - DEF_EXPAND_TILES_NUM_EXT );
	const CVec2i vMax( max(max(v1.x, v2.x), max(v3.x, v4.x)) + DEF_EXPAND_TILES_NUM_EXT,
										 max(max(v1.y, v2.y), max(v3.y, v4.y)) + DEF_EXPAND_TILES_NUM_EXT );

	CArray2D<BYTE> bridgeMask( vMax.x - vMin.x + 1, vMax.y - vMin.y + 1 );
	bridgeMask.FillZero();

	DrawRect( &bridgeMask, v1.x - vMin.x, v1.y - vMin.y, v2.x - vMin.x, v2.y - vMin.y, v3.x - vMin.x, v3.y - vMin.y, v4.x - vMin.x, v4.y - vMin.y, 1 );

	WiseFill( &bridgeMask, 1 );

	SmoothColor( &bridgeMask, DEF_EXPAND_TILES_NUM, 1, 0xff );
	ReplaceColor( &bridgeMask, 1, 0xff );

	DrawLine( &bridgeMask, v1.x - vMin.x, v1.y - vMin.y, v2.x - vMin.x, v2.y - vMin.y, 1 );
	DrawLine( &bridgeMask, v3.x - vMin.x, v3.y - vMin.y, v4.x - vMin.x, v4.y - vMin.y, 1 );

	SmoothColor( &bridgeMask, DEF_CONST_TILES_NUM, 1, 2 );
	ReplaceColor( &bridgeMask, 2, 1 );

	const int nFirstX = vMin.x >= 0 ? 0 : -vMin.x;
	const int nFirstY = vMin.y >= 0 ? 0 : -vMin.y;
	const int nLastX = ( vMin.x + bridgeMask.GetSizeX() ) <= terrainInfo.heights.GetSizeX() ? bridgeMask.GetSizeX() :
										 ( terrainInfo.heights.GetSizeX() - vMin.x );
	const int nLastY = ( vMin.y + bridgeMask.GetSizeY() ) <= terrainInfo.heights.GetSizeY() ? bridgeMask.GetSizeY() :
										 ( terrainInfo.heights.GetSizeY() - vMin.y );

	for ( int g = nFirstY; g < nLastY; ++g )
	{
		for ( int i = nFirstX; i < nLastX; ++i )
		{
			const int nIndX = i + vMin.x;
			const int nIndY = g + vMin.y;
			if ( bridgeMask[g][i] == 1 ) // const height
			{
				terrainInfo.heights[nIndY][nIndX] = fBridgeHeight;
				terrainAIInfo.heights[nIndY][nIndX] = fHeight;
			}
			else
			{
				if ( bridgeMask[g][i] == 0xff ) // interpolate height
				{
					const float fX = (float)nIndX * DEF_TILE_SIZE;
					const float fY = (float)nIndY * DEF_TILE_SIZE;
					const float fDist1 = GetDistToSegment( CVec2(fX, fY), vt1, vt2 );
					const float fDist2 = GetDistToSegment( CVec2(fX, fY), vt3, vt4 );

					const float fDist = sqrt( min(fDist1, fDist2) );
					const float fCoeff = GetBridgeTerraProfile( Clamp((fDist - DEF_CONST_DIST) * DEF_INV_INTERPOLATE_DIST, 0.0f, 1.0f) );
					terrainInfo.heights[nIndY][nIndX] = min( terrainInfo.heights[nIndY][nIndX] * fCoeff + fBridgeHeight * (1.0f - fCoeff), fBridgeHeight );
					terrainAIInfo.heights[nIndY][nIndX] = Vis2AIFast( terrainInfo.heights[nIndY][nIndX] );
				}
			}
		}
	}

	const CVec2i vMinTile( Clamp(nFirstX + vMin.x - 1, 0, terrainInfo.tiles.GetSizeX() - 1),
												 Clamp(nFirstY + vMin.y - 1, 0, terrainInfo.tiles.GetSizeY() - 1) );
	const CVec2i vMaxTile( Clamp(nLastX + vMin.x + 1, 0, terrainInfo.tiles.GetSizeX() - 1),
												 Clamp(nLastY + vMin.y + 1, 0, terrainInfo.tiles.GetSizeY() - 1) );

	UpdateVectorAreaInfo( vMinTile.x, vMinTile.y, vMaxTile.x, vMaxTile.y, TERRAIN_UPDATE_ALL );
}


