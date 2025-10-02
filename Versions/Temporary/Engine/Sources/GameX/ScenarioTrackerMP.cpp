#include "StdAfx.h"

#include "..\misc\2darray.h"
#include "..\zlib\zconf.h"
#include "..\stats_b2_m1\dbmapinfo.h"
#include "..\ui\commandparam.h"
#include "ScenarioTracker.hpp"
#include "../B2_M1_World/MissionObjectiveStates.h"
#include "GetConsts.h"
#include "InterfaceState.h"
#include "..\UISpecificB2\DBUISpecificB2.h"
#include "../AILogic/DBAIConsts.h"
#include "../Stats_B2_M1/RPGStats.h"
#include "../AILogic/B2AI.h"

extern int N_MAX_XP_LEVEL; // [0..]  defined in ScenarioTracker.cpp
const int NEUTRAL_COLOR_INDEX	= 3;
const int MP_COLOR_INDEX_BASE	= 4;

IScenarioTracker * CreateScenarioTrackerMultiplayer()
{
	return new NScenarioTracker::CScenarioTrackerMultiplayer;
}

namespace NScenarioTracker
{


// CScenarioTrackerMultiplayer


CScenarioTrackerMultiplayer::CScenarioTrackerMultiplayer() :
	nLocalPlayer( -1 ), eType( EGT_MULTI_FLAG_CONTROL )
{
}

const NDb::SReinforcement * CScenarioTrackerMultiplayer::GetReinforcement( int nPlayer, NDb::EReinforcementType eType ) const
{
	if ( players.size() <= nPlayer )
		return 0;

	CReinforcementTypes::const_iterator pos = players[nPlayer].reinforcementTypes.find( eType );
	if ( pos != players[nPlayer].reinforcementTypes.end() )
		return pos->second;
	return 0;
}

int CScenarioTrackerMultiplayer::GetReinforcementCallsLeft( int nPlayer )
{
	NI_VERIFY( nPlayer >= 0 && nPlayer < players.size(), "Index out of range", return 0 );
	if ( bNoKeyBuildings || eType == EGT_MULTI_FLAG_CONTROL )
		return players[nPlayer].nReinforcementCallsLeft;
	else
		return IAIScenarioTracker::INFINITE_CALLS;
}

void CScenarioTrackerMultiplayer::DecreaseReinforcementCallsLeft( int nPlayer, int nCalls )
{
	if ( !IsMissionActive() || !IsPlayerPresent( nPlayer ) || GetPlayerSide( nPlayer ) == 2 )
		return;

	int nActualCalls = ( nCalls == 0 ) ? 1 : nCalls;

	if ( nCalls == 0 )
		++players[nPlayer].nReinforcementCallsUsed;

	if ( bNoKeyBuildings || eType == EGT_MULTI_FLAG_CONTROL )
		players[nPlayer].nReinforcementCallsLeft = Max( players[nPlayer].nReinforcementCallsLeft - nActualCalls, 0 );
}

void CScenarioTrackerMultiplayer::RegisterReinforcementCall( int nPlayer, NDb::EReinforcementType eType )
{
	if ( !IsMissionActive() || !IsPlayerPresent( nPlayer ) || GetPlayerSide( nPlayer ) == 2 )
		return;

	DecreaseReinforcementCallsLeft( nPlayer, 0 );
	reinfCallsByType[nPlayer][eType] += 1;
}

void CScenarioTrackerMultiplayer::GetReinforcementCallsInfo( int nPlayer, vector<int> *pCallsByType )
{
	vector<int> &reinfUses = *pCallsByType;
	reinfUses.resize( NDb::_RT_NONE, 0 );

	if ( nPlayer < 0 || nPlayer >= reinfCallsByType.GetSizeY() )
		return;

	for ( int i = 0; i < NDb::_RT_NONE; ++i )
	{
		reinfUses[i] = reinfCallsByType[nPlayer][i];
	}
}

void CScenarioTrackerMultiplayer::MissionStart( const NDb::SMapInfo * _pMission, const int _nTechLevel ) 
{
	const NDb::SUIConstsB2 *pUIConsts = NGameX::GetUIConsts();

	ClearMissionScriptVars();

	pMission = _pMission;
	pLastMission = 0;
	bMissionWon = false;

	nSecondsToReinf = 0;

	nTechLevel = _nTechLevel;

	players.clear();
	players.resize( pMission->players.size() );

	reinfCallsByType.SetSizes( NDb::_RT_NONE, pMission->players.size() );
	reinfCallsByType.FillZero();
	
	// init neutral player color
	if ( pUIConsts )
	{
		playerColorNeutral.dwColor = pUIConsts->playersColors.neutralInfo.nColor | 0xFF000000;
		playerColorNeutral.pUnitFullInfo = pUIConsts->playersColors.neutralInfo.pUnitFullInfo;
		playerColorNeutral.nColorIndex = NEUTRAL_COLOR_INDEX;
	}

	kills.SetSizes( pMission->players.size(), pMission->players.size() );
	kills.FillZero();
	priceKills.SetSizes( pMission->players.size(), pMission->players.size() );
	priceKills.FillZero();

	if ( !pAIConsts )
	{
		// A hack for Main Menu background mission, which is started around Sc.Tracker...
		pAIConsts = NGameX::GetAIConsts();
		pMPConsts = NGameX::GetMPConsts();
	}
	NI_ASSERT( pAIConsts, "AI Consts inaccessible" );
	NI_ASSERT( pMPConsts, "MP Consts inaccessible" );

	// 
	nNeutralPlayer = -1;
	for ( int i = 0; i < pMission->players.size(); ++i )
	{
		SPlayerInfo &player = players[i];
		SearchAvalableReinforcements( pMission, &player.reinforcementTypes, 0, i );
		if ( eType == EGT_MULTI_FLAG_CONTROL )
		{
			player.nReinforcementCallsLeft = pMission->players[i].nReinforcementCalls;
		}
		else
		{
			if ( bNoKeyBuildings )
				player.nReinforcementCallsLeft = pMission->players[i].nReinforcementCalls;
			else
				player.nReinforcementCallsLeft = IAIScenarioTracker::INFINITE_CALLS;
		}
		player.nReinforcementCallsUsed = 0;

		if ( pMission->players[i].nDiplomacySide == 2 )			// Neutral side is always present
		{
			players[i].bPresent = true;
			nNeutralPlayer = i;
		}
		else
			players[i].bPresent = false;											// The rest have to be added explicitly

		players[i].timeKeyPointsOwned = 0;

		SetStatistics( i, ESK_SCORE, 0 );
		SetStatistics( i, ESK_REINFORCEMENTS_CALLED, 0 );

		// Set multiplayer sides
		players[i].nDiplomacySide = pMission->players[i].nDiplomacySide;
		players[i].nMultiplayerSide = -1;			// Default: use from map-info

		for ( int j = 0; j < pMPConsts->sides.size(); ++j )				// Find the side with the same PartyDependentInfo
		{
			if ( pMPConsts->sides[j].pPartyInfo == pMission->players[i].pPartyInfo )
			{
				players[i].nMultiplayerSide = j;
				break;
			}
		}

		UpdateReinforcements( i );
	}

	NI_ASSERT( nNeutralPlayer != -1, StrFmt( "No neutral player present on map \"%s\"", NDb::GetResName(pMission) ) );

	bNoKeyBuildings = true;
	// Init key buildings
	flags.clear();
	flagTimes.clear();
	for ( int i = 0; i < pMission->playerBonusObjects.size(); ++i )
	{
		flags[pMission->playerBonusObjects[i].nLinkID] = nNeutralPlayer;
		flagTimes[pMission->playerBonusObjects[i].nLinkID] = 0;
		bNoKeyBuildings = false;
	}
	timeMissionStart = Singleton<IGameTimer>()->GetSegmentTime();
}

void CScenarioTrackerMultiplayer::MissionCancel()
{
	UpdateStatistics();
	
	pLastMission = pMission;
	pMission = 0;
	bMissionWon = false;
}

void CScenarioTrackerMultiplayer::MissionWin()
{
	UpdateStatistics();
	
	NI_ASSERT( pMission != 0, "no mission started" );
	pLastMission = pMission;
	pMission = 0;
	bMissionWon = true;
}

void CScenarioTrackerMultiplayer::UpdateStatistics()
{
	for( int i = 0; i < players.size(); ++i )
	{
		if ( GetPlayerSide( i ) != 2 )
			CalculateScore( i );
	}
}

bool CScenarioTrackerMultiplayer::IsMissionActive() const
{
	return pMission != 0;
}

const NDb::SMapInfo * CScenarioTrackerMultiplayer::GetCurrentMission() const
{
	return pMission;
}

void CScenarioTrackerMultiplayer::ClearMissionScriptVars()
{
	const string szPrefix = "temp.";
	vector<string> globalVars;
	NGlobal::GetIDList( &globalVars );
	for ( vector<string>::iterator it = globalVars.begin(); it != globalVars.end(); ++it )
	{
		const string &szName = *it;
		if ( szName.compare( 0, szPrefix.size(), szPrefix ) == 0 )
		{
			NGlobal::RemoveVar( szName );
		}
	}
}

int CScenarioTrackerMultiplayer::GetNPlayers() const
{
	if ( GetCurrentMission() )
		return GetCurrentMission()->players.size() - 1;
	return 0;
}

int CScenarioTrackerMultiplayer::GetPlayerSide( int nPlayer ) const
{
	if ( players.size() > nPlayer )
		return players[nPlayer].nDiplomacySide;
	return 2;
}

enum EMissionObjectiveState CScenarioTrackerMultiplayer::GetObjectiveState( const int nID ) const 
{ 
	return EMOS_MIN; 
}

void CScenarioTrackerMultiplayer::AddPlayer( const int nPlayer )
{
	if ( nPlayer >= 0 && nPlayer < players.size() )
		players[nPlayer].bPresent = true;
}

void CScenarioTrackerMultiplayer::RemovePlayer( const int nPlayer )
{
	if ( nPlayer >= 0 && nPlayer < players.size() )
	{
		Singleton<IAILogic>()->NeutralizePlayer( nPlayer );
		players[nPlayer].bPresent = false;
	}
}

bool CScenarioTrackerMultiplayer::IsPlayerPresent( const int nPlayer ) const
{
	if ( nPlayer >= 0 && nPlayer < players.size() )
		return players[nPlayer].bPresent;
	else
		return false;
}

void CScenarioTrackerMultiplayer::KeyBuildingOwnerChange( const int nBuildingLinkID, const int nNewOwnerPlayer )
{
	if ( !pMission || !IsPlayerPresent( nNewOwnerPlayer ) )
		return;

	hash_map<int,int>::iterator pos = flags.find( nBuildingLinkID );
	if ( pos == flags.end() )
		return;
	int nOldPlayer = pos->second;

	NTimer::STime curTime = Singleton<IGameTimer>()->GetSegmentTime();

	NTimer::STime timeOwned = curTime - flagTimes[nBuildingLinkID];
	flagTimes[nBuildingLinkID] = curTime;

	if ( nOldPlayer != nNeutralPlayer )
		players[nOldPlayer].timeKeyPointsOwned += timeOwned;

	pos->second = nNewOwnerPlayer;
}

const int CScenarioTrackerMultiplayer::GetKeyBuildingOwner( const int nBuildingLinkID )
{
	hash_map<int,int>::const_iterator pos = flags.find( nBuildingLinkID );
	if ( pos == flags.end() )
		return nNeutralPlayer;
	else
		return pos->second;
}

const pair<int,int> CScenarioTrackerMultiplayer::GetKeyBuildingSummary()
{
	int nOwned0 = 0;
	int nOwned1 = 0;

	for ( hash_map<int,int>::const_iterator it = flags.begin(); it != flags.end(); ++it )
	{
		if ( GetPlayerSide( it->second ) == 0 )
			++nOwned0;
		else if ( GetPlayerSide( it->second ) == 1 )
			++nOwned1;
	}
	return pair<int,int>( nOwned0, nOwned1 );
}

#define FULL_RECYCLE_TIME 215.0f
#define MIN_RECYCLE_TIME 95.0f
const float CScenarioTrackerMultiplayer::GetRecycleSpeedCoeff( const int nSide )
{
	if ( !pMission || flags.empty() )
		return 1.0f;

	if ( eType == EGT_MULTI_FLAG_CONTROL )
	{
		float fCoeff = 1.0f;

		int nFlags = 0;
		int nOwnFlags = 0;

		for ( hash_map<int,int>::const_iterator it = flags.begin(); it != flags.end(); ++it )
		{
			++nFlags;
			if ( GetPlayerSide( it->second ) == nSide )
				++nOwnFlags;
		}

		float fFullTime = pMPConsts->vReinfCounterRecycle.y;
		float fMinTime = pMPConsts->vReinfCounterRecycle.x;
		if ( fFullTime < fMinTime )
			swap( fFullTime, fMinTime );
		if ( fFullTime == 0.0f )
			fFullTime = FULL_RECYCLE_TIME;
		if ( fMinTime == 0.0f )
			fMinTime = MIN_RECYCLE_TIME;
		
		fCoeff = ( fFullTime - ( fFullTime - fMinTime ) * nOwnFlags / nFlags )/* / fFullTime*/;

		return fCoeff;	// Time of recycle in seconds
	}

	if ( nSide == 2 || bNoKeyBuildings )
		return 1.0f;

	float fCoeff = -1.0f;
	for ( hash_map<int,int>::const_iterator it = flags.begin(); it != flags.end(); ++it )
	{
		if ( GetPlayerSide( it->second ) == nSide )
		{
			if ( fCoeff < 0.0f )
				fCoeff = 1.0f;
			else
				fCoeff *= 0.85f;
		}
	}

	return fCoeff;
}

int CScenarioTrackerMultiplayer::GetStatistics( int nPlayer, EStatisticsKind eKind ) const
{
	if ( nPlayer < statistic.size() && eKind < statistic[nPlayer].size() )
	{
		int nValue = statistic[nPlayer][eKind];
		NI_VERIFY( nValue >= 0, "Wrong statistics value", return 0 );
		return nValue;
	}
	NI_ASSERT( 0, "no statistics" );
	return 0;
}

void CScenarioTrackerMultiplayer::SetStatistics( int nPlayer, EStatisticsKind eKind, int nValue )
{
	NI_ASSERT( nValue >= 0, "Wrong statistics value" );

	if ( nPlayer >= statistic.size() )
		statistic.resize( nPlayer + 1 );

	if ( eKind >= statistic[nPlayer].size() )
		statistic[nPlayer].insert( statistic[nPlayer].end(), eKind + 1 - statistic[nPlayer].size(), 0 );

	statistic[nPlayer][eKind] = nValue;
}

CPtr<IScenarioTracker::SLeaderInfo> pInfo;

const IScenarioTracker::SLeaderInfo* CScenarioTrackerMultiplayer::GetLeaderInfo( const NDb::EReinforcementType eReinf ) const
{
	pInfo = new IScenarioTracker::SLeaderInfo();
	pInfo->info.fExp = GetReinforcementXP( GetLocalPlayer(), eReinf );
	pInfo->info.fExpDebt = 0.0f;
	pInfo->info.nRank = GetReinforcementXPLevel( GetLocalPlayer(), eReinf );
	pInfo->info.nUnitsKilled = 0;
	pInfo->info.nUnitsLost = 0;
	pInfo->pPicture = 0;
	pInfo->storedInfo.fExp = 0.0f;
	pInfo->storedInfo.fExpDebt = 0.0f;
	pInfo->storedInfo.nRank = 0;
	pInfo->storedInfo.nUnitsKilled = 0;
	pInfo->storedInfo.nUnitsLost = 0;
	pInfo->wszName = L"Mr.Noname";
	return pInfo;
}

float CScenarioTrackerMultiplayer::GetReinforcementXP( int nPlayer, NDb::EReinforcementType eType ) const
{
	if ( nPlayer == nLocalPlayer && eType == NDb::_RT_NONE )			// CRAP for designers' bastard baby - the recycle tooltip
	{
		if ( bNoKeyBuildings )
			return -1.0f;

		return nSecondsToReinf;
	}

	if ( NGlobal::GetVar( "multiplayer_unit_experience", 0 ) == 0 )
	{
		return 0.0f;
	}
	CReinforcementXPs::const_iterator pos = players[nPlayer].experience.find( eType );
	if ( pos != players[nPlayer].experience.end() )
		return pos->second;

	return 0.0f;
}

void CScenarioTrackerMultiplayer::SetReinforcementXP( int nPlayer, NDb::EReinforcementType eType, float fXP )
{
	if ( nPlayer == nLocalPlayer && eType == NDb::_RT_NONE )			// CRAP for designers' bastard baby - the recycle tooltip
	{
		nSecondsToReinf = fXP;
	}

	players[nPlayer].experience[eType] = fXP;
}

int CScenarioTrackerMultiplayer::GetReinforcementXPLevel( int nPlayer, NDb::EReinforcementType eType ) const
{
	if ( NGlobal::GetVar( "multiplayer_unit_experience", 0 ) == 0 )
	{
		return N_MAX_XP_LEVEL;
	}
#ifndef _FINALRELEASE
	if ( NGlobal::GetVar( "debug_unit_levels", 0 ) != 0 )
		return N_MAX_XP_LEVEL;
#endif _FINALRELEASE
	if ( eType == NDb::RT_ENGINEERING )
	{
		return N_MAX_XP_LEVEL;
	}

	CReinforcementLevels::const_iterator pos = players[nPlayer].reinfLevels.find( eType );
	if ( pos != players[nPlayer].reinfLevels.end() )
		return pos->second;

	return 0;
}

float CScenarioTrackerMultiplayer::GetReinforcementXPForLevel( NDb::EReinforcementType eType, int nLevel ) const
{
	if ( !pMPConsts )
		return 0.0f;

		for ( int i = 0; i < pMPConsts->expLevels.size(); ++i ) 
	{
		const NDb::SAIExpLevel *pLevels = pMPConsts->expLevels[i];
		if ( !pLevels )
			continue;
		if ( pLevels->eDBType != eType )
			continue;
		int nCheckedLevel = Min( pLevels->levels.size() - 1, nLevel );
		if ( nCheckedLevel < 0 )
			break;

		// Get it
		return pLevels->levels[nCheckedLevel].fExperience;
	}

	return 0.0f;
}

bool CScenarioTrackerMultiplayer::AddReinfExp( float *pExp, const int nPlayer, NDb::EReinforcementType eReinfType )
{
	bool bResult = false;
	int nLevel = GetReinforcementXPLevel( nPlayer, eReinfType );
	if ( nLevel < N_MAX_XP_LEVEL && *pExp > 0.0f )
	{
		while ( *pExp > 0.0f && nLevel < N_MAX_XP_LEVEL )
		{
			const float fXP = GetReinforcementXP( nPlayer, eReinfType );
			const float fXPToNextLevel = GetReinforcementXPForLevel( eReinfType, nLevel + 1 ) - fXP;
      const float fXPToAdd = min( *pExp, fXPToNextLevel );
			SetReinforcementXP( nPlayer, eReinfType, fXP + fXPToAdd );
			if ( fXPToNextLevel == fXPToAdd )
			{
				bResult = true;
				++nLevel;
			}
      *pExp -= fXPToAdd;
		}
		players[nPlayer].reinfLevels[eReinfType] = nLevel;
	}
	return bResult;
}

bool CScenarioTrackerMultiplayer::RegisterUnitKill( const SKillInfo &info )
{
	if ( !pAIConsts )
	{
		// A hack for Main Menu background mission, which is started around Sc.Tracker...
		pAIConsts = NGameX::GetAIConsts();
		pMPConsts = NGameX::GetMPConsts();
	}

	// register kill
	if ( info.nPlayer < kills.GetSizeY() && info.nKilledUnitPlayer < kills.GetSizeX() )
		kills[info.nPlayer][info.nKilledUnitPlayer] += info.bInfantryKill ? 0.1f : 1.0f;
	if ( info.nPlayer < priceKills.GetSizeY() && info.nKilledUnitPlayer < priceKills.GetSizeX() )
		priceKills[info.nPlayer][info.nKilledUnitPlayer] += info.fExpPrice;

	if ( info.eReinfType == NDb::_RT_NONE )
		return false;

	bool bResult = false;

	// give reinf exp
	float fRestReinfExp = info.fExpPrice;
	bResult = bResult || AddReinfExp( &fRestReinfExp, info.nPlayer, info.eReinfType );

	// split rest of exp
	fRestReinfExp *= pAIConsts->common.fExpReinfDistributionCoeff;
	if ( fRestReinfExp > 0.0f )
	{
		int nCount = 0;
		for ( CReinforcementTypes::iterator it = players[info.nPlayer].reinforcementTypes.begin();
			it != players[info.nPlayer].reinforcementTypes.end(); ++it )
		{
			const int nReinfType = it->first;
			if ( GetReinforcementXPLevel( info.nPlayer, (NDb::EReinforcementType)nReinfType ) < N_MAX_XP_LEVEL )
				++nCount;
		}
		if ( nCount > 0 )
		{
			const float fExpPart = fRestReinfExp / nCount;
			fRestReinfExp = 0.0f;
			for ( CReinforcementTypes::iterator it = players[info.nPlayer].reinforcementTypes.begin();
				it != players[info.nPlayer].reinforcementTypes.end(); ++it )
			{
				const int nReinfType = it->first;
				if ( GetReinforcementXPLevel( info.nPlayer, (NDb::EReinforcementType)nReinfType ) < N_MAX_XP_LEVEL )
				{
					float fExp = fExpPart;
					bool bLevelUp = AddReinfExp( &fExp, info.nPlayer, (NDb::EReinforcementType)( nReinfType ) );
					bResult = bResult || bLevelUp;
					fRestReinfExp += fExp;
				}
			}
		}
	}

	return bResult;
}

int CScenarioTrackerMultiplayer::GetUnitKills( const int nPlayer ) const
{
	float fSum = 0;

	for ( int i = 0; i < kills.GetSizeX(); ++i )
		fSum += kills[nPlayer][i];

	return fSum;
}

int CScenarioTrackerMultiplayer::GetUnitKills( const int nPlayer, const int nKilledPlayer ) const
{
	return kills[nPlayer][nKilledPlayer];
}

int CScenarioTrackerMultiplayer::GetUnitPriceKills( const int nPlayer, const int nKilledPlayer ) const
{
	return priceKills[nPlayer][nKilledPlayer];
}

void CScenarioTrackerMultiplayer::CalculateScore( const int nPlayer )
{
	NTimer::STime curTime = Singleton<IGameTimer>()->GetSegmentTime();

	// ESK_REINFORCEMENTS_CALLED
	{
		/*if ( bNoKeyBuildings )
			SetStatistics( nPlayer, ESK_REINFORCEMENTS_CALLED, pMission->players[nPlayer].nReinforcementCalls - GetReinforcementCallsLeft( nPlayer ) );
		else*/
			SetStatistics( nPlayer, ESK_REINFORCEMENTS_CALLED, players[nPlayer].nReinforcementCallsUsed );	
	}

	// ESK_KEY_BUILDINGS_CAPTURED
	{
		int nTime = players[nPlayer].timeKeyPointsOwned;

		for ( hash_map<int, int>::iterator it = flags.begin(); it != flags.end(); ++it )
		{
			if ( it->second == nPlayer )
				nTime += curTime - flagTimes[it->first];
		}

		nTime /= 1000;
		if ( nPlayer == nNeutralPlayer )
			nTime = 0;

		SetStatistics( nPlayer, ESK_KEY_BUILDINGS_CAPTURED, nTime );
	}

	// ESK_UNITS_LOST
	{
		int nCount = 0;
		for ( int i = 0; i < GetNPlayers(); ++i )
		{
			nCount += GetUnitKills( i, nPlayer );
		}
		SetStatistics( nPlayer, ESK_UNITS_LOST, nCount );
	}

	// ESK_UNITS_KILLED
	{
		int nCount = 0;
		for ( int i = 0; i < GetNPlayers(); ++i )
		{
			if ( nPlayer != i && GetPlayerSide( i ) != 2 && GetPlayerSide( nPlayer ) != GetPlayerSide( i ) )
			{
				nCount += GetUnitKills( nPlayer, i );
			}
		}
		SetStatistics( nPlayer, ESK_UNITS_KILLED, nCount );
	}

	// ESK_UNITS_LOST_PRICE
	{
		int nCount = 0;
		for ( int i = 0; i < GetNPlayers(); ++i )
		{
			nCount += GetUnitPriceKills( i, nPlayer );
		}
		SetStatistics( nPlayer, ESK_UNITS_LOST_PRICE, nCount );
	}

	// ESK_UNITS_KILLED_PRICE
	{
		int nCount = 0;
		for ( int i = 0; i < GetNPlayers(); ++i )
		{
			if ( nPlayer != i && GetPlayerSide( i ) != 2 && GetPlayerSide( nPlayer ) != GetPlayerSide( i ) )
			{
				nCount += GetUnitPriceKills( nPlayer, i );
			}
		}
		SetStatistics( nPlayer, ESK_UNITS_KILLED_PRICE, nCount );
	}

	// ESK_TACTICAL_EFFICIENCY
	{
		int nLostSum = GetStatistics( nPlayer, ESK_UNITS_LOST_PRICE );
		int nKilledSum = GetStatistics( nPlayer, ESK_UNITS_KILLED_PRICE );

		float fScore = 100.0f * nKilledSum / ( 0.1f + nLostSum + nKilledSum );
		SetStatistics( nPlayer, ESK_TACTICAL_EFFICIENCY, fScore );
	}

	// ESK_STRATEGIC_EFFICIENCY
	{
		int nKeyBuildingsCaptured = GetStatistics( nPlayer, ESK_KEY_BUILDINGS_CAPTURED );
		int nGameTime = ( curTime - timeMissionStart ) / 1000;

		float fScore = 100.0f * nKeyBuildingsCaptured / ( 1.0f + flags.size() * nGameTime );
		SetStatistics( nPlayer, ESK_STRATEGIC_EFFICIENCY, fScore );
	}

	// ESK_SCORE
	{
		float fScoreTactics = GetStatistics( nPlayer, ESK_TACTICAL_EFFICIENCY );
		float fScoreStrategy = GetStatistics( nPlayer, ESK_STRATEGIC_EFFICIENCY );

		float fScore = 7.0f * fScoreTactics + 3.0f * fScoreStrategy;

		SetStatistics( nPlayer, ESK_SCORE, fScore );
	}

}

int CScenarioTrackerMultiplayer::GetScoreWithUpdate( int nPlayer )
{
	CalculateScore( nPlayer );

	return GetStatistics( nPlayer, ESK_SCORE );
}

void CScenarioTrackerMultiplayer::SetMultiplayerInfo( const SMultiplayerInfo &info )
{
	multiplayerInfo = info;
}

IScenarioTracker::SMultiplayerInfo* CScenarioTrackerMultiplayer::GetMultiplayerInfo()
{
	return &multiplayerInfo;
}

void CScenarioTrackerMultiplayer::SetPlayerParty( const int nPlayer, const int nNewSide )
{
	if ( !IsMissionActive() || !IsPlayerPresent( nPlayer ) || GetPlayerSide( nPlayer ) == 2 )
		return;

	NI_VERIFY( nPlayer >= 0 && nPlayer < players.size(), StrFmt( "PRG: Player No %d not in bounds (max %d)", nPlayer, players.size() ), return );
	players[nPlayer].nMultiplayerSide = nNewSide;

	if ( players[nPlayer].nMultiplayerSide < 0 || players[nPlayer].nMultiplayerSide >= pMPConsts->sides.size() )
		players[nPlayer].nMultiplayerSide = -1;

	UpdateReinforcements( nPlayer );
}

const NDb::SPartyDependentInfo* CScenarioTrackerMultiplayer::GetPlayerParty( const int nPlayer )
{
	NI_VERIFY( nPlayer >= 0 && nPlayer < players.size(), StrFmt( "PRG: Player No %d not in bounds (max %d)", nPlayer, players.size() ), return 0 );

	if ( GetPlayerSide( nPlayer ) == 2 )			// Neutral party is taken from map
	{
		CDBPtr<NDb::SMapInfo> pMapInfo;
		if ( IsMissionActive() )
			pMapInfo = GetCurrentMission();
		else
			pMapInfo = GetLastMission();

		if ( !pMapInfo )
			return 0;

		return pMapInfo->players[nPlayer].pPartyInfo; 
	}

	if ( players[nPlayer].nMultiplayerSide >= 0 )
		return pMPConsts->sides[players[nPlayer].nMultiplayerSide].pPartyInfo;
	else
		return 0;
}

void CScenarioTrackerMultiplayer::SetPlayerSide( const int nPlayer, const int nNewTeam )
{
	NI_VERIFY( nPlayer >= 0 && nPlayer < players.size(), StrFmt( "PRG: Player No %d not in bounds (max %d)", nPlayer, players.size() ), return );
	if ( nNewTeam < 0 || nNewTeam > 2 )
		players[nPlayer].nDiplomacySide = 2;
	else
		players[nPlayer].nDiplomacySide = nNewTeam;
}

void CScenarioTrackerMultiplayer::UpdateReinforcements( const int nPlayer )
{
	if ( !IsMissionActive() || !IsPlayerPresent( nPlayer ) || GetPlayerSide( nPlayer ) == 2 )
		return;

	CReinforcementTypes &reinfs = players[nPlayer].reinforcementTypes;
	reinfs.clear();
	for ( int i = 0; i < pMission->players[nPlayer].reinforcementTypes.size(); ++i )
	{
		CDBPtr<NDb::SReinforcement> pReinf = pMission->players[nPlayer].reinforcementTypes[i];
		if ( !pReinf )
			continue;

		NDb::EReinforcementType eType = pReinf->eType;

		if ( nTechLevel < 0 || players[nPlayer].nMultiplayerSide == -1 )				// keep default if side (or techlevel) is undefined
			continue;

		// The following cycle selects the best (techlevel-wise, but not better than the selected tech level) reinforcement
		// from the MPConsts->sides. Default is taken from mapinfo 
		const vector<NDb::STechLevelReinfSet> &techLevels = pMPConsts->sides[players[nPlayer].nMultiplayerSide].techLevels;
		for ( int j = 0; j <= nTechLevel; ++j )
		{
			if ( j >= techLevels.size() )
				break;

			const vector< CDBPtr<NDb::SReinforcement> > &reinfSet = techLevels[j].reinforcements;

			for ( int k = 0; k < reinfSet.size(); ++k )
			{
				if ( reinfSet[k]->eType == eType )			// Found a reinf of this type in this tech level
				{
					reinfs[eType] = reinfSet[k];
					break;
				}
			}
		}
	}
}

const NDb::SReinforcement * CScenarioTrackerMultiplayer::GetStartUnits( int nPlayer ) const
{
	if ( !IsMissionActive() || !IsPlayerPresent( nPlayer ) || GetPlayerSide( nPlayer ) == 2 || nTechLevel < 0 )
		return 0;

	const NDb::STechLevelReinfSet &curTechLevel = pMPConsts->sides[players[nPlayer].nMultiplayerSide].techLevels[nTechLevel];

	return curTechLevel.pStartingUnits;
}

void CScenarioTrackerMultiplayer::IncreaseReinforcementCallsLeft( int nPlayer, int nCalls )
{
	if ( !IsMissionActive() || !IsPlayerPresent( nPlayer ) || GetPlayerSide( nPlayer ) == 2 )
		return;

	players[nPlayer].nReinforcementCallsLeft += nCalls;
	CONSOLE_BUFFER_LOG( CONSOLE_STREAM_DEBUG_WINDOW + 1, StrFmt( "Player %d reinf qty increase", nPlayer ) );
}

const IScenarioTracker::SPlayerColor& CScenarioTrackerMultiplayer::GetPlayerColor( int nPlayer ) const
{
	if ( nPlayer < 0 || nPlayer >= players.size() )
		return playerColorNeutral;
	
	return players[nPlayer].playerColor;
}

void CScenarioTrackerMultiplayer::SetPlayerColour( const int nPlayer, const int nNewColour )
{
	if ( !IsMissionActive() || nPlayer < 0 || nPlayer >= players.size() || nNewColour < 0 || nNewColour >= pMPConsts->playerColorInfos.size() )
		return;

	players[nPlayer].playerColor.dwColor = pMPConsts->playerColorInfos[nNewColour].nColor | 0xFF000000;
	players[nPlayer].playerColor.pUnitFullInfo = pMPConsts->playerColorInfos[nNewColour].pUnitFullInfo;
	players[nPlayer].playerColor.nColorIndex = MP_COLOR_INDEX_BASE + nNewColour;
}

const NDb::SObjectBaseRPGStats *CScenarioTrackerMultiplayer::GetKeyBuildingFlagObject( const int nPlayer )
{
	int nSide = GetPlayerSide( nPlayer );

	NI_ASSERT( nSide >= 0 && nSide < pMPConsts->diplomacyInfo.size(), StrFmt( "DATA: No info in MPConsts.diplomacyInfo for diplomacy side %d", nSide ) );
	if ( nSide >= 0 && nSide < pMPConsts->diplomacyInfo.size() )
	{
		if ( pMPConsts->diplomacyInfo[nSide] )
			return pMPConsts->diplomacyInfo[nSide]->pKeyBuildingFlag;
	}
	return 0;
}

}

REGISTER_SAVELOAD_CLASS_NM( 0x1117BB00, CScenarioTrackerMultiplayer, NScenarioTracker );

