#include "stdafx.h"

#include "HitProtection.h"
#include "System/RandomGen.h"

#include <algorithm>

namespace
{
bool CracksProtection( const float fBase, const float fRandom, const float fStrengthMin, const float fStrengthMax )
{
	const float fVariation = (std::max)( 0.0f, fRandom );
	const float fMaximumStrength = (std::max)( fStrengthMin, fStrengthMax );
	// Certain outcomes need no draws. In particular, default 0 crack versus 1 strength
	// preserves the legacy simulation's RNG sequence as well as its hit decisions.
	if ( fBase + fVariation < fStrengthMin )
	{
		return false;
	}
	if ( fBase - fVariation >= fMaximumStrength )
	{
		return true;
	}

	// Sequence strength before crack explicitly and retain fractional values.
	// RandomCheck(float, float) returns int in this engine, so use Random instead.
	float fStrength = fStrengthMin;
	if ( fMaximumStrength > fStrengthMin )
	{
		RecordRandomCall();
		fStrength = NRandom::Random( fStrengthMin, fMaximumStrength );
	}
	float fCrack = fBase;
	if ( fVariation > 0.0f )
	{
		RecordRandomCall();
		fCrack += NRandom::Random( -fVariation, fVariation );
	}
	return fCrack >= fStrength;
}
} // namespace

namespace NHitProtection
{
bool CracksAABB( const NDb::SWeaponRPGStats::SShell &shell, const NDb::SUnitBaseRPGStats &target )
{
	return CracksProtection(
			shell.fAABBCrackBase, shell.fAABBCrackRandom, target.fAABBStrengthMin, target.fAABBStrengthMax );
}

bool CracksCover( const NDb::SWeaponRPGStats::SShell &shell, const NDb::SUnitBaseRPGStats &target )
{
	return CracksProtection(
			shell.fCoverCrackBase, shell.fCoverCrackRandom, target.fCoverStrengthMin, target.fCoverStrengthMax );
}
} // namespace NHitProtection
