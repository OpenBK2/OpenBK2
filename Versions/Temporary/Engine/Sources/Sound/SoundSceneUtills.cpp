#include "stdafx.h"
#include "SoundSceneUtills.h"
#include "Sound.h"


size_t SIntThreeHash::operator() ( const struct SIntThree &v ) const 
{ 
	return v.z * RAND_MAX + ((v.x * RAND_MAX )>>2) +  v.y; 
}

bool CSoundStartTimePredicate::operator()( class CSound* one, class CSound *two ) const
{ 
	return one->GetBeginTime() < two->GetBeginTime();
};

bool CSoundsWithinDeltaPredicate::operator()( const CSound * sound ) const
{
	return sound->GetBeginTime() >= timeToCompare;
}

