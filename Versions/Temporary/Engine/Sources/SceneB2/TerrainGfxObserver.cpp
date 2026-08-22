#include "stdafx.h"

#include "B2_M1_Terrain/DBTerrain.h"
#include "TerrainGfxInfo.h"
#include "Terrain.h"
#include "3Dmotor/GView.h"
#include "System/Commands.h"

static bool bDisableRivers = false;

bool HasVSO( const int nID, const std::vector<NDb::SVSOInstance> &instances )
{
	for ( std::vector<NDb::SVSOInstance>::const_iterator it = instances.begin(); it != instances.end(); ++it )
	{
		if ( it->nVSOID == nID )
			return true;
	}
	return false;
}

bool HasSpotVSO( const int nID, const std::vector<NDb::STerrainSpotInstance> &instances )
{
	for ( std::vector<NDb::STerrainSpotInstance>::const_iterator it = instances.begin(); it != instances.end(); ++it )
	{
		if ( it->nSpotID == nID )
			return true;
	}
	return false;
}

template <class TYPE>
bool RemoveVSO( const int nID, std::vector<TYPE> &vsos )
{
	for ( std::vector<TYPE>::iterator it = vsos.begin(); it != vsos.end(); ++it )
	{
		if ( it->GetID() == nID ) 
		{
			vsos.erase( it );
			return true;
		}
	}
	return false;
}

template <class TYPE>
bool UpdateVSO( const int nVSOID, std::vector<TYPE> &vsos, NGScene::IGameView *pGameView )
{
	for ( std::vector<TYPE>::iterator it = vsos.begin(); it != vsos.end(); ++it )
	{
		if ( it->GetID() == nVSOID ) 
		{
			it->CreatePatches( pGameView );
			return true;
		}
	}
	return false;
}

void CSceneTerrain::UpdatePatchGeometry( std::vector<NMeshData::SMeshData> *pMeshData, const int nPatchInd )
{
	std::vector<NMeshData::SMeshData> &meshData = *pMeshData;
	patches[nPatchInd].resize( meshData.size() );

	SFBTransform placement;
	Identity( &placement.forward );
	Identity( &placement.backward );

	NGScene::SRoomInfo roomInfo( NGScene::LF_DO_NOT_MULTIPLY_ON_TRANSPARENCY, 0 );
	NGScene::SFullRoomInfo room( roomInfo, 0, 0 );

	for ( int k = 0; k < meshData.size(); ++k )
	{
		patches[nPatchInd][k].pPatch = 0;
		patches[nPatchInd][k].pHolder = 0;
		if ( !meshData[k].triangles.empty() )
		{
			patches[nPatchInd][k].pPatch = new CMeshDataPatch( &(meshData[k]) );
			//
			if ( ( pDBDesc->pTerraSet != 0 ) && ( k < pDBDesc->pTerraSet->terraTypes.size() ) )
				patches[nPatchInd][k].pHolder = pGameView->CreateMesh( pGameView->MakeMeshInfo( patches[nPatchInd][k].pPatch, pDBDesc->pTerraSet->terraTypes[k]->pMaterial ), placement, 0, 0, room );
		}
	}
}

void CSceneTerrain::UpdateBorderGeometry( std::vector<NMeshData::SMeshData> *pMeshData, const int nBorderID )
{
	std::vector<NMeshData::SMeshData> &meshData = *pMeshData;
	terraBorders[nBorderID].resize( meshData.size() );

	SFBTransform placement;
	Identity( &placement.forward );
	Identity( &placement.backward );

	CPtr<NGScene::IMaterial> pMaterial = 0;
	//pMaterial = Singleton<IScene>()->GetGView()->CreateMaterial( CVec4( 1, 0, 0, 0.5f ) );

	NGScene::SRoomInfo roomInfo( NGScene::LF_DO_NOT_MULTIPLY_ON_TRANSPARENCY, 0 );
	NGScene::SFullRoomInfo room( roomInfo, 0, 0 );

	for ( int k = 0; k < meshData.size(); ++k )
	{
		terraBorders[nBorderID][k].pPatch = 0;
		terraBorders[nBorderID][k].pHolder = 0;
		if ( !meshData[k].triangles.empty() )
		{
			terraBorders[nBorderID][k].pPatch = new CMeshDataPatch( &(meshData[k]) );
			//
			terraBorders[nBorderID][k].pHolder = pGameView->CreateMesh( pGameView->MakeMeshInfo( terraBorders[nBorderID][k].pPatch, pDBDesc->pTerraSet->terraTypes[0]->pMaterial ), placement, 0, 0, room );
		}
	}
}

void CSceneTerrain::AddPrecipice( SPrecipiceGFXInfo *pGfxInfo )
{
	precipices.push_back( CPrecipice( pGfxInfo, pGfxInfo->nID) );
	precipices.back().CreatePatches( pGameView );
}

bool CSceneTerrain::UpdatePrecipice( const int nVSOID )
{
	return UpdateVSO( nVSOID, precipices, pGameView );
}

void CSceneTerrain::RemovePrecipice( const int nVSOID )
{
	RemoveVSO( nVSOID, precipices );
}

