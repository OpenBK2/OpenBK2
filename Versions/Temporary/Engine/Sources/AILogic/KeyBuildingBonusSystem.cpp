#include "stdafx.h"
#include "./keybuildingbonussystem.h"
#include "Diplomacy.h"
#include "PlayerReinforcement.h"
#include "NewUpdater.h"
#include "LinkObject.h"
#include "ScenarioTracker.h"

#include <fmt/format.h>

extern CDiplomacy theDipl;
extern CEventUpdater updater;

CKeyBuildingBonusSystem theBonusSystem;
extern CPlayerReinforcementArray theReinfArray;

void CKeyBuildingBonusSystem::InitBonusSystem( const NDb::SMapInfo * pMapInfo )
{
	for ( int i = 0; i < pMapInfo->playerBonusObjects.size(); ++i )
		buildingBonuses[pMapInfo->playerBonusObjects[i].nLinkID] = pMapInfo->playerBonusObjects[i];
}

bool CKeyBuildingBonusSystem::IsStorage( int nLinkID ) const
{
	CBuildingBonuses::const_iterator pos = buildingBonuses.find( nLinkID );
	return pos != buildingBonuses.end() && pos->second.bStorage; 
}

bool CKeyBuildingBonusSystem::IsKeyBuilding( int nLinkID ) const
{
	CBuildingBonuses::const_iterator pos = buildingBonuses.find( nLinkID );
	return pos != buildingBonuses.end(); 
}

void CKeyBuildingBonusSystem::SendUpdates() const 
{ 
	if ( !GetScenarioTracker() )
		return;

	for ( CBuildingBonuses::const_iterator it = buildingBonuses.begin(); it != buildingBonuses.end(); ++it  )
	{
		CLinkObject * pBuilding = CLinkObject::GetObjectByLink( it->first );
		if ( pBuilding )
		{
			SAIKeyBuildingUpdate *pUpdate = new SAIKeyBuildingUpdate;
			pUpdate->info.nObjUniqueID = pBuilding->GetUniqueId();
			pUpdate->info.nPrevPlayer = -1;
			pUpdate->info.nPlayer = pBuilding->GetPlayer();
			pUpdate->info.bStorage = IsStorage( pBuilding->GetLink() );
			pUpdate->info.bFriendLost = false;
			updater.AddUpdate( pUpdate, ACTION_NOTIFY_NEW_KEY_BUILDING, pBuilding, pBuilding->GetPlayer() );

			GetScenarioTracker()->KeyBuildingOwnerChange( pBuilding->GetLink(), pBuilding->GetPlayer() );
		}
	}
}

void CKeyBuildingBonusSystem::ChangeOwnership( int _nOldPlayer, int _nNewPlayer, int nLinkID, bool bDuringMapLoad )
{
	if ( !GetScenarioTracker() )
		return;

	CBuildingBonuses::iterator pos = buildingBonuses.find( nLinkID );
	if ( pos == buildingBonuses.end() )
		return;

	const int nOldParty = theDipl.GetNParty( _nOldPlayer );
	const int nNewParty = theDipl.GetNParty( _nNewPlayer );

	// diplomacy didn't change
	if ( nOldParty == nNewParty )
		return;
	GetScenarioTracker()->KeyBuildingOwnerChange( nLinkID, _nNewPlayer );

	for ( int i = 0; i < theDipl.GetNPlayers(); ++i )
	{
		// take reinforcement point from nOldPlayer
		if ( theDipl.GetNParty( i ) == nOldParty )
		{
			if ( GetScenarioTracker()->GetGameType() != IAIScenarioTracker::EGT_SINGLE )
				theReinfArray.SetRecycleCoeff( i, GetScenarioTracker()->GetRecycleSpeedCoeff( nOldParty ) );

			if ( i != theDipl.GetNeutralPlayer() )
			{
				if ( !bDuringMapLoad )
					theReinfArray.GiveReinforcementCalls( i, -1, false );

				if ( pos->second.playerBonuses.size() > i )
				{
					const int nPositionID = pos->second.playerBonuses[i].nPointID;
					const auto message = fmt::format( "Reinforcement point {} taken from player {}", nPositionID, i );
					CONSOLE_BUFFER_LOG( CONSOLE_STREAM_CONSOLE, message.c_str() );
					theReinfArray[i].EnablePosition( nPositionID, false );
				}
			}
		}
		// give reinforcement point to nNewPlayer
		if ( theDipl.GetNParty( i ) == nNewParty )
		{
			if ( GetScenarioTracker()->GetGameType() != IAIScenarioTracker::EGT_SINGLE )
				theReinfArray.SetRecycleCoeff( i, GetScenarioTracker()->GetRecycleSpeedCoeff( nNewParty ) );

			if ( i != theDipl.GetNeutralPlayer() )
			{
				if ( !bDuringMapLoad )
					theReinfArray.GiveReinforcementCalls( i, 1, true );

				if ( pos->second.playerBonuses.size() > i )
				{
					const int nPositionID = pos->second.playerBonuses[i].nPointID;
					theReinfArray[i].EnablePosition( nPositionID, true );
					const auto message =  fmt::format( "New reinforcement point {} given to player {}", nPositionID, i );
					CONSOLE_BUFFER_LOG( CONSOLE_STREAM_CONSOLE, message.c_str() );
				}
			}
		}
	}
}

