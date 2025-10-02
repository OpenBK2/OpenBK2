#include "stdafx.h"
#include "LadderLobby.h"
#include "clients.h"
#include "System/RandomGen.h"
#include "Misc/Time64.h"
#include "Statistics.h"
#include "Server_Client_Common/GamePackets.h"
//#define LADDER_TEST

extern int MAX_NUMBER_OF_REINFORCEMENTS;

#define STEP_LENGTH_TIME 200
#define GAME_DEATH_TIMEOUT 3000

const int MAX_TECHLEVELS = 100;
const int MAX_MAPS = 500;

namespace NLadder{
	void DropFromListByLevel( const hash_map< int, CPtr<CLadderClient> > &players, list<int> *pWaitingOrder,
		const int nLevel, const int nDeltaPlus, const int nDeltaMinus );
	void DropFromListByTeamSize( const hash_map< int, CPtr<CLadderClient> > &players, list<int> *pWaitingOrder,
		const int nTeamSize );
	void DropFromListByHistoricityAndChecksum( const hash_map< int, CPtr<CLadderClient> > &players, list<int> *pWaitingOrder,
		const bool bHistoricity, const unsigned int uCheckSum );
	void DropFromListBySide( const hash_map< int, CPtr<CLadderClient> > &players, list<int> *pWaitingOrder,
		const int nSide );
	void RandomizeList( list<int> *pList );
	float TeamLevel( const hash_map< int, CPtr<CLadderClient> > &players, const list<int> &team );
	void DropWeakestExcept( const hash_map< int, CPtr<CLadderClient> > &players, list<int> *pList, const int nException );
	void DropStrongestExcept( const hash_map< int, CPtr<CLadderClient> > &players, list<int> *pList, const int nException );
}

class CLadderExperience
{
public:
	int nNewbieLevel;
	hash_map<int,float> lossFactor;
	hash_map<int, int> winXP;
	hash_map<int, int> loseXP;
	vector<int> levelXP;
	int operator&( IXmlSaver &f )
	{
		f.Add( "NewbieLevel", &nNewbieLevel );
		f.Add( "LossFactor", &lossFactor );
		f.Add( "Win", &winXP );
		f.Add( "Lose", &loseXP );
		f.Add( "Levels", &levelXP );
		return 0;
	}
};

class CLadderConsts : public CObjectBase
{
	OBJECT_NOCOPY_METHODS( CLadderConsts )
public: 
	int nLevelDelta;
	int nRaces;
	int nMaxUnitTypes;
	int nWaitTime1;
	int nWaitTime2;
	int nMistakes;
	int nMaxTeamSize;
	int nPlayersPerStep;
	CLadderExperience experience;
	vector<int> killedMedals;
	vector<float> killedLostMedals;
	vector<int> winsInSeriesMedals;
	int nFirstPlaceMedalLevel;
	int operator&( IXmlSaver &f )
	{ 
		f.Add( "LevelDelta", &nLevelDelta );
		f.Add( "NumberOfRaces", &nRaces );
		f.Add( "MaxUnitTypes", &nMaxUnitTypes );
		f.Add( "WaitTime1", &nWaitTime1 );
		f.Add( "WaitTime2", &nWaitTime2 );
		f.Add( "MistakesPerStep", &nMistakes );
		f.Add( "MaxTeamSize", &nMaxTeamSize );
		f.Add( "PlayersPerStep", &nPlayersPerStep );
		f.Add( "KilledMedals", &killedMedals );
		f.Add( "KilledLostMedals", &killedLostMedals );
		f.Add( "WinsInSeriesMedals", &winsInSeriesMedals );
		f.Add( "FirstPlaceMedalLevel", &nFirstPlaceMedalLevel );
		f.Add( "Experience", &experience );
		return 0;
	}
};

CLadderCacheLocker::CLadderCacheLocker( CClients* _pClients, const string &_szNick ) : pClients( _pClients ), szNick( _szNick )
{
	pClients->LockLadderInfo( szNick );
}

CLadderCacheLocker::~CLadderCacheLocker()
{
	pClients->UnlockLadderInfo( szNick );
}

BASIC_REGISTER_CLASS( CLadderConsts )

void CLadderLobby::Initialize( const string &_szCfgFile )
{
	REGISTER_PACKET_PROCESSOR( ProcessLadderInfoPacket );
	REGISTER_PACKET_PROCESSOR( ProcessLadderGameResultPacket );
	REGISTER_PACKET_PROCESSOR( ProcessLadderStatisticsRequestPacket );
	REGISTER_PACKET_PROCESSOR( ProcessSurrenderPacket );

	szCfgFile = _szCfgFile;

	nLastStepTime = GetLongTickCount();

	pConsts = new CLadderConsts;

	ReloadConfig();

	WriteMSG( "Ladder lobby configured.\n" );

	pStatisticsCollector = NStatistics::CreateCollector( "LADDER" );
	pStatisticsCollector->SetSpecific( "AverageWaitTime", NStatistics::CreateAverageValueCounter() );
	pStatisticsCollector->SetSpecific( "PlayersAverage", NStatistics::CreateAverageValueCounter() );
	pStatisticsCollector->SetSpecific( "GamesTotal", NStatistics::CreateEventsCounter() );
	pStatisticsCollector->SetSpecific( "RequestsPerSecond", NStatistics::CreateAverageValuePerTimeCounter() );
	pStatisticsCollector->SetSpecific( "GamesPerSecond", NStatistics::CreateAverageValuePerTimeCounter() );
	pStatisticsCollector->SetSpecific( "TotalPlayersEntered", NStatistics::CreateEventsCounter() );
	pStatisticsCollector->SetSpecific( "GamesAverage", NStatistics::CreateAverageValueCounter() );
}

