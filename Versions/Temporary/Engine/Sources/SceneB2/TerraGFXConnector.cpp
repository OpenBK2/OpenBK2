#include "stdafx.h"

#include "B2_M1_Terrain/DBPreLight.h"
#include "Misc/Win32Random.h"
#include "GenTerrain.h"

#include <cstdint>

#define DEF_TILE_MIN_ALPHA 0


inline CVec3 GetPreLightColor( const NDb::STerrain *pDesc, const CVec3 &vNorm, const CVec3 &vPreLightDir )
{
	const float fLightScaling = 4.0f; // scale always - 'bWhitening' flag already taken in account during DB loading
	const float fAng = vNorm * ( -vPreLightDir );
	return fAng >= 0.0f ? ( (-fAng * pDesc->pPreLight->vLightColor + pDesc->pPreLight->vAmbientColor) * fLightScaling ) :
	( ((1.0f - fAng) * pDesc->pPreLight->vShadeColor + pDesc->pPreLight->vShadeAmbientColor) * fLightScaling );
}


void CTerraGen::CalcTerraVertexData( NGScene::SVertex *pVertex, const CVec3 &vPos,
																		 const int nTileX, const int nTileY,
																		 const int nPatchX, const int nPatchY,
																		 const CVec3 *pRealPos, const CVec3 *pRealNorm,
																		 const bool bNeedFaster ) const
{
	//vert.tex.x = Clamp( ( vPos.x - (float)nPatchX * DEF_PATCH_WORLD_SIZE ) * DEF_INV_PATCH_WORLD_SIZE, 0.0f, 1.0f );
	//vert.tex.y = Clamp( ( vPos.y - (float)nPatchY * DEF_PATCH_WORLD_SIZE ) * DEF_INV_PATCH_WORLD_SIZE, 0.0f, 1.0f );
	pVertex->tex.x = ( vPos.x - (float)nPatchX * DEF_PATCH_WORLD_SIZE ) * DEF_INV_PATCH_WORLD_SIZE;
	pVertex->tex.y = ( vPos.y - (float)nPatchY * DEF_PATCH_WORLD_SIZE ) * DEF_INV_PATCH_WORLD_SIZE;
	CVec3 vNorm;
	if ( pRealNorm )
		vNorm = *pRealNorm;
	else
		vNorm = bNeedFaster ? GetTerraNormFast( vPos.x, vPos.y, nTileX, nTileY ) : FindNormalInVertex( vPos, nTileX, nTileY );
	CalcCompactVector( &(pVertex->normal), vNorm );
	pVertex->pos = ( pRealPos != 0 ) ? ( *pRealPos ) : vPos;
	CalcCompactVector( &(pVertex->texU), GetPreLightColor(pDesc, vNorm, vPreLightDir) );
	//CalcCompactVector( &(vert.texU), V3_AXIS_X );
	CalcCompactVector( &(pVertex->texV), V3_AXIS_Y );
}


void CTerraGen::UpdateAlphaByPosition( NGScene::SVertex *pVertex )
{
	const float fAlphaCoeff = IsPointOnMap( GetVec2(pVertex->pos) ) ? 1.0f : 0.0f;
	pVertex->normal.w *= fAlphaCoeff;
}

void CTerraGen::RemoveInvisibleTriangles( NMeshData::SMeshData *pPatch )
{
	for ( int iTriangle = 0; iTriangle < pPatch->triangles.size();  )
	{
		const STriangle &triangle = pPatch->triangles[iTriangle];
		if ( pPatch->vertices[triangle.i1].normal.w == 0 && 
			pPatch->vertices[triangle.i2].normal.w == 0 &&
			pPatch->vertices[triangle.i3].normal.w == 0 )
		{
			pPatch->triangles[iTriangle] = pPatch->triangles[pPatch->triangles.size() - 1];
			pPatch->triangles.resize( pPatch->triangles.size() - 1 );
		}
		else
			++iTriangle;
	}
}


int CTerraGen::AddUniqueVertex(	NMeshData::SMeshData *pMeshData,
															  NMeshData::SMeshDataTex2 *pTexData,
															  const CVec2 &vSecTex,
															  const uint8_t cAlpha,
															  NGScene::SVertex *pVertex,
															  const bool bNeedFaster ) const
{
	pVertex->normal.w = cAlpha;

	int nInd = 0;
	if ( pMeshData )
	{
		/*if ( !bNeedFaster )
		{
		for ( vector<NGScene::SVertex>::const_iterator it = pData1->vertices.begin(); it != pData1->vertices.end(); ++it, ++nInd )
		{
		if ( ( fabs2( it->pos - vert.pos ) < DEF_EPS ) && ( fabs2( it->tex - vert.tex ) < DEF_EPS ) )
		return nInd;
		}
		}*/
		pMeshData->vertices.push_back( *pVertex );
		pMeshData->attributes[0].data.push_back( pVertex->texU.dw );
		return ( pMeshData->vertices.size() - 1 );
	}

	/*if ( !bNeedFaster )
	{
	for ( vector<NGScene::SVertex>::const_iterator it = pData2->vertices.begin(); it != pData2->vertices.end(); ++it, ++nInd )
	{
	if ( ( fabs2( it->pos - vert.pos ) < DEF_EPS ) && ( fabs2( it->tex - vert.tex ) < DEF_EPS ) &&
	( fabs2( pData2->secondTex[nInd] - vSecTex ) < DEF_EPS ) )
	return nInd;
	}
	}*/
	//
	pTexData->vertices.push_back( *pVertex );
	pTexData->attributes[0].data.push_back( pVertex->texU.dw );
	pTexData->secondTex.push_back( vSecTex );
	return ( pTexData->vertices.size() - 1 );
}


inline uint8_t GetInterpolateValue( const CVec3 &v, const CArray2D<uint8_t> &mask )
{
	const int x = Clamp( int(v.x * DEF_INV_TILE_SIZE), 0, mask.GetSizeX() - 2 );
	const int y = Clamp( int(v.y * DEF_INV_TILE_SIZE), 0, mask.GetSizeY() - 2 );
	const float dx = Clamp( v.x - (float)x * DEF_TILE_SIZE, 0.0f, 1.0f );
	const float dy = Clamp( v.y - (float)y * DEF_TILE_SIZE, 0.0f, 1.0f );
	const float fAlpha = (float)( (float)mask[y][x] + (float)(mask[y][x + 1] - mask[y][x]) * dx ) * ( 1.0f - dy ) +
		(float)( (float)mask[y + 1][x] + (float)(mask[y + 1][x + 1] - mask[y + 1][x]) * dx ) * dy;
	return Clamp( Float2Int(fAlpha), 0, 255 );
}


