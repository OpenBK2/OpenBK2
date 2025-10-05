#include "stdafx.h"

#include "3Dmotor/DBScene.h"
#include "GenTerrain.h"

#include "System/Commands.h"
#include "System/VFSOperations.h"

#include <algorithm>


//#include <VTuneAPI.h>
//#pragma comment (lib, "vtuneapi.lib")

#define DEF_TILES_BLUR_FILTER_HX 2
#define DEF_TILES_BLUR_FILTER_HY 2

static CArray2D<BYTE> tileTerraMapDiffs;
static int nDiffTileX1 = -1, nDiffTileY1 = -1, nDiffTileX2 = -1, nDiffTileY2 = -1;
static int s_nTerrainBlurRaduis = 1;
static int s_nTerrainBlurThreshHold = 0;

static void BlurTileMask( CArray2D<BYTE> *pDstMask, const CArray2D<BYTE> &srcMask, const int nX1, const int nY1,
													const int nX2, const int nY2, const SInt2Type<0> &sep )
{
	// smooth mask
	for ( int g = 0; g < ( srcMask.GetSizeY() - 1 ); ++g )
	{
		for ( int i = 0; i < ( srcMask.GetSizeX() - 1 ); ++i )
		{
			(*pDstMask)[g][i] = ( (int)srcMask[g][i] + srcMask[g][i + 1] + srcMask[g + 1][i + 1] + srcMask[g + 1][i] ) >> 2;
		}
	}

	const int nLastColumn = srcMask.GetSizeX() - 1;
	for ( int g = 0; g < ( srcMask.GetSizeY() - 1 ); ++g )
		(*pDstMask)[g][nLastColumn] = ( (int)srcMask[g][nLastColumn] + srcMask[g + 1][nLastColumn] ) >> 1;

	const int nLastRow = srcMask.GetSizeY() - 1;
	for ( int i = 0; i < ( srcMask.GetSizeX() - 1 ); ++i )
		(*pDstMask)[nLastRow][i] = ( (int)srcMask[nLastRow][i] + srcMask[nLastRow][i + 1] ) >> 1;

	(*pDstMask)[nLastRow][nLastColumn] = srcMask[nLastRow][nLastColumn];
}

static void BlurSingleElement( CArray2D<BYTE> *pDstMask, const CArray2D<BYTE> &srcMask, const CArray2D<int> &filter,
															 const int nFilterHalfSize, const int nStartFilterX, const int nStartFilterY,
															 const int nEndFilterX, const int nEndFilterY, const int x, const int y )
{
	int nVal = 0, nCount = 0;
	for ( int gg = nStartFilterY; gg < nEndFilterY; ++gg )
	{
		for ( int ii = nStartFilterX; ii < nEndFilterX; ++ii )
		{
			nVal += filter[gg][ii] * srcMask[y - nFilterHalfSize + gg][x - nFilterHalfSize + ii];
			nCount += filter[gg][ii];
		}
	}
	if ( nCount == 0 )
	{
		nCount = 0;
		nVal = 0;
	}
	(*pDstMask)[y][x] = nVal / nCount;
}

static void BlurLastColumn( CArray2D<BYTE> *pDstMask, const CArray2D<BYTE> &srcMask, const CArray2D<int> &filter,
														const int nFilterHalfSize, const int nStartFilterX, const int nEndFilterX,
														const int x, const int nY1, const int nY2 )
{
	for ( int g = nY1; g <= nY2; ++g )
	{
		const int nStartFilterY = ( g >= nFilterHalfSize ) ? 0 : ( nFilterHalfSize - g );
		const int nEndFilterY = ( g < ( srcMask.GetSizeY() - nFilterHalfSize ) ) ? filter.GetSizeY() :
		( 2 * nFilterHalfSize - ( srcMask.GetSizeY() - g ) );
		BlurSingleElement( pDstMask, srcMask, filter, nFilterHalfSize, nStartFilterX, nStartFilterY, nEndFilterX, nEndFilterY, x, g );
	}
}

