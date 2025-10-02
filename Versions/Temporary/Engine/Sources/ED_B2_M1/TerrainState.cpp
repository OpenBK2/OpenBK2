#include "StdAfx.h"
#include "..\mapeditorlib\resourcedefines.h"
#include "..\mapeditorlib\commandhandlerdefines.h"
#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\iconsset.h"
#include "..\mapeditorlib\commoneditormethods.h"

#include "EditorScene.h"


#include "TerrainInterface.h"
#include "TerrainState.h"
#include "TerrainEditor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CTerrainState::CTerrainState( CTerrainEditor *_pTerrainEditor ) : pTerrainEditor( _pTerrainEditor )
{
}


void CTerrainState::Enter()
{
	NI_ASSERT( pTerrainEditor != 0, "CTerrainState::Enter(), pTerrainEditor == 0" );
	NI_ASSERT( !( pTerrainEditor->GetObjectSet().objectNameSet.empty() ), "CTerrainState::Enter() GetObjectSet().objectNameSet is empty" );
	IEditorScene *pScene = EditorScene();
	NI_ASSERT( pScene != 0, "CTerrainState::Enter(): pScene == 0" );
	
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_GAME_INPUT, reinterpret_cast<DWORD>( new CTerrainInterfaceCommand( new CTerrainInterface() ) ) );

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

