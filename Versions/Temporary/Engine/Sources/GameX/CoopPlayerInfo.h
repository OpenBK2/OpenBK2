#pragma once

#include "Misc/Geom.h"

#include "GameX_export.h"

#include <cstdint>
#include <string>
#include <vector>

struct IBinSaver;
struct SMultiplayerReplayInfo;

namespace NDb
{
struct SMapInfo;
}

namespace NGameX
{
constexpr uint32_t COOP_BOT_COLOR = 0xffff0000;

// Presentation only. Bots never occupy entries in the human network slot list.
struct SCoopPlayerInfo
{
	int nPlayerIndex = -1;
	std::wstring wszName;
	CVec2 vMPStartPos = VNULL2;

	GAMEX_EXPORT int operator&( IBinSaver &saver );
};

GAMEX_EXPORT std::vector<SCoopPlayerInfo> CollectCoopMapPlayers( const NDb::SMapInfo *pMap );
// Recorded empty lists are authoritative; only older COOP replays reread the map.
GAMEX_EXPORT std::vector<SCoopPlayerInfo> GetCoopReplayPlayers( const SMultiplayerReplayInfo &info );
} // namespace NGameX
