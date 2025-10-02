#pragma once

#include "EditorInterfaceBase.h"

class CMapInfoInterface : public CEditorInterfaceBase
{
	OBJECT_NOCOPY_METHODS( CMapInfoInterface );
	void ToggleCameraControl( const SGameMessage &msg, bool bEnableCameraControl );
	
	class CMapInfoState *pMapInfoState;
public:
	CMapInfoInterface( class CMapInfoState *_pMapInfoState = 0 ) : pMapInfoState( _pMapInfoState ) {}
};

INTERFACE_COMMAND_DECLARE( CMapInfoInterfaceCommand, CMapInfoInterface )