static void BlurTileMask( CArray2D<BYTE> *pDstMask, const CArray2D<BYTE> &srcMask, const int nX1, const int nY1,
													const int nX2, const int nY2, const SInt2Type<1> &sep )
{
	
	
	//filter[0][0] = 1; filter[0][1] = 4;	filter[0][2] = 7;	filter[0][3] = 4;	filter[0][4] = 1;
	//filter[1][0] = 4; filter[1][1] = 16; filter[1][2] = 26; filter[1][3] = 16; filter[1][4] = 4;
	//filter[2][0] = 7; filter[2][1] = 26; filter[2][2] = 41; filter[2][3] = 26; filter[2][4] = 7;
	//filter[3][0] = 4; filter[3][1] = 16; filter[3][2] = 26; filter[3][3] = 16; filter[3][4] = 4;
	//filter[4][0] = 1; filter[4][1] = 4;	filter[4][2] = 7;	filter[4][3] = 4;	filter[4][4] = 1;
	//const int nScale = 273;

	CArray2D<int> filter;
	int nScale;
	
	if ( s_nTerrainBlurRaduis == 2)
	{
		filter.SetSizes( 5, 5 );
		filter[0][0] = 0; filter[0][1] = 1; filter[0][2] = 2;	filter[0][3] = 1; filter[0][4] = 0;
		filter[1][0] = 1; filter[1][1] = 4; filter[1][2] = 8;	filter[1][3] = 4; filter[1][4] = 1;
		filter[2][0] = 2; filter[2][1] = 8; filter[2][2] = 16; filter[2][3] = 8; filter[2][4] = 2;
		filter[3][0] = 1; filter[3][1] = 4; filter[3][2] = 8;	filter[3][3] = 4; filter[3][4] = 1;
		filter[4][0] = 0; filter[4][1] = 1; filter[4][2] = 2;	filter[4][3] = 1; filter[4][4] = 0;
		nScale = 80;
	}
	else
	{
		// s_nTerrainBlurRaduis = 1
		filter.SetSizes( 3, 3 );
		filter[0][0] = 1; filter[0][1] = 2; filter[0][2] = 1;	
		filter[1][0] = 2; filter[1][1] = 4; filter[1][2] = 2;	
		filter[2][0] = 1; filter[2][1] = 2; filter[2][2] = 1;
		nScale = 16;
	}

	const int nFilterHalfSize = filter.GetSizeX() >> 1;

	const int nLastClearSizeX = srcMask.GetSizeX() - nFilterHalfSize - 1;
	const int nLastClearSizeY = srcMask.GetSizeY() - nFilterHalfSize - 1;

	int nVal;

	const int nBlurX1 = max( nX1, nFilterHalfSize );
	const int nBlurY1 = max( nY1, nFilterHalfSize );
	const int nBlurX2 = min( nX2, nLastClearSizeX );
	const int nBlurY2 = min( nY2, nLastClearSizeY );

	for ( int g = nBlurY1; g <= nBlurY2; ++g )
	{
		for ( int i = nBlurX1; i <= nBlurX2; ++i )
		{
			nVal = 0;
			for ( int gg = 0; gg < filter.GetSizeY(); ++gg )
			{
				for ( int ii = 0; ii < filter.GetSizeX(); ++ii )
				{
					nVal += filter[gg][ii] * srcMask[g - nFilterHalfSize + gg][i - nFilterHalfSize + ii];
				}
			}
			(*pDstMask)[g][i] = nVal / nScale;
		}
	}

	{
		const int nColumnX1 = max( 0, nX1 );
		const int nColumnX2 = min( nFilterHalfSize - 1, nX2 );
		for ( int i = nColumnX1; i <= nColumnX2; ++i )
		{
			const int nStartFilterX = nFilterHalfSize - i;
			BlurLastColumn( pDstMask, srcMask, filter, nFilterHalfSize, nStartFilterX, filter.GetSizeX(), i, nY1, nY2 );
		}
	}
	{
		const int nStartX = srcMask.GetSizeX() - nFilterHalfSize;
		const int nColumnX1 = max( nStartX, nX1 );
		const int nColumnX2 = min( srcMask.GetSizeX() - 1, nX2 );
		for ( int i = nColumnX1; i <= nColumnX2; ++i )
		{
			const int nEndFilterX = filter.GetSizeX() - 1 - ( i - nStartX );
			BlurLastColumn( pDstMask, srcMask, filter, nFilterHalfSize, 0, nEndFilterX, i, nY1, nY2 );
		}
	}

	const int nLastX = srcMask.GetSizeX() - nFilterHalfSize - 1;
	const int nRowX1 = max( nFilterHalfSize, nX1 );
	const int nRowX2 = min( nLastX, nX2 );
	for ( int i = nRowX1; i <= nRowX2; ++i )
	{
		{
			const int nRowY1 = max( 0, nY1 );
			const int nRowY2 = min( nFilterHalfSize - 1, nY2 );
			for ( int g = nRowY1; g <= nRowY2; ++g )
			{
				const int nStartFilterY = nFilterHalfSize - g;
				BlurSingleElement( pDstMask, srcMask, filter, nFilterHalfSize, 0, nStartFilterY, filter.GetSizeX(), filter.GetSizeY(), i, g );
			}
		}
		{
			const int nStartY = srcMask.GetSizeY() - nFilterHalfSize;
			const int nRowY1 = max( nStartY, nY1 );
			const int nRowY2 = min( srcMask.GetSizeY() - 1, nY2 );
			for ( int g = nRowY1; g <= nRowY2; ++g )
			{
				const int nEndFilterY = filter.GetSizeY() - 1 - ( g - nStartY );
				BlurSingleElement( pDstMask, srcMask, filter, nFilterHalfSize, 0, 0, filter.GetSizeX(), nEndFilterY, i, g );
			}
		}
	}
}

