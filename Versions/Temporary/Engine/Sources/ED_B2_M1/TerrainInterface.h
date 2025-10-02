#pragma once

#include "EditorInterfaceBase.h"

class CTerrainInterface : public CEditorInterfaceBase
{
	OBJECT_NOCOPY_METHODS( CTerrainInterface );
};

INTERFACE_COMMAND_DECLARE( CTerrainInterfaceCommand, CTerrainInterface )


