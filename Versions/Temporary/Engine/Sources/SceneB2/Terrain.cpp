#include "stdafx.h"

#include "Scene.h"
#include "Terrain.h"
#include "GenTerrain.h"

#include <fmt/format.h>

// precipice

void CMeshDataPatch::Recalc()
{
	if ( pValue == 0 )
		pValue = new NGScene::CObjectInfo;
	NGScene::CObjectInfo::SData objData;

	objData.verts = pInfo->vertices;
	objData.geometry = pInfo->triangles;
	objData.attributes = pInfo->attributes;

	SplitWrapping( &objData );

	pValue->Assign( &objData, true );
	if ( !Scene()->IsEditorMode() )
		pInfo->Clear();
}

void CPrecipice::CreatePatches( NGScene::IGameView *pGView )
{
	SFBTransform placement;
	Identity( &placement.forward );
	Identity( &placement.backward );
	// create ridge patches
	patches.resize( 0 );
	patches.reserve( pInfo->patches.size() );
	for ( std::vector<NMeshData::SMeshData>::iterator it = pInfo->patches.begin(); it != pInfo->patches.end(); ++it )
	{
		SPrecipicePatchHolder patch;
		patch.pPatch = new CMeshDataPatch( &(*it) );
		patch.pHolder = pGView->CreateMesh( pGView->MakeMeshInfo( patch.pPatch, pInfo->pMaterial ), placement, 0, 0 );
		patches.push_back( patch );
	}
}

// Peak

void CPeakPatch::Recalc()
{
	if ( pValue == 0 )
		pValue = new NGScene::CObjectInfo;
	NGScene::CObjectInfo::SData objData;

	objData.verts = pInfo->vertices;
	objData.geometry = pInfo->triangles;
	objData.secondTex = pInfo->secondTex;
	objData.attributes = pInfo->attributes;

	pValue->Assign( &objData, true );
	if ( !Scene()->IsEditorMode() )
		pInfo->Clear();
}

void CPeak::CreatePatches( NGScene::IGameView *pGView, const NDb::STerrain *pTerraDesc )
{
	SFBTransform placement;
	Identity( &placement.forward );
	Identity( &placement.backward );
	patches.resize( pInfo->patches.size() );

	NGScene::SRoomInfo roomInfo( NGScene::LF_DO_NOT_MULTIPLY_ON_TRANSPARENCY, 0 );
	NGScene::SFullRoomInfo room( roomInfo, 0, 0 );

	for ( int g = 0; g < pInfo->patches.size(); ++g )
	{
		std::vector<NMeshData::SMeshDataTex2> &patchesArr = pInfo->patches[g];
		const int nSize = min( patchesArr.size(), pTerraDesc->pTerraSet->terraTypes.size() );
		std::vector<SPeakPatchHolder> &visPatches = patches[g];
		for ( int i = 0; i < nSize; ++i )
		{
			NMeshData::SMeshDataTex2 &data = patchesArr[i];
			if ( !data.triangles.empty() )
			{
				visPatches.push_back( SPeakPatchHolder() );
				visPatches.back().pPatch = new CPeakPatch( &data );
				visPatches.back().pHolder = pGView->CreateMesh( pGView->MakeMeshInfo( visPatches.back().pPatch, pTerraDesc->pTerraSet->terraTypes[i]->pPeakMaterial ), placement, 0, 0, room );
			}
		}
	}
}

// Foot

void CFoot::CreatePatches( NGScene::IGameView *pGView, const NDb::STerrain *pTerraDesc )
{
	SFBTransform placement;
	Identity( &placement.forward );
	Identity( &placement.backward );
	patches.resize( 0 );
	patches.reserve( pInfo->patches.size() );
	for ( std::vector<NMeshData::SMeshData>::iterator it = pInfo->patches.begin(); it != pInfo->patches.end(); ++it )
	{
		patches.push_back( SFootPatchHolder() );
		patches.back().pPatch = new CMeshDataPatch( &(*it) );
		patches.back().pHolder = pGView->CreateMesh( pGView->MakeMeshInfo( patches.back().pPatch, pInfo->pMaterial ), placement, 0, 0 );
	}
}