//void CTerraGen::ConvertZoneMaskToTileMap()
//{
//	tileTerraMap.SetSizes( pDesc->nNumPatchesX * DEF_PATCH_SIZE + 1, pDesc->nNumPatchesY * DEF_PATCH_SIZE + 1 );
//	if ( modZoneMaskTex.IsEmpty() )
//		LoadZoneMaskTex();
//
//	if ( ( ( pDesc->nNumPatchesX * DEF_PATCH_SIZE * DEF_PATCH_PIX_PER_TILE ) != modZoneMaskTex.GetSizeX() ) ||
//			 ( ( pDesc->nNumPatchesY * DEF_PATCH_SIZE * DEF_PATCH_PIX_PER_TILE ) != modZoneMaskTex.GetSizeY() ) )
//	{
//		NI_ASSERT( false, "Wrong zone mask, convertion to tile map failed" );
//		tileTerraMap.FillZero();
//	}
//	else
//	{
//		const int nPixCheckRadius = DEF_PATCH_PIX_PER_TILE >> 1;
//		vector<int> terraCounts( tileTerraMasks.size() );
//		int nMaxTerrain;
//		const int nSizeX = tileTerraMap.GetSizeX() - 1;
//		const int nSizeY = tileTerraMap.GetSizeY() - 1;
//		for ( int g = 0; g < nSizeY; ++g )
//		{
//			for ( int i = 0; i < nSizeX; ++i )
//			{
//				const int nPixX1 = max( ( i << DEF_PATCH_PIX_PER_TILE_BITS ) - nPixCheckRadius, 0 );
//				const int nPixY1 = max( ( g << DEF_PATCH_PIX_PER_TILE_BITS ) - nPixCheckRadius, 0 );
//				const int nPixX2 = min( ( i << DEF_PATCH_PIX_PER_TILE_BITS ) + nPixCheckRadius, modZoneMaskTex.GetSizeX() );
//				const int nPixY2 = min( ( g << DEF_PATCH_PIX_PER_TILE_BITS ) + nPixCheckRadius, modZoneMaskTex.GetSizeY() );
//				const int nScale = ( nPixX2 - nPixX1 ) * ( nPixY2 - nPixY1 );
//				fill( terraCounts.begin(), terraCounts.end(), 0 );
//
//				if ( nScale > 0 )
//				{
//					for ( int gg = nPixY1; gg < nPixY2; ++gg )
//					{
//						for ( int ii = nPixX1; ii < nPixX2; ++ii )
//						{
//							const BYTE cTexCol = modZoneMaskTex[gg][ii];
//							if ( cTexCol < terraCounts.size() )
//								++terraCounts[cTexCol];
//						}
//					}
//					nMaxTerrain = 0;
//					for ( int k = 1; k < terraCounts.size(); ++k )
//					{
//						if ( terraCounts[k] > terraCounts[nMaxTerrain] )
//							nMaxTerrain = k;
//					}
//					tileTerraMap[g][i] = nMaxTerrain;
//				}
//				else
//					tileTerraMap[g][i] = 0;
//			}
//		}
//
//		for ( int g = 0; g < nSizeY; ++g )
//			tileTerraMap[g][nSizeX] = tileTerraMap[g][nSizeX - 1];
//
//		for ( int i = 0; i < nSizeX; ++i )
//			tileTerraMap[nSizeY][i] = tileTerraMap[nSizeY - 1][i];
//
//		tileTerraMap[nSizeY][nSizeX] = tileTerraMap[nSizeY - 1][nSizeX - 1];
//	}
//}

