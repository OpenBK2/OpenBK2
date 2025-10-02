#if !defined(__EDITOR_TEST_INTERFACE__)
#define __EDITOR_TEST_INTERFACE__
#pragma once

#include "EditorInterfaceBase.h"

class CEditorTestInterface : public CEditorInterfaceBase
{
	OBJECT_NOCOPY_METHODS( CEditorTestInterface );
};

INTERFACE_COMMAND_DECLARE( CEditorTestInterfaceCommand, CEditorTestInterface )

#endif // !defined(__EDITOR_TEST_INTERFACE__)

