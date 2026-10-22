#pragma once

// Defensive member defaults; explicit constructor values still take precedence.

#include "Common_RTS_AI_export.h"

#include <cstdint>

// angle (replacememt for uint16_t angle), multiplayer sync
struct COMMON_RTS_AI_EXPORT SAIAngle
{
	// Four bytes wide on purpose, which is all the union here was for: it held
	// the angle and an int named allign that nothing outside this header ever
	// touched, and the comment on it said the alignment was the point. The two
	// arms had different types, so there was no common initial sequence to
	// appeal to and reading the angle while the int was alive, which every
	// constructor arranged, was undefined. The width matters because this type
	// sits inside CBasePathUnit and other simulation state whose layout the
	// multiplayer checksum covers.
	uint16_t wAngle = 0;
	uint16_t wUnused = 0;

	SAIAngle() = default;

	SAIAngle( int _wAngle )
		: wAngle( uint16_t( _wAngle ) )
	{
	}

	operator int() const
	{
		return wAngle;
	}

	const SAIAngle &operator=( int _nAngle )
	{
		wAngle = _nAngle;
		return *this;
	}

	int operator&( IBinSaver &saver )
	{
		// Written and read as the single int the union arm used to be, so
		// saves, replays and network state are byte for byte what they were.
		// The upper half has always been zero: every constructor cleared the
		// whole union before storing the angle, and nothing ever wrote it.
		int nStorage = wAngle;
		saver.Add( 1, &nStorage );
		wAngle = uint16_t( nStorage );
		return 0;
	}

	SAIAngle &operator+=( const SAIAngle &an )
	{
		wAngle += an.wAngle;
		return *this;
	}
};

static_assert( sizeof( SAIAngle ) == 4, "the serialized width and the layout of the simulation state that carries it" );

COMMON_RTS_AI_EXPORT bool operator>( const SAIAngle &_1, const SAIAngle &_2 );
COMMON_RTS_AI_EXPORT bool operator>( int _1, const SAIAngle &_2 );
COMMON_RTS_AI_EXPORT bool operator>( const SAIAngle &_1, int _2 );

COMMON_RTS_AI_EXPORT bool operator<( const SAIAngle &_1, const SAIAngle &_2 );
COMMON_RTS_AI_EXPORT bool operator<( int _1, const SAIAngle &_2 );
COMMON_RTS_AI_EXPORT bool operator<( const SAIAngle &_1, int _2 );