void CSceneTerrain::RemoveAllPrecipices()
{
	precipices.clear();
}

void CSceneTerrain::AddRoad( SRoadGFXInfo *pGfxInfo )
{
	if ( HasVSO(pGfxInfo->nID, pDBDesc->roads) ) 
	{
		roads.push_back( CRoad(pGfxInfo, pGfxInfo->nID) );
		roads.back().CreatePatches( pGameView );
	}
}

bool CSceneTerrain::UpdateRoad( const int nVSOID )
{
	bool bResult = false;
	for ( std::vector<CRoad>::iterator it = roads.begin(); it != roads.end(); ++it )
	{
		if ( it->GetID() == nVSOID ) 
		{
			it->CreatePatches( pGameView );
			bResult = true;
		}
	}
	return bResult;
	//return UpdateVSO( nVSOID, roads, pGameView );
}

void CSceneTerrain::RemoveRoad( const int nVSOID )
{
	for ( std::vector<CRoad>::iterator it = roads.begin(); it != roads.end(); )
	{
		if ( it->GetID() == nVSOID )
			it = roads.erase( it );
		else
			++it;
	}
	//RemoveVSO( nVSOID, roads );
}

void CSceneTerrain::RemoveAllRoads()
{
	roads.clear();
}

void CSceneTerrain::AddPeak( SPeakGFXInfo *pGfxInfo )
{
	peaks.push_back( CPeak(pGfxInfo, pGfxInfo->nID) );
	peaks.back().CreatePatches( pGameView, pDBDesc );
}

bool CSceneTerrain::UpdatePeak( const int nVSOID )
{
	for ( std::vector<CPeak>::iterator it = peaks.begin(); it != peaks.end(); ++it )
	{
		if ( it->GetID() == nVSOID ) 
		{
			it->CreatePatches( pGameView, pDBDesc );
			return true;
		}
	}
	return false;
}

void CSceneTerrain::RemovePeak( const int nVSOID )
{
	/*for ( vector<CPeak>::iterator it = peaks.begin(); it != peaks.end(); )
	{
		if ( it->GetID() == nVSOID )
			it = peaks.erase( it );
		else
			++it;
	}*/
	RemoveVSO( nVSOID, peaks );
}

void CSceneTerrain::RemoveAllPeaks()
{
	peaks.clear();
}

void CSceneTerrain::AddFoot( SFootGFXInfo *pGfxInfo )
{
	foots.push_back( CFoot(pGfxInfo, pGfxInfo->nID) );
	foots.back().CreatePatches( pGameView, pDBDesc );
}

bool CSceneTerrain::UpdateFoot( const int nVSOID )
{
	bool bFlag = false;
	for ( std::vector<CFoot>::iterator it = foots.begin(); it != foots.end(); ++it )
	{
		if ( it->GetID() == nVSOID ) 
		{
			it->CreatePatches( pGameView, pDBDesc );
			bFlag = true;
		}
	}
	return bFlag;
}

void CSceneTerrain::RemoveFoot( const int nVSOID )
{
	for ( std::vector<CFoot>::iterator it = foots.begin(); it != foots.end(); )
	{
		if ( it->GetID() == nVSOID )
			it = foots.erase( it );
		else
			++it;
	}
}

void CSceneTerrain::RemoveAllFoots()
{
	foots.clear();
}

void CSceneTerrain::AddRiver( const SRiverGFXInfo *pGfxInfo )
{
	if ( !bDisableRivers ) 
	{
		if ( HasVSO(pGfxInfo->nID, pDBDesc->rivers) ) 
		{
			rivers.push_back( CRiver(pGfxInfo, pGfxInfo->nID, pAbsTimer.GetPtr()) );
			rivers.back().CreatePatches( pGameView );
		}
	}
}

bool CSceneTerrain::UpdateRiver( const int nVSOID )
{
	return bDisableRivers ? true : UpdateVSO( nVSOID, rivers, pGameView );
}

void CSceneTerrain::RemoveRiver( const int nVSOID )
{
	RemoveVSO( nVSOID, rivers );
}

void CSceneTerrain::RemoveAllRivers()
{
	rivers.clear();
}

void CSceneTerrain::AddTerraSpot( STerraSpotGFXInfo *pGfxInfo )
{
	if ( HasSpotVSO(pGfxInfo->nID, pDBDesc->spots) ) 
	{
		terraspots.push_back( CTerraSpot( pGfxInfo, pGfxInfo->nID ) );
		terraspots.back().CreatePatches( pGameView );
	}
}

bool CSceneTerrain::UpdateTerraSpot( const int nVSOID )
{
	return UpdateVSO( nVSOID, terraspots, pGameView );
}

void CSceneTerrain::RemoveTerraSpot( const int nVSOID )
{
	RemoveVSO( nVSOID, terraspots );
}

void CSceneTerrain::RemoveAllTerraSpots()
{
	terraspots.clear();
}

START_REGISTER(TerrainGfxObserverCommands)
REGISTER_VAR_EX( "disable_rivers", NGlobal::VarBoolHandler, &bDisableRivers, false, STORAGE_NONE );
FINISH_REGISTER


