#pragma once

#include "GameX_export.h"

#include <string>
#include <vector>

struct SMultiplayerReplayInfo;

namespace NGameX
{
enum EReplayPlayerResult
{
	RPR_UNFINISHED,
	RPR_WINNER,
	RPR_LOSER
};

// A replay viewpoint is an authored player ID, never a team or a network-list position.
struct SReplayPlayerInfo
{
	int nPlayerIndex = -1;
	std::wstring wszName;
	int nTeam = -1;
	bool bBot = false;
	bool bSelectable = false;
	EReplayPlayerResult eResult = RPR_UNFINISHED;
};

// Keep unavailable players visible, while validating selection against the current map.
GAMEX_EXPORT std::vector<SReplayPlayerInfo> GetReplayPlayers( const SMultiplayerReplayInfo &info );
GAMEX_EXPORT bool CanWatchReplayPlayer( const SMultiplayerReplayInfo &info, int nPlayerIndex );
} // namespace NGameX
