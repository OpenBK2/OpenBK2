#include "stdafx.h"

#include "Misc/Win32Random.h"
#include "GenTerrain.h"

#include "System/Commands.h"

#define DEF_EXPLOSION_HEIGHT_BASE 0.125f
#define DEF_EXPLOSION_HEIGHT 0.025f

static int s_nMaxExplosionsPerTile = 10;

void CTerraGen::CreateExplosionData( NMeshData::SMeshData *pData, const CVec2 &vMin, const CVec2 &vMax,
																		 const int nTileX1, const int nTileY1, const int nTileX2, const int nTileY2,
																		 const int nIncHeight )
{
	pData->vertices.reserve( 128 );
	pData->vertices.resize( 0 );
	pData->triangles.reserve( 64 );
	pData->triangles.resize( 0 );

	const float fSizeXInv = 1.0f / ( vMax.x - vMin.x );
	const float fSizeYInv = 1.0f / ( vMax.y - vMin.y );

	const float fTexX1 = ( (float)nTileX1 * DEF_TILE_SIZE - vMin.x ) * fSizeXInv;
	const float fTexY1 = ( (float)nTileY1 * DEF_TILE_SIZE - vMin.y ) * fSizeYInv;
	const float fTexX2 = ( (float)( nTileX2 + 1 ) * DEF_TILE_SIZE - vMax.x ) * fSizeXInv + 1.0f;
	const float fTexY2 = ( (float)( nTileY2 + 1 ) * DEF_TILE_SIZE - vMax.y ) * fSizeYInv + 1.0f;

	const float fCoeffX = ( fTexX2 - fTexX1 ) * DEF_INV_TILE_SIZE / ( nTileX2 - nTileX1 + 1 );
	const float fCoeffY = ( fTexY2 - fTexY1 ) * DEF_INV_TILE_SIZE / ( nTileY2 - nTileY1 + 1 );

	const float fOrgX = (float)nTileX1 * DEF_TILE_SIZE;
	const float fOrgY = (float)nTileY1 * DEF_TILE_SIZE;

	NGScene::SVertex vert;
	CalcCompactVector( &(vert.normal), CVec3(0, 0, 1) );
	CalcCompactVector( &(vert.texU), CVec3(1, 0, 0) );
	CalcCompactVector( &(vert.texV), CVec3(0, 1, 0) );

	int nVertOffs = 0;
	STriangle trg;

	const int nRotateType = NWin32Random::Random( 0, 3 );

	for ( int g = nTileY1; g <= nTileY2; ++g )
	{
		for ( int i = nTileX1; i <= nTileX2; ++i )
		{
			for ( vector<STriangle>::const_iterator it = terrainInfo.tiles[g][i].triangles.begin(); it != terrainInfo.tiles[g][i].triangles.end(); ++it )
			{
				trg.i1 = nVertOffs + it->i1;
				trg.i2 = nVertOffs + it->i2;
				trg.i3 = nVertOffs + it->i3;
				pData->triangles.push_back( trg );
			}
			vector<CVec3fEx>::const_iterator itVert = terrainInfo.tiles[g][i].vertices.begin();
			vector<float>::const_iterator itHeight = terrainInfo.tiles[g][i].addHeights.begin();
			for ( ; itVert != terrainInfo.tiles[g][i].vertices.end(); ++itVert, ++itHeight )
			{
				vert.pos.Set( itVert->x, itVert->y, max( itVert->z + ( *itHeight ), 0.0f ) + DEF_EXPLOSION_HEIGHT_BASE + DEF_EXPLOSION_HEIGHT * nIncHeight );
				const float tx = fTexX1 + ( itVert->x - fOrgX ) * fCoeffX;
				const float ty = fTexY1 + ( itVert->y - fOrgY ) * fCoeffY;
				vert.tex.Set( tx, ty );
				switch ( nRotateType )
				{
				case 1:
					vert.tex.Set( ty, ( 1.0f - tx ) );
					break;
				case 2:
					vert.tex.Set( ( 1.0f - tx ), ( 1.0f - ty) );
					break;
				case 3:
					vert.tex.Set( ( 1.0f - ty ), tx );
					break;
				}
				pData->vertices.push_back( vert );
				++nVertOffs;
			}
		}
	}
}

void CTerraGen::AddExplosion( const CVec2 &_vMin, const CVec2 &_vMax, const NDb::SMaterial *pMaterial, const bool bWriteHistory/* = true*/ )
{
	if ( bWriteHistory )
	{
		SExplosionHistory history;
		history.vMin = _vMin;
		history.vMax = _vMax;
		history.pMaterial = pMaterial;
		explosionsHistory.push_back( history );
	}

	CVec2 vMin = _vMin, vMax = _vMax;
	AI2Vis( &vMin );
	AI2Vis( &vMax );

	const int nTileX1 = Clamp( int(vMin.x * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY1 = Clamp( int(vMin.y * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );
	const int nTileX2 = Clamp( int(vMax.x * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY2 = Clamp( int(vMax.y * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );

	const int nCenterX = Clamp( int( ( vMin.x + vMax.x ) * 0.5f * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nCenterY = Clamp( int( ( vMin.y + vMax.y ) * 0.5f * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeY() - 1 );

	if ( ++terraExplosionsHeights[nCenterY][nCenterX] >= s_nMaxExplosionsPerTile )
		terraExplosionsHeights[nCenterY][nCenterX] = 0;

	NMeshData::SMeshData data;
	CreateExplosionData( &data, vMin, vMax, nTileX1, nTileY1, nTileX2, nTileY2, terraExplosionsHeights[nCenterY][nCenterX] );
	const int nID = explosionsManager.AddExplosion( data, pMaterial );

	for ( int nX = nTileX1; nX <= nTileX2; ++nX )
		for ( int nY = nTileY1; nY <= nTileY2; ++nY )
		{
			vector<int> &idArr = terraExplosionsHash[nY][nX];
			idArr.resize( s_nMaxExplosionsPerTile, -1 );
			explosionsManager.RemoveExplosion( idArr[s_nMaxExplosionsPerTile - 1] );
			for ( int k = s_nMaxExplosionsPerTile - 1; k >= 1; --k )
				idArr[k] = idArr[k - 1];
			idArr[0] = nID;
		}
}


START_REGISTER(ExplosionsBuiler)
REGISTER_VAR_EX( "misc_max_explosions_per_tile", NGlobal::VarIntHandler, &s_nMaxExplosionsPerTile, 10, STORAGE_USER )
FINISH_REGISTER

