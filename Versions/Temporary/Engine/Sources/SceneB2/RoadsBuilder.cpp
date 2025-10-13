#include "stdafx.h"

#include "3Dmotor/DBScene.h"
#include "GenTerrain.h"

#include <cstdint>

#define DEF_ROAD_HEIGHT 0.1f
#define DEF_ROAD_SAMPS_PER_PATCH 4
#define DEF_ROAD_PATCH_VERTS_RESERVE 256
#define DEF_ROAD_PATCH_TRGS_RESERVE 256
//
#define DEF_ROAD_VERT_EPS 0.001f

inline int AddUniqueRoadVertex( std::vector<NGScene::SVertex> *pVerts, const CVec3 &vert,
																const int nTileX, const int nTileY,
																const CVec3 &vNorm1, const CVec3 &vNorm2, const CVec3 &vNorm3, const CVec3 &vNorm4,
																const CVec2 &vTex, const float fAlpha )
{
	int nCount = 0;
	for ( std::vector<NGScene::SVertex>::const_iterator it = pVerts->begin(); it != pVerts->end(); ++it, ++nCount )
	{
		if ( (fabs2(it->pos.x - vert.x) < DEF_ROAD_VERT_EPS) && (fabs2(it->pos.y - vert.y) < DEF_ROAD_VERT_EPS) &&
				 (fabs2(it->pos.z - vert.z) < DEF_ROAD_VERT_EPS) )
			return nCount;
	}
	NGScene::SVertex v;
	CalcCompactVector( &(v.texU), V3_AXIS_X );
	CalcCompactVector( &(v.texV), V3_AXIS_Y );
	v.pos = vert;
	v.tex = vTex;
	const float fDx = Clamp( (vert.x - (float)nTileX * DEF_TILE_SIZE) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	const float fDy = Clamp( (vert.y - (float)nTileY * DEF_TILE_SIZE) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	CalcCompactVector( &(v.normal), (vNorm1 + (vNorm2 - vNorm1) * fDx) * (1.0f - fDy) + (vNorm4 + (vNorm3 - vNorm4) * fDx) * fDy );
	v.normal.w = Clamp( Float2Int(fAlpha * 255.0f), 0, 255 );
	pVerts->push_back( v );
	return ( pVerts->size() - 1 );
}

inline int CTerraGen::CalcTexAndAddRoadVertex(	const CVec3 &vert, const CVec3 &v1, const CVec3 &v2, const CVec3 &v3, const CVec3 &v4,
																								const CVec2 &vTex1, const CVec2 &vTex2, const CVec2 &vTex3, const CVec2 &vTex4,
																								const float fAlpha1, const float fAlpha2, const float fAlpha3, const float fAlpha4,
																								const int nTileX, const int nTileY,
																								std::vector<NGScene::SVertex> *pVerts )
{
	CVec2 vBary;
	GetBaryCoords( vert, v1, v2, v4, &vBary );
	if ( (vBary.x + vBary.y) < (1.0f + DEF_EPS) )
	{
		return AddUniqueRoadVertex( pVerts, vert, nTileX, nTileY,
																terrainNorms[nTileY][nTileX], terrainNorms[nTileY][nTileX + 1], terrainNorms[nTileY + 1][nTileX + 1], terrainNorms[nTileY + 1][nTileX],
																(vTex2 - vTex1) * vBary.x + (vTex4 - vTex1) * vBary.y + vTex1,
																(fAlpha2 - fAlpha1) * vBary.x + (fAlpha4 - fAlpha1) * vBary.y + fAlpha1 );
	}
	GetBaryCoords( vert, v3, v4, v2, &vBary );
	return AddUniqueRoadVertex( pVerts, vert, nTileX, nTileY,
															terrainNorms[nTileY][nTileX], terrainNorms[nTileY][nTileX + 1], terrainNorms[nTileY + 1][nTileX + 1], terrainNorms[nTileY + 1][nTileX],
															(vTex4 - vTex3) * vBary.x + (vTex2 - vTex3) * vBary.y + vTex3,
															(fAlpha4 - fAlpha3) * vBary.x + (fAlpha2 - fAlpha3) * vBary.y + fAlpha3 );
}

void CTerraGen::ProjectTrgOnTerrain( const CVec3 &v1, const CVec3 &v2, const CVec3 &v3,
																		 const CVec2 &vTex1, const CVec2 &vTex2, const CVec2 &vTex3,
																		 const float fAlpha1, const float fAlpha2, const float fAlpha3,
																		 NMeshData::SMeshData *pData )
{
	const int nTileX1 = Clamp( int(min(min(v1.x, v2.x), v3.x) * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY1 = Clamp( int(min(min(v1.y, v2.y), v3.y) * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );
	const int nTileX2 = Clamp( int(max(max(v1.x, v2.x), v3.x) * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY2 = Clamp( int(max(max(v1.y, v2.y), v3.y) * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );

	CVec2 vBary1, vBary2, vBary3, vBary;

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
													 max(tile.vertices[it->i1].z + tile.addHeights[it->i1], 0.0f) + DEF_ROAD_HEIGHT );
				const CVec3 vert2( tile.vertices[it->i2].x, tile.vertices[it->i2].y,
													 max(tile.vertices[it->i2].z + tile.addHeights[it->i2], 0.0f) + DEF_ROAD_HEIGHT );
				const CVec3 vert3( tile.vertices[it->i3].x, tile.vertices[it->i3].y,
													 max(tile.vertices[it->i3].z + tile.addHeights[it->i3], 0.0f) + DEF_ROAD_HEIGHT );
				GetBaryCoords( vert1, v1, v2, v3, &vBary1 );
				const bool bFlag1 = ( vBary1.x > -DEF_EPS ) && ( vBary1.y > -DEF_EPS ) && ( (vBary1.x + vBary1.y) < (1.0f + DEF_EPS) );
				GetBaryCoords( vert2, v1, v2, v3, &vBary2 );
				const bool bFlag2 = ( vBary2.x > -DEF_EPS ) && ( vBary2.y > -DEF_EPS ) && ( (vBary2.x + vBary2.y) < (1.0f + DEF_EPS) );
				GetBaryCoords( vert3, v1, v2, v3, &vBary3 );
				const bool bFlag3 = ( vBary3.x > -DEF_EPS ) && ( vBary3.y > -DEF_EPS ) && ( (vBary3.x + vBary3.y) < (1.0f + DEF_EPS) );
				if ( bFlag1 && bFlag2 && bFlag3 )
				{
					const int nInd1 = AddUniqueRoadVertex( &pData->vertices, vert1, i, g,
																								 terrainNorms[g][i], terrainNorms[g][i+1], terrainNorms[g+1][i+1], terrainNorms[g+1][i],
																								 (vTex2 - vTex1) * vBary1.x + (vTex3 - vTex1) * vBary1.y + vTex1,
																								 (fAlpha2 - fAlpha1) * vBary1.x + (fAlpha3 - fAlpha1) * vBary1.y + fAlpha1 );
					const int nInd2 = AddUniqueRoadVertex( &pData->vertices, vert2, i, g,
																								 terrainNorms[g][i], terrainNorms[g][i+1], terrainNorms[g+1][i+1], terrainNorms[g+1][i],
																								 (vTex2 - vTex1) * vBary2.x + (vTex3 - vTex1) * vBary2.y + vTex1,
																								 (fAlpha2 - fAlpha1) * vBary2.x + (fAlpha3 - fAlpha1) * vBary2.y + fAlpha1 );
					const int nInd3 = AddUniqueRoadVertex( &pData->vertices, vert3, i, g,
																								 terrainNorms[g][i], terrainNorms[g][i+1], terrainNorms[g+1][i+1], terrainNorms[g+1][i],
																								 (vTex2 - vTex1) * vBary3.x + (vTex3 - vTex1) * vBary3.y + vTex1,
																								 (fAlpha2 - fAlpha1) * vBary3.x + (fAlpha3 - fAlpha1) * vBary3.y + fAlpha1 );
					if ( IsCCW(pData->vertices[nInd1].pos, pData->vertices[nInd2].pos, pData->vertices[nInd3].pos) )
						pData->triangles.push_back( STriangle(nInd1, nInd2, nInd3) );
					else
						pData->triangles.push_back( STriangle(nInd1, nInd3, nInd2) );
				}
				else
				{
					points.resize( 0 );
					if ( bFlag1 )
						points.push_back( vert1 );
					if ( bFlag2 )
						points.push_back( vert2 );
					if ( bFlag3 )
						points.push_back( vert3 );

					GetBaryCoords( v1, vert1, vert2, vert3, &vBary );
					if ( (vBary.x >= 0.0f) && (vBary.y >= 0.0f) && ((vBary.x + vBary.y) <= 1.0f) )
						points.push_back( CVec3(v1.x, v1.y, (vert2.z - vert1.z) * vBary.x + (vert3.z - vert1.z) * vBary.y + vert1.z) );
					GetBaryCoords( v2, vert1, vert2, vert3, &vBary );
					if ( (vBary.x >= 0.0f) && (vBary.y >= 0.0f) && ((vBary.x + vBary.y) <= 1.0f) )
						points.push_back( CVec3(v2.x, v2.y, (vert2.z - vert1.z) * vBary.x + (vert3.z - vert1.z) * vBary.y + vert1.z) );
					GetBaryCoords( v3, vert1, vert2, vert3, &vBary );
					if ( (vBary.x >= 0.0f) && (vBary.y >= 0.0f) && ((vBary.x + vBary.y) <= 1.0f) )
						points.push_back( CVec3(v3.x, v3.y, (vert2.z - vert1.z) * vBary.x + (vert3.z - vert1.z) * vBary.y + vert1.z) );

					AddIntersection( &points, v1, v2, vert1, vert2 );
					AddIntersection( &points, v2, v3, vert1, vert2 );
					AddIntersection( &points, v3, v1, vert1, vert2 );

					AddIntersection( &points, v1, v2, vert2, vert3 );
					AddIntersection( &points, v2, v3, vert2, vert3 );
					AddIntersection( &points, v3, v1, vert2, vert3 );

					AddIntersection( &points, v1, v2, vert3, vert1 );
					AddIntersection( &points, v2, v3, vert3, vert1 );
					AddIntersection( &points, v3, v1, vert3, vert1 );

					if ( points.size() < 3 )
						continue;

					tempPoints.resize( 0 );
					for ( std::vector<CVec3>::const_iterator it = points.begin(); it != points.end(); ++it )
					{
						std::vector<CVec3>::const_iterator itTemp = tempPoints.begin();
						for ( ; itTemp != tempPoints.end(); ++itTemp )
						{
							if ( (fabs2(it->x - itTemp->x) + fabs2(it->y - itTemp->y)) < DEF_FLOAT_EPS2 )
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

					GetBaryCoords( *itResPoint, v1, v2, v3, &vBary );
					const int nInd0 = AddUniqueRoadVertex( &pData->vertices, *itResPoint, i, g,
																								 terrainNorms[g][i], terrainNorms[g][i+1], terrainNorms[g+1][i+1], terrainNorms[g+1][i],
																								 (vTex2 - vTex1) * vBary.x + (vTex3 - vTex1) * vBary.y + vTex1,
																								 (fAlpha2 - fAlpha1) * vBary.x + (fAlpha3 - fAlpha1) * vBary.y + fAlpha1 );

					++itResPoint;

					GetBaryCoords( *itResPoint, v1, v2, v3, &vBary );
					nPrevInd = AddUniqueRoadVertex( &pData->vertices, *itResPoint, i, g,
																					terrainNorms[g][i], terrainNorms[g][i+1], terrainNorms[g+1][i+1], terrainNorms[g+1][i],
																					(vTex2 - vTex1) * vBary.x + (vTex3 - vTex1) * vBary.y + vTex1,
																					(fAlpha2 - fAlpha1) * vBary.x + (fAlpha3 - fAlpha1) * vBary.y + fAlpha1 );
					++itResPoint;

					for ( ; itResPoint != resPoints.end(); ++itResPoint )
					{
						GetBaryCoords( *itResPoint, v1, v2, v3, &vBary );
						const int nInd = AddUniqueRoadVertex( &pData->vertices, *itResPoint, i, g,
																									terrainNorms[g][i], terrainNorms[g][i+1], terrainNorms[g+1][i+1], terrainNorms[g+1][i],
																									(vTex2 - vTex1) * vBary.x + (vTex3 - vTex1) * vBary.y + vTex1,
																									(fAlpha2 - fAlpha1) * vBary.x + (fAlpha3 - fAlpha1) * vBary.y + fAlpha1 );
						if ( IsCCW(pData->vertices[nInd0].pos, pData->vertices[nPrevInd].pos, pData->vertices[nInd].pos) )
							pData->triangles.push_back( STriangle(nInd0, nPrevInd, nInd) );
						else
							pData->triangles.push_back( STriangle(nInd0, nInd, nPrevInd) );

						nPrevInd = nInd;
					}
				}
			}
		}
	}
}

void CTerraGen::ProjectQuadOnTerrain( const CVec3 &v1, const CVec3 &v2, const CVec3 &v3, const CVec3 &v4,
																			const CVec2 &vTex1, const CVec2 &vTex2, const CVec2 &vTex3, const CVec2 &vTex4,
																			const float fAlpha1, const float fAlpha2, const float fAlpha3, const float fAlpha4,
																			NMeshData::SMeshData *pData )
{
	std::vector<CVec3> coeffs(4);
	GetLineEq( v1.x, v1.y, v2.x, v2.y, &(coeffs[0].x), &(coeffs[0].y), &(coeffs[0].z) );
	GetLineEq( v2.x, v2.y, v3.x, v3.y, &(coeffs[1].x), &(coeffs[1].y), &(coeffs[1].z) );
	GetLineEq( v3.x, v3.y, v4.x, v4.y, &(coeffs[2].x), &(coeffs[2].y), &(coeffs[2].z) );
	GetLineEq( v4.x, v4.y, v1.x, v1.y, &(coeffs[3].x), &(coeffs[3].y), &(coeffs[3].z) );

	const int nTileX1 = Clamp( int(min(min(v1.x, v2.x), min(v3.x, v4.x)) * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY1 = Clamp( int(min(min(v1.y, v2.y), min(v3.y, v4.y)) * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );
	const int nTileX2 = Clamp( int(max(max(v1.x, v2.x), max(v3.x, v4.x)) * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY2 = Clamp( int(max(max(v1.y, v2.y), max(v3.y, v4.y)) * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );

	CVec2 bary;

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
					max( tile.vertices[it->i1].z + tile.addHeights[it->i1], 0.0f ) + DEF_ROAD_HEIGHT );
				const CVec3 vert2( tile.vertices[it->i2].x, tile.vertices[it->i2].y,
					max( tile.vertices[it->i2].z + tile.addHeights[it->i2], 0.0f ) + DEF_ROAD_HEIGHT );
				const CVec3 vert3( tile.vertices[it->i3].x, tile.vertices[it->i3].y,
					max( tile.vertices[it->i3].z + tile.addHeights[it->i3], 0.0f ) + DEF_ROAD_HEIGHT );
				const bool bFlag1 = IsInside( coeffs, vert1 );
				const bool bFlag2 = IsInside( coeffs, vert2 );
				const bool bFlag3 = IsInside( coeffs, vert3 );
				if ( bFlag1 && bFlag2 && bFlag3 )
				{
					const int nInd1 = CalcTexAndAddRoadVertex( vert1, v1, v2, v3, v4, vTex1, vTex2, vTex3, vTex4,
																										 fAlpha1, fAlpha2, fAlpha3, fAlpha4, i, g, &pData->vertices );
					const int nInd2 = CalcTexAndAddRoadVertex( vert2, v1, v2, v3, v4, vTex1, vTex2, vTex3, vTex4,
																										 fAlpha1, fAlpha2, fAlpha3, fAlpha4, i, g, &pData->vertices );
					const int nInd3 = CalcTexAndAddRoadVertex( vert3, v1, v2, v3, v4, vTex1, vTex2, vTex3, vTex4,
																										 fAlpha1, fAlpha2, fAlpha3, fAlpha4, i, g, &pData->vertices );
					if ( IsCCW(pData->vertices[nInd1].pos, pData->vertices[nInd2].pos, pData->vertices[nInd3].pos) )
						pData->triangles.push_back( STriangle(nInd1, nInd2, nInd3) );
					else
						pData->triangles.push_back( STriangle(nInd1, nInd3, nInd2) );
				}
				else
				{
					points.resize( 0 );
					if ( bFlag1 )
						points.push_back( vert1 );
					if ( bFlag2 )
						points.push_back( vert2 );
					if ( bFlag3 )
						points.push_back( vert3 );

					GetBaryCoords( v1, vert1, vert2, vert3, &bary );
					if ( (bary.x >= 0.0f) && (bary.y >= 0.0f) && ((bary.x + bary.y) <= 1.0f) )
						points.push_back( CVec3(v1.x, v1.y, (vert2.z - vert1.z) * bary.x + (vert3.z - vert1.z) * bary.y + vert1.z) );
					GetBaryCoords( v2, vert1, vert2, vert3, &bary );
					if ( (bary.x >= 0.0f) && (bary.y >= 0.0f) && ((bary.x + bary.y) <= 1.0f) )
						points.push_back( CVec3(v2.x, v2.y, (vert2.z - vert1.z) * bary.x + (vert3.z - vert1.z) * bary.y + vert1.z) );
					GetBaryCoords( v3, vert1, vert2, vert3, &bary );
					if ( (bary.x >= 0.0f) && (bary.y >= 0.0f) && ((bary.x + bary.y) <= 1.0f) )
						points.push_back( CVec3(v3.x, v3.y, (vert2.z - vert1.z) * bary.x + (vert3.z - vert1.z) * bary.y + vert1.z) );
					GetBaryCoords( v4, vert1, vert2, vert3, &bary );
					if ( (bary.x >= 0.0f) && (bary.y >= 0.0f) && ((bary.x + bary.y) <= 1.0f) )
						points.push_back( CVec3(v4.x, v4.y, (vert2.z - vert1.z) * bary.x + (vert3.z - vert1.z) * bary.y + vert1.z) );

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
							if ( (fabs2(it->x - itTemp->x) + fabs2(it->y - itTemp->y)) < DEF_FLOAT_EPS2 )
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

					const int nInd0 = CalcTexAndAddRoadVertex( *itResPoint, v1, v2, v3, v4, vTex1, vTex2, vTex3, vTex4,
																										 fAlpha1, fAlpha2, fAlpha3, fAlpha4, i, g, &pData->vertices );
					++itResPoint;
					nPrevInd = CalcTexAndAddRoadVertex( *itResPoint, v1, v2, v3, v4, vTex1, vTex2, vTex3, vTex4,
																							fAlpha1, fAlpha2, fAlpha3, fAlpha4, i, g, &pData->vertices );
					++itResPoint;

					for ( ; itResPoint != resPoints.end(); ++itResPoint )
					{
						const int nInd = CalcTexAndAddRoadVertex( *itResPoint, v1, v2, v3, v4, vTex1, vTex2, vTex3, vTex4,
																											fAlpha1, fAlpha2, fAlpha3, fAlpha4, i, g, &pData->vertices );
						if ( IsCCW(pData->vertices[nInd0].pos, pData->vertices[nPrevInd].pos, pData->vertices[nInd].pos) )
							pData->triangles.push_back( STriangle(nInd0, nPrevInd, nInd) );
						else
							pData->triangles.push_back( STriangle(nInd0, nInd, nPrevInd) );

						nPrevInd = nInd;
					}
				}
			}
		}
	}
}

void CTerraGen::ProjectRoadLayer( const NDb::SVSOPoint &point1, const NDb::SVSOPoint &point2,
																	const float fMinTexX, const float fMaxTexX, const float fPrevDist,
																	const float fNextTexY, const float fTexY, NMeshData::SMeshData *pCurPatch,
																	const float fOrgTexX, const float fTexXD )
{
	const float w1 = ( fMinTexX - fOrgTexX ) * fTexXD * 2.0f - 1.0f;
	const float w2 = ( fMaxTexX - fOrgTexX ) * fTexXD * 2.0f - 1.0f;

	const CVec3 vCurL = point1.vPos + point1.vNorm * point1.fWidth * w1;
	const CVec3 vCurR = point1.vPos + point1.vNorm * point1.fWidth * w2;
	const CVec3 vNextL = point2.vPos + point2.vNorm * point2.fWidth * w1;
	const CVec3 vNextR = point2.vPos + point2.vNorm * point2.fWidth * w2;

	const bool bTrgInters = IsIntersectOneOf( vNextL, vCurL, vCurR, vNextR ) ||
													IsIntersectOneOf( vCurL, vCurR, vNextR, vNextL );

	if ( bTrgInters )
	{
		ProjectTrgOnTerrain( vCurL, vCurR, vNextR, CVec2(fMinTexX, fTexY), CVec2(fMaxTexX, fTexY), CVec2(fMaxTexX, fNextTexY),
												 point1.fOpacity, point1.fOpacity, point2.fOpacity, pCurPatch );
		ProjectTrgOnTerrain( vNextR, vNextL, vCurL, CVec2(fMaxTexX, fNextTexY), CVec2(fMinTexX, fNextTexY), CVec2(fMinTexX, fTexY),
												 point2.fOpacity, point2.fOpacity, point1.fOpacity, pCurPatch );
	}
	else
		ProjectQuadOnTerrain( vCurL, vCurR, vNextR, vNextL, CVec2(fMinTexX, fTexY), CVec2(fMaxTexX, fTexY),
													CVec2(fMaxTexX, fNextTexY), CVec2(fMinTexX, fNextTexY),
													point1.fOpacity, point1.fOpacity, point2.fOpacity, point2.fOpacity, pCurPatch );
	/*return;

	const int nStep = max( int( ( point1.fWidth + point2.fWidth ) * fabs( w1 - w2 ) * 0.5f * 2.0f / fPrevDist ), 2 ) - 1;
	const float fInvStep = 1.0f / nStep;

	for ( int k = 0; k < nStep; ++k )
	{
		const CVec3 v1 = vNextL + ( vNextR - vNextL ) * fInvStep * k;
		const CVec3 v2 = vCurL + ( vCurR - vCurL ) * fInvStep * k;
		const CVec3 v3 = vCurL + ( vCurR - vCurL ) * fInvStep * ( k + 1 );
		const CVec3 v4 = vNextL + ( vNextR - vNextL ) * fInvStep * ( k + 1 );
		const float fTexX1 = fMinTexX + ( fMaxTexX - fMinTexX ) * fInvStep * k;
		const float fTexX2 = fMinTexX + ( fMaxTexX - fMinTexX ) * fInvStep * ( k + 1 );
		const float fAlpha1 = point1.fOpacity;
		const float fAlpha2 = point2.fOpacity;

		if ( bTrgInters )
		{
			ProjectTrgOnTerrain( v1, v2, v3, CVec2( fTexX1, fNextTexY ), CVec2( fTexX1, fTexY ), CVec2( fTexX2, fTexY ),
				fAlpha2, fAlpha1, fAlpha1, curPatch );
			ProjectTrgOnTerrain( v3, v4, v1,	CVec2( fTexX2, fTexY ), CVec2( fTexX2, fNextTexY ), CVec2( fTexX1, fNextTexY ),
				fAlpha1, fAlpha2, fAlpha2, curPatch );
		}
		else
			ProjectQuadOnTerrain( v1, v2, v3, v4, CVec2( fTexX1, fNextTexY ), CVec2( fTexX1, fTexY ),
			CVec2( fTexX2, fTexY ), CVec2( fTexX2, fNextTexY ), fAlpha2, fAlpha1, fAlpha1, fAlpha2, curPatch );

		//ProjectQuadOnTerrain( vNextL, vCurL, vCurR, vNextR, CVec2( fMinTexX, fNextTexY ), CVec2( fMinTexX, fTexY ),
		//	CVec2( fMaxTexX, fTexY ), CVec2( fMaxTexX, fNextTexY ), points[g + 1].fOpacity, points[g].fOpacity,
		//	points[g].fOpacity, points[g + 1].fOpacity, curPatch );
	}*/
}

inline void ReservePatchData( NMeshData::SMeshData *pPatch )
{
	pPatch->vertices.reserve( 256 );
	pPatch->vertices.resize( 0 );
	pPatch->triangles.reserve( 256 );
	pPatch->triangles.resize( 0 );
}

inline void TryAddRoadGfxToObserver( STerrainGfxInfo *pTerraGfxInfo, const SRoadGFXInfo &gfxInfo, ITerraGfxObserver *pGfxObserver )
{
	int nSize = 0;

	for ( std::vector<NMeshData::SMeshData>::const_iterator it = gfxInfo.patches.begin(); it != gfxInfo.patches.end(); ++it )
		nSize += it->triangles.size();

	if ( nSize > 0 )
	{
		pTerraGfxInfo->roads.push_back( gfxInfo );
		if ( pGfxObserver )
			pGfxObserver->AddRoad( &(pTerraGfxInfo->roads.back()) );
	}
}

void CTerraGen::CreateRoadGfx( const NDb::SVSOInstance *pInstance, const std::vector<NDb::SVSOPoint> &points )
{
	// check, that such road GFX isn't exists
	for ( std::list<SRoadGFXInfo>::const_iterator it = terrainGfxInfo.roads.begin(); it != terrainGfxInfo.roads.end(); ++it )
	{
		if ( it->nID == pInstance->nVSOID )
		{
			//NI_ASSERT( it->nID != pInstance->nVSOID, "Such road already exists" );
			return;
		}
	}

	const int nPatchesNum = ( ((points.size() - 1) % DEF_ROAD_SAMPS_PER_PATCH) == 0 ) ?
		( (points.size() - 1) / DEF_ROAD_SAMPS_PER_PATCH ) : ( ((points.size() - 1) / DEF_ROAD_SAMPS_PER_PATCH) + 1 );

	SRoadGFXInfo gfxInfo, leftGfxInfo, rightGfxInfo;
	gfxInfo.nID = pInstance->nVSOID;
	gfxInfo.pDesc = static_cast<const NDb::SRoadDesc *>(pInstance->pDescriptor.GetPtr());
	gfxInfo.patches.resize( nPatchesNum );
	gfxInfo.ePart = SRoadGFXInfo::CENTER;

	leftGfxInfo.nID = gfxInfo.nID;
	leftGfxInfo.pDesc = gfxInfo.pDesc;
	leftGfxInfo.patches.resize( nPatchesNum );
	leftGfxInfo.ePart = SRoadGFXInfo::LEFT_BORDER;

	rightGfxInfo.nID = gfxInfo.nID;
	rightGfxInfo.pDesc = gfxInfo.pDesc;
	rightGfxInfo.patches.resize( nPatchesNum );
	rightGfxInfo.ePart = SRoadGFXInfo::RIGHT_BORDER;

	const int nCenterTexWidth = gfxInfo.pDesc->center.materials[0]->pTexture->nWidth;
	const float fMinTexX = (float)gfxInfo.pDesc->center.nUseFromPixel / nCenterTexWidth;
	const float fMaxTexX = (float)gfxInfo.pDesc->center.nUseToPixel / nCenterTexWidth;

	float fGlobalMinTexX = min( fMinTexX, fMaxTexX );
	float fGlobalMaxTexX = max( fMinTexX, fMaxTexX );

	float fLeftMinTexX = 0.0f, fLeftMaxTexX = 0.0f;
	if ( gfxInfo.pDesc->leftBorder.pMaterial )
	{
		const int nLeftTexWidth = gfxInfo.pDesc->leftBorder.pMaterial->pTexture->nWidth;
		fLeftMinTexX = (float)gfxInfo.pDesc->leftBorder.nUseFromPixel / nLeftTexWidth;
		fLeftMaxTexX = (float)gfxInfo.pDesc->leftBorder.nUseToPixel / nLeftTexWidth;
		fGlobalMinTexX = min( fGlobalMinTexX, min(fLeftMinTexX, fLeftMaxTexX) );
		fGlobalMaxTexX = max( fGlobalMaxTexX, max(fLeftMinTexX, fLeftMaxTexX) );
	}

	float fRightMinTexX = 0.0f, fRightMaxTexX = 0.0f;
	if ( gfxInfo.pDesc->rightBorder.pMaterial )
	{
		const int nRightTexWidth = gfxInfo.pDesc->rightBorder.pMaterial->pTexture->nWidth;
		fRightMinTexX = (float)gfxInfo.pDesc->rightBorder.nUseFromPixel / nRightTexWidth;
		fRightMaxTexX = (float)gfxInfo.pDesc->rightBorder.nUseToPixel / nRightTexWidth;
		fGlobalMinTexX = min( fGlobalMinTexX, min(fRightMinTexX, fRightMaxTexX) );
		fGlobalMaxTexX = max( fGlobalMaxTexX, max(fRightMinTexX, fRightMaxTexX) );
	}

	const float fTexCoeffX = fGlobalMaxTexX - fGlobalMinTexX;
	const float fInvTexCoeffX = 1.0f / fTexCoeffX;

	float fTexY = 0.0f;

	for ( int i = 0; i < nPatchesNum; ++i )
	{
		const int nFirstInd = i * DEF_ROAD_SAMPS_PER_PATCH;
		const int nLastInd = min( ( i + 1 ) * DEF_ROAD_SAMPS_PER_PATCH, points.size() - 1 );

		ReservePatchData( &gfxInfo.patches[i] );
		ReservePatchData( &leftGfxInfo.patches[i] );
		ReservePatchData( &rightGfxInfo.patches[i] );

		for ( int g = nFirstInd; g < nLastInd; ++g )
		{
			const float fPrevDist = fabs( points[g].vPos - points[g + 1].vPos );
			const float fNextTexY = fTexY + fPrevDist / ( points[g + 1].fWidth * 2.0f ) * fTexCoeffX;

			ProjectRoadLayer( points[g], points[g + 1], fMinTexX, fMaxTexX, fPrevDist, fNextTexY, fTexY, &(gfxInfo.patches[i]),
												fGlobalMinTexX, fInvTexCoeffX );

			if ( gfxInfo.pDesc->leftBorder.pMaterial )
				ProjectRoadLayer( points[g], points[g + 1], fLeftMinTexX, fLeftMaxTexX, fPrevDist, fNextTexY, fTexY, &(leftGfxInfo.patches[i]),
													fGlobalMinTexX, fInvTexCoeffX );

			if ( gfxInfo.pDesc->rightBorder.pMaterial )
				ProjectRoadLayer( points[g], points[g + 1], fRightMinTexX, fRightMaxTexX, fPrevDist, fNextTexY, fTexY, &(rightGfxInfo.patches[i]),
													fGlobalMinTexX, fInvTexCoeffX );

			fTexY = fNextTexY;
		}
	}

	TryAddRoadGfxToObserver( &terrainGfxInfo, gfxInfo, pGfxObserver );

	if ( gfxInfo.pDesc->leftBorder.pMaterial )
		TryAddRoadGfxToObserver( &terrainGfxInfo, leftGfxInfo, pGfxObserver );

	if ( gfxInfo.pDesc->rightBorder.pMaterial )
		TryAddRoadGfxToObserver( &terrainGfxInfo, rightGfxInfo, pGfxObserver );
}

void CTerraGen::AddRoad( const NDb::SVSOInstance *pInstance )
{
	//check if there already exist such road
	for ( std::list<STerrainInfo::SRoad>::const_iterator it = terrainInfo.roads.begin(); it != terrainInfo.roads.end(); ++it )
	{
		if ( it->nID == pInstance->nVSOID )
		{
			//NI_ASSERT( it->nID != pInstance->nVSOID, StrFmt("Road %d already exists", pInstance->nVSOID) );
			return;
		}
	}

	if ( pInstance->points.size() < 2 )
		return;

	STerrainInfo::SRoad curRoadInfo;
	curRoadInfo.nID = pInstance->nVSOID;

	std::vector<NDb::SVSOPoint> points;
	ConvertVSOPointsFromAIToVisAndPutOnTerrain( &points, pInstance->points );

	curRoadInfo.vBBMin.Set( FP_MAX_VALUE, FP_MAX_VALUE );
	curRoadInfo.vBBMax.Set( -FP_MAX_VALUE, -FP_MAX_VALUE );
	for ( std::vector<NDb::SVSOPoint>::const_iterator it = points.begin(); it != points.end(); ++it )
	{
		curRoadInfo.vBBMin.Minimize( CVec2(it->vPos.x, it->vPos.y) );
		curRoadInfo.vBBMax.Maximize( CVec2(it->vPos.x, it->vPos.y) );
	}

	terrainInfo.roads.push_back( curRoadInfo );

	CreateRoadGfx( pInstance, points );

	if ( pAIObserver )
		PutRoadToAI( pInstance );
}

void CTerraGen::UpdateRoad( const int nVSOID )
{
	RemoveRoad( nVSOID );
	for ( std::vector<NDb::SVSOInstance>::const_iterator it = pDesc->roads.begin(); it != pDesc->roads.end(); ++it )
	{
		if ( it->nVSOID == nVSOID )
		{
			AddRoad( &(*it) );
			return;
		}
	}
	NI_ASSERT( false, "Updated road does not exists" );
}

void CTerraGen::RemoveRoad( const int nVSOID )
{
	for ( std::list<STerrainInfo::SRoad>::iterator it = terrainInfo.roads.begin(); it != terrainInfo.roads.end(); ++it )
	{
		if ( it->nID == nVSOID )
		{
			terrainInfo.roads.erase( it );
			break;
		}
	}

	for ( std::list<SRoadGFXInfo>::iterator it = terrainGfxInfo.roads.begin(); it != terrainGfxInfo.roads.end(); )
	{
		if ( it->nID == nVSOID )
			it = terrainGfxInfo.roads.erase( it );
		else
			++it;
	}

	if ( pGfxObserver )
		pGfxObserver->RemoveRoad( nVSOID );
}

void CTerraGen::AddAllRoads()
{
	NI_ASSERT( pDesc, "Terrain is not loaded" );
	RemoveAllRoads();
	for ( std::vector<NDb::SVSOInstance>::const_iterator it = pDesc->roads.begin(); it != pDesc->roads.end(); ++it )
	{
		AddRoad( &(*it) );
	}
}

void CTerraGen::RemoveAllRoads()
{
	NI_ASSERT( pDesc, "Terrain is not loaded" );
	if ( pGfxObserver )
	{
		for ( std::list<STerrainInfo::SRoad>::const_iterator it = terrainInfo.roads.begin(); it != terrainInfo.roads.end(); ++it )
		{
			pGfxObserver->RemoveRoad( it->nID );
		}
	}
	terrainGfxInfo.roads.clear();
	terrainInfo.roads.clear();
}

const NDb::SVSOInstance* CTerraGen::FindRoad( int nID ) const
{
	for ( std::vector<NDb::SVSOInstance>::const_iterator it = pDesc->roads.begin(); it != pDesc->roads.end(); ++it )
	{
		if ( it->nVSOID == nID )
			return static_cast<const NDb::SVSOInstance *>( &(*it) );
	}
	//NI_ASSERT( false, StrFmt( "Couldn't find road's instance: ID=%d", nID ) );
	return 0;
}

/*static void ProjectRoad( const vector<NDb::SVSOPoint> &samples, const float fMinTexX, const float fMaxTexX, const float fOrgTexX,
												 const float fTexXD, const float fTexCoeffX, const CArray2D<STerrainInfo::STile> &tiles )
{
	if ( samples.size() < 2 )
		return;

	const float w1 = ( fMinTexX - fOrgTexX ) * fTexXD * 2.0f - 1.0f;
	const float w2 = ( fMaxTexX - fOrgTexX ) * fTexXD * 2.0f - 1.0f;

	vector<CVec2> roadPoints( 256 );
	roadPoints.resize( samples.size() * 2 );
	const int nLastSize = samples.size() * 2 - 1;

	for ( int i = 0; i < samples.size(); ++i )
	{
		const NDb::SVSOPoint &vso = samples[i];
		const float dx = vso.vNorm.x * vso.fWidth;
		const float dy = vso.vNorm.y * vso.fWidth;
		roadPoints[i].Set( vso.vPos.x + dx * w1, vso.vPos.y + dy * w1 );
		roadPoints[nLastSize - i].Set( vso.vPos.x + dx * w2, vso.vPos.y + dy * w2 );
	}

	vector<float> texYCoords( samples.size() );
	texYCoords[0] = 0.0f;
	for ( int i = 1; i < samples.size(); ++i )
	{
		const float fPrevDist = fabs( samples[i].vPos - samples[i + 1].vPos );
		texYCoords[i] = texYCoords[i + 1] + fPrevDist / ( samples[i + 1].fWidth * 2.0f ) * fTexCoeffX;
	}

	CArray2D<vector<int> > tileToQuads( tiles.GetSizeX(), tiles.GetSizeY() );
	int nMinTileX = INT_MAX, nMinTileY = INT_MAX, nMaxTileX = INT_MIN, nMaxTileY = INT_MIN;

	for ( int i = 0; i < ( samples.size() - 1 ); ++i )
	{
		const CVec2 &v1 = roadPoints[i];
		const CVec2 &v2 = roadPoints[nLastSize - i];
		const CVec2 &v3 = roadPoints[nLastSize - (i+1)];
		const CVec2 &v4 = roadPoints[i+1];

		const int nTMinX = min( int( min( min( v1.x, v2.x ), min( v3.x, v4.x ) ) * DEF_INV_TILE_SIZE ), 0 );
		const int nTMinY = min( int( min( min( v1.y, v2.y ), min( v3.y, v4.y ) ) * DEF_INV_TILE_SIZE ), 0 );
		const int nTMaxX = max( int( max( max( v1.x, v2.x ), max( v3.x, v4.x ) ) * DEF_INV_TILE_SIZE ), 0 );
		const int nTMaxY = max( int( max( max( v1.y, v2.y ), max( v3.y, v4.y ) ) * DEF_INV_TILE_SIZE ), 0 );

		for ( int y = nTMinX; y <= nTMaxX; ++y )
		{
			for ( int x = nTMinY; x <= nTMaxY; ++x )
			{
				tileToQuads[y][x].push_back( i );
			}
		}

		nMinTileX = min( nMinTileX, nTMinX );
		nMinTileY = min( nMinTileY, nTMinY );
		nMaxTileX = max( nMaxTileX, nTMaxX );
		nMaxTileY = max( nMaxTileY, nTMaxY );
	}

	vector<CVec2> vertTexCoords;
	CVec2 vBary;

	for ( int g = nMinTileY; g <= nMaxTileY; ++g )
	{
		for ( int i = nMinTileX; i <= nMaxTileX; ++i )
		{
			if ( !( tileToQuads[g][i].empty() ) )
			{
				vector<int> &useQuads = tileToQuads[g][i];
				const STerrainInfo::STile &tile = tiles[g][i];
				vertTexCoords.resize( tile.vertices.size() );
				fill( vertTexCoords.begin(), vertTexCoords.end(), CVec2(-1,-1) );
				for ( int k = 0; k < tile.vertices.size(); ++k )
				{
					for ( int t = 0; t < useQuads.size(); ++t )
					{
						const int &nInd = useQuads[t];
						const CVec3 &v1 = roadPoints[nInd];
						const CVec3 &v2 = roadPoints[nLastSize - nInd];
						const CVec3 &v3 = roadPoints[nLastSize - (nInd+1)];
						const CVec3 &v4 = roadPoints[nInd+1];

						GetBaryCoords( v1, v2, v4, &vBary );
						if ( IsBaryIn( vBary ) )
						{
							vertTexCoords[k].Set( fMinTexX + ( fMaxTexX - fMinTexX ) * vBary.x,
								texYCoords[nInd] + ( texYCoords[nInd+1] - texYCoords[nInd] ) * vBary.y );
							break;
						}

						GetBaryCoords( v3, v4, v2, &vBary );
						if ( IsBaryIn( vBary ) )
						{
							vertTexCoords[k].Set( fMaxTexX + ( fMinTexX - fMaxTexX ) * vBary.x,
								texYCoords[nInd+1] + ( texYCoords[nInd] - texYCoords[nInd+1] ) * vBary.y );
							break;
						}
					}
				}

				for ( vector<STriangle>::const_iterator it = tile.triangles.begin(); it != tile.triangles.end(); ++it )
				{
					const uint8_t &b1 = vertTexCoords[it->i1].x > -0.5f;
					const uint8_t &b2 = vertTexCoords[it->i2].x > -0.5f;
					const uint8_t &b3 = vertTexCoords[it->i3].x > -0.5f;
					if ( b1 && b2 && b3 )
					{
					}
				}
			}
		}
	}
}*/


