#pragma once

#include "..\ED_B2_M1\EditorInterfaceBase.h"

class CChapterInterface : public CEditorInterfaceBase
{
	OBJECT_NOCOPY_METHODS( CChapterInterface );
};

INTERFACE_COMMAND_DECLARE( CChapterInterfaceCommand, CChapterInterface )

