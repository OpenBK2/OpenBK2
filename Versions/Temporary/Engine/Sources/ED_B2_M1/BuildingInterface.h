#if !defined(__BUILDING_INTERFACE__)
#define __BUILDING_INTERFACE__
#pragma once

#include "EditorInterfaceBase.h"

class CBuildingInterface : public CEditorInterfaceBase
{
	OBJECT_NOCOPY_METHODS( CBuildingInterface );
};

INTERFACE_COMMAND_DECLARE( CBuildingInterfaceCommand, CBuildingInterface )

#endif // !defined(__BUILDING_INTERFACE__)
