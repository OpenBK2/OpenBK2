#if !defined(__MODEL_INTERFACE__)
#define __MODEL_INTERFACE__
#pragma once

#include "EditorInterfaceBase.h"

class CModelInterface : public CEditorInterfaceBase
{
	OBJECT_NOCOPY_METHODS( CModelInterface );
};

INTERFACE_COMMAND_DECLARE( CModelInterfaceCommand, CModelInterface )

#endif // !defined(__MODEL_INTERFACE__)

