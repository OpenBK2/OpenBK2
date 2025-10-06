#include "stdafx.h"

#include "3DLib/Transform.h"
#include "3Dmotor/GMaterial.hpp"
#include "3Dmotor/GfxBuffers.h"
#include "Main/GameTimer.h"
#include "Stats_B2_M1/DBAnimB2.h"
#include "VisObjDesc.h"
#include "TerrainManager.h"
#include "SceneInternal.h"

float CalcScaleFactor( const CVec2 &vElementSize, const CVec3 &vCenter, const CVec3 &vHalfSize, const CQuat &rot )
{
	SHMatrix matrix;
	rot.DecompEulerMatrix( &matrix );
	CVec3 vRes, vMin( 1000000, 1000000, 1000000 ), vMax( -1000000, -1000000, -1000000 );
	matrix.RotateVector( &vRes, CVec3( vCenter.x + vHalfSize.x, vCenter.y + vHalfSize.y, vCenter.z + vHalfSize.z ) );
	vMin.Minimize( vRes );
	vMax.Maximize( vRes );
	matrix.RotateVector( &vRes, CVec3( vCenter.x + vHalfSize.x, vCenter.y + vHalfSize.y, vCenter.z - vHalfSize.z ) );
	vMin.Minimize( vRes );
	vMax.Maximize( vRes );
	matrix.RotateVector( &vRes, CVec3( vCenter.x + vHalfSize.x, vCenter.y - vHalfSize.y, vCenter.z + vHalfSize.z ) );
	vMin.Minimize( vRes );
	vMax.Maximize( vRes );
	matrix.RotateVector( &vRes, CVec3( vCenter.x + vHalfSize.x, vCenter.y - vHalfSize.y, vCenter.z - vHalfSize.z ) );
	vMin.Minimize( vRes );
	vMax.Maximize( vRes );
	matrix.RotateVector( &vRes, CVec3( vCenter.x - vHalfSize.x, vCenter.y + vHalfSize.y, vCenter.z + vHalfSize.z ) );
	vMin.Minimize( vRes );
	vMax.Maximize( vRes );
	matrix.RotateVector( &vRes, CVec3( vCenter.x - vHalfSize.x, vCenter.y + vHalfSize.y, vCenter.z - vHalfSize.z ) );
	vMin.Minimize( vRes );
	vMax.Maximize( vRes );
	matrix.RotateVector( &vRes, CVec3( vCenter.x - vHalfSize.x, vCenter.y - vHalfSize.y, vCenter.z + vHalfSize.z ) );
	vMin.Minimize( vRes );
	vMax.Maximize( vRes );
	matrix.RotateVector( &vRes, CVec3( vCenter.x - vHalfSize.x, vCenter.y - vHalfSize.y, vCenter.z - vHalfSize.z ) );
	vMin.Minimize( vRes );
	vMax.Maximize( vRes );
	//
	const float fMaxX = (std::max)( fabs(vMin.x), fabs(vMax.x) ) * 2.0f;
	const float fMaxY = (std::max)( fabs(vMin.y), fabs(vMax.y) ) * 2.0f;
	return (std::min)( vElementSize.x / fMaxX, vElementSize.y / fMaxY );
}

void MakeScreenTransform( SHMatrix *pMatrix, const CVec2 &vScreenPos, const CVec2 &_vElementSize, 
												 const CVec3 &vModelCenter, const CVec3 &vModelHalfSize )
{
	const float fSomeMagicNumber = 1.25f; // CRAP - some magic number
	CVec2 vElementSize = _vElementSize * fSomeMagicNumber;
	const float fScreenScaleX = INTERFACE_3D_ELEMENT_WIDTH / SCREEN_VIRTUAL_WIDTH;
	const float fScreenScaleY = INTERFACE_3D_ELEMENT_HEIGHT / SCREEN_VIRTUAL_HEIGHT;
	float x = ( vScreenPos.x - SCREEN_VIRTUAL_WIDTH/2 ) * fScreenScaleX;
	float y = ( SCREEN_VIRTUAL_HEIGHT/2 - vScreenPos.y ) * fScreenScaleY;
	CMatrixStack43<6> mStack;
	Identity( pMatrix );
	mStack.Init( *pMatrix );
	float fAngleZ = 135;//NGlobal::GetVar("angle_z", 135).GetFloat();
	float fAngleX = -45;//NGlobal::GetVar("angle_x", -45).GetFloat();
	CQuat rot = CQuat( ToRadian(fAngleX), V3_AXIS_X ) * CQuat( ToRadian(fAngleZ), V3_AXIS_Z );
	mStack.Push( CVec3(x, y, 0), rot );
	const float fScaleFactor = CalcScaleFactor( CVec2(vElementSize.x * fScreenScaleX, vElementSize.y * fScreenScaleY), 
																							vModelCenter, vModelHalfSize, rot );
	mStack.PushScale( fScaleFactor );
	mStack.Push( CVec3(-vModelCenter.x, -vModelCenter.y, 0) );
	*pMatrix = mStack.Get();
}