inline uint8_t GetInterpolateValue( const CArray2D<uint8_t> &mask, const int x, const int y, const float dx, const float dy )
{
	const float fAlpha = (float)( (float)mask[y][x] + (float)( mask[y][x + 1] - mask[y][x] ) * dx ) * ( 1.0f - dy ) +
		(float)( (float)mask[y + 1][x] + (float)(mask[y + 1][x + 1] - mask[y + 1][x]) * dx ) * dy;
	return Clamp( Float2Int(fAlpha), 0, 255 );
}


struct SPushAlphaTriangle
{
	int nID;
	uint8_t cAlpha1, cAlpha2, cAlpha3;
	//
	SPushAlphaTriangle() {}
	SPushAlphaTriangle( const int _nID, const uint8_t a1, const uint8_t a2, const uint8_t a3 )
		:	nID(_nID), cAlpha1(a1), cAlpha2(a2), cAlpha3(a3) {}
};


static std::vector<SPushAlphaTriangle> prePushTrgs( 256 );


static void WrapChapterTexture( std::vector<NGScene::SVertex> *pVerts, const int nPatchesX, const int nPatchesY, const float fScale = 1 )
{
	const float fDefTexSize = 1024.0f;
	const CVec2 vCropSize( 696.0f, 591.0f );

	for ( std::vector<NGScene::SVertex>::iterator itVert = pVerts->begin(); itVert != pVerts->end(); ++itVert )
	{
		NGScene::SVertex &vert = (*itVert);
		const CVec3 &vPos = vert.pos;

		vert.tex.x = vPos.y / ( nPatchesY * DEF_PATCH_WORLD_SIZE ) / fDefTexSize * vCropSize.x;
		vert.tex.y = vPos.x / ( nPatchesX * DEF_PATCH_WORLD_SIZE ) / fDefTexSize * vCropSize.y;
	}
}


void CTerraGen::AddTileTriangle( std::vector<NMeshData::SMeshData> *pMeshData,
																 std::vector<NMeshData::SMeshDataTex2> *pTexData,
																 const CVec3 &v1, const CVec3 &v2, const CVec3 &v3,
																 const CVec2 &vSecTex1, const CVec2 &vSecTex2, const CVec2 &vSecTex3,
																 const int nTileX, const int nTileY,
																 const int nPatchX, const int nPatchY,
																 const CVec3 *pRealPos1, const CVec3 *pRealPos2, const CVec3 *pRealPos3,
																 const CVec3 *pRealNorm1, const CVec3 *pRealNorm2, const CVec3 *pRealNorm3,
																 const bool bNeedFaster ) const
{
	NI_VERIFY( pMeshData || pTexData, "CTerraGen::AddTileTriangle - Invalid destination data array", return )

	// initialize buffers for pre-light
	int nA1, nA2, nA3;

	prePushTrgs.reserve( tileTerraMasks.size() );
	prePushTrgs.resize( 0 );

	if ( tileTerraMasks.empty() )
		return;

	const CArray2D<uint8_t> &mask = tileTerraMasks[0];
	const int nV1x = Clamp( int(v1.x * DEF_INV_TILE_SIZE), 0, mask.GetSizeX() - 2 );
	const int nV1y = Clamp( int(v1.y * DEF_INV_TILE_SIZE), 0, mask.GetSizeY() - 2 );
	const float fV1dx = Clamp( v1.x - (float)nV1x * DEF_TILE_SIZE, 0.0f, 1.0f );
	const float fV1dy = Clamp( v1.y - (float)nV1y * DEF_TILE_SIZE, 0.0f, 1.0f );
	const int nV2x = Clamp( int(v2.x * DEF_INV_TILE_SIZE), 0, mask.GetSizeX() - 2 );
	const int nV2y = Clamp( int(v2.y * DEF_INV_TILE_SIZE), 0, mask.GetSizeY() - 2 );
	const float fV2dx = Clamp( v2.x - (float)nV2x * DEF_TILE_SIZE, 0.0f, 1.0f );
	const float fV2dy = Clamp( v2.y - (float)nV2y * DEF_TILE_SIZE, 0.0f, 1.0f );
	const int nV3x = Clamp( int(v3.x * DEF_INV_TILE_SIZE), 0, mask.GetSizeX() - 2 );
	const int nV3y = Clamp( int(v3.y * DEF_INV_TILE_SIZE), 0, mask.GetSizeY() - 2 );
	const float fV3dx = Clamp( v3.x - (float)nV3x * DEF_TILE_SIZE, 0.0f, 1.0f );
	const float fV3dy = Clamp( v3.y - (float)nV3y * DEF_TILE_SIZE, 0.0f, 1.0f );

	for ( int k = 0; k < tileTerraMasks.size(); ++k )
	{
		const CArray2D<uint8_t> &curMask = tileTerraMasks[k];
		const uint8_t cAlpha1 = GetInterpolateValue( curMask, nV1x, nV1y, fV1dx, fV1dy );
		const uint8_t cAlpha2 = GetInterpolateValue( curMask, nV2x, nV2y, fV2dx, fV2dy );
		const uint8_t cAlpha3 = GetInterpolateValue( curMask, nV3x, nV3y, fV3dx, fV3dy );
		if ( (cAlpha1 > DEF_TILE_MIN_ALPHA) || (cAlpha2 > DEF_TILE_MIN_ALPHA) || (cAlpha3 > DEF_TILE_MIN_ALPHA) )
			prePushTrgs.push_back( SPushAlphaTriangle(tilesOrder[k].nPrevNum, cAlpha1, cAlpha2, cAlpha3) );
	}

	const int nSizeMinusOne = prePushTrgs.size() - 1;
	nA1 = 0; nA2 = 0; nA3 = 0;

	NGScene::SVertex vert1, vert2, vert3;
	CalcTerraVertexData( &vert1, v1, nTileX, nTileY, nPatchX, nPatchY, pRealPos1, pRealNorm1, bNeedFaster );
	CalcTerraVertexData( &vert2, v2, nTileX, nTileY, nPatchX, nPatchY, pRealPos2, pRealNorm2, bNeedFaster );
	CalcTerraVertexData( &vert3, v3, nTileX, nTileY, nPatchX, nPatchY, pRealPos3, pRealNorm3, bNeedFaster );
	const CVec2 vTexBase1 = vert1.tex;
	const CVec2 vTexBase2 = vert2.tex;
	const CVec2 vTexBase3 = vert3.tex;

	for ( int k = nSizeMinusOne; k >= 0; --k )
	{
		const SPushAlphaTriangle &pushTrg = prePushTrgs[k];
		if ( k == 0 )
		{
			nA1 = 255; nA2 = 255; nA3 = 255;
		}
		else
		{
			nA1 += pushTrg.cAlpha1;
			nA2 += pushTrg.cAlpha2;
			nA3 += pushTrg.cAlpha3;
		}

		NMeshData::SMeshData *pSingleMeshData = ( pMeshData != 0 ) ? ( &(*pMeshData)[pushTrg.nID] ) : 0;
		NMeshData::SMeshDataTex2 *pSingleTexData = ( pTexData != 0 ) ? ( &(*pTexData)[pushTrg.nID] ) : 0;

		vert1.tex = vTexBase1 * pDesc->pTerraSet->terraTypes[pushTrg.nID]->fScaleCoeff;
		vert2.tex = vTexBase2 * pDesc->pTerraSet->terraTypes[pushTrg.nID]->fScaleCoeff;
		vert3.tex = vTexBase3 * pDesc->pTerraSet->terraTypes[pushTrg.nID]->fScaleCoeff;

		const int nInd1 = AddUniqueVertex( pSingleMeshData, pSingleTexData, vSecTex1, nA1, &vert1, bNeedFaster );
		const int nInd2 = AddUniqueVertex( pSingleMeshData, pSingleTexData, vSecTex2, nA2, &vert2, bNeedFaster );
		const int nInd3 = AddUniqueVertex( pSingleMeshData, pSingleTexData, vSecTex3, nA3, &vert3, bNeedFaster );

		std::vector<STriangle> &triangles = ( pSingleMeshData != 0 ) ? pSingleMeshData->triangles : pSingleTexData->triangles;

		if ( (nInd1 != nInd2) && (nInd2 != nInd3) && (nInd3 != nInd1) )
		{
			if ( IsCCW( v1, v2, v3 ) )
				triangles.push_back( STriangle(nInd1, nInd2, nInd3) );
			else
				triangles.push_back( STriangle(nInd1, nInd3, nInd2) );
		}

		if ( (nA1 >= 0xff) && (nA2 >= 0xff) && (nA3 >= 0xff) )
			break;
	}
}


