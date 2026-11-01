#include "stdafx.h"

#include "DifficultyLevel.h"
#include "Diplomacy.h"
#include "Misc/2Darray.h"
#include "ScenarioTracker.h"
#include "Stats_B2_M1/DBMapInfo.h"

CDiplomacy theDipl;

extern CDifficultyLevel theDifficultyLevel;

void CDiplomacy::Load( const std::vector<int> &_playerParty, const NDb::SMapInfo *pMapInfo )
{
	isAIPlayer.assign( _playerParty.size(), 0 );
	if ( !GetScenarioTracker() )
	{
		return;
	}
	playerParty = _playerParty;
	isPlayerExist.clear();
	isPlayerExist.resize( playerParty.size(), 0 );

	for ( int i = 0; i < isPlayerExist.size(); ++i )
	{
		isPlayerExist[i] = GetScenarioTracker()->IsPlayerPresent( i );
		if ( GetScenarioTracker()->IsCooperative() && pMapInfo && i < pMapInfo->players.size() )
		{
			isAIPlayer[i] = pMapInfo->players[i].ePlayerType == NDb::AI_BOT;
		}
		if ( GetScenarioTracker()->IsMissionActive() )
		{
			int nScenarioParty = GetScenarioTracker()->GetPlayerSide( i );

			// If the numbers from Scenario Tracker are valid, use them, otherwise use passed values (or neutral if these are invalid)
			if ( nScenarioParty >= 0 && nScenarioParty <= 2 )
			{
				playerParty[i] = nScenarioParty;
			}
			else if ( playerParty[i] < 0 || playerParty[i] > 2 )
			{
				playerParty[i] = 2;
			}
		}
	}

	// neutral always exists
	isPlayerExist[playerParty.size() - 1] = 1;

	nMyNumber = GetScenarioTracker()->GetLocalPlayer();
	SetMyNumber( nMyNumber );
}

void CDiplomacy::SetPlayerNotExist( const int nPlayer )
{
	isPlayerExist[nPlayer] = 0;
}

int CDiplomacy::operator&( IBinSaver &saver )
{
	if ( !saver.IsChecksum() )
	{
		saver.Add( 1, &playerParty );
		saver.Add( 2, &nMyNumber );
		saver.Add( 3, &bNetGame );
		saver.Add( 4, &isPlayerExist );
	}
	// Roles affect autonomous behavior and must survive saves and simulation checksums.
	if ( saver.IsReading() )
	{
		// Older saves have no role chunk and must not inherit a previous COOP map's bots.
		isAIPlayer.clear();
	}
	saver.Add( 5, &isAIPlayer );

	return 0;
}

bool CDiplomacy::IsPlayerExist( const int nPlayer ) const
{
	if ( nPlayer >= GetNPlayers() )
	{
		return false;
	}
	else
	{
		return isPlayerExist[nPlayer];
	}
}

void CDiplomacy::SetNetGame( bool _bNetGame )
{
	bNetGame = _bNetGame;

	if ( bNetGame )
	{
		theDifficultyLevel.SetLevel( 1 );
	}
}