// river

void CRiverPatch::Recalc()
{
	if ( pValue == 0 ) 
		pValue = new NGScene::CObjectInfo;
	NGScene::CObjectInfo::SData objData;
	if ( pTimer ) 
	{
		objData.verts = pInfo->vertices;
		//objData.secondTex.resize( pInfo->vertices.size() );
		const int nTime = pTimer->GetValue();
		for ( std::vector<NGScene::SVertex>::iterator it = objData.verts.begin(); it != objData.verts.end(); ++it )
			it->tex.y -= nTime * fStreamSpeed / 1000.0f; // hz why minus must present
		objData.geometry = pInfo->triangles;
		pValue->AssignFast( &objData );
	}
	else
	{
		objData.verts = pInfo->vertices;
		objData.geometry = pInfo->triangles;

		SplitWrapping( &objData );

		pValue->Assign( &objData, true );
	}
}

void CRiver::CreatePatches( NGScene::IGameView *pGView )
{
	SFBTransform placement;
	Identity( &placement.forward );
	Identity( &placement.backward );
	// create water patches
	waterPatches.resize( 0 );
	waterPatches.reserve( pInfo->waterPatches.size() );
	if ( pInfo->pDesc->waterLayers.size() >= 1 )
	{
		for ( int i = 0; i < pInfo->waterPatches.size(); ++i )
		{
			NI_ASSERT( pInfo->pDesc->waterLayers[0].materials[0], "Material is not set for first river layer" );
			if ( pInfo->pDesc->waterLayers[0].materials[0] == 0 ) 
				continue;
			//NGScene::SFullRoomInfo room( NGScene::SRoomInfo( NGScene::LF_SKIP_LIGHTING, -100 ), 0, 0 );
			NGScene::SFullRoomInfo room( NGScene::SRoomInfo( NGScene::LF_NEVER_STATIC, -100 ), 0, 0 );
			CCSBound *pBound = new CCSBound();
			// calc bound
			CVec3 vMin( FP_MAX_VALUE, FP_MAX_VALUE, FP_MAX_VALUE ), vMax( -FP_MAX_VALUE, -FP_MAX_VALUE, -FP_MAX_VALUE );
			for ( std::vector<NGScene::SVertex>::const_iterator it = pInfo->waterPatches[i].vertices.begin(); it != pInfo->waterPatches[i].vertices.end(); ++it )
			{
				vMin.Minimize( it->pos );
				vMax.Maximize( it->pos );
			}
			SBound bound;
			bound.BoxInit( vMin, vMax );
			pBound->Set( bound );
			//
			SRiverPatchHolder patch;
			patch.pPatch = new CRiverPatch( &pInfo->waterPatches[i], pInfo->pDesc->waterLayers[0].fStreamSpeed, pTimer );
			patch.pHolder = pGView->CreateDynamicMesh( pGView->MakeMeshInfo( patch.pPatch, pInfo->pDesc->waterLayers[0].materials[0] ), 0, pBound, NGScene::MakeLargeHintBound(), room );
			waterPatches.push_back( patch );
		}
	}

	// create water2 patches
	water2Patches.resize( 0 );
	water2Patches.reserve( pInfo->water2Patches.size() );
	if ( pInfo->pDesc->waterLayers.size() >= 2 )
	{
		for ( int i = 0; i < pInfo->water2Patches.size(); ++i )
		{
			NI_ASSERT( pInfo->pDesc->waterLayers[1].materials[0], "Material is not set for first river layer" );
			if ( pInfo->pDesc->waterLayers[1].materials[0] == 0 ) 
				continue;
			//NGScene::SFullRoomInfo room( NGScene::SRoomInfo( NGScene::LF_SKIP_LIGHTING, -100 ), 0, 0 );
			NGScene::SFullRoomInfo room( NGScene::SRoomInfo( NGScene::LF_NEVER_STATIC, -100 ), 0, 0 );
			CCSBound *pBound = new CCSBound();
			// calc bound
			CVec3 vMin( FP_MAX_VALUE, FP_MAX_VALUE, FP_MAX_VALUE ), vMax( -FP_MAX_VALUE, -FP_MAX_VALUE, -FP_MAX_VALUE );
			for ( std::vector<NGScene::SVertex>::const_iterator it = pInfo->water2Patches[i].vertices.begin(); it != pInfo->water2Patches[i].vertices.end(); ++it )
			{
				vMin.Minimize( it->pos );
				vMax.Maximize( it->pos );
			}
			SBound bound;
			bound.BoxInit( vMin, vMax );
			pBound->Set( bound );
			//
			SRiverPatchHolder patch;
			patch.pPatch = new CRiverPatch( &pInfo->water2Patches[i], pInfo->pDesc->waterLayers[1].fStreamSpeed, pTimer );
			patch.pHolder = pGView->CreateDynamicMesh( pGView->MakeMeshInfo( patch.pPatch, pInfo->pDesc->waterLayers[1].materials[0] ), 0, pBound, NGScene::MakeLargeHintBound(), room );
			water2Patches.push_back( patch );
		}
	}

	// create bottom patches
	bottomPatches.resize( 0 );
	bottomPatches.reserve( pInfo->bottomPatches.size() );
	for ( int i = 0; i < pInfo->bottomPatches.size(); ++i )
	{
		SRiverPatchHolder patch;
		patch.pPatch = new CRiverPatch( &pInfo->bottomPatches[i], 0, 0 );
		patch.pHolder = pGView->CreateMesh( pGView->MakeMeshInfo( patch.pPatch, pInfo->pDesc->pBottomMaterial ), placement, 0, 0 );
		bottomPatches.push_back( patch );
	}
}

