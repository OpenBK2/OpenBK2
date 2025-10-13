#include "stdafx.h"

#include <Limits.h>
#include "GenTerrain.h"
#include "Misc/HPTimer.h"

#include <cstdint>

void CTerraGen::GenerateTerrain()
{
	TIME_STAT_START( GenerateTerraGeometry )

	terrainInfo.heights.SetSizes( pDesc->nNumPatchesX * DEF_PATCH_SIZE + 1, pDesc->nNumPatchesY * DEF_PATCH_SIZE + 1 );
	terrainInfo.heights.FillEvery( DEF_TERRA_HEIGHT );
	//	
	terrainInfo.addHeights.SetSizes( terrainInfo.heights.GetSizeX(), terrainInfo.heights.GetSizeY() );
	terrainInfo.addHeights.FillEvery( 0.0f );
	//	
	terrainInfo.waterAddHeights.SetSizes( terrainInfo.heights.GetSizeX(), terrainInfo.heights.GetSizeY() );
	terrainInfo.waterAddHeights.FillEvery( 0.0f );
	//
	terrainInfo.riverHeights.SetSizes( terrainInfo.heights.GetSizeX(), terrainInfo.heights.GetSizeY() );
	terrainInfo.riverHeights.FillEvery( 0.0f );
	//
	terrainAIInfo.terrTypes.SetSizes( terrainInfo.tiles.GetSizeX(), terrainInfo.tiles.GetSizeY() );
	terrainAIInfo.terrTypes.FillZero();
	//
	terrainAIInfo.heights.SetSizes( terrainInfo.heights.GetSizeX(), terrainInfo.heights.GetSizeY() );
	terrainAIInfo.heights.FillEvery( DEF_TERRA_HEIGHT_AI );
	//
	terrainNorms.SetSizes( terrainInfo.heights.GetSizeX(), terrainInfo.heights.GetSizeY() );
	terrainNorms.FillEvery( V3_AXIS_Z );

	std::vector<NDb::STerrainAIProperties> params;
	for ( std::vector< CDBPtr<NDb::STGTerraType> >::const_iterator it = pDesc->pTerraSet->terraTypes.begin(); it != pDesc->pTerraSet->terraTypes.end(); ++it )
		params.push_back( (*it)->aIProperty );

	SetTerraTypesToAI( params );

	terrainInfo.nDescID = pDesc->GetRecordID();
	terrainInfo.nNumPatchesX = pDesc->nNumPatchesX;
	terrainInfo.nNumPatchesY = pDesc->nNumPatchesY;
	terrainInfo.tiles.SetSizes( pDesc->nNumPatchesX * DEF_PATCH_SIZE, pDesc->nNumPatchesY * DEF_PATCH_SIZE );

	terrainInfo.nRecreateRandSeed = GetTickCount();

	TIME_STAT_FINISH( GenerateTerraGeometry )
}


void CTerraGen::ResetTile( const int nTileX, const int nTileY )
{
	const float x = Tile2Vis( nTileX );
	const float y = Tile2Vis( nTileY );
	STerrainInfo::STile &tile = terrainInfo.tiles[nTileY][nTileX];

	tile.vertices.resize( 0 );
	tile.vertices.reserve( 4 );
	tile.vertices.resize( 0 );
	tile.vertices.push_back( CVec3fEx(x, y, terrainInfo.heights[nTileY][nTileX], 0) );
	tile.vertices.push_back( CVec3fEx(x + DEF_TILE_SIZE, y, terrainInfo.heights[nTileY][nTileX + 1], 0) );
	tile.vertices.push_back( CVec3fEx(x + DEF_TILE_SIZE, y + DEF_TILE_SIZE, terrainInfo.heights[nTileY + 1][nTileX + 1], 0) );
	tile.vertices.push_back( CVec3fEx(x, y + DEF_TILE_SIZE, terrainInfo.heights[nTileY + 1][nTileX], 0) );
	tile.addHeights.resize( 4 );

	tile.addHeights[0] = terrainInfo.riverHeights[nTileY][nTileX] + terrainInfo.addHeights[nTileY][nTileX];
	tile.addHeights[1] = terrainInfo.riverHeights[nTileY][nTileX + 1] + terrainInfo.addHeights[nTileY][nTileX + 1];
	tile.addHeights[2] = terrainInfo.riverHeights[nTileY + 1][nTileX + 1] + terrainInfo.addHeights[nTileY + 1][nTileX + 1];
	tile.addHeights[3] = terrainInfo.riverHeights[nTileY + 1][nTileX] + terrainInfo.addHeights[nTileY + 1][nTileX];
	
	tile.triangles.resize( 2 );
	tile.triangles[0].i1 = 0;
	tile.triangles[0].i2 = 1;
	tile.triangles[0].i3 = 2;
	tile.triangles[1].i1 = 2;
	tile.triangles[1].i2 = 3;
	tile.triangles[1].i3 = 0;
}


