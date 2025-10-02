#if !defined(__TERRAIN_INTERFACE__)
#define __TERRAIN_INTERFACE__
#pragma once

#include "EditorInterfaceBase.h"

class CTerrainInterface : public CEditorInterfaceBase
{
	OBJECT_NOCOPY_METHODS( CTerrainInterface );
};

INTERFACE_COMMAND_DECLARE( CTerrainInterfaceCommand, CTerrainInterface )

#endif // !defined(__TERRAIN_INTERFACE__)