void CLadderLobby::ReloadConfig()
{
	{
		CFileStream stream( szCfgFile, CFileStream::WIN_READ_ONLY );
		NI_ASSERT( stream.IsOk(), StrFmt( "Could not open cfg file: %s", szCfgFile ) );
		CPtr<IXmlSaver> pSaver = CreateXmlSaver( &stream, SAVER_MODE_READ );
		NI_ASSERT( pSaver.GetPtr(), "Could not create XML saver" );
		pSaver->Add( "LadderConsts", &( *pConsts ) );
	}
	CGameLobby::ReloadConfig();
	NUMBER_OF_RACES_IN_LADDER = pConsts->nRaces;
}

bool CLadderLobby::Segment()
{
	UINT64 nTime = GetLongTickCount();
	if ( nTime > nLastStepTime + STEP_LENGTH_TIME )
	{
		nLastStepTime = nTime;
		bool bGameCreated = false;
		for ( int i = 0; i < pConsts->nPlayersPerStep && ! bGameCreated; ++i )
		{
			bGameCreated = MatchMakingStep();
		}
		(*pStatisticsCollector)[ "GamesAverage" ]->Add( float( games.size() ) );
		(*pStatisticsCollector)[ "PlayersAverage" ]->Add( float( ladderClients.size() ) );

		// Do something
		UpdateGames();
	}

	CGameLobby::Segment();
	return true;
}

void CLadderLobby::UpdateGames()
{
	const UINT64 nTime = GetLongTickCount();
	list<int> gamesToDel;
	for ( hash_map< int, SLadderGameInfo >::iterator it = games.begin(); it != games.end(); ++it )
	{
		const SLadderGameInfo &gameInfo = it->second;
		if ( gameInfo.bIsDead && nTime > ( gameInfo.nDeathTime + GAME_DEATH_TIMEOUT ) )
		{
			CalcResults( it->first );
			gamesToDel.push_back( it->first );
		}
	}
	for ( list<int>::iterator it = gamesToDel.begin(); it != gamesToDel.end(); ++it )
	{
		games.erase( *it );
	}
}

bool CLadderLobby::CheckGameResultIsFake( const int nGameID )
{
	if ( games.find( nGameID ) == games.end() )
		return true;

	SLadderGameInfo &gameInfo = games[ nGameID ];
	const int nTeamSize = gameInfo.team1Players.size();

	if ( gameInfo.bInvalid )
		return true;

	if ( gameInfo.winners.size() == 0 )
	{
		gameInfo.eInvalidReason = SLadderGameInfo::EInvalidReason::NO_WINNERS;
		return true;
	}

	if ( gameInfo.winners.size() > gameInfo.team1Players.size() )
	{
		gameInfo.eInvalidReason = SLadderGameInfo::EInvalidReason::TOO_MANY_WINNERS;
		return true;
	}

	if ( gameInfo.playerRaces.size() != ( gameInfo.team1Players.size() + gameInfo.team2Players.size() ) )
	{
		gameInfo.eInvalidReason = SLadderGameInfo::EInvalidReason::INVALID_RACEINFO;
		return true;
	}

	for ( hash_map<int,int>::const_iterator it = gameInfo.playerRaces.begin(); it != gameInfo.playerRaces.end(); ++it )
	{
		if ( it->second >= NUMBER_OF_RACES_IN_LADDER || it->second < 0 )
		{
			gameInfo.eInvalidReason = SLadderGameInfo::EInvalidReason::INVALID_RACE_NUMBER;
			return true;
		}
	}

	for ( hash_map<int,int>::const_iterator it = gameInfo.unitsKilled.begin(); it != gameInfo.unitsKilled.end(); ++it )
	{
		if ( it->second < 0 )
		{
			gameInfo.eInvalidReason = SLadderGameInfo::EInvalidReason::NEGATIVE_KILLED;
			return true;
		}
	}

	for ( hash_map<int,int>::const_iterator it = gameInfo.unitsLost.begin(); it != gameInfo.unitsLost.end(); ++it )
	{
		if ( it->second < 0 )
		{
			gameInfo.eInvalidReason = SLadderGameInfo::EInvalidReason::NEGATIVE_LOST;
			return true;
		}
	}

	for ( hash_map<int,vector<int> >::iterator it = gameInfo.reinfUsed.begin(); it != gameInfo.reinfUsed.end(); ++it )
	{
		if ( it->second.size() > MAX_NUMBER_OF_REINFORCEMENTS )
		{
			gameInfo.eInvalidReason = SLadderGameInfo::EInvalidReason::TOO_MANY_REINFORCEMENTS;
			return true;
		}
	}

	return false;
}

