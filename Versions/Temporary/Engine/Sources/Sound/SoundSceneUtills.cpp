#include "stdafx.h"
#include "SoundSceneUtills.h"
#include "Sound.h"

size_t SIntThreeHash::operator()( const struct SIntThree &v ) const
{
	// Mixed in size_t. The components are ints and RAND_MAX is INT_MAX, so both
	// multiplies overflowed a signed type, and the shift was of a value that can
	// be negative, which is a second thing the standard leaves to the
	// implementation. Converting each component to an unsigned type is defined
	// for negative values and the arithmetic then wraps by definition, which is
	// all a hash wants from it.
	return static_cast<size_t>( v.z ) * RAND_MAX + ( ( static_cast<size_t>( v.x ) * RAND_MAX ) >> 2 ) +
	       static_cast<size_t>( v.y );
}

bool CSoundStartTimePredicate::operator()( class CSound *one, class CSound *two ) const
{
	return one->GetBeginTime() < two->GetBeginTime();
}

bool CSoundsWithinDeltaPredicate::operator()( const CSound *sound ) const
{
	return sound->GetBeginTime() >= timeToCompare;
}
