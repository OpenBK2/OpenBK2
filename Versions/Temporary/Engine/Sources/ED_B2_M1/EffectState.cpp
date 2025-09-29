#include "StdAfx.h"
#include "..\mapeditorlib\resourcedefines.h"
#include "..\mapeditorlib\commandhandlerdefines.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\iconsset.h"
#include "..\mapeditorlib\interface_commandhandler.h"

#include "EditorScene.h"
#include "..\SceneB2\Camera.h"
#include "..\3DMotor\DBScene.h"
#include "..\Main\GameTimer.h"


#include "EffectInterface.h"
#include "EffectState.h"
#include "EffectEditor.h"

#include "Tools_SceneGeometry.h"
#include "../Stats_B2_M1/Vis2AI.h"

#include "..\System\GResource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CEffectState::CEffectState( CEffectEditor *_pEffectEditor ) : pEffectEditor( _pEffectEditor )
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CEffectState::Enter()
{
	NI_ASSERT( pEffectEditor != 0, "CEffectState::Enter(), pEffectEditor == 0" );
	NI_ASSERT( !( pEffectEditor->GetObjectSet().objectNameSet.empty() ), "CEffectState::Enter() GetObjectSet().objectNameSet is empty" );
	IEditorScene *pScene = EditorScene();
	NI_ASSERT( pScene != 0, "CEffectState::Enter(): pScene == 0" );

	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_GAME_INPUT, reinterpret_cast<DWORD>( new CEffectInterfaceCommand( new CEffectInterface() ) ) );

	const CVec3 vShift = CVec3( 8.0f * AI_TILE_SIZE, 8.0f * AI_TILE_SIZE, 0.0f );
	CVec3 vCameraPos = Camera()->GetAnchor();
	Vis2AI( &vCameraPos );
	//
	CVec3 vEffectPos = VNULL3;
	int nEffectID = INVALID_NODE_ID;
	effectIDList.clear();
	ClearHoldQueue();
	if ( const NDb::SEffect *pEffect = NDb::Get<NDb::SEffect>( pEffectEditor->GetObjectSet().objectNameSet.begin()->first ) )
	{
		vEffectPos = vCameraPos - vShift;
		vEffectPos.z = GetTerrainHeight( vEffectPos.x, vEffectPos.y );
		nEffectID = pScene->AddEffect( 1, pEffect, Singleton<IGameTimer>()->GetGameTime(), vEffectPos, CQuat( -FP_PI2, V3_AXIS_Z ) );
		if ( nEffectID != INVALID_NODE_ID )
		{
			effectIDList.push_back( nEffectID );
		}
		//
		vEffectPos = vCameraPos;
		vEffectPos.z = GetTerrainHeight( vEffectPos.x, vEffectPos.y );
		nEffectID = pScene->AddEffect( 2, pEffect, Singleton<IGameTimer>()->GetGameTime(), vEffectPos, CQuat( 0.0f, V3_AXIS_Z ) );
		if ( nEffectID != INVALID_NODE_ID )
		{
			effectIDList.push_back( nEffectID );
		}
		//
		vEffectPos = vCameraPos + vShift;
		vEffectPos.z = GetTerrainHeight( vEffectPos.x, vEffectPos.y );
		nEffectID = pScene->AddEffect( 3, pEffect, Singleton<IGameTimer>()->GetGameTime(), vEffectPos, CQuat( FP_PI2, V3_AXIS_Z ) );
		if ( nEffectID != INVALID_NODE_ID )
		{
			effectIDList.push_back( nEffectID );
		}
	}
	// Обновляем сцену
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	//
	CDefaultInputState::Enter();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CEffectState::Leave()
{
	NI_ASSERT( pEffectEditor != 0, "CEffectState::Leave(), pEffectEditor == 0" );
	IEditorScene *pScene = EditorScene();
	NI_ASSERT( pScene != 0, "CEffectState::Enter(): pScene == 0" );
	//
	CDefaultInputState::Leave();
	// Выгружаем Эффект
	for ( list<int>::const_iterator itEffectID = effectIDList.begin(); itEffectID != effectIDList.end(); ++itEffectID )
	{
		pScene->RemoveObject( *itEffectID );
	}
	effectIDList.clear();
	// Обновляем сцену
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_DISABLE_GAME_INPUT, 0 );
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// basement storage  
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
