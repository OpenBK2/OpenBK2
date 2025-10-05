#include "stdafx.h"

#include "3DLib/Transform.h"
#include "SceneInternal.h"
#include "TerrainManager.h"
#include "DBSceneConsts.h"
#include "VisObjDesc.h"
#include "VisObjSelection.h"
#include "3Dmotor/Fader.h"

const float DEFAULT_SELECTION_SIZE = 1.5f;

void CVisObjSelectionInfo::Recalc()
{
	if ( pValue == 0 ) 
		pValue = new NGScene::CObjectInfo;

	if ( eSelType == NDb::SELECTION_TYPE_GROUND )
		pTerraManager->CreateSelection( &data, info.vMin, info.vMax );

	NGScene::CObjectInfo::SData objData;
	objData.verts = data.vertices;
	objData.geometry = data.triangles;

	pValue->AssignFast( &objData );
	bUpdate = false;
}

void CVisObjSelectionInfo::SetInfo( const SSelectionInfo &_info ) 
{ 
	if ( info == _info )
		return;
	info = _info;
	ForceUpdate();
}

void CScene::SelectObject( const int nID, const CVec3 &vPos, const float fSelScale, const NDb::ESelectionType eSelType )
{
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nID );
	if ( pos == data[eScene]->visObjects.end() )
		return;
	SModelVisObjDesc *pVOD = dynamic_cast_ptr<SModelVisObjDesc*>( pos->second );
	if ( pVOD == 0 )
		return;
	
	//pos->second->selection.selHolder.pHolder = 0;
	if ( pVOD->bHidden )						//Invisible objects (i.e. infantry in containers) do not display selections
	{
		pVOD->bSelected = false;
		return;
	}

	pVOD->bSelected = true;

	SVisObjSelection &selection = pVOD->selection;	
	MakeSelection( selection, pVOD, vPos, fSelScale, eSelType, -1.0f, false );
}

