#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "Misc/2Darray.h"
#include "Stats_B2_M1/IconsSet.h"
#include "MapEditorLib/CommonEditorMethods.h"

#include "EditorScene.h"

#include "TerrainInterface.h"
#include "TerrainState.h"
#include "TerrainEditor.h"

#include <cstdint>

#include <zconf.h>

CTerrainState::CTerrainState( CTerrainEditor *_pTerrainEditor ) : pTerrainEditor( _pTerrainEditor )
{
}


void CTerrainState::Enter()
{
	NI_ASSERT( pTerrainEditor != 0, "CTerrainState::Enter(), pTerrainEditor == 0" );
	NI_ASSERT( !( pTerrainEditor->GetObjectSet().objectNameSet.empty() ), "CTerrainState::Enter() GetObjectSet().objectNameSet is empty" );
	IEditorScene *pScene = EditorScene();
	NI_ASSERT( pScene != 0, "CTerrainState::Enter(): pScene == 0" );
	
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_GAME_INPUT, reinterpret_cast<uint32_t>( new CTerrainInterfaceCommand( new CTerrainInterface() ) ) );

	// Загружаем Terrain
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, false );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_RESET_CAMERA, 0 );
	// Обновляем сцену
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
	//
	CDefaultInputState::Enter();
}


void CTerrainState::Leave()
{
	NI_ASSERT( pTerrainEditor != 0, "CTerrainState::Leave(), pTerrainEditor == 0" );
	IEditorScene *pScene = EditorScene();
	NI_ASSERT( pScene != 0, "CTerrainState::Enter(): pScene == 0" );
	//
	CDefaultInputState::Leave();
	// Выгружаем Terrain
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_CLEAR, 0 );
	// Обновляем сцену
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_DISABLE_GAME_INPUT, 0 );
}

// basement storage  