void CVisObjReflectionInfo::Recalc()
{
	if ( !( pGeometry->HasZeroValue() ) )
	{
		if ( pValue == 0 )
			pValue = new NGScene::CObjectInfo;

		const std::vector<CVec3> &positions = pGeometry->GetValue()->GetPositions();
		const std::vector<NGScene::SUVInfo> &vertices = pGeometry->GetValue()->GetVertices();
		const std::vector<WORD> &indices = pGeometry->GetValue()->GetPositionIndices();
		data.triangles = pGeometry->GetValue()->GetGeometry();

		CVec3 v;
		data.vertices.resize( vertices.size() );
		for ( int g = 0; g < vertices.size(); ++g )
		{
			matr.RotateVector( &v, positions[indices[g]] );
			v += matr.GetTranslation();

			data.vertices[g].pos = v;
			data.vertices[g].normal = vertices[g].normal;
			data.vertices[g].texU = vertices[g].texU;
			data.vertices[g].texV = vertices[g].texV;
			data.vertices[g].tex = NGfx::GetTexCoords( vertices[g].tex );
		}

		pTerraManager->ClampUnderRivers( &data );

		NGScene::CObjectInfo::SData objData;
		objData.verts = data.vertices;
		objData.geometry = data.triangles;
		pValue->Assign( &objData, true );

		bWasUpdate = true;
	}
}

void CScene::AddReflectionFromObject( const NGScene::IGameView::SMeshInfo *pMeshInfo, const NDb::SModel *pModel,
																			const SHMatrix &matr )
{
	for ( int i = 0; i < pMeshInfo->parts.size(); ++i )
	{
		data[eScene]->visObjReflections.push_back( CVisObjReflectionInfoHolder() );
		const int nModelMaterial = min( i, pModel->materials.size() - 1 );

		data[eScene]->visObjReflections.back().pPatch = new CVisObjReflectionInfo( data[eScene]->pTerraManager );

		NGScene::SMaterialCreateInfo matInfo;
		data[eScene]->GetGScene()->CreateMaterialInfo( pModel->materials[nModelMaterial], &matInfo );
		matInfo.nPriority = 5;
		matInfo.alphaMode = NGScene::MF_GENERIC | NGScene::MF_OVERLAY;
		matInfo.bDoesCastShadow = false;
		data[eScene]->visObjReflections.back().pPatch->pMaterial = CreateMaterial( matInfo );

		data[eScene]->visObjReflections.back().pPatch->SetGeometry( pMeshInfo->parts[i].pGeometry );
		data[eScene]->visObjReflections.back().pPatch->SetTransform( matr );

		NGScene::IGameView::SMeshInfo meshInfo;
		meshInfo.parts.push_back( NGScene::IGameView::SPartInfo( data[eScene]->visObjReflections.back().pPatch,
			data[eScene]->visObjReflections.back().pPatch->pMaterial ) );

		SFBTransform placement;
		Identity( &placement.forward );
		Identity( &placement.backward );

		data[eScene]->visObjReflections.back().pHolder = data[eScene]->GetGScene()->CreateMesh( meshInfo, placement, 0, 0 );
	}
}

