#pragma once

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

const NDb::SGameRoot *GetGameRoot();
const NDb::SGameConsts *GetGameConsts();
const NDb::SSceneConsts *GetSceneConsts();
const NDb::SClientGameConsts *GetClientConsts();
const NDb::SUIConstsB2 *GetUIConsts();
const NDb::SAIGameConsts *GetAIConsts();
const NDb::SNetGameConsts *GetNetConsts();
const NDb::SMultiplayerConsts *GetMPConsts();

}

