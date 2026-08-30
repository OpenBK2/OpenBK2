#include "stdafx.h"

#include "Win32Random.h"

namespace NWin32Random
{

static int s_holdrand = 0;

void Seed( const int nSeed )
{
	s_holdrand = nSeed;
}
int GetSeed()
{
	return s_holdrand;
}

unsigned int Random()
{
	// masked with the same constant the float overload scales by, so the two
	// cannot drift apart
	return ( ((s_holdrand = s_holdrand * 214013L + 2531011L) >> 16) & N_RANDOM_MAX );
}

static SRandomFunc rndFunc;
const SRandomFunc& RndFunc()
{
	return rndFunc;
}

};