void CTerraGen::UpdateTileMasks( const int nX1, const int nY1, const int nX2, const int nY2, const bool bNeedGeomUpdate )
{
	static CArray2D<BYTE> tempTileMask;
	tempTileMask.SetSizes( terrainInfo.tileTerraMap.GetSizeX(), terrainInfo.tileTerraMap.GetSizeY() );

	// sort for true order
	tilesOrder.resize( pDesc->pTerraSet->terraTypes.size() );

	NI_ASSERT( tileTerraMasks.size() == pDesc->pTerraSet->terraTypes.size(), "Wrong tiles mask" );
	if ( tileTerraMasks.size() != pDesc->pTerraSet->terraTypes.size() )
		return;

	for ( int k = 0; k < tilesOrder.size(); ++k )
	{
		STileOrder &ord = tilesOrder[k];
		ord.nPrevNum = k;
		ord.nPriority = pDesc->pTerraSet->terraTypes[k]->pMaterial->nPriority;
	}
	std::sort( tilesOrder.begin(), tilesOrder.end() );

	const int nBlurX1 = max( nX1 - DEF_TILES_BLUR_FILTER_HX, 0 );
	const int nBlurY1 = max( nY1 - DEF_TILES_BLUR_FILTER_HX, 0 );
	const int nBlurX2 = min( nX2 + DEF_TILES_BLUR_FILTER_HX, terrainInfo.tileTerraMap.GetSizeX() - 1 );
	const int nBlurY2 = min( nY2 + DEF_TILES_BLUR_FILTER_HX, terrainInfo.tileTerraMap.GetSizeY() - 1 );
	const int nAffectX1 = max( nBlurX1 - DEF_TILES_BLUR_FILTER_HX, 0 );
	const int nAffectY1 = max( nBlurY1 - DEF_TILES_BLUR_FILTER_HY, 0 );
	const int nAffectX2 = min( nBlurX2 + DEF_TILES_BLUR_FILTER_HX, terrainInfo.tileTerraMap.GetSizeX() - 1 );
	const int nAffectY2 = min( nBlurY2 + DEF_TILES_BLUR_FILTER_HX, terrainInfo.tileTerraMap.GetSizeY() - 1 );

	for ( int k = 0; k < tileTerraMasks.size(); ++k )
	{
		CArray2D<BYTE> &curMask = tileTerraMasks[k];
		// fill mask
		for ( int g = nAffectY1; g <= nAffectY2; ++g )
		{
			for ( int i = nAffectX1; i <= nAffectX2; ++i )
			{
				//tempTileMask[g][i] = ( terrainInfo.tileTerraMap[g][i] == k ) ? 255 : 0;
				tempTileMask[g][i] = ( terrainInfo.tileTerraMap[g][i] == tilesOrder[k].nPrevNum ) ? 255 : 0;
			}
		}

		SInt2Type<1> separtor;
		BlurTileMask( &curMask, tempTileMask, nBlurX1, nBlurY1, nBlurX2, nBlurY2, separtor );
	}

	// Optimizing
	if ( s_nTerrainBlurThreshHold > 0 )
	{
		for ( int k = tileTerraMasks.size() - 1; k >= 0; --k )
		{
			CArray2D<BYTE> &curMask = tileTerraMasks[k];
			// fill mask
			for ( int g = nAffectY1; g <= nAffectY2; ++g )
			{
				for ( int i = nAffectX1; i <= nAffectX2; ++i )
				{
					if ( curMask[g][i] < s_nTerrainBlurThreshHold )
					{
						for ( int kk = k-1; kk >= 0; --kk )
						{
							if ( tileTerraMasks[kk][g][i] > 0 )
							{
								curMask[g][i] = 0;
								break;
							}
						}
					}
				}
			}
		}
	}


	if ( bNeedGeomUpdate )
	{
		const int nPatchX1 = min( nBlurX1 >> DEF_PATCH_SIZE_BITS, pDesc->nNumPatchesX - 1 );
		const int nPatchY1 = min( nBlurY1 >> DEF_PATCH_SIZE_BITS, pDesc->nNumPatchesY - 1 );
		const int nPatchX2 = min( nBlurX2 >> DEF_PATCH_SIZE_BITS, pDesc->nNumPatchesX - 1 );
		const int nPatchY2 = min( nBlurY2 >> DEF_PATCH_SIZE_BITS, pDesc->nNumPatchesY - 1 );

		UpdateGfxInfo( nPatchX1, nPatchY1, nPatchX2 + 1, nPatchY2 + 1 );
	}
}

