#include "stdafx.h"

#include "B2_M1_Terrain/DBPreLight.h"
#include "3Dmotor/DBScene.h"
#include "Stats_B2_M1/TerraAIObserver.h"
#include "TerraConverters.h"
#include "GenTerrain.h"

#include <cstdint>

#define DEF_TRGS_RESERVE_COUNT ( DEF_PATCH_SIZE * DEF_PATCH_SIZE * 2 )
#define DEF_VERTS_RESERVE_COUNT ( (DEF_PATCH_SIZE + 1) * (DEF_PATCH_SIZE + 1) )
#define DEF_TERRA_OBJS_COUNT 512
#define DEF_TERRA_OBJS_COUNT_PER_PATCH 32




int g_nTimeStatLevel = 0;

CTerraGen::CTerraGen()
	: vGeomModAreaMin( -1, -1 ),
	vGeomModAreaMax( -1, -1 ),
	vTexModMin( -1, -1 ),
	vTexModMax( -1, -1 )
{
	needAddFoots.reserve( 128 );
	needAddFoots.resize( 0 );
	updatedPrecipices.reserve( 128 );
	updatedPrecipices.resize( 0 );
	updatedPrecNodes.reserve( 512 );
	updatedPrecNodes.resize( 0 );
	//
	entrenchmentsHistory.reserve( 256 );
	entrenchmentsHistory.resize( 0 );
	explosionsHistory.reserve( 256 );
	explosionsHistory.resize( 0 );
}


void CTerraGen::Load( const NDb::STerrain *_pDesc, CDataStream *pStream )
{
	NI_VERIFY( _pDesc, "CTerraGen::Load - Invalid terrain descriptor received", return )

	TIME_STAT_START( CTerraGen__Load )
	pDesc = _pDesc;

	texModCheck.SetSizes( pDesc->nNumPatchesX, pDesc->nNumPatchesY );
	texModCheck.FillZero();

	// prepare data for geometry modifying
	needTexExportAfterGeomModifying.SetSizes( pDesc->nNumPatchesX, pDesc->nNumPatchesY );
	needTexExportAfterGeomModifying.FillZero();

	// init pre-light
	const float fHor = sin( ToRadian(pDesc->pPreLight->fPitch) );
	vPreLightDir.z = -cos( ToRadian(pDesc->pPreLight->fPitch) );
	vPreLightDir.x = fHor * cos( ToRadian(pDesc->pLight->fYaw) ); // use main light yaw angle !
	vPreLightDir.y = fHor * sin( ToRadian(pDesc->pLight->fYaw) );

	// prepare data for terra objects
	//terraObjsPatchesHash.SetSizes( pDesc->nNumPatchesX, pDesc->nNumPatchesY );

	// try to load or generate
	if ( !LoadTerrainInfo(pStream) )
	{
		// terrain info doesn't exists, generate it
		GenerateTerrain();
	}
	else
	{
		/*
		TIME_STAT_START( CTerraGen__UpdateGfxInfo )
		UpdateGfxInfo( 0, 0, terrainInfo.nNumPatchesX, terrainInfo.nNumPatchesY, false );
		TIME_STAT_FINISH( CTerraGen__UpdateGfxInfo )
		*/
	}

	terraExplosionsHash.SetSizes( terrainInfo.tiles.GetSizeX(), terrainInfo.tiles.GetSizeY() );
	terraExplosionsHeights.SetSizes( terrainInfo.tiles.GetSizeX(), terrainInfo.tiles.GetSizeY() );

	// {CRAP actually there's no need to regenerate CRAP}
	ReGenerate();

	TIME_STAT_FINISH( CTerraGen__Load )
}


void CTerraGen::Save( CDataStream *pStream )
{
	RemapPrecipices();
	SaveTerrainInfo( pStream );
}


void CTerraGen::ReGenerate()
{
	TIME_STAT_START( CTerraGen__ReGenerate )

	InitTileMasks();
	ResetTerrainTiles( 0, 0, terrainInfo.tiles.GetSizeX(), terrainInfo.tiles.GetSizeY() );
	InitWater();

	terrainInfo.crags.clear();
	terrainInfo.rivers.clear();
	terrainInfo.roads.clear();
	terrainInfo.peaks.clear();
	terrainInfo.foots.clear();
	terrainInfo.terraspots.clear();
	terrainInfo.precNodes.clear();
	terrainInfo.precipices.clear();

	if ( pGfxObserver )
	{
		pGfxObserver->RemoveAllRivers();
		pGfxObserver->RemoveAllRoads();
		pGfxObserver->RemoveAllTerraSpots();
		pGfxObserver->RemoveAllPeaks();
		pGfxObserver->RemoveAllFoots();
		pGfxObserver->RemoveAllPrecipices();
	}

	terrainGfxInfo.rivers.clear();
	terrainGfxInfo.roads.clear();
	terrainGfxInfo.peaks.clear();
	terrainGfxInfo.foots.clear();
	terrainGfxInfo.terraspots.clear();
	terrainGfxInfo.precipices.clear();

	updatedPrecipices.resize( 0 );
	updatedPrecNodes.resize( 0 );
	needAddFoots.resize( 0 );

	// regenerate all
	AddAllCrags();
	AddAllRivers();

	UpdateHeightsAfterRivers( 0, 0, terrainInfo.tiles.GetSizeX(), terrainInfo.tiles.GetSizeY() );

	PutAllCragsOnTerrain();
	PutAllRiversOnTerrain();

	UpdateArea( 0, 0, terrainInfo.tiles.GetSizeX(), terrainInfo.tiles.GetSizeY(), TERRAIN_UPDATE_GFX );
	UpdateRiversDepthes();

	AddAllRoads();
	AddAllTerraSpots();
	AddAllNeededFoots();

	RegenerateAIInfo();

	TIME_STAT_FINISH( CTerraGen__ReGenerate )
}


