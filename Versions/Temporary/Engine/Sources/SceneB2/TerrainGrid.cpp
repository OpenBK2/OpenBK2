#include "StdAfx.h"

#include "TerrainManager.h"
#include "SceneInternal.h"

static const CVec4 CLR_CENTER = CVec4(0.8f, 0.8f, 0.8f, 1.0f);
static const CVec4 CLR_BORDER = CVec4(1.0f, 1.0f, 1.0f, 1.0f);
static const bool bGridDepthTest = false;

static void CreatePolylineForOnePatch( vector<CVec3> *pVertices, vector<WORD> *pIndicesCenter, vector<WORD> *pIndicesBorder, 
																			 const int nStartX, const int nStartY, const int fTileSizeInAIUnits, const int nNumTilesInPatch )
{
	// create vertices
	pVertices->reserve( (nNumTilesInPatch + 1) * (nNumTilesInPatch + 1) );

	for ( int y = 0; y <= nNumTilesInPatch; ++y ) 
	{
		for ( int x = 0; x <= nNumTilesInPatch; ++x ) 
		{
			const CVec2 vPos = CVec2( (nStartX + x)*fTileSizeInAIUnits,	(nStartY + y)*fTileSizeInAIUnits );
			const float fZ = Scene()->GetZ( vPos.x, vPos.y );
			CVec3 vVisPos = CVec3( vPos, fZ );
			AI2Vis( &vVisPos );
			pVertices->push_back( vVisPos );
		}
	}
	// 
	const int nSizeX = (nNumTilesInPatch + 1);
	// create center indices (horizontal)
	for ( int y = 1; y < nNumTilesInPatch; ++y ) 
	{
		for ( int x = 0; x < nNumTilesInPatch; ++x ) 
		{
			pIndicesCenter->push_back( nSizeX*y + x );
			pIndicesCenter->push_back( nSizeX*y + x + 1 );
		}
	}
	// create center indices (vertical)
	for ( int y = 0; y < nNumTilesInPatch; ++y ) 
	{
		for ( int x = 1; x < nNumTilesInPatch; ++x ) 
		{
			pIndicesCenter->push_back( nSizeX*y + x );
			pIndicesCenter->push_back( nSizeX*(y + 1) + x );
		}
	}
	// create border indices
	for ( int i = 0; i < nNumTilesInPatch; ++i ) 
	{
		// horizontal
		pIndicesBorder->push_back( i );
		pIndicesBorder->push_back( i + 1 );
		pIndicesBorder->push_back( nNumTilesInPatch*nSizeX + i );
		pIndicesBorder->push_back( nNumTilesInPatch*nSizeX + i + 1 );
		// vertical
		pIndicesBorder->push_back( nSizeX*i );
		pIndicesBorder->push_back( nSizeX*(i + 1) );
		pIndicesBorder->push_back( nSizeX*i + nNumTilesInPatch );
		pIndicesBorder->push_back( nSizeX*(i + 1) + nNumTilesInPatch );
	}
}

void CScene::ShowTerrainGrid( ESceneShow eShow )
{
	bool bShow = data[eScene]->showModes[eShow];
	bool bAIGrid = ( eShow == SCENE_SHOW_AI_GRID );

	if ( !bShow )
	{
		data[eScene]->terrainGrid.clear();
	}
	else if ( data[eScene]->pTerraManager && data[eScene]->pTerraManager->GetDesc() ) 
	{
		vector< CObj<NGScene::CPolyline> > &grid = data[eScene]->terrainGrid;
		CPtr<NGScene::IGameView> pGScene = data[eScene]->GetGScene();
		
		const int nNumPatchesX = data[eScene]->pTerraManager->GetDesc()->nNumPatchesX;
		const int nNumPatchesY = data[eScene]->pTerraManager->GetDesc()->nNumPatchesY;
		grid.reserve( nNumPatchesY * nNumPatchesX * 2 );
		
		const int fTileSizeInAIUnits =	bAIGrid ? AI_TILE_SIZE : ( AI_TILE_SIZE * AI_TILES_IN_VIS_TILE );
		const int nNumTilesInPatch = bAIGrid ? (VIS_TILES_IN_PATCH * AI_TILES_IN_VIS_TILE) : VIS_TILES_IN_PATCH;

		for ( int y = 0; y < nNumPatchesY; ++y )
		{
			for ( int x = 0; x < nNumPatchesX; ++x )
			{
				vector<CVec3> vertices;
				vector<WORD> indicesCenter, indicesBorder;
				const int nStartX = x * nNumTilesInPatch;
				const int nStartY = y * nNumTilesInPatch;
				CreatePolylineForOnePatch( &vertices, &indicesCenter, &indicesBorder, nStartX, nStartY, fTileSizeInAIUnits, nNumTilesInPatch );
				//
				NGScene::CPolyline* pPolyGrid =	pGScene->CreatePolyline( vertices, indicesCenter, CLR_CENTER, bGridDepthTest );
				NGScene::CPolyline* pPolyBorder = pGScene->CreatePolyline( vertices, indicesBorder, CLR_BORDER, bGridDepthTest );
				grid.push_back( pPolyGrid );
				grid.push_back( pPolyBorder );
			}
		}
	}
}