// road

void CRoad::CreatePatches( NGScene::IGameView *pGView )
{
	SFBTransform placement;
	Identity( &placement.forward );
	Identity( &placement.backward );
	//
	patches.resize( 0 );
	patches.reserve( pInfo->patches.size() );

	NGScene::SRoomInfo roomInfo( NGScene::LF_DO_NOT_MULTIPLY_ON_TRANSPARENCY, 0 );
	NGScene::SFullRoomInfo room( roomInfo, 0, 0 );

	for ( int i = 0; i < pInfo->patches.size(); ++i )
	{
		SRoadPatchHolder patch;
		patch.pPatch = new CMeshDataPatch( &pInfo->patches[i] );
		switch ( pInfo->ePart )
		{
			case SRoadGFXInfo::CENTER:
				if ( pInfo->pDesc->center.materials[0] )
					patch.pHolder = pGView->CreateMesh( pGView->MakeMeshInfo( patch.pPatch, pInfo->pDesc->center.materials[0] ), placement, 0, 0, room );
				else
					NI_ASSERT( pInfo->pDesc->center.materials[0], fmt::format( "Center material is not set for road {}", pInfo->nID ) );
				break;

			case SRoadGFXInfo::LEFT_BORDER:
				if ( pInfo->pDesc->leftBorder.pMaterial )
					patch.pHolder = pGView->CreateMesh( pGView->MakeMeshInfo( patch.pPatch, pInfo->pDesc->leftBorder.pMaterial ), placement, 0, 0, room );
				else
					NI_ASSERT( pInfo->pDesc->leftBorder.pMaterial, fmt::format( "Left border material is not set for road {}", pInfo->nID ) );
				break;

			case SRoadGFXInfo::RIGHT_BORDER:
				if ( pInfo->pDesc->rightBorder.pMaterial )
					patch.pHolder = pGView->CreateMesh( pGView->MakeMeshInfo( patch.pPatch, pInfo->pDesc->rightBorder.pMaterial ), placement, 0, 0, room );
				else
					NI_ASSERT( pInfo->pDesc->rightBorder.pMaterial, fmt::format( "Right border material is not set for road {}", pInfo->nID ) );
				break;
		}
		patches.push_back( patch );
	}
}

