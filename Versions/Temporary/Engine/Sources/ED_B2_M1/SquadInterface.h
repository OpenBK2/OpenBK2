#pragma once

#include "EditorInterfaceBase.h"

class CSquadInterface : public CEditorInterfaceBase
{
	OBJECT_NOCOPY_METHODS( CSquadInterface );
};

INTERFACE_COMMAND_DECLARE( CSquadInterfaceCommand, CSquadInterface )


