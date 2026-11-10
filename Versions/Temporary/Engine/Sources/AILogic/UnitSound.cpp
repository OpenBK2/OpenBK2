#include "stdafx.h"

#include "CommonUnit.h"
#include "DBAIConsts.h"
#include "GlobalWarFog.h"
#include "Stats_B2_M1/Vis2AI.h"
#include "UnitSound.h"

#include <algorithm>
#include <cmath>

float GetUnitFOWSoundVolume( const CCommonUnit &unit, const int nParty, const NDb::SAIGameConsts *pConsts )
{
	if ( !pConsts || !std::isfinite( pConsts->fFOWSoundDistance ) || pConsts->fFOWSoundDistance < 0.0f )
	{
		return 1.0f;
	}
	if ( nParty < 0 || nParty > 1 )
	{
		return 0.0f;
	}
	// Keep visible aircraft and other visibility overrides audible, including with a zero distance limit.
	if ( unit.IsVisible( uint8_t( nParty ) ) )
	{
		return 1.0f;
	}
	const float fDistance =
			theWarFog.GetNormalizedDistanceToVisibleTerrain( unit, nParty, pConsts->fFOWSoundDistance * AI_TILE_SIZE );
	if ( fDistance >= 1.0f )
	{
		return 0.0f;
	}
	const float fBase = std::isfinite( pConsts->fFOWSoundMinBasePercentage )
	                        ? std::clamp( pConsts->fFOWSoundMinBasePercentage, 0.0f, 1.0f )
	                        : 0.1f;
	// The audible side of the cutoff starts at fBase; exactly at/beyond it stays silent.
	return fBase + ( 1.0f - fBase ) * ( 1.0f - fDistance );
}
