#pragma once

#include "GameX_export.h"

#include <cstdint>
#include <list>
#include <string>

struct IScenarioTracker;

namespace NDb
{
struct SMultiplayerConsts;
}

namespace NGameX
{
struct SMPStatisticsPlayer
{
	std::wstring wszName;
	int nCountry = -1;
	uint32_t dwColour = 0;
	int nTeam = 0;
	int nUnitsLost = 0;
	int nUnitsKilled = 0;
	int nScoreTactics = 0;
	int nScoreStrategy = 0;
	int nScore = 0;
};

struct SMPStatisticsTeams
{
	std::wstring wszLocalPlayerName;
	std::list<SMPStatisticsPlayer> wonTeam;
	std::list<SMPStatisticsPlayer> lostTeam;
};

// Build presentation rows from the final statistics snapshot. Map-controlled COOP
// players belong here, not in the human network roster used by lobby and replay code.
GAMEX_EXPORT SMPStatisticsTeams BuildMultiplayerStatistics( IScenarioTracker *pTracker,
                                                            const NDb::SMultiplayerConsts *pMPConsts );
} // namespace NGameX
