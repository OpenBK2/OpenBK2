#pragma once

#include <cstdint>
#include <vector>

// Player group numbers encode the sender in bits 1..4. Resolve ownership when the synchronized
// command executes, since a transfer may have occurred after the sender selected these IDs.
template <class TUnit, class TResolve>
std::vector<TUnit *> ResolvePlayerCommandGroup( const std::vector<int> &unitIDs, uint16_t wGroup, TResolve resolve )
{
	const int nPlayer = ( wGroup >> 1 ) & 0xf;
	std::vector<TUnit *> result;
	for ( int nID : unitIDs )
	{
		TUnit *pUnit = resolve( nID );
		if ( !pUnit || !pUnit->IsAlive() || pUnit->GetPlayer() != nPlayer )
		{
			continue;
		}
		// Group registration promotes selected soldiers to their formation.
		TUnit *pGroupUnit = pUnit->IsInFormation() ? pUnit->GetFormation() : pUnit;
		if ( pGroupUnit && pGroupUnit->IsAlive() && pGroupUnit->GetPlayer() == nPlayer )
		{
			result.push_back( pUnit );
		}
	}
	return result;
}
