#pragma once

#include "GameX_export.h"

namespace NDb
{

struct SGameRoot;
struct SGameConsts;
struct SSceneConsts;
struct SClientGameConsts;
struct SUIConstsB2;
struct SAIGameConsts;
struct SNetGameConsts;
struct SMultiplayerConsts;

}

namespace NGameX
{

// Call once after startup configs and the executable version have been set.
GAMEX_EXPORT void InitializeMPDataVersionChecksum();

const NDb::SGameRoot *GetGameRoot();
const NDb::SGameConsts *GetGameConsts();
const NDb::SSceneConsts *GetSceneConsts();
const NDb::SClientGameConsts *GetClientConsts();
const NDb::SUIConstsB2 *GetUIConsts();
const NDb::SAIGameConsts *GetAIConsts();
const NDb::SNetGameConsts *GetNetConsts();
const NDb::SMultiplayerConsts *GetMPConsts();

}