bool CScene::CheckObjExist( int nObjectID ) const
{
	//проверка на повторное создание объектов только для FastDebug
#if !defined(_BETARELEASE) && !defined(_FINALRELEASE)
	if ( data[eScene]->visObjects.find(nObjectID) == data[eScene]->visObjects.end() )
		return false;
	DebugTrace( StrFmt( "Object 0x%.8x already exist", nObjectID ) );
	return true;
#else
	return data[eScene]->visObjects.find(nObjectID) != data[eScene]->visObjects.end();
#endif
}

void CScene::AddInterfaceObject( IWindow *pScreen, int nID, const NDb::SModel *pModel, const CVec2 &vScreenPos, const CVec2 &vElementSize )
{
	SSceneData::CScreensData &screensData = data[eScene]->screensData;
	SSceneData::CScreensData::iterator it = find( screensData.begin(), screensData.end(), pScreen );
	if ( it != screensData.end() )
	{
		SSceneData::SScreenData &screenData = *it;

		const int nObjectID = nID;//GetID( nID );
		CPtr<SAnimatedVisObjDescBase> pVOD = new SAnimatedStaticVisObjDesc(); 
		// create animator
		NI_VERIFY( pModel->pSkeleton != 0, StrFmt( "Model \"%s\" doesn't have skeleton", pModel->GetDBID().ToString().c_str() ), return );
		pVOD->pAnimator = NAnimation::CreateSkeletonAnimator( NAnimation::SGrannySkeletonHandle(pModel->pSkeleton, 0), data[eScene]->pAbsTimer );
		// check for successful animator
		if ( pVOD->pAnimator == 0 )
			return;
		//
		pVOD->pTransform = new CCSFBTransform;
		pVOD->pAnimator->SetGlobalTransform( pVOD->pTransform );
		// set placement
		CVec3 vAABBCenter = pModel->pGeometry->vCenter, vAABBHalfSize = pModel->pGeometry->vSize*0.5f;
		if ( pModel->pGeometry->pAIGeometry )
		{
			vAABBCenter = pModel->pGeometry->pAIGeometry->vAABBCenter;
			vAABBHalfSize = pModel->pGeometry->pAIGeometry->vAABBHalfSize;
	//		AI2Vis( &vAABBCenter, pModel->pGeometry->pAIGeometry->vAABBCenter );
	//		AI2Vis( &vAABBHalfSize, pModel->pGeometry->pAIGeometry->vAABBHalfSize );
		}
		SHMatrix matTransform;
		MakeScreenTransform( &matTransform, vScreenPos, vElementSize, vAABBCenter, vAABBHalfSize );
		pVOD->UpdatePlacement( data[eScene]->GetGScene(), matTransform );
		//
		const int nNumMeshes = pModel->pGeometry->nNumMeshes;
		const int nMeshIndexInFile = 0;

		NGScene::IGameView::SMeshInfo meshInfoLocal;
		
//		data[eScene]->pInterfaceView->CreateMeshInfo( pModel, &meshInfoLocal, true );
		screenData.p3DView->CreateMeshInfo( pModel, &meshInfoLocal, true );
		//
//		if ( CObjectBase *pObj = pVOD->CreateAnimatedMesh(data[eScene]->pInterfaceView, pModel, &meshInfoLocal) )
		if ( CObjectBase *pObj = pVOD->CreateAnimatedMesh( screenData.p3DView, pModel, &meshInfoLocal ) )
		{
			screenData.interfaceVisObjects[nObjectID] = pVOD;
			//
			pVOD->pObj = pObj;
			pVOD->pModel = pModel;
			pVOD->nID = nObjectID;
		}
	}
	return;
}

void CScene::RemoveInterfaceObject( IWindow *pScreen, const int nID )
{
	SSceneData::CScreensData &screensData = data[eScene]->screensData;
	SSceneData::CScreensData::iterator it = find( screensData.begin(), screensData.end(), pScreen );
	if ( it != screensData.end() )
		it->interfaceVisObjects.erase( nID );
}

