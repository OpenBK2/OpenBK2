#include "StdAfx.h"

#include "DynamicDebrisManager.h"

void CVisDynamicDebrisPatch::Recalc()
{
	if ( pValue == 0 ) 
		pValue = new NGScene::CObjectInfo;

	NGScene::CObjectInfo::SData objData;
	objData.verts = data.vertices;
	objData.geometry = data.triangles;
	objData.secondTex = data.secondTex;

	pValue->AssignFast( &objData );
	data.Clear();
}

void CDynamicDebrisManager::AddDynamicDebris( const NMeshData::SMeshDataTex2 &debrisData, const NDb::SMaterial *pMaterial )
{
	debrisPatches.push_back( CVisDynamicDebrisPatchHolder() );
	CVisDynamicDebrisPatch *pCurPatch = new CVisDynamicDebrisPatch( debrisData );
	debrisPatches.back().pPatch = pCurPatch;
	if ( pGScene )
	{
		SFBTransform placement;
		Identity( &placement.forward );
		Identity( &placement.backward );
		//
		debrisPatches.back().pHolder = pGScene->CreateMesh( pGScene->MakeMeshInfo( pCurPatch, pMaterial ), placement, 0, 0 );
	}
}


