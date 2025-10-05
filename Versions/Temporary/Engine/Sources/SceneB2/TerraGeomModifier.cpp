#include "stdafx.h"

#include "GenTerrain.h"
#include "Scene.h"

static int nDiffHeightsX1 = -1;
static int nDiffHeightsY1 = -1;
static int nDiffHeightsX2 = -1;
static int nDiffHeightsY2 = -1;
static CArray2D<float> diffHeights;

void CTerraGen::ModifyTerraGeometryByBrush( const int nVisTileX, const int nVisTileY, bool bCenter,
																						const CArray2D<float> &brush,
																						const NTerraBrush::ETerraBrushUpdate terraBrushUpdate )
{
	if ( (terrainInfo.heights.GetSizeX() != diffHeights.GetSizeX()) ||
			 (terrainInfo.heights.GetSizeY() != diffHeights.GetSizeY()) )
	{
		diffHeights.SetSizes( terrainInfo.heights.GetSizeX(), terrainInfo.heights.GetSizeY() );
		diffHeights.FillEvery( 0.0f );
	}

	//const int nTileX = Clamp( nVisTileX, 0, terrainInfo.heights.GetSizeX() - 1 );
	//const int nTileY = Clamp( nVisTileY, 0, terrainInfo.heights.GetSizeY() - 1 );
	//
	const int nOffsX = bCenter ? ( nVisTileX - (brush.GetSizeX() >> 1) ) : nVisTileX;
	const int nOffsY = bCenter ? ( nVisTileY - (brush.GetSizeY() >> 1) ) : nVisTileY;
	//
	const int nBrushStartX = nOffsX >= 0 ? 0 : -nOffsX;
	const int nBrushStartY = nOffsY >= 0 ? 0 : -nOffsY;
	const int nBrushSizeX = min( nOffsX + brush.GetSizeX(), terrainInfo.heights.GetSizeX() ) - nOffsX;
	const int nBrushSizeY = min( nOffsY + brush.GetSizeY(), terrainInfo.heights.GetSizeY() ) - nOffsY;

	if ( terraBrushUpdate == NTerraBrush::TERRA_BRUSH_OVERRIDE )
	{
		if ( nDiffHeightsX1 == -1 )
		{
			nDiffHeightsX1 = nOffsX + nBrushStartX;
			nDiffHeightsY1 = nOffsY + nBrushStartY;
			nDiffHeightsX2 = nOffsX + nBrushSizeX - 1;
			nDiffHeightsY2 = nOffsY + nBrushSizeY - 1;
		}
		else
		{
			nDiffHeightsX1 = min( nDiffHeightsX1, nOffsX + nBrushStartX );
			nDiffHeightsY1 = min( nDiffHeightsY1, nOffsY + nBrushStartY );
			nDiffHeightsX2 = max( nDiffHeightsX2, nOffsX + nBrushSizeX - 1 );
			nDiffHeightsY2 = max( nDiffHeightsY2, nOffsY + nBrushSizeY - 1 );
		}
	}

	// update heights
	for ( int g = nBrushStartY; g < nBrushSizeY; ++g )
	{
		for ( int i = nBrushStartX; i < nBrushSizeX; ++i )
		{
			const int nIndX = i + nOffsX;
			const int nIndY = g + nOffsY;

			const float fVal = ( terraBrushUpdate != NTerraBrush::TERRA_BRUSH_SUB ) ?
													 max( terrainInfo.heights[nIndY][nIndX] + brush[g][i], 0.0f ) :
													 max( terrainInfo.heights[nIndY][nIndX] - brush[g][i], 0.0f );

			const float fReallyAdded = fVal - terrainInfo.heights[nIndY][nIndX];
			if ( terraBrushUpdate == NTerraBrush::TERRA_BRUSH_OVERRIDE )
				diffHeights[nIndY][nIndX] += fReallyAdded;

			terrainInfo.heights[nIndY][nIndX] = fVal;
			if ( terrainInfo.waterHeightCoeffs[nIndY][nIndX] < (1.0f - EPS_VALUE) )
				terrainInfo.waterAddHeights[nIndY][nIndX] = fReallyAdded;

			terrainAIInfo.heights[nIndY][nIndX] = Vis2AI( GetFullTerraHeight(nIndX, nIndY) );
		}
	}

	const int nFirstTileX = Clamp( nOffsX + nBrushStartX - 1, 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nFirstTileY = Clamp( nOffsY + nBrushStartY - 1, 0, terrainInfo.tiles.GetSizeY() - 1 );
	const int nLastTileX = Clamp( nOffsX + nBrushSizeX, 0, terrainInfo.tiles.GetSizeX() - 1 );
	const int nLastTileY = Clamp( nOffsY + nBrushSizeY, 0, terrainInfo.tiles.GetSizeY() - 1 );

	UpdateTileHeights( nFirstTileX, nFirstTileY, nLastTileX, nLastTileY );
	UpdateAIInfo( nFirstTileX, nFirstTileY, nLastTileX + 1, nLastTileY + 1 );
	UpdateNormals( nFirstTileX, nFirstTileY, nLastTileX, nLastTileY );

	if ( vGeomModAreaMin.x < 0 ) // first initialization
	{
		vGeomModAreaMin.Set( nFirstTileX, nFirstTileY );
		vGeomModAreaMax.Set( nLastTileX, nLastTileY );
	}
	else
	{
		vGeomModAreaMin.x = min( vGeomModAreaMin.x, nFirstTileX );
		vGeomModAreaMin.y = min( vGeomModAreaMin.y, nFirstTileY );
		vGeomModAreaMax.x = max( vGeomModAreaMax.x, nLastTileX );
		vGeomModAreaMax.y = max( vGeomModAreaMax.y, nLastTileY );
	}

	const int nPatchX1 = nFirstTileX >> DEF_PATCH_SIZE_BITS;
	const int nPatchY1 = nFirstTileY >> DEF_PATCH_SIZE_BITS;
	const int nPatchX2 = nLastTileX >> DEF_PATCH_SIZE_BITS;
	const int nPatchY2 = nLastTileY >> DEF_PATCH_SIZE_BITS;

	for ( int g = nPatchY1; g <= nPatchY2; ++g )
	{
		for ( int i = nPatchX1; i <= nPatchX2; ++i )
		{
			UpdateGfxInfo( i, g );
			needTexExportAfterGeomModifying[g][i] = 1;
		}
	}

	// grid updater (intellectual)
	if ( IScene *pScene = Scene() )
		pScene->UpdateGrid( nFirstTileX, nFirstTileY, nLastTileX, nLastTileY );
}


void CTerraGen::GetTerraGeometryUpdateDifferences( int *pOffsX, int *pOffsY, CArray2D<float> *pDiffs )
{
	if ( nDiffHeightsX1 != -1 )
	{
		// check for zero rows and columns
		/*bool bFlag;
		while ( nDiffHeightsX1 <= nDiffHeightsX2 )
		{
			bFlag = true;
			for ( int g = nDiffHeightsY1; g <= nDiffHeightsY2; ++g )
			{
				if ( fabs( diffHeights[g][nDiffHeightsX1] ) > EPS_VALUE )
				{
					bFlag = false;
					break;
				}
			}
			if ( bFlag )
				++nDiffHeightsX1;
			else
				break;
		}

		while ( nDiffHeightsX2 >= nDiffHeightsX1 )
		{
			bFlag = true;
			for ( int g = nDiffHeightsY1; g <= nDiffHeightsY2; ++g )
			{
				if ( fabs( diffHeights[g][nDiffHeightsX2] ) > EPS_VALUE )
				{
					bFlag = false;
					break;
				}
			}
			if ( bFlag )
				--nDiffHeightsX2;
			else
				break;
		}

		while ( nDiffHeightsY1 <= nDiffHeightsY2 )
		{
			bFlag = true;
			for ( int i = nDiffHeightsX1; i <= nDiffHeightsX2; ++i )
			{
				if ( fabs( diffHeights[nDiffHeightsY1][i] ) > EPS_VALUE )
				{
					bFlag = false;
					break;
				}
			}
			if ( bFlag )
				++nDiffHeightsY1;
			else
				break;
		}

		while ( nDiffHeightsY2 >= nDiffHeightsY1 )
		{
			bFlag = true;
			for ( int i = nDiffHeightsX1; i <= nDiffHeightsX2; ++i )
			{
				if ( fabs( diffHeights[nDiffHeightsY2][i] ) > EPS_VALUE )
				{
					bFlag = false;
					break;
				}
			}
			if ( bFlag )
				--nDiffHeightsY2;
			else
				break;
		}*/

		if ( (nDiffHeightsX1 <= nDiffHeightsX2) && (nDiffHeightsY1 <= nDiffHeightsY2) )
		{
			pDiffs->SetSizes( nDiffHeightsX2 - nDiffHeightsX1 + 1, nDiffHeightsY2 - nDiffHeightsY1 + 1 );
			*pOffsX = nDiffHeightsX1 + ( pDiffs->GetSizeX() >> 1 );
			*pOffsY = nDiffHeightsY1 + ( pDiffs->GetSizeY() >> 1 );
			for ( int g = 0; g < pDiffs->GetSizeY(); ++g )
			{
				for ( int i = 0; i < pDiffs->GetSizeX(); ++i )
				{
					(*pDiffs)[g][i] = diffHeights[g + nDiffHeightsY1][i + nDiffHeightsX1];
					diffHeights[g + nDiffHeightsY1][i + nDiffHeightsX1] = 0.0f;
				}
			}
			nDiffHeightsX1 = nDiffHeightsY1 = nDiffHeightsX2 = nDiffHeightsY2 = -1;
			return;
		}
	}

	pDiffs->SetSizes( 0, 0 );
	*pOffsX = *pOffsY = 0;
	nDiffHeightsX1 = nDiffHeightsY1 = nDiffHeightsX2 = nDiffHeightsY2 = -1;
}


static bool IsBrushTrue( const int nX, const int nY, const CArray2D<float> &brush )
{
	if ( (nX >= 0) && (nX < brush.GetSizeX()) && (nY >= 0) && (nY < brush.GetSizeY()) )
	{
		return ( fabs(brush[nY][nX]) > DEF_EPS );
	}
	return false;
}


void CTerraGen::UpdateTileHeights( const int nTileX1, const int nTileY1, const int nTileX2, const int nTileY2 )
{
	for ( int g = nTileY1; g <= nTileY2; ++g )
	{
		for ( int i = nTileX1; i <= nTileX2; ++i )
		{
			STerrainInfo::STile &curTile = terrainInfo.tiles[g][i];
			for ( std::vector<CVec3fEx>::iterator it = curTile.vertices.begin(); it != curTile.vertices.end(); ++it )
			{
				it->z = GetTerraHeight( it->x, it->y, i, g );
			}
		}
	}
}


void CTerraGen::UpdateAllObjectsInGeomModifyingArea()
{
	if ( vGeomModAreaMin.x == -1 )
		return;

	UpdateVectorAreaInfo( vGeomModAreaMin.x, vGeomModAreaMin.y, vGeomModAreaMax.x, vGeomModAreaMax.y, TERRAIN_UPDATE_ALL);

	vGeomModAreaMin.Set( -1, -1 );
	vGeomModAreaMax.Set( -1, -1 );
}


void CTerraGen::GetAreaHeights( CArray2D<float> *pAreaHeights, const int nX1, const int nY1, const int nX2, const int nY2 )
{
	const int nTileX1 = max( min(nX1, nX2), 0 );
	const int nTileY1 = max( min(nY1, nY2), 0 );
	const int nTileX2 = min( max(nX1, nX2), terrainInfo.heights.GetSizeX() - 1 );
	const int nTileY2 = min( max(nY1, nY2), terrainInfo.heights.GetSizeY() - 1 );
	if ( (nTileX1 <= nTileX2) && (nTileY1 <= nTileY2) )
	{
		pAreaHeights->SetSizes( nTileX2 - nTileX1 + 1, nTileY2 - nTileY1 + 1 );
		for ( int g = 0; g < pAreaHeights->GetSizeY(); ++g )
		{
			for ( int i = 0; i < pAreaHeights->GetSizeX(); ++i )
			{
				(*pAreaHeights)[g][i] = terrainInfo.heights[g + nTileY1][i + nTileX1];
			}
		}
	}
	else
	{
		pAreaHeights->SetSizes( 0, 0 );
	}
}


void CTerraGen::UpdateAfterTilesModifying()
{
	UpdateAITerraTypes( false );
	UpdateNeededPeaks();
	vTexModMin.Set( -1, -1 );
	vTexModMax.Set( -1, -1 );
}