void CScene::MakeSelection( SVisObjSelection &selection, SModelVisObjDesc *pVOD, const CVec3 &vPos, 
	float fSelScale, NDb::ESelectionType eSelType, float fDeltaTime, bool bOn )
{
	selection.fSelScale = fSelScale;
	selection.eSelType = eSelType;

	CVec3 vVisPos = vPos;
	AI2Vis( &vVisPos );

	if ( pVOD )
	{
		//const float fMaxSize = max( pVOD->pModel->pGeometry->vSize.x, pVOD->pModel->pGeometry->vSize.y ) * fSelScale * 0.5f;
		const float fMaxSize = sqrt( fabs2( pVOD->pModel->pGeometry->vSize.x ) +
			fabs2( pVOD->pModel->pGeometry->vSize.y ) ) * fSelScale * 0.5f;
		selection.fSelSize = fMaxSize;

		const SFBTransform &transform = pVOD->GetPlacement();
		CQuat quat;
		quat.FromEulerMatrix( transform.forward );

		selection.vSelCenter = pVOD->pModel->pGeometry->pAIGeometry->vAABBCenter;

		CVec3 vRotCent;
		quat.Rotate( &vRotCent, selection.vSelCenter );

		// centered from boundary
		vVisPos.x += vRotCent.x;
		vVisPos.y += vRotCent.y;
	}
	else
	{
		selection.fSelSize = DEFAULT_SELECTION_SIZE;
	}

	if ( selection.selHolder.pPatch == 0 )
		selection.selHolder.pPatch = new CVisObjSelectionInfo( data[eScene]->pTerraManager );
	selection.selHolder.pPatch->SetSelectionType( eSelType );

	NGScene::SFullRoomInfo room( NGScene::SRoomInfo( NGScene::LF_SKIP_LIGHTING, -100 ), 0, 0 );

	const CVec3 vSize( selection.fSelSize, selection.fSelSize, 1.0f );
	if ( eSelType == NDb::SELECTION_TYPE_GROUND )
	{
		SSelectionInfo info( vVisPos, vSize );
		SBound bound;
		info.MakeBound( &bound );
		if ( selection.pBound == 0 )
			selection.pBound = new CCSBound();
		selection.pBound->Set( bound );
		selection.selHolder.pPatch->SetInfo( info );

		const NDb::SMaterial *pSelectionMaterial = data[eScene]->pSceneConsts->selectionMaterials.pGround;
		NI_ASSERT( pSelectionMaterial != 0, "Selection material empty" );

		CDGPtr<CFuncBase<STime> > pTime = GetAbsTimer();
		pTime.Refresh();
		CPtr<NGScene::IFader> pFader = 0;
		if ( fDeltaTime >= 0.0f )
		{
			pFader = NGScene::CreateSimpleFader( NGScene::SFaderInfo( 0, !bOn, pTime->GetValue(), fDeltaTime ) );
			pFader->SetTimer( pTime );
		}

		selection.selHolder.pHolder = data[eScene]->GetGScene()->CreateDynamicMesh( 
			data[eScene]->GetGScene()->MakeMeshInfo( selection.selHolder.pPatch, data[eScene]->pSceneConsts->selectionMaterials.pGround ),
			0, selection.pBound, NGScene::MakeLargeHintBound(), room, pFader );
	}
	else
	{
		if ( pVOD )
			vVisPos.z += pVOD->pModel->pGeometry->vSize.z * 0.5f;
		SFBTransform place;
		MakeMatrix( &place, vSize, vVisPos );
		if ( selection.pTransform == 0 )
			selection.pTransform = new CCSFBTransform( place );
		else
			selection.pTransform->Set( place );
		NMeshData::SMeshData &patchData = selection.selHolder.pPatch->data;
		patchData.vertices.reserve( 4 );
		patchData.vertices.resize( 0 );
		patchData.triangles.resize( 2 );
		NGScene::SVertex vert;
		CalcCompactVector( &(vert.normal), CVec3(0, 0, 1) );
		vert.texU.dw = 0xffffffff;
		vert.texV.dw = 0xffffffff;
		//CalcCompactVector( &(vert.texU), CVec3(1, 0, 0) );
		//CalcCompactVector( &(vert.texV), CVec3(0, 1, 0) );
		vert.pos.Set( -1.0f, -1.0f, 0.0f );
		vert.tex.Set( 0.0f, 0.0f );
		patchData.vertices.push_back( vert );
		vert.pos.Set( -1.0f, 1.0f, 0.0f );
		vert.tex.Set( 0.0f, 1.0f );
		patchData.vertices.push_back( vert );
		vert.pos.Set( 1.0f, 1.0f, 0.0f );
		vert.tex.Set( 1.0f, 1.0f );
		patchData.vertices.push_back( vert );
		vert.pos.Set( 1.0f, -1.0f, 0.0f );
		vert.tex.Set( 1.0f, 0.0f );
		patchData.vertices.push_back( vert );
		patchData.triangles[0].i1 = 0;
		patchData.triangles[0].i2 = 3;
		patchData.triangles[0].i3 = 2;
		patchData.triangles[1].i1 = 2;
		patchData.triangles[1].i2 = 1;
		patchData.triangles[1].i3 = 0;

		const NDb::SMaterial *pSelectionMaterial = eSelType == NDb::SELECTION_TYPE_AIR ? 
			data[eScene]->pSceneConsts->selectionMaterials.pAir : data[eScene]->pSceneConsts->selectionMaterials.pWater;
		NI_ASSERT( pSelectionMaterial != 0, "Selection material empty" );

		selection.selHolder.pHolder = data[eScene]->GetGScene()->CreateMesh( 
			data[eScene]->GetGScene()->MakeMeshInfo( selection.selHolder.pPatch, pSelectionMaterial ),
			selection.pTransform.GetPtr(), 0, 0, room );

		selection.selHolder.pPatch->ForceUpdate();
	}
}

