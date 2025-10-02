#if !defined(__SQUAD_INTERFACE__)
#define __SQUAD_INTERFACE__
#pragma once

#include "EditorInterfaceBase.h"

class CSquadInterface : public CEditorInterfaceBase
{
	OBJECT_NOCOPY_METHODS( CSquadInterface );
};

INTERFACE_COMMAND_DECLARE( CSquadInterfaceCommand, CSquadInterface )

#endif // !defined(__SQUAD_INTERFACE__)
