#include "StdAfx.h"

#include "GenTerrain.h"
#include "TerrainManager.h"

#define DEF_SELECTION_HEIGHT 0.25f

void CTerraGen::CreateSelection( NMeshData::SMeshData *pData, const CVec3 &vMin, const CVec3 &vMax, CTerrainManager *pManager )
{
	ASSERT( pManager );

	pData->vertices.reserve( 128 );
	pData->vertices.resize( 0 );
	pData->triangles.reserve( 64 );
	pData->triangles.resize( 0 );

	const float fSizeXInv = 1.0f / ( vMax.x - vMin.x );
	const float fSizeYInv = 1.0f / ( vMax.y - vMin.y );

	const int nTileX1 = Clamp( int(vMin.x * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY1 = Clamp( int(vMin.y * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );
	const int nTileX2 = Clamp( int(vMax.x * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY2 = Clamp( int(vMax.y * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );

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
	vert.texU.dw = 0xffffffff;
	vert.texV.dw = 0xffffffff;
	//CalcCompactVector( &(vert.texU), CVec3(1, 0, 0) );
	//CalcCompactVector( &(vert.texV), CVec3(0, 1, 0) );

	int nVertOffs = 0;
	STriangle trg;

	float fMinHeight = 0.0f;
	int nMiddleX = (nTileX1 + nTileX2)/2;
	int nMiddleY = (nTileY1 + nTileY2)/2;

	float fTileAIHeight = AI2Vis( pManager->GetTileHeight( nMiddleX*AI_TILES_IN_VIS_TILE + 1, nMiddleY*AI_TILES_IN_VIS_TILE + 1 ) );
	float fTileHeight = -1.0f;
	if ( !terrainInfo.tiles[nMiddleY][nMiddleX].vertices.empty() )
		fTileHeight = terrainInfo.tiles[nMiddleY][nMiddleX].vertices[0].z;

	if ( fTileAIHeight > fTileHeight + 1.0f )
	{
		// AI objects such as bridges affects on heights too, and we should draw selections with considiration of it
		fMinHeight = fTileAIHeight;
	}

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
				vert.pos.Set( itVert->x, itVert->y, max( itVert->z + ( *itHeight ), fMinHeight ) + DEF_SELECTION_HEIGHT );
				vert.tex.Set( fTexX1 + ( itVert->x - fOrgX ) * fCoeffX, fTexY1 + ( itVert->y - fOrgY ) * fCoeffY );
				pData->vertices.push_back( vert );
				++nVertOffs;
			}
		}
	}
}


