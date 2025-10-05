#pragma once

struct SLadderStatistics
{
	enum EMedalTypes
	{
		UNDEFINED = 15,
		KILLED_3 = 0,
		KILLED_2 = 1,
		KILLED_1 = 2,
		KILLED_LOST_3 = 3,
		KILLED_LOST_2 = 4,
		KILLED_LOST_1 = 5,
		WIN_SERIES_3 = 6,
		WIN_SERIES_2 = 7,
		WIN_SERIES_1 = 8,
		FIRST_PLACE = 9
	};

	ZDATA
// From database
	int nLevel;
	int nXP;
	int nMaxXPEarned;
	int nMaxXPLost;
	int nUnitsKilled;
	int nUnitsLost;
	int nUnitEff;
	int nKeyPointEff;
	int nFavouriteReinforcement;
	int nStrongestAgainst;
	int nWeakestAgainst;
	int nTotalPlayTime;
	std::vector<int> raceWinsSolo;
	std::vector<int> raceLossesSolo;
	std::vector<int> raceWinsTeam;
	std::vector<int> raceLossesTeam;
	std::vector<int> medals;
	int nRace;
// From constants
	int nLevelXP;
	int nNextLevelXP;

	std::vector<int> techsPlayed;
	std::vector<int> mapsPlayed;
	ZEND int operator&( IBinSaver &f ) { f.Add(2,&nLevel); f.Add(3,&nXP); f.Add(4,&nMaxXPEarned); f.Add(5,&nMaxXPLost); f.Add(6,&nUnitsKilled); f.Add(7,&nUnitsLost); f.Add(8,&nUnitEff); f.Add(9,&nKeyPointEff); f.Add(10,&nFavouriteReinforcement); f.Add(11,&nStrongestAgainst); f.Add(12,&nWeakestAgainst); f.Add(13,&nTotalPlayTime); f.Add(14,&raceWinsSolo); f.Add(15,&raceLossesSolo); f.Add(16,&raceWinsTeam); f.Add(17,&raceLossesTeam); f.Add(18,&medals); f.Add(19,&nRace); f.Add(20,&nLevelXP); f.Add(21,&nNextLevelXP); f.Add(22,&techsPlayed); f.Add(23,&mapsPlayed); return 0; }
	SLadderStatistics() : nLevel( -1 ), nXP( 0 ), nLevelXP( 0 ), nNextLevelXP( 0 ), nMaxXPEarned( 0 ), nMaxXPLost( 0 ), nFavouriteReinforcement( -1 ),
		nUnitsKilled( 0 ), nUnitsLost( 0 ), nUnitEff( 0 ), nKeyPointEff( 0 ), nStrongestAgainst( -1 ), nWeakestAgainst( -1 ), nTotalPlayTime( 0 ), nRace( -1 ) {}

	static bool HasMedal( const std::vector<int> &medals, const int nRace, const int _nMedalType )
	{
		const int nMedalType = Clamp( _nMedalType, 0, 15 );
		if ( medals.size() < ( nRace + 1 ) )
			return false;
		return ( medals[nRace] & ( 0x0001 << nMedalType ) );
	}

	static void AddMedal( std::vector<int> *pMedals, const int nRace, const int _nMedalType )
	{
		const int nMedalType = Clamp( _nMedalType, 0, 15 );
		if ( pMedals->size() < ( nRace + 1 ) )
			pMedals->resize( nRace, 0 );
		(*pMedals)[nRace] |= 0x0001 << nMedalType;
	}

}; 


