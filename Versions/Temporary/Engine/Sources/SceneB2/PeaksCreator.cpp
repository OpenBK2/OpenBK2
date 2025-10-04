#include "stdafx.h"

#include "3Dmotor/DBScene.h"
#include "GenTerrain.h"

#define DEF_PENDENT_WIDTH ( DEF_TILE_SIZE * 0.1125f )
#define DEF_PENDENT_PART1_NUM 3
#define DEF_PENDENT_PART2_NUM 2
#define DEF_PENDENT_NUM ( DEF_PENDENT_PART1_NUM + DEF_PENDENT_PART2_NUM )
#define DEF_PENDENT_SECTEX_COEFF ( 1.0f / DEF_PENDENT_NUM )
#define DEF_PENDENT_SECTEX_X_COEFF ( 1.0f / ( DEF_PENDENT_WIDTH * DEF_PENDENT_NUM ) )
#define DEF_PEAK_PENDENT_MAX_ANGLE ( FP_PI4 )
#define DEF_PENDENT_PART1_ANGLE_COEFF ( DEF_PEAK_PENDENT_MAX_ANGLE / ( DEF_PENDENT_PART1_NUM - 1 ) )
#define DEF_PEAK_PATCH_SIZE 8

inline void CheckTileInsidingAndPush( vector<CVec3> *pInters, const CVec3 &v, const float x1, const float y1, const float x2, const float y2 )
{
	if ( (v.x > (x1 - DEF_EPS)) && (v.x < (x2 + DEF_EPS)) && (v.y > (y1 - DEF_EPS)) && (v.y < (y2 + DEF_EPS)) )
		pInters->push_back( v );
}

inline void CheckTrgInsidingAndPush( vector<CVec3> *pInters, const CVec3 &v, const CVec3 &v1, const CVec3 &v2, const CVec3 &v3 )
{
	CVec2 vBary;
	GetBaryCoords( v, v1, v2, v3, &vBary );
	if ( (vBary.x > -DEF_EPS) && (vBary.y > -DEF_EPS) && ((vBary.x + vBary.y) < (1.0f + DEF_EPS)) )
		AddUnique( pInters, v );
}

inline void CheckVerticalIntersectAndPush( vector<CVec3> *pInters, const CVec3 &v1, const CVec3 &v2,
																					 const float fY, const float fX1, const float fX2 )
{
	if ( fabs(v2.y - v1.y) > EPS_VALUE )
	{
		const float t = ( fY - v1.y ) / ( v2.y - v1.y );
		if ( (t > -DEF_EPS) && (t < (1.0f + DEF_EPS)) )
		{
			const float x = v1.x + ( v2.x - v1.x ) * t;
			if ( (x > (fX1 - DEF_EPS)) && (x < (fX2 + DEF_EPS)) )
			{
				PushBackUnique( pInters, CVec3(x, fY, v1.z + (v2.z - v1.z) * t) );
			}
		}
	}
}

inline void CheckHorizontalIntersectAndPush( vector<CVec3> *pInters, const CVec3 &v1, const CVec3 &v2,
																						 const float fX, const float fY1, const float fY2 )
{
	if ( fabs(v2.x - v1.x) > EPS_VALUE )
	{
		const float t = ( fX - v1.x ) / ( v2.x - v1.x );
		if ( (t > -DEF_EPS) && (t < (1.0f + DEF_EPS)) )
		{
			const float y = v1.y + ( v2.y - v1.y ) * t;
			if ( (y > (fY1 - DEF_EPS)) && (y < (fY2 + DEF_EPS)) )
			{
				PushBackUnique( pInters, CVec3(fX, y, v1.z + (v2.z - v1.z) * t) );
			}
		}
	}
}

inline void InterpolateHeight( CVec3 *pV, const CVec3 &v1, const CVec3 &v2, const CVec3 &v3, const CVec2 &vTex )
{
	pV->z = ( v2.z - v1.z ) * vTex.x + ( v3.z - v1.z ) * vTex.y + v1.z;
}