typedef std::unordered_map<int, CVec3> CPatchNormsHash;
typedef std::unordered_map<int, int> CPatchNormsCntHash;
typedef std::unordered_map<int, NGScene::SVertex> CPatchVertsHash;
typedef std::unordered_map<int, CVec3> CPatchPreLightHash;

inline int GetVertexHashKey( const CVec3 &v )
{
	return ( Float2Int( v.y * 100.0f ) << 16 ) | Float2Int( v.x * 100.0f );
}


inline void AddNormFromVertex( const CVec3 &v, const CVec3 &vNorm, CPatchNormsHash &patchNorms, CPatchNormsCntHash &patchNormsCnt )
{
	const int nVertKey = GetVertexHashKey( v );
	CPatchNormsHash::iterator itFind = patchNorms.find( nVertKey );
	if ( itFind == patchNorms.end() )
	{
		patchNorms[nVertKey] = vNorm;
		patchNormsCnt[nVertKey] = 1;
	}
	else
	{
		itFind->second += vNorm;
		++patchNormsCnt.find( nVertKey )->second;
	}
}


inline int AddGfxVertex( CPatchVertsHash *pVertsHash, const CVec3 &rVertex, const float fPatchX, const float fPatchY, const CPatchNormsHash &normsHash )
{
	const int nVertKey = GetVertexHashKey( rVertex );
	CPatchVertsHash::const_iterator itFind = pVertsHash->find( nVertKey );
	if ( itFind == pVertsHash->end() )
	{
		NGScene::SVertex vert;
		CalcCompactVector( &(vert.texU), V3_AXIS_X );
		CalcCompactVector( &(vert.texV), V3_AXIS_Y );
		CalcCompactVector( &(vert.normal), normsHash.find(nVertKey)->second );
		vert.pos = rVertex;
		vert.tex.Set( (rVertex.x - fPatchX) * DEF_INV_PATCH_WORLD_SIZE, (rVertex.y - fPatchY) * DEF_INV_PATCH_WORLD_SIZE );
		(*pVertsHash)[nVertKey] = vert;
	}
	return nVertKey;
}


inline int AddLayerVertex( std::unordered_map<int, int> *pVertsHash, std::vector<NGScene::SVertex> *pDataVerts, std::vector<uint32_t> *pAttr,
													 const int nInd, const NGScene::SVertex &vert, const float fTexScaleCoeff, CPatchPreLightHash &preLightHash /* - really const!*/ )
{
	std::unordered_map<int, int>::const_iterator itFind = pVertsHash->find( nInd );
	if ( itFind == pVertsHash->end() )
	{
		const int nLayerKey = pDataVerts->size();
		(*pVertsHash)[nInd] = nLayerKey;
		//pDataVerts->push_back( vert );
		std::vector<NGScene::SVertex>::iterator itAdded = pDataVerts->insert( pDataVerts->end(), vert );
		itAdded->tex *= fTexScaleCoeff;
		NGfx::SCompactVector compVect;
		NGfx::CalcCompactVector( &compVect, preLightHash[nInd] );
		pAttr->push_back( compVect.dw );
		itAdded->normal.w = 0;
		return nLayerKey;
	}
	return itFind->second;
}