void CLadderLobby::UpdatePlayerXP( SLadderGameInfo *pGameInfo, const int nPlayerID, SLadderDBInfo *pClientInfo, const bool bWin )
{
	if ( pGameInfo->updatedPlayers.find( nPlayerID ) != pGameInfo->updatedPlayers.end() )
		return;
	pGameInfo->updatedPlayers.insert( nPlayerID );

	int nPlayerTeam = 2;
	for ( list<int>::const_iterator it = pGameInfo->team1Players.begin(); it != pGameInfo->team1Players.end(); ++it )
	{
		if ( nPlayerID == *it )
			nPlayerTeam = 1;
	}

	int nXP = bWin ? pGameInfo->winXP[nPlayerTeam] : ( - pGameInfo->loseXP[nPlayerTeam] );

	if ( pClientInfo->nLevel <= pConsts->experience.nNewbieLevel && nXP < 0 )
		nXP *= pConsts->experience.lossFactor[ pClientInfo->nLevel ];
	pClientInfo->nXP += nXP;

	if ( nXP > 0 )
	{
		if ( pClientInfo->nMaxXPEarned < nXP )
			pClientInfo->nMaxXPEarned = nXP;	
	}
	else
	{
		if ( pClientInfo->nMaxXPLost < ( -nXP ) ) 
			pClientInfo->nMaxXPLost = -nXP;
	}
	if ( pClientInfo->nXP < 0 ) 
		pClientInfo->nXP = 0;

	for ( int i = pConsts->experience.levelXP.size() - 1; i >= 0; --i )
	{
		if ( pClientInfo->nXP >= pConsts->experience.levelXP[i] )
		{
			pClientInfo->nLevel = i + 1;
			break;
		}
	}

}

void CLadderLobby::CalcResults( const int nGameID )
{
#ifdef LADDER_TEST
	DebugTrace( "LADDER_TEST: Game %d finished. Calculating results.", nGameID );
#endif

	SLadderGameInfo &gameInfo = games[ nGameID ];
	const int nTeamSize = gameInfo.team1Players.size();

	list<int> players = gameInfo.team1Players;
	list<int> team2players = gameInfo.team2Players;
	players.splice( players.end(), team2players );

	if ( CheckGameResultIsFake( nGameID ) )
	{
		for ( list<int>::const_iterator it = players.begin(); it != players.end(); ++it )
		{
			const int nPlayerID = *it;
			CPtr<CNetPacket> pPacketToSend = new CLadderInvalidStatisticsPacket( nPlayerID, nGameID, gameInfo.eInvalidReason );
			PushPacket( pPacketToSend );
		}
		return;
	}

	const float fWinModifier = nTeamSize / ( ( !gameInfo.winners.empty() ) ? gameInfo.winners.size() : 1 );
	gameInfo.winXP[1] *= fWinModifier;
	gameInfo.winXP[2] *= fWinModifier;

	// Changing players info depending on the game result
	for ( list<int>::iterator it = players.begin(); it != players.end(); ++it )
	{
		const int nPlayerID = *it;
		const bool bWin = gameInfo.winners.count( nPlayerID ) == 1;
		int nPlayerTeam = 2;
		for ( list<int>::const_iterator it = gameInfo.team1Players.begin(); it != gameInfo.team1Players.end(); ++it )
		{
			if ( nPlayerID == *it )
				nPlayerTeam = 1;
		}

		CPtr<SLadderDBInfo> pClientInfo = gameInfo.playerInfo[nPlayerID];
		UpdatePlayerXP( &gameInfo, nPlayerID, pClientInfo, bWin );

		if ( bWin )
		{
      if ( nTeamSize == 1 )
					++pClientInfo->raceWinsSolo[ gameInfo.playerRaces[nPlayerID] ];
			else
				++pClientInfo->raceWinsTeam[ gameInfo.playerRaces[nPlayerID] ];
			++pClientInfo->nWinsInSeries;
		}
		else
		{
			if ( nTeamSize == 1 )
				++pClientInfo->raceLossesSolo[ gameInfo.playerRaces[nPlayerID] ];
			else
				++pClientInfo->raceLossesTeam[ gameInfo.playerRaces[nPlayerID] ];
			pClientInfo->nWinsInSeries = 0;
		}

		pClientInfo->nUnitsKilled += gameInfo.unitsKilled[ nPlayerID ];
		pClientInfo->nUnitsLost += gameInfo.unitsLost[ nPlayerID ];

		pClientInfo->nTotalPlayTime += int( gameInfo.nDeathTime - gameInfo.nStartTime ) / 1000;
		++pClientInfo->nTotalGamesPlayed;

		pClientInfo->nUnitsEffectiveness = 
			( pClientInfo->nUnitsEffectiveness * ( pClientInfo->nTotalGamesPlayed - 1 ) + gameInfo.playerUnitsEff[ nPlayerID ] ) / pClientInfo->nTotalGamesPlayed;
		pClientInfo->nKeyPointsEffectiveness =
			( pClientInfo->nKeyPointsEffectiveness * ( pClientInfo->nTotalGamesPlayed - 1 ) + gameInfo.playerKeyPointEff[ nPlayerID ] ) / pClientInfo->nTotalGamesPlayed;

		if ( pClientInfo->mapsPlayed.size() < ( gameInfo.nMapID + 1 ) )
			pClientInfo->mapsPlayed.resize( gameInfo.nMapID + 1, 0 );
		++pClientInfo->mapsPlayed[ gameInfo.nMapID ];

		if ( pClientInfo->techsPlayed.size() < ( gameInfo.nTechLevel + 1 ) )
			pClientInfo->techsPlayed.resize( gameInfo.nTechLevel + 1, 0 );
		++pClientInfo->techsPlayed[ gameInfo.nTechLevel ]; 

		if ( bWin )
		{
			list<int> &enemyPlayers = ( nPlayerTeam == 1 ) ? gameInfo.team2Players : gameInfo.team1Players;
			for ( list<int>::const_iterator it = enemyPlayers.begin(); it != enemyPlayers.end(); ++it )
				++pClientInfo->winsAgainst[ gameInfo.playerRaces[*it] ];
		}
		else
		{
			list<int> &enemyPlayers = ( nPlayerTeam == 1 ) ? gameInfo.team2Players : gameInfo.team1Players;
			for ( list<int>::const_iterator it = enemyPlayers.begin(); it != enemyPlayers.end(); ++it )
				if ( gameInfo.winners.find( *it ) != gameInfo.winners.end() )
					++pClientInfo->lossesAgainst[ gameInfo.playerRaces[*it] ];
		}

		const vector<int> &reinfUsed = gameInfo.reinfUsed[nPlayerID];
		if ( pClientInfo->reinforcementUsed.size() < reinfUsed.size() )
			pClientInfo->reinforcementUsed.resize( reinfUsed.size(), 0 );
		for ( int i = 0; i < reinfUsed.size(); ++i )
			pClientInfo->reinforcementUsed[ i ] += reinfUsed[ i ];

		pClients->PutLadderInfoToDB( gameInfo.nickByID[ nPlayerID ] );

#ifdef LADDER_TEST
		DebugTrace( "LADDER_TEST: %s gaines %d xp(%d total). His level now is %d.", gameInfo.nickByID[nPlayerID].c_str(), nXP, pClientInfo->nXP, pClientInfo->nLevel );
#endif
	}

	for ( list<int>::iterator it = players.begin(); it != players.end(); ++it )
	{
		const int nPlayerID = *it;
		CPtr<SLadderDBInfo> pClientInfo = gameInfo.playerInfo[nPlayerID];

		if ( CheckMedals( pClientInfo, gameInfo.unitsKilled[ nPlayerID ], gameInfo.unitsLost[ nPlayerID ] ) )
			pClients->PutLadderInfoToDB( gameInfo.nickByID[ nPlayerID ] );
		SendLadderInfoToPlayer( nPlayerID, gameInfo.nickByID[ nPlayerID ], true ); 
	}

	pClients->DBLogGameResult( &gameInfo );

}

