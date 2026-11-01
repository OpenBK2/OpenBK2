#include "stdafx.h"

#include "DBMPConsts.h"
#include "GetConsts.h"
#include "MPCoopRules.h"
#include "MPInterfaceData.h"
#include "Stats_B2_M1/DBMapInfo.h"

#include <algorithm>

namespace
{
bool RejectRules( std::string *pError, const char *pszReason )
{
	if ( pError )
	{
		*pError = pszReason;
	}
	return false;
}
} // namespace

namespace NGameX
{
bool IsCoopGame( const SB2GameSpecificData &game )
{
	return game.eType == SB2GameSpecificData::EGT_COOP;
}

int GetMultiplayerSlotCount( const SB2GameSpecificData &game )
{
	return IsCoopGame( game ) && game.pMPMap && game.pMPMap->pMap ? game.pMPMap->pMap->players.size() : game.nPlayers;
}

bool IsHumanMultiplayerSlot( const SB2GameSpecificData &game, int nSlot )
{
	if ( IsCoopGame( game ) && ( !game.pMPMap || !game.pMPMap->pMap ) )
	{
		return false;
	}
	if ( nSlot < 0 || nSlot >= GetMultiplayerSlotCount( game ) )
	{
		return false;
	}
	if ( !IsCoopGame( game ) )
	{
		return true;
	}
	int nHumans = 0;
	const auto &players = game.pMPMap->pMap->players;
	for ( int i = 0; i <= nSlot; ++i )
	{
		if ( players[i].nDiplomacySide != 2 && players[i].ePlayerType == NDb::HUMAN )
		{
			++nHumans;
		}
	}
	return players[nSlot].nDiplomacySide == 0 && players[nSlot].ePlayerType == NDb::HUMAN && nHumans <= game.nPlayers;
}

int GetPresentMultiplayerPlayerCount( const std::vector<SMPSlot> &slots )
{
	return std::count_if( slots.begin(), slots.end(), []( const SMPSlot &slot ) { return slot.bPresent; } );
}

int FindAvailableMultiplayerSlot( const SB2GameSpecificData &game, const std::vector<SMPSlot> &slots )
{
	if ( GetPresentMultiplayerPlayerCount( slots ) >= game.nPlayers )
	{
		return -1;
	}
	for ( int i = 0; i < slots.size(); ++i )
	{
		if ( IsHumanMultiplayerSlot( game, i ) && !slots[i].bPresent && !slots[i].bAccept )
		{
			return i;
		}
	}
	return -1;
}

std::vector<int> GetAllowedMultiplayerNations( const SB2GameSpecificData &game, const NDb::SMultiplayerConsts *pConsts )
{
	if ( !pConsts )
	{
		pConsts = GetMPConsts();
	}
	std::vector<int> nations;
	if ( !pConsts )
	{
		return nations;
	}
	for ( int i = 0; i < pConsts->sides.size(); ++i )
	{
		if ( !pConsts->sides[i].pPartyInfo || !IsSideAvailable( pConsts, i, game.nTechLevel ) )
		{
			continue;
		}
		if ( game.pMPMap && !game.pMPMap->allowedNations.empty() &&
		     std::find( game.pMPMap->allowedNations.begin(), game.pMPMap->allowedNations.end(), i ) ==
		         game.pMPMap->allowedNations.end() )
		{
			continue;
		}
		nations.push_back( i );
	}
	return nations;
}

bool ResolveMultiplayerMapRules( SB2GameSpecificData *pGame,
                                 std::string *pError,
                                 const NDb::SMultiplayerConsts *pConsts )
{
	if ( !pConsts )
	{
		pConsts = GetMPConsts();
	}
	if ( !pGame || !pGame->pMPMap || !pGame->pMPMap->pMap || !pConsts )
	{
		return RejectRules( pError, "The multiplayer map or multiplayer constants are missing." );
	}
	const auto &map = *pGame->pMPMap;
	if ( map.eGameType != NDb::CLASSIC && map.eGameType != NDb::COOP )
	{
		return RejectRules( pError, "The map has an unsupported game type." );
	}
	pGame->eType = map.eGameType == NDb::COOP ? SB2GameSpecificData::EGT_COOP : SB2GameSpecificData::EGT_FLAG_CONTROL_2;
	pGame->nDifficultyLevel = map.nDifficultyLevel;
	if ( map.nForcedTechLevel < -1 || map.nForcedTechLevel >= int( pConsts->techLevels.size() ) )
	{
		return RejectRules( pError, "The map's forced tech level is outside MultiplayerConsts.TechLevels." );
	}
	if ( map.nForcedTechLevel >= 0 )
	{
		pGame->nTechLevel = map.nForcedTechLevel;
	}
	if ( pGame->nTechLevel < 0 || pGame->nTechLevel >= pConsts->techLevels.size() )
	{
		return RejectRules( pError, "Select a valid multiplayer tech level." );
	}
	for ( int nNation : map.allowedNations )
	{
		if ( nNation < 0 || nNation >= pConsts->sides.size() )
		{
			return RejectRules( pError, "The map's allowed nations contain an invalid MultiplayerConsts side index." );
		}
	}
	if ( GetAllowedMultiplayerNations( *pGame, pConsts ).empty() )
	{
		return RejectRules( pError, "No allowed nation is available at this tech level." );
	}
	if ( pGame->nPlayers < 2 || pGame->nPlayers > map.nPlayers )
	{
		return RejectRules( pError,
		                    "The lobby requires at least two human slots and cannot exceed the map's player limit." );
	}
	if ( IsCoopGame( *pGame ) )
	{
		const auto &players = map.pMap->players;
		// Connectivity and lockstep membership use 16-bit player-ID masks.
		if ( players.size() < 3 || players.size() > 16 || players.back().nDiplomacySide != 2 )
		{
			return RejectRules( pError, "COOP maps require at most 16 map players, with the neutral player last." );
		}
		int nHumans = 0;
		for ( int i = 0; i + 1 < players.size(); ++i )
		{
			const auto &player = players[i];
			if ( player.ePlayerType == NDb::HUMAN && player.nDiplomacySide == 0 )
			{
				++nHumans;
			}
			else if ( player.ePlayerType != NDb::AI_BOT || player.nDiplomacySide != 1 )
			{
				return RejectRules( pError,
				                    "COOP players must be HUMAN on side 0 or AI_BOT on side 1, followed by one neutral." );
			}
		}
		if ( nHumans < map.nPlayers )
		{
			return RejectRules( pError, "The COOP map declares more human slots than its HUMAN players provide." );
		}
		pGame->bRandomPlacement = false;
		pGame->nTimeLimit = 0; // COOP ends only when the map script decides its result.
	}
	if ( pError )
	{
		pError->clear();
	}
	return true;
}

bool ValidateMultiplayerLobby( const SB2GameSpecificData &game,
                               const std::vector<SMPSlot> &slots,
                               std::string *pError,
                               const NDb::SMultiplayerConsts *pConsts )
{
	if ( !pConsts )
	{
		pConsts = GetMPConsts();
	}
	SB2GameSpecificData resolved = game;
	if ( !ResolveMultiplayerMapRules( &resolved, pError, pConsts ) )
	{
		return false;
	}
	if ( game.eType != resolved.eType || game.nTechLevel != resolved.nTechLevel ||
	     game.nDifficultyLevel != resolved.nDifficultyLevel || slots.size() != GetMultiplayerSlotCount( game ) ||
	     ( IsCoopGame( game ) && ( game.bRandomPlacement || game.nTimeLimit != 0 ) ) )
	{
		return RejectRules( pError, "The lobby settings do not match the map's rules." );
	}
	const auto nations = GetAllowedMultiplayerNations( game, pConsts );
	int nHumans = 0;
	int nTeams[2] = {};
	for ( int i = 0; i < slots.size(); ++i )
	{
		const auto &slot = slots[i];
		if ( !slot.bPresent )
		{
			continue;
		}
		if ( !IsHumanMultiplayerSlot( game, i ) || slot.nTeam < 0 || slot.nTeam > 1 ||
		     ( IsCoopGame( game ) && slot.nTeam != 0 ) ||
		     std::find( nations.begin(), nations.end(), slot.nCountry ) == nations.end() )
		{
			return RejectRules( pError, "A player occupies an unavailable slot, team or nation." );
		}
		if ( !slot.bAccept )
		{
			return RejectRules( pError, "Every player must be ready before starting." );
		}
		if ( slot.nColour < 0 || slot.nColour >= pConsts->playerColorInfos.size() )
		{
			return RejectRules( pError, "A player has selected an unavailable colour." );
		}
		++nHumans;
		++nTeams[slot.nTeam];
		for ( int j = 0; j < i; ++j )
		{
			if ( slots[j].bPresent && slots[j].nColour == slot.nColour )
			{
				return RejectRules( pError, "Players must choose different colours." );
			}
		}
	}
	if ( nHumans < 2 || ( !IsCoopGame( game ) && ( nTeams[0] == 0 || nTeams[1] == 0 ) ) )
	{
		return RejectRules( pError, "At least two players are required; CLASSIC also requires both teams." );
	}
	return true;
}
} // namespace NGameX
