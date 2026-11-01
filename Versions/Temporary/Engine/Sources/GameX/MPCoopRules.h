#pragma once

#include "GameX_export.h"

#include <string>
#include <vector>

namespace NDb
{
struct SMultiplayerConsts;
}

struct SB2GameSpecificData;
struct SMPSlot;

namespace NGameX
{
// Lobby indices remain map player IDs. AI and neutral holes never become network clients.
GAMEX_EXPORT bool IsCoopGame( const SB2GameSpecificData &game );
GAMEX_EXPORT int GetMultiplayerSlotCount( const SB2GameSpecificData &game );
GAMEX_EXPORT bool IsHumanMultiplayerSlot( const SB2GameSpecificData &game, int nSlot );
GAMEX_EXPORT int GetPresentMultiplayerPlayerCount( const std::vector<SMPSlot> &slots );
GAMEX_EXPORT int FindAvailableMultiplayerSlot( const SB2GameSpecificData &game, const std::vector<SMPSlot> &slots );
GAMEX_EXPORT std::vector<int> GetAllowedMultiplayerNations( const SB2GameSpecificData &game,
                                                            const NDb::SMultiplayerConsts *pConsts = nullptr );
GAMEX_EXPORT bool ResolveMultiplayerMapRules( SB2GameSpecificData *pGame,
                                              std::string *pError,
                                              const NDb::SMultiplayerConsts *pConsts = nullptr );
GAMEX_EXPORT bool ValidateMultiplayerLobby( const SB2GameSpecificData &game,
                                            const std::vector<SMPSlot> &slots,
                                            std::string *pError,
                                            const NDb::SMultiplayerConsts *pConsts = nullptr );
} // namespace NGameX