bool CLadderLobby::CheckMedals( SLadderDBInfo *pInfo, const int nUnitsKilledInLastGame, const int nUnitsLostInLastGame )
{
	bool bNewMedals = false;
	const int nRace = pInfo->GetFavouriteRace();
	// KILLED - medals
	int nArrayIndex = 0;
	for ( int nMedalType = SLadderStatistics::EMedalTypes::KILLED_3; nMedalType <= SLadderStatistics::EMedalTypes::KILLED_1; ++nMedalType )
	{
		if ( !SLadderStatistics::HasMedal( pInfo->medals, nRace, nMedalType ) && pInfo->nUnitsKilled > pConsts->killedMedals[nArrayIndex] )
		{
			SLadderStatistics::AddMedal( &pInfo->medals, nRace, nMedalType );
			bNewMedals = true;
			break;
		}
		++nArrayIndex;
	}
	
	nArrayIndex = 0;
	for ( int nMedalType = SLadderStatistics::EMedalTypes::KILLED_LOST_3; nMedalType <= SLadderStatistics::EMedalTypes::KILLED_LOST_1; ++nMedalType )
	{
		if ( !SLadderStatistics::HasMedal( pInfo->medals, nRace, nMedalType ) &&
			nUnitsKilledInLastGame > pConsts->killedLostMedals[nArrayIndex] * nUnitsLostInLastGame )
		{
			SLadderStatistics::AddMedal( &pInfo->medals, nRace, nMedalType );
			bNewMedals = true;
			break;
		}
		++nArrayIndex;
	}

	nArrayIndex = 0;
	for ( int nMedalType = SLadderStatistics::EMedalTypes::WIN_SERIES_3; nMedalType <= SLadderStatistics::EMedalTypes::WIN_SERIES_1; ++nMedalType )
	{
		if ( !SLadderStatistics::HasMedal( pInfo->medals, nRace, nMedalType ) && pInfo->nWinsInSeries >= pConsts->winsInSeriesMedals[nArrayIndex] )
		{
			SLadderStatistics::AddMedal( &pInfo->medals, nRace, nMedalType );
			bNewMedals = true;
			break;
		}
		++nArrayIndex;
	}

	if ( !SLadderStatistics::HasMedal( pInfo->medals, nRace, SLadderStatistics::EMedalTypes::FIRST_PLACE ) &&
		pInfo->nXP >= pClients->GetMaxXP() && pInfo->nLevel >= pConsts->nFirstPlaceMedalLevel )
	{
		SLadderStatistics::AddMedal( &pInfo->medals, nRace, SLadderStatistics::EMedalTypes::FIRST_PLACE );
		bNewMedals = true;
	}
	return bNewMedals;
}

bool CLadderLobby::ProcessSurrenderPacket( CLadderSurrenderPacket *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;
	const int nPlayerID = pPacket->nClientID;
	const int nGameID = pPacket->nGameID;
	
	hash_map< int, SLadderGameInfo >::iterator it = games.find( nGameID );
	if ( it != games.end() )
	{
		SLadderGameInfo &gameInfo = it->second;
		CPtr<SLadderDBInfo> pClientInfo = gameInfo.playerInfo[nPlayerID];
		UpdatePlayerXP( &gameInfo, nPlayerID, pClientInfo, false );
	}
	string szNick;
	if ( pClients->GetNick( nPlayerID, &szNick ) )
	{
		SendLadderInfoToPlayer( nPlayerID, szNick, true );
	}
	return true;
}

