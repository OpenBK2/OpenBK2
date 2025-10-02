#include "stdafx.h"

#include "Statistics.h"
#include "ScenarioTracker.h"
#include "NewUpdater.h"
#include "AILogicInternal.h"
#include "UnitsIterators.h"
#include "AIUnit.h"
#include "DBAIConsts.h"

extern CEventUpdater updater;
extern CDiplomacy theDipl;
CStatistics theStatistics;

void CStatistics::Init()
{
}

int CStatistics::GetUnitLevel( const NDb::EReinforcementType eType, const int nPlayer ) const
{
	if ( !GetScenarioTracker() )
		return 0;

	IAIScenarioTracker *pScenarioTracker = GetScenarioTracker();
	if ( pScenarioTracker )
		return pScenarioTracker->GetReinforcementXPLevel( nPlayer, eType );
	else
		return 0;
}

float CStatistics::GetValue( const int nValue, const int nPlayer ) const
{
	/*
	IPlayerScenarioInfo * pPlayer = pScenarioTracker->GetPlayer( nPlayer );
	if ( pPlayer )
		return pPlayer->GetMissionStats()->GetValue( (const EScenarioTrackerMissionTypes)nValue );
		*/
	return 0;
}

void CStatistics::UnitCaptured( const int nPlayer )
{
	//if ( pScenarioTracker )
		//pScenarioTracker->GetPlayer( nPlayer )->GetMissionStats()->AddValue( STMT_ENEMY_MACHINERY_CAPTURED, 1.0f );
}

void CStatistics::UnitKilled( const int nPlayer, const int nKilledUnitsPlayer, const float fTotalAIPrice,
															const EReinforcementType eKillerType, const EReinforcementType eDeadType, const bool bInfantry )
{
	if ( !GetScenarioTracker() )
		return;

	IAIScenarioTracker *pScenarioTracker = GetScenarioTracker();

	const int nOldLevel = pScenarioTracker->GetReinforcementXPLevel( nPlayer, eKillerType );
	const int nOldLeaderLevel = pScenarioTracker->GetLeaderLevel( nPlayer, eKillerType );

	const NDb::SUnitStatsModifier *pOldLeaderBonus = pScenarioTracker->GetLeaderModifier( nPlayer, eKillerType );
	IAIScenarioTracker::SKillInfo info;
	info.nPlayer = nPlayer;
	// NDb::EDBUnitRPGType eUnitType;
	info.eReinfType = eKillerType;
	info.nKilledUnitPlayer = nKilledUnitsPlayer;
	//NDb::EDBUnitRPGType eKilledUnitType;
	info.eKilledReinfType = eDeadType;
	info.fExpPrice = fTotalAIPrice;
	info.bInfantryKill = bInfantry;

	if ( nPlayer == theDipl.GetMyNumber() )
		updater.AddUpdate( EFB_GAIN_EXP, eKillerType, 0 );

	if ( pScenarioTracker->RegisterUnitKill( info ) )
	{
		// Level up
		CDBPtr<NDb::SUnitStatsModifier> pOldBonus, pNewBonus;
		const int nNewLevel = pScenarioTracker->GetReinforcementXPLevel( nPlayer, eKillerType );
		const int nNewLeaderLevel = pScenarioTracker->GetLeaderLevel( nPlayer, eKillerType );
		const int nOldAbilityLevel = ( nOldLeaderLevel < 0 ) ? 0 : Min ( nOldLevel, nOldLeaderLevel );
		// Issue bonus
		const NDb::SAIGameConsts *pConsts = dynamic_cast<CAILogic*>(Singleton<IAILogic>())->GetAIConsts();
		for ( int i = 0; i < pConsts->common.expLevels.size(); ++i )
		{
			const NDb::SAIExpLevel *pLevels = pConsts->common.expLevels[i];
			if ( !pLevels )
				continue;

			if ( pLevels->eDBType == eKillerType )
			{
				int nXPLevel = Min( pLevels->levels.size() - 1, nOldLevel );
				if ( nXPLevel >= 0 )
					pOldBonus = pLevels->levels[nXPLevel].pStatsBonus;

				nXPLevel = Min( pLevels->levels.size() - 1, nNewLevel );
				if ( nXPLevel >= 0 )
					pNewBonus = pLevels->levels[nXPLevel].pStatsBonus;

				for ( CGlobalIter iter( theDipl.GetNParty( nPlayer ), EDI_FRIEND ); !iter.IsFinished(); iter.Iterate() )
				{
					CAIUnit *pUnit = dynamic_cast<CAIUnit*>( *iter );
					if ( pUnit && pUnit->GetPlayer() == nPlayer && pUnit->GetReinforcementType() == eKillerType )
					{
						if ( nOldLevel != nNewLevel )				// Only if the own level changed. Don't bother if only leader's level changed
						{
							if ( pOldBonus )
								pUnit->ApplyStatsModifier( pOldBonus, false );
							if ( pNewBonus )
								pUnit->ApplyStatsModifier( pNewBonus, true );
						}
						if ( nOldLeaderLevel != nNewLeaderLevel )				// Only if the leader's level changed. Don't bother if only own level changed
						{
							const NDb::SUnitStatsModifier *pNewLeaderBonus = pScenarioTracker->GetLeaderModifier( nPlayer, eKillerType );
							pUnit->ApplyStatsModifier( pOldLeaderBonus, false );
							pUnit->ApplyStatsModifier( pNewLeaderBonus, true );
						}
						pUnit->InitSpecialAbilities( nOldAbilityLevel + 1 );
					}
				}

				break;
			}
		}

		// Send update if levelled up
		if ( pScenarioTracker->GetLeaderLevel( nPlayer, eKillerType ) != -1 && ( nPlayer == 0 || theDipl.IsNetGame() ) &&
					nOldLeaderLevel != nNewLeaderLevel )
		{
			CPtr<SFeedBackUnitsArray> pParam = new SFeedBackUnitsArray;
			for ( CGlobalIter iter( theDipl.GetNParty( nPlayer ), EDI_FRIEND ); !iter.IsFinished(); iter.Iterate() )
				if ( (*iter)->GetPlayer() == nPlayer && (*iter)->GetReinforcementType() == eKillerType ) 
					pParam->unitIDs.push_back( (*iter)->GetUniqueID() );

			if ( nPlayer == theDipl.GetMyNumber() )
				updater.AddUpdate( EFB_COMMANDER_LEVELUP, eKillerType, pParam );
		}
	}
}