int CScene::AddObject( const int nID, const NDb::SModel *pModel, const CVec3 &_vPos, const CQuat &qRot, const CVec3 &vScale, 
											 ESceneObjAnimMode eAnimMode, NGScene::IGameView::SMeshInfo *pMeshInfo, const NDb::SModel *pLowLevelModel, const bool bHasReflection )
{
	NI_VERIFY( pModel != 0, StrFmt("Adding object %d with empty model", nID), return -1 );
	const bool bAnimated = ( eAnimMode == OBJ_ANIM_MODE_FORCE_ANIMATED ) || 
												 ( eAnimMode == OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC ) ||
												 ( eAnimMode == OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC_ABS ) ||
												 ( pModel != 0 && pModel->pSkeleton != 0 && !pModel->pSkeleton->animations.empty() ) &&
												 ( eAnimMode != OBJ_ANIM_MODE_FORCE_NON_ANIMATED );
	if ( bAnimated ) 
		return AddAnimatedObject( nID, pModel, _vPos, qRot, vScale, eAnimMode, pMeshInfo, pLowLevelModel, bHasReflection );
	else
		return AddStaticObject( nID, pModel, _vPos, qRot, vScale, pMeshInfo, bHasReflection );
}

int CScene::AddStaticObject( const int nID, const NDb::SModel *pModel, const CVec3 &_vPos, const CQuat &qRot, 
														const CVec3 &vScale, NGScene::IGameView::SMeshInfo *pMeshInfoPassed, const bool bHasReflection )
{
	NI_VERIFY( pModel != 0, StrFmt("Adding object %d with empty model", nID), return -1 );
	
	data[eScene]->GetGScene()->Precache( pModel );
	//data[eScene]->GetGScene()->WaitForLoad();

	// check for object exist
	const int nObjectID = GetID( nID );
	if ( CheckObjExist(nObjectID) )
		return nObjectID;
	// make placement
	CPtr<SStaticVisObjDesc> pVOD = new SStaticVisObjDesc();
	CVec3 vPos;
	AI2Vis( &vPos, _vPos );
	pVOD->UpdatePlacement( data[eScene]->GetGScene(), vPos, qRot, vScale );
	//
	NGScene::IGameView::SMeshInfo meshInfoLocal;
	NGScene::IGameView::SMeshInfo *pMeshInfo = pMeshInfoPassed != 0 ? pMeshInfoPassed : &meshInfoLocal;
	data[eScene]->GetGScene()->CreateMeshInfo( pModel, pMeshInfo, false );

	if ( bHasReflection )
	{
		SHMatrix matr( vPos, qRot );
		matr._11 = vScale.x; matr._22 = vScale.y; matr._33 = vScale.z;
		AddReflectionFromObject( pMeshInfo, pModel, matr );
	}

	if ( CObjectBase *pObj = data[eScene]->GetGScene()->CreateMesh( *pMeshInfo, pVOD->transform, 0, 0 ) )
	{
		data[eScene]->visObjects[nObjectID] = pVOD;
		//
		pVOD->pObj = pObj;
		pVOD->pModel = pModel;
		pVOD->nID = nObjectID;
		if ( pModel->pGeometry->pAIGeometry ) 
		{
			pVOD->srcBind.Link( data[eScene]->pSyncSrc, pVOD );
			pVOD->srcBind.Update();
		}

		return nObjectID;
	}

	return -1;
}