bool CLadderLobby::MatchMakingStep()
{
	if ( waitingList.empty() )
		return false;

	int nFirstPlayerID = waitingList.front();
	waitingList.pop_front();

	list<int> players = waitingList;
	
	CPtr<CLadderClient> pPlayer = ladderClients[nFirstPlayerID];

	SLadderGameInfo gameInfo;

	const int nTeamSize = ( pPlayer->nTeamSize != 0 ) ? ( pPlayer->nTeamSize ) : NRandom::Random( 1, pConsts->nMaxTeamSize );
	const int nTeam1Side = ( pPlayer->nSide != 0 ) ? ( pPlayer->nSide ) : NRandom::Random( 1, 2 );
	const bool bHistoricity = pPlayer->bHistoricity;
	const int nLevel = pPlayer->nLevel;
	const unsigned int uCheckSum = pPlayer->uCheckSum;
 	
	NLadder::DropFromListByTeamSize( ladderClients, &players, nTeamSize );
	NLadder::DropFromListByHistoricityAndChecksum( ladderClients, &players, bHistoricity, uCheckSum );
	NLadder::DropFromListByLevel( ladderClients, &players, nLevel, 2 * pConsts->nLevelDelta, 2 * pConsts->nLevelDelta );

	if ( players.empty() )
	{
		waitingList.push_back( nFirstPlayerID );
		return false;
	}

	const int nPlayers = players.size();
	list<int> maps;
	list<int> techLevels;
	for ( hash_set<int>::iterator it = pPlayer->maps.begin(); it != pPlayer->maps.end(); ++it )
	{
		maps.push_back( *it );
	}
	for ( hash_set<int>::iterator it = pPlayer->techLevels.begin(); it != pPlayer->techLevels.end(); ++it )
	{
		techLevels.push_back( *it );
	}
	NLadder::RandomizeList( &maps );
	NLadder::RandomizeList( &techLevels );

	for ( list<int>::iterator mapIt = maps.begin(); mapIt != maps.end(); ++mapIt )
	{
		for ( list<int>::iterator tlIt = techLevels.begin(); tlIt != techLevels.end(); ++tlIt )
		{

			gameInfo.nMapID = *mapIt;
			gameInfo.nTechLevel = *tlIt;
			gameInfo.bHistoricity = bHistoricity;

			for ( int nMistakes = 0; nMistakes < ( pConsts->nMistakes * nPlayers ); ++nMistakes )
			{
				gameInfo.team1Players.clear();
				gameInfo.team2Players.clear();
				
				hash_map<int, list<int> > candidates;
				// Распихиваем игроков по командам, тех, кто не выбрал команду - распихиваем случайно
				// Если историчность отключена - всех случайно
				for ( list<int>::iterator it = players.begin(); it != players.end(); ++it )
				{
					int nPlayerID = *it;
					int nSide = ladderClients[nPlayerID]->nSide;

					if ( !ladderClients[nPlayerID]->CanPlay( gameInfo.nMapID, gameInfo.nTechLevel, pConsts ) )
						continue;

					if ( bHistoricity && nSide ) 
					{
						candidates[ nSide == nTeam1Side ? 1 : 2 ].push_back( *it );
					}
					else
					{
						candidates[ NRandom::Random( 1, 2 ) ].push_back( *it );
					}
				}

				if ( candidates[1].size() + candidates[2].size() + 1 < 2 * nTeamSize )
					break;

				NLadder::RandomizeList( &candidates[1] );
				NLadder::RandomizeList( &candidates[2] );
				// Если получилось слишком мало кандидатов в одну из команд - попробуем еще раз

        while ( candidates[1].size() >= ( nTeamSize - 1 ) && candidates[2].size() >= nTeamSize )
				{

					// Заполняем команды кандидатами
					gameInfo.team1Players.push_back( nFirstPlayerID );
					while( ( gameInfo.team1Players.size() < nTeamSize && !candidates[1].empty() ) 
						|| ( gameInfo.team2Players.size() < nTeamSize && !candidates[2].empty() ) )
					{
						if ( gameInfo.team1Players.size() < nTeamSize )
						{
							gameInfo.team1Players.push_back( candidates[1].front() );
							candidates[1].pop_front();
   					}
						if ( gameInfo.team2Players.size() < nTeamSize )
						{
							gameInfo.team2Players.push_back( candidates[2].front() );
							candidates[2].pop_front();
						}
					}

					if ( gameInfo.team1Players.size() < nTeamSize || gameInfo.team2Players.size() < nTeamSize )
						continue; 				// Кандидатов не хватило :(
					
					const float nTeam1Level = NLadder::TeamLevel( ladderClients, gameInfo.team1Players );
					const float nTeam2Level = NLadder::TeamLevel( ladderClients, gameInfo.team2Players );

					if ( fabs( nTeam2Level - nTeam1Level ) <= pConsts->nLevelDelta )
					{
						// Команды подобраны. Начинаем игру.
						CreateLadderGame( gameInfo );

#ifndef CONSOLE_LOG_SILENCE
						WriteMSG( "Game for %d players created. Players waiting %d.\n", 2 * nTeamSize, waitingList.size() );
#endif
						return true;
					}
						
					list<int> &weakTeam = ( nTeam1Level < nTeam2Level ) ? gameInfo.team1Players : gameInfo.team2Players;
					list<int> &strongTeam = ( nTeam1Level > nTeam2Level ) ? gameInfo.team1Players : gameInfo.team2Players;

					// Так как команды получились сильно разного уровня, то либо выкидываем слабейшего из слабой,
					//  либо выкидываем сильнейшего из сильной
					if ( NRandom::Random( 0, 1 ) )
					{
						NLadder::DropWeakestExcept( ladderClients, &weakTeam, nFirstPlayerID );
					}
					else
					{
						NLadder::DropStrongestExcept( ladderClients, &strongTeam, nFirstPlayerID );
					}
				}
			}
			
		}
	}
	waitingList.push_back( nFirstPlayerID );
	return false;
}

