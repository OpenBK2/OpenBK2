
#pragma once

#include "AILogic_export.h"

class IBinSaver;

namespace NGlobalObjects
{
AILOGIC_EXPORT void Clear();
AILOGIC_EXPORT void Serialize( int idChunk, IBinSaver &saver );
}; // namespace NGlobalObjects
