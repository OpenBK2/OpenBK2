#pragma once

#include "Server_Client_Common/LadderStatistics.h"
extern int NUMBER_OF_RACES_IN_LADDER;

struct SLadderDBInfo : public CObjectBase
{
	// Ladder player's info in internal representation
	OBJECT_NOCOPY_METHODS( SLadderDBInfo )
public:

	int nXP;
	int nLevel;
	int nMaxXPEarned;
	int nMaxXPLost;
	int nUnitsKilled;
	int nUnitsLost;
	int nUnitsEffectiveness;
	int nKeyPointsEffectiveness;
	int nTotalPlayTime;
	int nTotalGamesPlayed;
	int nWinsInSeries;
	std::vector<int> raceWinsSolo;
	std::vector<int> raceWinsTeam;
	std::vector<int> raceLossesSolo;
	std::vector<int> raceLossesTeam;
	std::vector<int> reinforcementUsed;
	std::vector<int> winsAgainst;
	std::vector<int> lossesAgainst;
	std::vector<int> mapsPlayed;
	std::vector<int> techsPlayed;
	std::vector<int> medals;

	int GetFavouriteRace() const
	{
		int nFavouriteRace = 0;
		int nFavouriteRaceGamesPlayed = 0;
		for ( int nRace = 0; nRace < NUMBER_OF_RACES_IN_LADDER; ++nRace )
		{
			const int nGamesPlayed = raceLossesSolo[nRace] + raceLossesTeam[nRace] +
				raceWinsSolo[nRace] + raceWinsTeam[nRace];
			if ( nGamesPlayed > nFavouriteRaceGamesPlayed )
			{
				nFavouriteRace = nRace;
				nFavouriteRaceGamesPlayed = nGamesPlayed;
			}
		}
		return nFavouriteRace;
	}

	void ConvertForClient( SLadderStatistics *pOutStatistics )
	{
		pOutStatistics->nXP = nXP;
		pOutStatistics->nLevel = nLevel;
		{
			int nMax = -1;
			int nFavourite = 0;
			for ( int i = 0; i < reinforcementUsed.size(); ++i )
			{
				if ( reinforcementUsed[i] > nMax )
				{
					nFavourite = i;
					nMax = reinforcementUsed[i];
				}
			}
			pOutStatistics->nFavouriteReinforcement = nFavourite;
		}
		pOutStatistics->nMaxXPEarned = nMaxXPEarned;
		pOutStatistics->nMaxXPLost = nMaxXPLost;
		pOutStatistics->nUnitsKilled = nUnitsKilled;
		pOutStatistics->nUnitsLost = nUnitsLost;
		pOutStatistics->nUnitEff = nUnitsEffectiveness;
		pOutStatistics->nKeyPointEff = nKeyPointsEffectiveness;
		{
			int nMax = -1;
			int nWeakest = 0;
			for ( int i = 0; i < lossesAgainst.size(); ++i )
			{
				if ( lossesAgainst[i] > nMax )
				{
					nMax = lossesAgainst[i];
					nWeakest = i;
				}
			}
			pOutStatistics->nWeakestAgainst = nWeakest;
		}
		{
			int nMax = -1;
			int nStrongest = 0;
			for ( int i = 0; i < winsAgainst.size(); ++i )
			{
				if ( winsAgainst[i] > nMax )
				{
					nMax = winsAgainst[i];
					nStrongest = i;
				}
			}
			pOutStatistics->nStrongestAgainst = nStrongest;
		}
		pOutStatistics->raceWinsSolo = raceWinsSolo;
		pOutStatistics->raceWinsTeam = raceWinsTeam;
		pOutStatistics->raceLossesSolo = raceLossesSolo;
		pOutStatistics->raceLossesTeam = raceLossesTeam;
		pOutStatistics->medals = medals;
		pOutStatistics->nTotalPlayTime = nTotalPlayTime;
		pOutStatistics->nRace = GetFavouriteRace();
		pOutStatistics->techsPlayed = techsPlayed;
		pOutStatistics->mapsPlayed = mapsPlayed;
	}

};


