#include "StdAfx.h"

#include "../3DMotor/GView.h"
#include "ExplosionsManager.h"

void CExplosionObjInfo::Recalc()
{
	if ( pValue == 0 )
		pValue = new NGScene::CObjectInfo();

	NGScene::CObjectInfo::SData objData;
	objData.verts = data.vertices;
	objData.geometry = data.triangles;

	pValue->AssignFast( &objData );
	data.Clear();
}

int CExplosionsManager::AddExplosion( const NMeshData::SMeshData &_data, const NDb::SMaterial *pMaterial )
{
	NI_ASSERT( pGScene, "GameView was not attached" );

	SExplosionObj explObj;
	explObj.nID = nLastID++;
	explObj.explosionHolder.pPatch = new CExplosionObjInfo( _data );

	SFBTransform placement;
	Identity( &placement.forward );
	Identity( &placement.backward );
	explObj.explosionHolder.pHolder = pGScene->CreateMesh( pGScene->MakeMeshInfo( explObj.explosionHolder.pPatch, pMaterial ), placement, 0, 0 );

	explosions.push_back( explObj );

	return explObj.nID;
}

void CExplosionsManager::RemoveExplosion( const int nID )
{
	for ( list<SExplosionObj>::iterator it = explosions.begin(); it != explosions.end(); )
	{
		if ( it->nID == nID )
			it = explosions.erase( it );
		else
			++it;
	}
}


