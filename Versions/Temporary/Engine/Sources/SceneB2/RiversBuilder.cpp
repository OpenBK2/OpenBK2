#include "StdAfx.h"

#include "../Misc/Win32Random.h"
#include "GenTerrain.h"
#include "TerraHeight.h"
#include "VSOConsts.h"
#include "Scene.h"

#define DEF_RIVER_SAMPLES_PER_PATCH 4
//
#define DEF_RIVER_HIGH_BORDER_RAND 0.5f
#define DEF_RIVER_WATER_LEVEL 0.1f
#define DEF_BOTTOM_TEX 5.0f
#define DEF_BOTTOM_INV_TEX (1.0f / DEF_BOTTOM_TEX)
#define DEF_BOTTOM_SAMPLES_NUM 8
#define DEF_BOTTOM_SAMPLES_COEFF ( 1.0f / ( DEF_BOTTOM_SAMPLES_NUM - 1 ) )
#define DEF_WATER_SAMPLES_NUM 6
//
#define DEF_WATER2_SAMPLES_NUM 4
//
#define DEF_WATER_TEX_WORLD_SCALING 0.2f
//#define DEF_WATER_LAYER_HEIGHT 2.0f
//
#define DEF_RIVER_RIDGE_WIDTH ( DEF_TILE_SIZE * 4 )
#define DEF_RIVER_RIDGE_NULL ( DEF_TILE_SIZE * FP_SQRT_2 / DEF_RIVER_RIDGE_WIDTH )
#define DEF_RIVER_RIDGE_COEFF ( 1.0f / ( 1.0f - DEF_RIVER_RIDGE_NULL ) )
//
#define DEF_DEPTH_INTERPOLATE_LEN 5
#define DEF_DEPTH_INTERPOLATE_COEFF ( 1.0f / ( DEF_DEPTH_INTERPOLATE_LEN - 1 ) )
//
//#define DEF_WATER_TEXTURE_SCALE ( 1.0f / 4.0f )
//
#define DEF_WATER_EXPAND 0.25f
//#define DEF_WATER_SAMPLES_CALC_COEFF /*0.25f*/0.1f
#define DEF_WATER2_TO_WATER_COEFF ( 2.0f / 3.0f )
#define DEF_WATER_ALPHA_INIT 1
#define DEF_WATER_ALPHA_FROM_HEIGHT_COEFF /*2.5f*/0.0f
//
#define DEF_MAX_REFL_ALPHA_LEN ( DEF_TILE_SIZE * 3 )
#define DEF_INV_MAX_REFL_ALPHA_LEN ( 1.0f / DEF_MAX_REFL_ALPHA_LEN )
#define DEF_MIN_REFL_ALPHA_VAL 128
#define DEF_REFL_ALPHA_RANGE ( 255 - DEF_MIN_REFL_ALPHA_VAL )

inline BYTE CalculateAlphaValue( const int nWaterSamplesNum, const int nAlphaCellsNumL, const int nAlphaCellsNumR,
																 const float fAlphaCellsCoeffL, const float fAlphaCellsCoeffR, const float x,
																 const float fOpacity)
{
	const float fAlphaL = x < nAlphaCellsNumL ? ( (float)x * fAlphaCellsCoeffL ) : 1.0f;
	const float fAlphaR = x > ( nWaterSamplesNum - 1 - nAlphaCellsNumR ) ? ( (float)( nWaterSamplesNum - 1 - x ) * fAlphaCellsCoeffR ) : 1.0f;
	return Clamp( Float2Int( min( fAlphaL, fAlphaR ) * fOpacity * 255.0f	), 0, 255 );
}

static void UpdateGfxHeights( SRiverGFXInfo *pGfxInfo, const STerrainInfo::SRiver *pRiver, const float fBottomHeight, const float fWaterHeight,
															const int nCurPatch, const int nPatchInd, int &nBottomInd, int &nWaterInd, int &nWater2Ind,
															const int nWaterSamplesNum, const int nWater2SamplesNum,
															const int nAlphaCellsNumL, const int nAlphaCellsNumR, const float fOpacity )
{
	const float fLayer0Opacity = pRiver->pDesc->waterLayers[0].fCenterOpacity;
	const float fLayer1Opacity = pRiver->pDesc->waterLayers[1].fCenterOpacity;

	if ( pGfxInfo )
	{
		int nAlpha2CellsNumL = nAlphaCellsNumL * DEF_WATER2_TO_WATER_COEFF;
		if ( nAlphaCellsNumL >= 1 )
			nAlpha2CellsNumL = max( nAlpha2CellsNumL, 1 );
		int nAlpha2CellsNumR = nAlphaCellsNumR * DEF_WATER2_TO_WATER_COEFF;
		if ( nAlphaCellsNumR >= 1 )
			nAlpha2CellsNumR = max( nAlpha2CellsNumR, 1 );
		const float fAlphaCellsCoeffL = 1.0f / nAlphaCellsNumL;
		const float fAlphaCellsCoeffR = 1.0f / nAlphaCellsNumR;
		const float fAlpha2CellsCoeffL = 1.0f / nAlpha2CellsNumL;
		const float fAlpha2CellsCoeffR = 1.0f / nAlpha2CellsNumR;

		for ( int g = 0; g < DEF_BOTTOM_SAMPLES_NUM; ++g )
		{
			float fAddHeight = pGfxInfo->bottomPatches[nCurPatch].vertices[nBottomInd + g].tex.x;
			fAddHeight = fAddHeight * ( 1 - fAddHeight ) / 10.0f;
			pGfxInfo->bottomPatches[nCurPatch].vertices[nBottomInd + g].pos.z = fAddHeight + fBottomHeight;
		}
		for ( int g = 0; g < nWaterSamplesNum; ++g )
		{
			pGfxInfo->waterPatches[nCurPatch].vertices[nWaterInd + g].pos.z = fWaterHeight;
			pGfxInfo->waterPatches[nCurPatch].vertices[nWaterInd + g].normal.w =
				CalculateAlphaValue( nWaterSamplesNum, nAlphaCellsNumL, nAlphaCellsNumR, fAlphaCellsCoeffL, fAlphaCellsCoeffR, g, fOpacity * fLayer0Opacity );
		}
		for ( int g = 0; g < nWater2SamplesNum; ++g )
		{
			pGfxInfo->water2Patches[nCurPatch].vertices[nWater2Ind + g].pos.z = fWaterHeight;
			pGfxInfo->water2Patches[nCurPatch].vertices[nWater2Ind + g].normal.w =
				CalculateAlphaValue( nWater2SamplesNum, nAlpha2CellsNumL, nAlpha2CellsNumR, fAlpha2CellsCoeffL, fAlpha2CellsCoeffR, g, fOpacity * fLayer1Opacity );
		}

		if ( (nPatchInd == DEF_RIVER_SAMPLES_PER_PATCH) && (nCurPatch < (pGfxInfo->bottomPatches.size() - 1)) )
		{
			for ( int g = 0; g < DEF_BOTTOM_SAMPLES_NUM; ++g )
			{
				float fAddHeight = pGfxInfo->bottomPatches[nCurPatch].vertices[nBottomInd + g].tex.x;
				fAddHeight = fAddHeight * ( 1 - fAddHeight ) / 10.0f;
				pGfxInfo->bottomPatches[nCurPatch + 1].vertices[g].pos.z = fAddHeight + fBottomHeight;
			}
			for ( int g = 0; g < nWaterSamplesNum; ++g )
			{
				pGfxInfo->waterPatches[nCurPatch + 1].vertices[g].pos.z = fWaterHeight;
				pGfxInfo->waterPatches[nCurPatch + 1].vertices[g].normal.w =
					CalculateAlphaValue( nWaterSamplesNum, nAlphaCellsNumL, nAlphaCellsNumR, fAlphaCellsCoeffL, fAlphaCellsCoeffR, g, fOpacity * fLayer0Opacity );
			}
			for ( int g = 0; g < nWater2SamplesNum; ++g )
			{
				pGfxInfo->water2Patches[nCurPatch + 1].vertices[g].pos.z = fWaterHeight;
				pGfxInfo->water2Patches[nCurPatch + 1].vertices[g].normal.w =
					CalculateAlphaValue( nWater2SamplesNum, nAlpha2CellsNumL, nAlpha2CellsNumR, fAlpha2CellsCoeffL, fAlpha2CellsCoeffR, g, fOpacity * fLayer1Opacity );
			}
		}

		nBottomInd += DEF_BOTTOM_SAMPLES_NUM;
		nWaterInd += nWaterSamplesNum;
		nWater2Ind += nWater2SamplesNum;
	}
}

void CTerraGen::AddRiver( const NDb::SVSOInstance *pInstance )
{
	if ( AddRiver(pInstance, GetVSOSeed(pInstance)) )
	{
		RiverManipulator( FindRiverInfo(pInstance->nVSOID), false );
		if ( pAIObserver )
			PutRiverToAI( pInstance );
	}
}

inline void InitBottomVertex( NGScene::SVertex &vert )
{
	CalcCompactVector( &(vert.texU), V3_AXIS_X );
	CalcCompactVector( &(vert.texV), V3_AXIS_Y );
	CalcCompactVector( &(vert.normal), V3_AXIS_Z );
}

inline void InitWaterVertex( NGScene::SVertex &vert )
{
	CalcCompactVector( &(vert.normal), V3_AXIS_Z );
	vert.texU.dw = 0xffffffff;
	vert.texV.dw = 0;
}