void CTerraGen::UpdateTileAreaType( const float fXo, const float fYo, const CArray2D<BYTE> &mask,
																		const NTerraBrush::ETerraBrushUpdate terraBrushUpdate )
{
	//VTResume();

	if ( ( terrainInfo.tileTerraMap.GetSizeX() != tileTerraMapDiffs.GetSizeX() ) ||
			 ( terrainInfo.tileTerraMap.GetSizeY() != tileTerraMapDiffs.GetSizeY() ) )
	{
		tileTerraMapDiffs.SetSizes( terrainInfo.tileTerraMap.GetSizeX(), terrainInfo.tileTerraMap.GetSizeY() );
		tileTerraMapDiffs.FillZero();
	}

	const int nX1 = AI2Vis( fXo ) * DEF_INV_TILE_SIZE;
	const int nY1 = AI2Vis( fYo ) * DEF_INV_TILE_SIZE;
	const int nX2 = nX1 + mask.GetSizeX() - 1;
	const int nY2 = nY1 + mask.GetSizeY() - 1;

	const int nTileX1 = max( nX1, 0 );
	const int nTileY1 = max( nY1, 0 );
	const int nTileX2 = min( nX2, terrainInfo.tileTerraMap.GetSizeX() - 1 );
	const int nTileY2 = min( nY2, terrainInfo.tileTerraMap.GetSizeY() - 1 );

	if ( terraBrushUpdate == NTerraBrush::TERRA_BRUSH_OVERRIDE )
	{
		if ( nDiffTileX1 == -1 )
		{
			nDiffTileX1 = nTileX1;
			nDiffTileY1 = nTileY1;
			nDiffTileX2 = nTileX2;
			nDiffTileY2 = nTileY2;
		}
		else
		{
			nDiffTileX1 = min( nDiffTileX1, nTileX1 );
			nDiffTileY1 = min( nDiffTileY1, nTileY1 );
			nDiffTileX2 = max( nDiffTileX2, nTileX2 );
			nDiffTileY2 = max( nDiffTileY2, nTileY2 );
		}
	}

	for ( int g = nTileY1; g <= nTileY2; ++g )
	{
		for ( int i = nTileX1; i <= nTileX2; ++i )
		{
			const int nIndX = i - nX1;
			const int nIndY = g - nY1;
			if ( terraBrushUpdate == NTerraBrush::TERRA_BRUSH_OVERRIDE )
			{
				if ( ( mask[nIndY][nIndX] < tileTerraMasks.size() ) && ( mask[nIndY][nIndX] >= 0 ) )
				{
					tileTerraMapDiffs[g][i] += mask[nIndY][nIndX] - terrainInfo.tileTerraMap[g][i];
					terrainInfo.tileTerraMap[g][i] = mask[nIndY][nIndX];
				}
			}
			else
			{
				if ( terraBrushUpdate == NTerraBrush::TERRA_BRUSH_ADD )
					terrainInfo.tileTerraMap[g][i] += mask[nIndY][nIndX];
				else
					terrainInfo.tileTerraMap[g][i] -= mask[nIndY][nIndX];
			}
		}
	}

	UpdateTileMasks( nTileX1, nTileY1, nTileX2, nTileY2, true );

	if ( vTexModMin.x < 0 ) // first initialization
	{
		vTexModMin.x = min( nTileX1, nTileX2 );
		vTexModMin.y = min( nTileY1, nTileY2 );
		vTexModMax.x = max( nTileX1, nTileX2 );
		vTexModMax.y = max( nTileY1, nTileY2 );
	}
	else
	{
		vTexModMin.x = min( vTexModMin.x, min( nTileX1, nTileX2 ) );
		vTexModMin.y = min( vTexModMin.y, min( nTileY1, nTileY2 ) );
		vTexModMax.x = max( vTexModMax.x, max( nTileX1, nTileX2 ) );
		vTexModMax.y = max( vTexModMax.y, max( nTileY1, nTileY2 ) );
	}

	vTexModMin.x = min( vTexModMin.x, terrainInfo.tiles.GetSizeX() - 1 );
	vTexModMin.y = min( vTexModMin.y, terrainInfo.tiles.GetSizeY() - 1 );
	vTexModMax.x = min( vTexModMax.x, terrainInfo.tiles.GetSizeX() - 1 );
	vTexModMax.y = min( vTexModMax.y, terrainInfo.tiles.GetSizeY() - 1 );

	const int nPatchX1 = Clamp( ( min( nTileX1, nTileX2 ) >> DEF_PATCH_SIZE_BITS ) - DEF_TILES_BLUR_FILTER_HX * 2, 0, pDesc->nNumPatchesX - 1 );
	const int nPatchY1 = Clamp( ( min( nTileY1, nTileY2 ) >> DEF_PATCH_SIZE_BITS ) - DEF_TILES_BLUR_FILTER_HY * 2, 0, pDesc->nNumPatchesY - 1 );
	const int nPatchX2 = Clamp( ( max( nTileX1, nTileX2 ) >> DEF_PATCH_SIZE_BITS ) + DEF_TILES_BLUR_FILTER_HX * 2, 0, pDesc->nNumPatchesX - 1 );
	const int nPatchY2 = Clamp( ( max( nTileY1, nTileY2 ) >> DEF_PATCH_SIZE_BITS ) + DEF_TILES_BLUR_FILTER_HY * 2, 0, pDesc->nNumPatchesY - 1 );

	for ( int g = nPatchY1; g <= nPatchY2; ++g )
	{
		for ( int i = nPatchX1; i <= nPatchX2; ++i )
		{
			texModCheck[g][i] = 1;
		}
	}

	//VTPause();
}

