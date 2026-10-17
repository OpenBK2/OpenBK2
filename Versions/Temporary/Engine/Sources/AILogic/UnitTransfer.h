#pragma once

#include "ScenarioTracker.h"

#include <set>
#include <vector>

// Read only synchronized match state. Check slots before looking up sides, including spectator IDs.
template <class TScenarioTracker>
bool CanTransferUnitsToPlayer( const TScenarioTracker *pTracker, int nSourcePlayer, int nTargetPlayer )
{
	if ( !pTracker || pTracker->GetGameType() == IAIScenarioTracker::EGT_SINGLE || !pTracker->IsMissionActive() ||
	     nSourcePlayer < 0 || nTargetPlayer < 0 || nSourcePlayer >= pTracker->GetNPlayers() ||
	     nTargetPlayer >= pTracker->GetNPlayers() || nSourcePlayer == nTargetPlayer ||
	     !pTracker->IsPlayerPresent( nSourcePlayer ) || !pTracker->IsPlayerPresent( nTargetPlayer ) )
	{
		return false;
	}
	const int nSide = pTracker->GetPlayerSide( nSourcePlayer );
	return nSide >= 0 && nSide < 2 && pTracker->GetPlayerSide( nTargetPlayer ) == nSide;
}

// Inspect validates ownership/liveness and returns attached unit IDs. Resolve each connected group before
// changing owners, so a transport cannot accidentally donate another player's passengers or split a crew.
template <class TInspect>
std::vector<int> CollectUnitTransfer( const std::vector<int> &selectedIDs, TInspect inspect )
{
	std::set<int> result;
	const std::set<int> roots( selectedIDs.begin(), selectedIDs.end() );
	for ( int nRoot : roots )
	{
		if ( result.count( nRoot ) != 0 )
		{
			continue;
		}
		std::set<int> pending{ nRoot };
		std::set<int> component;
		bool bValid = true;
		while ( !pending.empty() )
		{
			const int nID = *pending.begin();
			pending.erase( pending.begin() );
			if ( !component.insert( nID ).second )
			{
				continue;
			}
			std::vector<int> related;
			if ( !inspect( nID, &related ) )
			{
				bValid = false;
				break;
			}
			pending.insert( related.begin(), related.end() );
		}
		if ( bValid )
		{
			result.insert( component.begin(), component.end() );
		}
	}
	// Unique IDs establish one execution order regardless of selection and attachment traversal order.
	return std::vector<int>( result.begin(), result.end() );
}