void CLadderLobby::CreateLadderGame( const SLadderGameInfo &_gameInfo )
{
	(*pStatisticsCollector)[ "GamesTotal" ]->Add( 1.0f );
	(*pStatisticsCollector)[ "GamesPerSecond" ]->Add( 1.0f );
	const int nTeamSize = _gameInfo.team1Players.size() + _gameInfo.team2Players.size();
	const int nGameID = CreateGame( 2 * nTeamSize );
	games[ nGameID ] = _gameInfo;
	SLadderGameInfo &gameInfo = games[ nGameID ];
	gameInfo.bIsDead = false;
	gameInfo.nDeathTime = 0;
	gameInfo.winners.clear();
	gameInfo.playerRaces.clear();
	gameInfo.nStartTime = GetLongTickCount();
	gameInfo.bInvalid = false;
	gameInfo.eInvalidReason = SLadderGameInfo::EInvalidReason::OTHER_ERROR;
	list<int> players = gameInfo.team1Players;
	list<int> team2players = gameInfo.team2Players;
	players.splice( players.end(), team2players );
	const UINT64 nTime = GetLongTickCount();
	for ( list<int>::iterator it = players.begin(); it != players.end(); ++it )
	{
		const int nID = *it;
		PushPacket( new CLadderInvitePacket( nID, nGameID, gameInfo.nMapID, gameInfo.nTechLevel, gameInfo.team1Players, gameInfo.team2Players ) );

		for ( hash_set<int>::iterator it = ladderClients[nID]->maps.begin(); it != ladderClients[nID]->maps.begin(); ++it )
		{
			gameInfo.mapsRequested[nID].push_back( *it );
		}
		for ( hash_set<int>::iterator it = ladderClients[nID]->techLevels.begin(); it != ladderClients[nID]->techLevels.begin(); ++it )
		{
			gameInfo.techsRequested[nID].push_back( *it );
		}

		waitingList.remove( nID );
		const string &szNick = nickByID[ nID ];
		gameInfo.nickByID[ nID ] = szNick;
		gameInfo.playerInfo[nID] = pClients->GetLadderInfoFromDB( szNick );

		gameInfo.lockers.push_back( new CLadderCacheLocker( pClients, szNick ) );
		//		gameInfo.playerInfo[ nID ] = ladderClients[ nID ]->dbInfo;

		(*pStatisticsCollector)[ "AverageWaitTime" ]->Add( float( nTime - ladderClients[nID]->nStartTime ) / 1000 );
	}

	// Calculating average team level
	for ( list<int>::iterator it = gameInfo.team1Players.begin(); it != gameInfo.team1Players.end(); ++it )
	{
		gameInfo.teamLevels[1] += gameInfo.playerInfo[*it]->nLevel;
	}
	for ( list<int>::iterator it = gameInfo.team2Players.begin(); it != gameInfo.team2Players.end(); ++it )
	{
		gameInfo.teamLevels[2] += gameInfo.playerInfo[*it]->nLevel;
	}
	gameInfo.teamLevels[1] /= ( !gameInfo.team1Players.empty() ) ? gameInfo.team1Players.size() : 1;
	gameInfo.teamLevels[2] /= ( !gameInfo.team2Players.empty() ) ? gameInfo.team2Players.size() : 1;

	// Calculating winner's and loser's XP
	const int nLevelDiff = gameInfo.teamLevels[2] - gameInfo.teamLevels[1];
	gameInfo.winXP[1] = pConsts->experience.winXP[nLevelDiff];
	gameInfo.winXP[2] = pConsts->experience.winXP[-nLevelDiff];
	gameInfo.loseXP[1] = pConsts->experience.loseXP[nLevelDiff];
	gameInfo.loseXP[2] = pConsts->experience.loseXP[-nLevelDiff];

	games[ nGameID ] = gameInfo;
#ifdef LADDER_TEST
	DebugTrace( "LADDER_TEST: Game %d created. TeamSize = %d, TechLevel = %d, MapID = %d", nGameID, gameInfo.team1Players.size(),
		gameInfo.nTechLevel, gameInfo.nMapID );
#endif
}

void CLadderLobby::ClientEnterToLobby( const int nClientID )
{
	string szNick;
	pClients->GetNick( nClientID, &szNick );
	nickByID[ nClientID ] = szNick;
	(*pStatisticsCollector)[ "TotalPlayersEntered" ]->Add( 1.0f );
}

void CLadderLobby::ClientLeaveLobby( const int nClientID )
{
	ladderClients.erase( nClientID );
	waitingList.remove( nClientID );
	nickByID.erase( nClientID );
}

