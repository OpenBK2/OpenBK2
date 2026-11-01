#pragma once

#include "Stats_B2_M1/AITypes.h"

#include "AILogic_export.h"

#include <cstdint>
#include <vector>

class IBinSaver;

namespace NDb
{
struct SMapInfo;
}

// последняя party обязательно должна быть нейтральна ко всем
class CDiplomacy
{
public:
	AILOGIC_EXPORT int operator&( IBinSaver &saver );

private:
	;

	// 0, 1 - игровые стороны, 2 - нейтралы
	std::vector<int> playerParty;
	// Defaults cover construction before Init/load; explicit constructor values still take precedence.
	int nMyNumber = {};

	std::vector<int> isPlayerExist;
	// COOP roles belong to map players, independently of each client's viewpoint.
	std::vector<int> isAIPlayer;

	bool bNetGame = {};

public:
	CDiplomacy()
		: nMyNumber( 0 )
		, bNetGame( false )
	{
	}

	void Load( const std::vector<int> &playerParty, const NDb::SMapInfo *pMapInfo = nullptr );

	void SetNPlayers( const int nPlayers )
	{
		playerParty.resize( nPlayers + 1, 0 );
		playerParty[nPlayers] = 2;
		isAIPlayer.assign( nPlayers + 1, 0 );
	}

	void Clear()
	{
		isAIPlayer.clear();
	}

	const int GetNPlayers() const
	{
		return playerParty.size();
	}

	const EDiplomacyInfo GetDiplStatus( const uint8_t a, const uint8_t b ) const
	{
		if ( playerParty[a] == 2 || playerParty[b] == 2 )
		{
			return EDI_NEUTRAL;
		}
		else if ( playerParty[a] != playerParty[b] )
		{
			return EDI_ENEMY;
		}
		else
		{
			return EDI_FRIEND;
		}
	}

	const EDiplomacyInfo GetDiplStatusForParties( const uint8_t nParty1, const uint8_t nParty2 )
	{
		if ( nParty1 == 2 || nParty2 == 2 )
		{
			return EDI_NEUTRAL;
		}
		else if ( nParty1 != nParty2 )
		{
			return EDI_ENEMY;
		}
		else
		{
			return EDI_FRIEND;
		}
	}

	const uint8_t GetNParty( const uint8_t cPlayer ) const
	{
		return playerParty[cPlayer];
	}

	const uint8_t GetMyNumber() const
	{
		return nMyNumber;
	}

	const uint8_t GetMyParty() const
	{
		return GetNParty( GetMyNumber() );
	}

	const bool IsAIPlayer( const uint8_t cPlayer ) const
	{
		if ( !bNetGame )
		{
			return cPlayer != GetMyNumber();
		}
		// The final map slot is neutral even when its authored role is invalid.
		return cPlayer < isAIPlayer.size() && cPlayer < playerParty.size() && cPlayer != GetNeutralPlayer() &&
		       playerParty[cPlayer] != GetNeutralParty() && isAIPlayer[cPlayer] != 0;
	}

	const uint8_t GetNeutralPlayer() const
	{
		return GetNPlayers() - 1;
	}

	// номер нейтральной стороны
	int GetNeutralParty() const
	{
		return 2;
	}

	void SetParty( const uint8_t nPlayer, const uint8_t newParty )
	{
		playerParty[nPlayer] = newParty;
	}

	void SetMyNumber( const int nNumber )
	{
		nMyNumber = nNumber;
	}

	void SetNetGame( bool _bNetGame );

	bool IsNetGame() const
	{
		return bNetGame;
	}

	//
	bool IsPlayerExist( const int nPlayer ) const;
	void SetPlayerNotExist( const int nPlayer );
};

const int ANY_PARTY = EDI_FRIEND | EDI_ENEMY | EDI_NEUTRAL;

extern AILOGIC_EXPORT CDiplomacy theDipl;