inline void CalcTerraNorm( CVec3 *pNorm, const CVec3 &v1, const CVec3 &v2, const CVec3 &v3 )
{
	const CVec3 dv1( v2.x - v1.x, v2.y - v1.y, v2.z - v1.z );
	const CVec3 dv2( v3.x - v1.x, v3.y - v1.y, v3.z - v1.z );
	(*pNorm) = dv1 ^ dv2;
	Normalize( pNorm );
}


inline float GetDistInsideTile( const float x, const int nTile )
{
	return ( x - (float)nTile * DEF_TILE_SIZE ) * DEF_INV_TILE_SIZE;
}


void CTerraGen::PutAllGfxToObserver()
{
	if ( pGfxObserver )
	{
		/*for ( int g = 0; g < terrainInfo.nNumPatchesY; ++g )
		{
		for ( int i = 0; i < terrainInfo.nNumPatchesX; ++i )
		{
		pGfxObserver->UpdatePatchGeometry( terrainGfxInfo.terraPatches[g][i], i, g, false );
		}
		}*/
		ReCreateAllFeaturesGfx();
		//PutAllFeaturesToGfx();
	}
}


void CTerraGen::PutAllFeaturesToGfx()
{
	//for ( list<SCragGFXInfo>::const_iterator it = terrainGfxInfo.crags.begin(); it != terrainGfxInfo.crags.end(); ++it )
	//	pGfxObserver->AddCrag( &(*it) );
	for ( std::list<SRiverGFXInfo>::const_iterator it = terrainGfxInfo.rivers.begin(); it != terrainGfxInfo.rivers.end(); ++it )
		pGfxObserver->AddRiver( &(*it) );
	for ( std::list<SRoadGFXInfo>::iterator it = terrainGfxInfo.roads.begin(); it != terrainGfxInfo.roads.end(); ++it )
		pGfxObserver->AddRoad( &(*it) );
	for ( std::list<STerraSpotGFXInfo>::iterator it = terrainGfxInfo.terraspots.begin(); it != terrainGfxInfo.terraspots.end(); ++it )
		pGfxObserver->AddTerraSpot( &(*it) );
	for ( std::list<SPeakGFXInfo>::iterator it = terrainGfxInfo.peaks.begin(); it != terrainGfxInfo.peaks.end(); ++it )
		pGfxObserver->AddPeak( &(*it) );
	for ( std::list<SFootGFXInfo>::iterator it = terrainGfxInfo.foots.begin(); it != terrainGfxInfo.foots.end(); ++it )
		pGfxObserver->AddFoot( &(*it) );
	for ( std::list<SPrecipiceGFXInfo>::iterator it = terrainGfxInfo.precipices.begin(); it != terrainGfxInfo.precipices.end(); ++it )
		pGfxObserver->AddPrecipice( &(*it) );
}


void CTerraGen::UpdateAllGfxInfo()
{
	UpdateGfxInfo( 0, 0, terrainInfo.nNumPatchesX, terrainInfo.nNumPatchesY );

	if ( pGfxObserver )
		PutAllFeaturesToGfx();
}


void CTerraGen::ReCreateAllFeaturesGfx()
{
	TIME_STAT_START( CTerraGen__ReCreateAllFeaturesGfx )

	NWin32Random::Seed( terrainInfo.nRecreateRandSeed );

	// recreate roads
	if ( pDesc->roads.size() > 0 )
	{
		TIME_STAT_START( Roads_recreation )

			std::vector<NDb::SVSOPoint> points;
		for ( std::vector<NDb::SVSOInstance>::const_iterator it = pDesc->roads.begin(); it != pDesc->roads.end(); ++it )
		{
			ConvertVSOPointsFromAIToVisAndPutOnTerrain( &points, it->points );
			CreateRoadGfx( &*it, points );
		}

		TIME_STAT_FINISH( Roads_recreation )
	}
	// recreate crags
	if ( pDesc->crags.size() > 0 )
	{
		// recreate foots
		//AddAllFoots();
		for ( std::list<STerrainInfo::SFoot>::const_iterator it = terrainInfo.foots.begin(); it != terrainInfo.foots.end(); ++it )
			AddFoot( *it );

		terrainInfo.peaks.clear();

		// recreate precipices
		for ( std::list<STerrainInfo::SPrecipice>::iterator it = terrainInfo.precipices.begin(); it != terrainInfo.precipices.end(); ++it )
			CreatePrecipiceMesh( &(*it), false );

		// recreate peaks
		//for ( list<STerrainInfo::SPeak>::const_iterator it = terrainInfo.peaks.begin(); it != terrainInfo.peaks.end(); ++it )
		//	AddPeak( *it );
	}

	// recreate rivers
	if ( pDesc->rivers.size() > 0 )
	{
		for ( std::list<STerrainInfo::SRiver>::iterator it = terrainInfo.rivers.begin(); it != terrainInfo.rivers.end(); ++it )
		{
			const NDb::SVSOInstance *pInstance = FindRiver( it->nID );
			if ( pInstance )
				CreateRiverGfx( &(*it), pInstance, true );
		}
		if ( !terrainGfxInfo.rivers.empty() )
			UpdateRiversDepthes();
	}

	// recreate spots
	if ( pDesc->spots.size() > 0 )
	{
		for ( std::list<STerrainInfo::STerraSpot>::iterator it = terrainInfo.terraspots.begin(); it != terrainInfo.terraspots.end(); ++it )
		{
			const NDb::STerrainSpotInstance *pInstance = FindTerraSpot( it->nID );
			if ( pInstance && pInstance->pDescriptor )
				CreateTerraSpotGfx( &(*it), pInstance );
			else
				DebugTrace( "Can't find terra spot instance %d or instance descriptor empty", it->nID );
		}
	}

	TIME_STAT_FINISH( CTerraGen__ReCreateAllFeaturesGfx )
}


