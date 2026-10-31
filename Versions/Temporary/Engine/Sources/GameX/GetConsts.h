#pragma once

#include "GameX_export.h"

#include <string>

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
struct SMultiplayerSide;
struct SLadderRank;

} // namespace NDb

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

// Bounds-checked lookups into MultiplayerConsts.
//
// The country (race) and ladder rank indices the multiplayer screens display
// come from the ladder server, while the arrays they index are sized by the
// database, so the two have no guaranteed relationship: an index out of range
// is a wire value the client has to survive, not a programming error. It does
// not take a hostile server either, because SLadderStatistics::nRace is -1
// until something fills it in, which is what an unanswered or partially filled
// stats record carries.
//
// Return 0 when the index does not name anything, so callers test the result
// rather than repeating the bounds check.
const NDb::SMultiplayerSide *GetMultiplayerSide( const NDb::SMultiplayerConsts *pMPConsts, int nCountry );
const NDb::SLadderRank *GetLadderRank( const NDb::SMultiplayerConsts *pMPConsts, int nCountry, int nRank );

// True when nCountry names a side and that side can be played at nTechLevel.
//
// The game room asks this of every side each time it builds a country list,
// with a tech level out of the host's game description. Note that the two tech
// level arrays an nTechLevel indexes, SMultiplayerConsts::techLevels and a
// side's own techLevels, are sized independently, so bounding the value once
// on entry would not make either use safe. A side that does not define the
// level cannot be played at it, which is what false means here.
bool IsSideAvailable( const NDb::SMultiplayerConsts *pMPConsts, int nCountry, int nTechLevel );

// The SVG flag nCountry shows at nTechLevel, or an empty
// path when there is none and the side's ListItemIcon texture should be used.
//
// The year looked up is the first one the tech level represents. A tech level
// with no years, a side with neither FlagAuthority nor FlagTerritory, and a year
// none of the side's flags covers all give the empty path, which is what old
// data and mods without flag records get, so they keep their own icons.
std::string GetSideFlagImage( const NDb::SMultiplayerConsts *pMPConsts, int nCountry, int nTechLevel );

} // namespace NGameX