bool CTerraGen::LoadTerrainInfo( CDataStream *pStream )
{
	if ( !pStream || !pStream->IsOk() )
		return false;

	if ( CPtr<IBinSaver> pSaver = CreateBinSaver(pStream, SAVER_MODE_READ) )
	{
		pSaver->Add( 1, &terrainInfo );
		//
		RestoreTerrainInfoAfterOptimizing();
		InitTileMasks();
		return true;

		/*
		//		
		InitWater();
		CreateNormals();
		ReCreateAllFeaturesGfx();
		//
		UpdateAllAIInfo();
		PutAllFeaturesToAI();
		//
		return true;
		*/
	}
	//
	return false;
}


void CTerraGen::SaveTerrainInfo( CDataStream *pStream )
{
	if ( pStream->IsOk() ) 
	{
		OptimizeTerrainInfo();
		//
		if ( CPtr<IBinSaver> pSaver = CreateBinSaver(pStream, SAVER_MODE_WRITE) )
			pSaver->Add( 1, &terrainInfo );
		//
		RestoreTerrainInfoAfterOptimizing();
	}
}


void CTerraGen::InitTileMasks()
{
	if ( (terrainInfo.tileTerraMap.GetSizeX() != (pDesc->nNumPatchesX * DEF_PATCH_SIZE + 1)) ||
			 (terrainInfo.tileTerraMap.GetSizeY() != (pDesc->nNumPatchesY * DEF_PATCH_SIZE + 1)) )
	{
		// check for previous versions, where tile map was placed out of terrain info
		LegacyLoadTileMap( pDesc->szMapFilesPath, pDesc->nNumPatchesX, pDesc->nNumPatchesY, &terrainInfo.tileTerraMap );
	}

	tileTerraMasks.resize( pDesc->pTerraSet->terraTypes.size() );
	for ( int i = 0; i < tileTerraMasks.size(); ++i )
	{
		tileTerraMasks[i].SetSizes( terrainInfo.tileTerraMap.GetSizeX(), terrainInfo.tileTerraMap.GetSizeY() );
		if ( i == 0 )
			tileTerraMasks[i].FillEvery( 255 );
		else
			tileTerraMasks[i].FillZero();
	}

	UpdateTileMasks( 0, 0, pDesc->nNumPatchesX * DEF_PATCH_SIZE - 1, pDesc->nNumPatchesY * DEF_PATCH_SIZE - 1, false );
}


void CTerraGen::UpdateArea( const int nTileX1, const int nTileY1, const int nTileX2, const int nTileY2, const uint32_t dwUpdateFlags )
{
	UpdateHeightsAfterCrags( nTileX1, nTileY1, nTileX2, nTileY2 );
	UpdateNormals( nTileX1, nTileY1, nTileX2, nTileY2 );
	UpdateAllNeededPrecipices();

	const int nPatchX1 = Clamp( nTileX1 >> DEF_PATCH_SIZE_BITS, 0, terrainInfo.nNumPatchesX - 1 );
	const int nPatchY1 = Clamp( nTileY1 >> DEF_PATCH_SIZE_BITS, 0, terrainInfo.nNumPatchesY - 1 );
	const int nPatchX2 = Clamp( nTileX2 >> DEF_PATCH_SIZE_BITS, 0, terrainInfo.nNumPatchesX - 1 );
	const int nPatchY2 = Clamp( nTileY2 >> DEF_PATCH_SIZE_BITS, 0, terrainInfo.nNumPatchesY - 1 );

	if ( dwUpdateFlags & TERRAIN_UPDATE_GFX )
		UpdateGfxInfo( nPatchX1, nPatchY1, nPatchX2 + 1, nPatchY2 + 1 );

	if ( dwUpdateFlags & TERRAIN_UPDATE_AI )
		UpdateAIInfo( nTileX1, nTileY1, nTileX2, nTileY2 );
}


void CTerraGen::RestoreFromHistory()
{
	// restore entrenchments
	for ( std::vector<SEntrenchmentHistory>::const_iterator it = entrenchmentsHistory.begin(); it != entrenchmentsHistory.end(); ++it )
		AddEntrenchment( it->ctrlPoints, it->fWidth, false );

	// restore explosions
	for ( std::vector<SExplosionHistory>::const_iterator it = explosionsHistory.begin(); it != explosionsHistory.end(); ++it )
		AddExplosion( it->vMin, it->vMax, it->pMaterial, false );
}


void CTerraGen::SetGfxObserver( ITerraGfxObserver *_pGfxObserver )
{
	pGfxObserver = _pGfxObserver;
}


void CTerraGen::SetAIObserver( ITerraAIObserver *_pAIObserver )
{
	pAIObserver = _pAIObserver;
}


ITerraAIObserver* CTerraGen::GetAIObserver()
{
	return pAIObserver;
}


void CTerraGen::AttachGameView( NGScene::IGameView *_pGScene )
{
	pGScene = _pGScene;
	explosionsManager.AttachGameView( _pGScene );
	dynamicDebrisManager.AttachGameView( _pGScene );
}


void CTerraGen::AttachTimer( CFuncBase<STime> *_pTimer )
{
	pTimer = _pTimer;
	waterController.AttachTimer( _pTimer );
	surfController.AttachTimer( _pTimer );
}


int CTerraGen::operator&( IBinSaver &saver )
{
	saver.Add( 1, &entrenchmentsHistory );
	saver.Add( 2, &explosionsHistory );
	saver.Add( 3, &pAIObserver );
	//saver.Add( 4, &terraExplosionsHeights );

	return 0;
}


REGISTER_SAVELOAD_CLASS( 0x1007EA80, CTerraGen );


