#include "stdafx.h"

#include "GenTerrain.h"

extern bool g_bDontUpdateRoads;

void CTerraGen::CollectAllCragsAndRiversInArea( std::vector<int> *pColCrags, std::vector<int> *pColRivers,
																							 const CVec2i &vBBMin, const CVec2i &vBBMax,
																							 const int nExcludeCragID, const int nExcludeRiverID )
{
	pColCrags->resize( 0 );
	pColRivers->resize( 0 );

	// collect all crags, which are intersected by this crag
	for ( std::list<STerrainInfo::SCrag>::const_iterator it = terrainInfo.crags.begin(); it != terrainInfo.crags.end(); ++it )
	{
		if ( it->nID != nExcludeCragID )
		{
			if ( IsBBIntersect(vBBMin, vBBMax, it->vRidgeMin, it->vRidgeMax) )
				pColCrags->push_back( it->nID );
		}
	}

	// collect all rivers, which are intersected by this crag
	for ( std::list<STerrainInfo::SRiver>::const_iterator it = terrainInfo.rivers.begin(); it != terrainInfo.rivers.end(); ++it )
	{
		if ( it->nID != nExcludeRiverID )
		{
			if ( IsBBIntersect(vBBMin, vBBMax, it->vSampMin, it->vSampMax) )
				pColRivers->push_back( it->nID );
		}
	}
}


void CTerraGen::UpdateCragsAndRiversInArea( const std::vector<int> &updCrags, const std::vector<int> &updRivers,
																					 const CVec2i &vBBMin, const CVec2i &vBBMax )
{
	// remove parts of all affected crags
	for ( std::vector<int>::const_iterator it = updCrags.begin(); it != updCrags.end(); ++it )
	{
		RemoveFoot( *it );
	}

	ResetTerrainTiles( vBBMin.x, vBBMin.y, vBBMax.x + 1, vBBMax.y + 1 ); // ????? about +1 ?????
	UpdateHeightsAfterRivers( vBBMin.x, vBBMin.y, vBBMax.x + 1, vBBMax.y + 1 );

	for ( std::vector<int>::const_iterator it = updRivers.begin(); it != updRivers.end(); ++it )
		PutRiverOnTerrain( FindRiverInfo(*it), vBBMin, vBBMax );

	for ( std::vector<int>::const_iterator it = updCrags.begin(); it != updCrags.end(); ++it )
		PutCragOnTerrain( FindCragInfo(*it), vBBMin, vBBMax );
}


void CTerraGen::UpdateAllOnTerrainObjectsInArea( const CVec2i &vMin, const CVec2i &vMax )
{
	AddAllNeededFoots();

	static std::vector<int> updIDs( 128 );

	const CVec2 vWorldMin( (float)vMin.x * DEF_TILE_SIZE, (float)vMin.y * DEF_TILE_SIZE );
	const CVec2 vWorldMax( (float)(vMax.x + 1) * DEF_TILE_SIZE, (float)(vMax.y + 1) * DEF_TILE_SIZE );

	// update roads
	if ( !g_bDontUpdateRoads )
	{
		updIDs.resize( 0 );
		for ( std::list<STerrainInfo::SRoad>::const_iterator it = terrainInfo.roads.begin(); it != terrainInfo.roads.end(); ++it )
		{
			if ( IsBBIntersect(vWorldMin, vWorldMax, it->vBBMin, it->vBBMax) )
				updIDs.push_back( it->nID );
		}
		for ( std::vector<int>::const_iterator it = updIDs.begin(); it != updIDs.end(); ++it )
			UpdateRoad( *it );
	}

	// update spots
	updIDs.resize( 0 );
	for ( std::list<STerrainInfo::STerraSpot>::const_iterator it = terrainInfo.terraspots.begin(); it != terrainInfo.terraspots.end(); ++it )
	{
		if ( IsBBIntersect(vMin, vMax, it->vMin, it->vMax) )
			updIDs.push_back( it->nID );
	}
	for ( std::vector<int>::const_iterator it = updIDs.begin(); it != updIDs.end(); ++it )
		UpdateTerraSpot( *it );
}


void CTerraGen::ConvertVSOPointsFromAIToVisAndPutOnTerrain( std::vector<NDb::SVSOPoint> *pDstPoints, const std::vector<NDb::SVSOPoint> &srcPoints )
{
	float fHeight;
	pDstPoints->reserve( srcPoints.size() );
	pDstPoints->resize( 0 );
	for ( std::vector<NDb::SVSOPoint>::const_iterator it = srcPoints.begin(); it != srcPoints.end(); ++it )
	{
		pDstPoints->push_back( *it );
		AI2Vis( &(pDstPoints->back().vPos) );
		AI2Vis( &(pDstPoints->back().fWidth) );
		GetMaxCragHeightEx( CVec2( pDstPoints->back().vPos.x, pDstPoints->back().vPos.y ), &fHeight );
		pDstPoints->back().vPos.z = GetTerraHeight( pDstPoints->back().vPos.x, pDstPoints->back().vPos.y ) + fHeight;
	}
}


void CTerraGen::UpdateVectorAreaInfo( const int nTileX1, const int nTileY1, const int nTileX2, const int nTileY2, DWORD dwUpdateFlags )
{
	static std::vector<int> updatedCrags( 128 );
	static std::vector<int> updatedRivers( 128 );

	CollectAllCragsAndRiversInArea( &updatedCrags, &updatedRivers, CVec2i(nTileX1, nTileY1), CVec2i(nTileX2, nTileY2), -1, -1 );
	UpdateCragsAndRiversInArea( updatedCrags, updatedRivers, CVec2i(nTileX1, nTileY1), CVec2i(nTileX2, nTileY2) );
	UpdateArea( nTileX1, nTileY1, nTileX2 + 1, nTileY2 + 1, dwUpdateFlags );
	UpdateAllOnTerrainObjectsInArea( CVec2i(nTileX1, nTileY1), CVec2i(nTileX2, nTileY2) );
}