// terraspot

void CTerraSpot::CreatePatches( NGScene::IGameView *pGView )
{
	SFBTransform placement;
	Identity( &placement.forward );
	Identity( &placement.backward );
	//
	patch.pPatch = new CMeshDataPatch( &pInfo->data );
	patch.pHolder = pGView->CreateMesh( pGView->MakeMeshInfo( patch.pPatch, pInfo->pDesc->pMaterial ), placement, 0, 0 );
}

// terrain

CSceneTerrain::CSceneTerrain( const NDb::STerrain *_pDBDesc, NGScene::IGameView *_pGameView, CFuncBase<STime> *_pAbsTimer )
: pDBDesc( _pDBDesc ), pGameView( _pGameView ), pAbsTimer( _pAbsTimer )
{
	//patches.resize( pDBDesc->nNumPatchesX * pDBDesc->nNumPatchesY );
	patches.resize( pDBDesc->nNumPatchesX * pDBDesc->nNumPatchesY * DEF_BREAK_TERRA_PATCHES );
	terraBorders.resize( 4 );
	terraBorders[0].resize( pDBDesc->nNumPatchesX * DEF_BREAK_TERRA_PATCHES );
	terraBorders[1].resize( pDBDesc->nNumPatchesX * DEF_BREAK_TERRA_PATCHES );
	terraBorders[2].resize( pDBDesc->nNumPatchesY * DEF_BREAK_TERRA_PATCHES );
	terraBorders[3].resize( pDBDesc->nNumPatchesY * DEF_BREAK_TERRA_PATCHES );
}

void CSceneTerrain::HideTerrain( bool bHide )
{
	if ( bHide )
	{
		for ( std::vector<std::vector<STerrainPatchHolder> >::iterator itPatchesArr = patches.begin(); itPatchesArr != patches.end(); ++itPatchesArr )
		{
			for ( std::vector<STerrainPatchHolder>::iterator it = itPatchesArr->begin(); it != itPatchesArr->end(); ++it )
			{
				it->pHolder = 0;
			}
		}
	}
	else
	{
		NI_VERIFY( patches.size() > 0, "HideTerrain: no terrain patches!", return )

		SFBTransform placement;
		Identity( &placement.forward );
		Identity( &placement.backward );

		for ( std::vector<std::vector<STerrainPatchHolder> >::iterator itPatchesArr = patches.begin(); itPatchesArr != patches.end(); ++itPatchesArr )
		{
			for ( int k = 0; k < itPatchesArr->size(); ++k )
			{
				NGScene::IGameView::SMeshInfo meshInfo;

				if ( ( (*itPatchesArr)[k].pPatch != 0 ) && ( pDBDesc->pTerraSet != 0 ) && ( k < pDBDesc->pTerraSet->terraTypes.size() ) )
					(*itPatchesArr)[k].pHolder = pGameView->CreateMesh( pGameView->MakeMeshInfo( (*itPatchesArr)[k].pPatch, pDBDesc->pTerraSet->terraTypes[k]->pMaterial ), placement, 0, 0 );
			}
		}
	}
}

int CSceneTerrain::operator&( IBinSaver &saver )
{
	saver.Add( 1, &pDBDesc );
	//
	if ( saver.IsReading() ) 
	{
		patches.clear();
		terraBorders.clear();
		roads.clear();
	}
	//
	return 0;
}

REGISTER_SAVELOAD_CLASS( SCENEB2, 0x1006DC40, CSceneTerrain )