void CTerraGen::UpdateTerraBorders()
{
	// 4 borders
	terrainGfxInfo.terraBorders.resize( 4 );
	terrainGfxInfo.terraBorders[0].resize( GetTilesCountX() );
	terrainGfxInfo.terraBorders[1].resize( GetTilesCountX() );
	terrainGfxInfo.terraBorders[2].resize( GetTilesCountY() );
	terrainGfxInfo.terraBorders[3].resize( GetTilesCountY() );

	const float fLowerBorder = -10.0f;
	const float fFarX = Tile2Vis( GetTilesCountX() );
	const float fFarY = Tile2Vis( GetTilesCountY() );

	for ( int i = 0; i < GetTilesCountX(); ++i )
	{
		NMeshData::SMeshData &data = terrainGfxInfo.terraBorders[0][i];
		//
		data.vertices.resize( 4 );
		data.vertices[0].pos.Set( i * DEF_TILE_SIZE, 0.0f, GetTerraHeight(Tile2Vis(i), 0.0f) );
		data.vertices[1].pos.Set( (i + 1) * DEF_TILE_SIZE, 0.0f, GetTerraHeight(Tile2Vis(i + 1), 0.0f) );
		data.vertices[2].pos.Set( i * DEF_TILE_SIZE, 0.0f, fLowerBorder );
		data.vertices[3].pos.Set( (i + 1) * DEF_TILE_SIZE, 0.0f, fLowerBorder );
		CalcCompactVector( &(data.vertices[0].normal), CVec3(0, -1, 0) );
		CalcCompactVector( &(data.vertices[1].normal), CVec3(0, -1, 0) );
		CalcCompactVector( &(data.vertices[2].normal), CVec3(0, -1, 0) );
		CalcCompactVector( &(data.vertices[3].normal), CVec3(0, -1, 0) );
		data.vertices[0].tex = VNULL2;
		data.vertices[1].tex = VNULL2;
		data.vertices[2].tex = VNULL2;
		data.vertices[3].tex = VNULL2;
		data.triangles.resize( 2 );
		data.triangles[0] = STriangle( 0, 2, 1 );
		data.triangles[1] = STriangle( 1, 2, 3 );
	}
	for ( int i = 0; i < GetTilesCountX(); ++i )
	{
		NMeshData::SMeshData &data = terrainGfxInfo.terraBorders[1][i];
		//
		data.vertices.resize( 4 );
		data.vertices[0].pos.Set( i * DEF_TILE_SIZE, fFarY, GetTerraHeight(Tile2Vis(i), fFarY) );
		data.vertices[1].pos.Set( (i + 1) * DEF_TILE_SIZE, fFarY, GetTerraHeight(Tile2Vis(i + 1), fFarY) );
		data.vertices[2].pos.Set( i * DEF_TILE_SIZE, fFarY, fLowerBorder );
		data.vertices[3].pos.Set( (i + 1) * DEF_TILE_SIZE, fFarY, fLowerBorder );
		CalcCompactVector( &(data.vertices[0].normal), CVec3(0, 1, 0) );
		CalcCompactVector( &(data.vertices[1].normal), CVec3(0, 1, 0) );
		CalcCompactVector( &(data.vertices[2].normal), CVec3(0, 1, 0) );
		CalcCompactVector( &(data.vertices[3].normal), CVec3(0, 1, 0) );
		data.vertices[0].tex = VNULL2;
		data.vertices[1].tex = VNULL2;
		data.vertices[2].tex = VNULL2;
		data.vertices[3].tex = VNULL2;
		data.triangles.resize( 2 );
		data.triangles[0] = STriangle( 0, 1, 2 );
		data.triangles[1] = STriangle( 1, 3, 2 );
	}
	for ( int i = 0; i < GetTilesCountY(); ++i )
	{
		NMeshData::SMeshData &data = terrainGfxInfo.terraBorders[2][i];
		//
		data.vertices.resize( 4 );
		data.vertices[0].pos.Set( 0.0f, i * DEF_TILE_SIZE, GetTerraHeight(0.0f, Tile2Vis(i)) );
		data.vertices[1].pos.Set( 0.0f, (i + 1) * DEF_TILE_SIZE, GetTerraHeight(0.0f, Tile2Vis(i + 1)) );
		data.vertices[2].pos.Set( 0.0f, i * DEF_TILE_SIZE, fLowerBorder );
		data.vertices[3].pos.Set( 0.0f, (i + 1) * DEF_TILE_SIZE, fLowerBorder );
		CalcCompactVector( &(data.vertices[0].normal), CVec3(-1, 0, 0) );
		CalcCompactVector( &(data.vertices[1].normal), CVec3(-1, 0, 0) );
		CalcCompactVector( &(data.vertices[2].normal), CVec3(-1, 0, 0) );
		CalcCompactVector( &(data.vertices[3].normal), CVec3(-1, 0, 0) );
		data.vertices[0].tex = VNULL2;
		data.vertices[1].tex = VNULL2;
		data.vertices[2].tex = VNULL2;
		data.vertices[3].tex = VNULL2;
		data.triangles.resize( 2 );
		data.triangles[0] = STriangle( 0, 1, 2 );
		data.triangles[1] = STriangle( 1, 3, 2 );
	}
	for ( int i = 0; i < GetTilesCountY(); ++i )
	{
		NMeshData::SMeshData &data = terrainGfxInfo.terraBorders[3][i];
		//
		data.vertices.resize( 4 );
		data.vertices[0].pos.Set( fFarX, i * DEF_TILE_SIZE, GetTerraHeight(fFarX, Tile2Vis(i)) );
		data.vertices[1].pos.Set( fFarX, (i + 1) * DEF_TILE_SIZE, GetTerraHeight(fFarX, Tile2Vis(i + 1)) );
		data.vertices[2].pos.Set( fFarX, i * DEF_TILE_SIZE, fLowerBorder );
		data.vertices[3].pos.Set( fFarX, (i + 1) * DEF_TILE_SIZE, fLowerBorder );
		CalcCompactVector( &(data.vertices[0].normal), CVec3(1, 0, 0) );
		CalcCompactVector( &(data.vertices[1].normal), CVec3(1, 0, 0) );
		CalcCompactVector( &(data.vertices[2].normal), CVec3(1, 0, 0) );
		CalcCompactVector( &(data.vertices[3].normal), CVec3(1, 0, 0) );
		data.vertices[0].tex = VNULL2;
		data.vertices[1].tex = VNULL2;
		data.vertices[2].tex = VNULL2;
		data.vertices[3].tex = VNULL2;
		data.triangles.resize( 2 );
		data.triangles[0] = STriangle( 0, 2, 1 );
		data.triangles[1] = STriangle( 1, 2, 3 );
	}
	//
	if ( pGfxObserver )
	{
		pGfxObserver->UpdateBorderGeometry( &terrainGfxInfo.terraBorders[0], 0 );
		pGfxObserver->UpdateBorderGeometry( &terrainGfxInfo.terraBorders[1], 1 );
		pGfxObserver->UpdateBorderGeometry( &terrainGfxInfo.terraBorders[2], 2 );
		pGfxObserver->UpdateBorderGeometry( &terrainGfxInfo.terraBorders[3], 3 );
	}
}