const int CStatistics::GetXPLevel( const int nPlayer, const NDb::EReinforcementType eType )
{
	if ( !GetScenarioTracker() )
		return 0;

	//return 4;		//for debug: all abilities

	IAIScenarioTracker *pScenarioTracker = GetScenarioTracker();
	if ( pScenarioTracker )
		return pScenarioTracker->GetReinforcementXPLevel( nPlayer, eType );

	return 0;
}

const int CStatistics::GetAbilityLevel( const int nPlayer, const NDb::EReinforcementType eType )
{
	IAIScenarioTracker *pScenarioTracker = GetScenarioTracker();

	if ( !pScenarioTracker )
		return 0;

  const int nXPLevel = pScenarioTracker->GetReinforcementXPLevel( nPlayer, eType );
	const int nLeaderLevel = pScenarioTracker->GetLeaderLevel( nPlayer, eType );

	return ( nLeaderLevel < 0 ) ? 0 : Min ( nXPLevel, nLeaderLevel );
}

void CStatistics::ObjectDestroyed( const int nPlayer )
{
	/*
	if ( pScenarioTracker )
	{
		if ( IMissionStatistics *pPlayerStats = GetPlayerStats( nPlayer ) )
				pPlayerStats->AddValue( STMT_HOUSES_DESTROYED, 1.0f );
	}
	*/
}

void CStatistics::AviationCalled( const int nPlayer )
{
	/*
	if ( pScenarioTracker )
	{
		if ( IMissionStatistics *pPlayerStats = GetPlayerStats( nPlayer ) )
			pPlayerStats->AddValue( STMT_AVIATION_CALLED, 1.0f );
	}
	*/
}

void CStatistics::ReinforcementUsed( const int nPlayer )
{
	/*
	if ( pScenarioTracker ) 
	{
		if ( IMissionStatistics *pPlayerStats = GetPlayerStats( nPlayer ) )
			pPlayerStats->AddValue( STMT_REINFORCEMENT_USED, 1.0f );
	}
	*/
}

void CStatistics::ResourceUsed( const int nPlayer, const float fResources )
{
	/*
	if ( pScenarioTracker )
	{
		if ( IMissionStatistics *pPlayerStats = GetPlayerStats( nPlayer ) )
			pPlayerStats->AddValue( STMT_RESOURCES_USED, fResources );
	}
	*/
}

void CStatistics::UnitDead( CCommonUnit *pUnit )
{
	/*
	if ( pScenarioTracker )
	{
		if ( !pUnit->IsFormation() )
		{
			if ( IMissionStatistics *pPlayerStats = GetPlayerStats( pUnit->GetPlayer() ) )
				pPlayerStats->AddValue( STMT_UNITS_LOST_UNRECOVERABLY, 1.0f );
		}

	}
	*/
}

void CStatistics::IncreasePlayerExperience( const int nPlayer, const NDb::EReinforcementType eType, const float fPrice )
{
/*	IMissionStatistics * pStats = GetMissionStats( nPlayer );
	if ( pStats )
	{
		pStats->AddXP( eType, fPrice );
		pStats->UpdateValue( STMT_PLAYER_EXPERIENCE, fPrice );
	}*/
}

void CStatistics::SetFlagPoints( const int nParty, const float fPoints )
{
	/*
	if ( pScenarioTracker )
	{
		for ( IPlayerScenarioInfoIterator *pIter = pScenarioTracker->CreatePlayerScenarioInfoIterator(); !pIter->IsEnd(); pIter->Next() )
		{
			IPlayerScenarioInfo *pInfo = pIter->Get();
			if ( pInfo->GetDiplomacySide() == nParty )
				pInfo->GetMissionStats()->SetValue( STMT_FLAGPOINTS, fPoints );
		}
	}
	*/
}

void CStatistics::SetCapturedFlags( const int nParty, const int nFlags )
{
	/*
	if ( pScenarioTracker )
	{
		for ( IPlayerScenarioInfoIterator *pIter = pScenarioTracker->CreatePlayerScenarioInfoIterator(); !pIter->IsEnd(); pIter->Next() )
		{
			IPlayerScenarioInfo *pInfo = pIter->Get();
			if ( pInfo->GetDiplomacySide() == nParty )
				pInfo->GetMissionStats()->SetValue( STMT_FLAGS_CAPTURED, nFlags );
		}
	}
	*/
}

int CStatistics::operator&( IBinSaver &saver )
{
	saver.Add( 1, &bEnablePlayerExp );
//SKIP	saver.Add( 4, &playerMisisons );
	return 0;
}