inline void InterpolateSecTexCoords( CVec2 *pSecTex, const CVec2 &vSecTex1, const CVec2 &vSecTex2, const CVec2 &vSecTex3, const CVec2 &vBary )
{
	pSecTex->x = ( vSecTex2.x - vSecTex1.x ) * vBary.x + ( vSecTex3.x - vSecTex1.x ) * vBary.y + vSecTex1.x;
	pSecTex->y = ( vSecTex2.y - vSecTex1.y ) * vBary.x + ( vSecTex3.y - vSecTex1.y ) * vBary.y + vSecTex1.y;
}

inline void InterpolateRealPos( CVec3 *pPos, const CVec3 &vRealPos1, const CVec3 &vRealPos2, const CVec3 &vRealPos3, const CVec2 &vBary )
{
	(*pPos) = ( vRealPos2 - vRealPos1 ) * vBary.x + ( vRealPos3 - vRealPos1 ) * vBary.y + vRealPos1;
}

inline void InterpolateNorm( CVec3 *pNorm, const CVec3 &vNorm1, const CVec3 &vNorm2, const float t )
{
	(*pNorm) = vNorm1 + ( vNorm2 - vNorm1 ) * t;
	if ( fabs2(*pNorm) > DEF_EPS )
		Normalize( pNorm );
	else
		(*pNorm) = vNorm1;
}

inline CVec3 GetInterpolateNorm( const CVec3 &n1, const CVec3 &n2, const float t )
{
	CVec3 n = n1 + ( n2 - n1 ) * t;
	if ( fabs2( n ) > EPS_VALUE )
		Normalize( &n );
	else
		n = n1;
	return n;
}