void CTerraGen::ResetTerrainTiles( const int nTileX1, const int nTileY1, const int nTileX2, const int nTileY2 )
{
	for ( int g = nTileY1; g < nTileY2; ++g )
	{
		for ( int i = nTileX1; i < nTileX2; ++i )
		{
			ResetTile( i, g );
		}
	}
}


#define MAX_PACK_ZERO_NUM 255

union uF2I
{
	float f;
	uint32_t n;
};


static void PackFloatArrToRaw( std::vector<uint8_t> *pDst, const float *pData, const int nLength )
{
	pDst->reserve( 256 );
	pDst->resize( 0 );

	bool bWasZero = false;
	int nCnt = 0, nZeroCnt;
	uF2I uf2i;

	while ( nCnt < nLength )
	{
		nZeroCnt = 0;
		while ( (nZeroCnt < MAX_PACK_ZERO_NUM) && (nCnt < nLength) && (fabs(pData[nCnt]) < EPS_VALUE) )
		{
			++nZeroCnt;
			++nCnt;
		}
		pDst->push_back( nZeroCnt );
		if ( nZeroCnt == 0 )
		{
			uf2i.f = pData[nCnt];
			pDst->push_back( uf2i.n & 0xff );
			pDst->push_back( (uf2i.n >> 8) & 0xff );
			pDst->push_back( (uf2i.n >> 16) & 0xff );
			pDst->push_back( (uf2i.n >> 24) & 0xff );
			++nCnt;
		}
	}
}


static void UnpackRawToFloatArr( float *pData, const int nLength, const std::vector<uint8_t> &src )
{
	int k = 0;
	std::vector<uint8_t>::const_iterator it = src.begin();
	uF2I uf2i;

	while ( (k < nLength) && (it != src.end()) )
	{
		if ( *it == 0 )
		{
			uf2i.n = *(++it);
			uf2i.n |= uint32_t( *(++it) ) << 8;
			uf2i.n |= uint32_t( *(++it) ) << 16;
			uf2i.n |= uint32_t( *(++it) ) << 24;
			pData[k++] = uf2i.f;
		}
		else
		{
			for ( int i = 0; i < *it; ++i )
			{
				if ( k >= nLength )
				{
					NI_ASSERT( k < nLength, "Unpack error!" );
					break;
				}
				else
					pData[k++] = 0.0f;
			}
		}
		++it;
	}
}


#define DEF_OPTIMIZE_FLOAT2SHORT 100.0f
#define DEF_OPTIMIZE_SHORT2FLOAT 0.01f