bool CTerraGen::AddRiver( const NDb::SVSOInstance *pInstance, const int nRandSeed )
{
	NI_VERIFY( pInstance, "CTerraGen::AddRiver - Invalid river instance", return false )

	// if such river already was builded, than skip it building again
	for ( list<STerrainInfo::SRiver>::const_iterator it = terrainInfo.rivers.begin(); it != terrainInfo.rivers.end(); ++it )
	{
		if ( it->nID == pInstance->nVSOID )
			return false;
	}

	if ( pInstance->points.size() < 2 )
		return false;

	STerrainInfo::SRiver curRiverInfo;
	NWin32Random::Seed( nRandSeed );
	curRiverInfo.nRandSeed = NWin32Random::GetSeed();
	//DebugTrace( StrFmt("Add river with RandSeed=%d\n", curRiverInfo.nRandSeed) );
	curRiverInfo.nID = pInstance->nVSOID;
	curRiverInfo.pDesc = static_cast<const NDb::SRiverDesc *>(pInstance->pDescriptor.GetPtr());

	static vector<NDb::SVSOPoint> sampPoints( 256 );
	sampPoints = pInstance->points;

	for ( vector<NDb::SVSOPoint>::iterator it = sampPoints.begin(); it != sampPoints.end(); ++it )
	{
		const float fDist = NWin32Random::RandomCheck( -curRiverInfo.pDesc->fBorderRand * 0.5f, curRiverInfo.pDesc->fBorderRand * 0.5f );
		AI2Vis( &(it->vPos.x) );
		AI2Vis( &(it->vPos.y) );
		AI2Vis( &(it->vPos.z) );
		AI2Vis( &(it->fWidth) );
		it->vPos.x += it->vNorm.x * fDist;
		it->vPos.y += it->vNorm.y * fDist;
	}

	curRiverInfo.vBBMin.Set( FP_MAX_VALUE, FP_MAX_VALUE );
	curRiverInfo.vBBMax.Set( -FP_MAX_VALUE, -FP_MAX_VALUE );
	curRiverInfo.samples.resize( sampPoints.size() * 2 );

	// creat ridge
	{
		curRiverInfo.ridgeL.resize( sampPoints.size() * 2 );
		curRiverInfo.ridgeR.resize( sampPoints.size() * 2 );
		int nInd = 0;
		for ( vector<NDb::SVSOPoint>::const_iterator it = sampPoints.begin(); it != sampPoints.end(); ++it, ++nInd )
		{
			const int nInvInd = curRiverInfo.ridgeL.size() - 1 - nInd;
			const float fRidgeHeightL = Clamp( pInstance->points[nInd].fRadius, 0.0f, 1.0f ) * DEF_RIVER_DEPTH;
			const float fRidgeHeightR = Clamp( pInstance->points[nInd].fReserved, 0.0f, 1.0f ) * DEF_RIVER_DEPTH;
			curRiverInfo.ridgeL[nInd].Set( it->vPos - it->vNorm * it->fWidth, 0 );
			curRiverInfo.ridgeL[nInd].z =fRidgeHeightL;
			curRiverInfo.vBBMin.Minimize( CVec2(curRiverInfo.ridgeL[nInd].x, curRiverInfo.ridgeL[nInd].y) );
			curRiverInfo.vBBMax.Maximize( CVec2(curRiverInfo.ridgeL[nInd].x, curRiverInfo.ridgeL[nInd].y) );
			curRiverInfo.ridgeL[nInvInd].Set( it->vPos - it->vNorm * (it->fWidth + DEF_RIVER_RIDGE_WIDTH), 0 );
			curRiverInfo.ridgeL[nInvInd].z = fRidgeHeightL;
			curRiverInfo.vBBMin.Minimize( CVec2(curRiverInfo.ridgeL[nInvInd].x, curRiverInfo.ridgeL[nInvInd].y) );
			curRiverInfo.vBBMax.Maximize( CVec2(curRiverInfo.ridgeL[nInvInd].x, curRiverInfo.ridgeL[nInvInd].y) );
			curRiverInfo.ridgeR[nInd].Set( it->vPos + it->vNorm * it->fWidth, 0 );
			curRiverInfo.ridgeR[nInd].z = fRidgeHeightR;
			curRiverInfo.vBBMin.Minimize( CVec2(curRiverInfo.ridgeR[nInd].x, curRiverInfo.ridgeR[nInd].y) );
			curRiverInfo.vBBMax.Maximize( CVec2(curRiverInfo.ridgeR[nInd].x, curRiverInfo.ridgeR[nInd].y) );
			curRiverInfo.ridgeR[nInvInd].Set( it->vPos + it->vNorm * (it->fWidth + DEF_RIVER_RIDGE_WIDTH), 0 );
			curRiverInfo.ridgeR[nInvInd].z = fRidgeHeightR;
			curRiverInfo.vBBMin.Minimize( CVec2(curRiverInfo.ridgeR[nInvInd].x, curRiverInfo.ridgeR[nInvInd].y) );
			curRiverInfo.vBBMax.Maximize( CVec2(curRiverInfo.ridgeR[nInvInd].x, curRiverInfo.ridgeR[nInvInd].y) );
		}
	}

	// init data for precipice
	curRiverInfo.precVertsL.resize( sampPoints.size() );
	curRiverInfo.precVertsR.resize( sampPoints.size() );
	curRiverInfo.precHeightsL.resize( sampPoints.size() );
	curRiverInfo.precHeightsR.resize( sampPoints.size() );
	curRiverInfo.precNormsL.resize( sampPoints.size() );
	curRiverInfo.precNormsR.resize( sampPoints.size() );

	// find max river samples number
	curRiverInfo.nWaterSamplesNum = DEF_WATER_SAMPLES_NUM;
	if ( !curRiverInfo.pDesc->waterLayers.empty() )
		curRiverInfo.nWaterSamplesNum = curRiverInfo.pDesc->waterLayers[0].nNumCells;

	curRiverInfo.nWater2SamplesNum = max( curRiverInfo.nWaterSamplesNum * DEF_WATER2_TO_WATER_COEFF, 1 );
	if ( curRiverInfo.pDesc->waterLayers.size() > 1 )
		curRiverInfo.nWater2SamplesNum = curRiverInfo.pDesc->waterLayers[1].nNumCells;

//	curRiverInfo.nWaterSamplesNum = DEF_WATER_SAMPLES_NUM;
//	for ( int i = 1; i < sampPoints.size(); ++i )
//	{
//		const float fWidth = sampPoints[i].fWidth * 2.0f * ( 1.0f + DEF_WATER_EXPAND );
//		const float fDist = fabs( sampPoints[i].vPos - sampPoints[i - 1].vPos );
//		curRiverInfo.nWaterSamplesNum = max( curRiverInfo.nWaterSamplesNum , Float2Int(fWidth / fDist * DEF_WATER_SAMPLES_CALC_COEFF) );
//	}
//	curRiverInfo.nWater2SamplesNum = max( curRiverInfo.nWaterSamplesNum * DEF_WATER2_TO_WATER_COEFF, 1 );

	// create all data
	float fHeight;
	NDb::SVSOPoint addPoint;
	int nInd = 0;
	//int nCurPatch = 0, nPatchInd = 0;
	for ( vector<NDb::SVSOPoint>::const_iterator it = sampPoints.begin(); it != sampPoints.end(); ++it, ++nInd )
	{
		const int nInvInd = sampPoints.size() - 1 - nInd;
		const float xl = curRiverInfo.ridgeL[nInd].x;//it->vPos.x - it->vNorm.x * it->fWidth;
		const float yl = curRiverInfo.ridgeL[nInd].y;//it->vPos.y - it->vNorm.y * it->fWidth;
		const float xr = curRiverInfo.ridgeR[nInd].x;//it->vPos.x + it->vNorm.x * it->fWidth;
		const float yr = curRiverInfo.ridgeR[nInd].y;//it->vPos.y + it->vNorm.y * it->fWidth;
		const float fTerraHeightL = GetTerraHeight( xl, yl );
		const float fTerraHeightR = GetTerraHeight( xr, yr );
		curRiverInfo.samples[nInd].Set( xl, yl, curRiverInfo.ridgeL[nInd].z/*fTerraHeightL*/, 0 );
		curRiverInfo.samples[curRiverInfo.samples.size() - 1 - nInd].Set( xr, yr, curRiverInfo.ridgeR[nInd].z/*fTerraHeightR*/, 0 );
		curRiverInfo.vBBMin.x = min( curRiverInfo.vBBMin.x, min(xl, xr) );
		curRiverInfo.vBBMin.y = min( curRiverInfo.vBBMin.y, min(yl, yr) );
		curRiverInfo.vBBMax.x = max( curRiverInfo.vBBMax.x, max(xl, xr) );
		curRiverInfo.vBBMax.y = max( curRiverInfo.vBBMax.y, max(yl, yr) );

		// precipice
		//const float fMinHeight = min( fTerraHeightL - DEF_RIVER_DEPTH, fTerraHeightR - DEF_RIVER_DEPTH );
		//const float fPreRealHeight = fMinHeight < fPrevHeight ? fMinHeight : fPrevHeight;
		//const float fRealHeight = max( fPreRealHeight, 0.0f );
		//curRiverInfo.precHeightsL[nInd] = fRealHeight;
		//curRiverInfo.precHeightsR[nInvInd] = fRealHeight;
		curRiverInfo.precHeightsL[nInd] = 0.0f;
		curRiverInfo.precHeightsR[nInvInd] = 0.0f;
		curRiverInfo.precNormsL[nInd] = it->vNorm;
		curRiverInfo.precNormsR[nInvInd] = -it->vNorm;
		GetMaxCragHeightEx( CVec2(xl, yl), &fHeight );
		curRiverInfo.precVertsL[nInd].Set( xl, yl, fTerraHeightL + fHeight );
		GetMaxCragHeightEx( CVec2(xr, yr), &fHeight );
		curRiverInfo.precVertsR[nInvInd].Set( xr, yr, fTerraHeightR + fHeight );
		//fPrevHeight = fRealHeight;
	}

	curRiverInfo.vSampMin.x = Clamp( int(curRiverInfo.vBBMin.x * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
	curRiverInfo.vSampMin.y = Clamp( int(curRiverInfo.vBBMin.y * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );
	curRiverInfo.vSampMax.x = Clamp( int(curRiverInfo.vBBMax.x * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeX() - 1 );
	curRiverInfo.vSampMax.y = Clamp( int(curRiverInfo.vBBMax.y * DEF_INV_TILE_SIZE), 0, terrainInfo.tiles.GetSizeY() - 1 );

	terrainInfo.rivers.push_back( curRiverInfo );

	CreateRiverGfx( &(terrainInfo.rivers.back()), pInstance, false );

	return true;
}

void CTerraGen::UpdateRiverHeights( STerrainInfo::SRiver *pRiver, SRiverGFXInfo *pGfxInfo, const NDb::SVSOInstance *pInstance )
{
	// update river heights
	float fPrevHeight = FP_MAX_VALUE;
	int nBottomInd = 0, nWaterInd = 0, nWater2Ind = 0;
	int nPatchInd = 0, nCurPatch = 0;

	for ( int i = 0; i < pRiver->precVertsL.size(); ++i )
	{
		const int nInvInd = pRiver->precVertsL.size() - 1 - i;
		const float fTerraHeightL = GetTerraHeightNative( pRiver->precVertsL[i].x, pRiver->precVertsL[i].y );
		const float fTerraHeightR = GetTerraHeightNative( pRiver->precVertsR[nInvInd].x, pRiver->precVertsR[nInvInd].y );
		const float fBottomPreHeight = min( min( fTerraHeightL, fTerraHeightR ) - DEF_RIVER_DEPTH, fPrevHeight );
		const float fBottomHeight = max( fBottomPreHeight, 0.0f/*0.05f*/ );
		pRiver->precHeightsL[i] = fBottomHeight;
		pRiver->precHeightsR[nInvInd] = fBottomHeight;

		const float fWaterHeight = ( ( (fBottomPreHeight + DEF_RIVER_WATER_LEVEL) < (fBottomHeight + 0.04f) ) ? ( fBottomHeight + 0.04f ) : ( fBottomPreHeight + DEF_RIVER_WATER_LEVEL ) ) + 0.5f;
		const float fInvRiverWidth = 1.0f / AI2Vis( pInstance->points[i].fWidth );
		const int nAlphaCellsNumL = DEF_WATER_ALPHA_INIT + Float2Int( fabs(fTerraHeightL - fWaterHeight) * fInvRiverWidth * DEF_WATER_ALPHA_FROM_HEIGHT_COEFF );
		const int nAlphaCellsNumR = DEF_WATER_ALPHA_INIT + Float2Int( fabs(fTerraHeightR - fWaterHeight) * fInvRiverWidth * DEF_WATER_ALPHA_FROM_HEIGHT_COEFF );

		//const CVec2 vPoint( pRiver->precVertsL[i].x, pRiver->precVertsL[i].y );
		//const float fOpacity = IsPointOnMap( vPoint ) ? pInstance->points[i].fOpacity : 0.0f;
		UpdateGfxHeights( pGfxInfo, pRiver, fBottomHeight, fWaterHeight, nCurPatch, nPatchInd, nBottomInd, nWaterInd, nWater2Ind,
											pRiver->nWaterSamplesNum, pRiver->nWater2SamplesNum, nAlphaCellsNumL, nAlphaCellsNumR, pInstance->points[i].fOpacity );

		if ( ++nPatchInd > DEF_RIVER_SAMPLES_PER_PATCH )
		{
			nPatchInd -= DEF_RIVER_SAMPLES_PER_PATCH;
			++nCurPatch;
			nBottomInd = DEF_BOTTOM_SAMPLES_NUM;
			nWaterInd = pRiver->nWaterSamplesNum;
			nWater2Ind = pRiver->nWater2SamplesNum;
		}
		fPrevHeight = fBottomPreHeight;
	}

	for ( vector<NMeshData::SMeshData>::iterator itPatch = pGfxInfo->waterPatches.begin(); itPatch != pGfxInfo->waterPatches.end(); ++itPatch )
	{
		for ( vector<NGScene::SVertex>::iterator itVert = itPatch->vertices.begin(); itVert != itPatch->vertices.end(); ++itVert )
		{
			UpdateAlphaByPosition( itVert );
		}
		RemoveInvisibleTriangles( &(*itPatch) );
	}
	for ( vector<NMeshData::SMeshData>::iterator itPatch = pGfxInfo->water2Patches.begin(); itPatch != pGfxInfo->water2Patches.end(); ++itPatch )
	{
		for ( vector<NGScene::SVertex>::iterator itVert = itPatch->vertices.begin(); itVert != itPatch->vertices.end(); ++itVert )
		{
			UpdateAlphaByPosition( itVert );
		}
		RemoveInvisibleTriangles( &(*itPatch) );
	}
	for ( vector<NMeshData::SMeshData>::iterator itPatch = pGfxInfo->bottomPatches.begin(); itPatch != pGfxInfo->bottomPatches.end(); ++itPatch )
	{
		for ( vector<NGScene::SVertex>::iterator itVert = itPatch->vertices.begin(); itVert != itPatch->vertices.end(); ++itVert )
		{
			UpdateAlphaByPosition( itVert );
		}
		RemoveInvisibleTriangles( &(*itPatch) );
	}	
}

//void CTerraGen::ClampRiverGfxByMap( SRiverGFXInfo *pGfxInfo )
//{
//	for ( vector<NMeshData::SMeshData>::iterator itPatch = pGfxInfo->waterPatches.begin(); itPatch != pGfxInfo->waterPatches.end(); ++itPatch )
//	{
//		bool bIsOnMap = false;
//		for ( vector<NGScene::SVertex>::iterator itVert = itPatch->vertices.begin(); itVert != itPatch->vertices.end(); ++itVert )
//		{
//			if ( IsPointOnMap(GetVec2(itVert->pos)) )
//			{
//				bIsOnMap = true;
//				break;
//			}
//		}
//		if ( !bIsOnMap )
//		{
//			pGfxInfo->waterPatches.erase( itPatch );
//		}
//	}
//	for ( vector<NMeshData::SMeshData>::iterator itPatch = pGfxInfo->water2Patches.begin(); itPatch != pGfxInfo->water2Patches.end(); ++itPatch )
//	{
//		bool bIsOnMap = false;
//		for ( vector<NGScene::SVertex>::iterator itVert = itPatch->vertices.begin(); itVert != itPatch->vertices.end(); ++itVert )
//		{
//			if ( IsPointOnMap(GetVec2(itVert->pos)) )
//			{
//				bIsOnMap = true;
//				break;
//			}
//		}
//		if ( !bIsOnMap )
//		{
//			pGfxInfo->water2Patches.erase( itPatch );
//		}
//	}
//	for ( vector<NMeshData::SMeshData>::iterator itPatch = pGfxInfo->bottomPatches.begin(); itPatch != pGfxInfo->bottomPatches.end(); ++itPatch )
//	{
//		bool bIsOnMap = false;
//		for ( vector<NGScene::SVertex>::iterator itVert = itPatch->vertices.begin(); itVert != itPatch->vertices.end(); ++itVert )
//		{
//			if ( IsPointOnMap(GetVec2(itVert->pos)) )
//			{
//				bIsOnMap = true;
//				break;
//			}
//		}
//		if ( !bIsOnMap )
//		{
//			pGfxInfo->bottomPatches.erase( itPatch );
//		}
//	}
//}

void CTerraGen::CreateRiverGfx( STerrainInfo::SRiver *pRiver, const NDb::SVSOInstance *pInstance, const bool bNeedUpdateHeights )
{
	NI_ASSERT( pRiver->precVertsL.size() == pInstance->points.size(), "River's info is not corresponded to VSOInstance" );
	if ( pRiver->precVertsL.size() != pInstance->points.size() )
		return;

	NGScene::SVertex bottomVertL, bottomVertR, bottomVert;
	InitBottomVertex( bottomVertL );
	InitBottomVertex( bottomVertR );
	InitBottomVertex( bottomVert );

	NGScene::SVertex waterVertL, waterVertR, waterVert;
	InitWaterVertex( waterVertL );
	InitWaterVertex( waterVertR );
	InitWaterVertex( waterVert );

	const int nNumPatches = ( (pRiver->precVertsL.size() - 1) % DEF_RIVER_SAMPLES_PER_PATCH ) ?
													( (pRiver->precVertsL.size() - 1) / DEF_RIVER_SAMPLES_PER_PATCH + 1 ) :
													( (pRiver->precVertsL.size() - 1) / DEF_RIVER_SAMPLES_PER_PATCH );

	const float fWaterSamplesCoeff = 1.0f / ( pRiver->nWaterSamplesNum - 1 );
	const float fWater2SamplesCoeff = 1.0f / ( pRiver->nWater2SamplesNum - 1 );

	SRiverGFXInfo gfxInfo;
	gfxInfo.nID = pRiver->nID;
	gfxInfo.pDesc = pRiver->pDesc;
	gfxInfo.bottomPatches.resize( nNumPatches );
	gfxInfo.waterPatches.resize( nNumPatches );
	gfxInfo.water2Patches.resize( nNumPatches );
	for ( int i = 0; i < nNumPatches; ++i )
	{
		gfxInfo.bottomPatches[i].vertices.reserve( (DEF_RIVER_SAMPLES_PER_PATCH + 1) * DEF_BOTTOM_SAMPLES_NUM );
		gfxInfo.bottomPatches[i].vertices.resize( 0 );
		gfxInfo.bottomPatches[i].triangles.reserve( DEF_RIVER_SAMPLES_PER_PATCH * (DEF_BOTTOM_SAMPLES_NUM - 1) * 2 );
		gfxInfo.bottomPatches[i].triangles.resize( 0 );
		gfxInfo.waterPatches[i].vertices.reserve( (DEF_RIVER_SAMPLES_PER_PATCH + 1) * pRiver->nWaterSamplesNum );
		gfxInfo.waterPatches[i].vertices.resize( 0 );
		gfxInfo.waterPatches[i].triangles.reserve( DEF_RIVER_SAMPLES_PER_PATCH * (pRiver->nWaterSamplesNum - 1) * 2 );
		gfxInfo.waterPatches[i].triangles.resize( 0 );
		gfxInfo.water2Patches[i].vertices.reserve( (DEF_RIVER_SAMPLES_PER_PATCH + 1) * pRiver->nWater2SamplesNum );
		gfxInfo.water2Patches[i].vertices.resize( 0 );
		gfxInfo.water2Patches[i].triangles.reserve( DEF_RIVER_SAMPLES_PER_PATCH * (pRiver->nWater2SamplesNum - 1) * 2 );
		gfxInfo.water2Patches[i].triangles.resize( 0 );
	}

	float fBottomTexY = 0.0f;
	float fWaterTexY = 0.0f;
	float fWater2TexY = 0.0f;
	int nCurPatch = 0, nPatchInd = 0;
	for ( int i = 0; i < pInstance->points.size(); ++i )
	{
		const int nInvInd = pInstance->points.size() - 1 - i;
		const float xl = pRiver->ridgeL[i].x;
		const float yl = pRiver->ridgeL[i].y;
		const float xr = pRiver->ridgeR[i].x;
		const float yr = pRiver->ridgeR[i].y;

		const float fCurWidth = AI2Vis( pInstance->points[i].fWidth );

		// bottom
		bottomVertL.pos.Set( xl, yl, 0.0f/*fRealHeight*/ );
		bottomVertR.pos.Set( xr, yr, 0.0f/*fRealHeight*/ );

		const CVec2 vMid1 = ( i > 0 ) ? ( (pRiver->ridgeL[i - 1].GetVec2() + pRiver->ridgeR[i - 1].GetVec2()) * 0.5f ) :
																		( (pRiver->ridgeL[i].GetVec2() + pRiver->ridgeR[i].GetVec2()) * 0.5f );
		const CVec2 vMid2 = ( pRiver->ridgeL[i].GetVec2() + pRiver->ridgeR[i].GetVec2()) * 0.5f;

		const float fPrevDist = fabs( vMid2 - vMid1 );
		fBottomTexY += fPrevDist / ( fCurWidth * 2.0f );
		bottomVert.tex.y = fBottomTexY;
		for ( int k = 0; k < DEF_BOTTOM_SAMPLES_NUM; ++k )
		{
			bottomVert.tex.x = DEF_BOTTOM_SAMPLES_COEFF * k;
			bottomVert.pos = bottomVertL.pos + ( bottomVertR.pos - bottomVertL.pos ) * bottomVert.tex.x;
			bottomVert.pos.z = bottomVert.tex.x * ( 1.0f - bottomVert.tex.x )*2;

			//bottomVert.normal.w = Clamp<int>( (fTest - fTest*fTest)*4*255, 0, 255 );//IsPointOnMap( GetVec2(bottomVert.pos) ) ? 1.0f : 0.0f;
			gfxInfo.bottomPatches[nCurPatch].vertices.push_back( bottomVert );
			if ( (nPatchInd == DEF_RIVER_SAMPLES_PER_PATCH) && (nCurPatch < (nNumPatches - 1)) )
				gfxInfo.bottomPatches[nCurPatch + 1].vertices.push_back( bottomVert );
		}

		// water
		{
			waterVertL.pos.Set( xl - pRiver->precNormsL[i].x * fCurWidth * DEF_WATER_EXPAND, yl - pRiver->precNormsL[i].y * fCurWidth * DEF_WATER_EXPAND, 0.0f );
			waterVertR.pos.Set( xr - pRiver->precNormsR[nInvInd].x * fCurWidth * DEF_WATER_EXPAND, yr - pRiver->precNormsR[nInvInd].y * fCurWidth * DEF_WATER_EXPAND, 0.0f );
			const float fMaxTexX = 1;//Clamp( fCurWidth * (1.0f + DEF_WATER_EXPAND) * DEF_WATER_TEXTURE_SCALE, 0.0f, 15.0f );
			const float fMinTexX = 0;//-fMaxTexX;
			pRiver->pDesc->waterLayers[0].fTilingStep;
			fWaterTexY += pRiver->pDesc->waterLayers[0].fTilingStep;//fPrevDist / ( fCurWidth * (1.0f + DEF_WATER_EXPAND) * 2.0f * DEF_WATER_TEXTURE_SCALE );
			waterVert.tex.y = fWaterTexY;
			for ( int k = 0; k < pRiver->nWaterSamplesNum; ++k )
			{
				waterVert.tex.x = fWaterSamplesCoeff * k;
				waterVert.pos = waterVertL.pos + ( waterVertR.pos - waterVertL.pos ) * waterVert.tex.x;
				waterVert.tex.x = fMinTexX + ( fMaxTexX - fMinTexX ) * waterVert.tex.x;
				if ( (k != 0) && (k != (pRiver->nWaterSamplesNum - 1)) )
					waterVert.pos += pRiver->precNormsL[i] * NWin32Random::RandomCheck( -pRiver->pDesc->waterLayers[0].fDisturbance, pRiver->pDesc->waterLayers[0].fDisturbance );
				/*if ( ( k == 0 ) || ( k == ( curRiverInfo.nWaterSamplesNum - 1 ) ) )
				waterVert.normal.w = 0;
				else
				waterVert.normal.w = 0xff;
				waterVert.normal.w = Clamp( int( (float)waterVert.normal.w * it->fOpacity ), 0, 255 );*/
				waterVert.normal.w = 0;// IsPointOnMap( GetVec2(waterVert.pos) ) ? Clamp( Float2Int(pInstance->points[i].fOpacity * 255.0f), 0, 255 ) : 0.0f;
				gfxInfo.waterPatches[nCurPatch].vertices.push_back( waterVert );
				if ( (nPatchInd == DEF_RIVER_SAMPLES_PER_PATCH) && (nCurPatch < (nNumPatches - 1)) )
					gfxInfo.waterPatches[nCurPatch + 1].vertices.push_back( waterVert );
			}
		}

		// water2
		{
			const float fWater2InnerExpand = 0.25f;
			waterVertL.pos.Set( xl + pRiver->precNormsL[i].x * fCurWidth * fWater2InnerExpand, yl + pRiver->precNormsL[i].y * fCurWidth * fWater2InnerExpand, 0.0f );
			waterVertR.pos.Set( xr + pRiver->precNormsR[nInvInd].x * fCurWidth * fWater2InnerExpand, yr + pRiver->precNormsR[nInvInd].y * fCurWidth * fWater2InnerExpand, 0.0f );
			const float fMaxTexX = 1;//Clamp( fCurWidth * (1.0f - fWater2InnerExpand) * DEF_WATER_TEXTURE_SCALE, 0.0f, 15.0f );
			const float fMinTexX = 0;//-fMaxTexX;
			fWater2TexY += pRiver->pDesc->waterLayers[1].fTilingStep;//fPrevDist / ( fCurWidth * (1.0f - fWater2InnerExpand) * 2.0f * DEF_WATER_TEXTURE_SCALE );
			waterVert.tex.y = fWater2TexY;
			for ( int k = 0; k < pRiver->nWater2SamplesNum; ++k )
			{
				waterVert.tex.x = fWater2SamplesCoeff * k;
				waterVert.pos = waterVertL.pos + ( waterVertR.pos - waterVertL.pos ) * waterVert.tex.x;
				waterVert.tex.x = fMinTexX + ( fMaxTexX - fMinTexX ) * waterVert.tex.x;
				if ( (k != 0) && (k != (pRiver->nWater2SamplesNum - 1)) )
					waterVert.pos += pRiver->precNormsL[i] * NWin32Random::RandomCheck( -pRiver->pDesc->waterLayers[1].fDisturbance, pRiver->pDesc->waterLayers[1].fDisturbance );
				/*if ( ( k == 0 ) || ( k == ( curRiverInfo.nWater2SamplesNum - 1 ) ) )
				waterVert.normal.w = 0;
				else
				waterVert.normal.w = 0xff;
				waterVert.normal.w = Clamp( int( (float)waterVert.normal.w * it->fOpacity ), 0, 255 );*/
				waterVert.normal.w = 0;// IsPointOnMap( GetVec2(waterVert.pos) ) ? Clamp( Float2Int(pInstance->points[i].fOpacity * 255.0f), 0, 255 ) : 0.0f;
				gfxInfo.water2Patches[nCurPatch].vertices.push_back( waterVert );
				if ( (nPatchInd == DEF_RIVER_SAMPLES_PER_PATCH) && (nCurPatch < (nNumPatches - 1)) )
					gfxInfo.water2Patches[nCurPatch + 1].vertices.push_back( waterVert );
			}
		}

		if ( ++nPatchInd > DEF_RIVER_SAMPLES_PER_PATCH )
		{
			nPatchInd -= DEF_RIVER_SAMPLES_PER_PATCH;
			++nCurPatch;
		}
	}

	STriangle trg;
	const int nLastSize = ( (pInstance->points.size() - 1) % DEF_RIVER_SAMPLES_PER_PATCH ) ? ( (pInstance->points.size() - 1) % DEF_RIVER_SAMPLES_PER_PATCH ) :
												DEF_RIVER_SAMPLES_PER_PATCH;
	for ( int i = 0; i < nNumPatches; ++i )
	{
		const int nPointsNum = ( i < (nNumPatches - 1) ) ? DEF_RIVER_SAMPLES_PER_PATCH : nLastSize;
		for ( int g = 0; g < nPointsNum; ++g )
		{
			// bottom triangles
			const int nBottomVertsOffs = g * DEF_BOTTOM_SAMPLES_NUM;
			for ( int k = 0; k < (DEF_BOTTOM_SAMPLES_NUM - 1); ++k )
			{
				trg.i1 = nBottomVertsOffs + k + DEF_BOTTOM_SAMPLES_NUM;
				trg.i2 = trg.i1 - DEF_BOTTOM_SAMPLES_NUM;
				trg.i3 = trg.i2 + 1;
				gfxInfo.bottomPatches[i].triangles.push_back( trg );
				trg.i1 = trg.i3;
				trg.i2 = trg.i1 + DEF_BOTTOM_SAMPLES_NUM;
				trg.i3 = trg.i2 - 1;
				gfxInfo.bottomPatches[i].triangles.push_back( trg );
			}

			// water triangles
			const int nWaterVertsOffs = g * pRiver->nWaterSamplesNum;
			for ( int k = 0; k < (pRiver->nWaterSamplesNum - 1); ++k )
			{
				trg.i1 = nWaterVertsOffs + k + pRiver->nWaterSamplesNum;
				trg.i2 = trg.i1 - pRiver->nWaterSamplesNum;
				trg.i3 = trg.i2 + 1;
				gfxInfo.waterPatches[i].triangles.push_back( trg );
				trg.i1 = trg.i3;
				trg.i2 = trg.i1 + pRiver->nWaterSamplesNum;
				trg.i3 = trg.i2 - 1;
				gfxInfo.waterPatches[i].triangles.push_back( trg );
			}

			// water2 triangles
			const int nWater2VertsOffs = g * pRiver->nWater2SamplesNum;
			for ( int k = 0; k < (pRiver->nWater2SamplesNum - 1); ++k )
			{
				trg.i1 = nWater2VertsOffs + k + pRiver->nWater2SamplesNum;
				trg.i2 = trg.i1 - pRiver->nWater2SamplesNum;
				trg.i3 = trg.i2 + 1;
				gfxInfo.water2Patches[i].triangles.push_back( trg );
				trg.i1 = trg.i3;
				trg.i2 = trg.i1 + pRiver->nWater2SamplesNum;
				trg.i3 = trg.i2 - 1;
				gfxInfo.water2Patches[i].triangles.push_back( trg );
			}
		}
	}

	if ( bNeedUpdateHeights )
		UpdateRiverHeights( pRiver, &gfxInfo, pInstance );

	//ClampRiverGfxByMap( &gfxInfo );

	terrainGfxInfo.rivers.push_back( gfxInfo );

	if ( pGfxObserver )
		pGfxObserver->AddRiver( &(terrainGfxInfo.rivers.back()) );
}

void CTerraGen::PutAllRiversOnTerrain()
{
	for ( list<STerrainInfo::SRiver>::iterator it = terrainInfo.rivers.begin(); it != terrainInfo.rivers.end(); ++it )
		PutRiverOnTerrain( &(*it), CVec2i(-1, -1), CVec2i(-1, -1) );
}

void CTerraGen::PutRiverOnTerrain( STerrainInfo::SRiver *pRiver, const CVec2i &vMinTile, const CVec2i &vMaxTile )
{
	// make hole in terrain
	if ( vMinTile.x == -1 )
		MakeHole( pRiver->samples, pRiver->vSampMin.x, pRiver->vSampMin.y, pRiver->vSampMax.x, pRiver->vSampMax.y );
	else
		MakeHole( pRiver->samples, vMinTile.x, vMinTile.y, vMaxTile.x, vMaxTile.y );

	SRiverGFXInfo *pGfxInfo = FindRiverGfxInfo( pRiver->nID );
	const NDb::SVSOInstance *pRiverInstance = FindRiver( pRiver->nID );

	if ( pGfxInfo && pRiverInstance )
		UpdateRiverHeights( pRiver, pGfxInfo, pRiverInstance );

	float fHeight;
	for ( vector<CVec3>::iterator it = pRiver->precVertsL.begin(); it != pRiver->precVertsL.end(); ++it )
	{
		GetMaxCragHeightEx( CVec2(it->x, it->y), &fHeight );
		it->z = GetTerraHeight( it->x, it->y ) + fHeight;
	}
	for ( vector<CVec3>::iterator it = pRiver->precVertsR.begin(); it != pRiver->precVertsR.end(); ++it )
	{
		GetMaxCragHeightEx( CVec2(it->x, it->y), &fHeight );
		it->z = GetTerraHeight( it->x, it->y ) + fHeight;
	}

	AddToPrecipiceUpdateQueue( pRiver->nID | 0x10000 );

	if ( pGfxInfo && pGfxObserver )
		pGfxObserver->UpdateRiver( pRiver->nID );
}

void CTerraGen::RiverManipulator( STerrainInfo::SRiver *pRiver, const bool bRemove )
{
	const CVec2i vBBMin = pRiver->vSampMin;
	const CVec2i vBBMax = pRiver->vSampMax;

	vector<int> updatedCrags( 64 );
	vector<int> updatedRivers( 64 );

	CollectAllCragsAndRiversInArea( &updatedCrags, &updatedRivers, vBBMin, vBBMax, -1, pRiver->nID );

	// remove current river
	if ( bRemove )
	{
		RemovePrecipiceFromCollector( 0x10000 | (pRiver->nID), false );
		RemovePrecipiceFromCollector( 0x20000 | (pRiver->nID), false );
		RemoveRiverGfxInfo( pRiver->nID );
		if ( pGfxObserver )
		{
			pGfxObserver->RemoveRiver( pRiver->nID );
		}
		RemoveRiverInfo( pRiver->nID );
	}

	if ( !bRemove )
		AddUnique( &updatedRivers, pRiver->nID );

	UpdateCragsAndRiversInArea( updatedCrags, updatedRivers, vBBMin, vBBMax );

	UpdateArea( vBBMin.x, vBBMin.y, vBBMax.x + 1, vBBMax.y + 1, TERRAIN_UPDATE_ALL );
	UpdateAllOnTerrainObjectsInArea( vBBMin, vBBMax );

	// grid updater (intellectual)
	if ( IScene *pScene = Scene() )
		pScene->UpdateGrid( vBBMin.x, vBBMin.y, vBBMax.x, vBBMax.y );
}

void CTerraGen::RemoveRiver( const int nVSOID )
{
	// if such crag is not exists, than skip it removing
	list<STerrainInfo::SRiver>::iterator itRiver = terrainInfo.rivers.begin();
	for ( ; itRiver != terrainInfo.rivers.end(); ++itRiver )
	{
		if ( itRiver->nID == nVSOID )
			break;
	}
	if ( itRiver == terrainInfo.rivers.end() )
	{
		NI_ASSERT( itRiver != terrainInfo.rivers.end(), StrFmt("Removed river is not exists: %d", nVSOID) );
		return;
	}

	RiverManipulator( &(*itRiver), true );
}

void CTerraGen::UpdateRiver( const int nVSOID )
{
	RemoveRiver( nVSOID );
	AddRiver( FindRiver(nVSOID) );
}

void CTerraGen::RemoveRiverInfo( const int nVSOID )
{
	for ( list<STerrainInfo::SRiver>::iterator it = terrainInfo.rivers.begin(); it != terrainInfo.rivers.end(); ++it )
	{
		if ( it->nID == nVSOID )
		{
			terrainInfo.rivers.erase( it );
			return;
		}
	}
	NI_ASSERT( false, StrFmt("Couldn't find info for river with ID=%d", nVSOID) );
}

void CTerraGen::RemoveRiverGfxInfo( const int nVSOID )
{
	for ( list<SRiverGFXInfo>::iterator it = terrainGfxInfo.rivers.begin(); it != terrainGfxInfo.rivers.end(); ++it )
	{
		if ( it->nID == nVSOID )
		{
			terrainGfxInfo.rivers.erase( it );
			return;
		}
	}
	NI_ASSERT( false, StrFmt( "Couldn't find GFX info for river with ID=%d", nVSOID ) );
}

void CTerraGen::AddAllRivers()
{
	NI_ASSERT( pDesc, "Terrain is not loaded" );
	//RemoveAllRivers();

	for ( vector<NDb::SVSOInstance>::const_iterator it = pDesc->rivers.begin(); it != pDesc->rivers.end(); ++it )
		AddRiver( &(*it), GetVSOSeed(it) );

	//if ( !pDesc->rivers.empty() )
	//{
	//	NDb::SVSOInstance firstInst = pDesc->rivers[0];
	//	int nInd = 0;
	//	bool bFlag = true;
	//	for ( vector<NDb::SVSOPoint>::iterator it = firstInst.points.begin(); it != firstInst.points.end(); ++it, ++nInd )
	//	{
	//		if ( ( nInd % 7 ) == 0 )
	//			bFlag = !bFlag;
	//		it->fRadius = bFlag ? 1.0f : 0.0f;
	//		it->fReserved = 1.0f;
	//	}
	//	AddRiver( &firstInst, GetTickCount() );
	//	for ( int i = 1; i < pDesc->rivers.size(); ++i )
	//		AddRiver( &(pDesc->rivers[i]), GetTickCount() );
	//}
}

/*void CTerraGen::RemoveAllRivers()
{
	NI_ASSERT( pDesc, "Terrain is not loaded" );
	for ( list<STerrainInfo::SRiver>::const_iterator it = terrainInfo.rivers.begin(); it != terrainInfo.rivers.end(); ++it )
	{
		RemovePeakInfo( ( it->nID ) | 0x10000 );
		RemovePeakInfo( ( it->nID ) | 0x20000 );
		//RemovePeakInfo( 0xf0000 | ( it->nID ) );
		RemovePeakGfxInfo( it->nID | 0x10000 );
		RemovePeakGfxInfo( it->nID | 0x20000 );
		//RemovePeakGfxInfo( 0xf0000 | ( it->nID ) );
		if ( pGfxObserver )
		{
			pGfxObserver->RemoveRiver( it->nID );
			pGfxObserver->RemovePeak( ( it->nID ) | 0x10000 );
			pGfxObserver->RemovePeak( ( it->nID ) | 0x20000 );
			//pGfxObserver->RemovePeak( 0xf0000 | ( it->nID ) );
		}
	}
	terrainGfxInfo.rivers.clear();
	terrainInfo.rivers.clear();
}*/

inline const NDb::SVSOInstance* CTerraGen::FindRiver( int nID ) const
{
	for ( vector<NDb::SVSOInstance>::const_iterator it = pDesc->rivers.begin(); it != pDesc->rivers.end(); ++it )
	{
		if ( it->nVSOID == nID )
			return static_cast<const NDb::SVSOInstance *>( &(*it) );
	}
	//NI_ASSERT( false, StrFmt( "Couldn't find rivers's instance: ID=%d", nID ) );
	return 0;
}

STerrainInfo::SRiver* CTerraGen::FindRiverInfo( int nID )
{
	for ( list<STerrainInfo::SRiver>::iterator it = terrainInfo.rivers.begin(); it != terrainInfo.rivers.end(); ++it )
	{
		if ( it->nID == nID )
			return ( &(*it) );
	}
	//NI_ASSERT( false, StrFmt("Couldn't find river's info: ID=%d", nID) );
	return 0;
}

struct SRiverProfile
{
	float operator()( const float x ) const
	{ 
		// TODO: old riverprofile
		if ( x <= DEF_RIVER_RIDGE_NULL )
			return 1.0f;
		const float tx = ( x - DEF_RIVER_RIDGE_NULL ) * DEF_RIVER_RIDGE_COEFF;
		return 1.0f - tx * tx;
		//return 1.0f - x * x;

		// TODO: new riverprofile
		//if ( x <= DEF_RIVER_RIDGE_NULL )
		//	return 1.0f;
		//const float tx = ( x - DEF_RIVER_RIDGE_NULL ) / ( 1.0f - DEF_RIVER_RIDGE_NULL );
		//return ( 1 + cos(FP_PI * tx) ) / 2.0f;
	}
};

struct SRiverBottomProfile
{
	float operator()( const float x ) const
	{ 
		return 1.0f;
	}
};

float CTerraGen::GetMaxRiverHeight( const CVec2 &v ) const
{
	if ( terrainInfo.rivers.empty() )
		return 0.0f;

	bool bFlag = false;
	float fHeight = -FP_MAX_VALUE;
	float fH;
	SRiverProfile ridgeProfile;
	SRiverBottomProfile bottomProfile;
	for ( list<STerrainInfo::SRiver>::const_iterator it = terrainInfo.rivers.begin(); it != terrainInfo.rivers.end(); ++it )
	{
		if ( IsInsideBB(v, it->vBBMin, it->vBBMax) )
		{
			if ( GetIncRidgeHeight(v, it->ridgeL, &fH, ridgeProfile) )
			{
				fHeight = max( fHeight, fH );
				bFlag = true;
			}
			if ( GetIncRidgeHeight(v, it->ridgeR, &fH, ridgeProfile) )
			{
				fHeight = max( fHeight, fH );
				bFlag = true;
			}
		}
	}

	if ( !bFlag )
	{
		for ( list<STerrainInfo::SRiver>::const_iterator it = terrainInfo.rivers.begin(); it != terrainInfo.rivers.end(); ++it )
		{
			if ( IsInsideBB(v, it->vBBMin, it->vBBMax) )
			{
				if ( GetIncRidgeHeight(v, it->samples, &fH, bottomProfile) )
				{
					fHeight = DEF_RIVER_DEPTH;//fH;
					bFlag = true;
					break;
				}
			}
		}
	}

	return bFlag ? -fHeight : 0.0f;
}

#define DEF_MAX_UPDATES_COUNT 5
void CTerraGen::UpdateRiversDepthes()
{
	CVec2 vBary;
	float fMinHeight1, fMinHeight2, fPrevHeight;
	bool bUpdate;
	bool bWasUpdated = true;
	int nUpdateCount = 0;

	while ( (bWasUpdated) && (nUpdateCount < DEF_MAX_UPDATES_COUNT) )
	{
		bWasUpdated = false;
		++nUpdateCount;

		for ( list<STerrainInfo::SRiver>::iterator itCurRiver = terrainInfo.rivers.begin(); itCurRiver != terrainInfo.rivers.end(); ++itCurRiver )
		{
			bUpdate = false;

			NI_ASSERT( (itCurRiver->precVertsL.size() == itCurRiver->precVertsR.size()) &&
								 (itCurRiver->precVertsL.size() == itCurRiver->precNormsL.size()) &&
								 (itCurRiver->precVertsL.size() == itCurRiver->precNormsR.size()) &&
								 (itCurRiver->precVertsL.size() == itCurRiver->precHeightsL.size()) &&
								 (itCurRiver->precVertsL.size() == itCurRiver->precHeightsR.size()), "Wrong river's data" );

			for ( int i = 0; i < itCurRiver->precVertsL.size(); ++i )
			{
				const int nInvI = itCurRiver->precVertsL.size() - 1 - i;
				const CVec3 vert1( itCurRiver->precVertsL[i].x, itCurRiver->precVertsL[i].y, itCurRiver->precHeightsL[i] );
				const CVec3 vert2( itCurRiver->precVertsR[nInvI].x, itCurRiver->precVertsR[nInvI].y, itCurRiver->precHeightsR[nInvI] );
				fMinHeight1 = vert1.z;
				fMinHeight2 = vert2.z;

				NI_ASSERT( fabs(fMinHeight1 - fMinHeight2) < DEF_EPS, "Wrong river's heigths" );

				for ( list<STerrainInfo::SRiver>::iterator itLastRiver = terrainInfo.rivers.begin(); itLastRiver != terrainInfo.rivers.end(); ++itLastRiver )
				{
					if ( itCurRiver != itLastRiver )
					{
						if ( (IsInsideBB(CVec2(vert1.x, vert1.y), itLastRiver->vBBMin, itLastRiver->vBBMax)) ||
								 (IsInsideBB(CVec2(vert2.x, vert2.y), itLastRiver->vBBMin, itLastRiver->vBBMax)) )
						{
							for ( int g = 1; g < itLastRiver->precVertsL.size(); ++g )
							{
								const int nInvG = itLastRiver->precVertsL.size() - 1 - g;
								const CVec3 v1( itLastRiver->precVertsL[g - 1].x, itLastRiver->precVertsL[g - 1].y, itLastRiver->precHeightsL[g - 1] );
								const CVec3 v2( itLastRiver->precVertsL[g].x, itLastRiver->precVertsL[g].y, itLastRiver->precHeightsL[g] );
								const CVec3 v3( itLastRiver->precVertsR[nInvG + 1].x, itLastRiver->precVertsR[nInvG + 1].y, itLastRiver->precHeightsR[nInvG + 1] );
								const CVec3 v4( itLastRiver->precVertsR[nInvG].x, itLastRiver->precVertsR[nInvG].y, itLastRiver->precHeightsR[nInvG] );

								GetBaryCoords( vert1, v1, v3, v2, &vBary );
								if ( (vBary.x > -DEF_EPS) && (vBary.y > -DEF_EPS) && ((vBary.x + vBary.y) < (1.0f + DEF_EPS)) )
									fMinHeight1 = min( fMinHeight1, (v3.z - v1.z) * vBary.x + (v2.z - v1.z) * vBary.y + v1.z );
								GetBaryCoords( vert1, v4, v2, v3, &vBary );
								if ( (vBary.x > -DEF_EPS) && (vBary.y > -DEF_EPS) && ((vBary.x + vBary.y) < (1.0f + DEF_EPS)) )
									fMinHeight1 = min( fMinHeight1, (v2.z - v4.z) * vBary.x + (v3.z - v4.z) * vBary.y + v4.z );

								GetBaryCoords( vert2, v1, v3, v2, &vBary );
								if ( (vBary.x > -DEF_EPS) && (vBary.y > -DEF_EPS) && ((vBary.x + vBary.y) < (1.0f + DEF_EPS)) )
									fMinHeight2 = min( fMinHeight2, (v3.z - v1.z) * vBary.x + (v2.z - v1.z) * vBary.y + v1.z );
								GetBaryCoords( vert2, v4, v2, v3, &vBary );
								if ( (vBary.x > -DEF_EPS) && (vBary.y > -DEF_EPS) && ((vBary.x + vBary.y) < (1.0f + DEF_EPS)) )
									fMinHeight2 = min( fMinHeight2, (v2.z - v4.z) * vBary.x + (v3.z - v4.z) * vBary.y + v4.z );
							}
						}
					}
				}

				const float fMinHeight = min( fMinHeight1, fMinHeight2 );

				if ( fMinHeight < vert1.z )
				{
					bUpdate = true;

					itCurRiver->precHeightsL[i] = fMinHeight;
					itCurRiver->precHeightsR[nInvI] = fMinHeight;

					const int nFirst = i - DEF_DEPTH_INTERPOLATE_LEN;
					const int nFirstInd = max( nFirst, 0 );
					for ( int k = nFirstInd; k < i; ++k )
					{
						const float fCoeff = (float)( k - nFirst ) * DEF_DEPTH_INTERPOLATE_COEFF;
						itCurRiver->precHeightsL[i] = itCurRiver->precHeightsL[i] * fCoeff + fMinHeight * ( 1.0f - fCoeff );
						itCurRiver->precHeightsR[nInvI] = itCurRiver->precHeightsR[nInvI] * fCoeff + fMinHeight * ( 1.0f - fCoeff );
					}
				}
			}

			if ( bUpdate) // river was updated
			{
				fPrevHeight = FP_MAX_VALUE;
				for ( int i = 0; i < itCurRiver->precHeightsL.size(); ++i )
				{
					const int nInvInd = itCurRiver->precHeightsL.size() - 1 - i;
					fPrevHeight = min( fPrevHeight, min(itCurRiver->precHeightsL[i], itCurRiver->precHeightsR[nInvInd]) );
					itCurRiver->precHeightsL[i] = fPrevHeight;
					itCurRiver->precHeightsR[nInvInd] = fPrevHeight;
				}

				SRiverGFXInfo *pGfxInfo = FindRiverGfxInfo( itCurRiver->nID );
				if ( pGfxInfo )
				{
					int nBottomInd = 0, nWaterInd = 0, nWater2Ind = 0;
					int nPatchInd = 0, nCurPatch = 0;
					const int nWaterSamplesNum = max( itCurRiver->nWaterSamplesNum, DEF_WATER_SAMPLES_NUM );
					const int nWater2SamplesNum = max( itCurRiver->nWater2SamplesNum, DEF_WATER2_SAMPLES_NUM );
					const NDb::SVSOInstance *pRiverInstance = FindRiver( itCurRiver->nID );
					for ( int i = 0; i < itCurRiver->precHeightsL.size(); ++i )
					{
						const int nInvInd = itCurRiver->precVertsL.size() - 1 - i;

						const float fHeight = min( itCurRiver->precHeightsL[i], itCurRiver->precHeightsR[itCurRiver->precHeightsR.size() - 1 - i] );
						//const float fHeight = min( itCurRiver->precHeightsL[i], itCurRiver->precHeightsR[i] );
						const float fWaterHeight = min( fHeight + DEF_RIVER_WATER_LEVEL, pGfxInfo->waterPatches[nCurPatch].vertices[nWaterInd].pos.z );

						const float fTerraHeightL = GetTerraHeightNative( itCurRiver->precVertsL[i].x, itCurRiver->precVertsL[i].y );
						const float fTerraHeightR = GetTerraHeightNative( itCurRiver->precVertsR[nInvInd].x, itCurRiver->precVertsR[nInvInd].y );
						const float fInvRiverWidth = 1.0f / AI2Vis( pRiverInstance->points[i].fWidth );
						const int nAlphaCellsNumL = DEF_WATER_ALPHA_INIT + Float2Int( fabs( fTerraHeightL - fWaterHeight ) * fInvRiverWidth * DEF_WATER_ALPHA_FROM_HEIGHT_COEFF );
						const int nAlphaCellsNumR = DEF_WATER_ALPHA_INIT + Float2Int( fabs( fTerraHeightR - fWaterHeight ) * fInvRiverWidth * DEF_WATER_ALPHA_FROM_HEIGHT_COEFF );

						//const CVec2 vPoint( itCurRiver->precVertsL[i].x, itCurRiver->precVertsL[i].y );
						//const float fOpacity = IsPointOnMap( vPoint ) ? pRiverInstance->points[i].fOpacity : 0.0f;
						UpdateGfxHeights( pGfxInfo, &(*itCurRiver), fHeight, fWaterHeight, nCurPatch, nPatchInd, nBottomInd, nWaterInd, nWater2Ind,
															nWaterSamplesNum, nWater2SamplesNum, nAlphaCellsNumL, nAlphaCellsNumR, pRiverInstance->points[i].fOpacity );

						if ( ++nPatchInd > DEF_RIVER_SAMPLES_PER_PATCH )
						{
							nPatchInd -= DEF_RIVER_SAMPLES_PER_PATCH;
							++nCurPatch;
							nBottomInd = DEF_BOTTOM_SAMPLES_NUM;
							nWaterInd = nWaterSamplesNum;
							nWater2Ind = nWater2SamplesNum;
						}
					}

					if ( pGfxObserver )
						pGfxObserver->UpdateRiver( itCurRiver->nID );
				}

				bWasUpdated = true;

				AddToPrecipiceUpdateQueue( itCurRiver->nID | 0x10000 );
			}
		}
	}

	UpdateAllNeededPrecipices();
}

SRiverGFXInfo *CTerraGen::FindRiverGfxInfo( const int nID )
{
	for ( list<SRiverGFXInfo>::iterator it = terrainGfxInfo.rivers.begin(); it != terrainGfxInfo.rivers.end(); ++it )
	{
		if ( it->nID == nID )
			return &(*it);
	}
	return 0;
}

inline void CalcBBForGeometry( CVec3 *pBBMin, CVec3 *pBBMax, const NMeshData::SMeshData &data )
{
	pBBMin->Set( FP_MAX_VALUE, FP_MAX_VALUE, FP_MAX_VALUE );
	pBBMax->Set( -FP_MAX_VALUE, -FP_MAX_VALUE, -FP_MAX_VALUE );
	for ( vector<NGScene::SVertex>::const_iterator it = data.vertices.begin(); it != data.vertices.end(); ++it )
	{
		pBBMin->Minimize( it->pos );
		pBBMax->Maximize( it->pos );
	}
}

inline void AddInterpolatedVertex( vector<NGScene::SVertex> *pClampVerts, const CVec3 &v, const NGScene::SVertex &vert1,
																	 const NGScene::SVertex &vert2, const NGScene::SVertex &vert3,
																	 const float fAlpha1, const float fAlpha2, const float fAlpha3 )
{
	CVec2 vBary;
	GetBaryCoords( v, vert1.pos, vert2.pos, vert3.pos, &vBary );

	NGScene::SVertex vertex;
	vertex.pos = v;
	CVec3 vNorm = ( GetVector(vert1.normal) + GetVector(vert2.normal) + GetVector(vert3.normal) ) / 3.0f;
	if ( fabs2(vNorm) > DEF_EPS2 )
	{
		Normalize( &vNorm );
		CalcCompactVector( &(vertex.normal), vNorm );
	}
	else
		vertex.normal = vert1.normal;
	vertex.tex = ( vert2.tex - vert1.tex ) * vBary.x + ( vert3.tex - vert1.tex ) * vBary.y + vert1.tex;
	vertex.normal.w = 255 - ( DEF_MIN_REFL_ALPHA_VAL + Clamp(Float2Int(((fAlpha2 - fAlpha1) * vBary.x + (fAlpha3 - fAlpha1) * vBary.y + fAlpha1) * DEF_INV_MAX_REFL_ALPHA_LEN * DEF_REFL_ALPHA_RANGE), 0, DEF_REFL_ALPHA_RANGE) );

	vertex.texU = vert1.texU;
	vertex.texV = vert1.texV;
	pClampVerts->push_back( vertex );
}

inline bool IsPointUnderTrg( const CVec3 &v, const CVec3 &p1, const CVec3 &p2, const CVec3 &p3, const CVec3 &vNorm,
														 const float fDist, float &fDiff )
{
	CVec2 vBary;
	GetBaryCoords( v, p1, p2, p3, &vBary );
	if ( (vBary.x > -DEF_EPS) && (vBary.y > -DEF_EPS) && ((vBary.x + vBary.y) < (1.0f + DEF_EPS)) && (fabs(vNorm.z) > DEF_EPS) )
	{
		const float fPlaneZ = ( -fDist - v.x * vNorm.x - v.y * vNorm.y ) / vNorm.z;
		fDiff = fPlaneZ - v.z;
		return ( fDiff > ( - DEF_EPS ) );
	}

	fDiff = -FP_MAX_VALUE;

	return false;
}

inline void GetIntersectionOfPlaneAndSegment( vector<CVec3> *pIntersVerts, const CVec3 &v1, const CVec3 &v2, const CVec3 &vNorm,
																							const float fDist )
{
	const float d = ( v2 - v1 ) * vNorm;
	if ( fabs(d) > DEF_EPS )
	{
		const float t = -( vNorm * v1 + fDist ) / d;
		if ( (t > -DEF_EPS) && (t < (1.0f + DEF_EPS)) )
			PushBackUnique( pIntersVerts, v1 + (v2 - v1) * t );
	}
}

inline void GetClampVertices( vector<CVec3> *pClampVerts, const CVec3 &v1, const CVec3 &v2, const CVec3 &p1, const CVec3 &p2, const CVec3 &p3 )
{
	CVec2 vBary;
	GetBaryCoords( v1, p1, p2, p3, &vBary );
	if ( (vBary.x > -DEF_EPS) && (vBary.y > -DEF_EPS) && ((vBary.x + vBary.y) < (1.0f + DEF_EPS)) )
		//PushBackUnique( pClampVerts, v1 );
		pClampVerts->push_back( v1 );

	static vector<SIntersectPoint> locInters( 16 );
	locInters.resize( 0 );

	AddIntersection( &locInters, v1, v2, p1, p2 );
	AddIntersection( &locInters, v1, v2, p2, p3 );
	AddIntersection( &locInters, v1, v2, p3, p1 );
	sort( locInters.begin(), locInters.end() );

	for ( vector<SIntersectPoint>::const_iterator it = locInters.begin(); it != locInters.end(); ++it )
		PushBackUnique( pClampVerts, it->vPoint );

	GetBaryCoords( v2, p1, p2, p3, &vBary );
	if ( (vBary.x > -DEF_EPS) && (vBary.y > -DEF_EPS) && ((vBary.x + vBary.y) < (1.0f + DEF_EPS)) )
		//PushBackUnique( pClampVerts, v2 );
		pClampVerts->push_back( v2 );
}

inline void AddCorrectOrientedTrg( NMeshData::SMeshData *pData, const int nInd1, const int nInd2, const int nInd3 )
{
	CVec3 vNorm = ( pData->vertices[nInd2].pos - pData->vertices[nInd1].pos ) ^ ( pData->vertices[nInd3].pos - pData->vertices[nInd1].pos );
	Normalize( &vNorm );
	if ( (vNorm * GetVector(pData->vertices[nInd1].normal)) > 0.0f )
		pData->triangles.push_back( STriangle(nInd1, nInd2, nInd3) );
	else
		pData->triangles.push_back( STriangle(nInd1, nInd3, nInd2) );
}

struct SRiverVertsArrOrder
{
	int nNumber;
	float fDist;
	//
	SRiverVertsArrOrder() {}
	SRiverVertsArrOrder( const int _nNumber, const float _fDist )
		: nNumber( _nNumber ), fDist( _fDist ) {}
	bool operator < ( const SRiverVertsArrOrder &v ) const { return fDist < v.fDist; }
};

inline void AddVertexWithAlphaCalculation( vector<NGScene::SVertex> *pArray, const NGScene::SVertex &vert, const float fAlpha )
{
	pArray->push_back( vert );
	pArray->back().normal.w = 255 - ( DEF_MIN_REFL_ALPHA_VAL + Clamp(Float2Int(fAlpha * DEF_INV_MAX_REFL_ALPHA_LEN * DEF_REFL_ALPHA_RANGE), 0, DEF_REFL_ALPHA_RANGE) );
}

inline void GetIntersectionOfTrgAndSegment( vector<CVec3> &pIntersVerts, const CVec3 &v1, const CVec3 &v2,
																						const CVec3 &vNorm, const float fDist, const CVec3 &p1, const CVec3 &p2, const CVec3 &p3 )
{
	const float d = ( v2 - v1 ) * vNorm;
	if ( fabs(d) > DEF_EPS )
	{
		const float t = -( vNorm * v1 + fDist ) / d;
		if ( (t > -DEF_EPS) && (t < (1.0f + DEF_EPS)) )
		{
			const CVec3 p = v1 + ( v2 - v1 ) * t;
			CVec2 vBary;
			GetBaryCoords( p, p1, p2, p3, &vBary );
			if ( (vBary.x > -DEF_EPS) && (vBary.y > -DEF_EPS) && ((vBary.x + vBary.y) < (1.0f + DEF_EPS)) )
				pIntersVerts.push_back( p );
		}
	}
}

void CTerraGen::ClampUnderRivers( NMeshData::SMeshData *pData )
{
	if ( pData->triangles.empty() || pData->vertices.empty() )
		return;

	CVec3 vBBMin, vBBMax;
	CalcBBForGeometry( &vBBMin, &vBBMax, *pData );
	vBBMin.x -= DEF_EPS; vBBMin.y -= DEF_EPS;
	vBBMax.x += DEF_EPS; vBBMax.y += DEF_EPS;

	NMeshData::SMeshData newData;
	newData.vertices.reserve( 256 );
	newData.vertices.resize( 0 );
	newData.triangles.reserve( 256 );
	newData.triangles.resize( 0 );

	CVec3 vRiverBBMin, vRiverBBMax;

	vector<vector<CVec3> > riversClampVerts;
	riversClampVerts.reserve( 256 );

	vector<CVec3> intersVerts;
	intersVerts.reserve( 16 );
	vector<CVec3> clampVerts;
	clampVerts.reserve( 16 );

	vector<SRiverVertsArrOrder> orderVerts;
	orderVerts.reserve( 128 );
	vector<SRiverVertsArrOrder> localOrderVerts;
	localOrderVerts.reserve( 16 );
	vector<CVec3> localSwitchVerts;
	localSwitchVerts.reserve( 16 );

	CVec2 vBary;
	bool bFlag1, bFlag2, bFlag3;
	CVec3 vNorm, vOrder1, vOrder2, vOrder3, vMiddle;
	float fAlpha1, fAlpha2, fAlpha3, fDiff;

	int nCount, nOrderNum1, nOrderNum2;

	vector<CVec3> riversMins;
	riversMins.reserve( 64 );
	vector<CVec3> riversMaxs;
	riversMaxs.reserve( 64 );

	vector<SIntersectPoint> midPoints;
	midPoints.reserve( 32 );

	for ( vector<STriangle>::const_iterator itDataTrg = pData->triangles.begin(); itDataTrg != pData->triangles.end(); ++itDataTrg )
	{
		const NGScene::SVertex &v1 = pData->vertices[itDataTrg->i1];
		const NGScene::SVertex &v2 = pData->vertices[itDataTrg->i2];
		const NGScene::SVertex &v3 = pData->vertices[itDataTrg->i3];
		bFlag1 = bFlag2 = bFlag3 = false;
		fAlpha1 = fAlpha2 = fAlpha3 = -FP_MAX_VALUE;

		intersVerts.resize( 0 );

		for ( list<SRiverGFXInfo>::const_iterator itRiver = terrainGfxInfo.rivers.begin(); itRiver != terrainGfxInfo.rivers.end(); ++itRiver )
		{
			for ( vector<NMeshData::SMeshData>::const_iterator itPatch = itRiver->waterPatches.begin(); itPatch != itRiver->waterPatches.end(); ++itPatch )
			{
				CalcBBForGeometry( &vRiverBBMin, &vRiverBBMax, *itPatch );
				riversMins.push_back( vRiverBBMin );
				riversMaxs.push_back( vRiverBBMax );

				if ( (vBBMin.x < vRiverBBMax.x) && (vBBMin.y < vRiverBBMax.y) && (vBBMax.x > vRiverBBMin.x) && (vBBMax.y > vRiverBBMin.y) )
				{
					for ( vector<STriangle>::const_iterator itRiverTrg = itPatch->triangles.begin(); itRiverTrg != itPatch->triangles.end(); ++itRiverTrg )
					{
						const CVec3 &p1 = itPatch->vertices[itRiverTrg->i1].pos;
						const CVec3 &p2 = itPatch->vertices[itRiverTrg->i2].pos;
						const CVec3 &p3 = itPatch->vertices[itRiverTrg->i3].pos;

						// build plane
						vNorm = ( p2 - p1 ) ^ ( p3 - p1 );
						Normalize( &vNorm );
						if ( vNorm.z < 0.0f )
							vNorm = -vNorm;
						const float fDist = -p1 * vNorm;

						// check source vertices
						if ( IsPointUnderTrg(v1.pos, p1, p2, p3, vNorm, fDist, fDiff) )
							bFlag1 = true;
						fAlpha1 = max( fAlpha1, fDiff );
						if ( IsPointUnderTrg(v2.pos, p1, p2, p3, vNorm, fDist, fDiff) )
							bFlag2 = true;
						fAlpha2 = max( fAlpha2, fDiff );
						if ( IsPointUnderTrg(v3.pos, p1, p2, p3, vNorm, fDist, fDiff) )
							bFlag3 = true;
						fAlpha3 = max( fAlpha3, fDiff );

						GetIntersectionOfTrgAndSegment( intersVerts, v1.pos, v2.pos, vNorm, fDist, p1, p2, p3 );
						GetIntersectionOfTrgAndSegment( intersVerts, v2.pos, v3.pos, vNorm, fDist, p1, p2, p3 );
						GetIntersectionOfTrgAndSegment( intersVerts, v3.pos, v1.pos, vNorm, fDist, p1, p2, p3 );
					}
				}
			}
		}

		const int nVertsOffs = newData.vertices.size();
		nCount = 0;

		if ( bFlag1 )
		{
			AddVertexWithAlphaCalculation( &(newData.vertices), pData->vertices[itDataTrg->i1], fAlpha1 );
			++nCount;
		}
		if ( bFlag2 )
		{
			AddVertexWithAlphaCalculation( &(newData.vertices), pData->vertices[itDataTrg->i2], fAlpha2 );
			++nCount;
		}
		if ( bFlag3 )
		{
			AddVertexWithAlphaCalculation( &(newData.vertices), pData->vertices[itDataTrg->i3], fAlpha3 );
			++nCount;
		}

		if ( bFlag1 )
		{
			newData.vertices.push_back(pData->vertices[itDataTrg->i1]);
			++nCount;
		}
		if ( bFlag2 )
		{
			newData.vertices.push_back(pData->vertices[itDataTrg->i2]);
			++nCount;
		}
		if ( bFlag3 )
		{
			newData.vertices.push_back(pData->vertices[itDataTrg->i3]);
			++nCount;
		}

		if ( nCount > 0 )
		{
			if ( nCount == 3 )
				newData.triangles.push_back( STriangle(nVertsOffs, nVertsOffs + 1, nVertsOffs + 2) );
			else
			{

				if ( intersVerts.size() >= 2 )
				{
					int n1 = 0;
					int n2 = 1;
					float fMaxDist = fabs2( intersVerts[n1] - intersVerts[n2] );
					for ( int g = 0; g < intersVerts.size(); ++g )
					{
						for ( int i = g + 1; i < intersVerts.size(); ++i )
						{
							const float fCurDist = fabs2( intersVerts[i] - intersVerts[g] );
							if ( fCurDist > fMaxDist )
							{
								n1 = g;
								n2 = i;
								fMaxDist = fCurDist;
							}
						}
					}

					const CVec3 vInters1 = intersVerts[n1];
					const CVec3 vInters2 = intersVerts[n2];
					midPoints.resize( 0 );
					midPoints.push_back( SIntersectPoint(vInters1, 0.0f) );
					midPoints.push_back( SIntersectPoint(vInters2, 1.0f) );

					for ( list<SRiverGFXInfo>::const_iterator itRiver = terrainGfxInfo.rivers.begin(); itRiver != terrainGfxInfo.rivers.end(); ++itRiver )
					{
						for ( int nPatch = 0; nPatch < itRiver->waterPatches.size(); ++nPatch )
						{
							if ( (vBBMin.x < riversMaxs[nPatch].x) && (vBBMin.y < riversMaxs[nPatch].y) && (vBBMax.x > riversMins[nPatch].x) && (vBBMax.y > riversMins[nPatch].y) )
							{
								for ( vector<STriangle>::const_iterator itRiverTrg = itRiver->waterPatches[nPatch].triangles.begin(); itRiverTrg != itRiver->waterPatches[nPatch].triangles.end(); ++itRiverTrg )
								{
									const CVec3 &p1 = itRiver->waterPatches[nPatch].vertices[itRiverTrg->i1].pos;
									const CVec3 &p2 = itRiver->waterPatches[nPatch].vertices[itRiverTrg->i2].pos;
									const CVec3 &p3 = itRiver->waterPatches[nPatch].vertices[itRiverTrg->i3].pos;

									AddIntersection( &midPoints, vInters1, vInters2, p1, p2 );
									AddIntersection( &midPoints, vInters1, vInters2, p2, p3 );
									AddIntersection( &midPoints, vInters1, vInters2, p3, p1 );
								}
							}
						}
					}

					sort( midPoints.begin(), midPoints.end() );
				/*}

								// check intersections
								intersVerts.resize( 0 );
								GetIntersectionOfPlaneAndSegment( intersVerts, v1.pos, v2.pos, vNorm, fDist );
								GetIntersectionOfPlaneAndSegment( intersVerts, v2.pos, v3.pos, vNorm, fDist );
								GetIntersectionOfPlaneAndSegment( intersVerts, v3.pos, v1.pos, vNorm, fDist );

								clampVerts.resize( 0 );
								for ( int i = 0; i < ( intersVerts.size() - 1 ); ++i )
								{
									for ( int g = i + 1; g < intersVerts.size(); ++g )
									{
										GetClampVertices( clampVerts, intersVerts[i], intersVerts[g], p1, p2, p3 );
									}
								}

								if ( !( clampVerts.empty() ) )
									riversClampVerts.push_back( clampVerts );
							}
						}
					}
				}*/

					if ( nCount == 1 )
					{
						//for ( vector<vector<CVec3> >::const_iterator itRiversClampVertsArr = riversClampVerts.begin(); itRiversClampVertsArr != riversClampVerts.end(); ++itRiversClampVertsArr )
						for ( vector<SIntersectPoint>::const_iterator it = midPoints.begin(); it != midPoints.end(); ++it )
							//const int nCurOffset = newData.vertices.size();
							//for ( vector<CVec3>::const_iterator it = itRiversClampVertsArr->begin(); it != itRiversClampVertsArr->end(); ++it )
							//{
							AddInterpolatedVertex( &(newData.vertices), it->vPoint, v1, v2, v3, fAlpha1, fAlpha2, fAlpha3 );
							//}
						//for ( int i = 1; i < itRiversClampVertsArr->size(); ++i )
								//AddCorrectOrientedTrg( newData, nVertsOffs, nCurOffset + i - 1, nCurOffset + i );
						for ( int i = 1; i < midPoints.size(); ++i )
							AddCorrectOrientedTrg( &newData, nVertsOffs, nVertsOffs + i, nVertsOffs + i + 1 );
					}
					else // nCount == 2
					{
						if ( bFlag1 && bFlag2 )
						{
							if ( !IsIntersect(v1.pos, midPoints[0].vPoint, v2.pos, midPoints[midPoints.size() - 1].vPoint) )
							{
								nOrderNum1 = nVertsOffs;
								nOrderNum2 = nVertsOffs + 1;
							}
							else
							{
								nOrderNum1 = nVertsOffs + 1;
								nOrderNum2 = nVertsOffs;
							}
						}
						else if ( bFlag2 && bFlag3 )
						{
							if ( !IsIntersect(v2.pos, midPoints[0].vPoint, v3.pos, midPoints[midPoints.size() - 1].vPoint) )
							{
								nOrderNum1 = nVertsOffs;
								nOrderNum2 = nVertsOffs + 1;
							}
							else
							{
								nOrderNum1 = nVertsOffs + 1;
								nOrderNum2 = nVertsOffs;
							}
						}
						else //if ( bFlag2 && bFlag3 )
						{
							if ( !IsIntersect(v1.pos, midPoints[0].vPoint, v3.pos, midPoints[midPoints.size() - 1].vPoint) )
							{
								nOrderNum1 = nVertsOffs;
								nOrderNum2 = nVertsOffs + 1;
							}
							else
							{
								nOrderNum1 = nVertsOffs + 1;
								nOrderNum2 = nVertsOffs;
							}
						}

						for ( vector<SIntersectPoint>::const_iterator it = midPoints.begin(); it != midPoints.end(); ++it )
							AddInterpolatedVertex( &(newData.vertices), it->vPoint, v1, v2, v3, fAlpha1, fAlpha2, fAlpha3 );
						for ( int i = 1; i < midPoints.size(); ++i )
							AddCorrectOrientedTrg( &newData, nOrderNum1, nVertsOffs + i + 1, nVertsOffs + i + 2 );
						AddCorrectOrientedTrg( &newData, nVertsOffs + 2 + midPoints.size() - 1, nOrderNum2, nOrderNum1 );
					}
				}
			}
		}
	}

/*

					if ( bFlag1 && bFlag2 )
					{
						vOrder1 = v1.pos; vOrder2 = v2.pos; vOrder3 = v3.pos;
						nOrderNum1 = nVertsOffs; nOrderNum2 = nVertsOffs + 1;
					}
					else
						if ( bFlag2 && bFlag3 )
						{
							vOrder1 = v2.pos; vOrder2 = v3.pos; vOrder3 = v1.pos;
							nOrderNum1 = nVertsOffs; nOrderNum2 = nVertsOffs + 1;
						}
						else
						{
							vOrder1 = v1.pos; vOrder2 = v3.pos; vOrder3 = v2.pos;
							nOrderNum1 = nVertsOffs; nOrderNum2 = nVertsOffs + 1;
						}

					orderVerts.resize( 0 );
					for ( int i = 0; i < riversClampVerts.size(); ++i )
					{
						if ( !( riversClampVerts[i].empty() ) )
						{
							vMiddle.Set( 0, 0, 0 );
							nCount = 0;
							localOrderVerts.resize( 0 );
							for ( vector<CVec3>::const_iterator it = riversClampVerts[i].begin(); it != riversClampVerts[i].end(); ++it )
							{
								vMiddle += *it;
								GetBaryCoords( *it, vOrder1, vOrder2, vOrder3, &vBary );
								localOrderVerts.push_back( SRiverVertsArrOrder( nCount, vBary.x ) );
								++nCount;
							}

							sort( localOrderVerts.begin(), localOrderVerts.end() );

							localSwitchVerts.resize( 0 );
							for ( vector<SRiverVertsArrOrder>::const_iterator it = localOrderVerts.begin(); it != localOrderVerts.end(); ++it )
								localSwitchVerts.push_back( riversClampVerts[i][it->nNumber] );

							riversClampVerts[i] = localSwitchVerts;

							vMiddle /= nCount;
							GetBaryCoords( vMiddle, vOrder1, vOrder2, vOrder3, &vBary );
							orderVerts.push_back( SRiverVertsArrOrder( i, vBary.x ) );
						}
					}

					if ( !( orderVerts.empty() ) )
					{
						sort( orderVerts.begin(), orderVerts.end() );

						for ( vector<SRiverVertsArrOrder>::const_iterator itOrder = orderVerts.begin(); itOrder != orderVerts.end(); ++itOrder )
						{
							const int nCurOffset = newData.vertices.size();
							for ( vector<CVec3>::const_iterator it = riversClampVerts[itOrder->nNumber].begin(); it != riversClampVerts[itOrder->nNumber].end(); ++it )
							{
								AddInterpolatedVertex( newData.vertices, *it, v1, v2, v3, fAlpha1, fAlpha2, fAlpha3 );
							}
							for ( int i = 1; i < riversClampVerts[itOrder->nNumber].size(); ++i )
								AddCorrectOrientedTrg( newData, nOrderNum1, nCurOffset + i - 1, nCurOffset + i );
						}
						AddCorrectOrientedTrg( newData, nOrderNum2, nOrderNum1, newData.vertices.size() - 1 );
					}
				}*/
			/*}
		}
	}
	}*/

	// expand object to avoid z-fitting
	if ( !(newData.vertices.empty()) )
	{
		CVec3 vCenter( 0, 0, 0 ), vDist;
		for ( vector<NGScene::SVertex>::const_iterator it = newData.vertices.begin(); it != newData.vertices.end(); ++it )
			vCenter += it->pos;
		vCenter /= newData.vertices.size();
		for ( vector<NGScene::SVertex>::iterator it = newData.vertices.begin(); it != newData.vertices.end(); ++it )
		{
			vDist = it->pos - vCenter;
			if ( fabs2(vDist) > DEF_EPS )
			{
				const float d = fabs( vDist );
				vDist /= d;
				it->pos = vCenter + vDist * ( d + 0.05f );
			}
		}
	}

	pData->vertices = newData.vertices;
	pData->triangles = newData.triangles;
}

void CTerraGen::UpdateHeightsAfterRivers( const int nTileX1, const int nTileY1, const int nTileX2, const int nTileY2 )
{
	for ( int g = nTileY1; g <= nTileY2; ++g )
	{
		for ( int i = nTileX1; i <= nTileX2; ++i )
		{
			terrainInfo.riverHeights[g][i] = GetMaxRiverHeight( CVec2((float)i * DEF_TILE_SIZE, (float)g * DEF_TILE_SIZE) );
		}
	}
}

bool CTerraGen::IsPointInsideRivers( const CVec3 &v, const int nExcludeID )
{
	CVec3dEx vert( v, 0 );
	for ( list<STerrainInfo::SRiver>::const_iterator itRiver = terrainInfo.rivers.begin(); itRiver != terrainInfo.rivers.end(); ++itRiver )
	{
		if ( itRiver->nID != nExcludeID )
		{
			vector<CVec3dEx> samples( itRiver->samples.size() );
			for ( int i = 0; i < samples.size(); ++i )
				samples[i] = itRiver->samples[i];

			if ( IsInside(samples, vert) )
				return true;
		}
	}
	return false;
}


