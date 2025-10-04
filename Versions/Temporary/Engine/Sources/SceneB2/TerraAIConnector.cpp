#include "stdafx.h"

#include "GenTerrain.h"
#include "Stats_B2_M1/TerraAIObserver.h"

#define DEF_WATER_TYPE 0xff

bool CTerraGen::IsPointOnBridge( float x, float y ) const
{
	return ( pAIObserver ? pAIObserver->IsBridge(x, y) : false );
}


void CTerraGen::RegenerateAIInfo()
{
	//NI_VERIFY( pAIObserver, "RegenerateAIInfo - AI Observer does not exist", return )
	//
	UpdateAllAIInfo();
	PutAllFeaturesToAI();
	if ( pAIObserver )
		pAIObserver->FinalizeUpdates();
}


void CTerraGen::UpdateAIInfo()
{
	// clear ai info
	PutAllFeaturesToAI();
}


void CTerraGen::UpdateAITerraTypes( const bool bForceUpdateAll )
{
	//NI_VERIFY( pAIObserver, "UpdateAITerraTypes - AI Observer does not exist", return )
	//
	if ( !bForceUpdateAll && (vTexModMin.x < 0) )
		return;

	if ( bForceUpdateAll )
	{
		vTexModMin.Set( 0, 0 );
		vTexModMax.Set( terrainInfo.tiles.GetSizeX() - 1, terrainInfo.tiles.GetSizeY() - 1 );
	}

	if ( (terrainAIInfo.terrTypes.GetSizeX() != terrainInfo.tiles.GetSizeX()) ||
			 (terrainAIInfo.terrTypes.GetSizeX() != terrainInfo.tiles.GetSizeX()) )
	{
		terrainAIInfo.terrTypes.SetSizes( terrainInfo.tiles.GetSizeX(), terrainInfo.tiles.GetSizeY() );
		terrainAIInfo.terrTypes.FillZero();
	}

	for ( int g = vTexModMin.y; g <= vTexModMax.y; ++g )
	{
		for ( int i = vTexModMin.x; i <= vTexModMax.x; ++i )
		{
			terrainAIInfo.terrTypes[g][i] = ( terrainInfo.seaMask[g][i] ) ? DEF_WATER_TYPE : ( terrainInfo.tileTerraMap[g][i] );
		}
	}

	if ( pAIObserver )
		pAIObserver->UpdateTypes( vTexModMin.x, vTexModMin.y, vTexModMax.x + 1, vTexModMax.y + 1, terrainAIInfo.terrTypes );
}


void CTerraGen::SetTerraTypesToAI( const vector<NDb::STerrainAIProperties> &params ) const
{
	//NI_VERIFY( pAIObserver, "SetTerraTypesToAI - AI Observer does not exist", return )
	//
	if ( pAIObserver )
	{
		pAIObserver->SetTerraTypes( params );
	}
}


void CTerraGen::UpdateAIInfo( const int nX1, const int nY1, const int nX2, const int nY2 ) const
{
	//NI_VERIFY( pAIObserver, "UpdateAIInfo - AI Observer does not exist", return )
	//
	if ( pAIObserver )
	{
		pAIObserver->UpdateHeights( nX1, nY1, nX2 + 1, nY2 + 1, terrainAIInfo.heights );
		pAIObserver->UpdateTypes( nX1, nY1, nX2, nY2, terrainAIInfo.terrTypes );
	}
}


void CTerraGen::UpdateAllAIInfo()
{
	//NI_VERIFY( pAIObserver, "UpdateAllAIInfo - AI Observer does not exist", return )
	//
	TIME_STAT_START( CTerraGen__UpdateAllAIInfo )

	const int nMaxX = terrainInfo.heights.GetSizeX();
	const int nMaxY = terrainInfo.heights.GetSizeY();

	terrainAIInfo.heights.SetSizes( nMaxX, nMaxY );
	for ( int g = 0; g < nMaxY; ++g )
	{
		for ( int i = 0; i < nMaxX; ++i )
		{
			terrainAIInfo.heights[g][i] = max( Vis2AI(GetFullTerraHeight(i, g)), 0.0f );
		}
	}

	vector<NDb::STerrainAIProperties> params;
	params.reserve( pDesc->pTerraSet->terraTypes.size() );
	params.resize( 0 );
	for ( vector< CDBPtr< NDb::STGTerraType > >::const_iterator it = pDesc->pTerraSet->terraTypes.begin(); it != pDesc->pTerraSet->terraTypes.end(); ++it )
		params.push_back( (*it)->aIProperty );
	if ( pAIObserver )
		pAIObserver->SetTerraTypes( params );

	UpdateAITerraTypes( true );

	if ( pAIObserver )
		pAIObserver->UpdateHeights( 0, 0, nMaxX, nMaxY, terrainAIInfo.heights );

	TIME_STAT_FINISH( CTerraGen__UpdateAllAIInfo )
}