int CScene::AddAnimatedObject( const int nID, const NDb::SModel *pModel, const CVec3 &_vPos, const CQuat &qRot, const CVec3 &vScale, 
															 ESceneObjAnimMode eAnimMode, NGScene::IGameView::SMeshInfo *pMeshInfoPassed, const NDb::SModel *pLowLevelModel, const bool bHasReflection )
{
//	AddInterfaceObject( nID, pModel, CVec2(NGlobal::GetVar("pos_x", 400), NGlobal::GetVar("pos_y", 700)), CVec2(128, 128) );
	NI_VERIFY( pModel != 0, StrFmt("Adding object %d with empty model", nID), return -1 );

	data[eScene]->GetGScene()->Precache( pModel );
	//data[eScene]->GetGScene()->WaitForLoad();

	// check for object exist
	const int nObjectID = GetID( nID );
	if ( CheckObjExist(nObjectID) )
		return nObjectID;
	// 
	CPtr<SAnimatedVisObjDescBase> pVOD; 
	if ( eAnimMode == OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC || eAnimMode == OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC_ABS ) 
		pVOD = new SAnimatedStaticVisObjDesc();
	else
		pVOD = new SAnimatedVisObjDesc();
	// create animator
	NAnimation::SGrannySkeletonHandle handle;
	NI_VERIFY( pModel->pSkeleton != 0, StrFmt( "Model \"%s\" doesn't have skeleton", pModel->GetDBID().ToString().c_str() ), return -1 );
	handle.pSkeleton = pModel->pSkeleton;
	handle.nModelInFile = 0;
	if ( eAnimMode == OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC_ABS )
		pVOD->pAnimator = NAnimation::CreateSkeletonAnimator( handle, data[eScene]->pAbsTimer );
	else
		pVOD->pAnimator = NAnimation::CreateSkeletonAnimator( handle, data[eScene]->pGameTimer );
	// check for successful animator
	if ( pVOD->pAnimator == 0 )
		return -1;
	//
	pVOD->pTransform = new CCSFBTransform;
	pVOD->pAnimator->SetGlobalTransform( pVOD->pTransform );
	// set placement
	CVec3 vPos;
	AI2Vis( &vPos, _vPos );
	pVOD->UpdatePlacement( data[eScene]->GetGScene(), vPos, qRot, vScale );
	//

	const int nNumMeshes = pModel->pGeometry->nNumMeshes;
	const int nMeshIndexInFile = 0;

	NGScene::IGameView::SMeshInfo meshInfoLocal;
	NGScene::IGameView::SMeshInfo *pMeshInfo = pMeshInfoPassed != 0 ? pMeshInfoPassed : &meshInfoLocal;

	data[eScene]->GetGScene()->CreateMeshInfo( pModel, pMeshInfo, true );

	if ( bHasReflection )
	{
		SHMatrix matr( vPos, qRot );
		matr._11 = vScale.x; matr._22 = vScale.y; matr._33 = vScale.z;
		AddReflectionFromObject( pMeshInfo, pModel, matr );
	}

	if ( CObjectBase *pObj = pVOD->CreateAnimatedMesh(data[eScene]->GetGScene(), pModel, pMeshInfo) )
	{
		data[eScene]->visObjects[nObjectID] = pVOD;
		
		pVOD->pObj = pObj;
		pVOD->pModel = pModel;
		pVOD->pLowLevelModel = pLowLevelModel;

		pVOD->nID = nObjectID;
		if ( pModel->pGeometry->pAIGeometry ) 
		{
			pVOD->srcBind.Link( data[eScene]->pSyncSrc, pVOD );
			pVOD->srcBind.Update();
		}

		return nObjectID;
	}

	return -1;
}

// AddObject versions with precomputed transform
int CScene::AddObject( const int nID, const NDb::SModel *pModel, const SHMatrix &mPlace, 
											 ESceneObjAnimMode eAnimMode, NGScene::IGameView::SMeshInfo *pMeshInfo, const bool bHasReflection )
{
	const bool bAnimated = ( eAnimMode == OBJ_ANIM_MODE_FORCE_ANIMATED ) || 
												 ( eAnimMode == OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC ) ||
												 ( eAnimMode == OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC_ABS ) ||
												 ( pModel != 0 && pModel->pSkeleton != 0 && !pModel->pSkeleton->animations.empty() ) &&
												 ( eAnimMode != OBJ_ANIM_MODE_FORCE_NON_ANIMATED );
	if ( bAnimated ) 
		return AddAnimatedObject( nID, pModel, mPlace, eAnimMode, pMeshInfo, bHasReflection );
	else
		return AddStaticObject( nID, pModel, mPlace, pMeshInfo, bHasReflection );
}

