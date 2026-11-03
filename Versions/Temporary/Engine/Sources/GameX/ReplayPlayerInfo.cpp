#include "stdafx.h"

#include "CommandsHistory.h"
#include "CoopPlayerInfo.h"
#include "Misc/StrProc.h"
#include "ReplayPlayerInfo.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "System/Text.h"

#include <map>

namespace NGameX
{
namespace
{
	EReplayPlayerResult GetPlayerResult( const SMultiplayerReplayInfo &info, int nTeam )
	{
		if ( ( info.nMultiplayerMode == 1 && info.nScriptResult == 0 ) ||
		     ( info.nWinningSide != 0 && info.nWinningSide != 1 ) || ( nTeam != 0 && nTeam != 1 ) )
		{
			return RPR_UNFINISHED;
		}
		return nTeam == info.nWinningSide ? RPR_WINNER : RPR_LOSER;
	}

	bool IsMapViewpoint( const SMultiplayerReplayInfo &info, const SReplayPlayerInfo &player )
	{
		const NDb::SMapInfo *pMap = info.pMap;
		if ( !pMap || player.nPlayerIndex < 0 || player.nPlayerIndex >= pMap->players.size() )
		{
			return false;
		}
		if ( info.bSinglePlayer )
		{
			// Single-player AI roles depend on the local player; retain its original viewpoint.
			return player.nPlayerIndex == 0;
		}
		if ( info.nMultiplayerMode != 0 && info.nMultiplayerMode != 1 )
		{
			return false;
		}
		const auto &mapPlayer = pMap->players[player.nPlayerIndex];
		if ( player.nPlayerIndex == pMap->players.size() - 1 || mapPlayer.nDiplomacySide == 2 ||
		     ( player.nTeam != 0 && player.nTeam != 1 ) )
		{
			return false;
		}
		if ( info.nMultiplayerMode == 1 )
		{
			return player.bBot ? mapPlayer.ePlayerType == NDb::AI_BOT && mapPlayer.nDiplomacySide == 1
			                   : mapPlayer.ePlayerType == NDb::HUMAN && mapPlayer.nDiplomacySide == 0 && player.nTeam == 0;
		}
		// CLASSIC team/nation assignments come from the recorded slots, not map roles.
		return !player.bBot;
	}
} // namespace

std::vector<SReplayPlayerInfo> GetReplayPlayers( const SMultiplayerReplayInfo &info )
{
	if ( info.bSinglePlayer )
	{
		SReplayPlayerInfo player;
		player.nPlayerIndex = 0;
		player.nTeam = 0;
		if ( info.pMap && !info.pMap->players.empty() )
		{
			player.wszName = NText::GetText( info.pMap->players[0].szLocalizedPlayerNameFileRef );
		}
		if ( player.wszName.empty() )
		{
			player.wszName = L"Player 1";
		}
		player.bSelectable = IsMapViewpoint( info, player );
		player.eResult = GetPlayerResult( info, player.nTeam );
		return { player };
	}

	std::map<int, SReplayPlayerInfo> rows;
	for ( int nPlayer = 0; nPlayer < info.slots.size(); ++nPlayer )
	{
		const SMPSlot &slot = info.slots[nPlayer];
		if ( !slot.bPresent )
		{
			continue;
		}
		SReplayPlayerInfo player;
		player.nPlayerIndex = nPlayer;
		player.wszName = NStr::ToUnicode( slot.szName );
		if ( player.wszName.empty() )
		{
			player.wszName = L"Player " + std::to_wstring( nPlayer + 1 );
		}
		player.nTeam = slot.nTeam;
		player.bSelectable = IsMapViewpoint( info, player );
		player.eResult = GetPlayerResult( info, player.nTeam );
		rows.emplace( nPlayer, player );
	}
	for ( const auto &bot : GetCoopReplayPlayers( info ) )
	{
		SReplayPlayerInfo player;
		player.nPlayerIndex = bot.nPlayerIndex;
		player.wszName = bot.wszName;
		if ( player.wszName.empty() )
		{
			player.wszName = L"AI " + std::to_wstring( bot.nPlayerIndex + 1 );
		}
		player.nTeam = 1;
		player.bBot = true;
		player.bSelectable = IsMapViewpoint( info, player );
		player.eResult = GetPlayerResult( info, player.nTeam );
		const auto inserted = rows.emplace( player.nPlayerIndex, player );
		if ( !inserted.second )
		{
			// Conflicting/duplicate metadata must not silently choose an owner's viewpoint.
			inserted.first->second.bSelectable = false;
		}
	}

	std::vector<SReplayPlayerInfo> players;
	players.reserve( rows.size() );
	for ( const auto &row : rows )
	{
		players.push_back( row.second );
	}
	return players;
}

bool CanWatchReplayPlayer( const SMultiplayerReplayInfo &info, int nPlayerIndex )
{
	for ( const auto &player : GetReplayPlayers( info ) )
	{
		if ( player.nPlayerIndex == nPlayerIndex )
		{
			return player.bSelectable;
		}
	}
	return false;
}
} // namespace NGameX
