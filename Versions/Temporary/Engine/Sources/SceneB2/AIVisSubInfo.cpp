#include "StdAfx.h"

#include "ShootAreaMesh.h"
#include "LineMesh.h"
#include "SceneInternal.h"
#include "VisObjDesc.h"

/*
NAIVisInfo::CDebugSegment* CScene::CreateSegment( const CSegment &aiSegment, const int nThickness, 
																									const NAIVisInfo::EColor eColor )
{
	NAIVisInfo::CDebugSegment *pSegment = new NAIVisInfo::CDebugSegment();
	pSegment->Init( aiSegment, nThickness );
	pSegment->InitEngineInfo( eColor );

	return pSegment;
}

NAIVisInfo::CDebugCircle* CScene::CreateCircle( const CCircle &aiCircle, 
																								const NAIVisInfo::EColor eColor )
{
	NAIVisInfo::CDebugCircle *pCircle = new NAIVisInfo::CDebugCircle();
	pCircle->Init( aiCircle );
	pCircle->InitEngineInfo( eColor );

	return pCircle;
}

NAIVisInfo::CDebugMarker* CScene::CreateMarker( const list<CVec2> &tiles, 
																								const NAIVisInfo::EColor eColor )
{
	NAIVisInfo::CDebugMarker *pMarker = new NAIVisInfo::CDebugMarker();
	pMarker->Init( tiles );
	pMarker->InitEngineInfo( eColor );

	return pMarker;
}

void CScene::SetMarkerPosition( CObjectBase *pMarker, const list<CVec2> &tiles )
{
	NI_ASSERT( dynamic_cast<NAIVisInfo::CDebugMarker*>( pMarker ) != 0, "Wrong object passed" );
	dynamic_cast<NAIVisInfo::CDebugMarker*>( pMarker )->SetPosition( tiles );
}
*/

void CScene::AddShootArea( int nID, float fStartAngle, float fEndAngle, float fMinRadius, float fMaxRadius, const CVec3 &vColor, const CVec2 &vCenter )
{
	CVec2 vCenterVis = vCenter;
	AI2Vis( &vCenterVis );
	CPtr< CPtrFuncBase<NGScene::CObjectInfo> > pGeom = new CShootAreaMesh( vCenterVis, fStartAngle, fEndAngle,
																																				 AI2Vis(fMinRadius), AI2Vis(fMaxRadius),
																																				 data[eScene]->pTerraManager );
	NGScene::IGameView::SMeshInfo meshInfo;
	meshInfo.parts.resize( 1 );
	meshInfo.parts[0].pGeometry = pGeom;
	meshInfo.parts[0].pMaterial = data[eScene]->GetGScene()->CreateMaterial( CVec4( vColor, 0.5f ), false );
	SFBTransform transform;
	transform.forward.Set( CVec3( vCenterVis.x, vCenterVis.y, 0 ), QNULL );
	transform.backward.Set( -CVec3( vCenterVis.x, vCenterVis.y, 0 ), QNULL );
	data[eScene]->markers[ESMT_SHOOT_AREA][nID].push_back( data[eScene]->GetGScene()->CreateMesh( meshInfo, transform, 0, 0 ) );	
}

void CScene::AddKeyPointArea( const CVec2 &vCenter, float fRadius, const CVec3 &vColor )
{
	CVec2 vCenterVis = vCenter;
	AI2Vis( &vCenterVis );
	CPtr< CPtrFuncBase<NGScene::CObjectInfo> > pGeom = new CShootAreaMesh( vCenterVis, 0, 0, AI2Vis(0.0f), AI2Vis(fRadius), data[eScene]->pTerraManager );

	NGScene::IGameView::SMeshInfo meshInfo;
	meshInfo.parts.resize( 1 );
	meshInfo.parts[0].pGeometry = pGeom;
	meshInfo.parts[0].pMaterial = data[eScene]->GetGScene()->CreateMaterial( CVec4( vColor, 0.5f ), false );
	SFBTransform transform;
	transform.forward.Set( CVec3( vCenterVis.x, vCenterVis.y, 0 ), QNULL );
	transform.backward.Set( -CVec3( vCenterVis.x, vCenterVis.y, 0 ), QNULL );
	data[eScene]->markers[ESMT_KEY_AREA][0].push_back( data[eScene]->GetGScene()->CreateMesh( meshInfo, transform, 0, 0 ) );	
}

void CScene::ClearKeyPointAreas()
{
	ClearMarkers( ESMT_KEY_AREA, -1 );
}

void CScene::AddLineMarker( int nID, const CVec2 &vStart, const CVec2 &vEnd, const CVec3 &vColor )
{
	CVec2 vStartVis = vStart;
	CVec2 vEndVis = vEnd;
	AI2Vis( &vStartVis );
	AI2Vis( &vEndVis );
	CPtr< CPtrFuncBase<NGScene::CObjectInfo> > pGeom = new CLineMesh( vStartVis, vEndVis, data[eScene]->pTerraManager );	
	NGScene::IGameView::SMeshInfo meshInfo;
	meshInfo.parts.resize( 1 );
	meshInfo.parts[0].pGeometry = pGeom;
	meshInfo.parts[0].pMaterial = data[eScene]->GetGScene()->CreateMaterial( CVec4( vColor, 0.5f ) );
	SFBTransform transform;
	transform.forward = SHMatrix( 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 );
	transform.backward = transform.forward;
	data[eScene]->markers[ESMT_LINE][nID].push_back( data[eScene]->GetGScene()->CreateMesh( meshInfo, transform, 0, 0 ) );	
}

void CScene::ClearMarkers( ESceneMarkerType eType, int nID )
{
	if ( nID == -1 )
		data[eScene]->markers[eType].clear();
	else
		data[eScene]->markers[eType].erase( nID );
}

CObjectBase* CScene::CreateCircle( CFuncBase<SFBTransform> *pTransform, float fRadius, const CVec3 &vColor, float fWidth )
{
	CPtr< CPtrFuncBase<NGScene::CObjectInfo> > pGeom = new CShootAreaMesh( pTransform, 0, 0, AI2Vis(0.0f), AI2Vis(fRadius), data[eScene]->pTerraManager, fWidth );

	NGScene::IGameView::SMeshInfo meshInfo;
	meshInfo.parts.resize( 1 );
	meshInfo.parts[0].pGeometry = pGeom;
	meshInfo.parts[0].pMaterial = data[eScene]->GetGScene()->CreateMaterial( CVec4( vColor, 0.5f ), false );
	return data[eScene]->GetGScene()->CreateMesh( meshInfo, pTransform, 0, 0 );
}

void CScene::SetCircle( int nID, float fRadius, const CVec3 &vColor, float fWidth )
{
	SSceneData::CVisObjectsMap::iterator pos = data[eScene]->visObjects.find( nID );
	if ( pos == data[eScene]->visObjects.end() )
		return;
	CDynamicCast<SModelVisObjDesc> pVO = data[eScene]->visObjects[nID];
	if ( !pVO )
		return;

	if ( pVO->bHidden )
	{
		pVO->pCircle = 0;
		return;
	}

	Vis2AI( &fRadius );
	if ( fRadius > 0.0f )
		pVO->pCircle = CreateCircle( pos->second->GetTransform(), fRadius, vColor, fWidth );
	else
		pVO->pCircle = 0;
}