void CTerraGen::UpdateGfxInfo( const int nPatchX1, const int nPatchY1, const int nPatchX2, const int nPatchY2 )
{
	for ( int g = nPatchY1; g < nPatchY2; ++g )
	{
		for ( int i = nPatchX1; i < nPatchX2; ++i )
		{
			UpdateGfxInfo( i, g );
		}
	}
	// TODO: needs to be optimized
	UpdateTerraBorders();
}


void CTerraGen::UpdateGfxInfo( const int nPatchX, const int nPatchY )
{
	//if ( ( terrainGfxInfo.terraPatches.GetSizeX() != terrainInfo.nNumPatchesX ) ||
	//		 ( terrainGfxInfo.terraPatches.GetSizeY() != terrainInfo.nNumPatchesY ) )
	//{
	//	terrainGfxInfo.terraPatches.SetSizes( terrainInfo.nNumPatchesX, terrainInfo.nNumPatchesY );
	//	for ( int g = 0; g < terrainGfxInfo.terraPatches.GetSizeY(); ++g )
	//	{
	//		for ( int i = 0; i < terrainGfxInfo.terraPatches.GetSizeX(); ++i )
	//		{
	//			terrainGfxInfo.terraPatches[g][i].resize( tileTerraMasks.size() );
	//		}
	//	}
	//}
	const int nPatchesNum = terrainInfo.nNumPatchesX * terrainInfo.nNumPatchesY * DEF_BREAK_TERRA_PATCHES;
	if ( terrainGfxInfo.terraPatches.size() != nPatchesNum )
	{
		terrainGfxInfo.terraPatches.resize( nPatchesNum );
		for ( int i = 0; i < nPatchesNum; ++i )
			terrainGfxInfo.terraPatches[i].resize( tileTerraMasks.size() );
	}

	//UpdateNormals( nPatchX, nPatchY );

	const float fPatchX = (float)nPatchX * DEF_PATCH_WORLD_SIZE;
	const float fPatchY = (float)nPatchY * DEF_PATCH_WORLD_SIZE;

	const int _nTileX1 = nPatchX * DEF_PATCH_SIZE;
	const int _nTileY1 = nPatchY * DEF_PATCH_SIZE;
	const int _nTileX2 = ( nPatchX + 1 ) * DEF_PATCH_SIZE;
	const int _nTileY2 = ( nPatchY + 1 ) * DEF_PATCH_SIZE;

	//if ( terrainGfxInfo.terraPatches[nPatchY][nPatchX].size() != tileTerraMasks.size() )
	//	terrainGfxInfo.terraPatches[nPatchY][nPatchX].resize( tileTerraMasks.size() );

	const int nPatchInd = nPatchY * terrainInfo.nNumPatchesX * DEF_BREAK_TERRA_PATCHES + nPatchX * DEF_BREAK_TERRA_PATCHES;

	for ( int i = 0; i < DEF_BREAK_TERRA_PATCHES; ++i )
	{
		for ( int k = 0; k < tileTerraMasks.size(); ++k )
		{
			//NMeshData::SMeshData &data = terrainGfxInfo.terraPatches[nPatchY][nPatchX][k];
			NMeshData::SMeshData &data = terrainGfxInfo.terraPatches[nPatchInd + i][k];
			//data.vertices.reserve( DEF_VERTS_RESERVE_COUNT );
			data.vertices.resize( 0 );
			//data.triangles.reserve( DEF_TRGS_RESERVE_COUNT );
			data.triangles.resize( 0 );
			data.attributes.resize( 1 );
			data.attributes[0].nID = NGScene::GATTR_VERTEX_COLOR;
			//data.attributes[0].data.reserve( DEF_TRGS_RESERVE_COUNT );
			data.attributes[0].data.resize( 0 );
		}
	}

	for ( int p = 0; p < DEF_BREAK_TERRA_PATCHES; ++p )
	{
		const int ndx = p % DEF_BREAK_TERRA_PATCHES_ONE_AXIS;
		const int ndy = p / DEF_BREAK_TERRA_PATCHES_ONE_AXIS;

		const int nTileX1 = _nTileX1 + ndx * DEF_BREAK_TERRA_PATCHES_STEP;
		const int nTileX2 = _nTileX1 + ( ndx + 1 ) * DEF_BREAK_TERRA_PATCHES_STEP;
		const int nTileY1 = _nTileY1 + ndy * DEF_BREAK_TERRA_PATCHES_STEP;
		const int nTileY2 = _nTileY1 + ( ndy + 1 ) * DEF_BREAK_TERRA_PATCHES_STEP;

		// create normals for patch
		const int nX1 = max( nTileX1 - 1, 0 );
		const int nY1 = max( nTileY1 - 1, 0 );
		const int nX2 = min( nTileX2 + 1, terrainInfo.tiles.GetSizeX() );
		const int nY2 = min( nTileY2 + 1, terrainInfo.tiles.GetSizeY() );

		static CPatchNormsHash patchNorms( 1024 );
		static CPatchNormsCntHash patchNormsCnt( 1024 );
		patchNorms.clear();
		patchNormsCnt.clear();

		CVec3 vNorm;

		const int ntx1 = max( nTileX1 - 1, 0 );
		const int nty1 = max( nTileY1 - 1, 0 );
		const int ntx2 = min( nTileX2 + 1, terrainInfo.tiles.GetSizeX() );
		const int nty2 = min( nTileY2 + 1, terrainInfo.tiles.GetSizeY() );

		for ( int g = nty1; g < nty2; ++g )
		{
			for ( int i = ntx1; i < ntx2; ++i )
			{
				const STerrainInfo::STile &tile = terrainInfo.tiles[g][i];
				for ( std::vector<STriangle>::const_iterator it = tile.triangles.begin(); it != tile.triangles.end(); ++it )
				{
					const CVec3fEx &_v1 = tile.vertices[it->i1];
					const CVec3fEx &_v2 = tile.vertices[it->i2];
					const CVec3fEx &_v3 = tile.vertices[it->i3];
					const CVec3 v1( _v1.x, _v1.y, max(_v1.z + tile.addHeights[it->i1], 0.0f) );
					const CVec3 v2( _v2.x, _v2.y, max(_v2.z + tile.addHeights[it->i2], 0.0f) );
					const CVec3 v3( _v3.x, _v3.y, max(_v3.z + tile.addHeights[it->i3], 0.0f) );
					CalcTerraNorm( &vNorm, v1, v2, v3 );
					AddNormFromVertex( v1, vNorm, patchNorms, patchNormsCnt );
					AddNormFromVertex( v2, vNorm, patchNorms, patchNormsCnt );
					AddNormFromVertex( v3, vNorm, patchNorms, patchNormsCnt );
				}
			}
		}

		for ( CPatchNormsHash::iterator it = patchNorms.begin(); it != patchNorms.end(); ++it )
		{
			CPatchNormsCntHash::const_iterator itCnt = patchNormsCnt.find( it->first );
			if ( itCnt->second > 1 )
			{
				it->second /= itCnt->second;
				Normalize( &it->second );
			}
		}

		CPatchPreLightHash preLightHash( patchNorms.size() );
		for ( CPatchNormsHash::const_iterator it = patchNorms.begin(); it != patchNorms.end(); ++it )
			preLightHash[it->first] = GetPreLightColor( pDesc, it->second, vPreLightDir );

		std::vector<std::unordered_map<int, int> > patchesVertsHash( tileTerraMasks.size() );
		static std::unordered_map<int, NGScene::SVertex> vertsHash( 1024 );

		//vector<NMeshData::SMeshData> *pData = &(terrainGfxInfo.terraPatches[nPatchY][nPatchX]);

		int nAlpha1, nAlpha2, nAlpha3, nAlpha4;
		static std::vector<int> maskLayers( 64 );

		vertsHash.clear();
		for ( int g = nTileY1; g < nTileY2; ++g )
		{
			for ( int i = nTileX1; i < nTileX2; ++i )
			{
				maskLayers.resize( 0 );
				for ( int k = tileTerraMasks.size() - 1; k >= 0; --k )
				{
					const CArray2D<uint8_t> &mask = tileTerraMasks[k];
					if ( (mask[g][i] > 0) || (mask[g][i + 1] > 0) || (mask[g + 1][i + 1] > 0) || (mask[g + 1][i] > 0) )
						maskLayers.push_back( k );
				}
				if ( maskLayers.empty() )
					continue;

				const STerrainInfo::STile &tile = terrainInfo.tiles[g][i];
				for ( std::vector<STriangle>::const_iterator it = tile.triangles.begin(); it != tile.triangles.end(); ++it )
				{
					const CVec3fEx &_v1 = tile.vertices[it->i1];
					const CVec3fEx &_v2 = tile.vertices[it->i2];
					const CVec3fEx &_v3 = tile.vertices[it->i3];
					const CVec3 v1( _v1.x, _v1.y, max(_v1.z + tile.addHeights[it->i1], 0.0f) );
					const CVec3 v2( _v2.x, _v2.y, max(_v2.z + tile.addHeights[it->i2], 0.0f) );
					const CVec3 v3( _v3.x, _v3.y, max(_v3.z + tile.addHeights[it->i3], 0.0f) );
					const int nInd1 = AddGfxVertex( &vertsHash, v1, fPatchX, fPatchY, patchNorms );
					const int nInd2 = AddGfxVertex( &vertsHash, v2, fPatchX, fPatchY, patchNorms );
					const int nInd3 = AddGfxVertex( &vertsHash, v3, fPatchX, fPatchY, patchNorms );
					const NGScene::SVertex &vert1 = vertsHash[nInd1];
					const NGScene::SVertex &vert2 = vertsHash[nInd2];
					const NGScene::SVertex &vert3 = vertsHash[nInd3];

					const float fTileX = (float)i * DEF_TILE_SIZE;
					const float fTileY = (float)g * DEF_TILE_SIZE;
					const float dx1 = ( v1.x - fTileX ) * DEF_INV_TILE_SIZE;
					const float dy1 = ( v1.y - fTileY ) * DEF_INV_TILE_SIZE;
					const float dx2 = ( v2.x - fTileX ) * DEF_INV_TILE_SIZE;
					const float dy2 = ( v2.y - fTileY ) * DEF_INV_TILE_SIZE;
					const float dx3 = ( v3.x - fTileX ) * DEF_INV_TILE_SIZE;
					const float dy3 = ( v3.y - fTileY ) * DEF_INV_TILE_SIZE;

					nAlpha1 = nAlpha2 = nAlpha3 = nAlpha4 = 0;
					for ( int k = 0; k < maskLayers.size(); ++k )
					{
						const int nLayer = maskLayers[k];
						if ( k < (maskLayers.size() - 1) )
						{
							const CArray2D<uint8_t> &mask = tileTerraMasks[nLayer];
							nAlpha1 = min( nAlpha1 + mask[g][i], 255 );
							nAlpha2 = min( nAlpha2 + mask[g][i + 1], 255 );
							nAlpha3 = min( nAlpha3 + mask[g + 1][i], 255 );
							nAlpha4 = min( nAlpha4 + mask[g + 1][i + 1], 255 );
						}
						else
							nAlpha1 = nAlpha2 = nAlpha3 = nAlpha4 = 255;
						const int nSub1 = nAlpha2 - nAlpha1;
						const int nSub2 = nAlpha4 - nAlpha3;
						const uint8_t cA1 = min( Float2Int(((float)nAlpha1 + (float)nSub1 * dx1) * (1.0f - dy1) + ((float)nAlpha3 + (float)nSub2 * dx1) * dy1), 255 );
						const uint8_t cA2 = min( Float2Int(((float)nAlpha1 + (float)nSub1 * dx2) * (1.0f - dy2) + ((float)nAlpha3 + (float)nSub2 * dx2) * dy2), 255 );
						const uint8_t cA3 = min( Float2Int(((float)nAlpha1 + (float)nSub1 * dx3) * (1.0f - dy3) + ((float)nAlpha3 + (float)nSub2 * dx3) * dy3), 255 );
						std::unordered_map<int, int> &curPatchVertsHash = patchesVertsHash[nLayer];
						const int &nRealNum = tilesOrder[nLayer].nPrevNum;
						//NMeshData::SMeshData &curData = data[nRealNum];
						NMeshData::SMeshData &curData = terrainGfxInfo.terraPatches[nPatchInd + p][nRealNum];

						const float &fTexScaleCoeff = pDesc->pTerraSet->terraTypes[nRealNum]->fScaleCoeff;

						const int nLayerInd1 = AddLayerVertex( &curPatchVertsHash, &curData.vertices, &(curData.attributes[0].data), nInd1, vert1, fTexScaleCoeff, preLightHash );
						const int nLayerInd2 = AddLayerVertex( &curPatchVertsHash, &curData.vertices, &(curData.attributes[0].data), nInd2, vert2, fTexScaleCoeff, preLightHash );
						const int nLayerInd3 = AddLayerVertex( &curPatchVertsHash, &curData.vertices, &(curData.attributes[0].data), nInd3, vert3, fTexScaleCoeff, preLightHash );

						if ( pDesc->pTerraSet->bWrapTexture )
							WrapChapterTexture( &curData.vertices, pDesc->nNumPatchesX, pDesc->nNumPatchesY, fTexScaleCoeff );

						curData.vertices[nLayerInd1].normal.w = max( cA1, curData.vertices[nLayerInd1].normal.w );
						curData.vertices[nLayerInd2].normal.w = max( cA2, curData.vertices[nLayerInd2].normal.w );
						curData.vertices[nLayerInd3].normal.w = max( cA3, curData.vertices[nLayerInd3].normal.w );

						if ( nLayerInd1 == nLayerInd2 || nLayerInd1 == nLayerInd3 || nLayerInd2 == nLayerInd3 )
						{
							if ( (nAlpha1 == 255) && (nAlpha2 == 255) && (nAlpha3 == 255) && (nAlpha4 == 255) )
								break;

							continue;						
						}

						bool bFound = false;
						for ( int iTriangle = 0; iTriangle < curData.triangles.size(); ++iTriangle )
						{
							if ( (curData.triangles[iTriangle].i1 == nLayerInd1 || curData.triangles[iTriangle].i1 == nLayerInd2 || curData.triangles[iTriangle].i1 == nLayerInd3) &&
								(curData.triangles[iTriangle].i2 == nLayerInd1 || curData.triangles[iTriangle].i2 == nLayerInd2 || curData.triangles[iTriangle].i2 == nLayerInd3) &&
								(curData.triangles[iTriangle].i3 == nLayerInd1 || curData.triangles[iTriangle].i3 == nLayerInd2 || curData.triangles[iTriangle].i3 == nLayerInd3) )
							{
								bFound = true;
								continue;
							}
						}

						if ( bFound )
						{
							if ( (nAlpha1 == 255) && (nAlpha2 == 255) && (nAlpha3 == 255) && (nAlpha4 == 255) )
								break;

							continue;						
						}

						curData.triangles.push_back( STriangle(nLayerInd1, nLayerInd2, nLayerInd3) );

						if ( (nAlpha1 == 255) && (nAlpha2 == 255) && (nAlpha3 == 255) && (nAlpha4 == 255) )
							break;
					}
				}
				//if ( terrainInfo.seaMask.IsEmpty() || !( terrainInfo.seaMask[g][i] && waterBottomMap[g][i] ) )
				//{
				//	const STerrainInfo::STile &tile = terrainInfo.tiles[g][i];
				//	for ( vector<STriangle>::const_iterator it = terrainInfo.tiles[g][i].triangles.begin(); it != terrainInfo.tiles[g][i].triangles.end(); ++it )
				//	{
				//		const CVec3fEx &vTerra1 = tile.vertices[it->i1];
				//		const CVec3fEx &vTerra2 = tile.vertices[it->i2];
				//		const CVec3fEx &vTerra3 = tile.vertices[it->i3];

				//		const CVec3 v1( vTerra1.x, vTerra1.y, vTerra1.z + tile.addHeights[it->i1] );
				//		const CVec3 v2( vTerra2.x, vTerra2.y, vTerra2.z + tile.addHeights[it->i2] );
				//		const CVec3 v3( vTerra3.x, vTerra3.y, vTerra3.z + tile.addHeights[it->i3] );

				//		AddTileTriangle( pData, 0, v1, v2, v3, VNULL2, VNULL2, VNULL2, i, g, nPatchX, nPatchY,
				//										 0, 0, 0, 0, 0, 0, bNeedFaster );
				//	}
				//}
			}
		}
	}

	for ( int i = 0; i < DEF_BREAK_TERRA_PATCHES; ++i )
	{
		for ( int k = 0; k < tileTerraMasks.size(); ++k )
		{
			//NMeshData::SMeshData &data = terrainGfxInfo.terraPatches[nPatchY][nPatchX][k];
			NMeshData::SMeshData &data = terrainGfxInfo.terraPatches[nPatchInd + i][k];
			std::vector<NGScene::SVertex>( data.vertices ).swap( data.vertices );
			std::vector<STriangle>( data.triangles ).swap( data.triangles );
			std::vector<uint32_t>( data.attributes[0].data ).swap( data.attributes[0].data );
			if ( data.attributes[0].data.empty() )
				data.attributes.clear();
		}
	}

	// release unused attributes
	//for ( vector<NMeshData::SMeshData>::iterator it = data.begin(); it != data.end(); ++it )
	//{
	//	if ( it->attributes[0].data.empty() )
	//		it->attributes[0].data.resize( 0 );
	//}

	if ( pGfxObserver )
	{
		for ( int i = 0; i < DEF_BREAK_TERRA_PATCHES; ++i )
			pGfxObserver->UpdatePatchGeometry( &terrainGfxInfo.terraPatches[nPatchInd + i], nPatchInd + i );
	}
}