bool CLadderLobby::ProcessLadderInfoPacket( CLadderInfoPacket *pPacket )
{
	if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;

	(*pStatisticsCollector)[ "RequestsPerSecond" ]->Add( 1.0f );

	CPtr<CLadderClient> pClient = new CLadderClient();
	pClient->nSide = pPacket->nSide;
	pClient->bHistoricity = pPacket->bHistoricity;
	pClient->uCheckSum = pPacket->uCheckSum;
	for ( list<int>::iterator it = pPacket->techLevels.begin(); it != pPacket->techLevels.end(); ++it )
	{
		if ( *it < 0 || *it > MAX_TECHLEVELS )
			return false;
		pClient->techLevels.insert( *it );
	}
	for ( list<int>::iterator it = pPacket->maps.begin(); it != pPacket->maps.end(); ++it )
	{
		if ( *it < 0 || *it > MAX_MAPS )
			return false;
		pClient->maps.insert( *it );
	}
	pClient->nTeamSize = pPacket->nTeamSize;
	pClient->nStartTime = GetLongTickCount();

	{
		pClient->nLevel = pClients->GetLadderInfoFromDB( nickByID[ pPacket->nClientID ] )->nLevel;
	}

	ladderClients[ pPacket->nClientID ] = pClient;
	waitingList.remove( pPacket->nClientID );
	waitingList.push_back( pPacket->nClientID );

#ifdef LADDER_TEST
	string szText = StrFmt( "LADDER_TEST: Player %s ladder info received: " , nickByID[ pPacket->nClientID ].c_str() );
	szText += "TechLevels";
	for ( list<int>::iterator it = pPacket->techLevels.begin(); it != pPacket->techLevels.end(); ++it )
	{
		szText = szText + StrFmt( " %d", *it );
	}
	szText += ", Maps";
	for ( list<int>::iterator it = pPacket->maps.begin(); it != pPacket->maps.end(); ++it )
	{
		szText = szText + StrFmt( " %d", *it );
	}
	szText += StrFmt( ", Side %d, Historicity %d", pPacket->nSide, pPacket->bHistoricity ? 1 : 0 );
	DebugTrace( szText.c_str() );
#endif

	return true;
}

bool CLadderLobby::ProcessLadderGameResultPacket( CLadderGameResultPacket *pPacket )
{
  if ( !IsLobbyClient( pPacket->nClientID ) )
		return false;

	if ( games.find( pPacket->nGameID ) == games.end() )
		return true;
	SLadderGameInfo &gameInfo = games[ pPacket->nGameID ];
	if ( gameInfo.team1Players.size() + gameInfo.team2Players.size() != pPacket->races.size()
		|| pPacket->winners.size() > gameInfo.team1Players.size() )
	{
#ifdef LADDER_TEST
		DebugTrace( "LADDER_TEST: Corrupted statistics of game %d ignored", pPacket->nGameID );
#endif
		return true;
	}

	if ( !gameInfo.winners.empty() || !gameInfo.playerRaces.empty() )
	{
		if ( ( ! ( gameInfo.winners == pPacket->winners ) ) || ( ! ( gameInfo.playerRaces == pPacket->races ) ) )
		{
#ifdef LADDER_TEST
			DebugTrace( "LADDER_TEST: Statistics difference detected. Game id = %d results ignored.", pPacket->nGameID );
#endif
			games[ pPacket->nGameID ].bInvalid = true;
			games[ pPacket->nGameID ].eInvalidReason = SLadderGameInfo::EInvalidReason::DIFFERENCE_DETECTED;
		}
	}
	else
	{
#ifdef LADDER_TEST
		DebugTrace( "LADDER_TEST: %d game info stored. %d winners, %d players total.", pPacket->nGameID, pPacket->winners.size(), pPacket->races.size() );
#endif
		gameInfo.winners = pPacket->winners;
		gameInfo.playerRaces = pPacket->races;
		gameInfo.reinfUsed = pPacket->reinfUsed;
		gameInfo.playerUnitsEff = pPacket->playerUnitEff;
		gameInfo.playerKeyPointEff = pPacket->playerKeyPointEff;
		gameInfo.unitsKilled = pPacket->unitsKilled;
		gameInfo.unitsLost = pPacket->unitsLost;
		for ( hash_map<int,vector<int> >::iterator it = gameInfo.reinfUsed.begin(); it != gameInfo.reinfUsed.end(); ++it )
		{
			if ( it->second.size() > pConsts->nMaxUnitTypes )
			{
				gameInfo.reinfUsed.clear();
				break;
			}
		}
	}

	return true;
}

bool CLadderLobby::ProcessLadderStatisticsRequestPacket( CLadderStatisticsRequestPacket *pPacket )
{
	SendLadderInfoToPlayer( pPacket->nClientID, pPacket->szNick, pPacket->bSendFullStatistics );
	return true;
}

void CLadderLobby::SendLadderInfoToPlayer( const int nClientID, const string& szClientNick, const bool bFullStatistics )
{
	if ( bFullStatistics )
	{
		CPtr<SLadderDBInfo> pInfo = pClients->GetLadderInfoFromDB( szClientNick );
		SLadderStatistics infoToSend;
		pInfo->ConvertForClient( &infoToSend );
		const int nLevel = infoToSend.nLevel;
		if ( nLevel < pConsts->experience.levelXP.size() && nLevel > 0 )
		{
			if ( nLevel >= pConsts->experience.levelXP.size() )
			{
				infoToSend.nNextLevelXP = pInfo->nXP;
			}
			else
			{
				infoToSend.nNextLevelXP = pConsts->experience.levelXP[pInfo->nLevel];
			}
			infoToSend.nLevelXP = pConsts->experience.levelXP[pInfo->nLevel - 1];
		}
		CPtr<CNetPacket> pPacketToSend = new CLadderStatisticsPacket( nClientID, szClientNick, infoToSend );
		PushPacket( pPacketToSend );
	}
	else
	{
		CPtr<SLadderDBInfo> pInfo = pClients->GetLadderInfoFromDB( szClientNick );

		CPtr<CNetPacket> pPacketToSend = new CLadderShortStatisticsPacket( nClientID, szClientNick, pInfo->nLevel, pInfo->GetFavouriteRace() );
		PushPacket( pPacketToSend );
	}
}

