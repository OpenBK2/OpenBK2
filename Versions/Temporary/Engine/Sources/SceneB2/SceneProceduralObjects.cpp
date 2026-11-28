#include "stdafx.h"

#include "3Dmotor/DBScene.h"
#include "ProceduralRock.h"
#include "SceneInternal.h"
#include "Stats_B2_M1/RPGStats.h"
#include "VisObjDesc.h"

static CPtrFuncBase<NGScene::CObjectInfo> *
CreateProceduralGeometry( const NDb::SProceduralObjectRecipe *pRecipe, int nSeed, SBound *pBounds )
{
	if ( const auto *pRock = dynamic_cast<const NDb::SProceduralRockRecipe *>( pRecipe ) )
	{
		return CreateProceduralRock( pRock->vSize, pRock->fRoughness, static_cast<uint32_t>( nSeed ), pBounds );
	}
	return nullptr;
}

struct SProceduralVisObjDesc : public SStaticVisObjDesc
{
	OBJECT_NOCOPY_METHODS( SProceduralVisObjDesc );
	// Geometry is local to this placement. A save only needs the inputs to rebuild it.
	CObj<CPtrFuncBase<NGScene::CObjectInfo>> pGeometry;
	SBound bounds = {};

public:
	CDBPtr<NDb::SProceduralObjectRecipe> pRecipe;
	CDBPtr<NDb::SModel> pIntactModel;
	int nSeed = 0;

	int operator&( IBinSaver &saver ) override
	{
		saver.Add( 1, static_cast<SStaticVisObjDesc *>( this ) );
		saver.Add( 2, &pRecipe );
		saver.Add( 3, &nSeed );
		saver.Add( 4, &pIntactModel );
		return 0;
	}

	CObjectBase *CreateStaticMesh( NGScene::IGameView *pView ) override
	{
		// Damage stages supply their own models. Repairs restore the original seeded shape.
		if ( pModel != pIntactModel || !pRecipe || pModel->materials.empty() || !pModel->materials.front() )
		{
			return SStaticVisObjDesc::CreateStaticMesh( pView );
		}
		if ( !pGeometry )
		{
			pGeometry = CreateProceduralGeometry( pRecipe, nSeed, &bounds );
		}
		if ( !pGeometry )
		{
			return SStaticVisObjDesc::CreateStaticMesh( pView );
		}
		return pView->CreateMesh( pView->MakeMeshInfo( pGeometry, pModel->materials.front() ), transform, bounds, 0 );
	}

	void GetVisualBounds( CVec3 *pCenter, CVec3 *pHalfSize ) const override
	{
		if ( pGeometry && pModel == pIntactModel )
		{
			*pCenter = bounds.s.ptCenter;
			*pHalfSize = bounds.ptHalfBox;
		}
		else
		{
			SStaticVisObjDesc::GetVisualBounds( pCenter, pHalfSize );
		}
	}
};

int CScene::AddProceduralObject( int nID,
                                 const NDb::SModel *pModel,
                                 const NDb::SModel *pIntactModel,
                                 const NDb::SProceduralObjectRecipe *pRecipe,
                                 int nSeed,
                                 const CVec3 &vPos,
                                 const CQuat &qRot )
{
	if ( !pRecipe || !pModel || !pModel->pGeometry )
	{
		return -1;
	}
	const int nObjectID = GetID( nID );
	if ( CheckObjExist( nObjectID ) )
	{
		return nObjectID;
	}
	CPtr<SProceduralVisObjDesc> pDesc = new SProceduralVisObjDesc();
	pDesc->nID = nObjectID;
	pDesc->pModel = pModel;
	pDesc->pIntactModel = pIntactModel;
	pDesc->pRecipe = pRecipe;
	pDesc->nSeed = nSeed;
	CVec3 vVisPos;
	AI2Vis( &vVisPos, vPos );
	auto *pView = data[eScene]->GetGScene();
	pDesc->UpdatePlacement( pView, vVisPos, qRot, CVec3( 1, 1, 1 ) );
	pDesc->ReCreateObject(
			pView, data[eScene]->pSyncSrc, data[eScene]->pGameTimer, data[eScene]->showModes[SCENE_SHOW_BBOXES] );
	if ( !pDesc->pObj )
	{
		return -1;
	}
	data[eScene]->visObjects[nObjectID] = pDesc;
	return nObjectID;
}

REGISTER_SAVELOAD_CLASS( SCENEB2, 0x10141BC6, SProceduralVisObjDesc );
