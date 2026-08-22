#include "stdafx.h"
#include "MapEditorLib/ResourceDefines.h"
#include "MapEditorLib/CommandHandlerDefines.h"

#include "EditorTestInterface.h"
#include "SceneB2/Camera.h"
#include "MapEditorLib/Interface_CommandHandler.h"


CEditorInterfaceBase::CEditorInterfaceBase()
{
	AddObserver( "camera_control_on", ToggleCameraControl, true );
	AddObserver( "camera_control_off", ToggleCameraControl, false );
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


