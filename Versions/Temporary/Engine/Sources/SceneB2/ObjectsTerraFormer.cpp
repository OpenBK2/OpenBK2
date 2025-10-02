#include "StdAfx.h"

#include "TerraTools.h"
#include "../System/FastMath.h"
#include "GenTerrain.h"

#define DEF_EXPAND_TILES_NUM 3
#define DEF_EXPAND_TILES_NUM_EXT ( int( FP_SQRT_2 * DEF_EXPAND_TILES_NUM ) )
#define DEF_CONST_TILES_NUM 1
#define DEF_CONST_DIST ( DEF_TILE_SIZE * FP_SQRT_2 * DEF_CONST_TILES_NUM )
#define DEF_NON_CONST_DIST ( DEF_TILE_SIZE * FP_SQRT_2 * DEF_EXPAND_TILES_NUM - DEF_CONST_DIST )
#define DEF_INV_INTERPOLATE_DIST ( 1.0f / DEF_NON_CONST_DIST )

inline float GetObjTerraProfile( const float x )
{
	return 0.5f + NMath::Sin( x * FP_PI - FP_PI2 ) * 0.5f;
}

void CTerraGen::ApplyObjectTerraForm( const CVec2 &_p1, const CVec2 &_p2, const CVec2 &_p3, const CVec2 &_p4 )
{
	const CVec2 p1( AI2Vis(_p1.x), AI2Vis(_p1.y) );
	const CVec2 p2( AI2Vis(_p2.x), AI2Vis(_p2.y) );
	const CVec2 p3( AI2Vis(_p3.x), AI2Vis(_p3.y) );
	const CVec2 p4( AI2Vis(_p4.x), AI2Vis(_p4.y) );

	const CVec2i v1( int(p1.x * DEF_INV_TILE_SIZE), int(p1.y * DEF_INV_TILE_SIZE) );
	const CVec2i v2( int(p2.x * DEF_INV_TILE_SIZE), int(p2.y * DEF_INV_TILE_SIZE) );
	const CVec2i v3( int(p3.x * DEF_INV_TILE_SIZE), int(p3.y * DEF_INV_TILE_SIZE) );
	const CVec2i v4( int(p4.x * DEF_INV_TILE_SIZE), int(p4.y * DEF_INV_TILE_SIZE) );

	const CVec2 vt1( (float)v1.x * DEF_TILE_SIZE, (float)v1.y * DEF_TILE_SIZE );
	const CVec2 vt2( (float)v2.x * DEF_TILE_SIZE, (float)v2.y * DEF_TILE_SIZE );
	const CVec2 vt3( (float)v3.x * DEF_TILE_SIZE, (float)v3.y * DEF_TILE_SIZE );
	const CVec2 vt4( (float)v4.x * DEF_TILE_SIZE, (float)v4.y * DEF_TILE_SIZE );

	const CVec2i vMin( min(min( v1.x, v2.x), min(v3.x, v4.x)) - DEF_EXPAND_TILES_NUM_EXT,
										 min(min( v1.y, v2.y), min(v3.y, v4.y)) - DEF_EXPAND_TILES_NUM_EXT );
	const CVec2i vMax( max(max( v1.x, v2.x), max(v3.x, v4.x)) + DEF_EXPAND_TILES_NUM_EXT,
										 max(max( v1.y, v2.y), max(v3.y, v4.y)) + DEF_EXPAND_TILES_NUM_EXT );

	CArray2D<BYTE> objMask( vMax.x - vMin.x + 1, vMax.y - vMin.y + 1 );
	objMask.FillZero();

	DrawRect( &objMask, v1.x - vMin.x, v1.y - vMin.y, v2.x - vMin.x, v2.y - vMin.y, v3.x - vMin.x, v3.y - vMin.y,
		v4.x - vMin.x, v4.y - vMin.y, 1 );

	WiseFill( &objMask, 1 );

	SmoothColor( &objMask, DEF_EXPAND_TILES_NUM, 1, 0xff );

	const int nFirstX = vMin.x >= 0 ? 0 : -vMin.x;
	const int nFirstY = vMin.y >= 0 ? 0 : -vMin.y;
	const int nLastX = ( vMin.x + objMask.GetSizeX() ) <= terrainInfo.heights.GetSizeX() ? objMask.GetSizeX() :
										 ( terrainInfo.heights.GetSizeX() - vMin.x );
	const int nLastY = ( vMin.y + objMask.GetSizeY() ) <= terrainInfo.heights.GetSizeY() ? objMask.GetSizeY() :
										 ( terrainInfo.heights.GetSizeY() - vMin.y );

	float fMinHeight = FP_MAX_VALUE;
	bool bFlag = false;

	// find min height
	for ( int g = nFirstY; g < nLastY; ++g )
	{
		for ( int i = nFirstX; i < nLastX; ++i )
		{
			if ( objMask[g][i] == 1 )
			{
				fMinHeight = min( fMinHeight, terrainInfo.heights[g + vMin.y][i + vMin.x] );
				bFlag = true;
			}
		}
	}

	if ( !bFlag )
		return;

	for ( int g = nFirstY; g < nLastY; ++g )
	{
		for ( int i = nFirstX; i < nLastX; ++i )
		{
			const int nIndX = i + vMin.x;
			const int nIndY = g + vMin.y;
			if ( objMask[g][i] == 1 ) // const height
			{
				terrainInfo.heights[nIndY][nIndX] = fMinHeight;
				terrainAIInfo.heights[nIndY][nIndX] = Vis2AIFast( terrainInfo.heights[nIndY][nIndX] );
			}
			else
			{
				if ( objMask[g][i] == 0xff ) // interpolate height
				{
					const float fX = (float)nIndX * DEF_TILE_SIZE;
					const float fY = (float)nIndY * DEF_TILE_SIZE;
					const float fDist1 = GetDistToSegment( CVec2(fX, fY), vt1, vt2 );
					const float fDist2 = GetDistToSegment( CVec2(fX, fY), vt2, vt3 );
					const float fDist3 = GetDistToSegment( CVec2(fX, fY), vt3, vt4 );
					const float fDist4 = GetDistToSegment( CVec2(fX, fY), vt4, vt1 );

					const float fDist = sqrt( min(min(fDist1, fDist2), min(fDist3, fDist4)) );
					const float fCoeff = GetObjTerraProfile( Clamp((fDist - DEF_CONST_DIST) * DEF_INV_INTERPOLATE_DIST, 0.0f, 1.0f) );
					terrainInfo.heights[nIndY][nIndX] = terrainInfo.heights[nIndY][nIndX] * fCoeff + fMinHeight * ( 1.0f - fCoeff );
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