void CTerraGen::PeaksProjectTrgOnTiles( vector<NMeshData::SMeshDataTex2> *pDataArr, const CVec3 &v1, const CVec3 &v2, const CVec3 &v3,
																	 const CVec2 &vSecTex1, const CVec2 &vSecTex2, const CVec2 &vSecTex3,
																	 const CVec3 &vRealPos1, const CVec3 &vRealPos2, const CVec3 &vRealPos3,
																	 const CVec3 &vNorm1, const CVec3 &vNorm2, const CVec3 &vNorm3,
																	 const CVec3 &vBase1, const CVec3 &vBase2, const CVec3 &vBase3 )
{
	const int nTileX1 = Clamp( int(min(min(v1.x, v2.x), v3.x) * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY1 = Clamp( int(min(min(v1.y, v2.y), v3.y) * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );

	AddTileTriangle( 0, pDataArr, v1, v2, v3, vSecTex1, vSecTex2, vSecTex3, nTileX1, nTileY1,
									 nTileX1 >> DEF_PATCH_SIZE_BITS, nTileY1 >> DEF_PATCH_SIZE_BITS, &vRealPos1, &vRealPos2, &vRealPos3,
									 &vNorm1, &vNorm2, &vNorm3, false );

	/*return;

	const int nTileX2 = Clamp( int( max( max( v1.x, v2.x ), v3.x ) * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY2 = Clamp( int( max( max( v1.y, v2.y ), v3.y ) * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeY() - 1 );

	static vector<CVec3> inters( 16 );
	static vector<CVec3> dstVerts( 16 );
	static CVec3 tileVerts[4];

	CVec3 p1, p2, p, vPos1, vPos2, vPos, vN1, vN2, vN;
	CVec2 vTex1, vTex2, vTex;
	CVec2 vBary;

	for ( int g = nTileY1; g <= nTileY2; ++g )
	{
		for ( int i = nTileX1; i <= nTileX2; ++i )
		{
			inters.resize( 0 );

			const float fTileX1 = i * DEF_TILE_SIZE;
			const float fTileY1 = g * DEF_TILE_SIZE;
			const float fTileX2 = ( i + 1 ) * DEF_TILE_SIZE;
			const float fTileY2 = ( g + 1 ) * DEF_TILE_SIZE;

			tileVerts[0].Set( fTileX1, fTileY1, 0 );
			tileVerts[1].Set( fTileX2, fTileY1, 0 );
			tileVerts[2].Set( fTileX2, fTileY2, 0 );
			tileVerts[3].Set( fTileX1, fTileY2, 0 );

			CheckTileInsidingAndPush( inters, v1, fTileX1, fTileY1, fTileX2, fTileY2 );
			CheckTileInsidingAndPush( inters, v2, fTileX1, fTileY1, fTileX2, fTileY2 );
			CheckTileInsidingAndPush( inters, v3, fTileX1, fTileY1, fTileX2, fTileY2 );

			for ( int k = 0; k < 4; ++k )
				CheckTrgInsidingAndPush( inters, tileVerts[k], v1, v2, v3 );

			CheckVerticalIntersectAndPush( inters, v1, v2, fTileY1, fTileX1, fTileX2 );
			CheckVerticalIntersectAndPush( inters, v2, v3, fTileY1, fTileX1, fTileX2 );
			CheckVerticalIntersectAndPush( inters, v3, v1, fTileY1, fTileX1, fTileX2 );
			CheckVerticalIntersectAndPush( inters, v1, v2, fTileY2, fTileX1, fTileX2 );
			CheckVerticalIntersectAndPush( inters, v2, v3, fTileY2, fTileX1, fTileX2 );
			CheckVerticalIntersectAndPush( inters, v3, v1, fTileY2, fTileX1, fTileX2 );

			CheckHorizontalIntersectAndPush( inters, v1, v2, fTileX1, fTileY1, fTileY2 );
			CheckHorizontalIntersectAndPush( inters, v2, v3, fTileX1, fTileY1, fTileY2 );
			CheckHorizontalIntersectAndPush( inters, v3, v1, fTileX1, fTileY1, fTileY2 );
			CheckHorizontalIntersectAndPush( inters, v1, v2, fTileX2, fTileY1, fTileY2 );
			CheckHorizontalIntersectAndPush( inters, v2, v3, fTileX2, fTileY1, fTileY2 );
			CheckHorizontalIntersectAndPush( inters, v3, v1, fTileX2, fTileY1, fTileY2 );

			if ( inters.size() >= 3 )
			{
				CreateConvexHull( inters, dstVerts );

				if ( dstVerts.size() >= 3 )
				{
					const int nPatchX = i >> DEF_PATCH_SIZE_BITS;
					const int nPatchY = g >> DEF_PATCH_SIZE_BITS;

					p1 = dstVerts[0];
					p2 = dstVerts[1];
					GetBaryCoords( p1, v1, v2, v3, &vBary );
					InterpolateHeight( p1, v1, v2, v3, vBary );
					InterpolateRealPos( vPos1, vRealPos1, vRealPos2, vRealPos3, vBary );
					GetBaryCoords( p1, vBase1, vBase2, vBase3, &vBary );
					InterpolateSecTexCoords( vTex1, vSecTex1, vSecTex2, vSecTex3, vBary );
					InterpolateNorm( vN1, vNorm1, vNorm2, vBary.x );

					GetBaryCoords( p2, v1, v2, v3, &vBary );
					InterpolateHeight( p2, v1, v2, v3, vBary );
					InterpolateRealPos( vPos2, vRealPos1, vRealPos2, vRealPos3, vBary );
					GetBaryCoords( p2, vBase1, vBase2, vBase3, &vBary );
					InterpolateSecTexCoords( vTex2, vSecTex1, vSecTex2, vSecTex3, vBary );
					InterpolateNorm( vN2, vNorm1, vNorm2, vBary.x );

					for ( int k = 2; k < dstVerts.size(); ++k )
					{
						p = dstVerts[k];
						GetBaryCoords( p, v1, v2, v3, &vBary );
						InterpolateHeight( p, v1, v2, v3, vBary );
						InterpolateRealPos( vPos, vRealPos1, vRealPos2, vRealPos3, vBary );
						GetBaryCoords( p, vBase1, vBase2, vBase3, &vBary );
						InterpolateSecTexCoords( vTex, vSecTex1, vSecTex2, vSecTex3, vBary );
						InterpolateNorm( vN, vNorm1, vNorm2, vBary.x );

						//if ( ( fabs( vPos.z - vPos1.z ) < DEF_PEAK_MAX_HEIGHT_DISP ) && ( fabs( vPos.z - vPos2.z ) < DEF_PEAK_MAX_HEIGHT_DISP ) &&
						//		 ( fabs( vPos1.z - vPos2.z ) < DEF_PEAK_MAX_HEIGHT_DISP ) )
						AddTileTriangle( 0, &dataArr, p1, p2, p, vTex1, vTex2, vTex, i, g, nPatchX, nPatchY,
														 &vPos1, &vPos2, &vPos, &vN1, &vN2, &vN, false );

						p2 = p;
						vTex2 = vTex;
						vPos2 = vPos;
						vN2 = vN;
					}
				}
			}
		}
	}*/
}

inline float GetDist2( const CVec3 &v1, const CVec3fEx &v2 )
{
	//return fabs2( v1.x - v2.x ) + fabs2( v1.y - v2.y );
	return fabs2( v1.x - v2.x ) + fabs2( v1.y - v2.y ) + fabs2( v1.z - v2.z );
}

static bool FindNearestVertexInTile( CVec3 *pRes, float *fAddHeight, const CVec3 &v, const CArray2D<STerrainInfo::STile> &tiles,
																		const int x1, const int y1, const int x2, const int y2 )
{
	bool bFlag = true;
	CVec3fEx vRes;
	float fDist = FP_MAX_VALUE;

	for ( int g = y1; g <= y2; ++g )
	{
		for ( int i = x1; i <= x2; ++i )
		{
			const STerrainInfo::STile &tile = tiles[g][i];
			if ( tile.vertices.empty() )
				continue;
			if ( bFlag )
			{
				//vRes = tile.vertices[0]; vRes.z += tile.addHeights[0];
				const CVec3fEx &tileVert = tile.vertices[0];
				vRes.Set( tileVert.x, tileVert.y, max(tileVert.z + tile.addHeights[0], 0.0f), tileVert.flag );
				*fAddHeight = tile.addHeights[0];
				fDist = GetDist2( v, vRes );
				bFlag = false;
			}
			for ( int k = 0; k < tile.vertices.size(); ++k )
			{
				const CVec3fEx &vCur = tile.vertices[k];
				const float t = GetDist2( v, CVec3fEx(vCur.x, vCur.y, max(vCur.z + tile.addHeights[k], 0.0f), vCur.flag) );
				if ( t < fDist )
				{
					fDist = t;
					//vRes = vCur; vRes.z += tile.addHeights[k];
					*fAddHeight = tile.addHeights[k];
					vRes.Set( vCur.x, vCur.y, max(vCur.z + tile.addHeights[k], 0.0f), vCur.flag );
				}
			}
		}
	}

	if ( bFlag )
		return false;

	//NI_ASSERT( fDist < DEF_EPS, "Can't find vertex in tile" );
	pRes->Set( vRes.x, vRes.y, vRes.z );

	if ( fDist < 0.1f )
		return true;

	return false;
}

inline bool FindNearestVertexInTile( CVec3 *pRes, float *pAddHeight, const CVec3 &v, const CArray2D<STerrainInfo::STile> &tiles )
{
	const int nx = Clamp( int( v.x * DEF_INV_TILE_SIZE ), 0, tiles.GetSizeX() - 1 );
	const int ny = Clamp( int( v.y * DEF_INV_TILE_SIZE ), 0, tiles.GetSizeY() - 1 );
	return FindNearestVertexInTile( pRes, pAddHeight, v, tiles, nx, ny, nx, ny );
}

void CTerraGen::PeaksGetArrayOfFirstPoints( vector<SIntersectPoint> *pInters, const CVec3 &v1, const CVec3 &v2 )
{
	if ( fabs( fabs2( v1.x - v2.x ) + fabs2( v1.y - v2.y ) ) < EPS_VALUE )
		return;

	const int nTileX1 = Clamp( int(min(v1.x, v2.x) * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY1 = Clamp( int(min(v1.y, v2.y) * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );
	const int nTileX2 = Clamp( int(max(v1.x, v2.x) * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY2 = Clamp( int(max(v1.y, v2.y) * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );

	int nX1, nY1, nX2, nY2;
	if ( nTileX1 <= nTileX2 )
	{
		nX1 = nTileX1;
		nX2 = nTileX2;
	}
	else
	{
		nX1 = nTileX2;
		nX2 = nTileX1;
	}
	if ( nTileY1 <= nTileY2 )
	{
		nY1 = nTileY1;
		nY2 = nTileY2;
	}
	else
	{
		nY1 = nTileY2;
		nY2 = nTileY1;
	}

	const float fDx = fabs( v2.x - v1.x );
	const float fDy = fabs( v2.y - v1.y );
	const float fInvDx = ( fDx > EPS_VALUE ) ? ( 1.0f / (v2.x - v1.x) ) : 0.0f;
	const float fInvDy = ( fDy > EPS_VALUE ) ? ( 1.0f / (v2.y - v1.y) ) : 0.0f;
	float t;

	for ( int g = nY1; g <= nY2; ++g )
	{
		for ( int i = nX1; i <= nX2; ++i )
		{
			vector<CVec3fEx> &verts = terrainInfo.tiles[g][i].vertices;
			vector<float> &addHeights = terrainInfo.tiles[g][i].addHeights;
			for ( int k = 0; k < verts.size(); ++k )
			{
				CVec3fEx &curVert = verts[k];
				if ( fDx > EPS_VALUE )
				{
					t = ( curVert.x - v1.x ) * fInvDx;
					if ( (t > 0) && (t < 1.0f) && (fabs(v1.y + (v2.y - v1.y) * t - curVert.y) < DEF_EPS) )
					{
						curVert.z = v1.z + ( v2.z - v1.z ) * t - addHeights[k];
						//addHeights[k] = v1.z + ( v2.z - v1.z ) * t;
						PushBackUnique( pInters, SIntersectPoint(CVec3(curVert.x, curVert.y, max(curVert.z + addHeights[k], 0.0f)), t) );
					}
				}
				else
				{
					t = ( curVert.y - v1.y ) * fInvDy;
					if ( (t > 0) && (t < 1.0f) && (fabs(v1.x + (v2.x - v1.x) * t - curVert.x) < DEF_EPS) )
					{
						curVert.z = v1.z + ( v2.z - v1.z ) * t - addHeights[k];
						//addHeights[k] = v1.z + ( v2.z - v1.z ) * t;
						PushBackUnique( pInters, SIntersectPoint(CVec3(curVert.x, curVert.y, max(curVert.z + addHeights[k], 0.0f)), t) );
					}
				}
			}
		}
	}

	/*const int nTileX1 = Clamp( int( min( v1.x, v2.x ) * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY1 = Clamp( int( min( v1.y, v2.y ) * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeY() - 1 );
	const int nTileX2 = Clamp( int( max( v1.x, v2.x ) * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nTileY2 = Clamp( int( max( v1.y, v2.y ) * DEF_INV_TILE_SIZE ), 0, terrainInfo.tiles.GetSizeY() - 1 );

	for ( int g = nTileY1; g <= nTileY2; ++g )
	{
		for ( int i = nTileX1; i <= nTileX2; ++i )
		{
			const STerrainInfo::STile &tile = terrainInfo.tiles[g][i];
			for ( vector<STriangle>::const_iterator it = tile.triangles.begin(); it != tile.triangles.end(); ++it )
			{
				const CVec3 p1( tile.vertices[it->i1].x, tile.vertices[it->i1].y, tile.vertices[it->i1].z + tile.addHeights[it->i1] );
				const CVec3 p2( tile.vertices[it->i2].x, tile.vertices[it->i2].y, tile.vertices[it->i2].z + tile.addHeights[it->i2] );
				const CVec3 p3( tile.vertices[it->i3].x, tile.vertices[it->i3].y, tile.vertices[it->i3].z + tile.addHeights[it->i3] );
				AddIntersectOneSide( v1, v2, p1, p2, pInters );
				AddIntersectOneSide( v1, v2, p2, p3, pInters );
				AddIntersectOneSide( v1, v2, p3, p1, pInters );
			}
		}
	}*/

	sort( pInters->begin(), pInters->end() );

/*	firstArrPos.push_back( v1 );

	for ( vector<SIntersectPoint>::const_iterator it = pInters.begin(); it != pInters.end(); ++it )
		firstArrPos.push_back( it->vPoint );

	firstArrPos.push_back( v2 );*/
}

inline void RotateRealPos( CVec3 &pRealPos, const CVec3 &vSrcPos, const CVec3 &vSrcNorm, const float fCosAng, const float fSinAng, const float fWidth )
{
	pRealPos.x = vSrcPos.x + vSrcNorm.x * fCosAng * fWidth;
	pRealPos.y = vSrcPos.y + vSrcNorm.y * fCosAng * fWidth;
	pRealPos.z = vSrcPos.z - fSinAng * fWidth;
}

inline void ClearDataArr( vector<NMeshData::SMeshDataTex2> &pDataArr )
{
	for ( vector<NMeshData::SMeshDataTex2>::iterator it = pDataArr.begin(); it != pDataArr.end(); ++it )
	{
		it->vertices.resize( 0 );
		it->triangles.resize( 0 );
		it->secondTex.resize( 0 );
		it->attributes[0].data.resize( 0 );
	}
}

inline void ReleaseUnusedAttribues( vector<NMeshData::SMeshDataTex2> &pDataArr )
{
	for ( vector<NMeshData::SMeshDataTex2>::iterator it = pDataArr.begin(); it != pDataArr.end(); ++it )
	{
		if ( it->attributes[0].data.empty() )
			it->attributes.resize( 0 );
	}
}

void CTerraGen::AddPeak( const STerrainInfo::SPeak &peak )
{
	if ( !(pDesc->pTerraSet) || (pDesc->pTerraSet->terraTypes.empty()) )
		return;

	SPeakGFXInfo gfxInfo;
	gfxInfo.nID = peak.nID;
	gfxInfo.patches.reserve( 16 );
	gfxInfo.patches.resize( 0 );

	vector<NMeshData::SMeshDataTex2> dataArr( pDesc->pTerraSet->terraTypes.size() );
	for ( vector<NMeshData::SMeshDataTex2>::iterator it = dataArr.begin(); it != dataArr.end(); ++it )
	{
		it->vertices.reserve( 256 );
		it->triangles.reserve( 256 );
		it->secondTex.reserve( 256 );
		it->attributes.resize( 1 );
		it->attributes[0].nID = NGScene::GATTR_VERTEX_COLOR;
		it->attributes[0].data.reserve( 256 );
		it->attributes[0].data.resize( 0 );
	}

	CVec3 vPrev1, vPrev2, vPrevRealPos1, vPrevRealPos2, vRealPos1, vRealPos2;
	float fSecTexX = 0.0f, fSecTexY;

	const float fSecTexAspectRatio = (float)pDesc->pTerraSet->terraTypes[0]->pPeakMaterial->pBump->nHeight /
																	 pDesc->pTerraSet->terraTypes[0]->pPeakMaterial->pBump->nWidth;

	int nPatchesCount;
	vector<SIntersectPoint> firstArrPos;
	firstArrPos.reserve( 16 );
	CVec3 vPos1, vPos2;
	float fAddHeight1, fAddHeight2;

	for ( vector< vector<STerrainInfo::SVSOPoint> >::const_iterator itPointsArr = peak.points.begin(); itPointsArr != peak.points.end(); ++itPointsArr )
	{
		ClearDataArr( dataArr );
		nPatchesCount = 0;

		const int nPreLastNum = itPointsArr->size() - 1;
		for ( int i = 0; i < nPreLastNum; ++i )
		{
			const STerrainInfo::SVSOPoint &point1 = (*itPointsArr)[i];
			const STerrainInfo::SVSOPoint &point2 = (*itPointsArr)[i + 1];

			if ( !FindNearestVertexInTile(&vPos1, &fAddHeight1, point1.vPos, terrainInfo.tiles) )
				//vPos1 = point1.vPos;
				continue;
			if ( !FindNearestVertexInTile(&vPos2, &fAddHeight2, point2.vPos, terrainInfo.tiles) )
				//vPos2 = point2.vPos;
				continue;

			//vPrev1 = point1.vPos - point1.vNorm * DEF_PENDENT_WIDTH * 0.5f;
			//vPrev2 = point2.vPos - point2.vNorm * DEF_PENDENT_WIDTH * 0.5f;
			vPrev1 = vPos1;
			vPrev2 = vPos2;

			const CVec3 vNorm1 = GetTerraNorm( vPos1 );
			const CVec3 vNorm2 = GetTerraNorm( vPos2 );

			vPrevRealPos1 = vPos1;
			vPrevRealPos2 = vPos2;

			const float fNextSecTexX = fSecTexX + fabs( vPrev2 - vPrev1 ) * DEF_PENDENT_SECTEX_X_COEFF * fSecTexAspectRatio;
			fSecTexY = 0.0f;

			for ( int k = 0; k < DEF_PENDENT_NUM; ++k )
			{
				const float fWidth = DEF_PENDENT_WIDTH * ( k + 1 );

				const CVec3 vNext1( vPos1.x + point1.vNorm.x * fWidth, vPos1.y + point1.vNorm.y * fWidth, vPos1.z );
				const CVec3 vNext2( vPos2.x + point2.vNorm.x * fWidth, vPos2.y + point2.vNorm.y * fWidth, vPos2.z );

				const float fNextSecTexY = (float)( k + 1 ) * DEF_PENDENT_SECTEX_COEFF;

				const float fAngle = DEF_PENDENT_PART1_ANGLE_COEFF * k;
				const float fCosAng = cos( fAngle );
				const float fSinAng = sin( fAngle );

				if ( k < DEF_PENDENT_PART1_NUM )
				{
					RotateRealPos( vRealPos1, vPos1, point1.vNorm, fCosAng, fSinAng, fWidth );
					RotateRealPos( vRealPos2, vPos2, point2.vNorm, fCosAng, fSinAng, fWidth );
				}
				else
				{
					vRealPos1.z -= DEF_PENDENT_WIDTH;
					vRealPos2.z -= DEF_PENDENT_WIDTH;
				}

				const CVec2 vPrevSecTex1( fSecTexX, fSecTexY );
				const CVec2 vPrevSecTex2( fNextSecTexX, fSecTexY );
				const CVec2 vNextSecTex1( fSecTexX , fNextSecTexY );
				const CVec2 vNextSecTex2( fNextSecTexX, fNextSecTexY );

				if ( k == 0 )
				{
					firstArrPos.resize( 0 );
					firstArrPos.push_back( SIntersectPoint(vPrev1, 0.0f) );
					firstArrPos.push_back( SIntersectPoint(vPrev2, 1.0f) );
					PeaksGetArrayOfFirstPoints( &firstArrPos, vPrev1/*CVec3( vPrev1.x, vPrev1.y, fAddHeight1 )*/, vPrev2/*CVec3( vPrev2.x, vPrev2.y, fAddHeight2 )*/ );
					const int nLast = firstArrPos.size() - 1;
					for ( int p = nLast; p > 0; --p )
					{
						const SIntersectPoint &p1 = firstArrPos[p - 1];
						const SIntersectPoint &p2 = firstArrPos[p];
						PeaksProjectTrgOnTiles( &dataArr, p2.vPoint, p1.vPoint, vNext2, vPrevSecTex2, vPrevSecTex1, vNextSecTex2,
															 p2.vPoint, p1.vPoint, vRealPos2, GetInterpolateNorm(vNorm1, vNorm2, p2.fDist),
															 GetInterpolateNorm(vNorm1, vNorm2, p1.fDist), vNorm2, vPrev2, vPrev1, vNext2 );
															 //vNorm2, vNorm1, vPrev2, vPrev1, vNext2 );
					}
					PeaksProjectTrgOnTiles( &dataArr, vNext1, vNext2, firstArrPos[0].vPoint, vNextSecTex1, vNextSecTex2, vPrevSecTex1,
														 vRealPos1, vRealPos2, firstArrPos[0].vPoint, vNorm1, vNorm2, vNorm1, vNext1, vNext2, vPrev1 );
														 //ProjectTrgOnTiles( dataArr, vNext1, vNext2, firstArrPos[0], vNextSecTex1, vNextSecTex2, vPrevSecTex1,
														 //	 vRealPos1, vRealPos2, firstArrPos[0], vNorm1, vNorm2, vNext1, vNext2, vPrev1 );
				}
				else
				{
					PeaksProjectTrgOnTiles( &dataArr, vPrev2, vPrev1, vNext2, vPrevSecTex2, vPrevSecTex1, vNextSecTex2,
														 vPrevRealPos2, vPrevRealPos1, vRealPos2, vNorm2, vNorm1, vNorm2, vPrev2, vPrev1, vNext2 );
					PeaksProjectTrgOnTiles( &dataArr, vNext1, vNext2, vPrev1, vNextSecTex1, vNextSecTex2, vPrevSecTex1,
														 vRealPos1, vRealPos2, vPrevRealPos1, vNorm1, vNorm2, vNorm1, vNext1, vNext2, vPrev1 );

					//PeaksProjectTrgOnTiles( dataArr, vPrev2, vPrev1, vNext2, vPrevSecTex2, vPrevSecTex1, vNextSecTex2,
					//	vPrevRealPos2, vPrevRealPos1, vRealPos2, vNorm2, vNorm1, vPrev2, vPrev1, vNext2 );
					//PeaksProjectTrgOnTiles( dataArr, vNext1, vNext2, vPrev1, vNextSecTex1, vNextSecTex2, vPrevSecTex1,
					//	vRealPos1, vRealPos2, vPrevRealPos1, vNorm1, vNorm2, vNext1, vNext2, vPrev1 );
				}

				vPrev1 = vNext1;
				vPrev2 = vNext2;
				fSecTexY = fNextSecTexY;
				vPrevRealPos1 = vRealPos1;
				vPrevRealPos2 = vRealPos2;
			}

			fSecTexX = fNextSecTexX;

			if ( ++nPatchesCount >= DEF_PEAK_PATCH_SIZE )
			{
				gfxInfo.patches.push_back( dataArr );
				ReleaseUnusedAttribues( gfxInfo.patches.back() );
				nPatchesCount = 0;
				ClearDataArr( dataArr );
			}
		}

		if ( nPatchesCount > 0 )
		{
			gfxInfo.patches.push_back( dataArr );
			ReleaseUnusedAttribues( gfxInfo.patches.back() );
		}
	}
	
	terrainGfxInfo.peaks.push_back( gfxInfo );

	if ( pGfxObserver )
		pGfxObserver->AddPeak( &(terrainGfxInfo.peaks.back()) );
}

void CTerraGen::AddAllPeaks()
{
	for ( list<STerrainInfo::SPeak>::const_iterator it = terrainInfo.peaks.begin(); it != terrainInfo.peaks.end(); ++it )
		AddPeak( *it );
}

void CTerraGen::RemovePeakInfo( const int nVSOID )
{
	for ( list<STerrainInfo::SPeak>::iterator it = terrainInfo.peaks.begin(); it != terrainInfo.peaks.end(); ++it )
	{
		if ( it->nID == nVSOID )
		{
			terrainInfo.peaks.erase( it );
			return;
			//it = terrainInfo.peaks.erase( it );
		}
		//else
		//	++it;
	}
}

void CTerraGen::RemovePeakGfxInfo( const int nVSOID )
{
	for ( list<SPeakGFXInfo>::iterator it = terrainGfxInfo.peaks.begin(); it != terrainGfxInfo.peaks.end(); ++it )
	{
		if ( it->nID == nVSOID )
		{
			terrainGfxInfo.peaks.erase( it );
			return;
			//it = terrainGfxInfo.peaks.erase( it );
		}
		//else
		//	++it;
	}
}

void CTerraGen::UpdateNeededPeaks()
{
	if ( vTexModMin.x < 0 )
		return;

	const CVec2 vMin( (float)vTexModMin.x * DEF_TILE_SIZE, (float)vTexModMin.y * DEF_TILE_SIZE );
	const CVec2 vMax( (float)(vTexModMax.x + 1) * DEF_TILE_SIZE, (float)(vTexModMax.y + 1) * DEF_TILE_SIZE );

	for ( list<STerrainInfo::SPeak>::const_iterator it = terrainInfo.peaks.begin(); it != terrainInfo.peaks.end(); ++it )
	{
		if ( (it->vBBMax.x >= vMin.x) && (it->vBBMax.y >= vMin.y) && (it->vBBMin.x <= vMax.x) && (it->vBBMin.y <= vMax.y) )
		{
			RemovePeakGfxInfo( it->nID );
			if ( pGfxObserver )
				pGfxObserver->RemovePeak( it->nID );
			AddPeak( *it );
		}
	}
}


