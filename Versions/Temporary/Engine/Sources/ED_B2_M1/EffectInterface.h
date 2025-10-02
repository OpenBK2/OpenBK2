#if !defined(__EFFECT_INTERFACE__)
#define __EFFECT_INTERFACE__
#pragma once

#include "EditorInterfaceBase.h"

class CEffectInterface : public CEditorInterfaceBase
{
	OBJECT_NOCOPY_METHODS( CEffectInterface );
};

INTERFACE_COMMAND_DECLARE( CEffectInterfaceCommand, CEffectInterface )

#endif // !defined(__EFFECT_INTERFACE__)