void CLadderLobby::GameDead( const int nGameID )
{
	games[nGameID].bIsDead = true;
	games[nGameID].nDeathTime = GetLongTickCount();
#ifdef LADDER_TEST
	DebugTrace( "LADDER_TEST: Game %d is dead. Waiting for game results", nGameID );
#endif
}

//   CLadderClient

bool CLadderClient::CanPlay( int nMapID, int nTechLevel, const CLadderConsts *pConsts )
{
	int nErrorLevel = 0;
  if ( maps.find( nMapID ) == maps.end() )
		++nErrorLevel;
	if ( techLevels.find( nTechLevel ) == techLevels.end() )
		++nErrorLevel;
	if ( nErrorLevel == 0 )
		return true;
	const UINT64 nTime = GetLongTickCount();
	if ( nTime > nStartTime + pConsts->nWaitTime1 && nErrorLevel == 1 )
		return true;
	if ( nTime > nStartTime + pConsts->nWaitTime2 )
		return true;
	return false;
}

//   Helpful functions

namespace NLadder{
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void DropFromListByLevel( const hash_map< int, CPtr<CLadderClient> > &players, list<int> *pWaitingOrder,
			const int nLevel, const int nDeltaPlus, const int nDeltaMinus )
	{
		list<int>::iterator itNext = pWaitingOrder->begin();
		list<int>::iterator it = itNext++;
		for ( ; it != pWaitingOrder->end(); it = itNext++ )
		{
			int nID = *it;
			CLadderClient *pClient = players.find( nID )->second;
			int nPlayerLevel = pClient->nLevel;
			if ( ( nPlayerLevel < nLevel - nDeltaMinus ) || ( nPlayerLevel > nLevel + nDeltaPlus ) )
				pWaitingOrder->erase( it );
		}
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void DropFromListByTeamSize( const hash_map< int, CPtr<CLadderClient> > &players, list<int> *pWaitingOrder,
		const int nTeamSize )
	{
		list<int>::iterator itNext = pWaitingOrder->begin();
		list<int>::iterator it = itNext++;
		for ( ; it != pWaitingOrder->end(); it = itNext++ )
		{
			int nID = *it;
			CLadderClient *pClient = players.find( nID )->second;
			if ( pClient->nTeamSize != nTeamSize && pClient->nTeamSize != 0 )
				pWaitingOrder->erase( it );
		}
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void DropFromListByHistoricityAndChecksum( const hash_map< int, CPtr<CLadderClient> > &players, list<int> *pWaitingOrder,
		const bool bHistoricity, const unsigned int uCheckSum )
	{
		list<int>::iterator itNext = pWaitingOrder->begin();
		list<int>::iterator it = itNext++;
		for ( ; it != pWaitingOrder->end(); it = itNext++ )
		{
			int nID = *it;
			CLadderClient *pClient = players.find( nID )->second;
			if ( pClient->bHistoricity != bHistoricity || pClient->uCheckSum != uCheckSum )
				pWaitingOrder->erase( it );
		}
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void DropFromListBySide( const hash_map< int, CPtr<CLadderClient> > &players, list<int> *pWaitingOrder,
		const int nSide )
	{
		list<int>::iterator itNext = pWaitingOrder->begin();
		list<int>::iterator it = itNext++;
		for ( ; it != pWaitingOrder->end(); it = itNext++ )
		{
			int nID = *it;
			CLadderClient *pClient = players.find( nID )->second;
			if ( pClient->nSide != nSide )
				pWaitingOrder->erase( it );
		}
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	float TeamLevel( const hash_map< int, CPtr<CLadderClient> > &players, const list<int> &team )
	{
		float fLevel = 0;
		for ( list<int>::const_iterator it = team.begin(); it != team.end(); ++it )
		{
			int nID = *it;
			fLevel += players.find( nID )->second->nLevel;
		}
		return fLevel / team.size();
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void RandomizeList( list<int> *pList )
	{
		int nSize = pList->size();
		vector<int> tempVector( nSize );
		int nMaxRand = nSize - 1;
		for( int i = 0; i < nSize; ++i )
		{
			tempVector[ i ] = pList->front();
			pList->pop_front();
		}
		for( int i = 0; i < nSize; ++i )
		{
			int nRandom = NRandom::Random( 0, nMaxRand );
			int t = tempVector[nRandom];
			tempVector[nRandom] = tempVector[i];
			tempVector[i] = t;
		}
		for( int i = 0; i < nSize; ++i )
		{
			pList->push_back( tempVector[i] );
		}
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void DropWeakestExcept( const hash_map< int, CPtr<CLadderClient> > &players, list<int> *pList, const int nException )
	{
		int nMinLevel = 1e8;
		int nMinID = -1;
		for( list<int>::iterator it = pList->begin(); it != pList->end(); ++it )
		{
			int nID = *it;
			int nLevel = players.find( nID )->second->nLevel;
			if ( nLevel < nMinLevel && nID != nException )
			{
				nMinLevel = nLevel;
				nMinID = nID;
			}
		}
		pList->remove( nMinID );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void DropStrongestExcept( const hash_map< int, CPtr<CLadderClient> > &players, list<int> *pList, const int nException )
	{
		int nMaxLevel = -1;
		int nMaxID = -1;
		for( list<int>::iterator it = pList->begin(); it != pList->end(); ++it )
		{
			int nID = *it;
			int nLevel = players.find( nID )->second->nLevel;
			if ( nLevel > nMaxLevel && nID != nException )
			{
				nMaxLevel = nLevel;
				nMaxID = nID;
			}
		}
		pList->remove( nMaxID );
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

#undef STEP_LENGTH_TIME

BASIC_REGISTER_CLASS( CLadderClient ) 

