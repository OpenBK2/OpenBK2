#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"
//
#include "EditorTestInterface.h"
#include "EditorTestState.h"

#include <cstdint>

//

CEditorTestState::CEditorTestState( CEditorTestEditor *_pEditorTestEditor ) : pEditorTestEditor( _pEditorTestEditor )
{
}


void CEditorTestState::Enter()
{
	NI_ASSERT( pEditorTestEditor != 0, "CEditorTestState::Enter(), pEditorTestEditor == 0" );
	// Turn On Game Input
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_GAME_INPUT, reinterpret_cast<uint32_t>( new CEditorTestInterfaceCommand( new CEditorTestInterface() ) ) );
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}


void CEditorTestState::Leave()
{
	NI_ASSERT( pEditorTestEditor != 0, "CEditorTestState::Leave(), pEditorTestEditor == 0" );
	// Turn Off Game Input
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_DISABLE_GAME_INPUT, 0 );
}

// basement storage  


