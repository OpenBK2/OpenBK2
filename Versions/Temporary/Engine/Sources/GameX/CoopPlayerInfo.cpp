#include "stdafx.h"

#include "CommandsHistory.h"
#include "CoopPlayerInfo.h"
#include "Stats_B2_M1/DBMapInfo.h"
#include "System/BinSaver.h"
#include "System/Text.h"

namespace NGameX
{
int SCoopPlayerInfo::operator&( IBinSaver &saver )
{
	saver.Add( 2, &nPlayerIndex );
	saver.Add( 3, &wszName );
	saver.Add( 4, &vMPStartPos );
	return 0;
}

std::vector<SCoopPlayerInfo> CollectCoopMapPlayers( const NDb::SMapInfo *pMap )
{
	std::vector<SCoopPlayerInfo> players;
	if ( !pMap )
	{
		return players;
	}
	// Preserve authored ownership IDs even when human and bot slots interleave.
	for ( int nPlayer = 0; nPlayer < pMap->players.size(); ++nPlayer )
	{
		const auto &mapPlayer = pMap->players[nPlayer];
		if ( mapPlayer.ePlayerType != NDb::AI_BOT || mapPlayer.nDiplomacySide == 2 )
		{
			continue;
		}
		auto &player = players.emplace_back();
		player.nPlayerIndex = nPlayer;
		player.wszName = NText::GetText( mapPlayer.szLocalizedPlayerNameFileRef );
		if ( player.wszName.empty() )
		{
			player.wszName = L"AI " + std::to_wstring( nPlayer + 1 );
		}
		player.vMPStartPos = mapPlayer.vMPStartPos;
	}
	return players;
}

std::vector<SCoopPlayerInfo> GetCoopReplayPlayers( const SMultiplayerReplayInfo &info )
{
	if ( info.nMultiplayerMode != 1 )
	{
		return {};
	}
	if ( info.bCoopPlayersRecorded )
	{
		return info.coopPlayers;
	}
	return CollectCoopMapPlayers( info.pMap );
}

int FindReplayViewpoint( const SMultiplayerReplayInfo &info, int nTeam )
{
	if ( info.bSinglePlayer )
	{
		return nTeam == 0 ? 0 : -1;
	}
	if ( info.nMultiplayerMode == 1 && nTeam != 0 )
	{
		return -1;
	}
	const NDb::SMapInfo *pMap = info.pMap;
	for ( int i = 0; i < info.slots.size(); ++i )
	{
		if ( !info.slots[i].bPresent || info.slots[i].nTeam != nTeam || ( pMap && i >= pMap->players.size() ) )
		{
			continue;
		}
		if ( info.nMultiplayerMode == 1 && pMap &&
		     ( pMap->players[i].ePlayerType != NDb::HUMAN || pMap->players[i].nDiplomacySide != 0 ) )
		{
			continue;
		}
		return i;
	}
	return -1;
}
} // namespace NGameX
