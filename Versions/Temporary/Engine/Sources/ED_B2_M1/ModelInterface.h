#pragma once

#include "EditorInterfaceBase.h"

class CModelInterface : public CEditorInterfaceBase
{
	OBJECT_NOCOPY_METHODS( CModelInterface );
};

INTERFACE_COMMAND_DECLARE( CModelInterfaceCommand, CModelInterface )


