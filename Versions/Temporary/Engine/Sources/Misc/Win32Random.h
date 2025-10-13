
#pragma once

#include "Misc_export.h"

#include <boost/config.hpp>

namespace NWin32Random
{
	MISC_EXPORT void Seed( const int nSeed );
	MISC_EXPORT int GetSeed();
	MISC_EXPORT unsigned int Random();
	BOOST_FORCEINLINE unsigned int Random( const unsigned int uMax ) { return NWin32Random::Random() % uMax; }
	BOOST_FORCEINLINE int Random( const int nMin, const int nMax ) { return nMin + (int)NWin32Random::Random( (const unsigned int)(nMax - nMin + 1) ); }
	BOOST_FORCEINLINE float Random( const float fMin, const float fMax ) { return fMin + float( float(NWin32Random::Random()) / float(RAND_MAX) * (fMax - fMin) ); }

	BOOST_FORCEINLINE unsigned int RandomCheck( const unsigned int uMax ) { return uMax == 0 ? 0 : NWin32Random::Random( uMax ); }
	BOOST_FORCEINLINE int RandomCheck( const int nMin, const int nMax ) { return nMax < nMin ? nMin : NWin32Random::Random( nMin, nMax ); }
	BOOST_FORCEINLINE float RandomCheck( const float fMin, const float fMax ) { return fMax < fMin ? fMin : NWin32Random::Random( fMin, fMax ); }

	struct SRandomFunc
	{
		float operator()( float fMin, float fMax ) const { return Random( fMin, fMax ); }
	};
	const SRandomFunc& RndFunc();
};