void CScene::UpdateSelectionHandlers()
{
	NTimer::STime timeCurrent = GetAbsTimer()->GetValue();
	
	std::vector<int> obsoletes;
	obsoletes.reserve( 100 );
	for ( SSceneData::CSelectionMap::iterator it = data[eScene]->selectionHandlers.begin(); 
		it != data[eScene]->selectionHandlers.end(); ++it )
	{
		SVisObjSelectionHandler *pHandler = it->second;

		if ( pHandler->eState == SVisObjSelectionHandler::ESS_FADE_IN )
		{
			if ( pHandler->nFadeOut >= 0 && timeCurrent - pHandler->timeStart >= pHandler->nFadeIn + pHandler->nFadeOut )
			{
				pHandler->eState = SVisObjSelectionHandler::ESS_NONE;
			}
			else if ( timeCurrent - pHandler->timeStart >= pHandler->nFadeIn )
			{
				pHandler->eState = SVisObjSelectionHandler::ESS_FADE_OUT;
				pHandler->timeStart += pHandler->nFadeIn;

				pHandler->visObjSelection.Recreate();
				MakeSelection( pHandler->visObjSelection, 0, pHandler->vPos, pHandler->fScale, 
					pHandler->eSelType, max(0.001f, pHandler->nFadeOut * 0.001f), false );
			}
		}
		
		if ( pHandler->eState == SVisObjSelectionHandler::ESS_FADE_OUT )
		{
			if ( pHandler->nFadeOut >= 0 && timeCurrent - pHandler->timeStart >= pHandler->nFadeOut )
			{
				pHandler->eState = SVisObjSelectionHandler::ESS_NONE;
			}
		}

		if ( pHandler->eState == SVisObjSelectionHandler::ESS_NONE )
			obsoletes.push_back( it->first );
	}
	
	for ( int i = 0; i < obsoletes.size(); ++i )
	{
		data[eScene]->selectionHandlers.erase( obsoletes[i] );
	}
}

void CScene::UnselectObject( const int nID )
{
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nID );
	if ( pos == data[eScene]->visObjects.end() )
		return;
	SModelVisObjDesc *pVOD = dynamic_cast_ptr<SModelVisObjDesc*>( pos->second );
	if ( pVOD == 0 )
		return;
	pVOD->bSelected = false;
	pVOD->selection.selHolder.pHolder = 0;
}

int CScene::AddSelection( int nID, const CVec3 &vPos, float fSelScale, NDb::ESelectionType eSelType, 
	float fFadeInTime, float fFadeOutTime )
{
	SModelVisObjDesc *pVOD = 0;
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nID );
	if ( pos != data[eScene]->visObjects.end() )
		pVOD = dynamic_cast_ptr<SModelVisObjDesc*>( pos->second );

	SVisObjSelectionHandler *pHandler = data[eScene]->GetNewSelection();
	pHandler->eState = SVisObjSelectionHandler::ESS_FADE_IN;
	pHandler->timeStart = GetAbsTimer()->GetValue();
	pHandler->nFadeIn = fFadeInTime < 0.0f ? -1 : fFadeInTime * 1000.0f;
	pHandler->nFadeOut = fFadeOutTime < 0.0f ? -1 : fFadeOutTime * 1000.0f;
	pHandler->vPos = vPos;
	pHandler->fScale = fSelScale;
	pHandler->eSelType = eSelType;

	MakeSelection( pHandler->visObjSelection, 0, vPos, fSelScale, eSelType, fFadeInTime, true );

	return pHandler->nID;
}

void CScene::RemoveSelection( int nID )
{
	data[eScene]->selectionHandlers.erase( nID );
}

void CScene::ClearSelection()
{
	data[eScene]->selectionHandlers.clear();
}

REGISTER_SAVELOAD_CLASS( 0x1311E300, CVisObjSelectionInfo );


