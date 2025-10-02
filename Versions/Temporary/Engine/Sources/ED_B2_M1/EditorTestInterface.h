#pragma once

#include "EditorInterfaceBase.h"

class CEditorTestInterface : public CEditorInterfaceBase
{
	OBJECT_NOCOPY_METHODS( CEditorTestInterface );
};

INTERFACE_COMMAND_DECLARE( CEditorTestInterfaceCommand, CEditorTestInterface )