void CTerraGen::GetTileTypeUpdateDifferences( float *pOffsX, float *pOffsY, CArray2D<BYTE> *pDiffs )
{
	if ( nDiffTileX1 != -1 )
	{
		// check for zero rows and columns
		bool bFlag;
		while ( nDiffTileX1 <= nDiffTileX2 )
		{
			bFlag = true;
			for ( int g = nDiffTileY1; g <= nDiffTileY2; ++g )
			{
				if ( tileTerraMapDiffs[g][nDiffTileX1] != 0 )
				{
					bFlag = false;
					break;
				}
			}
			if ( bFlag )
				++nDiffTileX1;
			else
				break;
		}

		while ( nDiffTileX2 >= nDiffTileX1 )
		{
			bFlag = true;
			for ( int g = nDiffTileY1; g <= nDiffTileY2; ++g )
			{
				if ( tileTerraMapDiffs[g][nDiffTileX2] != 0 )
				{
					bFlag = false;
					break;
				}
			}
			if ( bFlag )
				--nDiffTileX2;
			else
				break;
		}

		while ( nDiffTileY1 <= nDiffTileY2 )
		{
			bFlag = true;
			for ( int i = nDiffTileX1; i <= nDiffTileX2; ++i )
			{
				if ( tileTerraMapDiffs[nDiffTileY1][i] != 0 )
				{
					bFlag = false;
					break;
				}
			}
			if ( bFlag )
				++nDiffTileY1;
			else
				break;
		}

		while ( nDiffTileY2 >= nDiffTileY1 )
		{
			bFlag = true;
			for ( int i = nDiffTileX1; i <= nDiffTileX2; ++i )
			{
				if ( tileTerraMapDiffs[nDiffTileY2][i] != 0 )
				{
					bFlag = false;
					break;
				}
			}
			if ( bFlag )
				--nDiffTileY2;
			else
				break;
		}

		if ( ( nDiffTileX1 <= nDiffTileX2 ) && ( nDiffTileY1 <= nDiffTileY2 ) )
		{
			*pOffsX = Vis2AI( (float)nDiffTileX1 * DEF_TILE_SIZE );
			*pOffsY = Vis2AI( (float)nDiffTileY1 * DEF_TILE_SIZE );
			pDiffs->SetSizes( nDiffTileX2 - nDiffTileX1 + 1, nDiffTileY2 - nDiffTileY1 + 1 );
			for ( int g = 0; g < pDiffs->GetSizeY(); ++g )
			{
				for ( int i = 0; i < pDiffs->GetSizeX(); ++i )
				{
					(*pDiffs)[g][i] = tileTerraMapDiffs[g + nDiffTileY1][i + nDiffTileX1];
					tileTerraMapDiffs[g + nDiffTileY1][i + nDiffTileX1] = 0;
				}
			}
			nDiffTileX1 = nDiffTileY1 = nDiffTileX2 = nDiffTileY2 = -1;
			return;
		}
	}

	pDiffs->SetSizes( 0, 0 );
	*pOffsX = *pOffsY = 0;
	nDiffTileX1 = nDiffTileY1 = nDiffTileX2 = nDiffTileY2 = -1;
}

