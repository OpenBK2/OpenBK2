#include "stdafx.h"
#include "MapInfoInterface.h"
#include "MapInfoState.h"

void CMapInfoInterface::ToggleCameraControl( const SGameMessage &msg, bool bEnableCameraControl )
{
	CEditorInterfaceBase::ToggleCameraControl( msg, bEnableCameraControl );
	if ( ( pMapInfoState != 0 ) && ( !bEnableCameraControl ) )
	{
		pMapInfoState->CancelSelection();
	}
}