void CTerraGen::PutCragToAI( const NDb::SVSOInstance *pCragInstance ) const
{
	//NI_VERIFY( pAIObserver, "PutCragToAI - AI Observer does not exist", return )
	NI_VERIFY( pCragInstance, "PutCragToAI - Invalid crag instance", return )
	//
	if ( pAIObserver )
	{
		pAIObserver->AddCrag( pCragInstance );
	}
}


void CTerraGen::PutRiverToAI( const NDb::SVSOInstance *pRiverInstance ) const
{
	//NI_VERIFY( pAIObserver, "PutRiverToAI - AI Observer does not exist", return )
	NI_VERIFY( pRiverInstance, "PutRiverToAI - Invalid river instance", return )
	//
	if ( pAIObserver )
	{
		pAIObserver->AddRiver( pRiverInstance );
	}
}


void CTerraGen::PutRoadToAI( const NDb::SVSOInstance *pRoadInstance ) const
{
	//NI_VERIFY( pAIObserver, "PutRoadToAI - AI Observer does not exist", return )
	NI_VERIFY( pRoadInstance, "PutRoadToAI - invalid road instance", return )
	//
	if ( pAIObserver )
	{
		pAIObserver->AddRoad( pRoadInstance );
	}
}


void CTerraGen::PutAllCragsToAI() const
{
	//NI_VERIFY( pAIObserver, "PutAllCragsToAI - AI Observer does not exist", return )
	//
	//if ( pAIObserver )
	{
		for ( list<STerrainInfo::SCrag>::const_iterator it = terrainInfo.crags.begin(); it != terrainInfo.crags.end(); ++it )
		{
			const NDb::SVSOInstance *pInstance = FindCrag( it->nID );
			if ( pInstance )
				PutCragToAI( pInstance );
		}
	}
}


void CTerraGen::PutAllRiversToAI() const
{
	//NI_VERIFY( pAIObserver, "PutAllRiversToAI - AI Observer does not exist", return )
	//
	//if ( pAIObserver )
	{
		for ( list<STerrainInfo::SRiver>::const_iterator it = terrainInfo.rivers.begin(); it != terrainInfo.rivers.end(); ++it )
		{
			const NDb::SVSOInstance *pInstance = FindRiver( it->nID );
			if ( pInstance ) 
				PutRiverToAI( pInstance );
		}
	}
}


void CTerraGen::PutAllRoadsToAI() const
{
	//NI_VERIFY( pAIObserver, "PutAllRoadsToAI - AI Observer does not exist", return )
	//
	//if ( pAIObserver )
	{
		for ( list<STerrainInfo::SRoad>::const_iterator it = terrainInfo.roads.begin(); it != terrainInfo.roads.end(); ++it )
		{
			const NDb::SVSOInstance *pInstance = FindRoad( it->nID );
			if ( pInstance ) 
				PutRoadToAI( pInstance );
		}
	}
}


void CTerraGen::PutAllWaterToAI() const
{
	//NI_VERIFY( pAIObserver, "PutAllWaterToAI - AI Observer does not exist", return )
	//
	if ( pAIObserver )
	{
		// coast
		if ( pDesc->coast.points.size() >= 2 )
			pAIObserver->AddWaterLine( &(pDesc->coast), false );
		// lakes & islands
		for ( int i = 0; i < pDesc->lakes.size(); ++i )
		{
			if ( pDesc->lakes[i].points.size() >= 2 )
			{
				const NDb::SLakeDesc *pLakeDesc = static_cast<const NDb::SLakeDesc *>( pDesc->lakes[i].pDescriptor.GetPtr() );
				pAIObserver->AddWaterLine( &(pDesc->lakes[i]), true /*pLakeDesc->bIsLake*/ );
			}
		}
	}
}


void CTerraGen::PutAllFeaturesToAI() const
{
	PutAllRoadsToAI();
	PutAllWaterToAI();
	PutAllRiversToAI();
	PutAllCragsToAI();
}