int CScene::AddStaticObject( const int nID, const NDb::SModel *pModel, const SHMatrix &mPlace, 
														NGScene::IGameView::SMeshInfo *pMeshInfoPassed, const bool bHasReflection )
{
	if ( pModel == 0 )
		return -1;

	data[eScene]->GetGScene()->Precache( pModel );
	//data[eScene]->GetGScene()->WaitForLoad();

	// check for object exist
	const int nObjectID = GetID( nID );
	if ( CheckObjExist(nObjectID) )
		return nObjectID;
	// make placement
	CPtr<SDynamicVisObjDesc> pVOD = new SDynamicVisObjDesc();
	pVOD->UpdatePlacement( data[eScene]->GetGScene(), mPlace );
	//
	NGScene::IGameView::SMeshInfo meshInfoLocal;
	NGScene::IGameView::SMeshInfo *pMeshInfo = pMeshInfoPassed != 0 ? pMeshInfoPassed : &meshInfoLocal;
	data[eScene]->GetGScene()->CreateMeshInfo( pModel, pMeshInfo, false );

	if ( bHasReflection )
		AddReflectionFromObject( pMeshInfo, pModel, mPlace );

	if ( CObjectBase *pObj = data[eScene]->GetGScene()->CreateMesh( *pMeshInfo, pVOD->pTransform.GetPtr(), 0, 0 ) )
	{
		data[eScene]->visObjects[nObjectID] = pVOD;
		//
		pVOD->pObj = pObj;
		pVOD->pModel = pModel;
		pVOD->nID = nObjectID;
		if ( pModel->pGeometry->pAIGeometry ) 
		{
			pVOD->srcBind.Link( data[eScene]->pSyncSrc, pVOD );
			pVOD->srcBind.Update();
		}
		return nObjectID;
	}
	return -1;
}

int CScene::AddAnimatedObject( const int nID, const NDb::SModel *pModel, const SHMatrix &mPlace, 
															 ESceneObjAnimMode eAnimMode, NGScene::IGameView::SMeshInfo *pMeshInfoPassed, const bool bHasReflection )
{
	// pModel = NDb::Get<NDb::SModel>( 253 );
	if ( pModel == 0 )
		return -1;

	
	data[eScene]->GetGScene()->Precache( pModel );
	//data[eScene]->GetGScene()->WaitForLoad();

	// check for object exist
	const int nObjectID = GetID( nID );
	if ( CheckObjExist(nObjectID) )
		return nObjectID;
	// 
	CPtr<SAnimatedVisObjDescBase> pVOD; 
	if ( eAnimMode == OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC || eAnimMode == OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC_ABS ) 
		pVOD = new SAnimatedStaticVisObjDesc();
	else
		pVOD = new SAnimatedVisObjDesc();
	// create animator
	NAnimation::SGrannySkeletonHandle handle;
	handle.pSkeleton = pModel->pSkeleton;
	handle.nModelInFile = 0;
	if ( eAnimMode == OBJ_ANIM_MODE_FORCE_ANIMATED_STATIC_ABS )
		pVOD->pAnimator = NAnimation::CreateSkeletonAnimator( handle, data[eScene]->pAbsTimer );
	else
		pVOD->pAnimator = NAnimation::CreateSkeletonAnimator( handle, data[eScene]->pGameTimer );
	// check for successfull skeleton animator
	if ( pVOD->pAnimator == 0 )
		return -1;
	//
	pVOD->pTransform = new CCSFBTransform;
	pVOD->pAnimator->SetGlobalTransform( pVOD->pTransform );
	// set placement
	pVOD->UpdatePlacement( data[eScene]->GetGScene(), mPlace );
	//
	const int nNumMeshes = pModel->pGeometry->nNumMeshes;
	const int nMeshIndexInFile = 0;

	NGScene::IGameView::SMeshInfo meshInfoLocal;
	NGScene::IGameView::SMeshInfo *pMeshInfo = pMeshInfoPassed != 0 ? pMeshInfoPassed : &meshInfoLocal;

	data[eScene]->GetGScene()->CreateMeshInfo( pModel, pMeshInfo, true );

	if ( bHasReflection )
		AddReflectionFromObject( pMeshInfo, pModel, mPlace );

	if ( CObjectBase *pObj = data[eScene]->GetGScene()->CreateMesh( *pMeshInfo, pVOD->pTransform.GetPtr(), 0, NGScene::CMeshAnimStuff( pModel, pVOD->pAnimator ) ) )
	{
		data[eScene]->visObjects[nObjectID] = pVOD;
		//
		pVOD->pObj = pObj;
		pVOD->pModel = pModel;
		pVOD->nID = nObjectID;
		if ( pModel->pGeometry->pAIGeometry ) 
		{
			pVOD->srcBind.Link( data[eScene]->pSyncSrc, pVOD );
			pVOD->srcBind.Update();
		}
		return nObjectID;
	}

	return -1;
}