void CTerraGen::OptimizeTerrainInfo()
{
	// fix bugs in tiles
	std::vector<STriangle> trgs;
	trgs.reserve( 512 );
	for ( int g = 0; g < terrainInfo.tiles.GetSizeY(); ++g )
	{
		for ( int i = 0; i < terrainInfo.tiles.GetSizeX(); ++i )
		{
			STerrainInfo::STile &tile = terrainInfo.tiles[g][i];
			trgs.resize( 0 );
			const CVec2 vMin( (float)i * DEF_TILE_SIZE - DEF_EPS, (float)g * DEF_TILE_SIZE - DEF_EPS );
			const CVec2 vMax( (float)( i + 1 ) * DEF_TILE_SIZE + DEF_EPS, (float)( g + 1 ) * DEF_TILE_SIZE + DEF_EPS );
			for ( std::vector<STriangle>::const_iterator it = tile.triangles.begin(); it != tile.triangles.end(); ++it )
			{
				if ( (IsInsideBB(CVec2(tile.vertices[it->i1].x, tile.vertices[it->i1].y), vMin, vMax)) &&
						 (IsInsideBB(CVec2(tile.vertices[it->i2].x, tile.vertices[it->i2].y), vMin, vMax)) &&
						 (IsInsideBB(CVec2(tile.vertices[it->i3].x, tile.vertices[it->i3].y), vMin, vMax)) )
					trgs.push_back( *it );
			}
			tile.triangles = trgs;
		}
	}

	// optimize tiles
	terrainInfo.optimizedTiles.reserve( 128 );
	terrainInfo.optimizedTiles.resize( 0 );
	for ( int g = 0; g < terrainInfo.tiles.GetSizeY(); ++g )
	{
		const float fTileY = (float)g * DEF_TILE_SIZE;
		for ( int i = 0; i < terrainInfo.tiles.GetSizeX(); ++i )
		{
			const STerrainInfo::STile &tile = terrainInfo.tiles[g][i];
			const float fTileX = (float)i * DEF_TILE_SIZE;
			if (	(tile.triangles.size() != 2) ||
						(fabs2(tile.vertices[0].GetVec2() - CVec2(fTileX, fTileY)) > DEF_EPS) ||
						(fabs2(tile.vertices[1].GetVec2() - CVec2(fTileX + DEF_TILE_SIZE, fTileY)) > DEF_EPS) ||
						(fabs2(tile.vertices[2].GetVec2() - CVec2(fTileX + DEF_TILE_SIZE, fTileY + DEF_TILE_SIZE)) > DEF_EPS) ||
						(fabs2(tile.vertices[3].GetVec2() - CVec2(fTileX, fTileY + DEF_TILE_SIZE)) > DEF_EPS) )
			{
				//terrainInfo.optimizedTiles.push_back( STerrainInfo::SOptimizedTile() );
				//terrainInfo.optimizedTiles.back().nIndex = g * terrainInfo.tiles.GetSizeX() + i;
				//terrainInfo.optimizedTiles.back().triangles = tile.triangles;
				//terrainInfo.optimizedTiles.back().vertices.reserve( tile.vertices.size() );
				//terrainInfo.optimizedTiles.back().vertices.resize( 0 );
				//for ( vector<CVec3fEx>::const_iterator itVert = tile.vertices.begin(); itVert != tile.vertices.end(); ++itVert )
				//	terrainInfo.optimizedTiles.back().vertices.push_back( GetVec3( *itVert ) );
				//terrainInfo.optimizedTiles.back().addHeights = tile.addHeights;

				std::vector<STerrainInfo::SOptimizedTile>::iterator itAdd = terrainInfo.optimizedTiles.insert( terrainInfo.optimizedTiles.end(), STerrainInfo::SOptimizedTile() );
				itAdd->nIndex = g * terrainInfo.tiles.GetSizeX() + i;
				itAdd->triangles = tile.triangles;
				itAdd->vertices.reserve( tile.vertices.size() );
				itAdd->vertices.resize( 0 );
				for ( std::vector<CVec3fEx>::const_iterator itVert = tile.vertices.begin(); itVert != tile.vertices.end(); ++itVert )
					itAdd->vertices.push_back( itVert->GetVec3() );
				itAdd->addHeights = tile.addHeights;
			}
		}
	}

	// optimize sea mask
	//terrainInfo.optimizedSeaMask.SetSizes( terrainInfo.seaMask.GetSizeX(), terrainInfo.seaMask.GetSizeY() );
	//terrainInfo.optimizedSeaMask.FillZero();
	//for ( int g = 0; g < terrainInfo.seaMask.GetSizeY(); ++g )
	//{
	//	for ( int i = 0; i < terrainInfo.seaMask.GetSizeX(); ++i )
	//	{
	//		if ( terrainInfo.seaMask[g][i] )
	//			terrainInfo.optimizedSeaMask.SetData( i, g );
	//	}
	//}
	//terrainInfo.seaMask.Clear();

	// optimize waterAddHeights
	PackFloatArrToRaw( &terrainInfo.waterAddHeightsPacked, &(terrainInfo.waterAddHeights[0][0]),
										 terrainInfo.waterAddHeights.GetSizeX() * terrainInfo.waterAddHeights.GetSizeY() );
}