void CTerraGen::FinalizeTexModifying()
{
	for ( int g = 0; g < texModCheck.GetSizeY(); ++g )
	{
		for ( int i = 0; i < texModCheck.GetSizeX(); ++i )
		{
			if ( texModCheck[g][i] )
			{
				UpdateGfxInfo( i, g );
				texModCheck[g][i] = 0;
			}
		}
	}
}

void CTerraGen::GetAreaTileTypes( CArray2D<BYTE> *pAreaTypes, const int nX1, const int nY1, const int nX2, const int nY2 )
{
	const int nTileX1 = max( min(nX1, nX2), 0 );
	const int nTileY1 = max( min(nY1, nY2), 0 );
	const int nTileX2 = min( max(nX1, nX2), terrainInfo.tileTerraMap.GetSizeX() - 1 );
	const int nTileY2 = min( max(nY1, nY2), terrainInfo.tileTerraMap.GetSizeY() - 1 );
	if ( ( nTileX1 <= nTileX2 ) && ( nTileY1 <= nTileY2 ) )
	{
		pAreaTypes->SetSizes( nTileX2 - nTileX1 + 1, nTileY2 - nTileY1 + 1 );
		for ( int g = 0; g < pAreaTypes->GetSizeY(); ++g )
		{
			for ( int i = 0; i < pAreaTypes->GetSizeX(); ++i )
			{
				(*pAreaTypes)[g][i] = terrainInfo.tileTerraMap[g + nTileY1][i + nTileX1];
			}
		}
	}
	else
	{
		pAreaTypes->SetSizes( 0, 0 );
	}
}

START_REGISTER( TerrainBluring )
	REGISTER_VAR_EX( "terrain_blur_radius", NGlobal::VarIntHandler, &s_nTerrainBlurRaduis, 1, STORAGE_USER );
	REGISTER_VAR_EX( "terrain_blur_treshhold", NGlobal::VarIntHandler, &s_nTerrainBlurThreshHold, 0, STORAGE_USER );
FINISH_REGISTER