void CScene::RemoveObject( const int nID )
{
	data[eScene]->visObjects.erase( nID );
}

bool CScene::MoveObject( const int nID, const CVec3 &_vPos, const CQuat &qRot, const CVec3 &vScale )
{
	//	NI_ASSERT( visObjects.find(nID) != visObjects.end(), StrFmt("Can't find object 0x%.8x to move", nID) );
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nID );
	if ( pos == data[eScene]->visObjects.end() )
		return false;
	CVec3 vPos;
	AI2Vis( &vPos, _vPos );
	pos->second->UpdatePlacement( data[eScene]->GetGScene(), vPos, qRot, vScale );
	pos->second->UpdateSrcBind();
	pos->second->UpdateStuff( data[eScene]->pVisObjIconsManager );

	return true;
}

bool CScene::MoveObject( const int nID, const SHMatrix &mPlace )
{
	//	NI_ASSERT( visObjects.find(nID) != visObjects.end(), StrFmt("Can't find object 0x%.8x to move", nID) );
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nID );
	if ( pos == data[eScene]->visObjects.end() )
		return false;

	pos->second->UpdatePlacement( data[eScene]->GetGScene(), mPlace );
	pos->second->UpdateSrcBind();
	pos->second->UpdateStuff( data[eScene]->pVisObjIconsManager );

	return true;
}

bool CScene::ChangeModel( const int nObjectID, const NDb::SModel *pModel )
{
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nObjectID );
	if ( pos == data[eScene]->visObjects.end() ) 
		return false;

	if ( pModel )
	{
		data[eScene]->GetGScene()->Precache( pModel );
		//data[eScene]->GetGScene()->WaitForLoad();
	}

	pos->second->ChangeModel( pModel, data[eScene]->pGameTimer, data[eScene]->GetGScene(), 
														data[eScene]->pSyncSrc, data[eScene]->showModes[SCENE_SHOW_BBOXES] );
	if ( SAnimatedVisObjDescBase *pVO = dynamic_cast_ptr<SAnimatedVisObjDescBase*>( pos->second ) )
	{
		NTimer::STime currTime = Singleton<IGameTimer>()->GetGameTime();
		for ( SModelVisObjDesc::CAttaches::iterator it = pVO->attachedObjects.begin(); it != pVO->attachedObjects.end(); ++it )
			it->second.clear();
	}
	return true;
}

void CScene::RemoveObjectPickability( const int nID )
{
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nID );
	if ( pos == data[eScene]->visObjects.end() ) 
		return;
	if ( SModelVisObjDesc *pVOD = dynamic_cast_ptr<SModelVisObjDesc *>(pos->second) )
	{
		pVOD->srcBind.Unlink();
		pVOD->srcBind.Update();
	}
}

void CScene::ShowObject( const int nID, const bool bShow )
{
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nID );
	if ( pos == data[eScene]->visObjects.end() )
		return;

	pos->second->bHidden = !bShow;
	if ( bShow )
	{
		pos->second->ReCreateObject( data[eScene]->GetGScene(), data[eScene]->pSyncSrc, data[eScene]->pGameTimer, data[eScene]->showModes[SCENE_SHOW_BBOXES] );
	}
	else
	{
		pos->second->ClearObject();
		UnselectObject( nID );
	}
}

NAnimation::ISkeletonAnimator *CScene::GetAnimator( const int nID, bool bRefreshAnimator )
{
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nID );
	return pos == data[eScene]->visObjects.end() ? 0 : pos->second->GetAnimator( bRefreshAnimator );
}
NAnimation::ISkeletonAnimator *CScene::GetInterfaceObjAnimator( IWindow *pScreen, const int nID )
{
	SSceneData::CScreensData &screensData = data[eScene]->screensData;
	SSceneData::CScreensData::iterator it = find( screensData.begin(), screensData.end(), pScreen );
	if ( it != screensData.end() )
	{
		SSceneData::CVisObjectsMap::iterator pos = it->interfaceVisObjects.find( nID );
		return pos == it->interfaceVisObjects.end() ? 0 : pos->second->GetAnimator( true );
	}
	return 0;
}

REGISTER_SAVELOAD_CLASS( 0x13129440, CVisObjReflectionInfo );