void CTerraGen::RestoreTerrainInfoAfterOptimizing()
{
	// restore heights
	if ( (terrainInfo.heights.GetSizeX() == 0) && (terrainInfo.heights.GetSizeY() == 0) )
	{
		terrainInfo.heights.SetSizes( terrainInfo.optimizedHeights.GetSizeX(), terrainInfo.optimizedHeights.GetSizeY() );
		for ( int g = 0; g < terrainInfo.heights.GetSizeY(); ++g )
		{
			for ( int i = 0; i < terrainInfo.heights.GetSizeX(); ++i )
			{
				terrainInfo.heights[g][i] = float( terrainInfo.optimizedHeights[g][i] ) * DEF_OPTIMIZE_SHORT2FLOAT;
			}
		}
	}

	//terrainInfo.heights.FillEvery( 4.0f );

	if ( (terrainInfo.addHeights.GetSizeX() == 0) && (terrainInfo.addHeights.GetSizeY() == 0) )
	{
		terrainInfo.addHeights.SetSizes( terrainInfo.optimizedAddHeights.GetSizeX(), terrainInfo.optimizedAddHeights.GetSizeY() );
		for ( int g = 0; g < terrainInfo.addHeights.GetSizeY(); ++g )
		{
			for ( int i = 0; i < terrainInfo.addHeights.GetSizeX(); ++i )
			{
				terrainInfo.addHeights[g][i] = float( terrainInfo.optimizedAddHeights[g][i] ) * DEF_OPTIMIZE_SHORT2FLOAT;
			}
		}
	}

	//CRAP { for support of maps without riverHeights
	if ( (terrainInfo.riverHeights.GetSizeX() != terrainInfo.addHeights.GetSizeX()) || (terrainInfo.riverHeights.GetSizeY() != terrainInfo.addHeights.GetSizeY()) )
	{
		terrainInfo.riverHeights.SetSizes( terrainInfo.addHeights.GetSizeX(), terrainInfo.addHeights.GetSizeY() );
		terrainInfo.riverHeights.FillEvery( 0.0f );
	}
	//CRAP }

	// CRAP { for support of maps without waterHeightCoeffs
	/*if ( ( terrainInfo.waterHeightCoeffs.GetSizeX() != terrainInfo.heights.GetSizeX() ) || ( terrainInfo.waterHeightCoeffs.GetSizeY() != terrainInfo.heights.GetSizeY() ) )
	{
		terrainInfo.waterHeightCoeffs.SetSizes( terrainInfo.heights.GetSizeX(), terrainInfo.heights.GetSizeY() );
		terrainInfo.waterHeightCoeffs.FillEvery( 1.0f );
	}*/
	// CRAP }

	// restore tiles
	if ( (terrainInfo.tiles.GetSizeX() == 0) && (terrainInfo.tiles.GetSizeY() == 0) )
	{
		terrainInfo.tiles.SetSizes( pDesc->nNumPatchesX * DEF_PATCH_SIZE, pDesc->nNumPatchesY * DEF_PATCH_SIZE );

		for ( std::vector<STerrainInfo::SOptimizedTile>::const_iterator it = terrainInfo.optimizedTiles.begin(); it != terrainInfo.optimizedTiles.end(); ++it )
		{
			const int nIndX = it->nIndex % terrainInfo.tiles.GetSizeX();
			const int nIndY = it->nIndex / terrainInfo.tiles.GetSizeX();
			NI_ASSERT( nIndX < terrainInfo.tiles.GetSizeX() && nIndY < terrainInfo.tiles.GetSizeY(), "Wrong tiles data" );
			if ( (nIndX < terrainInfo.tiles.GetSizeX()) && (nIndY < terrainInfo.tiles.GetSizeY()) )
			{
				STerrainInfo::STile &tile = terrainInfo.tiles[nIndY][nIndX];
				tile.triangles = it->triangles;
				tile.vertices.reserve( it->vertices.size() );
				tile.vertices.resize( 0 );
				for ( std::vector<CVec3>::const_iterator itVert = it->vertices.begin(); itVert != it->vertices.end(); ++itVert )
					tile.vertices.push_back( CVec3fEx(*itVert, 0) );
				tile.addHeights = it->addHeights;
			}
		}

		TIME_STAT_START( Part1 )

		ResetTerrainTiles( 0, 0, terrainInfo.tiles.GetSizeX(), terrainInfo.tiles.GetSizeY() );

		TIME_STAT_FINISH( Part1 )
	}

	// restore sea mask
	/*if ( (terrainInfo.seaMask.GetSizeX() == 0) && (terrainInfo.seaMask.GetSizeY() == 0) )
	{
		terrainInfo.seaMask.SetSizes( terrainInfo.optimizedSeaMask.GetSizeX(), terrainInfo.optimizedSeaMask.GetSizeY() );
		for ( int g = 0; g < terrainInfo.seaMask.GetSizeY(); ++g )
		{
			for ( int i = 0; i < terrainInfo.seaMask.GetSizeX(); ++i )
			{
					terrainInfo.seaMask[g][i] = terrainInfo.optimizedSeaMask.GetData( i, g );
			}
		}
	}*/

	// restore waterAddHeights
	terrainInfo.waterAddHeights.SetSizes( terrainInfo.heights.GetSizeX(), terrainInfo.heights.GetSizeY() );
	terrainInfo.waterAddHeights.FillZero();
	UnpackRawToFloatArr(	&(terrainInfo.waterAddHeights[0][0]),
												terrainInfo.waterAddHeights.GetSizeX() * terrainInfo.waterAddHeights.GetSizeY(),
												terrainInfo.waterAddHeightsPacked );

	//InitWaterParams();
}