void CScene::UpdateGrid( int nMinX, int nMinY, int nMaxX, int nMaxY )
{
	if ( IsShowOn(SCENE_SHOW_AI_GRID) )
		UpdateGrid( nMinX, nMinY, nMaxX, nMaxY, true );
	else if ( IsShowOn(SCENE_SHOW_GRID) )
		UpdateGrid( nMinX, nMinY, nMaxX, nMaxY, false );
}

void CScene::UpdateGrid( int nMinX, int nMinY, int nMaxX, int nMaxY, bool bAIGrid )
{
	if ( data[eScene]->pTerraManager && data[eScene]->pTerraManager->GetDesc() ) 
	{
		vector< CObj<NGScene::CPolyline> > &grid = data[eScene]->terrainGrid;
		CPtr<NGScene::IGameView> pGScene = data[eScene]->GetGScene();

		const int nNumPatchesX = data[eScene]->pTerraManager->GetDesc()->nNumPatchesX;
		const int nNumPatchesY = data[eScene]->pTerraManager->GetDesc()->nNumPatchesY;

		const int nTileSizeAI =	bAIGrid ? AI_TILE_SIZE : ( AI_TILE_SIZE * AI_TILES_IN_VIS_TILE );
		const int nTilesInPatchCount = bAIGrid ? (VIS_TILES_IN_PATCH * AI_TILES_IN_VIS_TILE) : VIS_TILES_IN_PATCH;

		int nMinUpdX = nMinX / VIS_TILES_IN_PATCH;
		int nMinUpdY = nMinY / VIS_TILES_IN_PATCH;
		int nMaxUpdX = nMaxX / VIS_TILES_IN_PATCH;
		int nMaxUpdY = nMaxY / VIS_TILES_IN_PATCH;

		nMinUpdX = Clamp( nMinUpdX, 0, nNumPatchesX );
		nMinUpdY = Clamp( nMinUpdY, 0, nNumPatchesY );
		nMaxUpdX = Clamp( nMaxUpdX, 0, nNumPatchesX );
		nMaxUpdY = Clamp( nMaxUpdY, 0, nNumPatchesY );

		if ( grid.size() != 2*(nNumPatchesY * nNumPatchesX) )
		{
			NI_ASSERT( grid.size() == (nNumPatchesY * nNumPatchesX), "CScene::UpdateGrid(): grid.size() != (nNumPatchesY * nNumPatchesX)" );
			return;
		}

		for ( int y = nMinUpdY; y <= nMaxUpdY; ++y )
		{
			for ( int x = nMinUpdX; x <= nMaxUpdX; ++x )
			{
				vector<CVec3> vertices;
				vector<WORD> indicesCenter, indicesBorder;
				const int nStartX = x * nTilesInPatchCount;
				const int nStartY = y * nTilesInPatchCount;

				CreatePolylineForOnePatch( &vertices, &indicesCenter, &indicesBorder, nStartX, nStartY, nTileSizeAI, nTilesInPatchCount );

				int nPatchIndex = 2*( (y * nNumPatchesX) + x );
				if ( nPatchIndex < 0 || nPatchIndex >= grid.size() )
					continue;

				grid[nPatchIndex] = pGScene->CreatePolyline( vertices, indicesCenter, CLR_CENTER, bGridDepthTest );
				grid[nPatchIndex + 1] =	pGScene->CreatePolyline( vertices, indicesBorder, CLR_BORDER, bGridDepthTest );
			}
		}
	}
}

