#include "stdafx.h"

#include "EditorTestInterface.h"
#include "MapEditorLib/CommandHandlerDefines.h"
#include "MapEditorLib/Interface_CommandHandler.h"
#include "MapEditorLib/ResourceDefines.h"
#include "SceneB2/Camera.h"

CEditorInterfaceBase::CEditorInterfaceBase()
{
	AddObserver( "camera_control_on", &CEditorInterfaceBase::ToggleCameraControl, true );
	AddObserver( "camera_control_off", &CEditorInterfaceBase::ToggleCameraControl, false );
}

void CEditorInterfaceBase::ToggleCameraControl( const SGameMessage &msg, bool bEnableCameraControl )
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_ENABLE_INPUT, !bEnableCameraControl );
}

bool CEditorInterfaceBase::ProcessEvent( const struct SGameMessage &msg )
{
	if ( ICamera *pCamera = Camera() )
	{
		pCamera->ProcessEvent( msg );
	}
	return CGameInputInterface::ProcessEvent( msg );
}

void CEditorInterfaceBase::Step( bool bAppActive )
{
	Singleton<ICommandHandlerContainer>()->HandleCommand( CHID_SCENE, ID_SCENE_UPDATE, 0 );
}
