#include "StdAfx.h"

#include "GenTerrain.h"

#define DEF_VERTICES_NUM_ALLOC 256
#define DEF_TRIANGLES_NUM_ALLOC 256
#define DEF_DEBRIS_HEIGHT 0.1f
#define DEF_DEBRIS_TEX_SCALE 0.04f

inline int AddUniqueDebrisVertex( NMeshData::SMeshDataTex2 &data, const CVec3 &vert, const int nTileX, const int nTileY,
																	const CVec3 &vNorm1, const CVec3 &vNorm2, const CVec3 &vNorm3, const CVec3 &vNorm4,
																	const CVec2 &vTex, NGScene::SVertex &templ, const float fDistX, const float fDistY )
{
	int nCount = 0;
	for ( vector<NGScene::SVertex>::const_iterator it = data.vertices.begin(); it != data.vertices.end(); ++it, ++nCount )
	{
		if ( ( fabs2( it->pos.x - vert.x ) + fabs2( it->pos.y - vert.y ) + fabs2( it->pos.z - vert.z ) ) < DEF_FLOAT_EPS2 )
			return nCount;
	}
	templ.pos = vert;
	templ.tex.Set( fDistX * vTex.x * DEF_DEBRIS_TEX_SCALE, fDistY * vTex.y * DEF_DEBRIS_TEX_SCALE );
	const float fDx = Clamp( ( vert.x - (float)nTileX * DEF_TILE_SIZE ) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	const float fDy = Clamp( ( vert.y - (float)nTileY * DEF_TILE_SIZE ) * DEF_INV_TILE_SIZE, 0.0f, 1.0f );
	CalcCompactVector( &(templ.normal),
		( vNorm1 + ( vNorm2 - vNorm1 ) * fDx ) * ( 1.0f - fDy ) + ( vNorm4 + ( vNorm3 - vNorm4 ) * fDx ) * fDy );
	data.vertices.push_back( templ );
	data.secondTex.push_back( vTex );
	return ( data.vertices.size() - 1 );
}

void CTerraGen::AddDynamicDebris( const CVec2 &vPos, const CVec2 &vSize, const float fAngle, const int nSmoothRad, const NDb::SMaterial *pMaterial )
{
	const float fExtCoeff = (float)nSmoothRad / ( DEF_DYNAMIC_DEBRIS_TEX_SIZE - nSmoothRad * 2 ) + 0.5f;

	const CVec2 _v1( vPos.x - vSize.x * fExtCoeff, vPos.y - vSize.y * fExtCoeff );
	const CVec2 _v2( vPos.x - vSize.x * fExtCoeff, vPos.y + vSize.y * fExtCoeff );
	const CVec2 _v3( vPos.x + vSize.x * fExtCoeff, vPos.y + vSize.y * fExtCoeff );
	const CVec2 _v4( vPos.x + vSize.x * fExtCoeff, vPos.y - vSize.y * fExtCoeff );

	const float fCos = cos( fAngle );
	const float fSin = sin( fAngle );

	CVec3 v1( (float)( _v1.x - vPos.x ) * fCos - (float)( _v1.y - vPos.y ) * fSin + vPos.x,
						(float)( _v1.x - vPos.x ) * fSin + (float)( _v1.y - vPos.y ) * fCos + vPos.y, 0.0f );
	CVec3 v2( (float)( _v2.x - vPos.x ) * fCos - (float)( _v2.y - vPos.y ) * fSin + vPos.x,
						(float)( _v2.x - vPos.x ) * fSin + (float)( _v2.y - vPos.y ) * fCos + vPos.y, 0.0f );
	CVec3 v3( (float)( _v3.x - vPos.x ) * fCos - (float)( _v3.y - vPos.y ) * fSin + vPos.x,
						(float)( _v3.x - vPos.x ) * fSin + (float)( _v3.y - vPos.y ) * fCos + vPos.y, 0.0f );
	CVec3 v4( (float)( _v4.x - vPos.x ) * fCos - (float)( _v4.y - vPos.y ) * fSin + vPos.x,
						(float)( _v4.x - vPos.x ) * fSin + (float)( _v4.y - vPos.y ) * fCos + vPos.y, 0.0f );

	AI2Vis( &v1 );
	AI2Vis( &v2 );
	AI2Vis( &v3 );
	AI2Vis( &v4 );

	const float fDist12 = fabs( v1 - v2 );
	const float fDist14 = fabs( v1 - v4 );

	vector<CVec3> coeffs(4);
	GetLineEq( v1.x, v1.y, v2.x, v2.y, &(coeffs[0].x), &(coeffs[0].y), &(coeffs[0].z) );
	GetLineEq( v2.x, v2.y, v3.x, v3.y, &(coeffs[1].x), &(coeffs[1].y), &(coeffs[1].z) );
	GetLineEq( v3.x, v3.y, v4.x, v4.y, &(coeffs[2].x), &(coeffs[2].y), &(coeffs[2].z) );
	GetLineEq( v4.x, v4.y, v1.x, v1.y, &(coeffs[3].x), &(coeffs[3].y), &(coeffs[3].z) );

	const int nTileX1 = Clamp( int( min( min(v1.x, v2.x), min(v3.x, v4.x) ) * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY1 = Clamp( int( min( min(v1.y, v2.y), min(v3.y, v4.y) ) * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeY() - 1 );
	const int nTileX2 = Clamp( int( max( max(v1.x, v2.x), max(v3.x, v4.x) ) * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY2 = Clamp( int( max( max(v1.y, v2.y), max(v3.y, v4.y) ) * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeY() - 1 );

	NMeshData::SMeshDataTex2 data;

	data.vertices.reserve( DEF_VERTICES_NUM_ALLOC );
	data.vertices.resize( 0 );
	data.secondTex.reserve( DEF_VERTICES_NUM_ALLOC );
	data.secondTex.resize( 0 );
	data.triangles.reserve( DEF_TRIANGLES_NUM_ALLOC );
	data.triangles.resize( 0 );

	CVec2 bary;
	NGScene::SVertex templ;
	CalcCompactVector( &(templ.texU), CVec3(1, 0, 0) );
	CalcCompactVector( &(templ.texV), CVec3(0, 1, 0) );

	static vector<CVec3> points(16);
	static vector<CVec3> tempPoints(16);
	static vector<CVec3> resPoints(16);

	int nPrevInd;

	for ( int g = nTileY1; g <= nTileY2; ++g )
	{
		for ( int i = nTileX1; i <= nTileX2; ++i )
		{
			const STerrainInfo::STile &tile = terrainInfo.tiles[g][i];
			for ( vector<STriangle>::const_iterator it = tile.triangles.begin(); it != tile.triangles.end(); ++it )
			{
				const CVec3 vert1( tile.vertices[it->i1].x, tile.vertices[it->i1].y,
					max( tile.vertices[it->i1].z + tile.addHeights[it->i1], 0.0f ) + DEF_DEBRIS_HEIGHT );
				const CVec3 vert2( tile.vertices[it->i2].x, tile.vertices[it->i2].y,
					max( tile.vertices[it->i2].z + tile.addHeights[it->i2], 0.0f ) + DEF_DEBRIS_HEIGHT );
				const CVec3 vert3( tile.vertices[it->i3].x, tile.vertices[it->i3].y,
					max( tile.vertices[it->i3].z + tile.addHeights[it->i3], 0.0f ) + DEF_DEBRIS_HEIGHT );
				const bool bFlag1 = IsInside( coeffs, vert1 );
				const bool bFlag2 = IsInside( coeffs, vert2 );
				const bool bFlag3 = IsInside( coeffs, vert3 );
				if ( bFlag1 && bFlag2 && bFlag3 )
				{
					GetBaryCoords( vert1, v1, v2, v4, &bary );
					const int nInd1 = AddUniqueDebrisVertex( data, vert1, i, g, terrainNorms[g][i], terrainNorms[g][i+1],
						terrainNorms[g+1][i+1], terrainNorms[g+1][i], bary, templ, fDist12, fDist14 );
					GetBaryCoords( vert2, v1, v2, v4, &bary );
					const int nInd2 = AddUniqueDebrisVertex( data, vert2, i, g, terrainNorms[g][i], terrainNorms[g][i+1],
						terrainNorms[g+1][i+1], terrainNorms[g+1][i], bary, templ, fDist12, fDist14 );
					GetBaryCoords( vert3, v1, v2, v4, &bary );
					const int nInd3 = AddUniqueDebrisVertex( data, vert3, i, g, terrainNorms[g][i], terrainNorms[g][i+1],
						terrainNorms[g+1][i+1], terrainNorms[g+1][i], bary, templ, fDist12, fDist14 );
					data.triangles.push_back( STriangle( nInd1, nInd2, nInd3 ) );
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
					for ( vector<CVec3>::const_iterator it = points.begin(); it != points.end(); ++it )
					{
						vector<CVec3>::const_iterator itTemp = tempPoints.begin();
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

					vector<CVec3>::const_iterator itResPoint = resPoints.begin();
					GetBaryCoords( *itResPoint, v1, v2, v4, &bary );
					const int nInd0 = AddUniqueDebrisVertex( data, *itResPoint, i, g, terrainNorms[g][i], terrainNorms[g][i+1],
						terrainNorms[g+1][i+1], terrainNorms[g+1][i], bary, templ, fDist12, fDist14 );
					++itResPoint;
					GetBaryCoords( *itResPoint, v1, v2, v4, &bary );
					nPrevInd = AddUniqueDebrisVertex( data, *itResPoint, i, g, terrainNorms[g][i], terrainNorms[g][i+1],
						terrainNorms[g+1][i+1], terrainNorms[g+1][i], bary, templ, fDist12, fDist14 );
					++itResPoint;

					for ( ; itResPoint != resPoints.end(); ++itResPoint )
					{
						GetBaryCoords( *itResPoint, v1, v2, v4, &bary );
						const int nInd = AddUniqueDebrisVertex( data, *itResPoint, i, g, terrainNorms[g][i], terrainNorms[g][i+1],
							terrainNorms[g+1][i+1], terrainNorms[g+1][i], bary, templ, fDist12, fDist14 );

						if ( IsCCW( data.vertices[nInd0].pos, data.vertices[nPrevInd].pos, data.vertices[nInd].pos ) )
							data.triangles.push_back( STriangle( nInd0, nPrevInd, nInd ) );
						else
							data.triangles.push_back( STriangle( nInd0, nInd, nPrevInd ) );

						nPrevInd = nInd;
					}
				}
			}
		}
	}

	dynamicDebrisManager.AddDynamicDebris( data, pMaterial );
}


