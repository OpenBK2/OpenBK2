#include "stdafx.h"

#include "GenTerrain.h"

#define DEF_VERTICES_NUM_ALLOC 256
#define DEF_TRIANGLES_NUM_ALLOC 256
#define DEF_TERRASPOT_HEIGHT 0.1f

inline int AddUniqueTerraSpotVertex( std::vector<NGScene::SVertex> &arr, const CVec3 &vert, const int nTileX, const int nTileY,
																		 const CVec3 &vNorm1, const CVec3 &vNorm2, const CVec3 &vNorm3, const CVec3 &vNorm4,
																		 const CVec2 &vTex, NGScene::SVertex &templ,
																		 const float fUsedTexSizeX, const float fUsedTexSizeY )
{
	int nCount = 0;
	for ( std::vector<NGScene::SVertex>::const_iterator it = arr.begin(); it != arr.end(); ++it, ++nCount )
	{
		if ( ( fabs2( it->pos.x - vert.x ) + fabs2( it->pos.y - vert.y ) + fabs2( it->pos.z - vert.z ) ) < DEF_FLOAT_EPS2 )
			return nCount;
	}
	templ.pos = vert;
	templ.tex.Set( vTex.x * fUsedTexSizeX, vTex.y * fUsedTexSizeY );
	const float fDx = Clamp( ( vert.x - (float)nTileX * DEF_TILE_SIZE ) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	const float fDy = Clamp( ( vert.y - (float)nTileY * DEF_TILE_SIZE ) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	CalcCompactVector( &(templ.normal),
		( vNorm1 + ( vNorm2 - vNorm1 ) * fDx ) * ( 1.0f - fDy ) + ( vNorm4 + ( vNorm3 - vNorm4 ) * fDx ) * fDy );
	arr.push_back( templ );
	return ( arr.size() - 1 );
}

void CTerraGen::CreateTerraSpotGfx( STerrainInfo::STerraSpot *pSpot, const NDb::STerrainSpotInstance *pInstance )
{
	if ( pInstance->pDescriptor == 0 )
		return;
	CVec3 v1( pInstance->points[0].x, pInstance->points[0].y, 0.0f );
	CVec3 v2( pInstance->points[1].x, pInstance->points[1].y, 0.0f );
	CVec3 v3( pInstance->points[2].x, pInstance->points[2].y, 0.0f );
	CVec3 v4( pInstance->points[3].x, pInstance->points[3].y, 0.0f );
	AI2Vis( &v1 );
	AI2Vis( &v2 );
	AI2Vis( &v3 );
	AI2Vis( &v4 );

	std::vector<CVec3> coeffs(4);
	GetLineEq( v1.x, v1.y, v2.x, v2.y, &(coeffs[0].x), &(coeffs[0].y), &(coeffs[0].z) );
	GetLineEq( v2.x, v2.y, v3.x, v3.y, &(coeffs[1].x), &(coeffs[1].y), &(coeffs[1].z) );
	GetLineEq( v3.x, v3.y, v4.x, v4.y, &(coeffs[2].x), &(coeffs[2].y), &(coeffs[2].z) );
	GetLineEq( v4.x, v4.y, v1.x, v1.y, &(coeffs[3].x), &(coeffs[3].y), &(coeffs[3].z) );

	const int nTileX1 = Clamp( int( min( min(v1.x, v2.x), min(v3.x, v4.x) ) * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY1 = Clamp( int( min( min(v1.y, v2.y), min(v3.y, v4.y) ) * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeY() - 1 );
	const int nTileX2 = Clamp( int( max( max(v1.x, v2.x), max(v3.x, v4.x) ) * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY2 = Clamp( int( max( max(v1.y, v2.y), max(v3.y, v4.y) ) * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeY() - 1 );

	pSpot->vMin.Set( nTileX1, nTileY1 );
	pSpot->vMax.Set( nTileX2, nTileY2 );

	STerraSpotGFXInfo gfxInfo;
	gfxInfo.nID = pInstance->nSpotID;
	gfxInfo.pDesc = pInstance->pDescriptor;
	gfxInfo.data.vertices.reserve( DEF_VERTICES_NUM_ALLOC );
	gfxInfo.data.vertices.resize( 0 );
	gfxInfo.data.triangles.reserve( DEF_TRIANGLES_NUM_ALLOC );
	gfxInfo.data.triangles.resize( 0 );

	const float fUsedTexSizeX = pInstance->pDescriptor->fUsedTexSizeX;
	const float fUsedTexSizeY = pInstance->pDescriptor->fUsedTexSizeY;

	CVec2 bary;
	NGScene::SVertex templ;
	CalcCompactVector( &(templ.texU), CVec3(1, 0, 0) );
	CalcCompactVector( &(templ.texV), CVec3(0, 1, 0) );

	static std::vector<CVec3> points(16);
	static std::vector<CVec3> tempPoints(16);
	static std::vector<CVec3> resPoints(16);

	int nPrevInd;

	for ( int g = nTileY1; g <= nTileY2; ++g )
	{
		for ( int i = nTileX1; i <= nTileX2; ++i )
		{
			const STerrainInfo::STile &tile = terrainInfo.tiles[g][i];
			for ( std::vector<STriangle>::const_iterator it = tile.triangles.begin(); it != tile.triangles.end(); ++it )
			{
				const CVec3 vert1( tile.vertices[it->i1].x, tile.vertices[it->i1].y,
					max( tile.vertices[it->i1].z + tile.addHeights[it->i1], 0.0f ) + DEF_TERRASPOT_HEIGHT );
				const CVec3 vert2( tile.vertices[it->i2].x, tile.vertices[it->i2].y,
					max( tile.vertices[it->i2].z + tile.addHeights[it->i2], 0.0f ) + DEF_TERRASPOT_HEIGHT );
				const CVec3 vert3( tile.vertices[it->i3].x, tile.vertices[it->i3].y,
					max( tile.vertices[it->i3].z + tile.addHeights[it->i3], 0.0f ) + DEF_TERRASPOT_HEIGHT );
				const bool bFlag1 = IsInside( coeffs, vert1 );
				const bool bFlag2 = IsInside( coeffs, vert2 );
				const bool bFlag3 = IsInside( coeffs, vert3 );
				if ( bFlag1 && bFlag2 && bFlag3 )
				{
					GetBaryCoords( vert1, v1, v2, v4, &bary );
					const int nInd1 = AddUniqueTerraSpotVertex( gfxInfo.data.vertices, vert1, i, g,
						terrainNorms[g][i], terrainNorms[g][i+1], terrainNorms[g+1][i+1], terrainNorms[g+1][i],
						bary, templ, fUsedTexSizeX, fUsedTexSizeY );
					GetBaryCoords( vert2, v1, v2, v4, &bary );
					const int nInd2 = AddUniqueTerraSpotVertex( gfxInfo.data.vertices, vert2, i, g,
						terrainNorms[g][i], terrainNorms[g][i+1], terrainNorms[g+1][i+1], terrainNorms[g+1][i],
						bary, templ, fUsedTexSizeX, fUsedTexSizeY );
					GetBaryCoords( vert3, v1, v2, v4, &bary );
					const int nInd3 = AddUniqueTerraSpotVertex( gfxInfo.data.vertices, vert3, i, g,
						terrainNorms[g][i], terrainNorms[g][i+1], terrainNorms[g+1][i+1], terrainNorms[g+1][i],
						bary, templ, fUsedTexSizeX, fUsedTexSizeY );
					gfxInfo.data.triangles.push_back( STriangle( nInd1, nInd2, nInd3 ) );
				}
				else
				{
					points.resize( 0 );
					if ( bFlag1 ) points.push_back( vert1 );
					if ( bFlag2 ) points.push_back( vert2 );
					if ( bFlag3 ) points.push_back( vert3 );

					GetBaryCoords( v1, vert1, vert2, vert3, &bary );
					if ( ( bary.x >= 0.0f ) && ( bary.y >= 0.0f ) && ( ( bary.x + bary.y ) <= 1.0f ) )
					{
						v1.z = ( vert2.z - vert1.z ) * bary.x + ( vert3.z - vert1.z ) * bary.y + vert1.z;
						points.push_back( v1 );
					}
					GetBaryCoords( v2, vert1, vert2, vert3, &bary );
					if ( ( bary.x >= 0.0f ) && ( bary.y >= 0.0f ) && ( ( bary.x + bary.y ) <= 1.0f ) )
					{
						v2.z = ( vert2.z - vert1.z ) * bary.x + ( vert3.z - vert1.z ) * bary.y + vert1.z;
						points.push_back( v2 );
					}
					GetBaryCoords( v3, vert1, vert2, vert3, &bary );
					if ( ( bary.x >= 0.0f ) && ( bary.y >= 0.0f ) && ( ( bary.x + bary.y ) <= 1.0f ) )
					{
						v3.z = ( vert2.z - vert1.z ) * bary.x + ( vert3.z - vert1.z ) * bary.y + vert1.z;
						points.push_back( v3 );
					}
					GetBaryCoords( v4, vert1, vert2, vert3, &bary );
					if ( ( bary.x >= 0.0f ) && ( bary.y >= 0.0f ) && ( ( bary.x + bary.y ) <= 1.0f ) )
					{
						v4.z = ( vert2.z - vert1.z ) * bary.x + ( vert3.z - vert1.z ) * bary.y + vert1.z;
						points.push_back( v4 );
					}

					AddIntersection( &points, v1, v2, vert1, vert2 );
					AddIntersection( &points, v2, v3, vert1, vert2 );
					AddIntersection( &points, v3, v4, vert1, vert2 );
					AddIntersection( &points, v4, v1, vert1, vert2 );

					AddIntersection( &points, v1, v2, vert2, vert3 );
					AddIntersection( &points, v2, v3, vert2, vert3 );
					AddIntersection( &points, v3, v4, vert2, vert3 );
					AddIntersection( &points, v4, v1, vert2, vert3 );

					AddIntersection( &points, v1, v2, vert3, vert1 );
					AddIntersection( &points, v2, v3, vert3, vert1 );
					AddIntersection( &points, v3, v4, vert3, vert1 );
					AddIntersection( &points, v4, v1, vert3, vert1 );

					if ( points.size() < 3 )
						continue;

					tempPoints.resize( 0 );
					for ( std::vector<CVec3>::const_iterator it = points.begin(); it != points.end(); ++it )
					{
						std::vector<CVec3>::const_iterator itTemp = tempPoints.begin();
						for ( ; itTemp != tempPoints.end(); ++itTemp )
						{
							if ( ( fabs2( it->x - itTemp->x ) + fabs2( it->y - itTemp->y ) ) < DEF_FLOAT_EPS2 )
								break;
						}
						if ( itTemp == tempPoints.end() )
						{
							tempPoints.push_back( *it );
						}
					}

					if ( tempPoints.size() < 3 )
						continue;

					CreateConvexHull( &resPoints, tempPoints );

					std::vector<CVec3>::const_iterator itResPoint = resPoints.begin();
					GetBaryCoords( *itResPoint, v1, v2, v4, &bary );
					const int nInd0 = AddUniqueTerraSpotVertex( gfxInfo.data.vertices, *itResPoint, i, g,
						terrainNorms[g][i], terrainNorms[g][i+1], terrainNorms[g+1][i+1], terrainNorms[g+1][i],
						bary, templ, fUsedTexSizeX, fUsedTexSizeY );
					++itResPoint;
					GetBaryCoords( *itResPoint, v1, v2, v4, &bary );
					nPrevInd = AddUniqueTerraSpotVertex( gfxInfo.data.vertices, *itResPoint, i, g,
						terrainNorms[g][i], terrainNorms[g][i+1], terrainNorms[g+1][i+1], terrainNorms[g+1][i],
						bary, templ, fUsedTexSizeX, fUsedTexSizeY );
					++itResPoint;

					for ( ; itResPoint != resPoints.end(); ++itResPoint )
					{
						GetBaryCoords( *itResPoint, v1, v2, v4, &bary );
						const int nInd = AddUniqueTerraSpotVertex( gfxInfo.data.vertices, *itResPoint, i, g,
							terrainNorms[g][i], terrainNorms[g][i+1], terrainNorms[g+1][i+1], terrainNorms[g+1][i],
							bary, templ, fUsedTexSizeX, fUsedTexSizeY );

						if ( IsCCW( gfxInfo.data.vertices[nInd0].pos, gfxInfo.data.vertices[nPrevInd].pos, gfxInfo.data.vertices[nInd].pos ) )
							gfxInfo.data.triangles.push_back( STriangle( nInd0, nPrevInd, nInd ) );
						else
							gfxInfo.data.triangles.push_back( STriangle( nInd0, nInd, nPrevInd ) );

						nPrevInd = nInd;
					}
				}
			}
		}
	}

	terrainGfxInfo.terraspots.push_back( gfxInfo );

	if ( pGfxObserver )
		pGfxObserver->AddTerraSpot( &gfxInfo );
}

void CTerraGen::AddTerraSpot( const NDb::STerrainSpotInstance *pInstance )
{
	NI_ASSERT( pInstance->points.size() == 4, "TerraSpot must contain 4 points for building" );

	//TIME_STAT_START( CTerraGen__AddTerraSpot )

	STerrainInfo::STerraSpot terraSpot;
	terraSpot.nID = pInstance->nSpotID;
	terrainInfo.terraspots.push_back( terraSpot );

	CreateTerraSpotGfx( &(terrainInfo.terraspots.back()), pInstance );

	//TIME_STAT_FINISH( CTerraGen__AddTerraSpot )
}

void CTerraGen::AddAllTerraSpots()
{
	NI_ASSERT( pDesc, "Terrain is not loaded" );
	RemoveAllTerraSpots();
	for ( std::vector<NDb::STerrainSpotInstance>::const_iterator it = pDesc->spots.begin(); it != pDesc->spots.end(); ++it )
	{
		AddTerraSpot( &(*it) );
	}
}

void CTerraGen::RemoveAllTerraSpots()
{
	NI_ASSERT( pDesc, "Terrain is not loaded" );
	if ( pGfxObserver )
	{
		for ( std::list<STerrainInfo::STerraSpot>::const_iterator it = terrainInfo.terraspots.begin(); it != terrainInfo.terraspots.end(); ++it )
		{
			pGfxObserver->RemoveTerraSpot( it->nID );
		}
	}
	terrainGfxInfo.terraspots.clear();
	terrainInfo.terraspots.clear();
}

void CTerraGen::UpdateTerraSpot( const int nSpotID )
{
	RemoveTerraSpot( nSpotID );
	for ( std::vector<NDb::STerrainSpotInstance>::const_iterator it = pDesc->spots.begin(); it != pDesc->spots.end(); ++it )
	{
		if ( it->nSpotID == nSpotID )
		{
			AddTerraSpot( &(*it) );
			return;
		}
	}
	NI_ASSERT( false, "Updated TerraSpot is not exists" );
}

void CTerraGen::RemoveTerraSpot( const int nSpotID )
{
	for ( std::list<STerrainInfo::STerraSpot>::iterator it = terrainInfo.terraspots.begin(); it != terrainInfo.terraspots.end(); ++it )
	{
		if ( it->nID == nSpotID )
		{
			terrainInfo.terraspots.erase( it );
			break;
		}
	}

	for ( std::list<STerraSpotGFXInfo>::iterator it = terrainGfxInfo.terraspots.begin(); it != terrainGfxInfo.terraspots.end(); ++it )
	{
		if ( it->nID == nSpotID )
		{
			terrainGfxInfo.terraspots.erase( it );
			break;
		}
	}

	if ( pGfxObserver )
		pGfxObserver->RemoveTerraSpot( nSpotID );
}

const NDb::STerrainSpotInstance* CTerraGen::FindTerraSpot( int nID ) const
{
	for ( std::vector<NDb::STerrainSpotInstance>::const_iterator it = pDesc->spots.begin(); it != pDesc->spots.end(); ++it )
	{
		if ( it->nSpotID == nID )
			return &(*it);
	}
	return 0;
}


