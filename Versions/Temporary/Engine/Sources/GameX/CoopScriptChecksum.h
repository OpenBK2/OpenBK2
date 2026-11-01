#pragma once

#include "GameX_export.h"

#include <cstdint>

namespace NDb
{
struct SMapInfo;
struct SAIGameConsts;
} // namespace NDb

namespace NGameX
{
// Hash the VFS script sources in CScripts::Load order. Callers cache this per match attempt.
GAMEX_EXPORT uint32_t CalculateCoopScriptChecksum( const NDb::SMapInfo *pMap, const NDb::SAIGameConsts *pAIConsts );
} // namespace NGameX
