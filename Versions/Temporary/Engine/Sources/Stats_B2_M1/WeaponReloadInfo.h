#pragma once

#include "System/BinSaver.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

// Presentation snapshot for one physical gun, independent of its ammunition alternatives.
struct SWeaponReloadInfo
{
	uint32_t nStartTime = 0;
	float fDuration = 0.0f;

	int operator&( IBinSaver &saver )
	{
		saver.Add( 2, &nStartTime );
		saver.Add( 3, &fDuration );
		return 0;
	}

	float GetRemainingFraction( uint32_t nTime ) const
	{
		if ( !std::isfinite( fDuration ) || fDuration <= 0.0f )
		{
			return 0.0f;
		}
		// Signed subtraction also handles a client frame just before the simulation's shot timestamp.
		const float fElapsed = (std::max)( 0.0f, float( int32_t( nTime - nStartTime ) ) );
		return (std::max)( 0.0f, 1.0f - fElapsed / fDuration );
	}
};
