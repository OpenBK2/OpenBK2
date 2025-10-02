#include "StdAfx.h"

#include "ScenarioTracker.hpp"
#include "../B2_M1_World/MissionObjectiveStates.h"
#include "../AILogic/DBAIConsts.h"
#include "GetConsts.h"
#include "../Misc/Win32Random.h"
#include "../Misc/StrProc.h"
#include "InterfaceState.h"
#include "../UISpecificB2/DBUISpecificB2.h"
#include "../Stats_B2_M1/RPGStats.h"
#include "../B2_M1_World/MapObj.h"
#include "../AILogic/B2AI.h"
#include "../System/Commands.h"
#include "../System/Text.h"

BASIC_REGISTER_CLASS(IAIScenarioTracker)

#define GET_ARRAY_SIZE( pre_name, name ) ( pre_name##name##s.empty() ? pre_name##name##FileRefs.size() : pre_name##name##s.size() )
#define GET_ARRAY_ELEMENT( pre_name, name, index ) ( pre_name##name##s.empty() ? NText::GetText( pre_name##name##FileRefs[index] ) : pre_name##name##s[index]->wszText )
#define CHECK_ARRAY_EMPTY( pre_name, name ) ( pre_name##name##s.empty() ? pre_name##name##FileRefs.empty() : true )
//#define GET_ARRAY_SIZE( pre_name, name ) ( pre_name##name##FileRefs.size() )
//#define GET_ARRAY_ELEMENT( pre_name, name, index ) ( NText::GetText( pre_name##name##FileRefs[index] ) )
//#define CHECK_ARRAY_EMPTY( pre_name, name ) ( pre_name##name##FileRefs.empty() )

int N_MAX_XP_LEVEL = 3; // [0..]

const int USER_COLOR_INDEX		= 0;
const int FRIEND_COLOR_INDEX	= 1;
const int ENEMY_COLOR_INDEX		= 2;
const int NEUTRAL_COLOR_INDEX	= 3;

static float s_fStaticPointerOffset = 5.0f;

// BEGIN new scenario tracker

IScenarioTracker * CreateScenarioTracker()
{
	return new NScenarioTracker::CScenarioTracker;
}


