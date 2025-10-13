#include "stdafx.h"

#include "EditorScene.h"
#include "SceneB2/Scene.h"

#include <cstdint>

void CEditorScene::RemoveAllScreens()
{
	Scene()->RemoveAllScreens();
}

void CEditorScene::AddScreen( struct IWindow *pScreen )
{
	Scene()->AddScreen( pScreen );
}

void CEditorScene::RemoveScreen( struct IWindow *pScreen )
{
	Scene()->RemoveScreen( pScreen );
}

NGScene::I2DGameView* CEditorScene::GetG2DView()
{
	return Scene()->GetG2DView();
}

NGScene::IGameView* CEditorScene::GetGView()
{
	return Scene()->GetGView();
}

CVec2 CEditorScene::GetScreenRect()
{
	return Scene()->GetScreenRect();
}

class CCSTime* CEditorScene::GetGameTimer()
{
	return Scene()->GetGameTimer();
}

uint32_t CEditorScene::GetNormal( const CVec2 &vPoint ) const
{
	return Scene()->GetNormal( vPoint );
}

void CEditorScene::ClearMarkers( ESceneMarkerType eType, int nID )
{
	Scene()->ClearMarkers( eType, nID );
}

void CEditorScene::AddShootArea( int nID, float fStartAngle, float fEndAngle, float fMinRadius, float fMaxRadius, const CVec3 &vColor, const CVec2 &vCenter )
{
	Scene()->AddShootArea( nID, fStartAngle, fEndAngle, fMinRadius, fMaxRadius, vColor, vCenter );
}

void CEditorScene::SwitchScene( const EScene eScene )
{
	Scene()->SwitchScene ( eScene );
}

int CEditorScene::AddObject( const int nID, const NDb::SModel *pModel, const CVec3 &vPos, const CQuat &qRot, 
											const CVec3 &vScale, ESceneObjAnimMode eAnimMode, NGScene::IGameView::SMeshInfo *pMeshInfo, const bool bHasReflection )
{
	return Scene()->AddObject( nID, pModel, vPos, qRot, vScale, eAnimMode, pMeshInfo, 0, bHasReflection );
}

bool CEditorScene::MoveObject( const int nID, const CVec3 &vPos, const CQuat &qRot, const CVec3 &vScale )
{
	return Scene()->MoveObject( nID, vPos, qRot, vScale );
}

void CEditorScene::RemoveObject( const int nID )
{
	Scene()->RemoveObject( nID );
}

void CEditorScene::SetFadedObjects( const list<int> &objects )
{
	Scene()->SetFadedObjects( objects );
}

void CEditorScene::SetFadedObjects( const list<int> &objects, float fFade )
{
	Scene()->SetFadedObjects( objects, fFade );
}

int CEditorScene::AddEffect( const int nID, const NDb::SEffect *pEffect, NTimer::STime timeStart, const CVec3 &vPos, const CQuat &qRot )
{
	return Scene()->AddEffect( nID, pEffect, timeStart, vPos, qRot );
}

int CEditorScene::AddPolyline( const int nID, const vector<CVec3> &points, const CVec4 &vColor, bool bDepthCheck )
{
	return Scene()->AddPolyline( nID, points, vColor, bDepthCheck );
}

int CEditorScene::AddIndexedPolyline( const int nID, const vector<CVec3> &points, const vector<uint16_t> &indices, const CVec4 &vColor, bool bDepthCheck )
{
	return Scene()->AddIndexedPolyline( nID, points, indices, vColor, bDepthCheck );
}

void CEditorScene::RemovePolyline( const int nID )
{
	Scene()->RemovePolyline( nID );
}

bool CEditorScene::ToggleShow( ESceneShow eShow )
{
	return Scene()->ToggleShow( eShow );
}

bool CEditorScene::ToggleAIGeometryMode()
{
	return Scene()->ToggleAIGeometryMode();
}

bool CEditorScene::IsShowOn( ESceneShow eShow )
{
	return Scene()->IsShowOn( eShow  );
}

ITerraManager* CEditorScene::GetTerraManager()
{
	return Scene()->GetTerraManager();
}

bool CEditorScene::GetIntersectionWithTerrainForEditor( CVec3 *pvResult, const CVec3 &vBegin, const CVec3 &vEnd ) const
{
	return Scene()->GetIntersectionWithTerrainForEditor( pvResult, vBegin, vEnd );
}

float CEditorScene::GetZ( float x, float y ) const
{
	return Scene()->GetZ( x, y );
}

float CEditorScene::GetTileHeight( int nX, int nY ) const
{
	return Scene()->GetTileHeight( nX, nY );
}

void CEditorScene::UpdateZ( CVec3 *pvPos )
{
	return Scene()->UpdateZ( pvPos );
}

bool CEditorScene::SetupMode( ESceneMode eMode, bool bEditorMode )
{
	return Scene()->SetupMode( eMode, bEditorMode );
}

void CEditorScene::SetLight( const NDb::SAmbientLight *pLight )
{
	Scene()->SetLight( pLight );
}

void CEditorScene::SetBackgroundColor( const CVec3 &rvBackgroundColor )
{
	Scene()->SetBackgroundColor( rvBackgroundColor );
}

CVec4 CEditorScene::SetBackgroundColor( const CVec4 &rvBackgroundColor )
{
	return Scene()->SetBackgroundColor( rvBackgroundColor );
}

void CEditorScene::ClearScene( const EScene eScene2Clear )
{
	Scene()->ClearScene( eScene2Clear );
}

void CEditorScene::Draw( NGScene::CRTPtr *pTarget )
{
	Scene()->Draw( pTarget );
}

void CEditorScene::PickObjects( list<int> &pickObjects, const CVec2 &vScreenPos )
{
	Scene()->PickObjects( pickObjects, vScreenPos );
}

void CEditorScene::PickObjects( list<int> &pickObjects, const CVec2 &vScreenPos1, const CVec2 &vScreenPos2 )
{
	Scene()->PickObjects( pickObjects, vScreenPos1, vScreenPos2, IScene::PO_CENTER_INSIDE );
}

void CEditorScene::InitHeights4Editor( int nSizeX, int nSizeY )
{
	Scene()->InitHeights4Editor( nSizeX, nSizeY );
}

void CEditorScene::ShowObject( const int nID, const bool bShow )
{
	Scene()->ShowObject( nID, bShow );
}

bool CEditorScene::DoesTerraManagerExist() const
{
	return Scene()->DoesTerraManagerExist();
}

NAnimation::ISkeletonAnimator* CEditorScene::GetAnimator( const int nID )
{
	return Scene()->GetAnimator( nID );
}

bool CEditorScene::ToggleGetSizeFromTarget( bool bGetSizesFromTarget )
{
	return Scene()->ToggleGetSizeFromTarget( bGetSizesFromTarget );
}

void CEditorScene::SetWarFog( const CArray2D<unsigned char> &fog, float fScale )
{
	Scene()->SetWarFog( fog, fScale );
}


void CEditorScene::SetWarFogBlend( const float fBlend )
{
	Scene()->SetWarFogBlend( fBlend );
}


REGISTER_SAVELOAD_CLASS( 0x301CBC80, CEditorScene );