namespace NScenarioTracker
{

void SearchAvalableReinforcements( const NDb::SMapInfo *pCurMission, CReinforcementTypes *pMissionReinf, CReinforcementTypes *pChapterReinf, int nPlayer )
{
	if ( !pCurMission )
		return;

	// find out what reinforcement types are allowed in this map
	pMissionReinf->clear();
	for( vector<CDBPtr<NDb::SReinforcement> >::const_iterator it = pCurMission->players[nPlayer].reinforcementTypes.begin();
		it != pCurMission->players[nPlayer].reinforcementTypes.end(); ++it )
	{
		if ( *it )
		{
			if ( pChapterReinf ) // ask chapter.
			{
				CReinforcementTypes::iterator pos = pChapterReinf->find( (*it)->eType );
				if ( pos != pChapterReinf->end() )
				{
					(*pMissionReinf)[(*it)->eType] = pos->second;
				}
			}
			else	// no chapter reinforcements at all. use mission ones
				(*pMissionReinf)[(*it)->eType] = *it;
		}
	}
}


// CScenarioTracker


CScenarioTracker::CScenarioTracker() : bCampaignFinished( false ), bChapterFinished( false ), nReinforcementCallsLeftInChapter( 0 ),
	nReinforcementCallsLeftInMission( 0 ), nMainEnemy( -1 ), fPlayerXP( 0.0f ), nEnemyReinfCallsLeft( 0 ),
	nDifficulty( 0 ), bMissionWon( false ), nReinforcementCallsUsed( 0 ),
	knownReinfs( NDb::_RT_NONE, false ), bIsCustomCampaign( false )
{
	favoriteReinfs.resize( NDb::_RT_NONE );
}

CScenarioTracker::~CScenarioTracker()
{
	ClearMissionScriptVars();
}

void CScenarioTracker::GiveChapterReinforcement( CReinforcementTypes *pChapterReinf, int nPlayer ) const
{
	pChapterReinf->clear();
	if ( pChapter->basePlayerReinforcements.size() > nPlayer )
	{
		for ( int i = 0; i < pChapter->basePlayerReinforcements[nPlayer].reinforcements.size(); ++i )
		{
			const NDb::SReinforcement *pReinf = pChapter->basePlayerReinforcements[nPlayer].reinforcements[i];
			if ( pReinf )
				(*pChapterReinf)[pReinf->eType] = pReinf;
		}
	}
}

void CScenarioTracker::MapStart()
{
	if ( !GetCurrentMission() )
		return;

	ClearMissionScriptVars();

	fPlayerXPAdds = 0.0f;
	// Clear leader xp
	for ( CLeaderList::iterator it = leaders.begin(); it != leaders.end(); ++it )
	{
		SLeaderInfo &leader = it->second;

		leader.info = leader.storedInfo;
	}

	nMainEnemy = -1;
	nEnemyReinfCallsLeft = 0;
	for ( int i = 0; i < pMission->players.size(); ++i )
	{
		if ( GetCurrentMission()->players[i].nDiplomacySide == 1 )
		{
			nMainEnemy = i;
			nEnemyReinfCallsLeft = GetCurrentMission()->players[i].nReinforcementCalls * GetEnemyDifficultyRCallsModifier();
			break;
		}
	}

	// Init reinf-calls statistics
	for ( int i = 0; i < pMission->players.size(); ++i )
		SetStatistics( i, ESK_REINFORCEMENTS_CALLED, 0 );

	kills.SetSizes( pMission->players.size(), pMission->players.size() );
	kills.FillZero();
	priceKills.SetSizes( pMission->players.size(), pMission->players.size() );
	priceKills.FillZero();

	nReinforcementCallsUsed = 0;
	nMainEnemyMissionCalls = 0;
	SearchAvalableReinforcements( GetCurrentMission(), &playerMission, pChapter && pChapter->bUseMapReinforcements ? 0 : &playerChapter, 0 );
	if ( nMainEnemy >= 0 )
		SearchAvalableReinforcements( GetCurrentMission(), &enemyMission, 0/*&enemyChapter*/, nMainEnemy );		// Don't give chapter reinfs to enemy, use from map
	SearchPotentialReinforcements();

	if ( !pChapter && pMission )			// Map started directly
	{
		for( vector<CDBPtr<NDb::SReinforcement> >::const_iterator it = pMission->players[0].reinforcementTypes.begin();
			it != pMission->players[0].reinforcementTypes.end(); ++it )
		{
			NI_ASSERT( *it != 0, StrFmt( "empty reinforcement entry for player 0" ) );
			if ( *it )
				playerMission[(*it)->eType] = *it;
		}

		if ( nMainEnemy >= 0 )
		{
			for( vector<CDBPtr<NDb::SReinforcement> >::const_iterator it = pMission->players[nMainEnemy].reinforcementTypes.begin();
				it != pMission->players[nMainEnemy].reinforcementTypes.end(); ++it )
			{
				NI_ASSERT( *it != 0, StrFmt( "empty reinforcement entry for player %d", nMainEnemy ) );
				if ( *it )
					enemyMission[(*it)->eType] = *it;
			}
		}

		// If map started directly, player has this number of calls
		nReinforcementCallsLeftInMission = pMission->players[0].nReinforcementCalls;
	}
	if ( pChapter )
	{
		if ( pChapter->bUseMapReinforcements )
			nReinforcementCallsLeftInMission = pMission->players[0].nReinforcementCalls;
		else
			nReinforcementCallsLeftInMission = Min( nReinforcementCallsLeftInChapter, GetMissionRecommendedReinfCalls() );
	}

	objectives.resize( GetCurrentMission()->objectives.size() );
	for ( int i = 0; i < objectives.size(); ++i )
	{
		objectives[i] = EMOS_WAITING;
	}
	known_objectives.clear();

	const int nLocalPlayer = 0;
	SetStatistics( nLocalPlayer, ESK_TIME, 0 );

	const NDb::SUIConstsB2 *pUIConsts = InterfaceState()->GetUIConsts();
	if ( pUIConsts )
	{
		playerColorUser.dwColor = pUIConsts->playersColors.userInfo.nColor | 0xFF000000;
		playerColorUser.pUnitFullInfo = pUIConsts->playersColors.userInfo.pUnitFullInfo;
		playerColorUser.nColorIndex = USER_COLOR_INDEX;

		playerColorFriend.dwColor = pUIConsts->playersColors.friendInfo.nColor | 0xFF000000;
		playerColorFriend.pUnitFullInfo = pUIConsts->playersColors.friendInfo.pUnitFullInfo;
		playerColorFriend.nColorIndex = FRIEND_COLOR_INDEX;

		playerColorEnemy.dwColor = pUIConsts->playersColors.enemyInfo.nColor | 0xFF000000;
		playerColorEnemy.pUnitFullInfo = pUIConsts->playersColors.enemyInfo.pUnitFullInfo;
		playerColorEnemy.nColorIndex = ENEMY_COLOR_INDEX;

		playerColorNeutral.dwColor = pUIConsts->playersColors.neutralInfo.nColor | 0xFF000000;
		playerColorNeutral.pUnitFullInfo = pUIConsts->playersColors.neutralInfo.pUnitFullInfo;
		playerColorNeutral.nColorIndex = NEUTRAL_COLOR_INDEX;
	}

	// Get AI consts - a slow, painful process...
	pAIConsts = NGameX::GetAIConsts();
}

void CScenarioTracker::MissionStart( const NDb::SMapInfo * _pMission, const int nTechLevel )
{
	// single player only
	NGlobal::RemoveVar( "Multiplayer.Host" );
	NGlobal::RemoveVar( "Multiplayer.Client" );

	ClearMissionScriptVars();

	pMission = _pMission;
	//pLastMission = 0;
	bMissionWon = false;

	MapStart();
}

void CScenarioTracker::CustomMissionStart( const NDb::SMapInfo * _pMission, int _nDifficulty, bool bTutorial )
{
	// single player only
	NGlobal::RemoveVar( "Multiplayer.Host" );
	NGlobal::RemoveVar( "Multiplayer.Client" );

	pCampaign = 0;
	pChapter = 0;
	pMission = _pMission;
	//pLastMission = 0;
	bMissionWon = false;
	bIsTutorial = bTutorial;
	nDifficulty = Clamp( _nDifficulty, 0, _pMission ? _pMission->customDifficultyLevels.size() - 1 : 0 );

	MapStart();
}

void CScenarioTracker::ApplyMissionBonus( const NDb::SChapterBonus *pBonus, SMissionStats *pMissionStats )
{
	if ( !pBonus )
	{
		NI_ASSERT( 0, "Attempt to issue a null mission bonus" );
		return;
	}

	switch( pBonus->eBonusType ) 
	{
	case NDb::CBT_REINF_CHANGE:
		{
			if ( !pBonus->bApplyToEnemy )
			{
				NI_ASSERT( pBonus->pReinforcementSet, StrFmt( "Null reinforcement (ChapterBonus \"%s\")", NDb::GetResName(pBonus) ) );
				if ( pBonus->pReinforcementSet )
				{
					playerChapter[pBonus->pReinforcementSet->eType] = pBonus->pReinforcementSet;
					playerReinfPotential[pBonus->pReinforcementSet->eType] = ERS_ENABLED;
					if ( pMissionStats )
						pMissionStats->bonusReinforcements.push_back( pBonus->pReinforcementSet );
						
					// chapter reinf
					chapterCurrentReinfs[pBonus->pReinforcementSet->eType].eState = ERS_ENABLED;
					chapterCurrentReinfs[pBonus->pReinforcementSet->eType].pDBReinf = pBonus->pReinforcementSet;
					chapterCurrentReinfs[pBonus->pReinforcementSet->eType].bFromPrevChapter = false;
				}
			}
			else
			{
				NI_ASSERT( 0, StrFmt( "Attempt to add reinforcement to enemy (ChapterBonus \"%s\")", NDb::GetResName(pBonus) ) );
			}
		}
		break;
	case NDb::CBT_REINF_DISABLE:
		{
			if ( pBonus->bApplyToEnemy )
			{
				enemyChapter.erase( pBonus->eReinforcementType );
			}
			else
			{
				NI_ASSERT( 0, StrFmt( "Attempt to remove reinforcement from player (ChapterBonus \"%s\")", NDb::GetResName(pBonus) ) );
			}
		}
		break;
	case NDb::CBT_ADD_CALLS:
		{
			if ( !pBonus->bApplyToEnemy )
			{
				nReinforcementCallsLeftInChapter += pBonus->nNumberOfCalls;
			}
			else
			{
				NI_ASSERT( 0, StrFmt( "Attempt to add mission calls to enemy (ChapterBonus \"%s\")", NDb::GetResName(pBonus) ) );
			}
		}
		break;
	}
}

void CScenarioTracker::MissionCancel()
{
	NI_VERIFY( pMission != 0 || !bMissionWon, "no mission started or mission finished", return );

	UpdateStatistics( false );

	playerMission.clear();
	enemyMission.clear();
	pLastMission = pMission;
	pMission = 0;
	nMainEnemy = -1;
	nReinforcementCallsLeftInMission = 0;
	nReinforcementCallsUsed = 0;
	bMissionWon = false;
}

void CScenarioTracker::MissionWin()
{
	NI_VERIFY( pMission != 0, "no mission started or mission finished", return );

	UpdateStatistics( true );

	if ( pMission && pChapter )
	{
		wonMissions.push_back( pMission );

		SMissionStats missionStats;

		for ( int i = 0; i < chapterCurrentReinfs.size(); ++i )
		{
			IScenarioTracker::SChapterReinf &reinf = chapterCurrentReinfs[i];
			SMissionStats::SOldReinf oldReinf;
			oldReinf.eState = reinf.eState;
			oldReinf.pDBReinf = reinf.pDBReinf;
			missionStats.oldReinfs.push_back( oldReinf );
		}
		
		int nRecommendedCalls = 0;

		// check if mission ends chapter (mission path in chapter)
		for ( int i = 0; i < pChapter->missionPath.size(); ++i )
		{
			if ( pChapter->missionPath[i].pMap == pMission )
			{
				nRecommendedCalls = pChapter->missionPath[i].nRecommendedCalls;
				if ( i == 0 )			// It was the final mission
				{
					bChapterFinished = true;
					break;
				}

				// Issue bonuses if chapter did not end
				for ( int j = 0; j < pChapter->missionPath[i].reward.size(); ++j )
					ApplyMissionBonus( pChapter->missionPath[i].reward[j], &missionStats );
			}
		}
		
		CMissions enabledMissions;
		GetEnabledMissions( &enabledMissions );
		if ( enabledMissions.empty() )
			bChapterFinished = true;
		if ( pChapter )
			nReinforcementCallsLeftOld = nReinforcementCallsLeftInChapter;
		//nReinforcementCallsLeftInChapter -= nReinforcementCallsUsed + pMission->players[0].nReinforcementCalls - nReinforcementCallsLeftInMission;
		int nChapterCallsUsed = Max( 0, nRecommendedCalls - nReinforcementCallsLeftInMission );		// 
		nReinforcementCallsLeftInChapter -= nChapterCallsUsed;
		nReinforcementCallsUsed = 0;
		nMainEnemy = -1;

		int nOldPlayerRankIndex = GetPlayerRankIndex( fPlayerXP );
		fPlayerXP += fPlayerXPAdds;
		fPlayerXPAdds = 0.0f;
		int nNewPlayerRankIndex = GetPlayerRankIndex( fPlayerXP );
		if ( nOldPlayerRankIndex != nNewPlayerRankIndex )
		{
			missionStats.pNewPlayerRank = GetPlayerRank();
			InterfaceState()->SetAutoShowCommanderScreen( true );
		}
		for ( int i = nOldPlayerRankIndex + 1; i <= nNewPlayerRankIndex; ++i )
		{
			const NDb::SRankExperience &rankExp = pCampaign->rankExperiences[i];
			nAvailablePromotions += rankExp.nAddPromotion;
		}

		// Add leader xp
		for ( CLeaderList::iterator it = leaders.begin(); it != leaders.end(); ++it )
		{
			SLeaderInfo &leader = it->second;

			leader.storedInfo = leader.info;
		}

		/////////////////// Give medals
		int nChapterIndex = 0;
		for ( int i = 0; i < pCampaign->chapters.size(); ++i )
		{
			if ( pCampaign->chapters[i] == pChapter )
			{
				nChapterIndex = i + 1;
				break;
			}
		}
		CDBPtr<NDb::SMedal> pMedal;
		bool bMedalGiven = false;
		if ( nChapterIndex )
		{
			// Chapter
			if ( bChapterFinished && pCampaign->medalsForChapter.size() >= nChapterIndex )
			{
				DebugTrace( "---- Adding Medal for chapter %d", nChapterIndex );
				missionStats.medals.push_back( pCampaign->medalsForChapter[nChapterIndex - 1].pMedal );
				bMedalGiven = true;
			}

			// Kills
			if ( !bMedalGiven && pCampaign->medalsForKills.size() > nMedalKillsGiven )
			{
				const NDb::SMedalConditions &conditions = pCampaign->medalsForKills[nMedalKillsGiven];
				float fValue = GetStatistics( 0, ESK_CAMPAIGN_UNITS_KILLED );
				if ( nChapterIndex >= conditions.nStartingChapter && fValue > conditions.fParameter )
				{
					bMedalGiven = true;
					missionStats.medals.push_back( conditions.pMedal );
					++nMedalKillsGiven;
					DebugTrace( "---- Kills Value %f, giving medal %d", fValue, nMedalKillsGiven );
				}
				else
					DebugTrace( "---- Kills Value %f, not giving medal", fValue );
			}
			// Kill Ratio
			if ( !bMedalGiven && pCampaign->medalsForTactics.size() > nMedalTacticsGiven )
			{
				const NDb::SMedalConditions &conditions = pCampaign->medalsForTactics[nMedalTacticsGiven];
				float fUnitsLost = GetStatistics( 0, ESK_UNITS_LOST );
				if ( fUnitsLost == 0.0f )
					fUnitsLost = 1.0f;		// To avoid division by 0
				float fValue = GetStatistics( 0, ESK_UNITS_KILLED ) / fUnitsLost;
				if ( nChapterIndex >= conditions.nStartingChapter && fValue > conditions.fParameter )
				{
					bMedalGiven = true;
					missionStats.medals.push_back( conditions.pMedal );
					++nMedalTacticsGiven;
					DebugTrace( "---- Kills Ratio Value %f, giving medal %d", fValue, nMedalTacticsGiven );
				}
				else
					DebugTrace( "---- Kills Ratio Value %f, not giving medal", fValue );
			}
			// Reinforcements
			if ( !bMedalGiven && pCampaign->medalsForEconomy.size() > nMedalEconomyGiven )
			{
				const NDb::SMedalConditions &conditions = pCampaign->medalsForEconomy[nMedalEconomyGiven];
				float fValue = 0;
				if ( nRecommendedCalls > 0 )
					fValue = ( nRecommendedCalls - GetStatistics( 0, ESK_REINFORCEMENTS_CALLED ) ) * 100 / nRecommendedCalls;	// In %%
				if ( nChapterIndex >= conditions.nStartingChapter && fValue > conditions.fParameter )
				{
					bMedalGiven = true;
					missionStats.medals.push_back( conditions.pMedal );
					++nMedalEconomyGiven;
					DebugTrace( "---- Economy Value %f, giving medal %d", fValue, nMedalEconomyGiven );
				}
				else
					DebugTrace( "---- Economy Value %f, not giving medal", fValue );
			}
			// Maxing Reinforcements
			if ( nMedalMunchkinGiven == 0 )
			{
				bool bCanGive = true;
				for ( int i = 0; i < NDb::_RT_NONE; ++i )
				{
					if ( GetReinforcementXPLevel( 0, NDb::EReinforcementType( i ) ) < N_MAX_XP_LEVEL )
					{
						bCanGive = false;
						break;
					}
				}
				if ( bCanGive )
				{
					missionStats.medals.push_back( pCampaign->pMedalForMunchkinism );
					++nMedalMunchkinGiven;
				}
			}
		}

		missionsStats[pMission] = missionStats;
	}
	
	for ( int i = 0; i < knownReinfs.size(); ++i )
	{
		const int nLocalPlayer = 0;
		if ( GetReinforcement( nLocalPlayer, (NDb::EReinforcementType)( i ) ) != 0 )
			knownReinfs[i] = true;
	}

	for ( int i = 0; i < favoriteReinfs.size(); ++i )
	{
		SFavoriteReinf &reinf = favoriteReinfs[i];
		reinf.nTotalCount += reinf.nCurrentCount;
	}

	pLastMission = pMission;
	pMission = 0;
	bMissionWon = true;
	
	if ( IsTutorialMission() )
		InterfaceState()->ApplyTutorialRecommendedMission();
}

const IScenarioTracker::SMissionStats* CScenarioTracker::GetMissionStats( const NDb::SMapInfo *pMission ) const
{
	CMissionsStats::const_iterator it = missionsStats.find( pMission );
	if ( it != missionsStats.end() )
		return &(it->second);
	return 0;
}

int CScenarioTracker::GetMissionRecommendedReinfCalls() const
{
	if ( pChapter && pMission )
	{
		for ( vector< NDb::SMissionEnableInfo >::const_iterator it = pChapter->missionPath.begin(); 
			it != pChapter->missionPath.end(); ++it )
		{
			const NDb::SMissionEnableInfo &info = *it;
			
			if ( info.pMap == pMission )
				return info.nRecommendedCalls;
		}
	}
	return -1;
}

bool CScenarioTracker::IsOnlyRecommendedReinfCalls() const
{
	if ( pChapter && pMission && !pChapter->missionPath.empty() )
	{
		const NDb::SMissionEnableInfo &info = pChapter->missionPath.front(); 

		return (info.pMap != pMission);
	}
	return false;
}

void CScenarioTracker::ClearMissionScriptVars()
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

void CScenarioTracker::GetEnabledMissions( CMissions *_pMissions )
{
	if ( !pChapter )
		return;

	_pMissions->clear();

	hash_map<CDBID/*mission dbid*/,int/*nMissions to enable*/> missionsToEnable;
	for ( int i = 0; i < pChapter->missionPath.size(); ++i )
	{
		const NDb::SMapInfo *pMap = pChapter->missionPath[i].pMap;
		NI_ASSERT( pMap != 0, StrFmt( "Designers: pMap == 0 (Chapter ID: \"%s\", MissionPath: %d)", NDb::GetResName(pChapter), i ) );
		missionsToEnable[pMap->GetDBID()] = pChapter->missionPath[i].nMissionsToEnable;
	}

	const int nWonMissions = wonMissions.size();
	
	// read chapter about all missions
	for ( int i = 0; i < pChapter->missionPath.size(); ++i )
	{
		const NDb::SMapInfo *pMission = pChapter->missionPath[i].pMap;
		// add missions, that is enabled by wonMissions.size()
		if ( missionsToEnable[pMission->GetDBID()] <= nWonMissions )
		{
			// don't add already won missions
			bool bEnable = true;
			for ( CWonMissions::const_iterator itWon = wonMissions.begin(); itWon != wonMissions.end(); ++itWon )
			{
				if ( pMission == *itWon )
				{
					bEnable = false;
					break;
				}
			}

			if ( !bEnable )
				continue;

			// Check reinforcement requirements
			bool bReinfEnable = false;
			for ( int j = 0; j < pMission->players[0].reinforcementTypes.size(); ++j )
			{
				const NDb::SReinforcement *pReinf = pMission->players[0].reinforcementTypes[j];

				if ( playerChapter.find( pReinf->eType ) != playerChapter.end() )
				{
					bReinfEnable = true;
					break;
				}
			}

			if ( !pMission->players[0].reinforcementTypes.size() )
				bReinfEnable = true;				// No reinforcements required = enable

			if ( bReinfEnable || (pChapter && pChapter->bUseMapReinforcements) )
				_pMissions->push_back( pMission );
		}
	}
}

void CScenarioTracker::GetCompletedMissions( CMissions *_pMissions )
{
	if ( !pChapter )
		return;

	_pMissions->clear();
	for ( CWonMissions::const_iterator itWon = wonMissions.begin(); itWon != wonMissions.end(); ++itWon )
	{
		_pMissions->push_back( *itWon );
	}
}

int CScenarioTracker::GetMissionToEnableCount() const
{
	if ( !pChapter || pChapter->missionPath.empty() )
		return 0;
	const int nMissionsToEnable = pChapter->missionPath[0].nMissionsToEnable;
	const int nWonMissions = wonMissions.size();
	const int nCount = Max( 0, nMissionsToEnable - nWonMissions );
	return nCount;
}

void CScenarioTracker::CampaignStart( const NDb::SCampaign *_pCampaign, const int _nDifficulty, bool _bIsTutorial, bool bCustom ) 
{
	// nothing to do yet :)
	pCampaign = _pCampaign;
	bIsTutorial = _bIsTutorial;
	bIsCustomCampaign = bCustom;
	pChapter = 0;
	pMission = 0;
	fPlayerXP = 0.0f;
	fPlayerXPAdds = 0.0f;
	nEnemyReinfCallsLeft = 0;
	nReinforcementCallsLeftOld = 0;
	nAvailablePromotions = 0;
	if ( pCampaign && !pCampaign->rankExperiences.empty() )
	{
		const NDb::SRankExperience &rankExp = pCampaign->rankExperiences.front();
		nAvailablePromotions = rankExp.nAddPromotion;
	}
	pLastMission = 0;
	bMissionWon = false;
	wonMissions.clear();
	for ( int i = 0; i < favoriteReinfs.size(); ++i )
	{	
		SFavoriteReinf &reinf = favoriteReinfs[i];
		reinf.nCurrentCount = 0;
	}

	if ( pCampaign )
	{
		nDifficulty = Clamp( _nDifficulty, 0, pCampaign->difficultyLevels.size() - 1 );

		freeLeaders.resize( pCampaign->leaders.size() );
		for ( int i = 0; i < pCampaign->leaders.size(); ++i )
			freeLeaders[i] = i;
	}

	chapterCurrentReinfs.resize( NDb::_RT_NONE );
	SChapterReinf reinf;
	reinf.eState = ERS_DISABLED;
	reinf.bFromPrevChapter = false;
	fill( chapterCurrentReinfs.begin(), chapterCurrentReinfs.end(), reinf );

	const int nLocalPlayer = 0;
	SetStatistics( nLocalPlayer, ESK_CAMPAIGN_TIME, 0 );
	SetStatistics( nLocalPlayer, ESK_CAMPAIGN_UNITS_LOST, 0 );
	SetStatistics( nLocalPlayer, ESK_CAMPAIGN_UNITS_KILLED, 0 );
	SetStatistics( nLocalPlayer, ESK_CAMPAIGN_MISSIONS_PASSED, 0 );

	nMedalKillsGiven = 0;
	nMedalTacticsGiven = 0;
	nMedalEconomyGiven = 0;
	nMedalMunchkinGiven = 0;

	fLastVisiblePlayerStatsExpCareer = 0.0f;
	fLastVisiblePlayerStatsExpNextRank = 0.0f;
}

void CScenarioTracker::NextChapter() 
{
	NI_ASSERT( pCampaign != 0, "no campaign started" );
	if ( !pCampaign )
		return;

	InterfaceState()->SetFirstTimeInChapter( true );
	InterfaceState()->SetAutoShowCommanderScreen( true );

	pMission = 0;
	wonMissions.clear();

	const bool bKRIDemo = NGlobal::GetVar( "DEMO_MODE", 0 ) != 0;

	if ( !pChapter )
		pChapter = pCampaign->chapters[0];
	else
	{
		for ( int i = 0; i < pCampaign->chapters.size(); ++i )
		{
			if ( pCampaign->chapters[i] == pChapter )
			{
				if ( i + 1 >= pCampaign->chapters.size() || (bKRIDemo && i >= 1) ) // campaign finished
				{
					bCampaignFinished = true;
					pChapter = 0;
				}
				else
				{
					pChapter = pCampaign->chapters[i+1];
					break;
				}
			}
		}
	}

	if ( pChapter )
	{
		nReinforcementCallsLeftOld = 0;
		nReinforcementCallsLeftInChapter = pChapter->nReinforcementCalls;
		bChapterFinished = false;
		// advance to next chapter - give new reinforcements
		GiveChapterReinforcement( &playerChapter, 0 );
		GiveChapterReinforcement( &enemyChapter, 1 );

		// chapter reinforcements

		// un-enable previous chapter info
		for ( int i = 0; i < chapterCurrentReinfs.size(); ++i )
		{
			SChapterReinf &reinf = chapterCurrentReinfs[i];
			if ( reinf.eState == ERS_ENABLED )
				reinf.eState = ERS_NOT_ENABLED;
			reinf.bFromPrevChapter = true;
		}

		// set initial (enabled) reinf
		if ( !pChapter->basePlayerReinforcements.empty() )
		{
			for ( int i = 0; i < pChapter->basePlayerReinforcements[0].reinforcements.size(); ++i )
			{
				const NDb::SReinforcement *pReinf = pChapter->basePlayerReinforcements[0].reinforcements[i];
				if ( pReinf )
				{
					chapterCurrentReinfs[pReinf->eType].eState = ERS_ENABLED;
					chapterCurrentReinfs[pReinf->eType].pDBReinf = pReinf;
					chapterCurrentReinfs[pReinf->eType].bFromPrevChapter = false;
				}
			}
		}

		// set possible (non-enabled) reinf
		for ( int i = 0; i < pChapter->missionPath.size(); ++i )
		{
			for ( int j = 0; j < pChapter->missionPath[i].reward.size(); ++j )
			{
				CDBPtr<NDb::SChapterBonus> pBonus = pChapter->missionPath[i].reward[j];

				if ( !pBonus )
				{
					NI_ASSERT( 0, StrFmt( "Empty mission reward, chapter \"%s\", mission %d, reward %d", NDb::GetResName( pBonus ), i, j ) );
					continue;
				}

				if ( pBonus->eBonusType == NDb::CBT_REINF_CHANGE )
				{
					NI_ASSERT( pBonus->pReinforcementSet, StrFmt( "No reinforcement specified in ChapterBonus \"%s\"", NDb::GetResName(pBonus) ) );
					if ( pBonus->pReinforcementSet )
					{
						const NDb::EReinforcementType eType = pBonus->pReinforcementSet->eType;

						if ( !pBonus->bApplyToEnemy )
						{
							if ( chapterCurrentReinfs[eType].eState != ERS_ENABLED )
							{
								chapterCurrentReinfs[eType].eState = ERS_NOT_ENABLED;
								chapterCurrentReinfs[eType].pDBReinf = pBonus->pReinforcementSet;
								chapterCurrentReinfs[eType].bFromPrevChapter = false;
							}
						}
					}
				}
			}
		}
	}
}

const NDb::SReinforcement * CScenarioTracker::GetReinforcement( int nPlayer, NDb::EReinforcementType eType ) const
{
	if ( nPlayer == 0 )
	{
		if ( IsMissionActive() )
		{
			CReinforcementTypes::const_iterator pos = playerMission.find( eType );
			if ( pos != playerMission.end() )
				return pos->second;
		}
		else if ( pChapter )				// Give chapter reinf
		{
			CReinforcementTypes::const_iterator pos = playerChapter.find( eType );
			if ( pos != playerChapter.end() )
				return pos->second;
		}
	}
	else if ( nPlayer == nMainEnemy )
	{
		CReinforcementTypes::const_iterator pos = enemyMission.find( eType );
		if ( pos != enemyMission.end() )
			return pos->second;
	}
	return 0;
}

int CScenarioTracker::GetNPlayers() const
{
	if ( GetCurrentMission() )
		return GetCurrentMission()->players.size();
	return 0;
}

int CScenarioTracker::GetPlayerSide( int nPlayer ) const
{
	if ( GetCurrentMission() && GetCurrentMission()->players.size() > nPlayer )
		return GetCurrentMission()->players[nPlayer].nDiplomacySide;
	return 2;
}

float CScenarioTracker::GetReinforcementXP( int nPlayer, NDb::EReinforcementType eType ) const
{
	NI_ASSERT( 0, "Wrong call" );
	return 0.0f;
}

void CScenarioTracker::SetReinforcementXP( int nPlayer, NDb::EReinforcementType eType, float fXP )
{
	NI_ASSERT( 0, "Wrong call" );
}

int CScenarioTracker::GetReinforcementXPLevel( int nPlayer, NDb::EReinforcementType eType ) const
{
	if ( eType == NDb::RT_ENGINEERING || eType == NDb::RT_RECON || eType == NDb::RT_SUPER_WEAPON )
	{
		return N_MAX_XP_LEVEL;
	}

	if ( nPlayer != GetLocalPlayer() )
		return 0;

	if ( !pCampaign || !pChapter )
	{
#ifndef _FINALRELEASE
		if ( NGlobal::GetVar( "debug_unit_levels", 0 ) != 0 )
			return N_MAX_XP_LEVEL;
#endif _FINALRELEASE
		return 0;
	}
	
	const SLeaderInfo *pLeader = GetLeaderInfo( eType );
	if ( pLeader )
		return pLeader->info.nRank;

	return 0;
}

float CScenarioTracker::GetReinforcementXPForLevel( NDb::EReinforcementType eType, int nLevel ) const
{
	if ( !pAIConsts )
		return 0.0f;

	for ( int i = 0; i < pAIConsts->common.expLevels.size(); ++i ) 
	{
		const NDb::SAIExpLevel *pLevels = pAIConsts->common.expLevels[i];
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

int CScenarioTracker::GetReinforcementCallsLeft( int nPlayer )
{
//	NI_ASSERT( GetCurrentMap() != 0, "mission not started" ); // ассерт не нужен, т.к. миссия может быть запущена как подложка - без ScenarioTracker
	if ( GetCurrentMission() == 0 )
	{
		return 0;
	}

	if ( nPlayer == 0 )
		return nReinforcementCallsLeftInMission;

	if ( nPlayer == nMainEnemy )
		return nEnemyReinfCallsLeft;

	return 0;
}

int CScenarioTracker::GetReinforcementCallsLeftInChapter() const
{
	return nReinforcementCallsLeftInChapter;
}


int CScenarioTracker::GetKnownObjectiveCount() const
{
	return known_objectives.size();
}

int CScenarioTracker::GetKnownObjectiveID( const int nIndex )
{
	return known_objectives[nIndex];
}

int CScenarioTracker::GetObjectiveCount() const
{
	return objectives.size();
}

enum EMissionObjectiveState CScenarioTracker::GetObjectiveState( const int nID ) const
{
	return objectives[nID];
}

void CScenarioTracker::SetObjectiveState( const int nID, const EMissionObjectiveState eState )
{
	if ( !GetCurrentMission() )
		return; // запущена карта без миссии

	NI_VERIFY( nID >= 0 && nID < objectives.size(), StrFmt("Objective ID (%d) out of range [0..%d]", nID, objectives.size()), return );
	NI_VERIFY( eState >= EMOS_MIN && eState <= EMOS_MAX, StrFmt("Objective state (%d) invalid (must be in range [%d..%d])", eState, EMOS_MIN + 1, EMOS_MAX - 1), return );

	if ( objectives[nID] == EMOS_WAITING && eState != EMOS_WAITING )
	{
		vector<int>::iterator it = find( known_objectives.begin(), known_objectives.end(), nID );
		if ( it == known_objectives.end() )
			known_objectives.push_back( nID );
	}
	else if ( objectives[nID] != EMOS_WAITING && eState == EMOS_WAITING )
	{
		vector<int>::iterator it = find( known_objectives.begin(), known_objectives.end(), nID );
		if ( it != known_objectives.end() )
			known_objectives.erase( it );
	}

	if ( objectives[nID] != EMOS_COMPLETED && eState == EMOS_COMPLETED )
		GiveXPToPlayer( GetLocalPlayer(), GetCurrentMission()->objectives[nID]->nExperience );
	else if ( objectives[nID] == EMOS_COMPLETED && eState != EMOS_COMPLETED )
		GiveXPToPlayer( GetLocalPlayer(), - GetCurrentMission()->objectives[nID]->nExperience );

	objectives[nID] = eState;
}

bool CScenarioTracker::GetObjectivePlaces( int nID, vector<CVec3> *pPlaces ) const
{
	CObjectivesObjects::const_iterator it = objectivesObjects.find( nID );
	if ( it != objectivesObjects.end() )
	{
		const CObjectiveObjects &objectiveObjects = it->second;
		pPlaces->clear();
		pPlaces->reserve( objectiveObjects.size() );
		for ( int i = 0; i < objectiveObjects.size(); ++i )
		{
			const CMapObj *pMO = objectiveObjects[i];
			if ( pMO && pMO->IsRefValid() && pMO->IsVisible() && pMO->IsAlive() )
			{
				pPlaces->push_back( pMO->GetCenter() );
			}
		}
		return true;
	}

	pPlaces->clear();
	const NDb::SMissionObjective *pObjective = 0;
	if ( pMission && nID >= 0 && nID < pMission->objectives.size() )
		pObjective = pMission->objectives[nID];
	if ( !pObjective )
		return false;

	IAILogic *pAILogic = Singleton<IAILogic>();
	
	pPlaces->resize( pObjective->mapPositions.size() );
	for ( int i = 0; i < pObjective->mapPositions.size(); ++i )
	{
		const CVec2 &vPos2 = pObjective->mapPositions[i];
		CVec3 vPos( vPos2.x, vPos2.y, pAILogic->GetZ( vPos2 ) + Vis2AI( s_fStaticPointerOffset ) );
		(*pPlaces)[i] = vPos;
	}
	return true;
}

void CScenarioTracker::SetObjectiveObjects( int nID, const vector< CMapObj* > &objects )
{
	objectivesObjects.resize( objects.size() );
	CObjectivesObjects::iterator it = objectivesObjects.insert( pair< int, CObjectiveObjects >( nID, CObjectiveObjects() ) ).first;
	CObjectiveObjects &objectiveObjects = it->second;
	objectiveObjects.resize( objects.size() );
	for ( int i = 0; i < objects.size(); ++i )
	{
		objectiveObjects[i] = objects[i];
	}
}

bool CScenarioTracker::IsDynamicObjective( int nID ) const
{
	CObjectivesObjects::const_iterator it = objectivesObjects.find( nID );
	return it != objectivesObjects.end();
}

IScenarioTracker::EReinforcementState CScenarioTracker::GetReinforcementEnableState( int nPlayer, NDb::EReinforcementType eType )
{
	if ( !GetCurrentMission() )
		return ERS_DISABLED;

	if ( nPlayer == 0 )
		return playerReinfPotential[eType];
	else
		return ERS_DISABLED;
}

void CScenarioTracker::GetChapterCurrentReinforcements( vector<SChapterReinf> *pReinf, int nPlayer ) const
{
	if ( nPlayer == 0 )
		*pReinf = chapterCurrentReinfs;
	else
		pReinf->clear();
}

void CScenarioTracker::SearchPotentialReinforcements()
{
	// Clear
	playerReinfPotential.resize( NDb::_RT_NONE );
	for ( int i = 0; i < NDb::_RT_NONE; ++i )
		playerReinfPotential[i] = ERS_DISABLED;

	if ( !pChapter )
		return;

	// Add all bonuses
	for ( int i = 0; i < pChapter->missionPath.size(); ++i )
	{
		for ( int j = 0; j < pChapter->missionPath[i].reward.size(); ++j )
		{
			if ( pChapter->missionPath[i].pMap == GetCurrentMission() )
				continue;			// Can not apply potential bonuses to mission itself

			const NDb::SChapterBonus *pBonus = pChapter->missionPath[i].reward[j];

			if ( !pBonus )
			{
				NI_ASSERT( 0, "Empty mission bonus" );
				continue;
			}

			if ( pBonus->eBonusType == NDb::CBT_REINF_CHANGE )
			{
				NI_ASSERT( pBonus->pReinforcementSet, StrFmt( "No reinforcement specified in ChapterBonus \"%s\"", NDb::GetResName(pBonus) ) );
				const NDb::EReinforcementType eType = pBonus->pReinforcementSet->eType;

				if ( !pBonus->bApplyToEnemy )
					playerReinfPotential[eType] = ERS_NOT_ENABLED;
			}
			else if ( pBonus->eBonusType == NDb::CBT_REINF_DISABLE )
			{
				// Mark potentially disabled enemy reinforcements?
			}
		}
	}

	// Set enabled and remove non-applicable
	for ( int i = 0; i < NDb::_RT_NONE; ++i )
	{
		if ( GetReinforcement( 0, NDb::EReinforcementType( i ) ) )
			playerReinfPotential[i] = ERS_ENABLED;
		else if ( playerReinfPotential[i] == ERS_NOT_ENABLED )
		{
			EReinforcementState eState = ERS_DISABLED;
			for( vector<CDBPtr<NDb::SReinforcement> >::const_iterator it = GetCurrentMission()->players[0].reinforcementTypes.begin();
				it != GetCurrentMission()->players[0].reinforcementTypes.end(); ++it )
			{
				NI_ASSERT( *it != 0, StrFmt( "empty reinforcement entry for player 0" ));
				if ( *it && (*it)->eType == i )
				{
					eState = ERS_NOT_ENABLED;
					break;
				}
			}

			playerReinfPotential[i] = eState;
		}
		else
			playerReinfPotential[i] = ERS_DISABLED;
	}
}

void CScenarioTracker::DecreaseReinforcementCallsLeft( int nPlayer, int nCalls )
{
	int nActualCalls = ( nCalls == 0 ) ? 1 : nCalls;
	if ( nPlayer == 0 )
		nReinforcementCallsLeftInMission = Max ( 0, nReinforcementCallsLeftInMission - nActualCalls );
	else if ( nPlayer == nMainEnemy )
	{
		nEnemyReinfCallsLeft = Max ( 0, nEnemyReinfCallsLeft - nActualCalls );
	}

	// Add to statistics
	if ( nCalls == 0 )
	{
		if ( nPlayer == GetLocalPlayer() )
			++nReinforcementCallsUsed;
		int nOldValue = GetStatistics( nPlayer, ESK_REINFORCEMENTS_CALLED );
		SetStatistics( nPlayer, ESK_REINFORCEMENTS_CALLED, nOldValue + 1 );
	}
}

void CScenarioTracker::IncreaseReinforcementCallsLeft( int nPlayer, int nCalls )
{
	if ( nPlayer == 0 )
	{
		nReinforcementCallsLeftInMission += nCalls;
	}
	else if ( nPlayer == nMainEnemy )
		nEnemyReinfCallsLeft += nCalls;
}

void CScenarioTracker::RegisterReinforcementCall( int nPlayer, NDb::EReinforcementType eType )
{
	DecreaseReinforcementCallsLeft( nPlayer, 0 );
}

bool CScenarioTracker::RegisterUnitKill( const SKillInfo &info )
{
 	if ( !pAIConsts )
	{
		// A hack for Main Menu background mission, which is started around Sc.Tracker...
		pAIConsts = NGameX::GetAIConsts();
	}

	// register kill
	if ( info.nPlayer < kills.GetSizeY() && info.nKilledUnitPlayer < kills.GetSizeX() )
		++kills[info.nPlayer][info.nKilledUnitPlayer];
	if ( info.nPlayer < priceKills.GetSizeY() && info.nKilledUnitPlayer < priceKills.GetSizeX() )
		priceKills[info.nPlayer][info.nKilledUnitPlayer] += info.fExpPrice;

	if ( info.eReinfType == NDb::_RT_NONE )
		return false;

	if ( IsCampaignActive() && info.nKilledUnitPlayer == GetLocalPlayer() )		// Leader who lost
	{
		CLeaderList::iterator it = leaders.find( info.eKilledReinfType );
		if ( it != leaders.end() )
		{
			SLeaderInfo &leader = it->second;
			++leader.info.nUnitsLost;
			leader.info.fExpDebt += info.fExpPrice * pAIConsts->common.fExpCommanderUnitPenaltyCoeff;
		}
	}

	if ( info.nPlayer != 0 )
		return false;

	fPlayerXPAdds += info.fExpPrice;

	bool bResult = false;

	// Adjust leader info
	if ( IsCampaignActive() && info.nPlayer == GetLocalPlayer() )					// Leader who killed
	{
		CLeaderList::iterator it = leaders.find( info.eReinfType );
		if ( it != leaders.end() )
		{
			SLeaderInfo &leader = it->second;
			++leader.info.nUnitsKilled;
			
			float fRestExp = info.fExpPrice;
			bResult = bResult || AddLeaderExp( &fRestExp, &leader, info.eReinfType );

			// split rest of exp
			fRestExp *= pAIConsts->common.fExpCommanderDistributionCoeff;
			while ( fRestExp > 0.0f )
			{
				int nCount = 0;
				for ( CLeaderList::const_iterator it = leaders.begin(); it != leaders.end(); ++it )
				{
					const SLeaderInfo &leader = it->second;
					if ( leader.info.nRank + 1 < pCampaign->leaderRanks.size() )
						++nCount;
				}
				if ( nCount == 0 )
					break;

				const float fExpPart = fRestExp / nCount;
				fRestExp = 0.0f;
				for ( CLeaderList::iterator it = leaders.begin(); it != leaders.end(); ++it )
				{
					SLeaderInfo &leader = it->second;

					if ( leader.info.nRank + 1 < pCampaign->leaderRanks.size() )
					{
						float fExp = fExpPart;
						bool bLevelUp = AddLeaderExp( &fExp, &leader, NDb::EReinforcementType(it->first) );
						bResult = bResult || bLevelUp;
						fRestExp += fExp;
					}
				}
			}
		}
	}

	return bResult;
}

bool CScenarioTracker::AddReinfExp( float *pExp, NDb::EReinforcementType eReinfType )
{
	NI_ASSERT( 0, "wrong call" );
	return false;
}

bool CScenarioTracker::AddLeaderExp( float *pExp, SLeaderInfo *pLeader, NDb::EReinforcementType eReinfType )
{
	bool bResult = false;
	
	while ( pLeader->info.nRank + 1 < pCampaign->leaderRanks.size() && *pExp > 0.0f )
	{
		// exp debt
		if ( pLeader->info.fExpDebt > 0.0f )
		{
			float fMaxPayDebt = *pExp * pAIConsts->common.fExpCommanderPenaltyCoeff;
			if ( fMaxPayDebt >= pLeader->info.fExpDebt )
			{
				*pExp -= pLeader->info.fExpDebt;
				pLeader->info.fExpDebt = 0.0f;
			}
			else
			{
				*pExp -= fMaxPayDebt;
				pLeader->info.fExpDebt -= fMaxPayDebt;
			}
		}
		pLeader->info.fExp += *pExp;
		*pExp = 0.0f;

		// check for levelup
		//float fRequiredXP = pCampaign->leaderRanks[pLeader->info.nRank + 1].nExpNeeded;
		float fRequiredXP = GetReinforcementXPForLevel( eReinfType, pLeader->info.nRank + 1 );
		float fExtraExp = pLeader->info.fExp - fRequiredXP;
		if ( fExtraExp >= 0.0f )
		{
			*pExp = fExtraExp;
			pLeader->info.fExp = fRequiredXP;
			if ( pLeader->info.nRank < pCampaign->leaderRanks.size()-1 )
				pLeader->info.nRank++;
			bResult = true;
		}
	}

	return bResult;
}

int CScenarioTracker::GetUnitKills( const int nPlayer ) const
{
	int nSum = 0;

	for ( int i = 0; i < kills.GetSizeX(); ++i )
		nSum += kills[nPlayer][i];

	return nSum;
}

int CScenarioTracker::GetUnitKills( const int nPlayer, const int nKilledPlayer ) const
{
	return kills[nPlayer][nKilledPlayer];
}

int CScenarioTracker::GetUnitPriceKills( const int nPlayer, const int nKilledPlayer ) const
{
	return priceKills[nPlayer][nKilledPlayer];
}

const NDb::SDifficultyLevel* CScenarioTracker::GetDifficultyLevelDB() const
{
	if ( pCampaign )
	{
		if ( nDifficulty >= 0 && nDifficulty < pCampaign->difficultyLevels.size() )
			return pCampaign->difficultyLevels[nDifficulty];
	}
	else if ( pMission )
	{
		if ( nDifficulty >= 0 && nDifficulty < pMission->customDifficultyLevels.size() )
			return pMission->customDifficultyLevels[nDifficulty];
	}

	return 0;
}

const NDb::SUnitStatsModifier* CScenarioTracker::GetEnemyDifficultyModifier()
{
	const NDb::SDifficultyLevel *pDifficultyLevel = GetDifficultyLevelDB();
	if ( pDifficultyLevel )
		return pDifficultyLevel->pEnemyStatModifier;

	return 0;
}

const NDb::SUnitStatsModifier* CScenarioTracker::GetPlayerDifficultyModifier()
{
	const NDb::SDifficultyLevel *pDifficultyLevel = GetDifficultyLevelDB();
	if ( pDifficultyLevel )
		return pDifficultyLevel->pPlayerStatModifier;

	return 0;
}

const float CScenarioTracker::GetEnemyDifficultyRCallsModifier()
{
	const NDb::SDifficultyLevel *pDifficultyLevel = GetDifficultyLevelDB();
	if ( pDifficultyLevel )
		return pDifficultyLevel->fEnemyReinfCallsCoeff;

	return 1.0f;
}

const float CScenarioTracker::GetEnemyDifficultyRTimeModifier()
{
	const NDb::SDifficultyLevel *pDifficultyLevel = GetDifficultyLevelDB();
	if ( pDifficultyLevel )
		return pDifficultyLevel->fEnemyReinfRecycleCoeff;

	return 1.0f;
}

const NDb::SPlayerRank* CScenarioTracker::GetPlayerRank() const
{
	if ( !pCampaign )
		return 0;

	int nIndex = GetPlayerRankIndex();
	if ( nIndex < 0 || nIndex >= pCampaign->rankExperiences.size() )
		return 0;

	return pCampaign->rankExperiences[nIndex].pRank;
}

int CScenarioTracker::GetPlayerRankIndex( int nXP ) const
{
	if ( !pCampaign || pCampaign->rankExperiences.empty() )
		return -1;

	int nChosenRank = 0;
	for ( int i = 1; i < pCampaign->rankExperiences.size(); ++i )
	{
		if ( pCampaign->rankExperiences[i].fExperience > nXP )
			break;
		else
			nChosenRank = i;
	}

	return nChosenRank;
}

int CScenarioTracker::GetPlayerRankIndex() const
{
	return GetPlayerRankIndex( fPlayerXP + fPlayerXPAdds );
}

int CScenarioTracker::CalcPossibleEnemyExp( int nPlayer ) const
{
	if ( !pMission )
		return 0;

	float fTotalExp = 0;
	
	const NDb::SMapPlayerInfo &player = pMission->players[nPlayer];
	for ( int i = 0; i < pMission->players.size(); ++i )
	{
		const NDb::SMapPlayerInfo &opponent = pMission->players[i];
		if ( i == nPlayer || opponent.nDiplomacySide == 2 || opponent.nDiplomacySide == player.nDiplomacySide )
			continue;
			
		float fMaxReinfExp = 0;
		for ( int j = 0; j < opponent.reinforcementTypes.size(); ++j )
		{
			const NDb::SReinforcement *pReinf = opponent.reinforcementTypes[j];
			if ( !pReinf )
				continue;

			float fReinfExp = 0;
			for ( int k = 0; k < pReinf->entries.size(); ++k )
			{
				const NDb::SReinforcementEntry &entry = pReinf->entries[k];
				
				if ( entry.pMechUnit )
				{
					if ( entry.pMechUnit->fExpPrice > 0.0f )
						fReinfExp += entry.pMechUnit->fExpPrice;
				}
				else if ( entry.pSquad )
				{
					for ( int m = 0; m < entry.pSquad->members.size(); ++m )
					{
						const NDb::SInfantryRPGStats *pInf = entry.pSquad->members[m];
						if ( !pInf )
							continue;

						if ( pInf->fExpPrice > FP_EPSILON )
							fReinfExp += pInf->fExpPrice;
					}
				}
			}
			if ( fReinfExp > fMaxReinfExp )
				fMaxReinfExp = fReinfExp;
		}
		
		if ( opponent.nReinforcementCalls > 0 )
			fTotalExp += fMaxReinfExp * opponent.nReinforcementCalls;
			
		for ( int j = 0; j < pMission->objects.size(); ++j )
		{
			const NDb::SMapObjectInfo &objInfo = pMission->objects[j];
			if ( objInfo.nPlayer != i && !objInfo.pObject )
				continue;
				
			const NDb::SUnitBaseRPGStats *pObj = dynamic_cast_ptr<const NDb::SUnitBaseRPGStats*>( objInfo.pObject );
			if ( pObj && pObj->fExpPrice > FP_EPSILON )
				fTotalExp += pObj->fExpPrice;
		}
	}
	
	return (int)( fTotalExp );
}

bool CScenarioTracker::IsPlayerPresent( const int nPlayer ) const
{
	return true;
}

int CScenarioTracker::GetStatistics( int nPlayer, EStatisticsKind eKind ) const
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

void CScenarioTracker::SetStatistics( int nPlayer, EStatisticsKind eKind, int nValue )
{
	NI_ASSERT( nValue >= 0, "Wrong statistics value" );
	
	if ( nPlayer >= statistic.size() )
		statistic.resize( nPlayer + 1 );

	if ( eKind >= statistic[nPlayer].size() )
		statistic[nPlayer].insert( statistic[nPlayer].end(), eKind + 1 - statistic[nPlayer].size(), 0 );

	statistic[nPlayer][eKind] = nValue;
}

bool CScenarioTracker::IsNewReinf( NDb::EReinforcementType eType ) const
{
	const int nLocalPlayer = 0;
	return GetReinforcement( nLocalPlayer, eType ) != 0 && !knownReinfs[eType];
}

const IScenarioTracker::SPlayerColor& CScenarioTracker::GetPlayerColor( int nPlayer ) const
{
	if ( nPlayer == GetLocalPlayer() )
		return playerColorUser;

	int nSide = GetPlayerSide( nPlayer );
	if ( nSide == 0 )
		return playerColorFriend;
	else if ( nSide == 1 )
		return playerColorEnemy;
	else
		return playerColorNeutral;
}

int CScenarioTracker::CalcScore( int nUnitsLost, int nUnitsKilled, int nLostSum, int nKilledSum, int nReinfCalled ) const
{
	// variant 1
//	int nScore = max( 0, (((nUnitsKilled * 2) / (nUnitsLost + 1) * 5) + - nReinfCalled * 10) * 10 );

	// variant 2
	int nScore = ((nKilledSum / (nLostSum + 1)) * (nUnitsKilled / (nReinfCalled + 1))) * 3;

	return nScore;
}

void CScenarioTracker::UpdateStatistics( bool bWin )
{
	int nLocalPlayer = GetLocalPlayer();
	
	// ESK_CAMPAIGN_TIME
	if ( bWin )
	{
		int nCampaignTime = GetStatistics( nLocalPlayer, ESK_CAMPAIGN_TIME );
		int nMissionTime = GetStatistics( nLocalPlayer, ESK_TIME );
		SetStatistics( nLocalPlayer, ESK_CAMPAIGN_TIME, nCampaignTime + nMissionTime );
	}

	// ESK_REINFORCEMENTS_CALLED
	{
		SetStatistics( nLocalPlayer, ESK_REINFORCEMENTS_CALLED, nReinforcementCallsUsed );
	}

	// ESK_EXP_EARNED
	{
		SetStatistics( nLocalPlayer, ESK_EXP_EARNED, fPlayerXPAdds );
	}

	// ESK_CAMPAIGN_EXP_CURRENT
	if ( bWin )
	{
		int nExp = fPlayerXP + fPlayerXPAdds;
		int nIndex = GetPlayerRankIndex( nExp );
		if ( nIndex >= 0 && nIndex < pCampaign->rankExperiences.size() )
			nExp -= pCampaign->rankExperiences[nIndex].fExperience;
		
		SetStatistics( nLocalPlayer, ESK_CAMPAIGN_EXP_CURRENT, nExp );
	}

	// ESK_CAMPAIGN_EXP_NEXT_LEVEL
	if ( bWin )
	{
		int nExp = 0;
		int nIndex = GetPlayerRankIndex( fPlayerXP + fPlayerXPAdds );
		if ( nIndex >= 0 && nIndex + 1 < pCampaign->rankExperiences.size() )
			nExp = pCampaign->rankExperiences[nIndex + 1].fExperience - pCampaign->rankExperiences[nIndex].fExperience;

		SetStatistics( nLocalPlayer, ESK_CAMPAIGN_EXP_NEXT_LEVEL, nExp );
	}
	
	// ESK_KEY_BUILDINGS_CAPTURED - not presents at single
	{
	}

	// ESK_ENEMY_UNITS_MAX_PRICE
	{
		int nExp = CalcPossibleEnemyExp( nLocalPlayer );
		SetStatistics( nLocalPlayer, ESK_ENEMY_UNITS_MAX_PRICE, nExp );
	}
	
	// ESK_CAMPAIGN_MISSIONS_PASSED
	if ( !IsCustomMission() && bWin )
	{
		int nMissionsPassed = GetStatistics( nLocalPlayer, ESK_CAMPAIGN_MISSIONS_PASSED );
		SetStatistics( nLocalPlayer, ESK_CAMPAIGN_MISSIONS_PASSED, nMissionsPassed + 1 );
	}

	for ( int nPlayer = 0; nPlayer < GetNPlayers(); ++nPlayer )
	{
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

		// ESK_SCORE
		{
			int nUnitsLost = GetStatistics( nPlayer, ESK_UNITS_LOST );
			int nUnitsKilled = GetStatistics( nPlayer, ESK_UNITS_KILLED );
			int nLostSum = GetStatistics( nPlayer, ESK_UNITS_LOST_PRICE );
			int nKilledSum = GetStatistics( nPlayer, ESK_UNITS_KILLED_PRICE );
			int nReinfCalled = GetStatistics( nPlayer, ESK_REINFORCEMENTS_CALLED );
			int nScore = CalcScore( nUnitsLost, nUnitsKilled, nLostSum, nKilledSum, nReinfCalled );

			SetStatistics( nPlayer, ESK_SCORE, nScore );
		}

		// ESK_CAMPAIGN_UNITS_LOST
		// ESK_CAMPAIGN_UNITS_KILLED
		if ( !IsCustomMission() && bWin && nPlayer == nLocalPlayer )
		{
			int nUnitsKilled = GetStatistics( nPlayer, ESK_UNITS_KILLED );
			int nUnitsLost = GetStatistics( nPlayer, ESK_UNITS_LOST );
			int nCampaignUnitsKilled = GetStatistics( nPlayer, ESK_CAMPAIGN_UNITS_KILLED );
			int nCampaignUnitsLost = GetStatistics( nPlayer, ESK_CAMPAIGN_UNITS_LOST );
			SetStatistics( nPlayer, ESK_CAMPAIGN_UNITS_KILLED, nCampaignUnitsKilled + nUnitsKilled );
			SetStatistics( nPlayer, ESK_CAMPAIGN_UNITS_LOST, nCampaignUnitsLost + nUnitsLost );
		}
	}
}

const NDb::SPartyDependentInfo *CScenarioTracker::GetPlayerParty( const int nPlayer ) 
{ 
	CDBPtr<NDb::SMapInfo> pMapInfo;
	if ( IsMissionActive() )
		pMapInfo = GetCurrentMission();
	else
		pMapInfo = GetLastMission();

	if ( !pMapInfo )
		return 0;

	NI_VERIFY( nPlayer >= 0 && nPlayer < pMapInfo->players.size(), StrFmt( "PRG: Player No %d not in bounds (max %d)", nPlayer, pMapInfo->players.size() ), return 0 );

	return pMapInfo->players[nPlayer].pPartyInfo; 
}

const IScenarioTracker::SLeaderInfo *CScenarioTracker::GetLeaderInfo( const NDb::EReinforcementType eReinf ) const
{
	CLeaderList::const_iterator it = leaders.find( eReinf );
	if ( it == leaders.end() )
		return 0;

	return &(it->second);
}

void CScenarioTracker::AutoGenerateLeaderInfo( SGenerateLeaderInfo *pInfo ) const
{
	if ( !freeLeaders.empty() )
	{
		pInfo->nID = NWin32Random::Random( 0, freeLeaders.size() - 1 );
		const NDb::SCampaign::SLeader &dbLeader = pCampaign->leaders[freeLeaders[pInfo->nID]];
		pInfo->wszName = GET_TEXT_PRE( dbLeader., Name );
		pInfo->pPicture = dbLeader.pPicture;
	}
	else
	{
		pInfo->nID = -1;
		if ( !pCampaign->leaders.empty() )
		{
			const NDb::SCampaign::SLeader &dbLeader = pCampaign->leaders.front();
			pInfo->wszName = GET_TEXT_PRE( dbLeader., Name );
			pInfo->pPicture = dbLeader.pPicture;
		}
	}
}

bool CScenarioTracker::AssignLeader( const NDb::EReinforcementType eReinf, const SGenerateLeaderInfo &info, SUndoLeaderInfo *pUndo )
{
	if ( !IsCampaignActive() )
		return false;
	NI_VERIFY( nAvailablePromotions > 0, "No available promotions to assign a leader", return false );

	--nAvailablePromotions;
	if ( info.nID >= 0 )
	{
		if ( pUndo )
		{
			pUndo->eReinf = eReinf;
			pUndo->nValue = freeLeaders[info.nID];
		}
		freeLeaders.erase( freeLeaders.begin() + info.nID );
	}
	
	leaders[eReinf].wszName = info.wszFullName;
	leaders[eReinf].pPicture = info.pPicture;
	leaders[eReinf].info.fExp = 0.0f;
	leaders[eReinf].info.nRank = 0;
	leaders[eReinf].info.nUnitsKilled = 0;
	leaders[eReinf].info.nUnitsLost = 0;
	leaders[eReinf].storedInfo = leaders[eReinf].info;

	return true;
}

void CScenarioTracker::SetLeaderLastSeenInfo( const NDb::EReinforcementType eReinf, 
	const SLeaderInfo::SLeaderStatSet &lastSeenInfo )
{
	CLeaderList::iterator it = leaders.find( eReinf );
	if ( it != leaders.end() )
	{
		SLeaderInfo &leader = it->second;
		leader.lastSeenInfo = lastSeenInfo;
	}
}

float CScenarioTracker::GetLeaderRankExp( NDb::EReinforcementType eType, int nLevel ) const
{
	return GetReinforcementXPForLevel( eType, nLevel );
}

void CScenarioTracker::UndoAssignLeader( const SUndoLeaderInfo &undo )
{
	if ( !IsCampaignActive() )
		return;

	CLeaderList::iterator it = leaders.find( undo.eReinf );
	if ( it != leaders.end() )
	{
		++nAvailablePromotions;
		if ( undo.nValue >= 0 )
			freeLeaders.push_back( undo.nValue );
			
		leaders.erase( it );
	}
}

int CScenarioTracker::GetLeaderLevel( int nPlayer, NDb::EReinforcementType eType )
{
	if ( eType == NDb::RT_ENGINEERING || eType == NDb::RT_RECON || eType == NDb::RT_SUPER_WEAPON )
	{
		return N_MAX_XP_LEVEL;
	}

	if ( nPlayer != GetLocalPlayer() )
		return -1;

	if ( !pCampaign || !pChapter )
	{
#ifndef _FINALRELEASE
		if ( NGlobal::GetVar( "debug_unit_levels", 0 ) != 0 )
			return N_MAX_XP_LEVEL;
#endif _FINALRELEASE
		return -1;
	}

	CLeaderList::const_iterator it = leaders.find( eType );
	if ( it == leaders.end() )
		return -1;

	return (it->second).info.nRank;
}

const NDb::SUnitStatsModifier *CScenarioTracker::GetLeaderModifier( int nPlayer, NDb::EReinforcementType eType )
{
	if ( !IsCampaignActive() || nPlayer != GetLocalPlayer() )
		return 0;

	CLeaderList::const_iterator it = leaders.find( eType );
	if ( it == leaders.end() )
		return 0;

	NI_ASSERT( 0 <= it->second.info.nRank && it->second.info.nRank < pCampaign->leaderRanks.size(), StrFmt( "Rank index (%d) out of range (0..%d)", it->second.info.nRank, pCampaign->leaderRanks.size() ) );
	const int nRank = Clamp( it->second.info.nRank, 0, pCampaign->leaderRanks.size()-1 );
	return pCampaign->leaderRanks[nRank].pStatsBonus;
}

const wstring& CScenarioTracker::GetLeaderRankName( int nRank ) const
{
	if ( pCampaign )
	{
		if ( 0 <= nRank && nRank < pCampaign->leaderRanks.size() )
		{
			const NDb::SLeaderExpLevel &level = pCampaign->leaderRanks[nRank];
			if ( CHECK_TEXT_NOT_EMPTY_PRE(level.,RankName) )
				return GET_TEXT_PRE(level.,RankName);
		}
	}
	
	static wstring wszEmpty;
	return wszEmpty;
}

int CScenarioTracker::GetAvailablePromotions() const
{
	return nAvailablePromotions;
}

void CScenarioTracker::SetAvailablePromotions( int nCount )
{
	nAvailablePromotions = nCount;
}

bool CScenarioTracker::GiveXP( const int nPlayer, NDb::EReinforcementType eReinf, const int nXP )
{
	if ( nPlayer == 0 )
	{
		bool bResult = false;
		float fXP = nXP;
		if ( AddReinfExp( &fXP, eReinf ) )
			bResult = true;

		fXP = nXP;
		if ( IsCampaignActive() )
		{
			CLeaderList::iterator it = leaders.find( eReinf );
			if ( it != leaders.end() )
			{
				SLeaderInfo &leader = it->second;
				if ( AddLeaderExp( &fXP, &leader, eReinf ) )
					bResult = true;
			}
		}

		return bResult;
	}

	return false;
}

bool CScenarioTracker::GiveXPToPlayer( const int nPlayer, const int nXP )
{
	if ( nPlayer != 0 )
		return false;

	fPlayerXPAdds = Max( 0.0f, fPlayerXPAdds + nXP );

	return false;
}

void CScenarioTracker::GetAllMissionStats( vector<const SMissionStats*> *pMissions ) const
{
	if ( !pMissions )
		return;
	pMissions->resize( 0 );
	pMissions->reserve( missionsStats.size() );
	for ( CMissionsStats::const_iterator it = missionsStats.begin(); it != missionsStats.end(); ++it )
	{
		const SMissionStats &mission = it->second;
		pMissions->push_back( &mission );
	}
}

NDb::EReinforcementType CScenarioTracker::GetFavoriteReinf() const
{
	int nMax = 0;
	NDb::EReinforcementType eType = NDb::_RT_NONE;
	for ( int i = 0; i < favoriteReinfs.size(); ++i )
	{	
		const SFavoriteReinf &reinf = favoriteReinfs[i];
		if ( reinf.nCurrentCount > nMax )
		{
			nMax = reinf.nCurrentCount;
			eType = (NDb::EReinforcementType)( i );
		}
	}
	return eType;
}

void CScenarioTracker::MarkFavoriteReinf( NDb::EReinforcementType eType )
{
	if ( eType < 0 || eType >= favoriteReinfs.size() )
		return;
	favoriteReinfs[eType].nCurrentCount++;
}

void CScenarioTracker::SetLastVisiblePlayerStatsExp( float fCareer, float fNextRank )
{
	fLastVisiblePlayerStatsExpCareer = fCareer;
	fLastVisiblePlayerStatsExpNextRank = fNextRank;
}

void CScenarioTracker::GetLastVisiblePlayerStatsExp( float *pCareer, float *pNextRank ) const
{
	*pCareer = fLastVisiblePlayerStatsExpCareer;
	*pNextRank = fLastVisiblePlayerStatsExpNextRank;
}

wstring CScenarioTracker::GetReinfName( NDb::EReinforcementType eType ) const
{
	wstring wszReinf;
	if ( eType == NDb::_RT_NONE )
		return wszReinf;

	for ( int i = 0; i < chapterCurrentReinfs.size(); ++i )
	{
		const SChapterReinf &reinf = chapterCurrentReinfs[i];
		if ( reinf.pDBReinf && reinf.pDBReinf->eType == eType )
		{
			if ( CHECK_TEXT_NOT_EMPTY_PRE(reinf.pDBReinf->,LocalizedName) )
				wszReinf = GET_TEXT_PRE(reinf.pDBReinf->,LocalizedName);
			break;
		}
	}
	
	return wszReinf;
}

const NDb::SUnitStatsModifier* CScenarioTracker::GetPlayerChapterModifier( NDb::EReinforcementType eReinf )
{
	if ( !pChapter )
		return 0;

	return ( pChapter->general.eReinforcementType == eReinf) ? pChapter->general.pStatBonus : 0;
}

}

static void AddPromotions( const string &szID, const vector<wstring> &paramsSet, void *pContext )
{
#ifndef _FINALRELEASE
	if ( !paramsSet.empty() ) 
	{
		int nCount = NStr::ToInt( NStr::ToMBCS( paramsSet[0] ) );
		Singleton<IScenarioTracker>()->SetAvailablePromotions( 
			Singleton<IScenarioTracker>()->GetAvailablePromotions() + nCount );
	}
#endif //_FINALRELEASE
}

START_REGISTER(ScenarioTrackerCommands)
REGISTER_CMD( "add_promotions", AddPromotions )
REGISTER_VAR_EX( "objective_static_pointer_offset", NGlobal::VarFloatHandler, &s_fStaticPointerOffset, 5.0f, STORAGE_NONE );
FINISH_REGISTER

REGISTER_SAVELOAD_CLASS_NM( 0x11165340, CScenarioTracker, NScenarioTracker );
BASIC_REGISTER_CLASS( IScenarioTracker )
BASIC_REGISTER_CLASS( IAIScenarioTracker )
using namespace NScenarioTracker;
REGISTER_SAVELOAD_CLASS_NM( 0x33234C40, SLeaderInfo, IScenarioTracker )
